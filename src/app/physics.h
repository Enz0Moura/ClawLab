#ifndef APP_PHYSICS_H
#define APP_PHYSICS_H

#include "app/machine.h"

/* Axis sweeps, equal-mass impulses and damped settling; no allocations. */
void physics_step(PrizeMachine *machine, float dt);

#endif
