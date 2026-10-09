#include "../include/render.h"

#include <stdlib.h>
#include <string.h>

#include <raylib.h>
#include <rlgl.h>

static bool IsHiddenTexture(const char *name)
{
    return strncmp(name, "sky", 3) == 0
        || strncmp(name, "clip", 4) == 0
        || strncmp(name, "trigger", 7) == 0
        || strncmp(name, "aaatrigger", 10) == 0
        || strcmp(name, "null") == 0
        || strcmp(name, "hint") == 0
        || strcmp(name, "skip") == 0
        || strcmp(name, "origin") == 0;
}

static uint32_t CreateTexture(const BSP_Texture *texture)
{
    size_t count = (size_t)texture->width * (size_t)texture->height;
    bool masked = texture->name[0] == '{';
    Image image;
    Texture2D gpu;
    Color *pixels;
    size_t i;

    image = GenImageColor((int)texture->width, (int)texture->height, BLANK);

    if (!image.data)
        return 0;

    pixels = image.data;

    for (i = 0; i < count; i++)
    {
        uint8_t index = texture->pixels[i];

        pixels[i].r = texture->palette[index][0];
        pixels[i].g = texture->palette[index][1];
        pixels[i].b = texture->palette[index][2];
        pixels[i].a = (masked && index == 255) ? 0 : 255;
    }

    gpu = LoadTextureFromImage(image);
    UnloadImage(image);

    if (gpu.id == 0)
        return 0;

    SetTextureFilter(gpu, TEXTURE_FILTER_POINT);

    return gpu.id;
}

static void AppendVertex(BSP_Batch *batch, const BSP_TexInfo *texinfo, const BSP_Texture *texture, const BSP_Vertex *vertex)
{
    BSP_RenderVertex *out;
    float s;
    float t;

    if (batch->count == batch->capacity)
    {
        int capacity = batch->capacity == 0 ? 256 : batch->capacity * 2;
        BSP_RenderVertex *grown = realloc(batch->vertices, sizeof(BSP_RenderVertex) * (size_t)capacity);

        if (!grown)
            return;

        batch->vertices = grown;
        batch->capacity = capacity;
    }

    BSP_GetTexCoord(texinfo, vertex, &s, &t);

    out = &batch->vertices[batch->count++];
    out->x = vertex->x;
    out->y = vertex->z;
    out->z = -vertex->y;
    out->s = s / (float)texture->width;
    out->t = -t / (float)texture->height;
}

static void AddFace(BSP_Renderer *renderer, const BSP_Map *map, const BSP_Face *face)
{
    const BSP_TexInfo *texinfo;
    const BSP_Texture *texture;
    BSP_Batch *batch;
    BSP_Vertex *vertices;
    int count;
    int i;

    if (face->texinfo >= map->num_texinfo)
        return;

    texinfo = &map->texinfo[face->texinfo];

    if (texinfo->miptex < 0 || texinfo->miptex >= map->num_textures)
        return;

    batch = &renderer->batches[texinfo->miptex];
    texture = &map->textures[texinfo->miptex];

    if (batch->texture_id == 0 || face->numedges < 3)
        return;

    vertices = malloc(sizeof(BSP_Vertex) * face->numedges);

    if (!vertices)
        return;

    count = BSP_GetFaceVertices(map, face, vertices);

    for (i = 1; i + 1 < count; i++)
    {
        AppendVertex(batch, texinfo, texture, &vertices[0]);
        AppendVertex(batch, texinfo, texture, &vertices[i]);
        AppendVertex(batch, texinfo, texture, &vertices[i + 1]);

        AppendVertex(batch, texinfo, texture, &vertices[i + 1]);
        AppendVertex(batch, texinfo, texture, &vertices[i]);
        AppendVertex(batch, texinfo, texture, &vertices[0]);
    }

    free(vertices);
}

bool BSP_RendererInit(BSP_Renderer *renderer, const BSP_Map *map)
{
    int i;

    if (!renderer || !map)
        return false;

    memset(renderer, 0, sizeof(*renderer));

    if (map->num_textures == 0)
        return true;

    renderer->batches = calloc((size_t)map->num_textures, sizeof(BSP_Batch));

    if (!renderer->batches)
        return false;

    renderer->num_batches = map->num_textures;

    for (i = 0; i < map->num_textures; i++)
    {
        const BSP_Texture *texture = &map->textures[i];

        if (!texture->pixels || IsHiddenTexture(texture->name))
            continue;

        renderer->batches[i].texture_id = CreateTexture(texture);
    }

    for (i = 0; i < map->num_faces; i++)
        AddFace(renderer, map, &map->faces[i]);

    return true;
}

void BSP_RendererShutdown(BSP_Renderer *renderer)
{
    int i;

    if (!renderer)
        return;

    for (i = 0; i < renderer->num_batches; i++)
    {
        if (renderer->batches[i].texture_id)
            rlUnloadTexture(renderer->batches[i].texture_id);

        free(renderer->batches[i].vertices);
    }

    free(renderer->batches);
    memset(renderer, 0, sizeof(*renderer));
}

void BSP_Render(const BSP_Renderer *renderer)
{
    int i;
    int v;

    if (!renderer)
        return;

    rlDisableBackfaceCulling();

    for (i = 0; i < renderer->num_batches; i++)
    {
        const BSP_Batch *batch = &renderer->batches[i];

        if (batch->count == 0)
            continue;

        rlSetTexture(batch->texture_id);
        rlBegin(RL_TRIANGLES);

        for (v = 0; v < batch->count; v++)
        {
            const BSP_RenderVertex *vertex = &batch->vertices[v];

            rlTexCoord2f(vertex->s, vertex->t);
            rlColor4ub(255, 255, 255, 255);
            rlVertex3f(vertex->x, vertex->y, vertex->z);
        }

        rlEnd();
    }

    rlSetTexture(0);
    rlEnableBackfaceCulling();
}
