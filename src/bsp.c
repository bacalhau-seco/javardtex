#include "../include/bsp.h"

#include "raymath.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BSP_MIP_LEVELS 4
#define BSP_MIP_HEADER_SIZE 40
#define BSP_PALETTE_COLORS 256
#define BSP_PALETTE_BYTES (BSP_PALETTE_COLORS * 3)

static inline int32_t ReadInt32(const uint8_t *p)
{
    int32_t value;

    memcpy(&value, p, sizeof(value));
    return value;
}

static inline uint16_t ReadUInt16(const uint8_t *p)
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

static bool LoadMipTexture(const uint8_t *miptex, size_t available, BSP_Texture *texture)
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

        LoadMipTexture(data + offset, (size_t)(lump->length - offset), &map->textures[i]);
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
        && LoadArray(file, &lumps[BSP_LUMP_VERTICES], sizeof(Vector3), (void **)&map->vertices, &map->num_vertices)
        && LoadArray(file, &lumps[BSP_LUMP_NODES], sizeof(BSP_Node), (void **)&map->nodes, &map->num_nodes)
        && LoadArray(file, &lumps[BSP_LUMP_TEXINFO], sizeof(BSP_TexInfo), (void **)&map->texinfo, &map->num_texinfo)
        && LoadArray(file, &lumps[BSP_LUMP_FACES], sizeof(BSP_Face), (void **)&map->faces, &map->num_faces)
        && LoadArray(file, &lumps[BSP_LUMP_CLIPNODES], sizeof(BSP_ClipNode), (void **)&map->clipnodes, &map->num_clipnodes)
        && LoadArray(file, &lumps[BSP_LUMP_LEAVES], sizeof(BSP_Leaf), (void **)&map->leaves, &map->num_leaves)
        && LoadArray(file, &lumps[BSP_LUMP_MARKSURFACES], sizeof(uint16_t), (void **)&map->marksurfaces, &map->num_marksurfaces)
        && LoadArray(file, &lumps[BSP_LUMP_EDGES], sizeof(BSP_Edge), (void **)&map->edges, &map->num_edges)
        && LoadArray(file, &lumps[BSP_LUMP_SURFEDGES], sizeof(int32_t), (void **)&map->surfedges, &map->num_surfedges)
        && LoadArray(file, &lumps[BSP_LUMP_MODELS], sizeof(BSP_Model), (void **)&map->models, &map->num_models)
        && LoadArray(file, &lumps[BSP_LUMP_LIGHTING], 1, (void **)&map->lighting, &map->lighting_size);
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
    free(map->lighting);
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

static bool GetSurfedgeVertex(const BSP_Map *map, int32_t surfedge, Vector3 *out)
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

int BSP_GetFaceVertices(const BSP_Map *map, const BSP_Face *face, Vector3 *out)
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

void BSP_GetTexCoord(const BSP_TexInfo *texinfo, const Vector3 *vertex, float *s, float *t)
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

static bool ReadQuoted(const char **cursor, const char *end, char *out, size_t size)
{
    const char *start = *cursor;
    const char *close;
    size_t length;

    while (start < end && *start != '"')
        start++;

    if (start >= end)
        return false;

    start++;
    close = start;

    while (close < end && *close != '"')
        close++;

    if (close >= end)
        return false;

    length = (size_t)(close - start);

    if (length >= size)
        length = size - 1;

    memcpy(out, start, length);
    out[length] = '\0';
    *cursor = close + 1;

    return true;
}

