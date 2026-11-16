#ifndef CG_GEOMETRY_H
#define CG_GEOMETRY_H

#include "cg/math.h"
#include <stddef.h>

typedef struct {
    float r, g, b;
} Color;
typedef struct Geometry Geometry;
struct Geometry {
    Color color;
    float opacity; /* Multiplies rendered alpha; 1 is opaque. */
    AABB bounds;
    const char *name;             /* Borrowed static string. */
    void (*draw)(Geometry *self); /* Requires a current compatibility GL context. */
    Geometry *(*set_color)(Geometry *self, Color color);
    void (*destroy)(Geometry *self); /* Also valid without an OpenGL context. */
};
typedef struct {
    Geometry base;
} Box;
typedef struct {
    Geometry base;
    unsigned segments;
} Cylinder;
/* A triangle mesh with authored per-vertex normals and colors. The base color
 * multiplies vertex colors, so set_color also works as a tint on a mesh. */
typedef struct {
    Vec3 position, normal;
    Color color;
} MeshVertex;
typedef struct {
    Geometry base;
    MeshVertex *vertices;
    size_t vertex_count;
} Mesh;
typedef struct {
    Box *(*create)(Color color);
} BoxFactory;
typedef struct {
    Cylinder *(*create)(Color color, unsigned segments);
} CylinderFactory;
typedef struct {
    /* Copies vertices; count must be positive and divisible by three.
     * Calculates local bounds. Returns NULL on invalid input/allocation failure. */
    Mesh *(*create)(const MeshVertex *vertices, size_t count);
} MeshFactory;
/* Centered unit box and unit-height Y cylinder of radius 0.5. NULL on failure.
 * Each successful factory result has one owner, and must be destroyed once. */
extern const BoxFactory BoxFabric;
extern const CylinderFactory CylinderFabric;
extern const MeshFactory MeshFabric;

#endif
