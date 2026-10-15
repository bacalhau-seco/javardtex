#ifndef RENDER_H
#define RENDER_H

#include <stdbool.h>
#include <stdint.h>

#include <raylib.h>

#include "bsp.h"
#include "lightmap.h"

typedef struct {
    float x;
    float y;
    float z;
    float s;
    float t;
    float lm_s;
    float lm_t;
} BSP_RenderVertex;

typedef struct {
    uint32_t texture_id;
    uint32_t vao;
    uint32_t vbo;
    BSP_RenderVertex *vertices;
    int count;
    int capacity;
} BSP_Batch;

typedef struct {
    uint32_t vao;
    uint32_t vbo;
    float *vertices;
    int count;
    int capacity;
    Shader shader;
    int mvp_loc;
    int sky_color_loc;
    int camera_position_loc;
    int horizon_color_loc;
    int fade_loc;
    float color[3];
    float horizon_color[3];
    float fade;
} BSP_Sky;

typedef struct {
    Shader shader;
    int mvp_loc;
    int lightmap_loc;
    BSP_Lightmap lightmap;
    BSP_Batch *batches;
    int num_batches;
    BSP_Sky sky;
} BSP_Renderer;

bool BSP_RendererInit(BSP_Renderer *renderer, const BSP_Map *map);
void BSP_RendererShutdown(BSP_Renderer *renderer);
void BSP_Render(const BSP_Renderer *renderer);
void BSP_RenderSky(const BSP_Renderer *renderer, Vector3 cameraPosition, float time);

#endif
