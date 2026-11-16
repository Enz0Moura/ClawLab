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
static bool solid_child(PrizeMachine *m, SceneNode *parent, const char *name, Mat4 local) {
    if (m->solid_count >= MAX_SOLIDS)
        return false;
    SceneNode *node = add_shape(parent, name, (Color){.045f, .09f, .24f}, false, local);
    if (!node)
        return false;
    m->solids[m->solid_count++] = (Solid){node, name};
    return true;
}
static bool themed_solid(PrizeMachine *m, const char *name, ThemePart part,
                         const ThemeCabinetDesign *design) {
    if (m->solid_count >= MAX_SOLIDS)
        return false;
    Geometry *geometry = theme_cabinet_create_design(part, design);
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
        mat4_mul(mat4_translate(vec3(0, c->shoulder_height, 0)),
                 mat4_rotate_y(a->angles.current[0]));
    a->joint[1]->local = mat4_rotate_z(a->angles.current[1]);
    a->joint[2]->local =
        mat4_mul(mat4_translate(vec3(0, c->upper_length, 0)), mat4_rotate_z(a->angles.current[2]));
    a->joint[3]->local =
        mat4_mul(mat4_translate(vec3(0, c->fore_length, 0)), mat4_rotate_z(a->angles.current[3]));
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
    ThemeCabinetDesign design = theme_cabinet_default_design();
    if (!themed_solid(m, "piso", THEME_FLOOR, &design))
        return false;

    Geometry *wall = theme_cabinet_create_design(THEME_BACK_WALL, &design);
    if (!wall)
        return false;
    m->back_wall = add_node(m->root, "parede curva traseira", wall);
    if (!m->back_wall)
        return false;
    m->back_wall_collision = add_node(m->root, "colisoes da parede traseira", NULL);
    if (!m->back_wall_collision)
        return false;

    const float pi = 3.14159265358979323846f;
    const ThemeBackWallDesign *wall_design = &design.back_wall;
    unsigned segments = wall_design->collision_segments;
    for (unsigned i = 0; i < segments; ++i) {
        float a0 = pi + pi * (float)i / (float)segments;
        float a1 = pi + pi * (float)(i + 1) / (float)segments;
        float angle = .5f * (a0 + a1);
        Vec3 start = vec3(wall_design->center_x + wall_design->radius_x * cosf(a0), 0,
                          wall_design->radius_z * sinf(a0));
        Vec3 end = vec3(wall_design->center_x + wall_design->radius_x * cosf(a1), 0,
                        wall_design->radius_z * sinf(a1));
        float dx = -wall_design->radius_x * sinf(angle);
        float dz = wall_design->radius_z * cosf(angle);
        float yaw = atan2f(-dz, dx) * (180.0f / pi);
        Vec3 center = vec3(wall_design->center_x + wall_design->radius_x * cosf(angle),
                           wall_design->height * .5f,
                           wall_design->radius_z * sinf(angle));
        Mat4 local = mat4_mul(
            mat4_translate(center),
            mat4_mul(mat4_rotate_y(yaw),
                     mat4_scale(vec3(v3_length(v3_sub(end, start)), wall_design->height,
                                     wall_design->thickness))));
        if (!solid_child(m, m->back_wall_collision, "segmento da parede traseira", local))
            return false;
    }

    if (!solid(m, "coluna esquerda", vec3(-1.9f, 2.3f, 0), vec3(.16f, 4.6f, .16f), RED) ||
        !solid(m, "coluna direita", vec3(3.9f, 2.3f, 0), vec3(.16f, 4.6f, .16f), RED) ||
        !solid(m, "letreiro", vec3(1, 4.55f, -3.45f), vec3(2, .14f, .15f), RED) ||
        !solid(m, "batente", vec3(3.8f, .45f, 0), vec3(.18f, .90f, 2.4f), RED) ||
        !solid(m, "pedestal", vec3(0, .15f, 0), vec3(.8f, .30f, .8f), DARK))
        return false;
    m->tray_solid = m->solid_count;
    if (!solid(m, "bandeja", MACHINE_CONFIG.tray_center, GAME_MODES[m->mode].tray_size,
               (Color){.18f, .68f, .55f}))
        return false;
    /* New structural parts stay outside the playable interior, but also
     * participate in picking/collision as conservative AABBs. */
    if (!themed_solid(m, "cupula", THEME_ROOF, &design) ||
        !themed_solid(m, "fuselagem", THEME_BASE, &design) ||
        !themed_solid(m, "aleta esquerda", THEME_FIN_LEFT, &design) ||
        !themed_solid(m, "aleta direita", THEME_FIN_RIGHT, &design))
        return false;
    return true;
}
static bool build_arm(PrizeMachine *m) {
    RoboticArm *a = &m->arm;
    const MachineConfig *c = &MACHINE_CONFIG;
    SceneNode *parent = m->root;
    for (int i = 0; i < JOINT_COUNT; ++i) {
        a->angles.current[i] = a->angles.target[i] = c->joints[i].initial;
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
    m->selected = m->held = m->release_contact = m->capture_target = -1;
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
        ThemeAlienDesign alien_design = theme_alien_default_design(c->prize_colors[i]);
        Geometry *alien = theme_alien_create_design(&alien_design);
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

enum { IK_RRT_MAX_NODES = 1200 };
#define IK_RRT_MAX_ANGLE_STEP .5f
typedef struct {
    float angle[JOINT_COUNT];
    int parent;
} IkNode;

static bool pose_valid_at(PrizeMachine *m, const float angle[JOINT_COUNT]) {
    float saved[JOINT_COUNT];
    memcpy(saved, m->arm.angles.current, sizeof(saved));
    memcpy(m->arm.angles.current, angle, sizeof(saved));
    update_world(m);
    bool valid = machine_pose_valid(m);
    memcpy(m->arm.angles.current, saved, sizeof(saved));
    update_world(m);
    return valid;
}

static bool ik_segment_valid(PrizeMachine *m, const float from[JOINT_COUNT],
                             const float to[JOINT_COUNT]) {
    float largest = 0;
    for (int joint = 0; joint < JOINT_COUNT; ++joint)
        largest = fmaxf(largest, fabsf(to[joint] - from[joint]));
    int steps = (int)ceilf(largest / (float)IK_RRT_MAX_ANGLE_STEP);
    if (steps < 1)
        steps = 1;
    float pose[JOINT_COUNT];
    for (int step = 1; step <= steps; ++step) {
        float fraction = (float)step / (float)steps;
        for (int joint = 0; joint < JOINT_COUNT; ++joint)
            pose[joint] = from[joint] + (to[joint] - from[joint]) * fraction;
        if (!pose_valid_at(m, pose))
            return false;
    }
    return true;
}

static uint32_t ik_random(uint32_t *state) {
    uint32_t value = *state;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    *state = value ? value : UINT32_C(0x6d2b79f5);
    return *state;
}

static float ik_random_range(uint32_t *state, float minimum, float maximum) {
    float unit = (float)(ik_random(state) >> 8) / 16777215.0f;
    return minimum + (maximum - minimum) * unit;
}

static float ik_joint_distance_squared(const float a[JOINT_COUNT],
                                       const float b[JOINT_COUNT]) {
    float distance = 0;
    for (int joint = 0; joint < JOINT_COUNT; ++joint) {
        float delta = a[joint] - b[joint];
        distance += delta * delta;
    }
    return distance;
}

static float ik_orientation_error(Mat4 current, Mat4 target) {
    static const Vec3 basis[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    float trace = 0;
    for (int axis = 0; axis < 3; ++axis)
        trace += v3_dot(v3_normalize(mat4_vector(current, basis[axis])),
                        v3_normalize(mat4_vector(target, basis[axis])));
    float cosine = cg_clamp((trace - 1.0f) * .5f, -1.0f, 1.0f);
    return acosf(cosine) * 57.29577951308232f;
}

static float ik_pose_score(float position_error, float orientation_error, bool orientation_active) {
    if (!orientation_active)
        return position_error;
    const float orientation_lever = 1.5f;
    float weighted_orientation = cg_radians(orientation_error) * orientation_lever;
    return sqrtf(position_error * position_error + weighted_orientation * weighted_orientation);
}

static bool ik_goal_reached(float position_error, float orientation_error,
                            bool orientation_active) {
    return position_error <= MACHINE_IK_POSITION_TOLERANCE &&
           (!orientation_active || orientation_error <= MACHINE_IK_ORIENTATION_TOLERANCE);
}

static Mat4 prize_grip_orientation(const Prize *prize) {
    Mat4 alien = mat4_identity();
    Mat4 world = prize->node->world;
    for (int column = 0; column < 3; ++column) {
        Vec3 axis = v3_normalize(vec3(world.m[column * 4], world.m[column * 4 + 1],
                                      world.m[column * 4 + 2]));
        alien.m[column * 4] = axis.x;
        alien.m[column * 4 + 1] = axis.y;
        alien.m[column * 4 + 2] = axis.z;
    }
    /* The gripper's local +Y points down toward the prize at a valid grasp. */
    return mat4_mul(alien, mat4_rotate_z(180));
}

static bool plan_ik_path(PrizeMachine *m, const float start[JOINT_COUNT],
                         const float goal[JOINT_COUNT], int *path_count,
                         float path[MACHINE_IK_PATH_MAX][JOINT_COUNT]) {
    if (!pose_valid_at(m, start) || !pose_valid_at(m, goal))
        return false;
    if (ik_segment_valid(m, start, goal)) {
        memcpy(path[0], goal, sizeof(path[0]));
        *path_count = 1;
        return true;
    }

    IkNode nodes[IK_RRT_MAX_NODES];
    memcpy(nodes[0].angle, start, sizeof(nodes[0].angle));
    nodes[0].parent = -1;
    int count = 1, connection = -1;
    uint32_t random = UINT32_C(0x9e3779b9);
    for (int joint = 0; joint < JOINT_COUNT; ++joint) {
        uint32_t bits;
        memcpy(&bits, &start[joint], sizeof(bits));
        random ^= bits + UINT32_C(0x9e3779b9) + (random << 6) + (random >> 2);
        memcpy(&bits, &goal[joint], sizeof(bits));
        random ^= bits + UINT32_C(0x9e3779b9) + (random << 6) + (random >> 2);
    }
    if (!random)
        random = UINT32_C(0x6d2b79f5);

    for (int iteration = 0; iteration < IK_RRT_MAX_NODES - 1; ++iteration) {
        float sample[JOINT_COUNT];
        if (iteration % 5 == 0) {
            memcpy(sample, goal, sizeof(sample));
        } else {
            for (int joint = 0; joint < JOINT_COUNT; ++joint) {
                const JointConfig *limits = &MACHINE_CONFIG.joints[joint];
                sample[joint] = ik_random_range(&random, limits->min, limits->max);
            }
        }

        int nearest = 0;
        float nearest_distance = ik_joint_distance_squared(nodes[0].angle, sample);
        for (int node = 1; node < count; ++node) {
            float distance = ik_joint_distance_squared(nodes[node].angle, sample);
            if (distance < nearest_distance) {
                nearest = node;
                nearest_distance = distance;
            }
        }

        float largest = 0;
        for (int joint = 0; joint < JOINT_COUNT; ++joint)
            largest = fmaxf(largest, fabsf(sample[joint] - nodes[nearest].angle[joint]));
        if (largest < .01f)
            continue;
        float scale = fminf(1.0f, 14.0f / largest);
        float next[JOINT_COUNT];
        for (int joint = 0; joint < JOINT_COUNT; ++joint)
            next[joint] = nodes[nearest].angle[joint] +
                          (sample[joint] - nodes[nearest].angle[joint]) * scale;
        if (!ik_segment_valid(m, nodes[nearest].angle, next))
            continue;

        int added = count++;
        memcpy(nodes[added].angle, next, sizeof(nodes[added].angle));
        nodes[added].parent = nearest;
        float goal_delta = 0;
        for (int joint = 0; joint < JOINT_COUNT; ++joint)
            goal_delta = fmaxf(goal_delta, fabsf(goal[joint] - next[joint]));
        if (goal_delta <= 18.0f && ik_segment_valid(m, next, goal)) {
            connection = added;
            break;
        }
    }
    if (connection < 0)
        return false;

    int reverse[IK_RRT_MAX_NODES], route_count = 0;
    for (int node = connection; node > 0; node = nodes[node].parent)
        reverse[route_count++] = node;
    float route[IK_RRT_MAX_NODES + 1][JOINT_COUNT];
    int route_length = 0;
    for (int i = route_count - 1; i >= 0; --i)
        memcpy(route[route_length++], nodes[reverse[i]].angle, sizeof(route[0]));
    memcpy(route[route_length++], goal, sizeof(route[0]));

    /* Shortcut the RRT chain so animation remains smooth and the stored path
     * stays small. Every shortcut is checked against the same collision model. */
    float current[JOINT_COUNT];
    memcpy(current, start, sizeof(current));
    int stored = 0, cursor = 0;
    while (cursor < route_length && stored < MACHINE_IK_PATH_MAX) {
        int furthest = route_length - 1;
        bool valid_segment = false;
        while (furthest > cursor) {
            valid_segment = ik_segment_valid(m, current, route[furthest]);
            if (valid_segment)
                break;
            --furthest;
        }
        if (!valid_segment)
            valid_segment = ik_segment_valid(m, current, route[furthest]);
        if (!valid_segment)
            return false;
        memcpy(path[stored++], route[furthest], sizeof(path[0]));
        memcpy(current, route[furthest], sizeof(current));
        cursor = furthest + 1;
    }
    if (cursor < route_length || stored == 0)
        return false;
    *path_count = stored;
    return true;
}

static float solve_ik_seed(PrizeMachine *m, Vec3 target, const Mat4 *target_orientation,
                           const float seed[JOINT_COUNT], float solution[JOINT_COUNT],
                           float *position_error, float *orientation_error) {
    enum { MAX_ITERATIONS = 120 };
    const float max_step_degrees = 18.0f;
    const float radians_to_degrees = 57.29577951308232f;
    const bool orientation_active = target_orientation != NULL;
    static const Vec3 basis[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    memcpy(m->arm.angles.current, seed, sizeof(m->arm.angles.current));
    update_world(m);
    memcpy(solution, seed, sizeof(m->arm.angles.current));
    float best_position_error = v3_length(v3_sub(machine_grip_position(m), target));
    float best_orientation_error =
        orientation_active ? ik_orientation_error(m->arm.grip->world, *target_orientation) : 0;
    float best_score =
        ik_pose_score(best_position_error, best_orientation_error, orientation_active);

    /* Each joint step minimizes position error and, when requested, full tool
     * orientation using the same one-dimensional rotation about that joint. */
    for (int iteration = 0;
         iteration < MAX_ITERATIONS &&
         !ik_goal_reached(best_position_error, best_orientation_error, orientation_active);
         ++iteration) {
        for (int joint = JOINT_COUNT - 1; joint >= 0; --joint) {
            update_world(m);
            SceneNode *node = m->arm.joint[joint];
            Mat4 parent_world = node->parent ? node->parent->world : mat4_identity();
            Vec3 local_axis = joint == 0 ? vec3(0, 1, 0) : vec3(0, 0, 1);
            Vec3 axis = v3_normalize(mat4_vector(parent_world, local_axis));
            Vec3 origin = mat4_point(node->world, vec3(0, 0, 0));
            Vec3 end_vector = v3_sub(machine_grip_position(m), origin);
            Vec3 target_vector = v3_sub(target, origin);
            end_vector = v3_sub(end_vector, v3_scale(axis, v3_dot(end_vector, axis)));
            target_vector = v3_sub(target_vector, v3_scale(axis, v3_dot(target_vector, axis)));
            float sine = v3_dot(axis, v3_cross(end_vector, target_vector));
            float cosine = v3_dot(end_vector, target_vector);
            if (orientation_active) {
                const float orientation_weight = 2.25f; /* 1.5 unit length squared. */
                for (int local_axis = 0; local_axis < 3; ++local_axis) {
                    Vec3 current_axis =
                        v3_normalize(mat4_vector(m->arm.grip->world, basis[local_axis]));
                    Vec3 desired_axis =
                        v3_normalize(mat4_vector(*target_orientation, basis[local_axis]));
                    Vec3 current_projected =
                        v3_sub(current_axis, v3_scale(axis, v3_dot(current_axis, axis)));
                    Vec3 desired_projected =
                        v3_sub(desired_axis, v3_scale(axis, v3_dot(desired_axis, axis)));
                    sine += orientation_weight * v3_dot(axis, v3_cross(current_axis, desired_axis));
                    cosine += orientation_weight * v3_dot(current_projected, desired_projected);
                }
            }
            if (fabsf(sine) < 1e-8f && fabsf(cosine) < 1e-8f)
                continue;

            float delta = atan2f(sine, cosine) * radians_to_degrees;
            delta = cg_clamp(delta, -max_step_degrees, max_step_degrees);
            const JointConfig *limits = &MACHINE_CONFIG.joints[joint];
            m->arm.angles.current[joint] =
                cg_clamp(m->arm.angles.current[joint] + delta, limits->min, limits->max);
            update_world(m);
        }

        float candidate_position_error = v3_length(v3_sub(machine_grip_position(m), target));
        float candidate_orientation_error =
            orientation_active ? ik_orientation_error(m->arm.grip->world, *target_orientation) : 0;
        float candidate_score =
            ik_pose_score(candidate_position_error, candidate_orientation_error,
                          orientation_active);
        if (candidate_score < best_score) {
            best_position_error = candidate_position_error;
            best_orientation_error = candidate_orientation_error;
            best_score = candidate_score;
            memcpy(solution, m->arm.angles.current, sizeof(m->arm.angles.current));
        }
    }
    if (position_error)
        *position_error = best_position_error;
    if (orientation_error)
        *orientation_error = best_orientation_error;
    return best_score;
}

static bool machine_solve_ik_internal(PrizeMachine *m, Vec3 target,
                                      const Mat4 *target_orientation,
                                      float *position_error, float *orientation_error) {
    const bool orientation_active = target_orientation != NULL;
    if (!m || m->round != ROUND_PLAYING || !isfinite(target.x) || !isfinite(target.y) ||
        !isfinite(target.z))
        return false;

    AABB workspace = MACHINE_CONFIG.workspace;
    if (target.x < workspace.min.x || target.x > workspace.max.x || target.y < workspace.min.y ||
        target.y > workspace.max.y || target.z < workspace.min.z || target.z > workspace.max.z) {
        message(m, "Alvo fora da area de trabalho.");
        return false;
    }

    float original[JOINT_COUNT], best[JOINT_COUNT];
    memcpy(original, m->arm.angles.current, sizeof(original));
    memcpy(best, original, sizeof(best));
    float best_position_error = v3_length(v3_sub(machine_grip_position(m), target));
    float best_orientation_error =
        orientation_active ? ik_orientation_error(m->arm.grip->world, *target_orientation) : 0;
    float best_score =
        ik_pose_score(best_position_error, best_orientation_error, orientation_active);
    float candidates[16][JOINT_COUNT], candidate_scores[16], candidate_positions[16];
    float candidate_orientations[16];
    float path[MACHINE_IK_PATH_MAX][JOINT_COUNT], selected[JOINT_COUNT];
    int candidate_count = 0;
    int path_count = 0;
    float selected_position_error = best_position_error;
    float selected_orientation_error = best_orientation_error;
    bool planned = false;
    static const float restart_poses[][JOINT_COUNT] = {
        {0, -25, -85, -70}, {0, -45, -85, -50}, {55, -20, -100, -30},
        {-55, -20, -100, -30}, {0, -70, -100, -80}, {0, 0, -45, 0},
        {0, -80, -130, -100}, {45, -45, -85, -50}, {-45, -45, -85, -50},
        {60, 20, -20, 60}, {-70, -20, -50, 45}, {15, -60, -120, 25},
    };

    for (int attempt = -1; attempt < (int)(sizeof(restart_poses) / sizeof(restart_poses[0]));
         ++attempt) {
        float seed[JOINT_COUNT], solution[JOINT_COUNT];
        if (attempt < 0) {
            memcpy(seed, original, sizeof(seed));
        } else {
            for (int joint = 0; joint < JOINT_COUNT; ++joint) {
                const JointConfig *limits = &MACHINE_CONFIG.joints[joint];
                seed[joint] = cg_clamp(restart_poses[attempt][joint], limits->min, limits->max);
            }
        }
        float candidate_position_error, candidate_orientation_error;
        float candidate_score = solve_ik_seed(m, target, target_orientation, seed, solution,
                                               &candidate_position_error,
                                               &candidate_orientation_error);
        if (candidate_score < best_score) {
            best_score = candidate_score;
            best_position_error = candidate_position_error;
            best_orientation_error = candidate_orientation_error;
            memcpy(best, solution, sizeof(best));
        }
        if (!ik_goal_reached(candidate_position_error, candidate_orientation_error,
                             orientation_active) ||
            !pose_valid_at(m, solution))
            continue;
        bool duplicate = false;
        for (int existing = 0; existing < candidate_count; ++existing)
            if (ik_joint_distance_squared(candidates[existing], solution) < 4.0f) {
                duplicate = true;
                if (candidate_score < candidate_scores[existing]) {
                    memcpy(candidates[existing], solution, sizeof(solution));
                    candidate_scores[existing] = candidate_score;
                    candidate_positions[existing] = candidate_position_error;
                    candidate_orientations[existing] = candidate_orientation_error;
                }
                break;
            }
        if (!duplicate && candidate_count < 16) {
            memcpy(candidates[candidate_count], solution, sizeof(solution));
            candidate_scores[candidate_count] = candidate_score;
            candidate_positions[candidate_count] = candidate_position_error;
            candidate_orientations[candidate_count] = candidate_orientation_error;
            ++candidate_count;
        }
        if (attempt < 0 &&
            ik_goal_reached(candidate_position_error, candidate_orientation_error,
                            orientation_active) &&
            !duplicate) {
            planned = plan_ik_path(m, original, solution, &path_count, path);
            if (planned) {
                memcpy(selected, solution, sizeof(selected));
                selected_position_error = candidate_position_error;
                selected_orientation_error = candidate_orientation_error;
            }
        }
    }

    /* Keep the nearest collision-free approximation as a fallback for targets
     * outside the arm's true reach, if it can be approached safely. */
    if (candidate_count == 0 && pose_valid_at(m, best)) {
        memcpy(candidates[0], best, sizeof(best));
        candidate_scores[0] = best_score;
        candidate_positions[0] = best_position_error;
        candidate_orientations[0] = best_orientation_error;
        candidate_count = 1;
    }

    bool attempted[16] = {false};
    for (int attempt = 0; attempt < candidate_count && !planned; ++attempt) {
        int nearest = -1;
        float nearest_distance = FLT_MAX;
        for (int candidate = 0; candidate < candidate_count; ++candidate) {
            if (attempted[candidate])
                continue;
            float distance = ik_joint_distance_squared(original, candidates[candidate]);
            if (distance < nearest_distance) {
                nearest = candidate;
                nearest_distance = distance;
            }
        }
        if (nearest < 0)
            break;
        attempted[nearest] = true;
        planned = plan_ik_path(m, original, candidates[nearest], &path_count, path);
        if (planned) {
            memcpy(selected, candidates[nearest], sizeof(selected));
            selected_position_error = candidate_positions[nearest];
            selected_orientation_error = candidate_orientations[nearest];
        }
    }

    /* The search is instantaneous; normal frame updates animate along a
     * collision-checked joint-space route to the best bounded solution. */
    memcpy(m->arm.angles.current, original, sizeof(original));
    update_world(m);
    if (!planned) {
        m->ik.path_count = m->ik.path_index = 0;
        memcpy(m->arm.angles.target, original, sizeof(original));
        m->ik.target = target;
        m->ik.target_orientation = orientation_active ? *target_orientation : mat4_identity();
        m->ik.orientation_active = orientation_active;
        m->ik.error = v3_length(v3_sub(machine_grip_position(m), target));
        m->ik.orientation_error =
            orientation_active
                ? ik_orientation_error(m->arm.grip->world, *target_orientation)
                : 0;
        m->ik.target_active = true;
        if (position_error)
            *position_error = m->ik.error;
        if (orientation_error)
            *orientation_error = m->ik.orientation_error;
        message(m, "IK: nao encontrei uma rota livre de colisao.");
        return false;
    }
    memcpy(m->ik.path, path, (size_t)path_count * sizeof(path[0]));
    m->ik.path_count = path_count;
    m->ik.path_index = 1;
    memcpy(m->arm.angles.target, m->ik.path[0], sizeof(m->arm.angles.target));
    m->ik.target = target;
    m->ik.target_orientation = orientation_active ? *target_orientation : mat4_identity();
    m->ik.orientation_active = orientation_active;
    m->ik.error = v3_length(v3_sub(machine_grip_position(m), target));
    m->ik.orientation_error =
        orientation_active ? ik_orientation_error(m->arm.grip->world, *target_orientation) : 0;
    m->ik.target_active = true;
    if (position_error)
        *position_error = selected_position_error;
    if (orientation_error)
        *orientation_error = selected_orientation_error;
    bool reached = ik_goal_reached(selected_position_error, selected_orientation_error,
                                   orientation_active);
    if (reached)
        message(m, "IK: alvo alcancavel; movimento em andamento.");
    else
        message(m, "IK: aproximando o maximo; ajuste as juntas se necessario.");
    return reached;
}

static bool valid_rotation(Mat4 orientation) {
    Vec3 axes[3] = {mat4_vector(orientation, vec3(1, 0, 0)),
                    mat4_vector(orientation, vec3(0, 1, 0)),
                    mat4_vector(orientation, vec3(0, 0, 1))};
    for (int i = 0; i < 3; ++i) {
        if (!isfinite(axes[i].x) || !isfinite(axes[i].y) || !isfinite(axes[i].z) ||
            fabsf(v3_length(axes[i]) - 1.0f) > .01f)
            return false;
        for (int j = 0; j < i; ++j)
            if (fabsf(v3_dot(axes[i], axes[j])) > .01f)
                return false;
    }
    return v3_dot(axes[0], v3_cross(axes[1], axes[2])) > .99f;
}

bool machine_solve_ik(PrizeMachine *m, Vec3 target, float *error) {
    return machine_solve_ik_internal(m, target, NULL, error, NULL);
}

bool machine_solve_ik_pose(PrizeMachine *m, Vec3 target, Mat4 target_orientation,
                           float *position_error, float *orientation_error) {
    if (!valid_rotation(target_orientation))
        return false;
    return machine_solve_ik_internal(m, target, &target_orientation, position_error,
                                     orientation_error);
}

bool machine_solve_ik_capture(PrizeMachine *m, int prize, float *position_error,
                              float *orientation_error) {
    if (!m || prize < 0 || prize >= PRIZE_COUNT ||
        m->prizes[prize].state != PRIZE_AVAILABLE)
        return false;
    Vec3 target = mat4_point(m->prizes[prize].node->world, vec3(0, 0, 0));
    Mat4 orientation = prize_grip_orientation(&m->prizes[prize]);
    return machine_solve_ik_pose(m, target, orientation, position_error, orientation_error);
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
        AABB bounds = scene_bounds(m->solids[i].node);
        if (bounds.intersects_ray(&bounds, ray, &t) && t < length - .002f)
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
            AABB local = node->geometry->bounds;
            local.transform(&local, mat4_mul(inverse, node->world));
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
typedef struct {
    Vec3 center, axis[3];
    float half[3];
} OrientedBounds;

enum {
    ARM_UPPER_LINK = 0,
    ARM_FOREARM = 1,
    ARM_PALM = 2,
    ARM_FINGER_LEFT = 3,
    ARM_FINGER_RIGHT = 4,
    ARM_SHOULDER_HUB = 5,
    ARM_ELBOW_HUB = 6,
    ARM_WRIST_HUB = 7,
    ARM_BASE_ROTOR = 8
};

static bool oriented_bounds(const SceneNode *node, OrientedBounds *bounds) {
    if (!node || !node->geometry || !bounds)
        return false;
    AABB local = node->geometry->bounds;
    Vec3 local_center = v3_scale(v3_add(local.min, local.max), .5f);
    Vec3 local_half = v3_scale(v3_sub(local.max, local.min), .5f);
    bounds->center = mat4_point(node->world, local_center);
    Vec3 columns[3] = {vec3(node->world.m[0], node->world.m[1], node->world.m[2]),
                       vec3(node->world.m[4], node->world.m[5], node->world.m[6]),
                       vec3(node->world.m[8], node->world.m[9], node->world.m[10])};
    float local_extent[3] = {local_half.x, local_half.y, local_half.z};
    for (int axis = 0; axis < 3; ++axis) {
        float scale = v3_length(columns[axis]);
        if (scale < 1e-8f)
            return false;
        bounds->axis[axis] = v3_scale(columns[axis], 1.0f / scale);
        bounds->half[axis] = local_extent[axis] * scale;
    }
    return true;
}

/* Separating-axis test for the OBBs of two arm parts. The tolerance leaves a
 * small clearance; unlike world AABBs, it does not reject harmless projections. */
static bool oriented_bounds_overlap(OrientedBounds a, OrientedBounds b, float tolerance) {
    float rotation[3][3], absolute[3][3];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) {
            rotation[i][j] = v3_dot(a.axis[i], b.axis[j]);
            absolute[i][j] = fabsf(rotation[i][j]) + 1e-6f;
        }

    Vec3 offset = v3_sub(b.center, a.center);
    float translated[3] = {v3_dot(offset, a.axis[0]), v3_dot(offset, a.axis[1]),
                           v3_dot(offset, a.axis[2])};
    for (int i = 0; i < 3; ++i) {
        float radius_a = a.half[i];
        float radius_b = 0;
        for (int j = 0; j < 3; ++j)
            radius_b += b.half[j] * absolute[i][j];
        if (fabsf(translated[i]) >= radius_a + radius_b - tolerance)
            return false;
    }
    for (int j = 0; j < 3; ++j) {
        float radius_a = 0;
        for (int i = 0; i < 3; ++i)
            radius_a += a.half[i] * absolute[i][j];
        if (fabsf(translated[0] * rotation[0][j] + translated[1] * rotation[1][j] +
                  translated[2] * rotation[2][j]) >=
            radius_a + b.half[j] - tolerance)
            return false;
    }

    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) {
            float cross_axis_length_squared = 1.0f - rotation[i][j] * rotation[i][j];
            if (cross_axis_length_squared < 1e-8f)
                continue;
            float radius_a = a.half[(i + 1) % 3] * absolute[(i + 2) % 3][j] +
                             a.half[(i + 2) % 3] * absolute[(i + 1) % 3][j];
            float radius_b = b.half[(j + 1) % 3] * absolute[i][(j + 2) % 3] +
                             b.half[(j + 2) % 3] * absolute[i][(j + 1) % 3];
            float distance = fabsf(translated[(i + 2) % 3] * rotation[(i + 1) % 3][j] -
                                   translated[(i + 1) % 3] * rotation[(i + 2) % 3][j]);
            float margin = tolerance * sqrtf(cross_axis_length_squared);
            if (distance >= radius_a + radius_b - margin)
                return false;
        }
    return true;
}

static bool arm_parts_share_joint(int first, int second) {
    if (first > second) {
        int swap = first;
        first = second;
        second = swap;
    }
    return (first == ARM_UPPER_LINK &&
            (second == ARM_FOREARM || second == ARM_SHOULDER_HUB ||
             second == ARM_ELBOW_HUB || second == ARM_BASE_ROTOR)) ||
           (first == ARM_FOREARM &&
            (second == ARM_PALM || second == ARM_FINGER_LEFT || second == ARM_FINGER_RIGHT ||
             second == ARM_ELBOW_HUB || second == ARM_WRIST_HUB)) ||
           (first == ARM_PALM &&
            (second == ARM_FINGER_LEFT || second == ARM_FINGER_RIGHT || second == ARM_WRIST_HUB)) ||
           ((first == ARM_FINGER_LEFT || first == ARM_FINGER_RIGHT) && second == ARM_WRIST_HUB) ||
           (first == ARM_SHOULDER_HUB && second == ARM_BASE_ROTOR);
}

bool machine_pose_valid(const PrizeMachine *m) {
    float tolerance = MACHINE_CONFIG.collision_tolerance;
    int candidate = machine_capture_candidate(m, NULL);
    if (m->capture_target >= 0 && candidate != m->capture_target)
        candidate = -1;
    for (int first = 0; first < ARM_PART_COUNT; ++first) {
        OrientedBounds first_bounds;
        if (!oriented_bounds(m->arm.parts[first], &first_bounds))
            return false;
        for (int second = first + 1; second < ARM_PART_COUNT; ++second) {
            if (arm_parts_share_joint(first, second))
                continue;
            OrientedBounds second_bounds;
            if (!oriented_bounds(m->arm.parts[second], &second_bounds) ||
                oriented_bounds_overlap(first_bounds, second_bounds, tolerance))
                return false;
        }
    }
    for (int part = 0; part < ARM_PART_COUNT; ++part) {
        AABB bounds = scene_bounds(m->arm.parts[part]);
        if (!inside_workspace(bounds))
            return false;
        for (int s = 0; s < m->solid_count; ++s) {
            AABB obstacle = scene_bounds(m->solids[s].node);
            if (bounds.overlaps(&bounds, &obstacle, tolerance))
                return false;
        }
        for (int p = 0; p < PRIZE_COUNT; ++p) {
            if (m->prizes[p].state == PRIZE_HELD)
                continue;
            /* The palm is the top surface of the gripper and may touch the
             * prize being centered between the fingers. */
            if (part == 2 && p == candidate)
                continue;
            /* In the finger/prize pair only, refine the world AABB in the finger's
             * local frame. This avoids a yaw-expanded box closing the empty gap. */
            if (part == 3 || part == 4) {
                if (p == candidate || p == m->release_contact)
                    continue;
                Mat4 inverse;
                if (mat4_inverse(m->arm.parts[part]->world, &inverse)) {
                    AABB local = m->prizes[p].node->geometry->bounds;
                    local.transform(&local,
                                    mat4_mul(inverse, m->prizes[p].node->world));
                    AABB part_local = m->arm.parts[part]->geometry->bounds;
                    if (!local.overlaps(&local, &part_local, tolerance))
                        continue;
                }
            }
            AABB prize_bounds = scene_bounds(m->prizes[p].node);
            if (bounds.overlaps(&bounds, &prize_bounds, tolerance))
                return false;
        }
    }
    if (m->held >= 0) {
        AABB bounds = scene_bounds(m->prizes[m->held].node);
        if (!inside_workspace(bounds))
            return false;
        for (int s = 0; s < m->solid_count; ++s) {
            AABB obstacle = scene_bounds(m->solids[s].node);
            if (bounds.overlaps(&bounds, &obstacle, tolerance))
                return false;
        }
        for (int p = 0; p < PRIZE_COUNT; ++p)
            if (p != m->held) {
                AABB prize_bounds = scene_bounds(m->prizes[p].node);
                if (bounds.overlaps(&bounds, &prize_bounds, tolerance))
                    return false;
            }
    }
    return true;
}
void machine_target_joint(PrizeMachine *m, int joint, float degrees) {
    if (m->round != ROUND_PLAYING || joint < 0 || joint >= JOINT_COUNT || !isfinite(degrees))
        return;
    if (m->ik.path_count > 0) {
        m->ik.path_count = m->ik.path_index = 0;
        memcpy(m->arm.angles.target, m->arm.angles.current, sizeof(m->arm.angles.target));
    }
    m->ik.target_active = false;
    const JointConfig *c = &MACHINE_CONFIG.joints[joint];
    m->arm.angles.target[joint] = cg_clamp(degrees, c->min, c->max);
    if (degrees < c->min || degrees > c->max)
        message(m, "Limite da junta.");
}
void machine_nudge_joint(PrizeMachine *m, int joint, float degrees) {
    if (joint >= 0 && joint < JOINT_COUNT)
        machine_target_joint(m, joint,
                             (m->ik.path_count > 0 ? m->arm.angles.current[joint]
                                                  : m->arm.angles.target[joint]) +
                                 degrees);
}
void machine_open(PrizeMachine *m) {
    if (m->round != ROUND_PLAYING)
        return;
    m->capture_target = -1;
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
static bool start_closing(PrizeMachine *m, int capture_target) {
    if (m->round != ROUND_PLAYING)
        return false;
    if (m->held >= 0) {
        message(m, "Garra ocupada.");
        return false;
    }
    if (capture_target >= 0 && machine_capture_candidate(m, NULL) != capture_target)
        return false;
    m->capture_target = capture_target;
    m->arm.opening_target = MACHINE_CONFIG.finger_min;
    message(m, "Fechando a garra.");
    return true;
}
void machine_close(PrizeMachine *m) {
    (void)start_closing(m, -1);
}
bool machine_close_for_prize(PrizeMachine *m, int prize) {
    if (!m || prize < 0 || prize >= PRIZE_COUNT || m->prizes[prize].state != PRIZE_AVAILABLE)
        return false;
    return start_closing(m, prize);
}
static float approach(float value, float target, float step) {
    return value + cg_clamp(target - value, -step, step);
}
static void blocked(PrizeMachine *m) {
    ++m->blocked_moves;
    message(m, "Movimento bloqueado.");
}
static bool ik_waypoint_reached(const PrizeMachine *m) {
    for (int joint = 0; joint < JOINT_COUNT; ++joint)
        if (fabsf(m->arm.angles.current[joint] - m->arm.angles.target[joint]) > .0001f)
            return false;
    return true;
}
static void advance_ik_waypoint(PrizeMachine *m) {
    while (m->ik.path_count > 0 && ik_waypoint_reached(m)) {
        if (m->ik.path_index >= m->ik.path_count) {
            m->ik.path_count = m->ik.path_index = 0;
            return;
        }
        memcpy(m->arm.angles.target, m->ik.path[m->ik.path_index], sizeof(m->arm.angles.target));
        ++m->ik.path_index;
    }
}
static void move_ik_joints(PrizeMachine *m, float dt) {
    advance_ik_waypoint(m);
    if (m->ik.path_count == 0)
        return;

    float old[JOINT_COUNT], duration = 0;
    memcpy(old, m->arm.angles.current, sizeof(old));
    for (int joint = 0; joint < JOINT_COUNT; ++joint) {
        float distance = fabsf(m->arm.angles.target[joint] - old[joint]);
        float speed = MACHINE_CONFIG.joints[joint].speed;
        if (speed > 0)
            duration = fmaxf(duration, distance / speed);
    }
    if (duration <= 1e-7f) {
        advance_ik_waypoint(m);
        return;
    }
    float fraction = fminf(1.0f, dt / duration);
    for (int joint = 0; joint < JOINT_COUNT; ++joint)
        m->arm.angles.current[joint] =
            old[joint] + (m->arm.angles.target[joint] - old[joint]) * fraction;
    update_world(m);
    if (!machine_pose_valid(m)) {
        memcpy(m->arm.angles.current, old, sizeof(old));
        memcpy(m->arm.angles.target, old, sizeof(old));
        update_world(m);
        m->ik.path_count = m->ik.path_index = 0;
        blocked(m);
        return;
    }
    if (fraction >= 1.0f) {
        memcpy(m->arm.angles.current, m->arm.angles.target, sizeof(m->arm.angles.current));
        update_world(m);
        advance_ik_waypoint(m);
    }
}
static void move_joints(PrizeMachine *m, float dt) {
    if (m->ik.path_count > 0) {
        move_ik_joints(m, dt);
        return;
    }
    for (int i = 0; i < JOINT_COUNT; ++i) {
        float old = m->arm.angles.current[i];
        m->arm.angles.current[i] =
            approach(old, m->arm.angles.target[i], MACHINE_CONFIG.joints[i].speed * dt);
        if (m->arm.angles.current[i] == old)
            continue;
        update_world(m);
        if (!machine_pose_valid(m)) {
            m->arm.angles.current[i] = m->arm.angles.target[i] = old;
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
    if (m->capture_target >= 0 && candidate != m->capture_target) {
        m->capture_target = -1;
        m->arm.opening_target = old;
        message(m, "Captura cancelada: premio fora da garra.");
        return;
    }
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
        m->capture_target = -1;
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
    AABB floor = scene_bounds(m->solids[0].node);
    float center_x = (floor.min.x + floor.max.x) * .5f;
    float center_z = (floor.min.z + floor.max.z) * .5f;
    float radius_x = (floor.max.x - floor.min.x) * .5f;
    float radius_z = (floor.max.z - floor.min.z) * .5f;
    if (radius_x <= 0 || radius_z <= 0)
        return false;
    for (int p = 0; p < PRIZE_COUNT; ++p) {
        AABB box = scene_bounds(m->prizes[p].node);
        if (!inside_workspace(box) || fabsf(box.min.y) > .0001f)
            return false;
        /* The floor is an ellipse, not its enclosing AABB. Keep the whole
         * footprint inside it, with a margin for the 64-sided floor mesh. */
        for (int corner = 0; corner < 4; ++corner) {
            float x = ((corner & 1 ? box.max.x : box.min.x) - center_x) / radius_x;
            float z = ((corner & 2 ? box.max.z : box.min.z) - center_z) / radius_z;
            if (x * x + z * z > .98f)
                return false;
        }
        for (int s = 0; s < m->solid_count; ++s) {
            AABB obstacle = scene_bounds(m->solids[s].node);
            if (box.overlaps(&box, &obstacle, 0))
                return false;
        }
        for (int other = 0; other < p; ++other) {
            AABB other_box = scene_bounds(m->prizes[other].node);
            if (box.overlaps(&box, &other_box, -.12f))
                return false;
        }
    }
    return machine_pose_valid(m);
}
static bool spawn_move(PrizeMachine *m, int joint, float angle) {
    const JointConfig *config = &MACHINE_CONFIG.joints[joint];
    if (angle < config->min || angle > config->max)
        return false;
    m->arm.angles.target[joint] = angle;
    unsigned blocked_before = m->blocked_moves;
    /* Use the same swept joint steps as gameplay, without advancing the timer
     * or simulating gravity on the already-supported prizes. */
    for (int step = 0; step < 4096; ++step) {
        move_joints(m, MACHINE_CONFIG.max_step);
        if (m->blocked_moves != blocked_before)
            return false;
        if (fabsf(m->arm.angles.current[joint] - angle) < .0001f)
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
    /* Leave usable alignment room even on the smallest tray, rather than
     * accepting a spawn whose only validated drop sits exactly on an edge. */
    const float margin = .05f;
    return cargo.min.x >= tray.min.x + margin && cargo.max.x <= tray.max.x - margin &&
           cargo.min.z >= tray.min.z + margin && cargo.max.z <= tray.max.z - margin &&
           cargo.min.y > tray.min.y;
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
        if (m->ik.target_active) {
            m->ik.error = v3_length(v3_sub(machine_grip_position(m), m->ik.target));
            if (m->ik.orientation_active)
                m->ik.orientation_error =
                    ik_orientation_error(m->arm.grip->world, m->ik.target_orientation);
        }
        if (m->release_contact >= 0) {
            AABB p = scene_bounds(m->prizes[m->release_contact].node);
            AABB left_finger = scene_bounds(m->arm.parts[3]);
            AABB right_finger = scene_bounds(m->arm.parts[4]);
            if (!p.overlaps(&p, &left_finger, 0) && !p.overlaps(&p, &right_finger, 0))
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
        AABB bounds = scene_bounds(m->solids[s].node);
        if (bounds.intersects_ray(&bounds, ray, &distance))
            closest = fminf(closest, distance);
    }
    for (int p = 0; p < ARM_PART_COUNT; ++p) {
        float distance;
        AABB bounds = scene_bounds(m->arm.parts[p]);
        if (bounds.intersects_ray(&bounds, ray, &distance))
            closest = fminf(closest, distance);
    }
    /* Held/delivered prizes occlude too, although they are not selectable. */
    for (int p = 0; p < PRIZE_COUNT; ++p)
        if (m->prizes[p].state != PRIZE_AVAILABLE) {
            float distance;
            AABB bounds = scene_bounds(m->prizes[p].node);
            if (bounds.intersects_ray(&bounds, ray, &distance))
                closest = fminf(closest, distance);
        }
    for (int p = 0; p < PRIZE_COUNT; ++p)
        if (m->prizes[p].state == PRIZE_AVAILABLE) {
            float distance;
            AABB bounds = scene_bounds(m->prizes[p].node);
            if (bounds.intersects_ray(&bounds, ray, &distance) && distance < closest) {
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
    if (m->ik.path_count > 0)
        return false;
    for (int i = 0; i < JOINT_COUNT; ++i)
        if (fabsf(m->arm.angles.current[i] - m->arm.angles.target[i]) > .001f)
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
