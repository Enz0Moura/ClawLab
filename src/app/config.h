#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "cg/geometry.h"

enum { JOINT_COUNT = 4, PRIZE_COUNT = 3, MAX_SOLIDS = 24, ARM_PART_COUNT = 9 };
typedef struct {
    const char *name;
    float min, max, initial, speed;
} JointConfig;
typedef struct {
    JointConfig joints[JOINT_COUNT];
    float shoulder_height, upper_length, fore_length, grip_offset;
    float finger_min, finger_max, finger_speed;
    float prize_size, capture_x, capture_y, capture_z;
    float max_step, collision_tolerance, gravity;
    float prize_yaw[PRIZE_COUNT], drop_yaw[PRIZE_COUNT];
    Color prize_colors[PRIZE_COUNT];
    Vec3 tray_center, tray_size;
    AABB workspace; /* Interior safety envelope, including the open sides. */
} MachineConfig;
extern const MachineConfig MACHINE_CONFIG;
typedef enum { MODE_SANDBOX, MODE_EASY, MODE_MEDIUM, MODE_HARD, MODE_COUNT } GameMode;
typedef struct {
    const char *name;
    float time_limit, restitution, friction;
    int spawn_yaw_jitter, spawn_pose_first, spawn_pose_count;
} ModeConfig;
extern const ModeConfig GAME_MODES[MODE_COUNT];
/* Known FK pose used by the safe fallback layout. No runtime IK. */
extern const float PICKUP_POSE[JOINT_COUNT];

#endif
