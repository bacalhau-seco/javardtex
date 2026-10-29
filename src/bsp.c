#include "../include/bsp.h"
#include "../include/resource.h"
#include "../include/wad.h"

#include "raymath.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BSP_ENTITY_VALUE_SIZE 1024
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

    memcpy(texture->name, miptex, BSP_TEXTURE_NAME_SIZE);
    texture->name[BSP_TEXTURE_NAME_SIZE - 1] = '\0';
    texture->width = (uint32_t)width;
    texture->height = (uint32_t)height;

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
        && LoadArray(file, &lumps[BSP_LUMP_LIGHTING], 1, (void **)&map->lighting, &map->lighting_size)
        && LoadArray(file, &lumps[BSP_LUMP_VISIBILITY], 1, (void **)&map->visdata, &map->visdata_size);
}

static void LoadWadTextures(BSP_Map *map);

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
    snprintf(map->path, sizeof(map->path), "%s", path);

    LoadWadTextures(map);

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
    free(map->visdata);
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

static inline float NodePlaneDistance(const BSP_Plane *plane, Vector3 p)
{
    return Vector3DotProduct(plane->normal, p) - plane->dist;
}

int BSP_FindLeaf(const BSP_Map *map, Vector3 position)
{
    int num;

    if (!map || map->num_models == 0 || !map->nodes || !map->planes)
        return 0;

    num = map->models[0].headnode[0];

    while (num >= 0)
    {
        const BSP_Node *node;
        const BSP_Plane *plane;

        if (num >= map->num_nodes)
            return 0;

        node = &map->nodes[num];

        if (node->planenum < 0 || node->planenum >= map->num_planes)
            return 0;

        plane = &map->planes[node->planenum];
        num = node->children[NodePlaneDistance(plane, position) < 0.0f ? 1 : 0];
    }

    return ~num;
}

bool BSP_DecompressVis(const BSP_Map *map, int leaf, uint8_t *out, int out_size)
{
    int32_t visofs;
    int offset;
    int written = 0;

    if (!map || !out || out_size <= 0)
        return false;

    if (!map->visdata || map->visdata_size <= 0 || !map->leaves)
        return false;

    if (leaf <= 0 || leaf >= map->num_leaves)
        return false;

    visofs = map->leaves[leaf].visofs;

    if (visofs < 0 || visofs >= map->visdata_size)
        return false;

    offset = visofs;

    while (written < out_size)
    {
        uint8_t bits;

        if (offset >= map->visdata_size)
            return false;

        bits = map->visdata[offset++];

        if (bits != 0)
        {
            out[written++] = bits;
            continue;
        }

        if (offset >= map->visdata_size)
            return false;

        {
            int run = map->visdata[offset++];

            while (run > 0 && written < out_size)
            {
                out[written++] = 0;
                run--;
            }
        }
    }

    return true;
}


