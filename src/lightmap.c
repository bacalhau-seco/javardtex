#include "../include/lightmap.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <raylib.h>
#include <rlgl.h>

#define LIGHTMAP_CHANNELS 3
#define LIGHTMAP_PADDING 1

typedef struct {
    int min_s;
    int min_t;
    int width;
    int height;
} LightmapExtents;

typedef struct {
    int x;
    int y;
    int shelf;
} Packer;

static bool ComputeExtents(const BSP_Map *map, const BSP_Face *face, LightmapExtents *extents)
{
    const BSP_TexInfo *texinfo;
    Vector3 *vertices;
    float min_s;
    float max_s;
    float min_t;
    float max_t;
    int count;
    int i;

    if (face->texinfo >= map->num_texinfo || face->numedges < 3)
        return false;

    texinfo = &map->texinfo[face->texinfo];
    vertices = malloc(sizeof(Vector3) * face->numedges);

    if (!vertices)
        return false;

    count = BSP_GetFaceVertices(map, face, vertices);

    if (count == 0)
    {
        free(vertices);
        return false;
    }

    BSP_GetTexCoord(texinfo, &vertices[0], &min_s, &min_t);
    max_s = min_s;
    max_t = min_t;

    for (i = 1; i < count; i++)
    {
        float s;
        float t;

        BSP_GetTexCoord(texinfo, &vertices[i], &s, &t);

        min_s = fminf(min_s, s);
        max_s = fmaxf(max_s, s);
        min_t = fminf(min_t, t);
        max_t = fmaxf(max_t, t);
    }

    free(vertices);

    extents->min_s = (int)floorf(min_s / LIGHTMAP_LUXEL_SIZE);
    extents->min_t = (int)floorf(min_t / LIGHTMAP_LUXEL_SIZE);
    extents->width = (int)ceilf(max_s / LIGHTMAP_LUXEL_SIZE) - extents->min_s + 1;
    extents->height = (int)ceilf(max_t / LIGHTMAP_LUXEL_SIZE) - extents->min_t + 1;

    return true;
}

static bool PackRect(Packer *packer, int width, int height, int *out_x, int *out_y)
{
    if (width > LIGHTMAP_SIZE || height > LIGHTMAP_SIZE)
        return false;

    if (packer->x + width + LIGHTMAP_PADDING > LIGHTMAP_SIZE)
    {
        packer->x = 0;
        packer->y += packer->shelf + LIGHTMAP_PADDING;
        packer->shelf = 0;
    }

    if (packer->y + height + LIGHTMAP_PADDING > LIGHTMAP_SIZE)
        return false;

    *out_x = packer->x;
    *out_y = packer->y;

    packer->x += width + LIGHTMAP_PADDING;

    if (height > packer->shelf)
        packer->shelf = height;

    return true;
}

static bool CopySamples(const BSP_Map *map, const BSP_Face *face, const BSP_FaceLightmap *placed, Color *pixels)
{
    size_t base = (size_t)face->lightofs;
    size_t samples = (size_t)placed->width * (size_t)placed->height;
    int x;
    int y;

    if (base + samples * LIGHTMAP_CHANNELS > (size_t)map->lighting_size)
        return false;

    for (y = 0; y < placed->height; y++)
    {
        for (x = 0; x < placed->width; x++)
        {
            const uint8_t *src = map->lighting + base + ((size_t)y * placed->width + x) * LIGHTMAP_CHANNELS;
            Color *dst = &pixels[(placed->y + y) * LIGHTMAP_SIZE + placed->x + x];

            dst->r = src[0];
            dst->g = src[1];
            dst->b = src[2];
            dst->a = 255;
        }
    }

    return true;
}

bool BSP_LightmapInit(BSP_Lightmap *lightmap, const BSP_Map *map)
{
    Image atlas;
    Texture2D texture;
    Packer packer = { 0, 1, 0 };
    int i;

    if (!lightmap || !map)
        return false;

    memset(lightmap, 0, sizeof(*lightmap));

    lightmap->faces = calloc(map->num_faces > 0 ? (size_t)map->num_faces : 1, sizeof(BSP_FaceLightmap));

    if (!lightmap->faces)
        return false;

    atlas = GenImageColor(LIGHTMAP_SIZE, LIGHTMAP_SIZE, BLACK);

    for (i = 0; i < map->num_faces; i++)
    {
        const BSP_Face *face = &map->faces[i];
        BSP_FaceLightmap *placed = &lightmap->faces[i];
        LightmapExtents extents;

        if (face->styles[0] == 255 || face->lightofs < 0 || !ComputeExtents(map, face, &extents))
            continue;

        if (!PackRect(&packer, extents.width, extents.height, &placed->x, &placed->y))
        {
            TraceLog(LOG_WARNING, "Lightmap atlas full, face %d rendered unlit", i);
            continue;
        }

        placed->min_s = extents.min_s;
        placed->min_t = extents.min_t;
        placed->width = extents.width;
        placed->height = extents.height;

        placed->valid = CopySamples(map, face, placed, atlas.data);
    }

    texture = LoadTextureFromImage(atlas);
    UnloadImage(atlas);

    if (texture.id == 0)
    {
        free(lightmap->faces);
        memset(lightmap, 0, sizeof(*lightmap));
        return false;
    }

    SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
    lightmap->texture_id = texture.id;

    return true;
}

void BSP_LightmapShutdown(BSP_Lightmap *lightmap)
{
    if (!lightmap)
        return;

    if (lightmap->texture_id)
        rlUnloadTexture(lightmap->texture_id);

    free(lightmap->faces);
    memset(lightmap, 0, sizeof(*lightmap));
}

void BSP_LightmapUV(const BSP_FaceLightmap *face, float s, float t, float *u, float *v)
{
    if (!face->valid)
    {
        *u = 0.5f / LIGHTMAP_SIZE;
        *v = 0.5f / LIGHTMAP_SIZE;
        return;
    }

    float lx = s / LIGHTMAP_LUXEL_SIZE - (float)face->min_s;
    float ly = t / LIGHTMAP_LUXEL_SIZE - (float)face->min_t;

    if (lx < 0.0f)
        lx = 0.0f;

    if (ly < 0.0f)
        ly = 0.0f;

    if (lx > (float)face->width - 1.0f)
        lx = (float)face->width - 1.0f;

    if (ly > (float)face->height - 1.0f)
        ly = (float)face->height - 1.0f;

    *u = ((float)face->x + lx + 0.5f) / LIGHTMAP_SIZE;
    *v = ((float)face->y + ly + 0.5f) / LIGHTMAP_SIZE;
}
