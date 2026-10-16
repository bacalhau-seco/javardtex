#ifndef LIGHTMAP_H
#define LIGHTMAP_H

#include <stdbool.h>
#include <stdint.h>

#include "bsp.h"

#define LIGHTMAP_SIZE_MIN 1024
#define LIGHTMAP_SIZE_MAX 8192
#define LIGHTMAP_LUXEL_SIZE 16

typedef struct {
    bool valid;
    int atlas_size;
    int x;
    int y;
    int min_s;
    int min_t;
    int width;
    int height;
} BSP_FaceLightmap;

typedef struct {
    uint32_t texture_id;
    int size;
    BSP_FaceLightmap *faces;
} BSP_Lightmap;

bool BSP_LightmapInit(BSP_Lightmap *lightmap, const BSP_Map *map);
void BSP_LightmapShutdown(BSP_Lightmap *lightmap);
void BSP_LightmapUV(const BSP_FaceLightmap *face, float s, float t, float *u, float *v);

#endif
