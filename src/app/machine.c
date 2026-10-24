#include "app/machine.h"
#include "app/physics.h"
#include "app/theme.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const Color WHITE = {.78f, .84f, .91f}, DARK = {.10f, .13f, .24f};
static const Color RED = {.87f, .055f, .085f}, METAL = {.64f, .72f, .84f};
static bool initialize_spawns(PrizeMachine *m);

static uint32_t next_spawn_seed(void) {
    static uint32_t sequence;
    if (!sequence) {
        struct timespec now = {0};
        timespec_get(&now, TIME_UTC);
        sequence = (uint32_t)now.tv_sec ^ (uint32_t)now.tv_nsec;
    }
    return sequence += UINT32_C(0x9e3779b9);
}

static Mat4 shape(Vec3 position, Vec3 size) {
    return mat4_mul(mat4_translate(position), mat4_scale(size));
}
static void message(PrizeMachine *m, const char *text) {
    snprintf(m->message, sizeof(m->message), "%s", text);
    m->message_time = 2.8f;
}
static SceneNode *add_node(SceneNode *parent, const char *name, Geometry *geometry) {
    SceneNode *node = scene_node_create(name, geometry);
    if (!node) {
        if (geometry)
            geometry->destroy(geometry);
        return NULL;
    }
    if (parent && !scene_node_add(parent, node)) {
        scene_node_destroy(node);
        return NULL;
    }
    return node;
}
static SceneNode *add_shape(SceneNode *parent, const char *name, Color color, bool cylinder,
                            Mat4 local) {
    Geometry *g = NULL;
    if (cylinder) {
        Cylinder *c = CylinderFabric.create(color, 24);
        if (c)
            g = &c->base;
    } else {
        Box *b = BoxFabric.create(color);
        if (b)
            g = &b->base;
    }
    if (!g)
        return NULL;
    SceneNode *node = add_node(parent, name, g);
    if (node)
        node->local = local;
    return node;
}
static bool solid(PrizeMachine *m, const char *name, Vec3 position, Vec3 size, Color color) {
    if (m->solid_count >= MAX_SOLIDS)
        return false;
    SceneNode *node = add_shape(m->root, name, color, false, shape(position, size));
    if (!node)
        return false;
    m->solids[m->solid_count++] = (Solid){node, name};
    return true;
}
static bool themed_solid(PrizeMachine *m, const char *name, ThemePart part) {
    if (m->solid_count >= MAX_SOLIDS)
        return false;
    Geometry *geometry = theme_cabinet_create(part);
    if (!geometry)
        return false;
    SceneNode *node = add_node(m->root, name, geometry);
    if (!node)
        return false;
    m->solids[m->solid_count++] = (Solid){node, name};
    return true;
}
static void update_world(PrizeMachine *m) {
    RoboticArm *a = &m->arm;
    const MachineConfig *c = &MACHINE_CONFIG;
    a->joint[0]->local =
        mat4_mul(mat4_translate(vec3(0, c->shoulder_height, 0)), mat4_rotate_y(a->angle[0]));
    a->joint[1]->local = mat4_rotate_z(a->angle[1]);
    a->joint[2]->local =
        mat4_mul(mat4_translate(vec3(0, c->upper_length, 0)), mat4_rotate_z(a->angle[2]));
    a->joint[3]->local =
        mat4_mul(mat4_translate(vec3(0, c->fore_length, 0)), mat4_rotate_z(a->angle[3]));
    for (int i = 0; i < 2; ++i) {
        float side = i == 0 ? -1.0f : 1.0f;
        a->finger[i]->local =
            mat4_mul(mat4_translate(vec3(side * .18f, .20f, 0)), mat4_rotate_z(-side * a->opening));
    }
    scene_update(m->root, mat4_identity());
    if (m->held >= 0) {
        Prize *p = &m->prizes[m->held];
        p->node->local = mat4_mul(a->grip->world, p->relative_to_grip);
        scene_update(p->node, mat4_identity());
    }
}
static bool build_cabinet(PrizeMachine *m) {
    if (!themed_solid(m, "piso", THEME_FLOOR) ||
        !solid(m, "fundo", vec3(1, 2.2f, -3.30f), vec3(3, 4.4f, .16f),
               (Color){.045f, .09f, .24f}) ||
        !solid(m, "coluna esquerda", vec3(-1.9f, 2.3f, 0), vec3(.16f, 4.6f, .16f), RED) ||
        !solid(m, "coluna direita", vec3(3.9f, 2.3f, 0), vec3(.16f, 4.6f, .16f), RED) ||
        !solid(m, "letreiro", vec3(1, 4.55f, -3.45f), vec3(2, .14f, .15f), RED) ||
        !solid(m, "batente", vec3(3.8f, .45f, 0), vec3(.18f, .90f, 2.4f), RED) ||
        !solid(m, "pedestal", vec3(0, .15f, 0), vec3(.8f, .30f, .8f), DARK))
        return false;
    m->tray_solid = m->solid_count;
    if (!solid(m, "bandeja", MACHINE_CONFIG.tray_center, MACHINE_CONFIG.tray_size,
               (Color){.18f, .68f, .55f}))
        return false;
    /* New structural parts stay outside the playable interior, but also
     * participate in picking/collision as conservative AABBs. */
    if (!themed_solid(m, "cupula", THEME_ROOF) || !themed_solid(m, "fuselagem", THEME_BASE) ||
        !themed_solid(m, "aleta esquerda", THEME_FIN_LEFT) ||
        !themed_solid(m, "aleta direita", THEME_FIN_RIGHT))
        return false;
    Geometry *details = theme_cabinet_create(THEME_DETAILS);
    if (!details || !add_node(m->root, "estrelas e planetas", details))
        return false;
    return true;
}
static bool build_arm(PrizeMachine *m) {
    RoboticArm *a = &m->arm;
    const MachineConfig *c = &MACHINE_CONFIG;
    SceneNode *parent = m->root;
    for (int i = 0; i < JOINT_COUNT; ++i) {
        a->angle[i] = a->target[i] = c->joints[i].initial;
        a->joint[i] = add_node(parent, c->joints[i].name, NULL);
        if (!a->joint[i])
            return false;
        parent = a->joint[i];
    }
    a->opening = a->opening_target = c->finger_max;
    a->grip = add_node(a->joint[3], "centro da garra", NULL);
    if (!a->grip)
        return false;
    a->grip->local = mat4_translate(vec3(0, c->grip_offset, 0));
    a->parts[0] =
        add_shape(a->joint[1], "braco superior", WHITE, false,
                  shape(vec3(0, c->upper_length * .5f, 0), vec3(.22f, c->upper_length, .28f)));
    a->parts[1] =
        add_shape(a->joint[2], "antebraco", WHITE, false,
                  shape(vec3(0, c->fore_length * .5f, 0), vec3(.20f, c->fore_length, .26f)));
    a->parts[2] = add_shape(a->joint[3], "palma", DARK, false,
                            shape(vec3(0, .10f, 0), vec3(.62f, .20f, .30f)));
    for (int i = 0; i < 3; ++i)
        if (!a->parts[i])
            return false;
    for (int i = 0; i < 2; ++i) {
        a->finger[i] = add_node(a->joint[3], "pivo do dedo", NULL);
        if (!a->finger[i])
            return false;
        a->parts[3 + i] = add_shape(a->finger[i], "dedo", METAL, false,
                                    shape(vec3(0, .27f, 0), vec3(.12f, .54f, .22f)));
        if (!a->parts[3 + i])
            return false;
    }
    for (int i = 1; i < JOINT_COUNT; ++i) {
        Mat4 hub = mat4_mul(mat4_rotate_x(90), mat4_scale(vec3(.42f, .36f, .42f)));
        a->parts[4 + i] = add_shape(a->joint[i], "articulacao", RED, true, hub);
        if (!a->parts[4 + i])
            return false;
    }
    a->parts[8] = add_shape(a->joint[0], "rotor", METAL, true,
                            shape(vec3(0, -.14f, 0), vec3(.65f, .30f, .65f)));
    if (!a->parts[8])
        return false;
    return true;
}
PrizeMachine *machine_create(void) {
    return machine_create_seeded(MODE_SANDBOX, next_spawn_seed());
}
PrizeMachine *machine_create_seeded(GameMode mode, uint32_t seed) {
    if (mode < MODE_SANDBOX || mode >= MODE_COUNT)
        return NULL;
    PrizeMachine *m = calloc(1, sizeof(*m));
    if (!m)
        return NULL;
    m->selected = m->held = m->release_contact = -1;
    m->mode = mode;
    m->time_remaining = GAME_MODES[mode].time_limit;
    m->spawn_seed = seed;
    m->root = scene_node_create("maquina", NULL);
    if (!m->root || !build_cabinet(m) || !build_arm(m)) {
        machine_destroy(m);
        return NULL;
    }
    const MachineConfig *c = &MACHINE_CONFIG;
    static const char *names[PRIZE_COUNT] = {"Coral", "Azul", "Amarelo"};
    for (int i = 0; i < PRIZE_COUNT; ++i) {
        Geometry *alien = theme_alien_create(c->prize_colors[i]);
        if (!alien) {
            machine_destroy(m);
            return NULL;
        }
        m->prizes[i].node = add_node(m->root, names[i], alien);
        m->prizes[i].name = names[i];
        m->prizes[i].state = PRIZE_AVAILABLE;
        if (!m->prizes[i].node) {
            machine_destroy(m);
            return NULL;
        }
    }
    if (!initialize_spawns(m)) {
        machine_destroy(m);
        return NULL;
    }
    update_world(m);
    message(m, "Escolha um premio e aproxime a garra.");
    return m;
}
void machine_destroy(PrizeMachine *m) {
    if (m) {
        scene_node_destroy(m->root);
        free(m);
    }
}
bool machine_reset(PrizeMachine *m) {
    return machine_start(m, m->mode);
}
bool machine_start(PrizeMachine *m, GameMode mode) {
    if (mode < MODE_SANDBOX || mode >= MODE_COUNT)
        return false;
    PrizeMachine *fresh = machine_create_seeded(mode, next_spawn_seed());
    if (!fresh)
        return false;
    SceneNode *old = m->root;
    *m = *fresh;
    free(fresh);
    scene_node_destroy(old);
    return true;
}
Vec3 machine_grip_position(const PrizeMachine *m) {
    return mat4_point(m->arm.grip->world, vec3(0, 0, 0));
}
AABB machine_delivery_region(const PrizeMachine *m) {
    AABB region = scene_bounds(m->solids[m->tray_solid].node);
    region.min.y = region.max.y;
    region.max.y += .8f;
    return region;
}
static float finger_gap(float angle) {
    float a = cg_radians(angle);
    return 2 * (.18f + .40f * sinf(a) - .06f * cosf(a));
}
static bool unobstructed(const PrizeMachine *m, Vec3 a, Vec3 b) {
    Vec3 delta = v3_sub(b, a);
    float length = v3_length(delta);
    if (length < 1e-6f)
        return true;
    Ray ray = {a, v3_scale(delta, 1 / length)};
    for (int i = 0; i < m->solid_count; ++i) {
        float t;
        if (ray_aabb(ray, scene_bounds(m->solids[i].node), &t) && t < length - .002f)
            return false;
    }
    return true;
}
int machine_capture_candidate(const PrizeMachine *m, float *required_gap) {
    if (m->held >= 0 || m->round != ROUND_PLAYING)
        return -1;
    Mat4 inverse;
    if (!mat4_inverse(m->arm.grip->world, &inverse))
        return -1;
    int best = -1;
    float nearest = FLT_MAX, gap = 0;
    for (int i = 0; i < PRIZE_COUNT; ++i)
        if (m->prizes[i].state == PRIZE_AVAILABLE) {
            const SceneNode *node = m->prizes[i].node;
            AABB local = aabb_transform(node->geometry->bounds, mat4_mul(inverse, node->world));
            Vec3 center = v3_scale(v3_add(local.min, local.max), .5f);
            float width = local.max.x - local.min.x + 2 * fabsf(center.x);
            if (fabsf(center.x) > MACHINE_CONFIG.capture_x ||
                fabsf(center.y) > MACHINE_CONFIG.capture_y ||
                fabsf(center.z) > MACHINE_CONFIG.capture_z ||
                width > finger_gap(m->arm.opening) + .025f || local.max.z - local.min.z > .64f ||
                local.max.y - local.min.y > .64f)
                continue;
            if (!unobstructed(m, machine_grip_position(m), mat4_point(node->world, vec3(0, 0, 0))))
                continue;
            float distance = v3_length(center);
            if (distance < nearest) {
                best = i;
                nearest = distance;
                gap = width;
            }
        }
    if (best >= 0 && required_gap)
        *required_gap = gap;
    return best;
}
static bool inside_workspace(AABB box) {
    AABB w = MACHINE_CONFIG.workspace;
    float t = MACHINE_CONFIG.collision_tolerance;
    return box.min.x >= w.min.x - t && box.max.x <= w.max.x + t && box.min.y >= w.min.y - t &&
           box.max.y <= w.max.y + t && box.min.z >= w.min.z - t && box.max.z <= w.max.z + t;
}
bool machine_pose_valid(const PrizeMachine *m) {
    float tolerance = MACHINE_CONFIG.collision_tolerance;
    int candidate = machine_capture_candidate(m, NULL);
    for (int part = 0; part < ARM_PART_COUNT; ++part) {
        AABB bounds = scene_bounds(m->arm.parts[part]);
        if (!inside_workspace(bounds))
            return false;
        for (int s = 0; s < m->solid_count; ++s)
            if (aabb_overlap(bounds, scene_bounds(m->solids[s].node), tolerance))
                return false;
        for (int p = 0; p < PRIZE_COUNT; ++p) {
            if (m->prizes[p].state == PRIZE_HELD)
                continue;
            /* In the finger/prize pair only, refine the world AABB in the finger's
             * local frame. This avoids a yaw-expanded box closing the empty gap. */
            if (part == 3 || part == 4) {
                if (p == candidate || p == m->release_contact)
                    continue;
                Mat4 inverse;
                if (mat4_inverse(m->arm.parts[part]->world, &inverse)) {
                    AABB local = aabb_transform(m->prizes[p].node->geometry->bounds,
                                                mat4_mul(inverse, m->prizes[p].node->world));
                    if (!aabb_overlap(local, m->arm.parts[part]->geometry->bounds, tolerance))
                        continue;
                }
            }
            if (aabb_overlap(bounds, scene_bounds(m->prizes[p].node), tolerance))
                return false;
        }
    }
    if (m->held >= 0) {
        AABB bounds = scene_bounds(m->prizes[m->held].node);
        if (!inside_workspace(bounds))
            return false;
        for (int s = 0; s < m->solid_count; ++s)
            if (aabb_overlap(bounds, scene_bounds(m->solids[s].node), tolerance))
                return false;
        for (int p = 0; p < PRIZE_COUNT; ++p)
            if (p != m->held)
                if (aabb_overlap(bounds, scene_bounds(m->prizes[p].node), tolerance))
                    return false;
    }
    return true;
}
void machine_target_joint(PrizeMachine *m, int joint, float degrees) {
    if (m->round != ROUND_PLAYING || joint < 0 || joint >= JOINT_COUNT || !isfinite(degrees))
        return;
    const JointConfig *c = &MACHINE_CONFIG.joints[joint];
    m->arm.target[joint] = cg_clamp(degrees, c->min, c->max);
    if (degrees < c->min || degrees > c->max)
        message(m, "Limite da junta.");
}
void machine_nudge_joint(PrizeMachine *m, int joint, float degrees) {
    if (joint >= 0 && joint < JOINT_COUNT)
        machine_target_joint(m, joint, m->arm.target[joint] + degrees);
}
void machine_open(PrizeMachine *m) {
    if (m->round != ROUND_PLAYING)
        return;
    Vec3 release_velocity = machine_at_target(m) ? vec3(0, 0, 0) : m->held_velocity;
    m->arm.opening_target = MACHINE_CONFIG.finger_max;
    if (m->held >= 0) {
        Prize *p = &m->prizes[m->held];
        /* Prize always belongs to root: attachment changes transforms, not owners. */
        p->node->local = p->node->world;
        p->state = PRIZE_FALLING;
        p->velocity = release_velocity;
        m->release_contact = m->held;
        m->held = -1;
        message(m, "Premio solto.");
    } else
        message(m, "Abrindo a garra.");
}
void machine_close(PrizeMachine *m) {
    if (m->round != ROUND_PLAYING)
        return;
    if (m->held >= 0) {
        message(m, "Garra ocupada.");
        return;
    }
    m->arm.opening_target = MACHINE_CONFIG.finger_min;
    message(m, "Fechando a garra.");
}
static float approach(float value, float target, float step) {
    return value + cg_clamp(target - value, -step, step);
}
static void blocked(PrizeMachine *m) {
    ++m->blocked_moves;
    message(m, "Movimento bloqueado.");
}
static void move_joints(PrizeMachine *m, float dt) {
    for (int i = 0; i < JOINT_COUNT; ++i) {
        float old = m->arm.angle[i];
        m->arm.angle[i] = approach(old, m->arm.target[i], MACHINE_CONFIG.joints[i].speed * dt);
        if (m->arm.angle[i] == old)
            continue;
        update_world(m);
        if (!machine_pose_valid(m)) {
            m->arm.angle[i] = m->arm.target[i] = old;
            update_world(m);
            blocked(m);
        }
    }
}
static void move_fingers(PrizeMachine *m, float dt) {
    float old = m->arm.opening,
          new_angle = approach(old, m->arm.opening_target, MACHINE_CONFIG.finger_speed * dt);
    if (new_angle == old)
        return;
    float required = 0;
    int candidate = machine_capture_candidate(m, &required);
    m->arm.opening = new_angle;
    update_world(m);
    if (!machine_pose_valid(m)) {
        m->arm.opening = m->arm.opening_target = old;
        update_world(m);
        blocked(m);
        return;
    }
    if (candidate >= 0 && new_angle < old && finger_gap(new_angle) <= required + .02f) {
        Mat4 inverse;
        if (!mat4_inverse(m->arm.grip->world, &inverse))
            return;
        Prize *p = &m->prizes[candidate];
        p->relative_to_grip = mat4_mul(inverse, p->node->world);
        p->state = PRIZE_HELD;
        p->velocity = vec3(0, 0, 0);
        m->held_velocity = vec3(0, 0, 0);
        m->held = candidate;
        m->arm.opening_target = new_angle;
        message(m, "Premio capturado!");
    }
}
/* Integer FK poses keep the palm upright and the grip within capture tolerance
 * of a floor-supported alien. Wider mode ranges require more elbow adjustment.
 * Candidates still have to pass the actual collision and capture rules below. */
