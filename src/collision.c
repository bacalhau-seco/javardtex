#include "../include/collision.h"
#include "../include/cvar.h"

#include "raymath.h"

#include <stdio.h>

#define COLLISION_PLAYER_HALF_WIDTH 16.0f
#define COLLISION_DIST_EPSILON 0.03125f
#define COLLISION_TRACE_BUMPS 0.1f
#define COLLISION_MAX_TRACE_DEPTH 256

static inline float PlaneDistance(const BSP_Plane *plane, Vector3 p)
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

static inline Vector3 Collision_FeetToOrigin(Vector3 feet, bool crouched)
{
    feet.z += Collision_PlayerHeight(crouched) * 0.5f;

    return feet;
}

static inline Vector3 Collision_OriginToFeet(Vector3 origin, bool crouched)
{
    origin.z -= Collision_PlayerHeight(crouched) * 0.5f;

    return origin;
}

bool Collision_GetSpawn(const BSP_Map *map, Vector3 *feet)
{
    float x;
    float y;
    float z;

    if (!feet || !BSP_GetPlayerStart(map, &x, &y, &z))
        return false;

    *feet = (Vector3){ x, y, z };
    feet->z -= Collision_PlayerHeight(false) * 0.5f;

    return true;
}

bool Collision_CheckNextLevel(const BSP_Map *map, Vector3 feet, bool crouched, char *mapName, int mapNameSize)
{
    BSP_NextLevel next;
    int index;

    if (!mapName || mapNameSize <= 0)
        return false;

    Vector3 origin = Collision_FeetToOrigin(feet, crouched);

    const float halfWidth = COLLISION_PLAYER_HALF_WIDTH;
    const float halfHeight = Collision_PlayerHeight(crouched) * 0.5f;

    for (index = 0; BSP_GetNextLevelByIndex(map, index, &next); index++)
    {
        if (origin.x + halfWidth < next.mins.x || origin.x - halfWidth > next.maxs.x)
            continue;

        if (origin.y + halfWidth < next.mins.y || origin.y - halfWidth > next.maxs.y)
            continue;

        if (origin.z + halfHeight < next.mins.z || origin.z - halfHeight > next.maxs.z)
            continue;

        snprintf(mapName, (size_t)mapNameSize, "%s", next.map);

        return true;
    }

    return false;
}

bool Collision_GetSpawnYaw(const BSP_Map *map, float *yaw)
{
    float angle;

    if (!yaw || !BSP_GetPlayerStartAngle(map, &angle))
        return false;

    *yaw = (angle - 90.0f) * DEG2RAD;

    return true;
}

static int Collision_HullPointContents(const BSP_Map *map, int num, Vector3 origin)
{
    while (num >= 0)
    {
        const BSP_ClipNode *node;

        if (num >= map->num_clipnodes)
            return BSP_CONTENTS_EMPTY;

        node = &map->clipnodes[num];

        if (node->planenum < 0 || node->planenum >= map->num_planes)
            return BSP_CONTENTS_EMPTY;

        num = node->children[PlaneDistance(&map->planes[node->planenum], origin) < 0.0f ? 1 : 0];
    }

    return num;
}

static inline int Collision_HullHeadnode(const BSP_Map *map, bool crouched)
{
    return map->models[0].headnode[crouched ? BSP_HULL_CROUCH : BSP_HULL_PLAYER];
}

bool Collision_IsSolid(const BSP_Map *map, Vector3 feet, bool crouched)
{
    Vector3 origin;

    if (!map || map->num_models == 0 || !map->clipnodes || !map->planes)
        return false;

    origin = Collision_FeetToOrigin(feet, crouched);

    return Collision_HullPointContents(map, Collision_HullHeadnode(map, crouched), origin) == BSP_CONTENTS_SOLID;
}

typedef struct {
    const BSP_Map *map;
    int headnode;
    int depth;
    Collision_TraceResult *trace;
} Collision_HullTrace;

