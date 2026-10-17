#ifndef WORLD_H
#define WORLD_H

#include "move.h"
#include "input.h"
#include "bsp.h"

#define WORLD_TICK_RATE_MIN 10.0f
#define WORLD_TICK_RATE_MAX 1000.0f
#define WORLD_TICK_MAX_STEPS 8

void World_Reset(Vector3 feet);
void World_Pause(void);
void World_Update(const BSP_Map *map, InputState input, float yaw, float delta);

float World_TickDelta(void);

const Body *World_Body(void);
Body World_ViewBody(void);

#endif