static const float SPAWN_POSES[][JOINT_COUNT] = {
    {0, -53, -70, -57}, {0, -50, -75, -55}, {0, -48, -80, -52},  {0, -45, -85, -50},
    {0, -43, -90, -47}, {0, -40, -95, -45}, {0, -38, -100, -42},
};
static uint32_t spawn_random(uint32_t *state) {
    /* Local SplitMix32 stream: no dependency on libc rand or gameplay updates. */
    uint32_t z = (*state += UINT32_C(0x9e3779b9));
    z = (z ^ (z >> 16)) * UINT32_C(0x21f0aaad);
    z = (z ^ (z >> 15)) * UINT32_C(0x735a2d97);
    return z ^ (z >> 15);
}
static void place_spawn(Prize *p, const float pose[JOINT_COUNT], float yaw) {
    const MachineConfig *c = &MACHINE_CONFIG;
    memcpy(p->pickup_pose, pose, sizeof(p->pickup_pose));
    p->pickup_pose[0] = yaw;
    float radius = -c->upper_length * sinf(cg_radians(pose[1])) -
                   c->fore_length * sinf(cg_radians(pose[1] + pose[2]));
    Vec3 center =
        vec3(radius * cosf(cg_radians(yaw)), c->prize_size * .5f, -radius * sinf(cg_radians(yaw)));
    p->node->local =
        mat4_mul(mat4_translate(center),
                 mat4_mul(mat4_rotate_y(yaw),
                          mat4_scale(vec3(c->prize_size, c->prize_size, c->prize_size))));
}
static bool spawn_sites_valid(const PrizeMachine *m) {
    for (int p = 0; p < PRIZE_COUNT; ++p) {
        AABB box = scene_bounds(m->prizes[p].node);
        if (!inside_workspace(box) || fabsf(box.min.y) > .0001f)
            return false;
        /* The floor is an ellipse, not its enclosing AABB. Keep the whole
         * footprint inside it, with a margin for the 64-sided floor mesh. */
        for (int corner = 0; corner < 4; ++corner) {
            float x = ((corner & 1 ? box.max.x : box.min.x) - 1) / 3;
            float z = (corner & 2 ? box.max.z : box.min.z) / 3.6f;
            if (x * x + z * z > .98f)
                return false;
        }
        for (int s = 0; s < m->solid_count; ++s)
            if (aabb_overlap(box, scene_bounds(m->solids[s].node), 0))
                return false;
        for (int other = 0; other < p; ++other)
            if (aabb_overlap(box, scene_bounds(m->prizes[other].node), -.12f))
                return false;
    }
    return machine_pose_valid(m);
}
static bool spawn_move(PrizeMachine *m, int joint, float angle) {
    const JointConfig *config = &MACHINE_CONFIG.joints[joint];
    if (angle < config->min || angle > config->max)
        return false;
    m->arm.target[joint] = angle;
    unsigned blocked_before = m->blocked_moves;
    /* Use the same swept joint steps as gameplay, without advancing the timer
     * or simulating gravity on the already-supported prizes. */
    for (int step = 0; step < 4096; ++step) {
        move_joints(m, MACHINE_CONFIG.max_step);
        if (m->blocked_moves != blocked_before)
            return false;
        if (fabsf(m->arm.angle[joint] - angle) < .0001f)
            return true;
    }
    return false;
}
static bool spawn_route_valid(PrizeMachine *m, int p) {
    const float *pose = m->prizes[p].pickup_pose;
    /* Start at the far end of the tray sweep so the next pickup also works
     * after a delivery. The other aliens remain in place during validation. */
    float tray_end = MACHINE_CONFIG.drop_yaw[PRIZE_COUNT - 1];
    if (!spawn_move(m, 0, tray_end) || !spawn_move(m, 0, pose[0]) || !spawn_move(m, 2, pose[2]) ||
        !spawn_move(m, 3, pose[3]) || !spawn_move(m, 1, pose[1]) ||
        machine_capture_candidate(m, NULL) != p)
        return false;
    m->arm.opening_target = MACHINE_CONFIG.finger_min;
    for (int step = 0; step < 480 && m->held < 0; ++step)
        move_fingers(m, MACHINE_CONFIG.max_step);
    if (m->held != p || !spawn_move(m, 1, MACHINE_CONFIG.joints[1].initial) ||
        !spawn_move(m, 3, MACHINE_CONFIG.joints[3].initial) ||
        !spawn_move(m, 2, MACHINE_CONFIG.joints[2].initial) || !spawn_move(m, 0, tray_end) ||
        !spawn_move(m, 0, MACHINE_CONFIG.drop_yaw[p]))
        return false;
    AABB cargo = scene_bounds(m->prizes[p].node), tray = machine_delivery_region(m);
    return cargo.min.x >= tray.min.x && cargo.max.x <= tray.max.x && cargo.min.z >= tray.min.z &&
           cargo.max.z <= tray.max.z && cargo.min.y > tray.min.y;
}
static bool spawn_layout_valid(PrizeMachine *m) {
    if (!spawn_sites_valid(m))
        return false;
    PrizeMachine saved = *m;
    Mat4 local[PRIZE_COUNT];
    for (int p = 0; p < PRIZE_COUNT; ++p)
        local[p] = m->prizes[p].node->local;
    for (int p = 0; p < PRIZE_COUNT; ++p) {
        bool valid = spawn_route_valid(m, p);
        /* Restore both the value state and the borrowed scene nodes, including
         * failures halfway through a capture. No validation leaks into play. */
        *m = saved;
        for (int i = 0; i < PRIZE_COUNT; ++i)
            m->prizes[i].node->local = local[i];
        update_world(m);
        if (!valid)
            return false;
    }
    return true;
}
static bool initialize_spawns(PrizeMachine *m) {
    const ModeConfig *mode = &GAME_MODES[m->mode];
    uint32_t random = m->spawn_seed;
    int slots[PRIZE_COUNT] = {0, 1, 2};
    for (int i = PRIZE_COUNT - 1; i > 0; --i) {
        int other = (int)(spawn_random(&random) % (uint32_t)(i + 1));
        int swap = slots[i];
        slots[i] = slots[other];
        slots[other] = swap;
    }
    for (int attempt = 0; attempt < 16; ++attempt) {
        for (int p = 0; p < PRIZE_COUNT; ++p) {
            int index = mode->spawn_pose_first +
                        (int)(spawn_random(&random) % (uint32_t)mode->spawn_pose_count);
            int jitter = (int)(spawn_random(&random) % (uint32_t)(2 * mode->spawn_yaw_jitter + 1)) -
                         mode->spawn_yaw_jitter;
            place_spawn(&m->prizes[p], SPAWN_POSES[index],
                        MACHINE_CONFIG.prize_yaw[slots[p]] + jitter);
        }
        update_world(m);
        if (spawn_layout_valid(m))
            return true;
    }
    /* Bounded retries, then the original reachable layout with shuffled colors.
     * Recheck it too, so future cabinet changes cannot introduce an unsafe spawn. */
    for (int p = 0; p < PRIZE_COUNT; ++p)
        place_spawn(&m->prizes[p], PICKUP_POSE, MACHINE_CONFIG.prize_yaw[slots[p]]);
    update_world(m);
    return spawn_layout_valid(m);
}
void machine_update(PrizeMachine *m, float dt) {
    if (!isfinite(dt) || dt <= 0 || m->round != ROUND_PLAYING)
        return;
    float elapsed = dt;
    bool timed = GAME_MODES[m->mode].time_limit > 0;
    if (timed)
        dt = fminf(dt, m->time_remaining);
    /* Cap physics work after stalls, but account for the full elapsed clock time. */
    dt = fminf(dt, .10f);
    m->message_time = fmaxf(0, m->message_time - elapsed);
    int steps = (int)ceilf(dt / MACHINE_CONFIG.max_step);
    float step = steps > 0 ? dt / (float)steps : 0;
    for (int i = 0; i < steps; ++i) {
        int held = m->held;
        Vec3 before =
            held >= 0 ? mat4_point(m->prizes[held].node->world, vec3(0, 0, 0)) : vec3(0, 0, 0);
        move_joints(m, step);
        move_fingers(m, step);
        if (held >= 0 && m->held == held) {
            Vec3 after = mat4_point(m->prizes[held].node->world, vec3(0, 0, 0));
            m->held_velocity =
                machine_at_target(m) ? vec3(0, 0, 0) : v3_scale(v3_sub(after, before), 1 / step);
        }
        int delivered = m->delivered;
        physics_step(m, step);
        if (m->release_contact >= 0) {
            AABB p = scene_bounds(m->prizes[m->release_contact].node);
            if (!aabb_overlap(p, scene_bounds(m->arm.parts[3]), 0) &&
                !aabb_overlap(p, scene_bounds(m->arm.parts[4]), 0))
                m->release_contact = -1;
        }
        if (timed)
            m->time_remaining = fmaxf(0, m->time_remaining - step);
        if (m->delivered > delivered)
            message(m, "Entrega concluida!");
        if (m->delivered == PRIZE_COUNT) {
            m->round = ROUND_WON;
            message(m, "Tripulacao resgatada! R: repetir. P: pausa.");
            return;
        }
    }
    if (timed) {
        m->time_remaining = fmaxf(0, m->time_remaining - fmaxf(0, elapsed - dt));
        if (m->time_remaining <= .00001f) {
            m->time_remaining = 0;
            m->round = ROUND_LOST;
            message(m, "Tempo esgotado! R: repetir. P: pausa.");
        }
    }
}
int machine_pick(PrizeMachine *m, Ray ray) {
    if (m->round != ROUND_PLAYING)
        return -1;
    float closest = FLT_MAX;
    int selected = -1;
    for (int s = 0; s < m->solid_count; ++s) {
        float distance;
        if (ray_aabb(ray, scene_bounds(m->solids[s].node), &distance))
            closest = fminf(closest, distance);
    }
    for (int p = 0; p < ARM_PART_COUNT; ++p) {
        float distance;
        if (ray_aabb(ray, scene_bounds(m->arm.parts[p]), &distance))
            closest = fminf(closest, distance);
    }
    /* Held/delivered prizes occlude too, although they are not selectable. */
    for (int p = 0; p < PRIZE_COUNT; ++p)
        if (m->prizes[p].state != PRIZE_AVAILABLE) {
            float distance;
            if (ray_aabb(ray, scene_bounds(m->prizes[p].node), &distance))
                closest = fminf(closest, distance);
        }
    for (int p = 0; p < PRIZE_COUNT; ++p)
        if (m->prizes[p].state == PRIZE_AVAILABLE) {
            float distance;
            if (ray_aabb(ray, scene_bounds(m->prizes[p].node), &distance) && distance < closest) {
                selected = p;
                closest = distance;
            }
        }
    m->selected = selected;
    if (selected >= 0)
        message(m, "Premio selecionado. Controle as juntas.");
    else
        message(m, "Nenhum premio selecionado.");
    return selected;
}
bool machine_at_target(const PrizeMachine *m) {
    for (int i = 0; i < JOINT_COUNT; ++i)
        if (fabsf(m->arm.angle[i] - m->arm.target[i]) > .001f)
            return false;
    return fabsf(m->arm.opening - m->arm.opening_target) < .001f;
}
const char *prize_state_name(PrizeState state) {
    switch (state) {
    case PRIZE_AVAILABLE:
        return "Disponivel";
    case PRIZE_HELD:
        return "Na garra";
    case PRIZE_FALLING:
        return "Em movimento";
    case PRIZE_DELIVERED:
        return "Entregue";
    }
    return "?";
}
