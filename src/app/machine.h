#ifndef APP_MACHINE_H
#define APP_MACHINE_H

#include "app/config.h"
#include "cg/scene.h"
#include <stdint.h>

typedef enum { PRIZE_AVAILABLE, PRIZE_HELD, PRIZE_FALLING, PRIZE_DELIVERED } PrizeState;
typedef enum { ROUND_PLAYING, ROUND_WON, ROUND_LOST } RoundState;
enum { MACHINE_IK_PATH_MAX = 128 };
#define MACHINE_IK_POSITION_TOLERANCE .05f
#define MACHINE_IK_ORIENTATION_TOLERANCE 4.0f
typedef struct {
    SceneNode *node; /* All node references are borrowed from root. */
    PrizeState state;
    Mat4 relative_to_grip;
    Vec3 velocity;
    const char *name;
    float pickup_pose[JOINT_COUNT]; /* Spawn's validated FK solution; not an autopilot. */
} Prize;
typedef struct {
    float current[JOINT_COUNT]; /* Pose used by FK to build the scene transforms. */
    float target[JOINT_COUNT];  /* Desired pose shared by manual control and IK. */
} JointAngles;
typedef struct {
    SceneNode *joint[JOINT_COUNT], *finger[2], *grip;
    SceneNode *parts[ARM_PART_COUNT]; /* Upper, forearm, palm, fingers, three hubs, rotor. */
    JointAngles angles;
    float opening, opening_target;
} RoboticArm;
typedef struct {
    SceneNode *node;
    const char *name;
} Solid;
typedef struct {
    Vec3 target;
    Mat4 target_orientation;
    float error, orientation_error; /* World-space distance and angular degrees. */
    bool target_active;
    bool orientation_active;
    float path[MACHINE_IK_PATH_MAX][JOINT_COUNT];
    int path_count, path_index;
} IkState;
typedef struct {
    SceneNode *root; /* Sole owner of every node and geometry. */
    RoboticArm arm;
    Prize prizes[PRIZE_COUNT];
    Solid solids[MAX_SOLIDS];
    int solid_count, tray_solid, held, selected, delivered, release_contact, capture_target;
    unsigned blocked_moves;
    char message[96];
    float message_time;
    IkState ik;
    GameMode mode;
    RoundState round;
    float time_remaining;
    Vec3 held_velocity;
    unsigned bounces;
    uint32_t spawn_seed;
} PrizeMachine;

PrizeMachine *machine_create(void); /* NULL on allocation failure. No GL needed. */
/* Reproducible new round, including seed zero. NULL on invalid mode or failure. */
PrizeMachine *machine_create_seeded(GameMode mode, uint32_t seed);
void machine_destroy(PrizeMachine *machine);
bool machine_reset(PrizeMachine *machine);                /* Transactional; unchanged on failure. */
bool machine_start(PrizeMachine *machine, GameMode mode); /* Transactional new round. */
void machine_update(PrizeMachine *machine, float dt);
void machine_target_joint(PrizeMachine *machine, int joint, float degrees);
void machine_nudge_joint(PrizeMachine *machine, int joint, float degrees);
void machine_open(PrizeMachine *machine); /* Releases immediately at current world pose. */
void machine_close(PrizeMachine *machine);
/* Starts a close only while this exact prize is a valid capture candidate. */
bool machine_close_for_prize(PrizeMachine *machine, int prize);
int machine_pick(PrizeMachine *machine, Ray ray); /* Sets selected, returns ID or -1. */
int machine_capture_candidate(const PrizeMachine *machine, float *required_gap);
Vec3 machine_grip_position(const PrizeMachine *machine);
/* Collision-aware CCD position IK; a safe approximation may be queued on false. */
bool machine_solve_ik(PrizeMachine *machine, Vec3 target, float *error);
/* Position and full gripper orientation in world space; errors are meters and degrees. */
bool machine_solve_ik_pose(PrizeMachine *machine, Vec3 target, Mat4 target_orientation,
                           float *position_error, float *orientation_error);
/* Targets a prize center with the gripper aligned to its yaw and approaching from above. */
bool machine_solve_ik_capture(PrizeMachine *machine, int prize, float *position_error,
                              float *orientation_error);
AABB machine_delivery_region(const PrizeMachine *machine);
bool machine_pose_valid(const PrizeMachine *machine); /* Current scene matrices must be updated. */
bool machine_at_target(const PrizeMachine *machine);
const char *prize_state_name(PrizeState state);

#endif
