#include "../include/bsp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rlgl.h"

#define BSP_MIP_LEVELS 4
#define BSP_MAX_TEXTURES 4096

static void *BSP_ReadLumpData(FILE *file, const BSP_Lump *lump)
{
    void *data;

    if (lump->length <= 0)
        return NULL;

    if (fseek(file, lump->offset, SEEK_SET) != 0)
        return NULL;

    data = malloc((size_t)lump->length);
    if (!data)
        return NULL;

    if (fread(data, 1, (size_t)lump->length, file) != (size_t)lump->length)
    {
        free(data);
        return NULL;
    }

    return data;
}

static bool BSP_LoadLump(FILE *file, const BSP_Lump *lump, void **out, int *count, size_t element_size)
{
    if (lump->length == 0)
    {
        *out = NULL;
        *count = 0;
        return true;
    }

    if (lump->length % (int32_t)element_size != 0)
        return false;

    *out = BSP_ReadLumpData(file, lump);

    if (!*out)
        return false;

    *count = lump->length / (int32_t)element_size;
    return true;
}

static bool BSP_LoadEntities(FILE *file, const BSP_Lump *lump, BSP_Map *map)
{
    if (lump->length == 0)
        return true;

    map->entities = BSP_ReadLumpData(file, lump);

    if (!map->entities)
        return false;

    map->entities_size = lump->length;

    return true;
}

static bool BSP_LoadVisibility(FILE *file, const BSP_Lump *lump, BSP_Map *map)
{
    if (lump->length == 0)
        return true;

    map->visibility = BSP_ReadLumpData(file, lump);

    if (!map->visibility)
        return false;

    map->visibility_size = lump->length;

    return true;
}

static bool BSP_LoadLighting(FILE *file, const BSP_Lump *lump, BSP_Map *map)
{
    if (lump->length == 0)
        return true;

    map->lighting = BSP_ReadLumpData(file, lump);

    if (!map->lighting)
        return false;

    map->lighting_size = lump->length;

    return true;
}

static bool BSP_LoadTextureData(const unsigned char *data, size_t size, BSP_Texture *texture)
{
    int32_t width;
    int32_t height;
    int32_t offsets[4];
    size_t mip_size;
    size_t pixels_size;
    size_t palette_offset;
    uint16_t palette_entries;
    size_t palette_size;
    int i;

    if (size < 40)
        return false;

    memcpy(texture->name, data, BSP_MAX_TEXTURE_NAME);
    texture->name[BSP_MAX_TEXTURE_NAME - 1] = '\0';

    memcpy(&width, data + 16, sizeof(width));
    memcpy(&height, data + 20, sizeof(height));
    memcpy(offsets, data + 24, sizeof(offsets));

    if (width <= 0 || height <= 0)
        return false;

    for (i = 0; i < BSP_MIP_LEVELS; i++)
    {
        if (offsets[i] < 0 || (size_t)offsets[i] >= size)
            return false;
    }

    mip_size = (size_t)width * (size_t)height;
    pixels_size = mip_size + mip_size / 4 + mip_size / 16 + mip_size / 64;

    if ((size_t)offsets[0] + pixels_size > size)
        return false;

    palette_offset = (size_t)offsets[3] + mip_size / 64;

    if (palette_offset + sizeof(uint16_t) > size)
        return false;

    memcpy(&palette_entries, data + palette_offset, sizeof(palette_entries));

    if (palette_entries == 0)
        return false;

    palette_size = (size_t)palette_entries * 3;

    if (palette_offset + sizeof(uint16_t) + palette_size > size)
        return false;

    texture->width = (uint32_t)width;
    texture->height = (uint32_t)height;

    for (i = 0; i < BSP_MIP_LEVELS; i++)
        texture->offsets[i] = (uint32_t)offsets[i];

    texture->data = malloc(pixels_size);

    if (!texture->data)
        return false;

    memcpy(texture->data, data + offsets[0], pixels_size);
    texture->data_size = pixels_size;

    for (i = 0; i < BSP_MIP_LEVELS; i++)
        texture->pixels[i] = texture->data + (offsets[i] - offsets[0]);

    texture->palette = malloc(palette_size);

    if (!texture->palette)
    {
        free(texture->data);
        texture->data = NULL;
        return false;
    }

    memcpy(texture->palette, data + palette_offset + sizeof(uint16_t), palette_size);

    return true;
}

