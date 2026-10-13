#include "../include/collision.h"
#include "../include/cvar.h"

#include "raymath.h"

#include <stdio.h>

#define COLLISION_PLAYER_HALF_WIDTH 16.0f

static Vector3 WorldToQuake(Vector3 w)
{
    return (Vector3){ w.x, -w.z, w.y };
}

static Vector3 QuakeToWorld(Vector3 q)
{
    return (Vector3){ q.x, q.z, -q.y };
}

static float PlaneDistance(const BSP_Plane *plane, Vector3 p)
{
    return Vector3DotProduct(plane->normal, p) - plane->dist;
}

float Collision_PlayerHeight(bool crouched)
{
    return crouched
        ? Cvar_Value("crouchheight", 36.0f)
        : Cvar_Value("standheight", 72.0f);
}

float Collision_EyeHeight(bool crouched)
{
    float height = Collision_PlayerHeight(crouched);

    return height - height / 5.0f;
}

static Vector3 Collision_FeetToOrigin(Vector3 feet, bool crouched)
{
    feet.y += Collision_PlayerHeight(crouched) * 0.5f;

    return feet;
}

bool Collision_GetSpawn(const BSP_Map *map, Vector3 *feet)
{
    float x;
    float y;
    float z;

    if (!feet || !BSP_GetPlayerStart(map, &x, &y, &z))
        return false;

    *feet = QuakeToWorld((Vector3){ x, y, z });
    feet->y -= Collision_PlayerHeight(false) * 0.5f;

    return true;
}

bool Collision_CheckNextLevel(const BSP_Map *map, Vector3 feet, bool crouched, char *mapName, int mapNameSize)
{
    BSP_NextLevel next;

    if (!mapName || mapNameSize <= 0 || !BSP_GetNextLevel(map, &next))
        return false;

    Vector3 origin = WorldToQuake(Collision_FeetToOrigin(feet, crouched));

    const float halfWidth = COLLISION_PLAYER_HALF_WIDTH;
    const float halfHeight = Collision_PlayerHeight(crouched) * 0.5f;

    if (origin.x + halfWidth < next.mins.x || origin.x - halfWidth > next.maxs.x)
        return false;

    if (origin.y + halfWidth < next.mins.y || origin.y - halfWidth > next.maxs.y)
        return false;

    if (origin.z + halfHeight < next.mins.z || origin.z - halfHeight > next.maxs.z)
        return false;

    snprintf(mapName, (size_t)mapNameSize, "%s", next.map);

    return true;
}

bool Collision_GetSpawnYaw(const BSP_Map *map, float *yaw)
{
    float angle;

    if (!yaw || !BSP_GetPlayerStartAngle(map, &angle))
        return false;

    *yaw = (angle - 90.0f) * DEG2RAD;

    return true;
}

bool Collision_IsSolid(const BSP_Map *map, Vector3 feet, bool crouched)
{
    Vector3 origin;
    int num;

    if (!map || map->num_models == 0 || !map->clipnodes || !map->planes)
        return false;

    origin = WorldToQuake(Collision_FeetToOrigin(feet, crouched));

    num = map->models[0].headnode[crouched ? BSP_HULL_CROUCH : BSP_HULL_PLAYER];

    while (num >= 0)
    {
        const BSP_ClipNode *node;

        if (num >= map->num_clipnodes)
            return false;

        node = &map->clipnodes[num];

        if (node->planenum < 0 || node->planenum >= map->num_planes)
            return false;

        num = node->children[PlaneDistance(&map->planes[node->planenum], origin) < 0.0f ? 1 : 0];
    }

    return num == BSP_CONTENTS_SOLID;
}
