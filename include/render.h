#ifndef RENDER_H
#define RENDER_H

#include <stdbool.h>
#include <stdint.h>

#include "bsp.h"

typedef struct {
    float x;
    float y;
    float z;
    float s;
    float t;
} BSP_RenderVertex;

typedef struct {
    uint32_t texture_id;
    BSP_RenderVertex *vertices;
    int count;
    int capacity;
} BSP_Batch;

typedef struct {
    BSP_Batch *batches;
    int num_batches;
} BSP_Renderer;

bool BSP_RendererInit(BSP_Renderer *renderer, const BSP_Map *map);
void BSP_RendererShutdown(BSP_Renderer *renderer);
void BSP_Render(const BSP_Renderer *renderer);

#endif
