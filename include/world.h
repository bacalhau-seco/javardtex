#ifndef WORLD_H
#define WORLD_H

#include "move.h"
#include "input.h"
#include "bsp.h"

#define WORLD_MAX_FRAME_DELTA 0.0625f

void World_Reset(Vector3 feet);
void World_Pause(void);
void World_Update(const BSP_Map *map, InputState input, float yaw, float delta);


const Body *World_Body(void);
Body World_ViewBody(void);

#endif
