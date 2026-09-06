#include "../include/bsp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BSP_MIP_LEVELS 4

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
    int32_t offsets[BSP_MIP_LEVELS];
    size_t mip0_size;
    size_t mip1_size;
    size_t mip2_size;
    size_t mip3_size;
    size_t pixels_size;
    size_t palette_offset;
    uint16_t palette_entries;
    size_t palette_size;
    int i;

    if (!data || !texture || size < 40)
        return false;

    memcpy(texture->name, data, BSP_MAX_TEXTURE_NAME);
    texture->name[BSP_MAX_TEXTURE_NAME - 1] = '\0';

    memcpy(&width, data + 16, sizeof(width));
    memcpy(&height, data + 20, sizeof(height));
    memcpy(offsets, data + 24, sizeof(offsets));

    if (width <= 0 || height <= 0)
        return false;

    if ((width & 3) != 0 || (height & 3) != 0)
        return false;

    for (i = 0; i < BSP_MIP_LEVELS; i++)
    {
        if (offsets[i] < 40 || (size_t)offsets[i] >= size)
            return false;
    }

    mip0_size = (size_t)width * (size_t)height;
    mip1_size = ((size_t)width >> 1) * ((size_t)height >> 1);
    mip2_size = ((size_t)width >> 2) * ((size_t)height >> 2);
    mip3_size = ((size_t)width >> 3) * ((size_t)height >> 3);

    pixels_size = mip0_size + mip1_size + mip2_size + mip3_size;

    if ((size_t)offsets[0] + pixels_size > size)
        return false;

    palette_offset = (size_t)offsets[3] + mip3_size;

    if (palette_offset + sizeof(uint16_t) > size)
        return false;

    memcpy(&palette_entries, data + palette_offset, sizeof(palette_entries));

    if (palette_entries != 256)
        return false;

    palette_size = 256 * 3;

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

    texture->data_size = (uint32_t)pixels_size;

    texture->pixels[0] = texture->data;
    texture->pixels[1] = texture->pixels[0] + mip0_size;
    texture->pixels[2] = texture->pixels[1] + mip1_size;
    texture->pixels[3] = texture->pixels[2] + mip2_size;

    texture->palette = malloc(palette_size);

    if (!texture->palette)
    {
        free(texture->data);
        texture->data = NULL;

        for (i = 0; i < BSP_MIP_LEVELS; i++)
            texture->pixels[i] = NULL;

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

        if (!BSP_LoadTextureData(
                data + offset,
                (size_t)lump->length - (size_t)offset,
                &map->textures[i]))
        {
            memset(&map->textures[i], 0, sizeof(BSP_Texture));
        }
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

        if (surface->texinfo->miptex < 0 ||
            surface->texinfo->miptex >= map->num_textures)
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

void BSP_Free(BSP_Map *map)
{
    int i;

    if (!map)
        return;

    if (map->textures)
    {
        for (i = 0; i < map->num_textures; i++)
        {
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

        if (face->firstedge + i < 0 ||
            face->firstedge + i >= map->num_surfedges)
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
    *s = vertex->x * texinfo->vecs[0][0] +
         vertex->y * texinfo->vecs[0][1] +
         vertex->z * texinfo->vecs[0][2] +
         texinfo->vecs[0][3];

    *t = vertex->x * texinfo->vecs[1][0] +
         vertex->y * texinfo->vecs[1][1] +
         vertex->z * texinfo->vecs[1][2] +
         texinfo->vecs[1][3];
}

static bool BSP_ParseVector3(const char *text, float *x, float *y, float *z)
{
    return sscanf(text, "%f %f %f", x, y, z) == 3;
}

bool BSP_GetPlayerStart(const BSP_Map *map, float *x, float *y, float *z)
{
    const char *entity;
    const char *end;
    const char *origin;

    if (!map || !map->entities || !x || !y || !z)
        return false;

    entity = map->entities;

    while (*entity)
    {
        end = strchr(entity, '}');

        if (!end)
            break;

        if (strstr(entity, "\"classname\" \"player_start\"") &&
            strstr(entity, "\"origin\"") < end)
        {
            origin = strstr(entity, "\"origin\"");

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

            return BSP_ParseVector3(origin, x, y, z);
        }

        entity = end + 1;
    }

    return false;
}
