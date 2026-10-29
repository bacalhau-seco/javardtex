#include "../include/sky.h"
#include "../include/resource.h"

#include <stdio.h>
#include <string.h>

#include <rlgl.h>

#define SKY_HALF_SIZE 1024.0f

static const char *SkyFaceSuffix[SKY_FACE_COUNT] = { "ft", "bk", "lf", "rt", "up", "dn" };

static const float SkyFaceCorners[SKY_FACE_COUNT][4][3] = {
    { {  1.0f,  1.0f,  1.0f }, {  1.0f,  1.0f, -1.0f }, {  1.0f, -1.0f, -1.0f }, {  1.0f, -1.0f,  1.0f } },
    { { -1.0f, -1.0f,  1.0f }, { -1.0f, -1.0f, -1.0f }, { -1.0f,  1.0f, -1.0f }, { -1.0f,  1.0f,  1.0f } },
    { { -1.0f,  1.0f,  1.0f }, { -1.0f,  1.0f, -1.0f }, {  1.0f,  1.0f, -1.0f }, {  1.0f,  1.0f,  1.0f } },
    { {  1.0f, -1.0f,  1.0f }, {  1.0f, -1.0f, -1.0f }, { -1.0f, -1.0f, -1.0f }, { -1.0f, -1.0f,  1.0f } },
    { { -1.0f,  1.0f,  1.0f }, {  1.0f,  1.0f,  1.0f }, {  1.0f, -1.0f,  1.0f }, { -1.0f, -1.0f,  1.0f } },
    { {  1.0f,  1.0f, -1.0f }, { -1.0f,  1.0f, -1.0f }, { -1.0f, -1.0f, -1.0f }, {  1.0f, -1.0f, -1.0f } }
};

static const float SkyFaceTexCoords[4][2] = { { 0.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f } };

static bool LoadSkyFace(const BSP_Map *map, const char *skyname, int face, Texture2D *out)
{
    char relative[RESOURCE_PATH_SIZE];
    char path[RESOURCE_PATH_SIZE];

    snprintf(relative, sizeof(relative), "gfx/env/%s%s.tga", skyname, SkyFaceSuffix[face]);

    if (!Resource_FindFile(map->path, relative, path, (int)sizeof(path)))
    {
        TraceLog(LOG_WARNING, "SKY: missing %s", relative);
        return false;
    }

    *out = LoadTexture(path);

    if (out->id == 0)
    {
        TraceLog(LOG_WARNING, "SKY: failed to load %s", path);
        return false;
    }

    SetTextureFilter(*out, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(*out, TEXTURE_WRAP_CLAMP);

    return true;
}

bool Sky_Load(Sky *sky, const BSP_Map *map)
{
    char skyname[BSP_MAP_NAME_SIZE];
    int i;

    if (!sky || !map)
        return false;

    memset(sky, 0, sizeof(*sky));

    if (!BSP_GetSkyName(map, skyname, (int)sizeof(skyname)))
        return false;

    for (i = 0; i < SKY_FACE_COUNT; i++)
    {
        if (!LoadSkyFace(map, skyname, i, &sky->faces[i]))
        {
            Sky_Unload(sky);
            return false;
        }
    }

    sky->loaded = true;

    return true;
}

void Sky_Unload(Sky *sky)
{
    int i;

    if (!sky)
        return;

    for (i = 0; i < SKY_FACE_COUNT; i++)
    {
        if (sky->faces[i].id != 0)
            UnloadTexture(sky->faces[i]);
    }

    memset(sky, 0, sizeof(*sky));
}

void Sky_Draw(const Sky *sky, Vector3 origin)
{
    int face;
    int corner;

    if (!sky || !sky->loaded)
        return;

    rlDrawRenderBatchActive();
    rlDisableBackfaceCulling();
    rlDisableDepthTest();
    rlDisableDepthMask();

    for (face = 0; face < SKY_FACE_COUNT; face++)
    {
        rlSetTexture(sky->faces[face].id);
        rlBegin(RL_QUADS);
        rlColor4ub(255, 255, 255, 255);

        for (corner = 0; corner < 4; corner++)
        {
            rlTexCoord2f(SkyFaceTexCoords[corner][0], SkyFaceTexCoords[corner][1]);
            rlVertex3f(
                origin.x + SkyFaceCorners[face][corner][0] * SKY_HALF_SIZE,
                origin.y + SkyFaceCorners[face][corner][1] * SKY_HALF_SIZE,
                origin.z + SkyFaceCorners[face][corner][2] * SKY_HALF_SIZE);
        }

        rlEnd();
        rlDrawRenderBatchActive();
    }

    rlSetTexture(0);
    rlEnableDepthMask();
    rlEnableDepthTest();
    rlEnableBackfaceCulling();
}
