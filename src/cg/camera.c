#include "cg/camera.h"

#include <math.h>

Camera camera_default(void) {
    return (Camera){{1, 2.2f, 0}, 28, 18, 14.4f, 42, .1f, 60};
}
Vec3 camera_eye(const Camera *c) {
    float yaw = cg_radians(c->yaw), pitch = cg_radians(c->pitch);
    return v3_add(c->target, vec3(c->distance * cosf(pitch) * sinf(yaw), c->distance * sinf(pitch),
                                  c->distance * cosf(pitch) * cosf(yaw)));
}
Mat4 camera_view(const Camera *c) {
    return mat4_look_at(camera_eye(c), c->target, vec3(0, 1, 0));
}
Mat4 camera_projection(const Camera *c, float aspect) {
    return mat4_perspective(c->fov, aspect, c->near_z, c->far_z);
}
void camera_orbit(Camera *c, float yaw, float pitch) {
    c->yaw = fmodf(c->yaw + yaw, 360);
    c->pitch = cg_clamp(c->pitch + pitch, 8, 78);
}
void camera_zoom(Camera *c, float delta) {
    c->distance = cg_clamp(c->distance + delta, 6, 22);
}
void camera_pan(Camera *c, float dx, float dy, int height) {
    if (height <= 0 || !isfinite(dx) || !isfinite(dy))
        return;
    Vec3 forward = v3_normalize(v3_sub(c->target, camera_eye(c)));
    Vec3 right = v3_normalize(v3_cross(forward, vec3(0, 1, 0)));
    Vec3 up = v3_cross(right, forward);
    float scale = 2 * c->distance * tanf(cg_radians(c->fov) * .5f) / (float)height;
    c->target = v3_add(c->target, v3_add(v3_scale(right, -dx * scale), v3_scale(up, dy * scale)));
}
void camera_look(Camera *c, float yaw, float pitch) {
    if (!isfinite(yaw) || !isfinite(pitch))
        return;
    Vec3 eye = camera_eye(c);
    c->yaw = fmodf(c->yaw + yaw, 360);
    c->pitch = cg_clamp(c->pitch + pitch, -89, 89);
    c->target = v3_add(c->target, v3_sub(eye, camera_eye(c)));
}
void camera_dolly(Camera *c, float distance) {
    if (isfinite(distance))
        c->target =
            v3_add(c->target, v3_scale(v3_normalize(v3_sub(c->target, camera_eye(c))), distance));
}
bool camera_ray(const Camera *c, float x, float y, int width, int height, Ray *out) {
    Mat4 inverse;
    if (!out || width <= 0 || height <= 0 ||
        !mat4_inverse(mat4_mul(camera_projection(c, (float)width / (float)height), camera_view(c)),
                      &inverse))
        return false;
    float nx = 2 * x / (float)width - 1, ny = 1 - 2 * y / (float)height;
    Vec3 near_point = mat4_point(inverse, vec3(nx, ny, -1)),
         far_point = mat4_point(inverse, vec3(nx, ny, 1));
    *out = (Ray){near_point, v3_normalize(v3_sub(far_point, near_point))};
    return true;
}
bool camera_project(const Camera *c, Vec3 point, int width, int height, Vec3 *screen) {
    if (!screen || width <= 0 || height <= 0)
        return false;
    Vec3 view = mat4_point(camera_view(c), point);
    if (view.z >= -c->near_z)
        return false;
    Vec3 ndc = mat4_point(camera_projection(c, (float)width / (float)height), view);
    *screen = vec3((ndc.x + 1) * .5f * (float)width, (1 - ndc.y) * .5f * (float)height, ndc.z);
    return ndc.z >= -1 && ndc.z <= 1;
}
