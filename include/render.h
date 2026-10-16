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
    int *faces;
    int face_count;
    int face_capacity;
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
    int sun_direction_loc;
    float color[3];
    float horizon_color[3];
    float sun_direction[3];
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
    int *face_first;
    int *face_count;
    uint8_t *face_visible;
    uint8_t *leaf_vis;
    int leaf_vis_size;
    int num_faces;
    int drawn_faces;
    int draw_calls;
} BSP_Renderer;

bool BSP_RendererInit(BSP_Renderer *renderer, const BSP_Map *map);
void BSP_RendererShutdown(BSP_Renderer *renderer);
void BSP_Render(BSP_Renderer *renderer, const BSP_Map *map, Camera camera, float aspect);
void BSP_RenderSky(const BSP_Renderer *renderer, Vector3 cameraPosition, float time);

#endif
