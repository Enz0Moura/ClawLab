#ifndef CG_MATH_H
#define CG_MATH_H

#include <stdbool.h>

/* Column vectors, column-major storage, right-handed coordinates, Y up.
 * Public rotation/FOV arguments are degrees. Matrices compose right to left. */
typedef struct {
    float x, y, z;
} Vec3;
typedef struct {
    float m[16];
} Mat4;
typedef struct {
    Vec3 origin, direction;
} Ray;
typedef struct AABB AABB;
struct AABB {
    Vec3 min, max;
    /* C-style methods. Construct boxes with aabb_make() or AABB_INIT(). */
    void (*transform)(AABB *self, Mat4 world);
    bool (*overlaps)(const AABB *self, const AABB *other, float tolerance);
    bool (*intersects_ray)(const AABB *self, Ray ray, float *distance);
};
void aabb_transform_method(AABB *self, Mat4 world);
bool aabb_overlap_method(const AABB *self, const AABB *other, float tolerance);
bool ray_aabb_method(const AABB *self, Ray ray, float *distance);

/* Aggregate initializer; arguments are min.x/y/z followed by max.x/y/z. */
#define AABB_INIT(min_x, min_y, min_z, max_x, max_y, max_z)                                  \
    {                                                                                         \
        .min = {min_x, min_y, min_z}, .max = {max_x, max_y, max_z},                           \
        .transform = aabb_transform_method, .overlaps = aabb_overlap_method,                  \
        .intersects_ray = ray_aabb_method                                                     \
    }

Vec3 vec3(float x, float y, float z);
Vec3 v3_add(Vec3 a, Vec3 b);
Vec3 v3_sub(Vec3 a, Vec3 b);
Vec3 v3_scale(Vec3 v, float s);
float v3_dot(Vec3 a, Vec3 b);
Vec3 v3_cross(Vec3 a, Vec3 b);
float v3_length(Vec3 v);
Vec3 v3_normalize(Vec3 v); /* A zero vector remains zero. */
float cg_clamp(float x, float lo, float hi);
float cg_radians(float degrees);
Mat4 mat4_identity(void);
Mat4 mat4_mul(Mat4 a, Mat4 b);
Mat4 mat4_translate(Vec3 position);
Mat4 mat4_scale(Vec3 scale);
Mat4 mat4_rotate_x(float degrees);
Mat4 mat4_rotate_y(float degrees);
Mat4 mat4_rotate_z(float degrees);
Mat4 mat4_perspective(float fov_y, float aspect, float near_z, float far_z);
Mat4 mat4_look_at(Vec3 eye, Vec3 target, Vec3 up);
Vec3 mat4_point(Mat4 m, Vec3 p);             /* Homogeneous point, including divide by w. */
Vec3 mat4_vector(Mat4 m, Vec3 v);            /* Direction: w = 0. */
bool mat4_inverse(Mat4 m, Mat4 *out);        /* False if singular; out unchanged. */
AABB aabb_make(Vec3 min, Vec3 max);
AABB aabb_transform(AABB local, Mat4 world); /* Transforms all eight corners. */
bool aabb_overlap(AABB a, AABB b, float tolerance);
bool ray_aabb(Ray ray, AABB box, float *distance); /* Slabs; nearest t >= 0. */

#endif
