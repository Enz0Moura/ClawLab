#include "app/physics.h"

#include <math.h>

static float component(Vec3 v, int axis) {
    return axis == 0 ? v.x : axis == 1 ? v.y : v.z;
}
static void set_component(Vec3 *v, int axis, float value) {
    if (axis == 0)
        v->x = value;
    else if (axis == 1)
        v->y = value;
    else
        v->z = value;
}
static bool movable(const Prize *p) {
    return p->state == PRIZE_AVAILABLE || p->state == PRIZE_FALLING;
}
static bool overlap_other_axes(AABB a, AABB b, int axis) {
    for (int i = 0; i < 3; ++i)
        if (i != axis && (component(a.max, i) <= component(b.min, i) + .00001f ||
                          component(a.min, i) >= component(b.max, i) - .00001f))
            return false;
    return true;
}
static bool sweep(AABB a, AABB b, int axis, float delta, float *allowed) {
    if (!overlap_other_axes(a, b, axis))
        return false;
    if (delta > 0 && component(a.max, axis) <= component(b.min, axis) + .0001f) {
        float gap = fmaxf(0, component(b.min, axis) - component(a.max, axis));
        if (gap <= *allowed) {
            *allowed = gap;
            return true;
        }
    } else if (delta < 0 && component(a.min, axis) >= component(b.max, axis) - .0001f) {
        float gap = fminf(0, component(b.max, axis) - component(a.min, axis));
        if (gap >= *allowed) {
            *allowed = gap;
            return true;
        }
    }
    return false;
}
static void move_axis(PrizeMachine *m, int p, int axis, float dt) {
    Prize *prize = &m->prizes[p];
    float speed = component(prize->velocity, axis), delta = speed * dt;
    if (delta == 0)
        return;
    AABB bounds = scene_bounds(prize->node), space = MACHINE_CONFIG.workspace;
    float allowed = delta;
    int hit = -1; /* -1: no hit; -2: static surface; >= 0: another prize. */
    float edge = delta > 0 ? component(space.max, axis) - component(bounds.max, axis)
                           : component(space.min, axis) - component(bounds.min, axis);
    if ((delta > 0 && delta >= edge) || (delta < 0 && delta <= edge)) {
        allowed = delta > 0 ? fmaxf(0, edge) : fminf(0, edge);
        hit = -2;
    }
    for (int s = 0; s < m->solid_count; ++s)
        if (sweep(bounds, scene_bounds(m->solids[s].node), axis, delta, &allowed))
            hit = -2;
    int candidate = machine_capture_candidate(m, NULL);
    for (int part = 0; part < ARM_PART_COUNT; ++part) {
        if ((part == 3 || part == 4) && (p == m->release_contact || p == candidate))
            continue;
        if (sweep(bounds, scene_bounds(m->arm.parts[part]), axis, delta, &allowed))
            hit = -2;
    }
    for (int other = 0; other < PRIZE_COUNT; ++other)
        if (other != p && sweep(bounds, scene_bounds(m->prizes[other].node), axis, delta, &allowed))
            hit = other;
    prize->node->local.m[12 + axis] += allowed;
    scene_update(prize->node, mat4_identity());
    if (hit == -1)
        return;
    float restitution = GAME_MODES[m->mode].restitution;
    if (hit >= 0 && movable(&m->prizes[hit])) {
        Prize *other = &m->prizes[hit];
        float other_speed = component(other->velocity, axis);
        float relative = speed - other_speed;
        if (relative * delta > 0) {
            if (fabsf(relative) > .35f) {
                float impulse = (1 + restitution) * relative * .5f;
                set_component(&prize->velocity, axis, speed - impulse);
                set_component(&other->velocity, axis, other_speed + impulse);
                other->state = PRIZE_FALLING;
                ++m->bounces;
            } else
                set_component(&prize->velocity, axis, other_speed);
        }
    } else {
        set_component(&prize->velocity, axis, fabsf(speed) > .35f ? -speed * restitution : 0);
        if (fabsf(speed) > .35f)
            ++m->bounces;
    }
    if (axis == 1 && delta < 0) {
        float friction = expf(-GAME_MODES[m->mode].friction * dt);
        prize->velocity.x *= friction;
        prize->velocity.z *= friction;
        if (fabsf(prize->velocity.x) < .03f)
            prize->velocity.x = 0;
        if (fabsf(prize->velocity.z) < .03f)
            prize->velocity.z = 0;
    }
}
static int support(const PrizeMachine *m, int p) {
    AABB box = scene_bounds(m->prizes[p].node);
    for (int i = 0; i < m->solid_count + PRIZE_COUNT + ARM_PART_COUNT; ++i) {
        if (i == m->solid_count + p)
            continue;
        AABB other;
        if (i < m->solid_count)
            other = scene_bounds(m->solids[i].node);
        else if (i < m->solid_count + PRIZE_COUNT)
            other = scene_bounds(m->prizes[i - m->solid_count].node);
        else
            other = scene_bounds(m->arm.parts[i - m->solid_count - PRIZE_COUNT]);
        if (fabsf(box.min.y - other.max.y) < .0002f && overlap_other_axes(box, other, 1))
            return i;
    }
    return -1;
}
void physics_step(PrizeMachine *m, float dt) {
    for (int p = 0; p < PRIZE_COUNT; ++p) {
        Prize *prize = &m->prizes[p];
        if (!movable(prize))
            continue;
        prize->velocity.y -= MACHINE_CONFIG.gravity * dt;
        move_axis(m, p, 0, dt);
        move_axis(m, p, 2, dt);
        move_axis(m, p, 1, dt);
    }
    for (int p = 0; p < PRIZE_COUNT; ++p) {
        Prize *prize = &m->prizes[p];
        if (!movable(prize))
            continue;
        int resting_on = support(m, p);
        bool settled = resting_on >= 0 && v3_length(prize->velocity) < .03f;
        prize->state = settled ? PRIZE_AVAILABLE : PRIZE_FALLING;
        if (!settled)
            continue;
        prize->velocity = vec3(0, 0, 0);
        AABB bounds = scene_bounds(prize->node), area = machine_delivery_region(m);
        if (resting_on == m->tray_solid && bounds.min.x >= area.min.x &&
            bounds.max.x <= area.max.x && bounds.min.z >= area.min.z &&
            bounds.max.z <= area.max.z) {
            prize->state = PRIZE_DELIVERED;
            ++m->delivered;
            if (m->selected == p)
                m->selected = -1;
        }
    }
}
