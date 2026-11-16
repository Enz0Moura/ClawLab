#include "cg/math.h"

#include <float.h>
#include <math.h>

Vec3 vec3(float x, float y, float z) {
    return (Vec3){x, y, z};
}
Vec3 v3_add(Vec3 a, Vec3 b) {
    return vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}
Vec3 v3_sub(Vec3 a, Vec3 b) {
    return vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}
Vec3 v3_scale(Vec3 v, float s) {
    return vec3(v.x * s, v.y * s, v.z * s);
}
float v3_dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
Vec3 v3_cross(Vec3 a, Vec3 b) {
    return vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
float v3_length(Vec3 v) {
    return sqrtf(v3_dot(v, v));
}
Vec3 v3_normalize(Vec3 v) {
    float length = v3_length(v);
    return length > 1e-8f ? v3_scale(v, 1.0f / length) : vec3(0, 0, 0);
}
float cg_clamp(float x, float lo, float hi) {
    return fmaxf(lo, fminf(x, hi));
}
float cg_radians(float degrees) {
    return degrees * 0.017453292519943295f;
}
Mat4 mat4_identity(void) {
    return (Mat4){{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}};
}
Mat4 mat4_mul(Mat4 a, Mat4 b) {
    Mat4 r = {{0}};
    for (int col = 0; col < 4; ++col)
        for (int row = 0; row < 4; ++row)
            for (int k = 0; k < 4; ++k)
                r.m[col * 4 + row] += a.m[k * 4 + row] * b.m[col * 4 + k];
    return r;
}
Mat4 mat4_translate(Vec3 p) {
    Mat4 r = mat4_identity();
    r.m[12] = p.x;
    r.m[13] = p.y;
    r.m[14] = p.z;
    return r;
}
Mat4 mat4_scale(Vec3 s) {
    Mat4 r = mat4_identity();
    r.m[0] = s.x;
    r.m[5] = s.y;
    r.m[10] = s.z;
    return r;
}
Mat4 mat4_rotate_x(float d) {
    float c = cosf(cg_radians(d)), s = sinf(cg_radians(d));
    Mat4 r = mat4_identity();
    r.m[5] = c;
    r.m[6] = s;
    r.m[9] = -s;
    r.m[10] = c;
    return r;
}
Mat4 mat4_rotate_y(float d) {
    float c = cosf(cg_radians(d)), s = sinf(cg_radians(d));
    Mat4 r = mat4_identity();
    r.m[0] = c;
    r.m[2] = -s;
    r.m[8] = s;
    r.m[10] = c;
    return r;
}
Mat4 mat4_rotate_z(float d) {
    float c = cosf(cg_radians(d)), s = sinf(cg_radians(d));
    Mat4 r = mat4_identity();
    r.m[0] = c;
    r.m[1] = s;
    r.m[4] = -s;
    r.m[5] = c;
    return r;
}
Mat4 mat4_perspective(float fov, float aspect, float near_z, float far_z) {
    float f = 1.0f / tanf(cg_radians(fov) * 0.5f);
    Mat4 r = {{0}};
    r.m[0] = f / aspect;
    r.m[5] = f;
    r.m[10] = (far_z + near_z) / (near_z - far_z);
    r.m[11] = -1;
    r.m[14] = 2 * far_z * near_z / (near_z - far_z);
    return r;
}
Mat4 mat4_look_at(Vec3 eye, Vec3 target, Vec3 up) {
    Vec3 f = v3_normalize(v3_sub(target, eye));
    Vec3 s = v3_normalize(v3_cross(f, up)), u = v3_cross(s, f);
    Mat4 r = {{s.x, u.x, -f.x, 0, s.y, u.y, -f.y, 0, s.z, u.z, -f.z, 0, -v3_dot(s, eye),
               -v3_dot(u, eye), v3_dot(f, eye), 1}};
    return r;
}
Vec3 mat4_point(Mat4 m, Vec3 p) {
    float w = m.m[3] * p.x + m.m[7] * p.y + m.m[11] * p.z + m.m[15];
    Vec3 r = vec3(m.m[0] * p.x + m.m[4] * p.y + m.m[8] * p.z + m.m[12],
                  m.m[1] * p.x + m.m[5] * p.y + m.m[9] * p.z + m.m[13],
                  m.m[2] * p.x + m.m[6] * p.y + m.m[10] * p.z + m.m[14]);
    return fabsf(w) > 1e-8f ? v3_scale(r, 1.0f / w) : r;
}
Vec3 mat4_vector(Mat4 m, Vec3 v) {
    return vec3(m.m[0] * v.x + m.m[4] * v.y + m.m[8] * v.z,
                m.m[1] * v.x + m.m[5] * v.y + m.m[9] * v.z,
                m.m[2] * v.x + m.m[6] * v.y + m.m[10] * v.z);
}
bool mat4_inverse(Mat4 m, Mat4 *out) {
    /* Gauss-Jordan with partial pivoting; no OpenGL state is consulted. */
    float a[4][8];
    if (!out)
        return false;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 8; ++c)
            a[r][c] = c < 4 ? m.m[c * 4 + r] : (float)(c - 4 == r);
    for (int c = 0; c < 4; ++c) {
        int pivot = c;
        for (int r = c + 1; r < 4; ++r)
            if (fabsf(a[r][c]) > fabsf(a[pivot][c]))
                pivot = r;
        if (fabsf(a[pivot][c]) < 1e-8f)
            return false;
        for (int k = 0; k < 8; ++k) {
            float t = a[c][k];
            a[c][k] = a[pivot][k];
            a[pivot][k] = t;
        }
        float divisor = a[c][c];
        for (int k = 0; k < 8; ++k)
            a[c][k] /= divisor;
        for (int r = 0; r < 4; ++r)
            if (r != c) {
                float factor = a[r][c];
                for (int k = 0; k < 8; ++k)
                    a[r][k] -= factor * a[c][k];
            }
    }
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            out->m[c * 4 + r] = a[r][c + 4];
    return true;
}
AABB aabb_make(Vec3 min, Vec3 max) {
    return (AABB){.min = min,
                  .max = max,
                  .transform = aabb_transform_method,
                  .overlaps = aabb_overlap_method,
                  .intersects_ray = ray_aabb_method};
}
void aabb_transform_method(AABB *self, Mat4 m) {
    if (!self)
        return;
    Vec3 min = vec3(FLT_MAX, FLT_MAX, FLT_MAX), max = vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    for (int i = 0; i < 8; ++i) {
        Vec3 p = mat4_point(m, vec3(i & 1 ? self->max.x : self->min.x,
                                    i & 2 ? self->max.y : self->min.y,
                                    i & 4 ? self->max.z : self->min.z));
        min.x = fminf(min.x, p.x);
        min.y = fminf(min.y, p.y);
        min.z = fminf(min.z, p.z);
        max.x = fmaxf(max.x, p.x);
        max.y = fmaxf(max.y, p.y);
        max.z = fmaxf(max.z, p.z);
    }
    self->min = min;
    self->max = max;
}
bool aabb_overlap_method(const AABB *a, const AABB *b, float t) {
    return a && b && a->max.x > b->min.x + t && a->min.x < b->max.x - t &&
           a->max.y > b->min.y + t && a->min.y < b->max.y - t &&
           a->max.z > b->min.z + t && a->min.z < b->max.z - t;
}
bool ray_aabb_method(const AABB *box, Ray ray, float *distance) {
    if (!box)
        return false;
    float origin[3] = {ray.origin.x, ray.origin.y, ray.origin.z};
    float dir[3] = {ray.direction.x, ray.direction.y, ray.direction.z};
    float lo[3] = {box->min.x, box->min.y, box->min.z};
    float hi[3] = {box->max.x, box->max.y, box->max.z};
    float near_t = 0, far_t = FLT_MAX;
    if (v3_length(ray.direction) < 1e-8f)
        return false;
    for (int i = 0; i < 3; ++i) {
        if (fabsf(dir[i]) < 1e-8f) {
            if (origin[i] < lo[i] || origin[i] > hi[i])
                return false;
        } else {
            float a = (lo[i] - origin[i]) / dir[i], b = (hi[i] - origin[i]) / dir[i];
            if (a > b) {
                float t = a;
                a = b;
                b = t;
            }
            near_t = fmaxf(near_t, a);
            far_t = fminf(far_t, b);
            if (near_t > far_t)
                return false;
        }
    }
    if (distance)
        *distance = near_t;
    return true;
}
AABB aabb_transform(AABB box, Mat4 world) {
    if (!box.transform)
        box = aabb_make(box.min, box.max);
    box.transform(&box, world);
    return box;
}
bool aabb_overlap(AABB a, AABB b, float tolerance) {
    if (a.overlaps)
        return a.overlaps(&a, &b, tolerance);
    return aabb_overlap_method(&a, &b, tolerance);
}
bool ray_aabb(Ray ray, AABB box, float *distance) {
    if (box.intersects_ray)
        return box.intersects_ray(&box, ray, distance);
    return ray_aabb_method(&box, ray, distance);
}