static bool GetSurfedgeVertex(const BSP_Map *map, int32_t surfedge, Vector3 *out)
{
    int64_t edge_index = surfedge < 0 ? -(int64_t)surfedge : (int64_t)surfedge;
    uint16_t vertex_index;

    if (edge_index >= (int64_t)map->num_edges)
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

static bool NextEntity(const char **cursor, const char **blockEnd)
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

static bool ValueForKey(const char *start, const char *end, const char *key, char *out, int size)
{
    const char *cursor = start;
    char foundKey[64];
    char value[BSP_ENTITY_VALUE_SIZE];

    while (ReadQuoted(&cursor, end, foundKey, sizeof(foundKey)) &&
           ReadQuoted(&cursor, end, value, sizeof(value)))
    {
        if (strcmp(foundKey, key) != 0)
            continue;

        snprintf(out, (size_t)size, "%s", value);

        return true;
    }

    return false;
}

static bool FindEntityByClass(const BSP_Map *map, const char *classname, int index, const char **start, const char **end)
{
    const char *cursor;
    const char *blockEnd;
    char value[64];
    int matches = 0;

    if (!map || !map->entities)
        return false;

    cursor = map->entities;

    while (NextEntity(&cursor, &blockEnd))
    {
        if (ValueForKey(cursor, blockEnd, "classname", value, sizeof(value)) && strcmp(value, classname) == 0)
        {
            if (matches == index)
            {
                *start = cursor;
                *end = blockEnd;

                return true;
            }

            matches++;
        }

        cursor = blockEnd + 1;
    }

    return false;
}

static inline int CountMissingTextures(const BSP_Map *map)
{
    int missing = 0;
    int i;

    for (i = 0; i < map->num_textures; i++)
    {
        if (!map->textures[i].pixels && map->textures[i].name[0] != '\0')
            missing++;
    }

    return missing;
}

static inline const char *WadFileName(const char *path)
{
    const char *name = path;
    const char *cursor;

    for (cursor = path; *cursor != '\0'; cursor++)
    {
        if (*cursor == '/' || *cursor == '\\')
            name = cursor + 1;
    }

    return name;
}

static int FillTexturesFromWad(BSP_Map *map, const char *wadPath)
{
    Wad wad;
    int filled = 0;
    int i;

    if (!Wad_Open(&wad, wadPath))
    {
        TraceLog(LOG_WARNING, "WAD: failed to open %s", wadPath);
        return 0;
    }

    for (i = 0; i < map->num_textures; i++)
    {
        BSP_Texture *texture = &map->textures[i];
        uint8_t *data = NULL;
        int size = 0;

        if (texture->pixels || texture->name[0] == '\0')
            continue;

        if (!Wad_ReadMipTexture(&wad, texture->name, &data, &size))
            continue;

        if (LoadMipTexture(data, (size_t)size, texture))
            filled++;

        free(data);
    }

    Wad_Close(&wad);

    return filled;
}

static void LoadWadTextures(BSP_Map *map)
{
    char list[BSP_ENTITY_VALUE_SIZE];
    char path[RESOURCE_PATH_SIZE];
    const char *start;
    const char *end;
    char *cursor;
    int missing;

    if (!map || map->num_textures == 0)
        return;

    missing = CountMissingTextures(map);

    if (missing == 0)
        return;

    if (!FindEntityByClass(map, "worldspawn", 0, &start, &end))
        return;

    if (!ValueForKey(start, end, "wad", list, (int)sizeof(list)))
        return;

    cursor = list;

    while (*cursor != '\0' && missing > 0)
    {
        char *separator = strchr(cursor, ';');
        const char *name;

        if (separator)
            *separator = '\0';

        name = WadFileName(cursor);

        if (name[0] != '\0')
        {
            if (Resource_FindFile(map->path, name, path, (int)sizeof(path)))
                missing -= FillTexturesFromWad(map, path);
            else
                TraceLog(LOG_WARNING, "WAD: %s not found", name);
        }

        if (!separator)
            break;

        cursor = separator + 1;
    }

    if (missing > 0)
        TraceLog(LOG_WARNING, "WAD: %d texture(s) unresolved", missing);
}

static int ClampColorComponent(int value)
{
    if (value < 0)
        return 0;

    if (value > 255)
        return 255;

    return value;
}

static bool ParseNextLevel(const BSP_Map *map, const char *start, const char *end, BSP_NextLevel *out)
{
    char value[64];
    Vector3 offset = { 0.0f, 0.0f, 0.0f };
    int model = -1;

    if (!ValueForKey(start, end, "map", value, sizeof(value)) || value[0] == '\0')
        return false;

    snprintf(out->map, sizeof(out->map), "%s", value);

    if (!ValueForKey(start, end, "model", value, sizeof(value)) || value[0] != '*')
        return false;

    if (sscanf(value + 1, "%d", &model) != 1)
        return false;

    if (!map->models || model < 0 || model >= map->num_models)
        return false;

    if (ValueForKey(start, end, "origin", value, sizeof(value)))
        sscanf(value, "%f %f %f", &offset.x, &offset.y, &offset.z);

    out->mins = Vector3Add(map->models[model].mins, offset);
    out->maxs = Vector3Add(map->models[model].maxs, offset);

    return true;
}

bool BSP_GetNextLevelByIndex(const BSP_Map *map, int index, BSP_NextLevel *out)
{
    const char *start;
    const char *end;
    BSP_NextLevel found;
    int matches = 0;
    int i;

    if (!out || index < 0)
        return false;

    for (i = 0; FindEntityByClass(map, "func_next_level", i, &start, &end); i++)
    {
        if (!ParseNextLevel(map, start, end, &found))
            continue;

        if (matches == index)
        {
            *out = found;

            return true;
        }

        matches++;
    }

    return false;
}

bool BSP_GetSunlightColor(const BSP_Map *map, unsigned char *r, unsigned char *g, unsigned char *b)
{
    const char *start;
    const char *end;
    char value[64];
    int cr;
    int cg;
    int cb;

    if (!r || !g || !b || !FindEntityByClass(map, "worldspawn", 0, &start, &end))
        return false;

    if (!ValueForKey(start, end, "_sunlight_color", value, sizeof(value)))
        return false;

    if (sscanf(value, "%d %d %d", &cr, &cg, &cb) != 3)
        return false;

    *r = (unsigned char)ClampColorComponent(cr);
    *g = (unsigned char)ClampColorComponent(cg);
    *b = (unsigned char)ClampColorComponent(cb);

    return true;
}

bool BSP_GetMapName(const BSP_Map *map, char *out, int size)
{
    const char *start;
    const char *end;
    char value[64];

    if (!out || size <= 0 || !FindEntityByClass(map, "worldspawn", 0, &start, &end))
        return false;

    if (!ValueForKey(start, end, "mapname", value, sizeof(value)) || value[0] == '\0')
        return false;

    snprintf(out, (size_t)size, "%s", value);

    return true;
}

bool BSP_GetSkyName(const BSP_Map *map, char *out, int size)
{
    const char *start;
    const char *end;
    char value[64];

    if (!out || size <= 0 || !FindEntityByClass(map, "worldspawn", 0, &start, &end))
        return false;

    if (!ValueForKey(start, end, "skyname", value, sizeof(value)) || value[0] == '\0')
        return false;

    snprintf(out, (size_t)size, "%s", value);

    return true;
}

static bool FindPlayerStart(const BSP_Map *map, float *x, float *y, float *z, float *angle)
{
    const char *start;
    const char *end;
    char value[64];
    float pitch;
    float roll;

    if (!FindEntityByClass(map, "info_player_start", 0, &start, &end))
        return false;

    if (!ValueForKey(start, end, "origin", value, sizeof(value)))
        return false;

    if (sscanf(value, "%f %f %f", x, y, z) != 3)
        return false;

    *angle = 0.0f;

    if (ValueForKey(start, end, "angles", value, sizeof(value)))
        sscanf(value, "%f %f %f", &pitch, angle, &roll);
    else if (ValueForKey(start, end, "angle", value, sizeof(value)))
        sscanf(value, "%f", angle);

    return true;
}

bool BSP_GetPlayerStart(const BSP_Map *map, float *x, float *y, float *z)
{
    float angle;

    if (!x || !y || !z)
        return false;

    return FindPlayerStart(map, x, y, z, &angle);
}

bool BSP_GetPlayerStartAngle(const BSP_Map *map, float *angle)
{
    float x;
    float y;
    float z;

    if (!angle)
        return false;

    return FindPlayerStart(map, &x, &y, &z, angle);
}
