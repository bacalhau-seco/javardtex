#include "../include/collision.h"

#define PLAYER_FEET_TO_ORIGIN 36.0f

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
    return plane->normal[0] * p.x
        + plane->normal[1] * p.y
        + plane->normal[2] * p.z
        - plane->dist;
}

bool Collision_GetSpawn(const BSP_Map *map, Vector3 *feet)
{
    float x;
    float y;
    float z;

    if (!feet || !BSP_GetPlayerStart(map, &x, &y, &z))
        return false;

    *feet = QuakeToWorld((Vector3){ x, y, z });
    feet->y -= PLAYER_FEET_TO_ORIGIN;

    return true;
}

bool Collision_IsSolid(const BSP_Map *map, Vector3 feet)
{
    Vector3 origin;
    int num;

    if (!map || map->num_models == 0)
        return false;

    origin = feet;
    origin.y += PLAYER_FEET_TO_ORIGIN;
    origin = WorldToQuake(origin);

    num = map->models[0].headnode[BSP_HULL_PLAYER];

    while (num >= 0)
    {
        const BSP_ClipNode *node;
        const BSP_Plane *plane;

        if (num >= map->num_clipnodes)
            return false;

        node = &map->clipnodes[num];
        plane = &map->planes[node->planenum];

        num = node->children[PlaneDistance(plane, origin) < 0.0f ? 1 : 0];
    }

    return num == BSP_CONTENTS_SOLID;
}