static bool ParsePlayerStartBlock(const char *start, const char *end, float *x, float *y, float *z, float *angle)
{
    const char *cursor = start;
    char key[64];
    char value[64];
    bool is_start = false;
    bool has_origin = false;
    float ox = 0.0f;
    float oy = 0.0f;
    float oz = 0.0f;
    float yaw = 0.0f;
    float pitch = 0.0f;
    float roll = 0.0f;

    while (ReadQuoted(&cursor, end, key, sizeof(key)) &&
           ReadQuoted(&cursor, end, value, sizeof(value)))
    {
        if (strcmp(key, "classname") == 0 && strcmp(value, "info_player_start") == 0)
            is_start = true;

        if (strcmp(key, "origin") == 0 && sscanf(value, "%f %f %f", &ox, &oy, &oz) == 3)
            has_origin = true;

        if (strcmp(key, "angle") == 0)
            sscanf(value, "%f", &yaw);

        if (strcmp(key, "angles") == 0)
            sscanf(value, "%f %f %f", &pitch, &yaw, &roll);
    }

    if (!is_start || !has_origin)
        return false;

    *x = ox;
    *y = oy;
    *z = oz;
    *angle = yaw;

    return true;
}

static bool NextEntityBlock(const char **cursor, const char **blockEnd)
{
    const char *open;
    const char *close;

    if (!cursor || !*cursor || !blockEnd)
        return false;

    open = strchr(*cursor, '{');

    if (!open)
        return false;

    close = strchr(open, '}');

    if (!close)
        return false;

    *cursor = open + 1;
    *blockEnd = close;

    return true;
}

static bool ParseNextLevelBlock(const BSP_Map *map, const char *start, const char *end, BSP_NextLevel *out)
{
    const char *cursor = start;
    char key[64];
    char value[64];
    bool is_next_level = false;
    bool has_map = false;
    int model_index = -1;
    Vector3 offset = { 0.0f, 0.0f, 0.0f };
    BSP_NextLevel found = { 0 };

    while (ReadQuoted(&cursor, end, key, sizeof(key)) &&
           ReadQuoted(&cursor, end, value, sizeof(value)))
    {
        if (strcmp(key, "classname") == 0 && strcmp(value, "func_next_level") == 0)
            is_next_level = true;

        if (strcmp(key, "model") == 0 && value[0] == '*')
            sscanf(value + 1, "%d", &model_index);

        if (strcmp(key, "origin") == 0)
            sscanf(value, "%f %f %f", &offset.x, &offset.y, &offset.z);

        if (strcmp(key, "map") == 0 && value[0] != '\0')
        {
            snprintf(found.map, sizeof(found.map), "%s", value);
            has_map = true;
        }
    }

    if (!is_next_level || !has_map)
        return false;

    if (!map->models || model_index < 0 || model_index >= map->num_models)
        return false;

    found.mins = Vector3Add(map->models[model_index].mins, offset);
    found.maxs = Vector3Add(map->models[model_index].maxs, offset);

    *out = found;

    return true;
}

bool BSP_GetNextLevel(const BSP_Map *map, BSP_NextLevel *out)
{
    const char *cursor;
    const char *end;

    if (!map || !map->entities || !out)
        return false;

    cursor = map->entities;

    while (NextEntityBlock(&cursor, &end))
    {
        if (ParseNextLevelBlock(map, cursor, end, out))
            return true;

        cursor = end + 1;
    }

    return false;
}

static bool FindPlayerStart(const BSP_Map *map, float *x, float *y, float *z, float *angle)
{
    const char *cursor;
    const char *end;

    if (!map || !map->entities || !x || !y || !z || !angle)
        return false;

    cursor = map->entities;

    while (NextEntityBlock(&cursor, &end))
    {
        if (ParsePlayerStartBlock(cursor, end, x, y, z, angle))
            return true;

        cursor = end + 1;
    }

    return false;
}

static int ClampColorComponent(int value)
{
    if (value < 0)
        return 0;

    if (value > 255)
        return 255;

    return value;
}

