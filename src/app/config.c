#include "app/config.h"

const MachineConfig MACHINE_CONFIG = {
    .joints = {{"Base", -110, 65, 0, 35},
               {"Ombro", -85, 25, -25, 25},
               {"Cotovelo", -135, -15, -85, 30},
               {"Punho", -110, 80, -70, 40}},
    .shoulder_height = .60f,
    .upper_length = 2,
    .fore_length = 1.8f,
    .grip_offset = .60f,
    .finger_min = 0,
    .finger_max = 40,
    .finger_speed = 35,
    .prize_size = .50f,
    .capture_x = .09f,
    .capture_y = .10f,
    .capture_z = .10f,
    .max_step = 1.0f / 240.0f,
    .collision_tolerance = .002f,
    .gravity = 5,
    .prize_yaw = {-40, 0, 40},
    .drop_yaw = {-75, -90, -105},
    .prize_colors = {{1, .32f, .30f}, {.16f, .74f, .95f}, {1, .76f, .16f}},
    .tray_center = {0, .06f, 2.5f},
    .tray_size = {2.5f, .12f, 1.6f},
    .workspace = {{-1.8f, 0, -3.4f}, {3.7f, 4.4f, 3.4f}}};
const float PICKUP_POSE[JOINT_COUNT] = {0, -45, -85, -50};
const ModeConfig GAME_MODES[MODE_COUNT] = {
    {"Sandbox", 0, .18f, 5, 5, 3, 1},
    {"Facil", 240, .30f, 4, 5, 3, 1},
    {"Medio", 180, .55f, 3, 10, 2, 3},
    {"Dificil", 120, .78f, 2, 15, 0, 7},
};
