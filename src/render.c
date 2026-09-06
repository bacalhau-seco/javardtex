#include "../include/render.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <raylib.h>
#include <rlgl.h>

static void BSP_RendererVertex(const BSP_Vertex *vertex, float s, float t)
{
    rlTexCoord2f(s, t);
    rlColor4ub(255, 255, 255, 255);
    rlVertex3f(vertex->x, vertex->z, -vertex->y);
}

static void BSP_RenderTriangle(const BSP_Surface *surface, const BSP_Vertex *v0, const BSP_Vertex *v1, const BSP_Vertex *v2)
{
    float s0;
    float t0;
    float s1;
    float t1;
    float s2;
    float t2;

    BSP_GetTexCoord(surface->texinfo, v0, &s0, &t0);
    BSP_GetTexCoord(surface->texinfo, v1, &s1, &t1);
    BSP_GetTexCoord(surface->texinfo, v2, &s2, &t2);

    s0 /= (float)surface->texture->width;
    t0 /= (float)surface->texture->height;

    s1 /= (float)surface->texture->width;
    t1 /= (float)surface->texture->height;

    s2 /= (float)surface->texture->width;
    t2 /= (float)surface->texture->height;

    BSP_RendererVertex(v0, s0, -t0);
    BSP_RendererVertex(v1, s1, -t1);
    BSP_RendererVertex(v2, s2, -t2);
}

static void BSP_RenderFace(const BSP_Map *map, const BSP_Renderer *renderer, const BSP_Surface *surface)
{
    BSP_Vertex *vertices;
    int num_vertices;
    int texture_index;
    uint32_t texture;

    if (!surface->texinfo || !surface->texture)
        return;

    if (surface->texture->width == 0 ||
        surface->texture->height == 0)
        return;

    if (!surface->texture->pixels[0] ||
        !surface->texture->palette)
        return;

    texture_index = surface->texinfo->miptex;

    if (texture_index < 0 ||
        texture_index >= renderer->num_textures)
        return;

    texture = renderer->textures[texture_index];

    if (!texture)
        return;

    if (surface->face.numedges < 3)
        return;

    vertices = malloc(sizeof(BSP_Vertex) * (size_t)surface->face.numedges);

    if (!vertices)
        return;

    num_vertices = BSP_GetFaceVertices(map, &surface->face, vertices);

    if (num_vertices < 3)
    {
        free(vertices);
        return;
    }

    rlSetTexture(texture);

    rlBegin(RL_TRIANGLES);

    for (int i = 1; i < num_vertices - 1; i++)
    {
        BSP_RenderTriangle(
            surface,
            &vertices[0],
            &vertices[i],
            &vertices[i + 1]
        );

        BSP_RenderTriangle(
            surface,
            &vertices[i + 1],
            &vertices[i],
            &vertices[0]
        );
    }

    rlEnd();

    rlSetTexture(0);

    free(vertices);
}

bool BSP_RendererInit(BSP_Renderer *renderer, const BSP_Map *map)
{
    if (!renderer || !map)
        return false;

    memset(renderer, 0, sizeof(*renderer));

    if (map->num_textures <= 0)
        return true;

    renderer->textures = calloc(
        (size_t)map->num_textures,
        sizeof(uint32_t)
    );

    if (!renderer->textures)
        return false;

    renderer->num_textures = map->num_textures;

    for (int i = 0; i < map->num_textures; i++)
    {
        const BSP_Texture *texture = &map->textures[i];
        size_t pixel_count;
        Color *pixels;
        Image image;
        Texture2D gpu_texture;

        if (!texture->pixels[0] ||
            !texture->palette ||
            texture->width == 0 ||
            texture->height == 0)
            continue;

        pixel_count =
            (size_t)texture->width *
            (size_t)texture->height;

        pixels = malloc(sizeof(Color) * pixel_count);

        if (!pixels)
            continue;

        for (size_t p = 0; p < pixel_count; p++)
        {
            uint8_t index = texture->pixels[0][p];

            pixels[p].r = texture->palette[index * 3 + 0];
            pixels[p].g = texture->palette[index * 3 + 1];
            pixels[p].b = texture->palette[index * 3 + 2];
            pixels[p].a = 255;

            if (index == 255 && texture->name[0] == '{')
                pixels[p].a = 0;
        }

        image = GenImageColor(
            (int)texture->width,
            (int)texture->height,
            BLANK
        );

        if (!image.data)
        {
            free(pixels);
            continue;
        }

        memcpy(
            image.data,
            pixels,
            pixel_count * sizeof(Color)
        );

        gpu_texture = LoadTextureFromImage(image);

        UnloadImage(image);
        free(pixels);

        if (gpu_texture.id == 0)
            continue;

        renderer->textures[i] = gpu_texture.id;

        SetTextureFilter(
            gpu_texture,
            TEXTURE_FILTER_POINT
        );
    }

    return true;
}

void BSP_RendererShutdown(BSP_Renderer *renderer)
{
    if (!renderer)
        return;

    if (renderer->textures)
    {
        for (int i = 0; i < renderer->num_textures; i++)
        {
            if (renderer->textures[i])
                rlUnloadTexture(renderer->textures[i]);
        }

        free(renderer->textures);
    }

    memset(renderer, 0, sizeof(*renderer));
}

void BSP_Render(const BSP_Map *map, const BSP_Renderer *renderer)
{
    if (!map || !renderer || !map->surfaces)
        return;

    rlDisableBackfaceCulling();

    for (int i = 0; i < map->num_faces; i++)
        BSP_RenderFace(
            map,
            renderer,
            &map->surfaces[i]
        );

    rlEnableBackfaceCulling();
}
