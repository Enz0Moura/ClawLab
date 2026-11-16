#include "app/theme.h"

#include <math.h>
#include <stdlib.h>

static const Color RED = {.87f, .055f, .085f}, RED_LIGHT = {1, .11f, .13f};
static const Color YELLOW = {1, .82f, .07f}, BLUE = {.075f, .27f, .82f};
static const Color GREEN = {.61f, .89f, .10f}, WHITE = {1, .99f, .88f}, INK = {.025f, .035f, .09f};
enum { VERTEX_CAPACITY = 40000 };
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
static void ellipsoid(Builder *b, Vec3 center, Vec3 size, Color color) {
    const int slices = 16, rings = 10;
    for (int r = 0; r < rings; ++r)
        for (int s = 0; s < slices; ++s) {
            float lo = cg_radians(-90 + 180.0f * (float)r / rings),
                  hi = cg_radians(-90 + 180.0f * (float)(r + 1) / rings);
            float a = cg_radians(360.0f * (float)s / slices),
                  next = cg_radians(360.0f * (float)(s + 1) / slices);
            MeshVertex v0 = sphere_vertex(center, size, a, lo, color),
                       v1 = sphere_vertex(center, size, a, hi, color);
            MeshVertex v2 = sphere_vertex(center, size, next, hi, color),
                       v3 = sphere_vertex(center, size, next, lo, color);
            triangle(b, v0, v1, v2);
            triangle(b, v0, v2, v3);
        }
}
/* Elliptical surface of revolution: profile stores (height, radius fraction).
 * Profile tangents give analytic normals; no GLUT solid primitives are used. */
