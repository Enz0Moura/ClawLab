#include "app/theme.h"

#include <math.h>
#include <stdlib.h>

static const Color RED = {.87f, .055f, .085f}, RED_LIGHT = {1, .11f, .13f};
static const Color YELLOW = {1, .82f, .07f}, BLUE = {.075f, .27f, .82f};
static const Color GREEN = {.61f, .89f, .10f}, WHITE = {1, .99f, .88f}, INK = {.025f, .035f, .09f};
enum { VERTEX_CAPACITY = 40000 };
static const ThemeProfilePoint ROOF_DOME_PROFILE[] = {
    {4.48f, 1}, {4.72f, 1}, {5.12f, .96f}, {5.55f, .84f}, {5.98f, .67f}, {6.25f, .46f}};
static const ThemeProfilePoint ROOF_CAP_PROFILE[] = {
    {6.25f, .46f}, {6.40f, .31f}, {6.48f, 0}};
static const ThemeProfilePoint ROOF_TRIM_PROFILE[] = {{4.45f, 1.005f}, {4.54f, 1.005f}};
static const ThemeProfilePoint BASE_SHELL_PROFILE[] = {
    {-1.55f, .60f}, {-1.35f, .74f}, {-.72f, .94f}, {-.34f, 1}};
static const ThemeProfilePoint BASE_BELT_PROFILE[] = {{-.37f, 1.01f}, {-.02f, 1.01f}};
static const ThemePoint2 FIN_OUTLINE[] = {
    {0, 1.4f}, {.42f, 1.05f}, {1.12f, -.40f}, {1.12f, -1.55f}, {.12f, -.78f}};
typedef struct {
    MeshVertex *v;
    size_t count;
    bool valid;
} Builder;

static Builder begin_mesh(void) {
    MeshVertex *vertices = malloc(VERTEX_CAPACITY * sizeof(*vertices));
    return (Builder){vertices, 0, vertices != NULL};
}
static void triangle(Builder *b, MeshVertex a, MeshVertex c, MeshVertex d) {
    if (!b->valid)
        return;
    if (b->count + 3 > VERTEX_CAPACITY) {
        b->valid = false;
        return;
    }
    b->v[b->count++] = a;
    b->v[b->count++] = c;
    b->v[b->count++] = d;
}
static void face(Builder *b, Vec3 a, Vec3 c, Vec3 d, Color color) {
    Vec3 n = v3_normalize(v3_cross(v3_sub(c, a), v3_sub(d, a)));
    triangle(b, (MeshVertex){a, n, color}, (MeshVertex){c, n, color}, (MeshVertex){d, n, color});
}
static void quad(Builder *b, Vec3 a, Vec3 c, Vec3 d, Vec3 e, Color color) {
    face(b, a, c, d, color);
    face(b, a, d, e, color);
}
static Geometry *finish_mesh(Builder *b) {
    Mesh *mesh = b->valid ? MeshFabric.create(b->v, b->count) : NULL;
    free(b->v);
    return mesh ? &mesh->base : NULL;
}
static MeshVertex sphere_vertex(Vec3 center, Vec3 size, float longitude, float latitude,
                                Color color) {
    Vec3 d =
        vec3(cosf(latitude) * cosf(longitude), sinf(latitude), cosf(latitude) * sinf(longitude));
    Vec3 p = v3_add(center, vec3(.5f * size.x * d.x, .5f * size.y * d.y, .5f * size.z * d.z));
    /* Nonuniformly scaled sphere: bake the inverse-transpose into its normal. */
    Vec3 n = v3_normalize(vec3(d.x / size.x, d.y / size.y, d.z / size.z));
    return (MeshVertex){p, n, color};
}
static void ellipsoid(Builder *b, const ThemeEllipsoid *shape) {
    if (!shape || shape->size.x <= 0 || shape->size.y <= 0 || shape->size.z <= 0)
        return;
    const int slices = 16, rings = 10;
    for (int r = 0; r < rings; ++r)
        for (int s = 0; s < slices; ++s) {
            float lo = cg_radians(-90 + 180.0f * (float)r / rings),
                  hi = cg_radians(-90 + 180.0f * (float)(r + 1) / rings);
            float a = cg_radians(360.0f * (float)s / slices),
                  next = cg_radians(360.0f * (float)(s + 1) / slices);
            MeshVertex v0 = sphere_vertex(shape->center, shape->size, a, lo, shape->color),
                       v1 = sphere_vertex(shape->center, shape->size, a, hi, shape->color);
            MeshVertex v2 = sphere_vertex(shape->center, shape->size, next, hi, shape->color),
                       v3 = sphere_vertex(shape->center, shape->size, next, lo, shape->color);
            triangle(b, v0, v1, v2);
            triangle(b, v0, v2, v3);
        }
}
/* Elliptical surface of revolution: profile stores (height, radius fraction).
 * Profile tangents give analytic normals; no GLUT solid primitives are used. */
