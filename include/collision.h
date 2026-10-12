#ifndef COLLISION_H
#define COLLISION_H

#include <stdbool.h>

#include "raylib.h"
#include "bsp.h"

bool Collision_GetSpawn(const BSP_Map *map, Vector3 *feet);
bool Collision_IsSolid(const BSP_Map *map, Vector3 feet, bool crouched);
float Collision_PlayerHeight(bool crouched);
float Collision_EyeHeight(bool crouched);

#endif