static bool Collision_RecursiveHullCheck(Collision_HullTrace *hull, int num, float p1f, float p2f, Vector3 p1, Vector3 p2)
{
    const BSP_ClipNode *node;
    const BSP_Plane *plane;
    float t1;
    float t2;
    float frac;
    float midf;
    Vector3 mid;
    int side;

    if (num < 0)
    {
        if (num != BSP_CONTENTS_SOLID)
            hull->trace->allsolid = false;
        else
            hull->trace->startsolid = true;

        return true;
    }

    if (num >= hull->map->num_clipnodes || hull->depth >= COLLISION_MAX_TRACE_DEPTH)
    {
        hull->trace->allsolid = false;
        return true;
    }

    node = &hull->map->clipnodes[num];

    if (node->planenum < 0 || node->planenum >= hull->map->num_planes)
    {
        hull->trace->allsolid = false;
        return true;
    }

    plane = &hull->map->planes[node->planenum];
    t1 = PlaneDistance(plane, p1);
    t2 = PlaneDistance(plane, p2);

    hull->depth++;

    if (t1 >= 0.0f && t2 >= 0.0f)
    {
        bool ok = Collision_RecursiveHullCheck(hull, node->children[0], p1f, p2f, p1, p2);

        hull->depth--;
        return ok;
    }

    if (t1 < 0.0f && t2 < 0.0f)
    {
        bool ok = Collision_RecursiveHullCheck(hull, node->children[1], p1f, p2f, p1, p2);

        hull->depth--;
        return ok;
    }

    frac = t1 < 0.0f
        ? (t1 + COLLISION_DIST_EPSILON) / (t1 - t2)
        : (t1 - COLLISION_DIST_EPSILON) / (t1 - t2);

    if (frac < 0.0f)
        frac = 0.0f;

    if (frac > 1.0f)
        frac = 1.0f;

    midf = p1f + (p2f - p1f) * frac;
    mid = Vector3Add(p1, Vector3Scale(Vector3Subtract(p2, p1), frac));
    side = t1 < 0.0f ? 1 : 0;

    if (!Collision_RecursiveHullCheck(hull, node->children[side], p1f, midf, p1, mid))
    {
        hull->depth--;
        return false;
    }

    if (Collision_HullPointContents(hull->map, node->children[side ^ 1], mid) != BSP_CONTENTS_SOLID)
    {
        bool ok = Collision_RecursiveHullCheck(hull, node->children[side ^ 1], midf, p2f, mid, p2);

        hull->depth--;
        return ok;
    }

    hull->depth--;

    if (hull->trace->allsolid)
        return false;

    hull->trace->hit = true;
    hull->trace->normal = side == 0
        ? plane->normal
        : Vector3Negate(plane->normal);

    while (Collision_HullPointContents(hull->map, hull->headnode, mid) == BSP_CONTENTS_SOLID)
    {
        frac -= COLLISION_TRACE_BUMPS;

        if (frac < 0.0f)
        {
            hull->trace->fraction = midf;
            hull->trace->endpos = mid;
            return false;
        }

        midf = p1f + (p2f - p1f) * frac;
        mid = Vector3Add(p1, Vector3Scale(Vector3Subtract(p2, p1), frac));
    }

    hull->trace->fraction = midf;
    hull->trace->endpos = mid;

    return false;
}

bool Collision_Trace(const BSP_Map *map, Vector3 startFeet, Vector3 endFeet, bool crouched, Collision_TraceResult *trace)
{
    Collision_HullTrace hull;
    Vector3 p1;
    Vector3 p2;

    if (!trace)
        return false;

    trace->allsolid = false;
    trace->startsolid = false;
    trace->hit = false;
    trace->fraction = 1.0f;
    trace->endpos = endFeet;
    trace->normal = (Vector3){ 0.0f, 0.0f, 1.0f };

    if (!map || map->num_models == 0 || !map->clipnodes || !map->planes)
        return false;

    p1 = Collision_FeetToOrigin(startFeet, crouched);
    p2 = Collision_FeetToOrigin(endFeet, crouched);

    trace->allsolid = true;
    trace->endpos = p2;
    trace->normal = (Vector3){ 0.0f, 0.0f, 1.0f };

    hull.map = map;
    hull.depth = 0;
    hull.headnode = Collision_HullHeadnode(map, crouched);
    hull.trace = trace;

    Collision_RecursiveHullCheck(&hull, hull.headnode, 0.0f, 1.0f, p1, p2);

    if (trace->allsolid)
        trace->startsolid = true;

    if (trace->startsolid)
    {
        trace->fraction = 0.0f;
        trace->endpos = p1;
    }

    trace->hit = trace->hit || trace->fraction < 1.0f;
    trace->endpos = Collision_OriginToFeet(trace->endpos, crouched);
    trace->normal = Vector3Normalize(trace->normal);

    return true;
}