static void lathe(Builder *b, const ThemeLatheDesign *shape) {
    if (!shape || !shape->profile || shape->profile_count < 2 || shape->radius_x <= 0 ||
        shape->radius_z <= 0)
        return;
    const int slices = 64;
    for (size_t p = 0; p + 1 < shape->profile_count; ++p)
        for (int s = 0; s < slices; ++s) {
            MeshVertex v[4];
            for (int k = 0; k < 4; ++k) {
                size_t row = p + (k == 1 || k == 2);
                float angle = cg_radians(360.0f * (float)(s + (k >= 2)) / slices), c = cosf(angle),
                      z = sinf(angle);
                Vec3 point = v3_add(shape->center,
                                    vec3(shape->radius_x * shape->profile[row].radius_fraction * c,
                                         shape->profile[row].height,
                                         shape->radius_z * shape->profile[row].radius_fraction * z));
                size_t previous = row ? row - 1 : row;
                size_t following = row + 1 < shape->profile_count ? row + 1 : row;
                float dy = shape->profile[following].height - shape->profile[previous].height;
                float dr = shape->profile[following].radius_fraction -
                           shape->profile[previous].radius_fraction;
                Vec3 normal = v3_normalize(vec3(dy * c / shape->radius_x, -dr,
                                                dy * z / shape->radius_z));
                v[k] = (MeshVertex){point, normal, shape->color};
            }
            triangle(b, v[0], v[1], v[2]);
            triangle(b, v[0], v[2], v[3]);
        }
}
static void ring(Builder *b, const ThemeRing *shape) {
    /* Ring in XY, useful for a porthole or planet emblem on a front surface. */
    for (int s = 0; s < 48; ++s) {
        float a = cg_radians(360.0f * s / 48), next = cg_radians(360.0f * (s + 1) / 48);
        Vec3 p[4] = {
            {shape->center.x + shape->radius_x * cosf(a),
             shape->center.y + shape->radius_y * sinf(a), shape->center.z},
            {shape->center.x + (shape->radius_x - shape->thickness) * cosf(a),
             shape->center.y + (shape->radius_y - shape->thickness) * sinf(a), shape->center.z},
            {shape->center.x + (shape->radius_x - shape->thickness) * cosf(next),
             shape->center.y + (shape->radius_y - shape->thickness) * sinf(next), shape->center.z},
            {shape->center.x + shape->radius_x * cosf(next),
             shape->center.y + shape->radius_y * sinf(next), shape->center.z}};
        quad(b, p[0], p[3], p[2], p[1], shape->color);
    }
}
static Vec3 back_wall_inward(float angle, const ThemeBackWallDesign *design) {
    Vec3 tangent = vec3(-design->radius_x * sinf(angle), 0,
                        design->radius_z * cosf(angle));
    Vec3 normal = v3_normalize(v3_cross(tangent, vec3(0, 1, 0)));
    return normal;
}
static Vec3 back_wall_point(float angle, float y, float inset,
                            const ThemeBackWallDesign *design) {
    Vec3 point = vec3(design->center_x + design->radius_x * cosf(angle), y,
                      design->radius_z * sinf(angle));
    return v3_add(point, v3_scale(back_wall_inward(angle, design), inset));
}
static void fin(Builder *b, float side, const ThemeFinDesign *design) {
    if (!design || !design->outline || design->outline_count < 3 ||
        design->outline_count > THEME_MAX_FIN_POINTS)
        return;
    Vec3 p[2][THEME_MAX_FIN_POINTS];
    for (int z = 0; z < 2; ++z)
        for (size_t i = 0; i < design->outline_count; ++i)
            p[z][i] = vec3(design->center_x + side * (design->side_offset + design->outline[i].x),
                           design->outline[i].y,
                           design->center_z + (z ? design->half_depth : -design->half_depth));
    for (size_t i = 1; i + 1 < design->outline_count; ++i) {
        size_t a = side > 0 ? i + 1 : i, c = side > 0 ? i : i + 1;
        face(b, p[1][0], p[1][a], p[1][c], design->front_color);
        face(b, p[0][0], p[0][c], p[0][a], design->back_color);
    }
    for (size_t i = 0; i < design->outline_count; ++i) {
        size_t next = (i + 1) % design->outline_count;
        size_t a = side > 0 ? i : next, c = side > 0 ? next : i;
        quad(b, p[0][a], p[0][c], p[1][c], p[1][a], design->side_color);
    }
}
ThemeAlienDesign theme_alien_default_design(Color lower_body_color) {
    ThemeAlienDesign design = {0};
    design.lower_body = (ThemeEllipsoid){vec3(0, -.22f, 0), vec3(.48f, .38f, .37f),
                                         lower_body_color};
    design.head = (ThemeEllipsoid){vec3(0, .13f, 0), vec3(.72f, .46f, .48f), GREEN};
    design.ears[0] = (ThemeEllipsoid){vec3(-.40f, .16f, 0), vec3(.20f, .14f, .12f), GREEN};
    design.ears[1] = (ThemeEllipsoid){vec3(.40f, .16f, 0), vec3(.20f, .14f, .12f), GREEN};
    design.arms[0] = (ThemeEllipsoid){vec3(-.30f, -.20f, .02f), vec3(.16f, .12f, .17f), GREEN};
    design.arms[1] = (ThemeEllipsoid){vec3(.30f, -.20f, .02f), vec3(.16f, .12f, .17f), GREEN};
    design.legs[0] = (ThemeEllipsoid){vec3(-.14f, -.44f, .045f), vec3(.21f, .12f, .40f),
                                      (Color){.16f, .10f, .47f}};
    design.legs[1] = (ThemeEllipsoid){vec3(.14f, -.44f, .045f), vec3(.21f, .12f, .40f),
                                      (Color){.16f, .10f, .47f}};
    design.antenna_stem =
        (ThemeEllipsoid){vec3(0, .395f, 0), vec3(.025f, .13f, .025f), GREEN};
    design.antenna_tip = (ThemeEllipsoid){vec3(0, .46f, 0), vec3(.08f, .08f, .08f), GREEN};
    for (int eye = 0; eye < 3; ++eye) {
        float x = (eye - 1) * .16f;
        design.eyes[eye] = (ThemeEllipsoid){vec3(x, .18f, .225f), vec3(.145f, .14f, .10f), WHITE};
        design.pupils[eye] = (ThemeEllipsoid){vec3(x, .18f, .276f), vec3(.045f, .06f, .024f), INK};
    }
    design.mouth = (ThemeEllipsoid){vec3(0, .015f, .224f), vec3(.13f, .045f, .026f), INK};
    design.badge = (ThemeEllipsoid){vec3(.04f, -.17f, .185f), vec3(.10f, .10f, .035f), BLUE};
    design.badge_ring = (ThemeRing){vec3(.04f, -.17f, .207f), .076f, .024f, .008f, YELLOW};
    design.facing_degrees = 35;
    design.bounds = aabb_make(vec3(-.5f, -.5f, -.5f), vec3(.5f, .5f, .5f));
    return design;
}
Geometry *theme_alien_create_design(const ThemeAlienDesign *requested_design) {
    ThemeAlienDesign fallback;
    if (!requested_design) {
        fallback = theme_alien_default_design(GREEN);
        requested_design = &fallback;
    }
    const ThemeAlienDesign *design = requested_design;
    Builder b = begin_mesh();
    ellipsoid(&b, &design->lower_body);
    ellipsoid(&b, &design->head);
    for (int i = 0; i < 2; ++i) {
        ellipsoid(&b, &design->ears[i]);
        ellipsoid(&b, &design->arms[i]);
        ellipsoid(&b, &design->legs[i]);
    }
    ellipsoid(&b, &design->antenna_stem);
    ellipsoid(&b, &design->antenna_tip);
    for (int i = 0; i < 3; ++i) {
        ellipsoid(&b, &design->eyes[i]);
        ellipsoid(&b, &design->pupils[i]);
    }
    ellipsoid(&b, &design->mouth);
    ellipsoid(&b, &design->badge);
    ring(&b, &design->badge_ring);
    Mat4 facing = mat4_rotate_y(design->facing_degrees);
    for (size_t i = 0; i < b.count; ++i) {
        b.v[i].position = mat4_point(facing, b.v[i].position);
        b.v[i].normal = mat4_vector(facing, b.v[i].normal);
    }
    Geometry *geometry = finish_mesh(&b);
    if (geometry) {
        geometry->bounds = design->bounds.transform ? design->bounds
                                                    : aabb_make(vec3(-.5f, -.5f, -.5f),
                                                                vec3(.5f, .5f, .5f));
    }
    return geometry;
}
Geometry *theme_alien_create(Color lower_body_color) {
    ThemeAlienDesign design = theme_alien_default_design(lower_body_color);
    return theme_alien_create_design(&design);
}

