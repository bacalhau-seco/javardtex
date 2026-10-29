#ifndef RENDER_H
#define RENDER_H

#include <stdbool.h>
#include <stdint.h>

#include <raylib.h>

#include "bsp.h"
#include "lightmap.h"
#include "sky.h"

#define BSP_MAX_DETAIL_TEXTURES 64

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
    uint32_t detail_id;
    float detail_scale[2];
    uint32_t vao;
    uint32_t vbo;
    BSP_RenderVertex *vertices;
    int count;
    int capacity;
    int *faces;
    int face_count;
    int face_capacity;
} BSP_Batch;

typedef struct {
    Shader shader;
    int mvp_loc;
    int lightmap_loc;
    int detail_loc;
    int detail_scale_loc;
    uint32_t neutral_detail_id;
    uint32_t detail_ids[BSP_MAX_DETAIL_TEXTURES];
    char detail_paths[BSP_MAX_DETAIL_TEXTURES][64];
    int num_detail_textures;
    Sky sky;
    BSP_Lightmap lightmap;
    BSP_Batch *batches;
    int num_batches;
    int *face_first;
    int *face_count;
    uint8_t *face_visible;
    uint8_t *leaf_vis;
    int leaf_vis_size;
    int num_faces;
    bool mipmaps;
    int drawn_faces;
    int draw_calls;
} BSP_Renderer;

bool BSP_RendererInit(BSP_Renderer *renderer, const BSP_Map *map);
void BSP_RendererShutdown(BSP_Renderer *renderer);
void BSP_Render(BSP_Renderer *renderer, const BSP_Map *map, Camera camera, float aspect);

#endif