static bool BSP_LoadTextures(FILE *file, const BSP_Lump *lump, BSP_Map *map)
{
    unsigned char *data;
    int32_t count;
    int i;

    if (lump->length == 0)
        return true;

    if (lump->length < (int32_t)sizeof(int32_t))
        return false;

    data = BSP_ReadLumpData(file, lump);

    if (!data)
        return false;

    memcpy(&count, data, sizeof(count));

    if (count < 0 || count > BSP_MAX_TEXTURES)
    {
        free(data);
        return false;
    }

    map->num_textures = count;

    if (count > 0)
    {
        map->textures = calloc((size_t)count, sizeof(BSP_Texture));

        if (!map->textures)
        {
            free(data);
            return false;
        }
    }

    for (i = 0; i < count; i++)
    {
        int32_t offset;

        if ((size_t)(4 + i * 4) + sizeof(int32_t) > (size_t)lump->length)
        {
            free(data);
            return false;
        }

        memcpy(&offset, data + 4 + i * 4, sizeof(offset));

        if (offset == -1)
            continue;

        if (offset < 0 || (size_t)offset >= (size_t)lump->length)
            continue;

        BSP_LoadTextureData(data + offset, (size_t)lump->length - (size_t)offset, &map->textures[i]);
    }

    free(data);

    return true;
}

static bool BSP_BuildSurfaces(BSP_Map *map)
{
    int i;

    if (map->num_faces == 0)
        return true;

    map->surfaces = calloc((size_t)map->num_faces, sizeof(BSP_Surface));

    if (!map->surfaces)
        return false;

    for (i = 0; i < map->num_faces; i++)
    {
        BSP_Surface *surface = &map->surfaces[i];
        BSP_Face *face = &map->faces[i];

        surface->face = *face;

        if (face->texinfo >= map->num_texinfo)
            continue;

        surface->texinfo = &map->texinfo[face->texinfo];

        if (surface->texinfo->miptex < 0 || surface->texinfo->miptex >= map->num_textures)
            continue;

        surface->texture = &map->textures[surface->texinfo->miptex];
    }

    return true;
}

bool BSP_Load(BSP_Map *map, const char *path)
{
    FILE *file;
    BSP_Header header;

    if (!map || !path)
        return false;

    memset(map, 0, sizeof(*map));

    file = fopen(path, "rb");

    if (!file)
        return false;

    if (fread(&header.version, sizeof(header.version), 1, file) != 1)
    {
        fclose(file);
        return false;
    }

    if (header.version != BSP_VERSION)
    {
        fclose(file);
        return false;
    }

    if (fread(header.lumps, sizeof(BSP_Lump), BSP_LUMP_COUNT, file) != BSP_LUMP_COUNT)
    {
        fclose(file);
        return false;
    }

    map->version = header.version;

    if (!BSP_LoadEntities(file, &header.lumps[BSP_LUMP_ENTITIES], map) ||
        !BSP_LoadLump(file, &header.lumps[BSP_LUMP_PLANES], (void **)&map->planes, &map->num_planes, sizeof(BSP_Plane)) ||
        !BSP_LoadTextures(file, &header.lumps[BSP_LUMP_TEXTURES], map) ||
        !BSP_LoadLump(file, &header.lumps[BSP_LUMP_VERTICES], (void **)&map->vertices, &map->num_vertices, sizeof(BSP_Vertex)) ||
        !BSP_LoadVisibility(file, &header.lumps[BSP_LUMP_VISIBILITY], map) ||
        !BSP_LoadLump(file, &header.lumps[BSP_LUMP_NODES], (void **)&map->nodes, &map->num_nodes, sizeof(BSP_Node)) ||
        !BSP_LoadLump(file, &header.lumps[BSP_LUMP_TEXINFO], (void **)&map->texinfo, &map->num_texinfo, sizeof(BSP_TexInfo)) ||
        !BSP_LoadLump(file, &header.lumps[BSP_LUMP_FACES], (void **)&map->faces, &map->num_faces, sizeof(BSP_Face)) ||
        !BSP_LoadLighting(file, &header.lumps[BSP_LUMP_LIGHTING], map) ||
        !BSP_LoadLump(file, &header.lumps[BSP_LUMP_CLIPNODES], (void **)&map->clipnodes, &map->num_clipnodes, sizeof(BSP_ClipNode)) ||
        !BSP_LoadLump(file, &header.lumps[BSP_LUMP_LEAVES], (void **)&map->leaves, &map->num_leaves, sizeof(BSP_Leaf)) ||
        !BSP_LoadLump(file, &header.lumps[BSP_LUMP_MARKSURFACES], (void **)&map->marksurfaces, &map->num_marksurfaces, sizeof(uint16_t)) ||
        !BSP_LoadLump(file, &header.lumps[BSP_LUMP_EDGES], (void **)&map->edges, &map->num_edges, sizeof(BSP_Edge)) ||
        !BSP_LoadLump(file, &header.lumps[BSP_LUMP_SURFEDGES], (void **)&map->surfedges, &map->num_surfedges, sizeof(int32_t)))
    {
        fclose(file);
        BSP_Free(map);
        return false;
    }

    fclose(file);

    return BSP_BuildSurfaces(map);
}