ThemeCabinetDesign theme_cabinet_default_design(void) {
    ThemeCabinetDesign design = {0};
    const Vec3 cabinet_center = vec3(1, 0, 0);
    design.floor = (ThemeFloorDesign){1, THEME_MAP_RADIUS_X, THEME_MAP_RADIUS_Z, -.32f, 64,
                                      BLUE, {.07f, .14f, .34f}};
    design.back_wall = (ThemeBackWallDesign){1, THEME_MAP_RADIUS_X, THEME_MAP_RADIUS_Z,
                                             THEME_BACK_WALL_HEIGHT,
                                             THEME_BACK_WALL_THICKNESS, 96,
                                             THEME_BACK_WALL_SEGMENTS, {.045f, .09f, .24f}};
    design.roof.dome = (ThemeLatheDesign){cabinet_center, 3.18f, 3.76f, ROOF_DOME_PROFILE,
                                          sizeof(ROOF_DOME_PROFILE) / sizeof(ROOF_DOME_PROFILE[0]),
                                          RED};
    design.roof.cap = (ThemeLatheDesign){cabinet_center, 3.18f, 3.76f, ROOF_CAP_PROFILE,
                                         sizeof(ROOF_CAP_PROFILE) / sizeof(ROOF_CAP_PROFILE[0]),
                                         YELLOW};
    design.roof.trim = (ThemeLatheDesign){cabinet_center, 3.18f, 3.76f, ROOF_TRIM_PROFILE,
                                          sizeof(ROOF_TRIM_PROFILE) / sizeof(ROOF_TRIM_PROFILE[0]),
                                          YELLOW};
    design.base.shell = (ThemeLatheDesign){cabinet_center, 3.18f, 3.76f, BASE_SHELL_PROFILE,
                                           sizeof(BASE_SHELL_PROFILE) /
                                               sizeof(BASE_SHELL_PROFILE[0]),
                                           RED};
    design.base.belt = (ThemeLatheDesign){cabinet_center, 3.18f, 3.76f, BASE_BELT_PROFILE,
                                          sizeof(BASE_BELT_PROFILE) / sizeof(BASE_BELT_PROFILE[0]),
                                          BLUE};
    design.base.porthole = (ThemeEllipsoid){vec3(1, -.84f, 3.37f), vec3(1.40f, .78f, .055f), INK};
    design.base.porthole_ring =
        (ThemeRing){vec3(1, -.84f, 3.405f), .76f, .44f, .07f, RED_LIGHT};
    design.base.indicator_count = THEME_MAX_BASE_INDICATORS;
    for (size_t i = 0; i < design.base.indicator_count; ++i) {
        float angle = cg_radians(26 + 21.0f * (float)i);
        Vec3 center = vec3(1 + 3.23f * cosf(angle), -.18f, 3.82f * sinf(angle));
        Color color = i % 2 ? WHITE : YELLOW;
        design.base.indicators[i] =
            (ThemeEllipsoid){center, vec3(.13f, .13f, .13f), color};
    }
    design.fin = (ThemeFinDesign){FIN_OUTLINE,
                                  sizeof(FIN_OUTLINE) / sizeof(FIN_OUTLINE[0]),
                                  1, 3.08f, .7f, .38f, RED_LIGHT, RED, RED};
    return design;
}

