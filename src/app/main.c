#include "app/ui.h"
#include "cg/render.h"

#include <GL/freeglut.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static PrizeMachine *machine;
static UserInterface ui;
static int last_time, drag_x, drag_y, held_direction;
static bool smoke;
static int drag_button = -1;
static const char *screenshot;
static int exit_code = EXIT_SUCCESS, smoke_phase, smoke_prize, smoke_frames;
static float smoke_wait;
static const char *pending_capture;
static int smoke_menu_phase;

static void reset_camera(void) {
    ui.camera = camera_default();
    ui.free_camera = false;
    drag_button = -1;
}
static void choose_mode(GameMode mode) {
    if (!machine_start(machine, mode)) {
        fprintf(stderr, "Sem memoria para iniciar; partida anterior preservada.\n");
        return;
    }
    ui.menu = ui.help = ui.paused = false;
    reset_camera();
    ui.active_joint = held_direction = 0;
    last_time = glutGet(GLUT_ELAPSED_TIME);
}
static void return_to_modes(void) {
    if (!machine_reset(machine)) {
        fprintf(stderr, "Sem memoria para voltar aos modos; partida pausada preservada.\n");
        return;
    }
    ui.menu = true;
    ui.help = ui.paused = false;
    reset_camera();
    ui.active_joint = 0;
    held_direction = 0;
    last_time = glutGet(GLUT_ELAPSED_TIME);
}
static void toggle_pause(void) {
    ui.paused = !ui.paused;
    ui.help = false;
    held_direction = 0;
    drag_button = -1;
    /* Discard time spent in the overlay, including the interval between timer callbacks. */
    last_time = glutGet(GLUT_ELAPSED_TIME);
}