bool BSP_LoadTexturesGL(BSP_Map *map)
{
    int i;

    if (!map)
        return false;

    for (i = 0; i < map->num_textures; i++)
    {
        BSP_Texture *texture = &map->textures[i];
        Color *pixels;
        Image image;
        size_t pixel_count;
        size_t j;

        if (!texture->data || !texture->palette)
            continue;

        pixel_count = (size_t)texture->width * (size_t)texture->height;

        pixels = malloc(pixel_count * sizeof(Color));

        if (!pixels)
            return false;

        for (j = 0; j < pixel_count; j++)
        {
            uint8_t index = texture->pixels[0][j];

            pixels[j].r = texture->palette[index * 3 + 0];
            pixels[j].g = texture->palette[index * 3 + 1];
            pixels[j].b = texture->palette[index * 3 + 2];
            pixels[j].a = 255;

            if (texture->name[0] == '{' && index == 255)
                pixels[j].a = 0;
        }

        image.data = pixels;
        image.width = (int)texture->width;
        image.height = (int)texture->height;
        image.mipmaps = 1;
        image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

        texture->texture = LoadTextureFromImage(image);
        texture->loaded = IsTextureValid(texture->texture);

        UnloadImage(image);

        if (texture->loaded)
            SetTextureFilter(texture->texture, TEXTURE_FILTER_POINT);
    }

    return true;
}

void BSP_Free(BSP_Map *map)
{
    int i;

    if (!map)
        return;

    if (map->textures)
    {
        for (i = 0; i < map->num_textures; i++)
        {
            if (map->textures[i].loaded)
                UnloadTexture(map->textures[i].texture);

            free(map->textures[i].data);
            free(map->textures[i].palette);
        }
    }

    free(map->entities);
    free(map->planes);
    free(map->vertices);
    free(map->nodes);
    free(map->texinfo);
    free(map->faces);
    free(map->clipnodes);
    free(map->leaves);
    free(map->marksurfaces);
    free(map->edges);
    free(map->surfedges);
    free(map->visibility);
    free(map->lighting);
    free(map->surfaces);
    free(map->textures);

    memset(map, 0, sizeof(*map));
}

BSP_Texture *BSP_GetTexture(BSP_Map *map, int index)
{
    if (!map || index < 0 || index >= map->num_textures)
        return NULL;

    return &map->textures[index];
}

int BSP_GetFaceVertices(const BSP_Map *map, const BSP_Face *face, BSP_Vertex *out)
{
    int i;

    if (!map || !face || !out)
        return 0;

    for (i = 0; i < face->numedges; i++)
    {
        int surfedge_index;
        int edge_index;
        uint16_t vertex_index;

        if (face->firstedge + i >= map->num_surfedges)
            return 0;

        surfedge_index = map->surfedges[face->firstedge + i];

        if (surfedge_index >= 0)
        {
            edge_index = surfedge_index;

            if (edge_index >= map->num_edges)
                return 0;

            vertex_index = map->edges[edge_index].v[0];
        }
        else
        {
            edge_index = -surfedge_index;

            if (edge_index >= map->num_edges)
                return 0;

            vertex_index = map->edges[edge_index].v[1];
        }

        if (vertex_index >= map->num_vertices)
            return 0;

        out[i] = map->vertices[vertex_index];
    }

    return face->numedges;
}

void BSP_GetTexCoord(const BSP_TexInfo *texinfo, const BSP_Vertex *vertex, float *s, float *t)
{
    *s = vertex->x * texinfo->vecs[0][0] + vertex->y * texinfo->vecs[0][1] + vertex->z * texinfo->vecs[0][2] + texinfo->vecs[0][3];
    *t = vertex->x * texinfo->vecs[1][0] + vertex->y * texinfo->vecs[1][1] + vertex->z * texinfo->vecs[1][2] + texinfo->vecs[1][3];
}

