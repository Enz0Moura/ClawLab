#include "app/machine.h"
#include "cg/camera.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int checks;
#define CHECK(condition)                                                                           \
    do {                                                                                           \
        ++checks;                                                                                  \
        if (!(condition)) {                                                                        \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);                   \
            exit(EXIT_FAILURE);                                                                    \
        }                                                                                          \
    } while (0)
static bool near(float a, float b) {
    return fabsf(a - b) < .001f;
}
static void check_vec(Vec3 a, Vec3 b) {
    CHECK(near(a.x, b.x));
    CHECK(near(a.y, b.y));
    CHECK(near(a.z, b.z));
}
static void advance(PrizeMachine *m, float seconds) {
    for (int i = 0; i < (int)ceilf(seconds * 120); ++i)
        machine_update(m, 1.0f / 120);
}
static void target(PrizeMachine *m, int joint, float value) {
    unsigned old = m->blocked_moves;
    machine_target_joint(m, joint, value);
    advance(m, 8);
    if (!near(m->arm.angle[joint], value))
        fprintf(stderr, "joint %d requested %.2f reached %.2f, blocks %u, grip %.3f %.3f %.3f\n",
                joint, value, m->arm.angle[joint], m->blocked_moves - old,
                machine_grip_position(m).x, machine_grip_position(m).y, machine_grip_position(m).z);
    CHECK(near(m->arm.angle[joint], value));
    CHECK(m->blocked_moves == old);
}
static void test_math(void) {
    check_vec(v3_cross(vec3(1, 0, 0), vec3(0, 1, 0)), vec3(0, 0, 1));
    check_vec(v3_normalize(vec3(0, 0, 0)), vec3(0, 0, 0));
    Mat4 parent = mat4_mul(mat4_translate(vec3(2, 0, 0)), mat4_rotate_z(90));
    Mat4 world = mat4_mul(parent, mat4_translate(vec3(0, 3, 0)));
    check_vec(mat4_point(world, vec3(0, 0, 0)), vec3(-1, 0, 0));
    Mat4 matrix = mat4_mul(world, mat4_scale(vec3(2, 3, .5f))), inverse;
    CHECK(mat4_inverse(matrix, &inverse));
    check_vec(mat4_point(mat4_mul(inverse, matrix), vec3(3, -2, 8)), vec3(3, -2, 8));
    CHECK(!mat4_inverse(mat4_scale(vec3(1, 0, 1)), &inverse));
    AABB unit = {{-.5f, -.5f, -.5f}, {.5f, .5f, .5f}};
    AABB rotated = aabb_transform(unit, mat4_rotate_y(45));
    CHECK(near(rotated.max.x, sqrtf(.5f)));
    CHECK(near(rotated.min.z, -sqrtf(.5f)));
    CHECK(aabb_overlap(unit, unit, 0));
    CHECK(!aabb_overlap(unit, aabb_transform(unit, mat4_translate(vec3(1, 0, 0))), 0));
    float distance;
    CHECK(ray_aabb((Ray){{0, 0, 3}, {0, 0, -1}}, unit, &distance));
    CHECK(near(distance, 2.5f));
    CHECK(!ray_aabb((Ray){{2, 0, 3}, {0, 0, -1}}, unit, &distance));
    CHECK(!ray_aabb((Ray){{0, 0, 3}, {0, 0, 1}}, unit, &distance));
    CHECK(ray_aabb((Ray){{0, 0, 0}, {1, 0, 0}}, unit, &distance));
    CHECK(near(distance, 0));
    CHECK(!ray_aabb((Ray){{0, 0, 0}, {0, 0, 0}}, unit, &distance));
    Camera c = camera_default();
    Vec3 point = vec3(1, .25f, 1), screen;
    Ray ray;
    CHECK(camera_project(&c, point, 960, 650, &screen));
    CHECK(camera_ray(&c, screen.x, screen.y, 960, 650, &ray));
    CHECK(v3_length(v3_cross(v3_sub(point, ray.origin), ray.direction)) < .001f);
    CHECK(!camera_ray(&c, 1, 1, 0, 0, &ray));
}
static void test_free_camera(void) {
    Camera c = camera_default();
    Vec3 point = c.target, screen, eye = camera_eye(&c), target_before = c.target;
    CHECK(camera_project(&c, point, 960, 650, &screen));
    camera_pan(&c, 80, -35, 650);
    Vec3 moved;
    CHECK(camera_project(&c, point, 960, 650, &moved));
    CHECK(near(moved.x - screen.x, 80));
    CHECK(near(moved.y - screen.y, -35));
    check_vec(v3_sub(camera_eye(&c), eye), v3_sub(c.target, target_before));
    Vec3 target = c.target;
    camera_pan(&c, 80, 20, 0);
    camera_pan(&c, NAN, 20, 650);
    check_vec(c.target, target);
    eye = camera_eye(&c);
    camera_look(&c, 38, -60);
    check_vec(camera_eye(&c), eye);
    CHECK(c.pitch < 0);
    Ray ray;
    CHECK(camera_ray(&c, 480, 325, 960, 650, &ray));
    CHECK(v3_dot(ray.direction, v3_normalize(v3_sub(c.target, eye))) > .999f);
    camera_dolly(&c, 2);
    CHECK(near(v3_length(v3_sub(camera_eye(&c), eye)), 2));
    CHECK(near(c.distance, camera_default().distance));
    camera_dolly(&c, -2);
    check_vec(camera_eye(&c), eye);
    camera_look(&c, 720, 1000);
    CHECK(near(c.pitch, 89));
    CHECK(camera_ray(&c, 480, 325, 960, 650, &ray));
    camera_look(&c, 0, -1000);
    CHECK(near(c.pitch, -89));
    CHECK(camera_ray(&c, 480, 325, 960, 650, &ray));
    check_vec(camera_eye(&c), eye);
}
static void test_scene(void) {
    Box *box = BoxFabric.create((Color){1, 0, 0});
    Cylinder *cylinder = CylinderFabric.create((Color){0, 1, 0}, 16);
    CHECK(box && cylinder);
    CHECK(!CylinderFabric.create((Color){0, 0, 0}, 2));
    Geometry *base = &box->base;
    CHECK((void *)box == (void *)base);
    CHECK(base->set_color(base, (Color){0, 0, 1}) == base);
    CHECK(base->color.b == 1);
    SceneNode *root = scene_node_create("root", NULL), *joint = scene_node_create("joint", NULL);
    SceneNode *mesh = scene_node_create("box", base),
              *child = scene_node_create("cylinder", &cylinder->base);
    CHECK(root && joint && mesh && child);
    CHECK(scene_node_add(root, joint));
    CHECK(scene_node_add(joint, mesh));
    CHECK(scene_node_add(joint, child));
    CHECK(!scene_node_add(child, root));
    CHECK(!scene_node_add(root, child));
    joint->local = mat4_rotate_z(90);
    mesh->local = mat4_scale(vec3(1, 20, 1));
    child->local = mat4_translate(vec3(0, 2, 0));
    scene_update(root, mat4_identity());
    check_vec(mat4_point(child->world, vec3(0, 0, 0)), vec3(-2, 0, 0));
    scene_node_destroy(root);
}
static void test_mesh_contract(void) {
    MeshVertex vertices[] = {{{-1, 0, 0}, {0, 0, 1}, {1, 0, 0}},
                             {{1, 0, 0}, {0, 0, 1}, {0, 1, 0}},
                             {{0, 1, 0}, {0, 0, 1}, {0, 0, 1}}};
    CHECK(!MeshFabric.create(NULL, 3));
    CHECK(!MeshFabric.create(vertices, 2));
    Mesh *mesh = MeshFabric.create(vertices, 3);
    CHECK(mesh && mesh->vertex_count == 3);
    check_vec(mesh->base.bounds.min, vec3(-1, 0, 0));
    check_vec(mesh->base.bounds.max, vec3(1, 1, 0));
    vertices[0].position.x = 9;
    CHECK(near(mesh->vertices[0].position.x, -1));
    mesh->base.destroy(&mesh->base);
    vertices[0].position.x = NAN;
    CHECK(!MeshFabric.create(vertices, 3));
    PrizeMachine *m = machine_create();
    CHECK(m);
    for (int p = 0; p < PRIZE_COUNT; ++p) {
        const Mesh *alien = (const Mesh *)m->prizes[p].node->geometry;
        bool enclosed = true, unit_normals = true;
        for (size_t i = 0; i < alien->vertex_count; ++i) {
            Vec3 v = alien->vertices[i].position;
            enclosed =
                enclosed && fabsf(v.x) <= .50001f && fabsf(v.y) <= .50001f && fabsf(v.z) <= .50001f;
            unit_normals = unit_normals && near(v3_length(alien->vertices[i].normal), 1);
        }
        CHECK(enclosed);
        CHECK(unit_normals);
    }
    Vec3 p = mat4_point(m->prizes[1].node->world, vec3(0, 0, 0));
    CHECK(machine_pick(m, (Ray){{p.x, 20, p.z}, {0, -1, 0}}) == -1);
    machine_destroy(m);
}
static void pickup(PrizeMachine *m, int p) {
    target(m, 0, m->prizes[p].pickup_pose[0]);
    target(m, 2, m->prizes[p].pickup_pose[2]);
    target(m, 3, m->prizes[p].pickup_pose[3]);
    target(m, 1, m->prizes[p].pickup_pose[1]);
    CHECK(machine_capture_candidate(m, NULL) == p);
    Vec3 before = mat4_point(m->prizes[p].node->world, vec3(0, 0, 0));
    machine_close(m);
    advance(m, 2);
    CHECK(m->held == p);
    CHECK(m->prizes[p].state == PRIZE_HELD);
    check_vec(mat4_point(m->prizes[p].node->world, vec3(0, 0, 0)), before);
    target(m, 1, -25);
    target(m, 3, -70);
    target(m, 2, -85);
    Mat4 expected = mat4_mul(m->arm.grip->world, m->prizes[p].relative_to_grip);
    for (int i = 0; i < 16; ++i)
        CHECK(near(expected.m[i], m->prizes[p].node->world.m[i]));
}
static void test_full_game(void) {
    PrizeMachine *m = machine_create();
    CHECK(m);
    CHECK(machine_pose_valid(m));
    CHECK(m->held == -1 && m->selected == -1 && m->delivered == 0);
    machine_close(m);
    advance(m, 2);
    CHECK(m->held == -1);
    CHECK(near(m->arm.opening, 0));
    machine_open(m);
    advance(m, 2);
    Camera c = camera_default();
    /* Random spawns can be hidden by the arm in the default view. Face the
     * open side before testing selection, just as a player can orbit there. */
    camera_orbit(&c, -30, 0);
    for (int p = 0; p < PRIZE_COUNT; ++p) {
        Vec3 screen;
        Ray ray;
        CHECK(camera_project(&c, mat4_point(m->prizes[p].node->world, vec3(0, 0, 0)), 960, 650,
                             &screen));
        CHECK(camera_ray(&c, screen.x, screen.y, 960, 650, &ray));
        CHECK(machine_pick(m, ray) == p);
        pickup(m, p);
        target(m, 0, MACHINE_CONFIG.drop_yaw[p]);
        CHECK(m->delivered == p);
        advance(m, 2);
        CHECK(m->delivered == p);
        Mat4 before = m->prizes[p].node->world;
        machine_open(m);
        CHECK(m->held == -1);
        CHECK(m->prizes[p].state == PRIZE_FALLING);
        for (int i = 0; i < 16; ++i)
            CHECK(near(before.m[i], m->prizes[p].node->world.m[i]));
        advance(m, 3);
        CHECK(m->prizes[p].state == PRIZE_DELIVERED);
        CHECK(m->delivered == p + 1);
        advance(m, 3);
        CHECK(m->delivered == p + 1);
        CHECK(near(m->arm.opening, 40));
    }
    unsigned blocked = m->blocked_moves;
    CHECK(blocked == 0);
    CHECK(machine_reset(m));
    CHECK(m->delivered == 0 && m->held == -1 && m->selected == -1);
    for (int p = 0; p < PRIZE_COUNT; ++p)
        CHECK(m->prizes[p].state == PRIZE_AVAILABLE);
    /* Opening outside the tray leaves a prize available on its support. */
    pickup(m, 1);
    machine_open(m);
    advance(m, 3);
    CHECK(m->delivered == 0 && m->prizes[1].state == PRIZE_AVAILABLE);
    CHECK(near(scene_bounds(m->prizes[1].node).min.y, 0));
    machine_destroy(m);
}
static void test_limits_collision_time(void) {
    PrizeMachine *m = machine_create(), *n = machine_create();
    CHECK(m && n);
    for (int i = 0; i < JOINT_COUNT; ++i) {
        CHECK(machine_reset(m));
        machine_target_joint(m, i, 1000);
        CHECK(near(m->arm.target[i], MACHINE_CONFIG.joints[i].max));
        advance(m, 10);
        CHECK(m->arm.angle[i] <= MACHINE_CONFIG.joints[i].max);
        CHECK(machine_pose_valid(m));
        CHECK(machine_reset(m));
        machine_target_joint(m, i, -1000);
        CHECK(near(m->arm.target[i], MACHINE_CONFIG.joints[i].min));
        advance(m, 10);
        CHECK(m->arm.angle[i] >= MACHINE_CONFIG.joints[i].min);
        CHECK(machine_pose_valid(m));
    }
    CHECK(machine_reset(m));
    target(m, 2, -70);
    target(m, 2, -85);
    target(m, 3, -60);
    target(m, 3, -70);
    Vec3 before = machine_grip_position(m);
    target(m, 0, 20);
    CHECK(v3_length(v3_sub(before, machine_grip_position(m))) > .5f);
    CHECK(machine_reset(m));
    machine_target_joint(m, 0, 30);
    machine_target_joint(n, 0, 30);
    for (int i = 0; i < 30; ++i)
        machine_update(m, 1.0f / 30);
    for (int i = 0; i < 144; ++i)
        machine_update(n, 1.0f / 144);
    CHECK(near(m->arm.angle[0], n->arm.angle[0]));
    CHECK(machine_reset(m));
    machine_target_joint(m, 1, -85);
    advance(m, 5);
    CHECK(m->blocked_moves > 0);
    CHECK(m->arm.angle[1] > -85);
    CHECK(machine_pose_valid(m));
    /* The back wall occludes a ray from outside the cabinet. */
    m->prizes[2].node->local =
        mat4_mul(mat4_translate(vec3(1, .25f, -2)), mat4_scale(vec3(.5f, .5f, .5f)));
    scene_update(m->prizes[2].node, mat4_identity());
    CHECK(machine_pick(m, (Ray){{1, .25f, -7}, {0, 0, 1}}) == -1);
    for (int i = 0; i < 100; ++i) {
        CHECK(machine_reset(m));
        CHECK(machine_pose_valid(m));
    }
    machine_destroy(m);
    machine_destroy(n);
}
static void test_occlusion_and_cargo(void) {
    PrizeMachine *m = machine_create();
    CHECK(m);
    /* Nearest eligible box wins, but an ineligible box still blocks the ray. */
    for (int i = 0; i < PRIZE_COUNT; ++i) {
        m->prizes[i].node->local =
            mat4_mul(mat4_translate(vec3(2 - (float)i, 3, 0)), mat4_scale(vec3(.5f, .5f, .5f)));
        scene_update(m->prizes[i].node, mat4_identity());
    }
    Ray ray = {{3, 3, 0}, {-1, 0, 0}};
    CHECK(machine_pick(m, ray) == 0);
    m->prizes[0].state = PRIZE_DELIVERED;
    CHECK(machine_pick(m, ray) == -1);
    CHECK(machine_reset(m));
    pickup(m, 1);
    AABB cargo = scene_bounds(m->prizes[1].node);
    Vec3 center = v3_scale(v3_add(cargo.min, cargo.max), .5f);
    center.y = cargo.min.y + .02f;
    SceneNode *barrier = m->solids[5].node;
    barrier->local = mat4_mul(mat4_translate(center), mat4_scale(vec3(.04f, .04f, .04f)));
    scene_update(barrier, mat4_identity());
    for (int i = 0; i < ARM_PART_COUNT; ++i)
        CHECK(!aabb_overlap(scene_bounds(barrier), scene_bounds(m->arm.parts[i]), 0));
    CHECK(!machine_pose_valid(m)); /* Only the carried prize hits this obstacle. */
    CHECK(machine_reset(m));
    target(m, 0, m->prizes[1].pickup_pose[0]);
    target(m, 2, m->prizes[1].pickup_pose[2]);
    target(m, 3, m->prizes[1].pickup_pose[3]);
    target(m, 1, m->prizes[1].pickup_pose[1]);
    CHECK(machine_capture_candidate(m, NULL) == 1);
    m->prizes[1].node->local.m[12] += .06f;
    scene_update(m->prizes[1].node, mat4_identity());
    CHECK(machine_capture_candidate(m, NULL) == 1);
    Vec3 grip = machine_grip_position(m),
         prize = mat4_point(m->prizes[1].node->world, vec3(0, 0, 0));
    barrier = m->solids[5].node;
    barrier->local = mat4_mul(mat4_translate(v3_scale(v3_add(grip, prize), .5f)),
                              mat4_scale(vec3(.006f, .02f, .02f)));
    scene_update(barrier, mat4_identity());
    CHECK(machine_capture_candidate(m, NULL) == -1);
    machine_destroy(m);
}
static void place_prize(PrizeMachine *m, int p, Vec3 position, Vec3 velocity) {
    m->prizes[p].node->local = mat4_mul(mat4_translate(position), mat4_scale(vec3(.5f, .5f, .5f)));
    scene_update(m->prizes[p].node, mat4_identity());
    m->prizes[p].state = PRIZE_FALLING;
    m->prizes[p].velocity = velocity;
}
static void test_difficulty_trays(void) {
    PrizeMachine *m = machine_create_seeded(MODE_SANDBOX, 42);
    CHECK(m);
    Vec3 previous_size = vec3(100, 0, 100);
    for (int mode = MODE_SANDBOX; mode < MODE_COUNT; ++mode) {
        CHECK(machine_start(m, (GameMode)mode));
        AABB tray = scene_bounds(m->solids[m->tray_solid].node);
        Vec3 size = v3_sub(tray.max, tray.min);
        CHECK(size.x < previous_size.x && size.z < previous_size.z);
        check_vec(v3_scale(v3_add(tray.min, tray.max), .5f), MACHINE_CONFIG.tray_center);
        CHECK(near(tray.max.y, .12f));
        AABB region = machine_delivery_region(m);
        CHECK(near(region.min.x, tray.min.x) && near(region.max.x, tray.max.x));
        CHECK(near(region.min.z, tray.min.z) && near(region.max.z, tray.max.z));
        CHECK(near(region.min.y, tray.max.y));
        CHECK(machine_reset(m));
        AABB reset_tray = scene_bounds(m->solids[m->tray_solid].node);
        check_vec(reset_tray.min, tray.min);
        check_vec(reset_tray.max, tray.max);
        previous_size = size;

        /* Isolate the edge rule from random floor positions. A partly overhanging
         * prize may rest on the tray, but must not earn a delivery. */
        place_prize(m, 1, vec3(-1, .25f, -1.5f), vec3(0, 0, 0));
        place_prize(m, 2, vec3(-1, .25f, -2.5f), vec3(0, 0, 0));
        float half = MACHINE_CONFIG.prize_size * .5f;
        Vec3 edge = vec3(tray.max.x - half + .03f, 1, MACHINE_CONFIG.tray_center.z);
        place_prize(m, 0, edge, vec3(0, 0, 0));
        advance(m, 12);
        CHECK(m->prizes[0].state == PRIZE_AVAILABLE && m->delivered == 0);
        CHECK(near(scene_bounds(m->prizes[0].node).min.y, tray.max.y));
        edge.x = tray.max.x - half - .05f;
        place_prize(m, 0, edge, vec3(0, 0, 0));
        advance(m, 12);
        CHECK(m->prizes[0].state == PRIZE_DELIVERED && m->delivered == 1);
    }
    machine_destroy(m);
}
static void test_modes_and_clock(void) {
    PrizeMachine *m = machine_create();
    CHECK(m && m->mode == MODE_SANDBOX && m->round == ROUND_PLAYING);
    advance(m, 5);
    CHECK(m->time_remaining == 0 && m->round == ROUND_PLAYING);
    for (int mode = MODE_EASY; mode < MODE_COUNT; ++mode) {
        CHECK(machine_start(m, (GameMode)mode));
        CHECK(near(m->time_remaining, GAME_MODES[mode].time_limit));
        machine_update(m, 5); /* Real clock time is not capped with physics. */
        CHECK(near(m->time_remaining, GAME_MODES[mode].time_limit - 5));
        float time = m->time_remaining;
        machine_update(m, NAN);
        machine_update(m, -1);
        CHECK(m->time_remaining == time);
        SceneNode *root = m->root;
        CHECK(!machine_start(m, MODE_COUNT));
        CHECK(m->root == root && m->time_remaining == time);
        m->time_remaining = .02f;
        machine_target_joint(m, 0, 20);
        machine_update(m, .1f);
        CHECK(m->round == ROUND_LOST && m->time_remaining == 0);
        float angle = m->arm.angle[0], target_before = m->arm.target[0];
        machine_target_joint(m, 0, -50);
        machine_close(m);
        machine_open(m);
        machine_update(m, 1);
        CHECK(m->arm.angle[0] == angle && m->arm.target[0] == target_before);
        CHECK(m->arm.opening == MACHINE_CONFIG.finger_max);
        CHECK(machine_capture_candidate(m, NULL) == -1);
        CHECK(machine_reset(m));
        CHECK(m->mode == (GameMode)mode && m->round == ROUND_PLAYING);
        CHECK(m->time_remaining == GAME_MODES[mode].time_limit && m->bounces == 0);
    }
    machine_destroy(m);
}
static void test_bouncing_prizes(void) {
    PrizeMachine *m = machine_create();
    CHECK(m);
    float previous_rebound = 0;
    for (int mode = MODE_SANDBOX; mode < MODE_COUNT; ++mode) {
        CHECK(machine_start(m, (GameMode)mode));
        place_prize(m, 0, vec3(-1, 2, -2), vec3(0, 0, 0));
        float rebound = 0;
        for (int i = 0; i < 300; ++i) {
            machine_update(m, 1.0f / 240);
            if (m->prizes[0].velocity.y > 0) {
                rebound = m->prizes[0].velocity.y;
                break;
            }
        }
        CHECK(rebound > previous_rebound);
        previous_rebound = rebound;
        CHECK(m->bounces > 0 && m->delivered == 0);
        advance(m, 12);
        CHECK(m->prizes[0].state == PRIZE_AVAILABLE);
        CHECK(near(scene_bounds(m->prizes[0].node).min.y, 0));
    }
    CHECK(machine_start(m, MODE_HARD));
    place_prize(m, 0, vec3(-1, 1, -2), vec3(2, 0, 0));
    place_prize(m, 1, vec3(-.25f, 1, -2), vec3(-1, 0, 0));
    advance(m, .15f);
    CHECK(m->prizes[0].velocity.x < 0 && m->prizes[1].velocity.x > 0);
    CHECK(near(m->prizes[0].velocity.x + m->prizes[1].velocity.x, 1));
    CHECK(!aabb_overlap(scene_bounds(m->prizes[0].node), scene_bounds(m->prizes[1].node), .001f));
    CHECK(m->bounces > 0);
    CHECK(machine_reset(m));
    place_prize(m, 0, vec3(3, 2.8f, 1.5f), vec3(100, 0, 0));
    machine_update(m, 1.0f / 120);
    CHECK(scene_bounds(m->prizes[0].node).max.x <= MACHINE_CONFIG.workspace.max.x + .001f);
    CHECK(m->prizes[0].velocity.x < 0);
    CHECK(machine_reset(m));
    SceneNode *barrier = m->solids[5].node;
    barrier->local =
        mat4_mul(mat4_translate(vec3(1.5f, 2.5f, -1.5f)), mat4_scale(vec3(.02f, 1, 1)));
    scene_update(barrier, mat4_identity());
    place_prize(m, 0, vec3(0, 2.5f, -1.5f), vec3(1000, 0, 0));
    machine_update(m, 1.0f / 240);
    CHECK(scene_bounds(m->prizes[0].node).max.x <= scene_bounds(barrier).min.x + .001f);
    CHECK(m->prizes[0].velocity.x < 0);
    CHECK(machine_reset(m));
    place_prize(m, 0, vec3(0, 2, 2.5f), vec3(0, 0, 0));
    for (int i = 0; i < 600 && m->bounces == 0; ++i)
        machine_update(m, 1.0f / 120);
    CHECK(m->bounces > 0);
    CHECK(m->delivered == 0 && m->prizes[0].state == PRIZE_FALLING);
    advance(m, 12);
    CHECK(m->delivered == 1 && m->prizes[0].state == PRIZE_DELIVERED);
    place_prize(m, 1, vec3(0, 2, 2.5f), vec3(0, 0, 0));
    advance(m, 12);
    CHECK(m->delivered == 1 && m->prizes[1].state == PRIZE_AVAILABLE);
    CHECK(machine_reset(m));
    place_prize(m, 0, vec3(1.4f, 2, 2.5f), vec3(0, 0, 0));
    advance(m, 12);
    CHECK(m->delivered == 0 && m->prizes[0].state == PRIZE_AVAILABLE);
    machine_destroy(m);
}
static void test_release_velocity(void) {
    PrizeMachine *m = machine_create();
    CHECK(m);
    pickup(m, 1);
    machine_target_joint(m, 0, -25);
    for (int i = 0; i < 600 && !machine_at_target(m); ++i)
        machine_update(m, 1.0f / 60);
    CHECK(machine_at_target(m));
    machine_open(m); /* Release on the exact frame the target is reached. */
    check_vec(m->prizes[1].velocity, vec3(0, 0, 0));
    CHECK(machine_reset(m));
    pickup(m, 1);
    machine_target_joint(m, 0, -25);
    machine_update(m, 1.0f / 60);
    CHECK(!machine_at_target(m));
    Vec3 moving = m->held_velocity;
    CHECK(v3_length(moving) > .1f);
    machine_open(m);
    check_vec(m->prizes[1].velocity, moving);
    machine_destroy(m);
}
static void wait_for_arm(PrizeMachine *m) {
    for (int i = 0; i < 1200 && !machine_at_target(m) && m->round == ROUND_PLAYING; ++i)
        machine_update(m, 1.0f / 120);
    CHECK(machine_at_target(m));
    CHECK(m->round == ROUND_PLAYING);
}
static void timed_target(PrizeMachine *m, int joint, float angle) {
    machine_target_joint(m, joint, angle);
    wait_for_arm(m);
    CHECK(near(m->arm.angle[joint], angle));
}
static void test_random_spawns(void) {
    const int orders[][PRIZE_COUNT] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2},
                                       {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
    float previous_radius_range = -1, previous_yaw_range = -1;
    CHECK(!machine_create_seeded((GameMode)-1, 0));
    CHECK(!machine_create_seeded(MODE_COUNT, 0));
    for (int mode = MODE_SANDBOX; mode < MODE_COUNT; ++mode) {
        float radius_min = 10, radius_max = 0, yaw_min = 180, yaw_max = -180;
        bool color_slots[PRIZE_COUNT][PRIZE_COUNT] = {{false}};
        Mat4 previous[PRIZE_COUNT];
        for (uint32_t seed = 0; seed < 24; ++seed) {
            PrizeMachine *m = machine_create_seeded((GameMode)mode, seed);
            CHECK(m);
            CHECK(m->spawn_seed == seed && m->mode == (GameMode)mode);
            CHECK(m->time_remaining == GAME_MODES[mode].time_limit);
            CHECK(m->blocked_moves == 0 && m->bounces == 0 && m->held == -1);
            CHECK(m->round == ROUND_PLAYING && machine_pose_valid(m));
            bool changed = false;
            for (int p = 0; p < PRIZE_COUNT; ++p) {
                const Prize *prize = &m->prizes[p];
                CHECK(prize->state == PRIZE_AVAILABLE);
                Vec3 center = mat4_point(prize->node->world, vec3(0, 0, 0));
                float radius = hypotf(center.x, center.z);
                radius_min = fminf(radius_min, radius);
                radius_max = fmaxf(radius_max, radius);
                float yaw = prize->pickup_pose[0];
                yaw_min = fminf(yaw_min, yaw);
                yaw_max = fmaxf(yaw_max, yaw);
                color_slots[p][yaw < -20 ? 0 : yaw > 20 ? 2 : 1] = true;
                AABB box = scene_bounds(prize->node), space = MACHINE_CONFIG.workspace;
                CHECK(near(box.min.y, 0));
                CHECK(box.min.x >= space.min.x && box.max.x <= space.max.x);
                CHECK(box.min.z >= space.min.z && box.max.z <= space.max.z);
                for (int corner = 0; corner < 4; ++corner) {
                    float x = ((corner & 1 ? box.max.x : box.min.x) - 1) / 3;
                    float z = (corner & 2 ? box.max.z : box.min.z) / 3.6f;
                    CHECK(x * x + z * z < 1);
                }
                for (int s = 0; s < m->solid_count; ++s)
                    CHECK(!aabb_overlap(box, scene_bounds(m->solids[s].node), 0));
                for (int other = 0; other < p; ++other)
                    CHECK(!aabb_overlap(box, scene_bounds(m->prizes[other].node), 0));
                if (seed && memcmp(&previous[p], &prize->node->local, sizeof(Mat4)))
                    changed = true;
                previous[p] = prize->node->local;
            }
            if (seed)
                CHECK(changed);
            if (seed == 0 || seed == 23) {
                PrizeMachine *copy = machine_create_seeded((GameMode)mode, seed);
                CHECK(copy);
                for (int p = 0; p < PRIZE_COUNT; ++p)
                    CHECK(!memcmp(&m->prizes[p].node->local, &copy->prizes[p].node->local,
                                  sizeof(Mat4)));
                machine_destroy(copy);
            }
            /* Simulate actual controls, collisions, capture, falling and delivery.
             * Each mode exercises every collection order with several layouts. */
            for (int collected = 0; collected < PRIZE_COUNT; ++collected) {
                int p = orders[seed % 6][collected];
                timed_target(m, 0, m->prizes[p].pickup_pose[0]);
                timed_target(m, 2, m->prizes[p].pickup_pose[2]);
                timed_target(m, 3, m->prizes[p].pickup_pose[3]);
                timed_target(m, 1, m->prizes[p].pickup_pose[1]);
                machine_close(m);
                wait_for_arm(m);
                CHECK(m->held == p);
                timed_target(m, 1, -25);
                timed_target(m, 3, -70);
                timed_target(m, 2, -85);
                timed_target(m, 0, MACHINE_CONFIG.drop_yaw[p]);
                machine_open(m);
                for (int i = 0;
                     i < 2400 && m->prizes[p].state != PRIZE_DELIVERED && m->round == ROUND_PLAYING;
                     ++i)
                    machine_update(m, 1.0f / 120);
                CHECK(m->prizes[p].state == PRIZE_DELIVERED);
                CHECK(m->delivered == collected + 1);
                AABB landed = scene_bounds(m->prizes[p].node), tray = machine_delivery_region(m);
                CHECK(landed.min.x >= tray.min.x + .049f && landed.max.x <= tray.max.x - .049f);
                CHECK(landed.min.z >= tray.min.z + .049f && landed.max.z <= tray.max.z - .049f);
                CHECK(near(landed.min.y, tray.min.y));
                for (int other = 0; other < PRIZE_COUNT; ++other)
                    if (other != p && m->prizes[other].state == PRIZE_DELIVERED)
                        CHECK(!aabb_overlap(landed, scene_bounds(m->prizes[other].node), 0));
                if (mode != MODE_SANDBOX)
                    CHECK(m->time_remaining > 0);
            }
            CHECK(m->round == ROUND_WON && m->blocked_moves == 0 && m->bounces > 0);
            float time = m->time_remaining;
            advance(m, 2);
            CHECK(m->time_remaining == time && m->delivered == PRIZE_COUNT);
            machine_destroy(m);
        }
        for (int p = 0; p < PRIZE_COUNT; ++p)
            for (int slot = 0; slot < PRIZE_COUNT; ++slot)
                CHECK(color_slots[p][slot]);
        float radius_range = radius_max - radius_min, yaw_range = yaw_max - yaw_min;
        if (mode >= MODE_MEDIUM) {
            CHECK(radius_range > previous_radius_range + .15f);
            CHECK(yaw_range > previous_yaw_range + 5);
        }
        previous_radius_range = radius_range;
        previous_yaw_range = yaw_range;
    }
    PrizeMachine *m = machine_create();
    CHECK(m);
    for (int mode = MODE_SANDBOX; mode < MODE_COUNT; ++mode) {
        CHECK(machine_start(m, (GameMode)mode));
        uint32_t seed = m->spawn_seed;
        CHECK(machine_reset(m));
        CHECK(m->spawn_seed != seed && m->mode == (GameMode)mode);
        PrizeMachine *replay = machine_create_seeded((GameMode)mode, m->spawn_seed);
        CHECK(replay);
        for (int p = 0; p < PRIZE_COUNT; ++p)
            CHECK(!memcmp(&replay->prizes[p].node->local, &m->prizes[p].node->local, sizeof(Mat4)));
        machine_destroy(replay);
    }
    machine_destroy(m);
}
int main(void) {
    test_math();
    test_free_camera();
    test_scene();
    test_mesh_contract();
    test_full_game();
    test_limits_collision_time();
    test_occlusion_and_cargo();
    test_difficulty_trays();
    test_modes_and_clock();
    test_bouncing_prizes();
    test_release_velocity();
    test_random_spawns();
    printf("PASS: %d checks (math, camera, ownership, FK, picking, collisions, difficulty trays, "
           "randomized spawns, 96 seeded rounds, all collection orders, reset)\n",
           checks);
    return EXIT_SUCCESS;
}
