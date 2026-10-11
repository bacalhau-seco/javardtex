#include "../include/bsp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BSP_MIP_LEVELS 4
#define BSP_MIP_HEADER_SIZE 40
#define BSP_PALETTE_COLORS 256
#define BSP_PALETTE_BYTES (BSP_PALETTE_COLORS * 3)

static int32_t ReadInt32(const uint8_t *p)
{
    int32_t value;

    memcpy(&value, p, sizeof(value));
    return value;
}

static uint16_t ReadUInt16(const uint8_t *p)
{
    uint16_t value;

    memcpy(&value, p, sizeof(value));
    return value;
}

static bool ReadLumpBytes(FILE *file, const BSP_Lump *lump, uint8_t **out)
{
    uint8_t *data;

    if (lump->offset < 0 || lump->length < 0)
        return false;

    if (fseek(file, lump->offset, SEEK_SET) != 0)
        return false;

    data = malloc((size_t)lump->length + 1);

    if (!data)
        return false;

    if (fread(data, 1, (size_t)lump->length, file) != (size_t)lump->length)
    {
        free(data);
        return false;
    }

    data[lump->length] = '\0';
    *out = data;

    return true;
}

static bool LoadArray(FILE *file, const BSP_Lump *lump, size_t element_size, void **out, int *count)
{
    uint8_t *data;

    if (lump->length < 0 || (size_t)lump->length % element_size != 0)
        return false;

    if (!ReadLumpBytes(file, lump, &data))
        return false;

    *out = data;
    *count = (int)(lump->length / (int32_t)element_size);

    return true;
}

static bool LoadTexture(const uint8_t *miptex, size_t available, BSP_Texture *texture)
{
    int32_t width;
    int32_t height;
    int32_t offsets[BSP_MIP_LEVELS];
    size_t mip0_size;
    size_t mip3_size;
    size_t palette_offset;
    uint16_t colors;
    int i;

    if (available < BSP_MIP_HEADER_SIZE)
        return false;

    width = ReadInt32(miptex + 16);
    height = ReadInt32(miptex + 20);

    for (i = 0; i < BSP_MIP_LEVELS; i++)
        offsets[i] = ReadInt32(miptex + 24 + i * 4);

    if (width <= 0 || height <= 0)
        return false;

    for (i = 0; i < BSP_MIP_LEVELS; i++)
    {
        if (offsets[i] < BSP_MIP_HEADER_SIZE || (size_t)offsets[i] >= available)
            return false;
    }

    mip0_size = (size_t)width * (size_t)height;
    mip3_size = ((size_t)width >> 3) * ((size_t)height >> 3);
    palette_offset = (size_t)offsets[3] + mip3_size;

    if (palette_offset + sizeof(uint16_t) + BSP_PALETTE_BYTES > available)
        return false;

    colors = ReadUInt16(miptex + palette_offset);

    if (colors != BSP_PALETTE_COLORS)
        return false;

    if ((size_t)offsets[0] + mip0_size > available)
        return false;

    texture->pixels = malloc(mip0_size);

    if (!texture->pixels)
        return false;

    memcpy(texture->pixels, miptex + offsets[0], mip0_size);
    memcpy(texture->palette, miptex + palette_offset + sizeof(uint16_t), BSP_PALETTE_BYTES);
    memcpy(texture->name, miptex, BSP_TEXTURE_NAME_SIZE);
    texture->name[BSP_TEXTURE_NAME_SIZE - 1] = '\0';
    texture->width = (uint32_t)width;
    texture->height = (uint32_t)height;

    return true;
}

static bool LoadTextures(FILE *file, const BSP_Lump *lump, BSP_Map *map)
{
    uint8_t *data = NULL;
    int32_t count;
    bool ok = false;
    int i;

    if (lump->length == 0)
        return true;

    if (lump->length < 4 || !ReadLumpBytes(file, lump, &data))
        return false;

    count = ReadInt32(data);

    if (count < 0 || count > BSP_MAX_TEXTURES || 4 + count * 4 > lump->length)
        goto done;

    if (count > 0)
    {
        map->textures = calloc((size_t)count, sizeof(BSP_Texture));

        if (!map->textures)
            goto done;
    }

    map->num_textures = count;

    for (i = 0; i < count; i++)
    {
        int32_t offset = ReadInt32(data + 4 + i * 4);

        if (offset < 0 || offset >= lump->length)
            continue;

        LoadTexture(data + offset, (size_t)(lump->length - offset), &map->textures[i]);
    }

    ok = true;

done:
    free(data);
    return ok;
}

static bool LoadMapLumps(FILE *file, const BSP_Lump *lumps, BSP_Map *map)
{
    return LoadArray(file, &lumps[BSP_LUMP_ENTITIES], 1, (void **)&map->entities, &map->entities_size)
        && LoadArray(file, &lumps[BSP_LUMP_PLANES], sizeof(BSP_Plane), (void **)&map->planes, &map->num_planes)
        && LoadTextures(file, &lumps[BSP_LUMP_TEXTURES], map)
        && LoadArray(file, &lumps[BSP_LUMP_VERTICES], sizeof(BSP_Vertex), (void **)&map->vertices, &map->num_vertices)
        && LoadArray(file, &lumps[BSP_LUMP_NODES], sizeof(BSP_Node), (void **)&map->nodes, &map->num_nodes)
        && LoadArray(file, &lumps[BSP_LUMP_TEXINFO], sizeof(BSP_TexInfo), (void **)&map->texinfo, &map->num_texinfo)
        && LoadArray(file, &lumps[BSP_LUMP_FACES], sizeof(BSP_Face), (void **)&map->faces, &map->num_faces)
        && LoadArray(file, &lumps[BSP_LUMP_CLIPNODES], sizeof(BSP_ClipNode), (void **)&map->clipnodes, &map->num_clipnodes)
        && LoadArray(file, &lumps[BSP_LUMP_LEAVES], sizeof(BSP_Leaf), (void **)&map->leaves, &map->num_leaves)
        && LoadArray(file, &lumps[BSP_LUMP_MARKSURFACES], sizeof(uint16_t), (void **)&map->marksurfaces, &map->num_marksurfaces)
        && LoadArray(file, &lumps[BSP_LUMP_EDGES], sizeof(BSP_Edge), (void **)&map->edges, &map->num_edges)
        && LoadArray(file, &lumps[BSP_LUMP_SURFEDGES], sizeof(int32_t), (void **)&map->surfedges, &map->num_surfedges)
        && LoadArray(file, &lumps[BSP_LUMP_MODELS], sizeof(BSP_Model), (void **)&map->models, &map->num_models);
}

