#ifndef BSP_H
#define BSP_H

#include <stdbool.h>
#include <stdint.h>

#include "raylib.h"

#define BSP_VERSION 30
#define BSP_LUMP_COUNT 15
#define BSP_MAX_TEXTURES 4096
#define BSP_TEXTURE_NAME_SIZE 16
#define BSP_MAP_NAME_SIZE 64
#define BSP_CONTENTS_EMPTY -1
#define BSP_CONTENTS_SOLID -2
#define BSP_HULL_PLAYER 1
#define BSP_HULL_CROUCH 3

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
#define BSP_LUMP_EDGES        12
#define BSP_LUMP_SURFEDGES    13
#define BSP_LUMP_MODELS       14

typedef struct {
    int32_t offset;
    int32_t length;
} BSP_Lump;

typedef struct {
    Vector3 normal;
    float dist;
    int32_t type;
} BSP_Plane;

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
    int32_t planenum;
    int16_t children[2];
} BSP_ClipNode;

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
    Vector3 mins;
    Vector3 maxs;
    Vector3 origin;
    int32_t headnode[4];
    int32_t visleafs;
    int32_t firstface;
    int32_t numfaces;
} BSP_Model;

typedef struct {
    char name[BSP_TEXTURE_NAME_SIZE];
    uint32_t width;
    uint32_t height;
    uint8_t *pixels;
    uint8_t palette[256][3];
} BSP_Texture;

typedef struct {
    int32_t version;

    char *entities;
    int entities_size;

    uint8_t *lighting;
    int lighting_size;

    uint8_t *visdata;
    int visdata_size;

    BSP_Plane *planes;
    int num_planes;

    Vector3 *vertices;
    int num_vertices;

    BSP_Edge *edges;
    int num_edges;

    int32_t *surfedges;
    int num_surfedges;

    BSP_TexInfo *texinfo;
    int num_texinfo;

    BSP_Face *faces;
    int num_faces;

    BSP_Node *nodes;
    int num_nodes;

    BSP_ClipNode *clipnodes;
    int num_clipnodes;

    BSP_Leaf *leaves;
    int num_leaves;

    uint16_t *marksurfaces;
    int num_marksurfaces;

    BSP_Model *models;
    int num_models;

    BSP_Texture *textures;
    int num_textures;
} BSP_Map;

typedef char BSP_LayoutAssert[
    (sizeof(BSP_Lump) == 8
     && sizeof(BSP_Plane) == 20
     && sizeof(BSP_Edge) == 4
     && sizeof(BSP_Node) == 24
     && sizeof(BSP_TexInfo) == 40
     && sizeof(BSP_Face) == 20
     && sizeof(BSP_ClipNode) == 8
     && sizeof(BSP_Leaf) == 28
     && sizeof(BSP_Model) == 64
     && sizeof(Vector3) == 12) ? 1 : -1];

bool BSP_Load(BSP_Map *map, const char *path);
void BSP_Free(BSP_Map *map);


int BSP_GetFaceVertices(const BSP_Map *map, const BSP_Face *face, Vector3 *out);

void BSP_GetTexCoord(const BSP_TexInfo *texinfo, const Vector3 *vertex, float *s, float *t);

typedef struct BSP_NextLevel
{
    char map[BSP_MAP_NAME_SIZE];
    Vector3 mins;
    Vector3 maxs;
} BSP_NextLevel;

bool BSP_GetPlayerStart(const BSP_Map *map, float *x, float *y, float *z);
bool BSP_GetPlayerStartAngle(const BSP_Map *map, float *angle);
bool BSP_GetNextLevel(const BSP_Map *map, BSP_NextLevel *out);
bool BSP_GetSunlightColor(const BSP_Map *map, unsigned char *r, unsigned char *g, unsigned char *b);
bool BSP_GetMapName(const BSP_Map *map, char *out, int size);
int BSP_FindLeaf(const BSP_Map *map, Vector3 position);
bool BSP_DecompressVis(const BSP_Map *map, int leaf, uint8_t *out, int out_size);

typedef struct {
    float top[3];
    float bottom[3];
    float fade;
    float sun_direction[3];
} BSP_SkyGradient;

bool BSP_GetSkyGradient(const BSP_Map *map, BSP_SkyGradient *out);

#endif