static void lathe(Builder *b, Vec3 center, float rx, float rz, const float profile[][2],
                  size_t count, Color color) {
    const int slices = 64;
    for (size_t p = 0; p + 1 < count; ++p)
        for (int s = 0; s < slices; ++s) {
            MeshVertex v[4];
            for (int k = 0; k < 4; ++k) {
                size_t row = p + (k == 1 || k == 2);
                float angle = cg_radians(360.0f * (float)(s + (k >= 2)) / slices), c = cosf(angle),
                      z = sinf(angle);
                Vec3 point = v3_add(center, vec3(rx * profile[row][1] * c, profile[row][0],
                                                 rz * profile[row][1] * z));
                size_t previous = row ? row - 1 : row;
                size_t following = row + 1 < count ? row + 1 : row;
                float dy = profile[following][0] - profile[previous][0];
                float dr = profile[following][1] - profile[previous][1];
                Vec3 normal = v3_normalize(vec3(dy * c / rx, -dr, dy * z / rz));
                v[k] = (MeshVertex){point, normal, color};
            }
            triangle(b, v[0], v[1], v[2]);
            triangle(b, v[0], v[2], v[3]);
        }
}
static void ring(Builder *b, Vec3 center, float rx, float ry, float thickness, Color color) {
    /* Ring in XY, useful for a porthole or planet emblem on a front surface. */
    for (int s = 0; s < 48; ++s) {
        float a = cg_radians(360.0f * s / 48), next = cg_radians(360.0f * (s + 1) / 48);
        Vec3 p[4] = {{center.x + rx * cosf(a), center.y + ry * sinf(a), center.z},
                     {center.x + (rx - thickness) * cosf(a), center.y + (ry - thickness) * sinf(a),
                      center.z},
                     {center.x + (rx - thickness) * cosf(next),
                      center.y + (ry - thickness) * sinf(next), center.z},
                     {center.x + rx * cosf(next), center.y + ry * sinf(next), center.z}};
        quad(b, p[0], p[3], p[2], p[1], color);
    }
}
static Vec3 back_wall_inward(float angle) {
    Vec3 tangent = vec3(-THEME_MAP_RADIUS_X * sinf(angle), 0,
                        THEME_MAP_RADIUS_Z * cosf(angle));
    Vec3 normal = v3_normalize(v3_cross(tangent, vec3(0, 1, 0)));
    return normal;
}
static Vec3 back_wall_point(float angle, float y, float inset) {
    Vec3 point = vec3(1 + THEME_MAP_RADIUS_X * cosf(angle), y,
                      THEME_MAP_RADIUS_Z * sinf(angle));
    return v3_add(point, v3_scale(back_wall_inward(angle), inset));
}
static void fin(Builder *b, float side) {
    /* Convex extruded silhouette, attached outside the reachable volume. */
    const float points[][2] = {
        {0, 1.4f}, {.42f, 1.05f}, {1.12f, -.40f}, {1.12f, -1.55f}, {.12f, -.78f}};
    Vec3 p[2][5];
    for (int z = 0; z < 2; ++z)
        for (int i = 0; i < 5; ++i)
            p[z][i] =
                vec3(1 + side * (3.08f + points[i][0]), points[i][1], .7f + (z ? .38f : -.38f));
    for (int i = 1; i < 4; ++i) {
        int a = side > 0 ? i + 1 : i, c = side > 0 ? i : i + 1;
        face(b, p[1][0], p[1][a], p[1][c], RED_LIGHT);
        face(b, p[0][0], p[0][c], p[0][a], RED);
    }
    for (int i = 0; i < 5; ++i) {
        int a = side > 0 ? i : (i + 1) % 5, c = side > 0 ? (i + 1) % 5 : i;
        quad(b, p[0][a], p[0][c], p[1][c], p[1][a], RED);
    }
}
Geometry *theme_alien_create(Color uniform) {
    Builder b = begin_mesh();
    ellipsoid(&b, vec3(0, -.22f, 0), vec3(.48f, .38f, .37f), uniform);
    ellipsoid(&b, vec3(0, .13f, 0), vec3(.72f, .46f, .48f), GREEN);
    for (int side = -1; side <= 1; side += 2) {
        ellipsoid(&b, vec3(side * .40f, .16f, 0), vec3(.20f, .14f, .12f), GREEN);
        ellipsoid(&b, vec3(side * .30f, -.20f, .02f), vec3(.16f, .12f, .17f), GREEN);
        ellipsoid(&b, vec3(side * .14f, -.44f, .045f), vec3(.21f, .12f, .40f),
                  (Color){.16f, .10f, .47f});
    }
    ellipsoid(&b, vec3(0, .395f, 0), vec3(.025f, .13f, .025f), GREEN);
    ellipsoid(&b, vec3(0, .46f, 0), vec3(.08f, .08f, .08f), GREEN);
    for (int eye = -1; eye <= 1; ++eye) {
        ellipsoid(&b, vec3(eye * .16f, .18f, .225f), vec3(.145f, .14f, .10f), WHITE);
        ellipsoid(&b, vec3(eye * .16f, .18f, .276f), vec3(.045f, .06f, .024f), INK);
    }
    ellipsoid(&b, vec3(0, .015f, .224f), vec3(.13f, .045f, .026f), INK);
    ellipsoid(&b, vec3(.04f, -.17f, .185f), vec3(.10f, .10f, .035f), BLUE);
    ring(&b, vec3(.04f, -.17f, .207f), .076f, .024f, .008f, YELLOW);
    Mat4 facing = mat4_rotate_y(35);
    for (size_t i = 0; i < b.count; ++i) {
        b.v[i].position = mat4_point(facing, b.v[i].position);
        b.v[i].normal = mat4_vector(facing, b.v[i].normal);
    }
    Geometry *geometry = finish_mesh(&b);
    /* The whole character fits this conservative unit envelope. Keeping it
     * explicit preserves the geometric grasp and the documented FK tutorial. */
    if (geometry)
        geometry->bounds = aabb_make(vec3(-.5f, -.5f, -.5f), vec3(.5f, .5f, .5f));
    return geometry;
}
Geometry *theme_cabinet_create(ThemePart part) {
    Builder b = begin_mesh();
    if (part == THEME_FLOOR) {
        const float profile[][2] = {{-.32f, 1}, {0, 1}};
        lathe(&b, vec3(1, 0, 0), THEME_MAP_RADIUS_X, THEME_MAP_RADIUS_Z, profile, 2, BLUE);
        for (int i = 0; i < 64; ++i) {
            float a = cg_radians(360.0f * i / 64), next = cg_radians(360.0f * (i + 1) / 64);
            face(&b, vec3(1, 0, 0),
                 vec3(1 + THEME_MAP_RADIUS_X * cosf(next), 0, THEME_MAP_RADIUS_Z * sinf(next)),
                 vec3(1 + THEME_MAP_RADIUS_X * cosf(a), 0, THEME_MAP_RADIUS_Z * sinf(a)),
                 (Color){.07f, .14f, .34f});
        }
    } else if (part == THEME_BACK_WALL) {
        static const Color panel = {.045f, .09f, .24f};
        const int curve_segments = 96;
        const float pi = 3.14159265358979323846f;
        for (int i = 0; i < curve_segments; ++i) {
            float a0 = pi + pi * (float)i / curve_segments;
            float a1 = pi + pi * (float)(i + 1) / curve_segments;
            Vec3 n0 = back_wall_inward(a0), n1 = back_wall_inward(a1);
            MeshVertex low0 = {back_wall_point(a0, 0, THEME_BACK_WALL_THICKNESS * .5f), n0, panel};
            MeshVertex low1 = {back_wall_point(a1, 0, THEME_BACK_WALL_THICKNESS * .5f), n1, panel};
            MeshVertex high1 = {back_wall_point(a1, THEME_BACK_WALL_HEIGHT,
                                                THEME_BACK_WALL_THICKNESS * .5f),
                                n1, panel};
            MeshVertex high0 = {back_wall_point(a0, THEME_BACK_WALL_HEIGHT,
                                                THEME_BACK_WALL_THICKNESS * .5f),
                                n0, panel};
            triangle(&b, low0, low1, high1);
            triangle(&b, low0, high1, high0);
        }
    } else if (part == THEME_ROOF) {
        const float dome[][2] = {{4.48f, 1},    {4.72f, 1},    {5.12f, .96f},
                                 {5.55f, .84f}, {5.98f, .67f}, {6.25f, .46f}};
        const float cap[][2] = {{6.25f, .46f}, {6.40f, .31f}, {6.48f, 0}};
        lathe(&b, vec3(1, 0, 0), 3.18f, 3.76f, dome, 6, RED);
        lathe(&b, vec3(1, 0, 0), 3.18f, 3.76f, cap, 3, YELLOW);
        const float trim[][2] = {{4.45f, 1.005f}, {4.54f, 1.005f}};
        lathe(&b, vec3(1, 0, 0), 3.18f, 3.76f, trim, 2, YELLOW);
    } else if (part == THEME_BASE) {
        const float body[][2] = {{-1.55f, .60f}, {-1.35f, .74f}, {-.72f, .94f}, {-.34f, 1}};
        const float belt[][2] = {{-.37f, 1.01f}, {-.02f, 1.01f}};
        lathe(&b, vec3(1, 0, 0), 3.18f, 3.76f, body, 4, RED);
        lathe(&b, vec3(1, 0, 0), 3.18f, 3.76f, belt, 2, BLUE);
        ellipsoid(&b, vec3(1, -.84f, 3.37f), vec3(1.40f, .78f, .055f), INK);
        ring(&b, vec3(1, -.84f, 3.405f), .76f, .44f, .07f, RED_LIGHT);
        for (int i = 0; i < 7; ++i) {
            float a = cg_radians(26 + 21.0f * i);
            Vec3 center = vec3(1 + 3.23f * cosf(a), -.18f, 3.82f * sinf(a));
            ellipsoid(&b, center, vec3(.13f, .13f, .13f), i % 2 ? WHITE : YELLOW);
        }
    } else if (part == THEME_FIN_LEFT || part == THEME_FIN_RIGHT) {
        fin(&b, part == THEME_FIN_LEFT ? -1.0f : 1.0f);
    }
    return finish_mesh(&b);
}
