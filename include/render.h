#ifndef RENDER_H
#define RENDER_H

#include <stdbool.h>
#include <stdint.h>

#include "bsp.h"

typedef struct {
    uint32_t *textures;
    int num_textures;
} BSP_Renderer;

bool BSP_RendererInit(BSP_Renderer *renderer, const BSP_Map *map);
void BSP_RendererShutdown(BSP_Renderer *renderer);
void BSP_Render(const BSP_Map *map, const BSP_Renderer *renderer);

#endif