Geometry *theme_cabinet_create_design(ThemePart part, const ThemeCabinetDesign *requested_design) {
    ThemeCabinetDesign fallback;
    if (!requested_design) {
        fallback = theme_cabinet_default_design();
        requested_design = &fallback;
    }
    const ThemeCabinetDesign *design = requested_design;
    Builder b = begin_mesh();
    if (part == THEME_FLOOR) {
        ThemeProfilePoint skirt_profile[] = {{design->floor.skirt_height, 1}, {0, 1}};
        ThemeLatheDesign skirt = {vec3(design->floor.center_x, 0, 0),
                                  design->floor.radius_x,
                                  design->floor.radius_z,
                                  skirt_profile,
                                  sizeof(skirt_profile) / sizeof(skirt_profile[0]),
                                  design->floor.skirt_color};
        lathe(&b, &skirt);
        unsigned segments = design->floor.segments;
        if (segments < 3)
            segments = 3;
        if (segments > 256)
            segments = 256;
        for (unsigned i = 0; i < segments; ++i) {
            float a = cg_radians(360.0f * (float)i / (float)segments);
            float next = cg_radians(360.0f * (float)(i + 1) / (float)segments);
            Vec3 center = vec3(design->floor.center_x, 0, 0);
            face(&b, center,
                 vec3(center.x + design->floor.radius_x * cosf(next), 0,
                      design->floor.radius_z * sinf(next)),
                 vec3(center.x + design->floor.radius_x * cosf(a), 0,
                      design->floor.radius_z * sinf(a)),
                 design->floor.top_color);
        }
    } else if (part == THEME_BACK_WALL) {
        const ThemeBackWallDesign *wall = &design->back_wall;
        unsigned segments = wall->mesh_segments;
        if (segments < 1)
            segments = 1;
        if (segments > 512)
            segments = 512;
        const float pi = 3.14159265358979323846f;
        for (unsigned i = 0; i < segments; ++i) {
            float a0 = pi + pi * (float)i / (float)segments;
            float a1 = pi + pi * (float)(i + 1) / (float)segments;
            Vec3 n0 = back_wall_inward(a0, wall), n1 = back_wall_inward(a1, wall);
            MeshVertex low0 = {back_wall_point(a0, 0, wall->thickness * .5f, wall), n0,
                               wall->color};
            MeshVertex low1 = {back_wall_point(a1, 0, wall->thickness * .5f, wall), n1,
                               wall->color};
            MeshVertex high1 = {back_wall_point(a1, wall->height, wall->thickness * .5f, wall), n1,
                                wall->color};
            MeshVertex high0 = {back_wall_point(a0, wall->height, wall->thickness * .5f, wall), n0,
                                wall->color};
            triangle(&b, low0, low1, high1);
            triangle(&b, low0, high1, high0);
        }
    } else if (part == THEME_ROOF) {
        lathe(&b, &design->roof.dome);
        lathe(&b, &design->roof.cap);
        lathe(&b, &design->roof.trim);
    } else if (part == THEME_BASE) {
        lathe(&b, &design->base.shell);
        lathe(&b, &design->base.belt);
        ellipsoid(&b, &design->base.porthole);
        ring(&b, &design->base.porthole_ring);
        size_t count = design->base.indicator_count;
        if (count > THEME_MAX_BASE_INDICATORS)
            count = THEME_MAX_BASE_INDICATORS;
        for (size_t i = 0; i < count; ++i)
            ellipsoid(&b, &design->base.indicators[i]);
    } else if (part == THEME_FIN_LEFT || part == THEME_FIN_RIGHT) {
        fin(&b, part == THEME_FIN_LEFT ? -1.0f : 1.0f, &design->fin);
    }
    return finish_mesh(&b);
}
Geometry *theme_cabinet_create(ThemePart part) {
    ThemeCabinetDesign design = theme_cabinet_default_design();
    return theme_cabinet_create_design(part, &design);
}
