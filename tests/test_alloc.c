/* GNU/LLVM linker wrapping injects failure in author-owned allocations only.
 * No production allocator hooks, graphics context, or system-wide failures. */
#include "app/machine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *__real_malloc(size_t size);
void *__real_calloc(size_t count, size_t size);
void __real_free(void *pointer);
static size_t calls, fail_at, live;
void *__wrap_malloc(size_t size) {
    if (++calls == fail_at)
        return NULL;
    void *p = __real_malloc(size);
    if (p)
        ++live;
    return p;
}
void *__wrap_calloc(size_t count, size_t size) {
    if (++calls == fail_at)
        return NULL;
    void *p = __real_calloc(count, size);
    if (p)
        ++live;
    return p;
}
void __wrap_free(void *pointer) {
    if (pointer) {
        if (!live) {
            fputs("FAIL: unmatched free\n", stderr);
            exit(EXIT_FAILURE);
        }
        --live;
    }
    __real_free(pointer);
}
#define CHECK(condition)                                                                           \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            fprintf(stderr, "FAIL allocation %zu: %s\n", fail_at, #condition);                     \
            exit(EXIT_FAILURE);                                                                    \
        }                                                                                          \
    } while (0)
int main(void) {
    PrizeMachine *m = machine_create();
    CHECK(m);
    size_t total = calls;
    machine_destroy(m);
    CHECK(live == 0);
    for (size_t i = 1; i <= total; ++i) {
        calls = 0;
        fail_at = i;
        m = machine_create();
        CHECK(m == NULL);
        CHECK(live == 0);
    }
    fail_at = 0;
    m = machine_create();
    CHECK(m);
    CHECK(machine_start(m, MODE_HARD));
    m->time_remaining = 42;
    SceneNode *root = m->root;
    m->selected = 1;
    PrizeMachine saved;
    memcpy(&saved, m, sizeof(saved));
    Mat4 positions[PRIZE_COUNT];
    for (int p = 0; p < PRIZE_COUNT; ++p)
        positions[p] = m->prizes[p].node->local;
    size_t baseline = live;
    for (size_t i = 1; i <= total; ++i) {
        calls = 0;
        fail_at = i;
        CHECK(!machine_reset(m));
        CHECK(!memcmp(&saved, m, sizeof(saved)));
        for (int p = 0; p < PRIZE_COUNT; ++p)
            CHECK(!memcmp(&positions[p], &m->prizes[p].node->local, sizeof(Mat4)));
        CHECK(m->root == root);
        CHECK(m->selected == 1);
        CHECK(m->mode == MODE_HARD && m->time_remaining == 42);
        CHECK(live == baseline);
        CHECK(machine_pose_valid(m));
    }
    fail_at = 0;
    for (size_t i = 1; i <= total; ++i) {
        calls = 0;
        fail_at = i;
        CHECK(!machine_start(m, MODE_EASY));
        CHECK(!memcmp(&saved, m, sizeof(saved)));
        CHECK(m->root == root && m->selected == 1);
        CHECK(m->mode == MODE_HARD && m->time_remaining == 42);
        CHECK(live == baseline);
    }
    fail_at = 0;
    CHECK(machine_reset(m));
    CHECK(m->selected == -1);
    CHECK(m->mode == MODE_HARD && m->time_remaining == GAME_MODES[MODE_HARD].time_limit);
    CHECK(live == baseline);
    machine_destroy(m);
    CHECK(live == 0);
    printf("PASS: failure at all %zu allocations during creation, reset and mode changes; no "
           "abandoned owners\n",
           total);
    return EXIT_SUCCESS;
}