static bool ParseSunlightColorBlock(const char *start, const char *end, unsigned char *r, unsigned char *g, unsigned char *b)
{
    const char *cursor = start;
    char key[64];
    char value[64];
    bool is_worldspawn = false;
    bool has_color = false;
    int cr = 255;
    int cg = 255;
    int cb = 255;

    while (ReadQuoted(&cursor, end, key, sizeof(key)) &&
           ReadQuoted(&cursor, end, value, sizeof(value)))
    {
        if (strcmp(key, "classname") == 0 && strcmp(value, "worldspawn") == 0)
            is_worldspawn = true;

        if (strcmp(key, "_sunlight_color") == 0 && sscanf(value, "%d %d %d", &cr, &cg, &cb) == 3)
            has_color = true;
    }

    if (!is_worldspawn || !has_color)
        return false;

    *r = (unsigned char)ClampColorComponent(cr);
    *g = (unsigned char)ClampColorComponent(cg);
    *b = (unsigned char)ClampColorComponent(cb);

    return true;
}

bool BSP_GetSunlightColor(const BSP_Map *map, unsigned char *r, unsigned char *g, unsigned char *b)
{
    const char *cursor;
    const char *end;

    if (!map || !map->entities || !r || !g || !b)
        return false;

    cursor = map->entities;

    while (NextEntityBlock(&cursor, &end))
    {
        if (ParseSunlightColorBlock(cursor, end, r, g, b))
            return true;

        cursor = end + 1;
    }

    return false;
}

static float ClampFloat(float value, float min, float max)
{
    if (value < min)
        return min;

    if (value > max)
        return max;

    return value;
}

static inline void ScaleSkyColor(const int color[3], float intensity, float *out)
{
    float scale = ClampFloat(intensity / 200.0f, 0.35f, 1.0f);
    int i;

    for (i = 0; i < 3; i++)
        out[i] = (float)ClampColorComponent(color[i]) / 255.0f * scale;
}

static bool ParseSkyGradientBlock(const char *start, const char *end, BSP_SkyGradient *out)
{
    const char *cursor = start;
    char key[64];
    char value[64];
    bool is_worldspawn = false;
    int sun_color[3] = { 255, 255, 255 };
    int sun2_color[3] = { 255, 255, 255 };
    float sun = 0.0f;
    float sun2 = 0.0f;

    while (ReadQuoted(&cursor, end, key, sizeof(key)) &&
           ReadQuoted(&cursor, end, value, sizeof(value)))
    {
        if (strcmp(key, "classname") == 0 && strcmp(value, "worldspawn") == 0)
            is_worldspawn = true;

        if (strcmp(key, "_sunlight") == 0)
            sscanf(value, "%f", &sun);

        if (strcmp(key, "_sunlight2") == 0)
            sscanf(value, "%f", &sun2);

        if (strcmp(key, "_sunlight_color") == 0)
            sscanf(value, "%d %d %d", &sun_color[0], &sun_color[1], &sun_color[2]);

        if (strcmp(key, "_sunlight2_color") == 0)
            sscanf(value, "%d %d %d", &sun2_color[0], &sun2_color[1], &sun2_color[2]);
    }

    if (!is_worldspawn)
        return false;

    ScaleSkyColor(sun_color, 200.0f, out->top);
    ScaleSkyColor(sun2_color, sun2, out->bottom);

    if (sun + sun2 > 0.0f)
        out->fade = ClampFloat(sun / (sun + sun2), 0.15f, 0.85f);
    else
        out->fade = 0.5f;

    return true;
}

bool BSP_GetSkyGradient(const BSP_Map *map, BSP_SkyGradient *out)
{
    const char *cursor;
    const char *end;

    if (!map || !map->entities || !out)
        return false;

    cursor = map->entities;

    while (NextEntityBlock(&cursor, &end))
    {
        if (ParseSkyGradientBlock(cursor, end, out))
            return true;

        cursor = end + 1;
    }

    return false;
}

bool BSP_GetPlayerStart(const BSP_Map *map, float *x, float *y, float *z)
{
    float angle = 0.0f;

    if (!x || !y || !z)
        return false;

    return FindPlayerStart(map, x, y, z, &angle);
}

bool BSP_GetPlayerStartAngle(const BSP_Map *map, float *angle)
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    if (!angle)
        return false;

    return FindPlayerStart(map, &x, &y, &z, angle);
}
