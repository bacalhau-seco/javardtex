#ifndef COLLISION_H
#define COLLISION_H

#include <stdbool.h>

#include "raylib.h"
#include "bsp.h"

/* Player hull and view geometry (GoldSrc compatible, matches the BSP clipnode
 * hulls the compilers build). Not cvars: changing these breaks map collision. */
#define COLLISION_PLAYER_HALF_WIDTH 16.0f
#define COLLISION_STAND_HEIGHT 72.0f
#define COLLISION_CROUCH_HEIGHT 36.0f
#define COLLISION_VIEW_OFFSET 28.0f
#define COLLISION_CROUCH_VIEW_OFFSET 12.0f

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
bool Collision_TouchesModel(const BSP_Map *map, int model, Vector3 origin, Vector3 feet, bool crouched);
bool Collision_IsSolid(const BSP_Map *map, Vector3 feet, bool crouched);
bool Collision_Trace(const BSP_Map *map, Vector3 startFeet, Vector3 endFeet, bool crouched, Collision_TraceResult *trace);
float Collision_PlayerHeight(bool crouched);
float Collision_EyeHeight(bool crouched);

#endif
