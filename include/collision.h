#ifndef COLLISION_H
#define COLLISION_H

#include <stdbool.h>

#include "raylib.h"
#include "bsp.h"

typedef struct {
    bool allsolid;
    bool startsolid;
    bool hit;
    float fraction;
    Vector3 endpos;
    Vector3 normal;
} Collision_TraceResult;

bool Collision_GetSpawn(const BSP_Map *map, Vector3 *feet);
bool Collision_GetSpawnYaw(const BSP_Map *map, float *yaw);
bool Collision_CheckNextLevel(const BSP_Map *map, Vector3 feet, bool crouched, char *mapName, int mapNameSize);
bool Collision_IsSolid(const BSP_Map *map, Vector3 feet, bool crouched);
bool Collision_Trace(const BSP_Map *map, Vector3 startFeet, Vector3 endFeet, bool crouched, Collision_TraceResult *trace);
float Collision_PlayerHeight(bool crouched);
float Collision_EyeHeight(bool crouched);

#endif
