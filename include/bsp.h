#ifndef BSP_H
#define BSP_H

#include <stdint.h>
#include <stdbool.h>

#include "raylib.h"

#define BSP_VERSION 30
#define BSP_LUMP_COUNT 15
#define BSP_MAX_TEXTURE_NAME 16

#define BSP_LUMP_ENTITIES      0
#define BSP_LUMP_PLANES        1
#define BSP_LUMP_TEXTURES      2
#define BSP_LUMP_VERTICES      3
#define BSP_LUMP_VISIBILITY    4
#define BSP_LUMP_NODES         5
#define BSP_LUMP_TEXINFO       6
#define BSP_LUMP_FACES         7
#define BSP_LUMP_LIGHTING      8
#define BSP_LUMP_CLIPNODES     9
#define BSP_LUMP_LEAVES       10
#define BSP_LUMP_MARKSURFACES 11
#define BSP_LUMP_EDGES         12
#define BSP_LUMP_SURFEDGES     13
#define BSP_LUMP_MODELS       14

typedef struct {
    int32_t offset;
    int32_t length;
} BSP_Lump;

typedef struct {
    int32_t version;
    BSP_Lump lumps[BSP_LUMP_COUNT];
} BSP_Header;

typedef struct {
    float normal[3];
    float dist;
    int32_t type;
} BSP_Plane;

typedef struct {
    float x;
    float y;
    float z;
} BSP_Vertex;

typedef struct {
    uint16_t v[2];
} BSP_Edge;

typedef struct {
    int32_t planenum;
    int16_t children[2];
    int16_t mins[3];
    int16_t maxs[3];
    uint16_t firstface;
    uint16_t numfaces;
} BSP_Node;

typedef struct {
    float vecs[2][4];
    int32_t miptex;
    int32_t flags;
} BSP_TexInfo;

typedef struct {
    uint16_t planenum;
    uint16_t side;
    int32_t firstedge;
    uint16_t numedges;
    uint16_t texinfo;
    uint8_t styles[4];
    int32_t lightofs;
} BSP_Face;

typedef struct {
    int32_t contents;
    int32_t visofs;
    int16_t mins[3];
    int16_t maxs[3];
    uint16_t firstmarksurface;
    uint16_t nummarksurfaces;
    uint8_t ambient_level[4];
} BSP_Leaf;

typedef struct {
    int32_t planenum;
    int16_t children[2];
} BSP_ClipNode;

typedef struct {
    char name[BSP_MAX_TEXTURE_NAME];
    uint32_t width;
    uint32_t height;
    uint32_t offsets[4];
    uint8_t *pixels[4];
    uint8_t *palette;
    uint8_t *data;
    uint32_t data_size;
    Texture2D texture;
    bool loaded;
} BSP_Texture;

typedef struct {
    BSP_Face face;
    BSP_TexInfo *texinfo;
    BSP_Texture *texture;
} BSP_Surface;

typedef struct {
    int version;
    char *entities;
    int entities_size;
    BSP_Plane *planes;
    int num_planes;
    BSP_Vertex *vertices;
    int num_vertices;
    BSP_Node *nodes;
    int num_nodes;
    BSP_TexInfo *texinfo;
    int num_texinfo;
    BSP_Face *faces;
    int num_faces;
    BSP_ClipNode *clipnodes;
    int num_clipnodes;
    BSP_Leaf *leaves;
    int num_leaves;
    uint16_t *marksurfaces;
    int num_marksurfaces;
    BSP_Edge *edges;
    int num_edges;
    int32_t *surfedges;
    int num_surfedges;
    uint8_t *visibility;
    int visibility_size;
    uint8_t *lighting;
    int lighting_size;
    BSP_Surface *surfaces;
    BSP_Texture *textures;
    int num_textures;
} BSP_Map;

bool BSP_Load(BSP_Map *map, const char *path);
void BSP_Free(BSP_Map *map);
bool BSP_LoadTexturesGL(BSP_Map *map);
void BSP_Render(const BSP_Map *map);
BSP_Texture *BSP_GetTexture(BSP_Map *map, int index);
int BSP_GetFaceVertices(const BSP_Map *map, const BSP_Face *face, BSP_Vertex *out);
void BSP_GetTexCoord(const BSP_TexInfo *texinfo, const BSP_Vertex *vertex, float *s, float *t);
bool BSP_GetPlayerStart(const BSP_Map *map, Vector3 *position);

#endif