bool BSP_Load(BSP_Map *map, const char *path)
{
    FILE *file;
    BSP_Lump lumps[BSP_LUMP_COUNT];
    int32_t version;
    bool ok;

    if (!map || !path)
        return false;

    memset(map, 0, sizeof(*map));

    file = fopen(path, "rb");

    if (!file)
        return false;

    ok = fread(&version, sizeof(version), 1, file) == 1
        && version == BSP_VERSION
        && fread(lumps, sizeof(BSP_Lump), BSP_LUMP_COUNT, file) == (size_t)BSP_LUMP_COUNT
        && LoadMapLumps(file, lumps, map);

    fclose(file);

    if (!ok)
    {
        BSP_Free(map);
        return false;
    }

    map->version = version;

    return true;
}

void BSP_Free(BSP_Map *map)
{
    int i;

    if (!map)
        return;

    for (i = 0; map->textures && i < map->num_textures; i++)
        free(map->textures[i].pixels);

    free(map->textures);
    free(map->entities);
    free(map->planes);
    free(map->vertices);
    free(map->edges);
    free(map->surfedges);
    free(map->texinfo);
    free(map->faces);
    free(map->nodes);
    free(map->clipnodes);
    free(map->leaves);
    free(map->marksurfaces);
    free(map->models);

    memset(map, 0, sizeof(*map));
}

BSP_Texture *BSP_GetTexture(BSP_Map *map, int index)
{
    if (!map || index < 0 || index >= map->num_textures)
        return NULL;

    return &map->textures[index];
}

static bool GetSurfedgeVertex(const BSP_Map *map, int32_t surfedge, BSP_Vertex *out)
{
    int32_t edge_index = surfedge < 0 ? -surfedge : surfedge;
    uint16_t vertex_index;

    if (edge_index >= map->num_edges)
        return false;

    vertex_index = map->edges[edge_index].v[surfedge < 0 ? 1 : 0];

    if (vertex_index >= map->num_vertices)
        return false;

    *out = map->vertices[vertex_index];
    return true;
}

int BSP_GetFaceVertices(const BSP_Map *map, const BSP_Face *face, BSP_Vertex *out)
{
    int i;

    if (!map || !face || !out)
        return 0;

    if (face->firstedge < 0 || face->firstedge + face->numedges > map->num_surfedges)
        return 0;

    for (i = 0; i < face->numedges; i++)
    {
        if (!GetSurfedgeVertex(map, map->surfedges[face->firstedge + i], &out[i]))
            return 0;
    }

    return face->numedges;
}

void BSP_GetTexCoord(const BSP_TexInfo *texinfo, const BSP_Vertex *vertex, float *s, float *t)
{
    *s = vertex->x * texinfo->vecs[0][0]
        + vertex->y * texinfo->vecs[0][1]
        + vertex->z * texinfo->vecs[0][2]
        + texinfo->vecs[0][3];

    *t = vertex->x * texinfo->vecs[1][0]
        + vertex->y * texinfo->vecs[1][1]
        + vertex->z * texinfo->vecs[1][2]
        + texinfo->vecs[1][3];
}

static bool ReadQuoted(const char **cursor, char *out, size_t size)
{
    const char *start;
    const char *end;
    size_t length;

    start = strchr(*cursor, '"');

    if (!start)
        return false;

    start++;
    end = strchr(start, '"');

    if (!end)
        return false;

    length = (size_t)(end - start);

    if (length >= size)
        length = size - 1;

    memcpy(out, start, length);
    out[length] = '\0';
    *cursor = end + 1;

    return true;
}

bool BSP_GetPlayerStart(const BSP_Map *map, float *x, float *y, float *z)
{
    const char *cursor;

    if (!map || !map->entities || !x || !y || !z)
        return false;

    cursor = map->entities;

    while ((cursor = strchr(cursor, '{')) != NULL)
    {
        char key[64];
        char value[64];
        bool is_start = false;
        bool has_origin = false;
        float ox = 0.0f;
        float oy = 0.0f;
        float oz = 0.0f;

        cursor++;

        while (*cursor && *cursor != '}')
        {
            if (!ReadQuoted(&cursor, key, sizeof(key)) ||
                !ReadQuoted(&cursor, value, sizeof(value)))
                break;

            if (strcmp(key, "classname") == 0 && strcmp(value, "info_player_start") == 0)
                is_start = true;

            if (strcmp(key, "origin") == 0 && sscanf(value, "%f %f %f", &ox, &oy, &oz) == 3)
                has_origin = true;
        }

        if (is_start && has_origin)
        {
            *x = ox;
            *y = oy;
            *z = oz;
            return true;
        }
    }

    return false;
}
