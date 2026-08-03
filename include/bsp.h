#ifndef __BSP_H
#define __BSP_H

#define BSPVERSION      30
#define HEADER_LUMPS    15

#define LUMP_ENTITIES       0
#define LUMP_PLANES         1
#define LUMP_TEXTURES       2
#define LUMP_VERTEXES       3
#define LUMP_VISIBILITY     4
#define LUMP_NODES          5
#define LUMP_TEXINFO        6
#define LUMP_FACES          7
#define LUMP_LIGHTING       8
#define LUMP_CLIPNODES      9
#define LUMP_LEAFS          10
#define LUMP_MARKSURFACES   11
#define LUMP_EDGES          12
#define LUMP_SURFEDGES      13
#define LUMP_MODELS         14

typedef struct
{
    int fileofs;
    int filelen;
} lump_t;

typedef struct
{
    int version;
    lump_t lumps[HEADER_LUMPS];
} dheader_t;

typedef struct
{
    float point[3];
} dvertex_t;

typedef struct
{
    float normal[3];
    float dist;
    int type;
} dplane_t;

#define MIPLEVELS 4

typedef struct
{
    int nummiptex;
    int dataofs[4];
} dmiptexlump_t;

typedef struct
{
    char name[16];
    unsigned width;
    unsigned height;
    unsigned offsets[MIPLEVELS];
} miptex_t;

#endif
