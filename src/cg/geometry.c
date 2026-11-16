#include "cg/geometry.h"

#include <GL/freeglut.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static Geometry *set_color(Geometry *self, Color color) {
    self->color = color;
    return self;
}
static void box_destroy(Geometry *self) {
    free((Box *)self);
}
static void cylinder_destroy(Geometry *self) {
    free((Cylinder *)self);
}
static void mesh_destroy(Geometry *self) {
    Mesh *mesh = (Mesh *)self;
    free(mesh->vertices);
    free(mesh);
}
static void mesh_draw(Geometry *self) {
    const Mesh *mesh = (const Mesh *)self;
    float alpha = cg_clamp(self->opacity, 0, 1);
    glBegin(GL_TRIANGLES);
    for (size_t i = 0; i < mesh->vertex_count; ++i) {
        const MeshVertex *v = &mesh->vertices[i];
        glColor4f(v->color.r * self->color.r, v->color.g * self->color.g,
                  v->color.b * self->color.b, alpha);
        glNormal3f(v->normal.x, v->normal.y, v->normal.z);
        glVertex3f(v->position.x, v->position.y, v->position.z);
    }
    glEnd();
}
static void box_draw(Geometry *self) {
    static const float normals[6][3] = {{1, 0, 0},  {-1, 0, 0}, {0, 1, 0},
                                        {0, -1, 0}, {0, 0, 1},  {0, 0, -1}};
    static const float vertices[6][4][3] = {
        {{.5f, -.5f, -.5f}, {.5f, .5f, -.5f}, {.5f, .5f, .5f}, {.5f, -.5f, .5f}},
        {{-.5f, -.5f, .5f}, {-.5f, .5f, .5f}, {-.5f, .5f, -.5f}, {-.5f, -.5f, -.5f}},
        {{-.5f, .5f, -.5f}, {-.5f, .5f, .5f}, {.5f, .5f, .5f}, {.5f, .5f, -.5f}},
        {{-.5f, -.5f, .5f}, {-.5f, -.5f, -.5f}, {.5f, -.5f, -.5f}, {.5f, -.5f, .5f}},
        {{-.5f, -.5f, .5f}, {.5f, -.5f, .5f}, {.5f, .5f, .5f}, {-.5f, .5f, .5f}},
        {{.5f, -.5f, -.5f}, {-.5f, -.5f, -.5f}, {-.5f, .5f, -.5f}, {.5f, .5f, -.5f}}};
    glColor4f(self->color.r, self->color.g, self->color.b, cg_clamp(self->opacity, 0, 1));
    glBegin(GL_QUADS);
    for (int face = 0; face < 6; ++face) {
        glNormal3fv(normals[face]);
        for (int corner = 0; corner < 4; ++corner)
            glVertex3fv(vertices[face][corner]);
    }
    glEnd();
}
static void cylinder_draw(Geometry *self) {
    Cylinder *cylinder = (Cylinder *)self;
    glColor4f(self->color.r, self->color.g, self->color.b, cg_clamp(self->opacity, 0, 1));
    glBegin(GL_QUAD_STRIP);
    for (unsigned i = 0; i <= cylinder->segments; ++i) {
        float a = cg_radians(360.0f * (float)i / (float)cylinder->segments), x = cosf(a),
              z = sinf(a);
        glNormal3f(x, 0, z);
        glVertex3f(.5f * x, .5f, .5f * z);
        glVertex3f(.5f * x, -.5f, .5f * z);
    }
    glEnd();
    for (int side = -1; side <= 1; side += 2) {
        glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0, (float)side, 0);
        glVertex3f(0, .5f * (float)side, 0);
        for (unsigned i = 0; i <= cylinder->segments; ++i) {
            float a = cg_radians(-(float)side * 360.0f * (float)i / (float)cylinder->segments);
            glVertex3f(.5f * cosf(a), .5f * (float)side, .5f * sinf(a));
        }
        glEnd();
    }
}
static Geometry base(Color color, const char *name, void (*draw)(Geometry *),
                     void (*destroy)(Geometry *)) {
    return (Geometry){.color = color,
                      .opacity = 1,
                      .bounds = AABB_INIT(-.5f, -.5f, -.5f, .5f, .5f, .5f),
                      .name = name,
                      .draw = draw,
                      .set_color = set_color,
                      .destroy = destroy};
}
static Box *box_create(Color color) {
    Box *box = malloc(sizeof(*box));
    if (box)
        box->base = base(color, "Box", box_draw, box_destroy);
    return box;
}
static Cylinder *cylinder_create(Color color, unsigned segments) {
    if (segments < 3 || segments > 256)
        return NULL;
    Cylinder *cylinder = malloc(sizeof(*cylinder));
    if (cylinder) {
        cylinder->base = base(color, "Cylinder", cylinder_draw, cylinder_destroy);
        cylinder->segments = segments;
    }
    return cylinder;
}
static Mesh *mesh_create(const MeshVertex *vertices, size_t count) {
    if (!vertices || count < 3 || count % 3 || count > SIZE_MAX / sizeof(*vertices))
        return NULL;
    AABB bounds = aabb_make(vertices[0].position, vertices[0].position);
    for (size_t i = 0; i < count; ++i) {
        const MeshVertex *v = &vertices[i];
        if (!isfinite(v->position.x) || !isfinite(v->position.y) || !isfinite(v->position.z) ||
            !isfinite(v->normal.x) || !isfinite(v->normal.y) || !isfinite(v->normal.z) ||
            !isfinite(v->color.r) || !isfinite(v->color.g) || !isfinite(v->color.b))
            return NULL;
        bounds.min.x = fminf(bounds.min.x, v->position.x);
        bounds.min.y = fminf(bounds.min.y, v->position.y);
        bounds.min.z = fminf(bounds.min.z, v->position.z);
        bounds.max.x = fmaxf(bounds.max.x, v->position.x);
        bounds.max.y = fmaxf(bounds.max.y, v->position.y);
        bounds.max.z = fmaxf(bounds.max.z, v->position.z);
    }
    Mesh *mesh = malloc(sizeof(*mesh));
    if (!mesh)
        return NULL;
    mesh->vertices = malloc(count * sizeof(*vertices));
    if (!mesh->vertices) {
        free(mesh);
        return NULL;
    }
    memcpy(mesh->vertices, vertices, count * sizeof(*vertices));
    mesh->vertex_count = count;
    mesh->base = base((Color){1, 1, 1}, "Mesh", mesh_draw, mesh_destroy);
    mesh->base.bounds = bounds;
    return mesh;
}
const BoxFactory BoxFabric = {box_create};
const CylinderFactory CylinderFabric = {cylinder_create};
const MeshFactory MeshFabric = {mesh_create};