static void BSP_VertexRaylib(const BSP_Vertex *in, float *x, float *y, float *z)
{
    *x = in->x;
    *y = in->z;
    *z = -in->y;
}
static void BSP_RenderFace(const BSP_Map *map, const BSP_Surface *surface)
{
    BSP_Vertex *vertices;
    int count;
    int i;

    if (surface->face.numedges < 3)
        return;

    vertices = malloc((size_t)surface->face.numedges * sizeof(BSP_Vertex));

    if (!vertices)
        return;

    count = BSP_GetFaceVertices(map, &surface->face, vertices);

    if (count < 3)
    {
        free(vertices);
        return;
    }

    if (surface->texture && surface->texture->loaded && surface->texinfo)
        rlSetTexture(surface->texture->texture.id);
    else
        rlSetTexture(0);

    rlBegin(RL_TRIANGLES);

    for (i = 1; i < count - 1; i++)
    {
        BSP_Vertex *v0 = &vertices[0];
        BSP_Vertex *v1 = &vertices[i];
        BSP_Vertex *v2 = &vertices[i + 1];

        float x0, y0, z0;
        float x1, y1, z1;
        float x2, y2, z2;

        float s0 = 0.0f;
        float t0 = 0.0f;
        float s1 = 0.0f;
        float t1 = 0.0f;
        float s2 = 0.0f;
        float t2 = 0.0f;

        BSP_VertexRaylib(v0, &x0, &y0, &z0);
        BSP_VertexRaylib(v1, &x1, &y1, &z1);
        BSP_VertexRaylib(v2, &x2, &y2, &z2);

        if (surface->texinfo && surface->texture && surface->texture->loaded)
        {
            BSP_GetTexCoord(surface->texinfo, v0, &s0, &t0);
            BSP_GetTexCoord(surface->texinfo, v1, &s1, &t1);
            BSP_GetTexCoord(surface->texinfo, v2, &s2, &t2);

            s0 /= surface->texture->width;
            t0 = -t0 / surface->texture->height;

            s1 /= surface->texture->width;
            t1 = -t1 / surface->texture->height;

            s2 /= surface->texture->width;
            t2 = -t2 / surface->texture->height;
        }

        rlColor4ub(255, 255, 255, 255);

        rlTexCoord2f(s0, t0);
        rlVertex3f(x0, y0, z0);

        rlTexCoord2f(s1, t1);
        rlVertex3f(x1, y1, z1);

        rlTexCoord2f(s2, t2);
        rlVertex3f(x2, y2, z2);

        rlTexCoord2f(s0, t0);
        rlVertex3f(x0, y0, z0);

        rlTexCoord2f(s2, t2);
        rlVertex3f(x2, y2, z2);

        rlTexCoord2f(s1, t1);
        rlVertex3f(x1, y1, z1);
    }

    rlEnd();
    rlSetTexture(0);

    free(vertices);
}

void BSP_Render(const BSP_Map *map)
{
    int i;

    if (!map || !map->surfaces)
        return;

    rlDisableBackfaceCulling();

    for (i = 0; i < map->num_faces; i++)
        BSP_RenderFace(map, &map->surfaces[i]);

    rlEnableBackfaceCulling();
}

static bool BSP_ParseVector3(const char *text, Vector3 *position)
{
    return sscanf(text, "%f %f %f", &position->x, &position->y, &position->z) == 3;
}

bool BSP_GetPlayerStart(const BSP_Map *map, Vector3 *position)
{
    const char *entity;
    const char *end;

    if (!map || !map->entities || !position)
        return false;

    entity = map->entities;

    while (*entity)
    {
        end = strchr(entity, '}');

        if (!end)
            break;

        if (strstr(entity, "\"classname\" \"player_start\"") && strstr(entity, "\"origin\"") < end)
        {
            const char *origin = strstr(entity, "\"origin\"");

            origin = strchr(origin, '"');
            if (!origin)
                return false;

            origin = strchr(origin + 1, '"');
            if (!origin)
                return false;

            origin++;

            origin = strchr(origin, '"');
            if (!origin || origin >= end)
                return false;

            origin++;

            if (!BSP_ParseVector3(origin, position))
                return false;

            return true;
        }

        entity = end + 1;
    }

    return false;
}