static void cleanup(void) {
    machine_destroy(machine);
    machine = NULL;
}
static void quit_failure(const char *why) {
    fprintf(stderr, "FAIL graphical smoke: %s\n", why);
    if (machine) {
        fprintf(stderr, "mode %d spawn seed %u\n", machine->mode, (unsigned)machine->spawn_seed);
        for (int p = 0; p < PRIZE_COUNT; ++p) {
            Vec3 pos = mat4_point(machine->prizes[p].node->world, vec3(0, 0, 0));
            Vec3 v = machine->prizes[p].velocity;
            fprintf(stderr, "prize %d state %d position %.3f %.3f %.3f velocity %.3f %.3f %.3f\n",
                    p, machine->prizes[p].state, (double)pos.x, (double)pos.y, (double)pos.z,
                    (double)v.x, (double)v.y, (double)v.z);
        }
    }
    exit_code = EXIT_FAILURE;
    glutLeaveMainLoop();
}
static void reset(void) {
    if (!machine_reset(machine)) {
        fprintf(stderr, "Sem memoria para reiniciar; cena anterior preservada.\n");
        return;
    }
    reset_camera();
    ui.paused = ui.help = false;
    ui.active_joint = 0;
    held_direction = 0;
    last_time = glutGet(GLUT_ELAPSED_TIME);
}
static void key(unsigned char k, int x, int y) {
    (void)x;
    (void)y;
    k = (unsigned char)tolower(k);
    if (ui.menu) {
        if (k >= '1' && k <= '4')
            choose_mode((GameMode)(k - '1'));
        else if (k == 27)
            glutLeaveMainLoop();
        glutPostRedisplay();
        return;
    }
    if (k == 'p' || k == 27) {
        toggle_pause();
        glutPostRedisplay();
        return;
    }
    if (ui.paused) {
        if (k == 'r')
            reset();
        else if (k == 'm')
            return_to_modes();
        glutPostRedisplay();
        return;
    }
    if (ui.help && k != 'h')
        return;
    if (k >= '1' && k <= '4') {
        ui.active_joint = k - '1';
        held_direction = 0;
    }
    switch (k) {
    case '+':
    case '=':
        machine_nudge_joint(machine, ui.active_joint, 5);
        break;
    case '-':
    case '_':
        machine_nudge_joint(machine, ui.active_joint, -5);
        break;
    case '[':
        machine_nudge_joint(machine, ui.active_joint, -1);
        break;
    case ']':
        machine_nudge_joint(machine, ui.active_joint, 1);
        break;
    case 'c':
        machine_close(machine);
        break;
    case 'o':
        machine_open(machine);
        break;
    case ' ':
        if (machine->held >= 0 || machine->arm.opening_target < 20)
            machine_open(machine);
        else
            machine_close(machine);
        break;
    case 'r':
        reset();
        break;
    case 'h':
        ui.help = !ui.help;
        held_direction = 0;
        drag_button = -1;
        break;
    case 'd':
        ui.debug = !ui.debug;
        break;
    case 'f':
        ui.free_camera = !ui.free_camera;
        drag_button = -1;
        break;
    default:
        break;
    }
    glutPostRedisplay();
}
static void special(int k, int x, int y) {
    (void)x;
    (void)y;
    if (ui.menu || ui.paused)
        return;
    if (k == GLUT_KEY_F1) {
        key('h', 0, 0);
        return;
    }
    if (ui.help)
        return;
    if (k == GLUT_KEY_LEFT)
        held_direction = -1;
    if (k == GLUT_KEY_RIGHT)
        held_direction = 1;
    if (k == GLUT_KEY_HOME)
        reset_camera();
}
static void special_up(int k, int x, int y) {
    (void)x;
    (void)y;
    if ((k == GLUT_KEY_LEFT && held_direction < 0) || (k == GLUT_KEY_RIGHT && held_direction > 0)) {
        held_direction = 0;
        /* Releasing a held key stops its remaining target immediately. */
        machine->arm.target[ui.active_joint] = machine->arm.angle[ui.active_joint];
    }
}
static void mouse(int button, int state, int x, int y) {
    if (ui.menu) {
        int mode = ui_mode_at(&ui, x, y);
        if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && mode >= 0)
            choose_mode((GameMode)mode);
        return;
    }
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && y >= 23 && y <= 57 &&
        x >= ui.width - 155 && x <= ui.width - 25) {
        toggle_pause();
        return;
    }
    if (ui.paused) {
        if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
            unsigned char command = ui_pause_at(&ui, x, y);
            if (command)
                key(command, x, y);
        }
        return;
    }
    if (button == GLUT_RIGHT_BUTTON || button == GLUT_MIDDLE_BUTTON) {
        if (state == GLUT_UP && drag_button == button)
            drag_button = -1;
        if (state == GLUT_DOWN && !ui.help && x >= 0 && x < ui_view_width(&ui) && y >= UI_TOP &&
            y < ui.height - UI_BOTTOM)
            drag_button = button;
        drag_x = x;
        drag_y = y;
        return;
    }
    if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN)
        return;
    if (x >= ui_view_width(&ui)) {
        unsigned char command = ui_click(&ui, x, y);
        if (command >= 'a' && command <= 'd') {
            ui.active_joint = command - 'a';
            key('-', x, y);
        } else if (command >= 'e' && command <= 'h') {
            ui.active_joint = command - 'e';
            key('+', x, y);
        } else if (command)
            key(command, x, y);
    } else if (!ui.help && y >= UI_TOP && y < ui.height - UI_BOTTOM) {
        Ray ray;
        if (camera_ray(&ui.camera, (float)x, (float)(y - UI_TOP), ui_view_width(&ui),
                       ui_view_height(&ui), &ray))
            machine_pick(machine, ray);
    }
}
static void motion(int x, int y) {
    if (ui.menu || ui.help || ui.paused)
        return;
    float dx = (float)(x - drag_x), dy = (float)(y - drag_y);
    if (drag_button == GLUT_MIDDLE_BUTTON)
        camera_pan(&ui.camera, dx, dy, ui_view_height(&ui));
    else if (drag_button == GLUT_RIGHT_BUTTON) {
        if (ui.free_camera)
            camera_look(&ui.camera, -dx * .35f, -dy * .25f);
        else
            camera_orbit(&ui.camera, -dx * .35f, dy * .25f);
    }
    drag_x = x;
    drag_y = y;
}
static void wheel(int wheel_number, int direction, int x, int y) {
    (void)wheel_number;
    if (!ui.menu && !ui.help && !ui.paused && x >= 0 && x < ui_view_width(&ui) && y >= UI_TOP &&
        y < ui.height - UI_BOTTOM) {
        if (ui.free_camera)
            camera_dolly(&ui.camera, (float)direction * .6f);
        else
            camera_zoom(&ui.camera, -(float)direction * .6f);
    }
}
static void entry(int state) {
    if (state == GLUT_LEFT) {
        held_direction = 0;
        drag_button = -1;
    }
}
static void reshape(int width, int height) {
    ui.width = width > 0 ? width : 1;
    ui.height = height > 0 ? height : 1;
}
static void update_game(float dt) {
    if (ui.menu || ui.help || ui.paused)
        return;
    if (held_direction)
        machine_target_joint(machine, ui.active_joint,
                             machine->arm.angle[ui.active_joint] +
                                 (float)held_direction *
                                     MACHINE_CONFIG.joints[ui.active_joint].speed * fminf(dt, .1f));
    machine_update(machine, dt);
}
static void smoke_target(int joint, float value) {
    key((unsigned char)('1' + joint), 0, 0);
    int steps = (int)lroundf((value - machine->arm.target[joint]) / 5);
    for (int i = 0; i < abs(steps); ++i)
        key(steps > 0 ? '+' : '-', 0, 0);
    int fine = (int)lroundf(value - machine->arm.target[joint]);
    for (int i = 0; i < abs(fine); ++i)
        key(fine > 0 ? ']' : '[', 0, 0);
}
static bool same_camera(Camera a, Camera b) {
    return v3_length(v3_sub(a.target, b.target)) < .001f && fabsf(a.yaw - b.yaw) < .001f &&
           fabsf(a.pitch - b.pitch) < .001f && fabsf(a.distance - b.distance) < .001f;
}
typedef struct {
    PrizeMachine machine;
    Camera camera;
    int active_joint;
    bool free_camera, debug;
    Mat4 local[PRIZE_COUNT], world[PRIZE_COUNT], arm[ARM_PART_COUNT], tray;
} SmokeSnapshot;
static SmokeSnapshot smoke_pause_snapshot;
static int smoke_pause_frames;
static void snapshot_game(SmokeSnapshot *saved) {
    memcpy(&saved->machine, machine, sizeof(*machine));
    saved->camera = ui.camera;
    saved->active_joint = ui.active_joint;
    saved->free_camera = ui.free_camera;
    saved->debug = ui.debug;
    saved->tray = machine->solids[machine->tray_solid].node->world;
    for (int p = 0; p < PRIZE_COUNT; ++p) {
        saved->local[p] = machine->prizes[p].node->local;
        saved->world[p] = machine->prizes[p].node->world;
    }
    for (int p = 0; p < ARM_PART_COUNT; ++p)
        saved->arm[p] = machine->arm.parts[p]->world;
}
static bool matches_snapshot(const SmokeSnapshot *saved) {
    if (memcmp(&saved->machine, machine, sizeof(*machine)) != 0 ||
        !same_camera(saved->camera, ui.camera) || saved->active_joint != ui.active_joint ||
        saved->free_camera != ui.free_camera || saved->debug != ui.debug ||
        memcmp(&saved->tray, &machine->solids[machine->tray_solid].node->world, sizeof(Mat4)) != 0)
        return false;
    for (int p = 0; p < PRIZE_COUNT; ++p)
        if (memcmp(&saved->local[p], &machine->prizes[p].node->local, sizeof(Mat4)) != 0 ||
            memcmp(&saved->world[p], &machine->prizes[p].node->world, sizeof(Mat4)) != 0)
            return false;
    for (int p = 0; p < ARM_PART_COUNT; ++p)
        if (memcmp(&saved->arm[p], &machine->arm.parts[p]->world, sizeof(Mat4)) != 0)
            return false;
    return true;
}
static void smoke_pause_click(int action) {
    UIRect r = ui_pause_rect(&ui, action);
    mouse(GLUT_LEFT_BUTTON, GLUT_DOWN, (int)(r.x + r.w / 2), (int)(r.y + r.h / 2));
}
static bool smoke_preserve_pause(void) {
    SmokeSnapshot before;
    snapshot_game(&before);
    mouse(GLUT_LEFT_BUTTON, GLUT_DOWN, ui.width - 90, 40);
    key('c', 0, 0);
    key('o', 0, 0);
    key('4', 0, 0);
    key('+', 0, 0);
    key('f', 0, 0);
    key('h', 0, 0);
    key('d', 0, 0);
    special(GLUT_KEY_LEFT, 0, 0);
    special(GLUT_KEY_HOME, 0, 0);
    special(GLUT_KEY_F1, 0, 0);
    mouse(GLUT_LEFT_BUTTON, GLUT_DOWN, ui_view_width(&ui) + 30, 320);
    mouse(GLUT_MIDDLE_BUTTON, GLUT_DOWN, 300, 300);
    motion(400, 360);
    wheel(0, 1, 300, 300);
    update_game(10); /* The same update path as the real timer callback. */
    special_up(GLUT_KEY_LEFT, 0, 0);
    if (!ui.paused || ui.menu || drag_button != -1 || held_direction != 0 ||
        !matches_snapshot(&before)) {
        quit_failure("pause freezes the full game state and blocks input");
        return false;
    }
    smoke_pause_click(0);
    motion(450, 400);
    if (ui.paused || !matches_snapshot(&before)) {
        quit_failure("continue preserves the full game state");
        return false;
    }
    key(27, 0, 0);
    update_game(5);
    key('p', 0, 0);
    if (ui.paused || !matches_snapshot(&before)) {
        quit_failure("keyboard pause and resume preserve state");
        return false;
    }
    return true;
}
static void smoke_step(float dt) {
    if (smoke_menu_phase < 2) {
        if (!ui.menu) {
            quit_failure("initial mode menu");
            return;
        }
        if (smoke_menu_phase == 0) {
            key('c', 0, 0);
            if (machine->arm.opening_target != MACHINE_CONFIG.finger_max) {
                quit_failure("menu input isolation");
                return;
            }
            for (int i = 0; i < MODE_COUNT; ++i) {
                UIRect r = ui_mode_rect(&ui, i);
                mouse(GLUT_LEFT_BUTTON, GLUT_DOWN, (int)(r.x + r.w / 2), (int)(r.y + r.h / 2));
                if (ui.menu || ui.free_camera || drag_button != -1 ||
                    !same_camera(ui.camera, camera_default()) || machine->mode != (GameMode)i ||
                    machine->time_remaining != GAME_MODES[i].time_limit) {
                    quit_failure("mode mouse selection");
                    return;
                }
                AABB tray = scene_bounds(machine->solids[machine->tray_solid].node);
                if (v3_length(v3_sub(v3_sub(tray.max, tray.min), GAME_MODES[i].tray_size)) >
                    .001f) {
                    quit_failure("mode selects its delivery tray size");
                    return;
                }
                key('f', 0, 0);
                mouse(GLUT_MIDDLE_BUTTON, GLUT_DOWN, 300, 300);
                motion(340, 320);
                key('+', 0, 0);
                update_game(.1f);
                if (!smoke_preserve_pause())
                    return;
                float before_resume = machine->time_remaining;
                update_game(.02f);
                if (i != MODE_SANDBOX && machine->time_remaining >= before_resume) {
                    quit_failure("countdown resumes after continue");
                    return;
                }
                key('p', 0, 0);
                smoke_pause_click(2);
                if (!ui.menu || ui.paused || ui.free_camera || drag_button != -1 ||
                    !same_camera(ui.camera, camera_default()) ||
                    machine->time_remaining != GAME_MODES[i].time_limit ||
                    machine->arm.angle[0] != MACHINE_CONFIG.joints[0].initial) {
                    quit_failure("returning to modes discards the current round");
                    return;
                }
            }
            pending_capture = "build/smoke_menu.ppm";
        } else {
            if (machine->time_remaining != GAME_MODES[MODE_HARD].time_limit) {
                quit_failure("paused menu clock");
                return;
            }
            key('1', 0, 0);
            key('f', 0, 0);
            mouse(GLUT_MIDDLE_BUTTON, GLUT_DOWN, 300, 300);
            motion(340, 320);
            Camera before_help = ui.camera;
            special(GLUT_KEY_F1, 0, 0);
            motion(400, 360);
            wheel(0, 1, 300, 300);
            key('f', 0, 0);
            if (!ui.help || !ui.free_camera || drag_button != -1 ||
                !same_camera(ui.camera, before_help)) {
                quit_failure("help cancels camera drag and blocks camera input");
                return;
            }
            special(GLUT_KEY_F1, 0, 0);
            motion(450, 400);
            if (ui.help || !same_camera(ui.camera, before_help)) {
                quit_failure("closing help does not resume camera drag");
                return;
            }
            key('r', 0, 0);
            if (ui.free_camera || !same_camera(ui.camera, camera_default())) {
                quit_failure("retry restores orbital camera");
                return;
            }
        }
        ++smoke_menu_phase;
        return;
    }
    if (smoke_pause_frames > 0) {
        if (!ui.paused || !matches_snapshot(&smoke_pause_snapshot)) {
            quit_failure("pause preserves state across real timer callbacks");
            return;
        }
        if (--smoke_pause_frames == 0)
            smoke_pause_click(0);
        return;
    }
    if (++smoke_frames > 2400) {
        quit_failure("timeout");
        return;
    }
    smoke_wait += dt;
    if (!machine_at_target(machine) || smoke_wait < .12f)
        return;
    smoke_wait = 0;
    if (machine->blocked_moves && smoke_phase < 12) {
        quit_failure("documented route blocked");
        return;
    }
    switch (smoke_phase) {
    case 0: {
        if (smoke_prize == 0) {
            /* Random spawns may be behind the arm from the default angle.
             * Orbit to the open side using the same mouse path as a player. */
            mouse(GLUT_RIGHT_BUTTON, GLUT_DOWN, 400, 300);
            motion(486, 300);
            mouse(GLUT_RIGHT_BUTTON, GLUT_UP, 486, 300);
            pending_capture = "build/smoke_initial.ppm";
        }
        Vec3 screen;
        if (!camera_project(&ui.camera,
                            mat4_point(machine->prizes[smoke_prize].node->world, vec3(0, 0, 0)),
                            ui_view_width(&ui), ui_view_height(&ui), &screen)) {
            quit_failure("project prize");
            return;
        }
        mouse(GLUT_LEFT_BUTTON, GLUT_DOWN, (int)lroundf(screen.x), (int)lroundf(screen.y) + UI_TOP);
        if (machine->selected != smoke_prize) {
            quit_failure("mouse picking");
            return;
        }
        smoke_target(0, machine->prizes[smoke_prize].pickup_pose[0]);
        break;
    }
    case 1:
        smoke_target(3, machine->prizes[smoke_prize].pickup_pose[3]);
        break;
    case 2:
        smoke_target(1, machine->prizes[smoke_prize].pickup_pose[1]);
        break;
    case 3:
        key('c', 0, 0);
        break;
    case 4:
        if (machine->held != smoke_prize) {
            quit_failure("capture");
            return;
        }
        if (!smoke_preserve_pause())
            return;
        if (smoke_prize == 0)
            pending_capture = "build/smoke_pickup.ppm";
        smoke_target(1, -25);
        break;
    case 5:
        smoke_target(3, -70);
        break;
    case 6:
        smoke_target(0, MACHINE_CONFIG.drop_yaw[smoke_prize]);
        break;
    case 7:
        if (machine->delivered != smoke_prize) {
            quit_failure("count before release");
            return;
        }
        key('o', 0, 0);
        update_game(1.0f / 60);
        if (!smoke_preserve_pause())
            return;
        if (smoke_prize == 0) {
            snapshot_game(&smoke_pause_snapshot);
            key('p', 0, 0);
            smoke_pause_frames = 3;
            pending_capture = "build/smoke_pause.ppm";
        }
        break;
    case 8:
        if (machine->prizes[smoke_prize].state == PRIZE_FALLING)
            return;
        if (machine->delivered != smoke_prize + 1) {
            quit_failure("delivery");
            return;
        }
        if (++smoke_prize < PRIZE_COUNT) {
            smoke_phase = 0;
            return;
        }
        pending_capture = "build/smoke_delivery.ppm";
        break;
    case 9: {
        Camera before = ui.camera;
        int selected = machine->selected;
        mouse(GLUT_MIDDLE_BUTTON, GLUT_DOWN, 300, 300);
        motion(340, 320);
        mouse(GLUT_MIDDLE_BUTTON, GLUT_UP, 340, 320);
        if (v3_length(v3_sub(ui.camera.target, before.target)) < .01f ||
            machine->selected != selected) {
            quit_failure("middle-button pan");
            return;
        }
        Vec3 released = ui.camera.target;
        motion(390, 330);
        if (v3_length(v3_sub(released, ui.camera.target)) > .001f) {
            quit_failure("released camera drag");
            return;
        }
        key('f', 0, 0);
        Vec3 eye = camera_eye(&ui.camera);
        mouse(GLUT_RIGHT_BUTTON, GLUT_DOWN, 300, 300);
        motion(390, 330);
        mouse(GLUT_RIGHT_BUTTON, GLUT_UP, 390, 330);
        if (!ui.free_camera || v3_length(v3_sub(eye, camera_eye(&ui.camera))) > .001f) {
            quit_failure("free-look eye position");
            return;
        }
        wheel(0, 1, 300, 300);
        if (v3_length(v3_sub(eye, camera_eye(&ui.camera))) < .5f) {
            quit_failure("free camera dolly");
            return;
        }
        special(GLUT_KEY_HOME, 0, 0);
    }
        mouse(GLUT_RIGHT_BUTTON, GLUT_DOWN, 300, 300);
        motion(390, 330);
        mouse(GLUT_RIGHT_BUTTON, GLUT_UP, 390, 330);
        wheel(0, 1, 300, 300);
        special(GLUT_KEY_HOME, 0, 0);
        key('d', 0, 0);
        if (!ui.debug || ui.free_camera ||
            fabsf(ui.camera.distance - camera_default().distance) > .001f) {
            quit_failure("camera/debug");
            return;
        }
        pending_capture = "build/smoke_debug.ppm";
        break;
    case 10:
        key('h', 0, 0);
        pending_capture = "build/smoke_help.ppm";
        break;
    case 11:
        key('h', 0, 0);
        key('r', 0, 0);
        smoke_target(1, -85);
        break;
    case 12:
        if (!machine->blocked_moves || !machine_pose_valid(machine)) {
            quit_failure("collision rejection");
            return;
        }
        key('r', 0, 0);
        if (machine->delivered || machine->held != -1 || machine->selected != -1) {
            quit_failure("reset");
            return;
        }
        key('p', 0, 0);
        smoke_pause_click(2);
        key('4', 0, 0);
        machine->time_remaining = .01f;
        break;
    case 13:
        if (machine->round != ROUND_LOST) {
            quit_failure("timed round expiration");
            return;
        }
        pending_capture = "build/smoke_timeout.ppm";
        break;
    case 14: {
        uint32_t previous_seed = machine->spawn_seed;
        key('p', 0, 0);
        smoke_pause_click(1);
        if (machine->round != ROUND_PLAYING || machine->mode != MODE_HARD ||
            machine->time_remaining != GAME_MODES[MODE_HARD].time_limit ||
            machine->spawn_seed == previous_seed) {
            quit_failure("retry same difficulty");
            return;
        }
        if (ui.debug)
            key('d', 0, 0);
        pending_capture = "build/smoke_hard_tray.ppm";
        break;
    }
    case 15:
        puts("prize_machine: graphical session passed (pause/resume, frozen state and timer, "
             "difficulty trays, random spawns, reset, return to modes, all 3 prizes, free camera, "
             "collision, delivery, timeout)");
        glutLeaveMainLoop();
        break;
    default:
        break;
    }
    ++smoke_phase;
}
static void display(void) {
    ui_draw(&ui, machine);
    if (smoke && glGetError() != GL_NO_ERROR) {
        quit_failure("OpenGL error");
        return;
    }
    if (pending_capture || screenshot) {
        const char *path = pending_capture ? pending_capture : screenshot;
        if (!render_save_ppm(path, ui.width, ui.height)) {
            fprintf(stderr, "Nao foi possivel gravar %s\n", path);
            exit_code = EXIT_FAILURE;
        }
        pending_capture = NULL;
        if (screenshot)
            glutLeaveMainLoop();
    }
    glutSwapBuffers();
}
static void timer(int value) {
    (void)value;
    if (!machine)
        return;
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = smoke ? 1.0f / 60 : fmaxf((float)(now - last_time) * .001f, 0);
    last_time = now;
    update_game(dt);
    if (smoke)
        smoke_step(dt);
    glutPostRedisplay();
    glutTimerFunc(smoke ? 1 : 16, timer, 0);
}
int main(int argc, char **argv) {
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--smoke-test") == 0)
            smoke = true;
        else if (strcmp(argv[i], "--screenshot") == 0 && i + 1 < argc)
            screenshot = argv[++i];
        else {
            fprintf(stderr, "Uso: %s [--smoke-test | --screenshot arquivo.ppm]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }
    machine = machine_create();
    if (!machine) {
        fprintf(stderr, "Sem memoria para construir a cena.\n");
        return EXIT_FAILURE;
    }
    ui = (UserInterface){.width = 1280, .height = 800, .menu = true, .camera = camera_default()};
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(ui.width, ui.height);
    glutCreateWindow("Pizza Planet | LaClaw Lab");
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    glutIgnoreKeyRepeat(1);
    render_init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(key);
    glutSpecialFunc(special);
    glutSpecialUpFunc(special_up);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutMouseWheelFunc(wheel);
    glutEntryFunc(entry);
    glutCloseFunc(cleanup);
    last_time = glutGet(GLUT_ELAPSED_TIME);
    glutTimerFunc(16, timer, 0);
    glutMainLoop();
    cleanup();
    return exit_code;
}
