#include "../include/render.h"
#include "../include/cvar.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

#define DIFFUSE_TEXTURE_UNIT 0
#define LIGHTMAP_TEXTURE_UNIT 1

static const char *LightmapVertexShader =
    "#version 330\n"
    "layout(location = 0) in vec3 vertexPosition;\n"
    "layout(location = 1) in vec2 vertexTexCoord;\n"
    "layout(location = 5) in vec2 vertexLightmapCoord;\n"
    "uniform mat4 mvp;\n"
    "out vec2 fragTexCoord;\n"
    "out vec2 fragLightmapCoord;\n"
    "void main()\n"
    "{\n"
    "    fragTexCoord = vertexTexCoord;\n"
    "    fragLightmapCoord = vertexLightmapCoord;\n"
    "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
    "}\n";

static const char *LightmapFragmentShader =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "in vec2 fragLightmapCoord;\n"
    "uniform sampler2D texture0;\n"
    "uniform sampler2D lightmap;\n"
    "out vec4 finalColor;\n"
    "void main()\n"
    "{\n"
    "    vec4 diffuse = texture(texture0, fragTexCoord);\n"
    "    if (diffuse.a < 0.5) discard;\n"
    "    vec3 light = texture(lightmap, fragLightmapCoord).rgb * 2.0;\n"
    "    finalColor = vec4(diffuse.rgb * light, 1.0);\n"
    "}\n";

static const char *SkyVertexShader =
    "#version 330\n"
    "layout(location = 0) in vec3 vertexPosition;\n"
    "uniform mat4 mvp;\n"
    "out vec3 fragPosition;\n"
    "void main()\n"
    "{\n"
    "    fragPosition = vertexPosition;\n"
    "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
    "}\n";

static const char *SkyFragmentShader =
    "#version 330\n"
    "in vec3 fragPosition;\n"
    "uniform vec3 skyColor;\n"
    "uniform vec3 horizonColor;\n"
    "uniform vec3 cameraPosition;\n"
    "uniform float skyFade;\n"
    "uniform vec3 sunDirection;\n"
    "out vec4 finalColor;\n"
    "void main()\n"
    "{\n"
    "    vec3 dir = normalize(fragPosition - cameraPosition);\n"
    "    float height = clamp(dot(dir, normalize(sunDirection)), 0.0, 1.0);\n"
    "    float blend = pow(height, clamp(skyFade, 0.15, 0.85) * 2.0);\n"
    "    vec3 color = mix(horizonColor, skyColor, blend);\n"
    "    finalColor = vec4(color, 1.0);\n"
    "}\n";

typedef struct {
    const BSP_TexInfo *texinfo;
    const BSP_Texture *texture;
    const BSP_FaceLightmap *lightmap;
} FaceContext;

static inline bool IsSkyTexture(const char *name)
{
    return strncmp(name, "sky", 3) == 0;
}

static bool IsHiddenTexture(const char *name)
{
    return strncmp(name, "sky", 3) == 0
        || strncmp(name, "clip", 4) == 0
        || strncmp(name, "trigger", 7) == 0
        || strncmp(name, "aaatrigger", 10) == 0
        || strcmp(name, "null") == 0
        || strcmp(name, "hint") == 0
        || strcmp(name, "skip") == 0
        || strcmp(name, "origin") == 0;
}

static uint32_t CreateTexture(const BSP_Texture *texture)
{
    size_t count = (size_t)texture->width * (size_t)texture->height;
    bool masked = texture->name[0] == '{';
    Image image;
    Texture2D gpu;
    Color *pixels;
    size_t i;

    image = GenImageColor((int)texture->width, (int)texture->height, BLANK);

    if (!image.data)
        return 0;

    pixels = image.data;

    for (i = 0; i < count; i++)
    {
        uint8_t index = texture->pixels[i];

        pixels[i].r = texture->palette[index][0];
        pixels[i].g = texture->palette[index][1];
        pixels[i].b = texture->palette[index][2];
        pixels[i].a = (masked && index == 255) ? 0 : 255;
    }

    gpu = LoadTextureFromImage(image);
    UnloadImage(image);

    if (gpu.id == 0)
        return 0;

    SetTextureFilter(gpu, TEXTURE_FILTER_POINT);

    return gpu.id;
}

static bool GrowBatch(BSP_Batch *batch)
{
    int capacity;
    BSP_RenderVertex *grown;

    if (batch->count < batch->capacity)
        return true;

    capacity = batch->capacity == 0 ? 256 : batch->capacity * 2;
    grown = realloc(batch->vertices, sizeof(BSP_RenderVertex) * (size_t)capacity);

    if (!grown)
        return false;

    batch->vertices = grown;
    batch->capacity = capacity;

    return true;
}

static void AppendVertex(BSP_Batch *batch, const FaceContext *context, const Vector3 *vertex)
{
    BSP_RenderVertex *out;
    float s;
    float t;
    float lm_s;
    float lm_t;

    if (!GrowBatch(batch))
        return;

    BSP_GetTexCoord(context->texinfo, vertex, &s, &t);
    BSP_LightmapUV(context->lightmap, s, t, &lm_s, &lm_t);

    out = &batch->vertices[batch->count++];
    out->x = vertex->x;
    out->y = vertex->z;
    out->z = -vertex->y;
    out->s = s / (float)context->texture->width;
    out->t = -t / (float)context->texture->height;
    out->lm_s = lm_s;
    out->lm_t = lm_t;
}

static void AppendTriangle(BSP_Batch *batch, const FaceContext *context, const Vector3 *a, const Vector3 *b, const Vector3 *c)
{
    AppendVertex(batch, context, a);
    AppendVertex(batch, context, c);
    AppendVertex(batch, context, b);
}

static FaceContext MakeFaceContext(const BSP_Renderer *renderer, const BSP_Map *map, int index, int *miptex)
{
    const BSP_Face *face = &map->faces[index];
    FaceContext context;

    context.texinfo = &map->texinfo[face->texinfo];
    *miptex = context.texinfo->miptex;
    context.texture = &map->textures[*miptex];
    context.lightmap = &renderer->lightmap.faces[index];

    return context;
}

static bool IsFaceDrawable(const BSP_Renderer *renderer, const BSP_Map *map, int index)
{
    const BSP_Face *face = &map->faces[index];
    int miptex;

    if (face->texinfo >= map->num_texinfo || face->numedges < 3)
        return false;

    miptex = map->texinfo[face->texinfo].miptex;

    if (miptex < 0 || miptex >= map->num_textures)
        return false;

    return renderer->batches[miptex].texture_id != 0;
}

static bool GrowBatchFaces(BSP_Batch *batch)
{
    int capacity;
    int *grown;

    if (batch->face_count < batch->face_capacity)
        return true;

    capacity = batch->face_capacity == 0 ? 64 : batch->face_capacity * 2;
    grown = realloc(batch->faces, sizeof(int) * (size_t)capacity);

    if (!grown)
        return false;

    batch->faces = grown;
    batch->face_capacity = capacity;

    return true;
}

static void AddFace(BSP_Renderer *renderer, const BSP_Map *map, int index)
{
    const BSP_Face *face = &map->faces[index];
    FaceContext context;
    BSP_Batch *batch;
    Vector3 *vertices;
    int miptex;
    int count;
    int first;
    int i;

    if (!IsFaceDrawable(renderer, map, index))
        return;

    context = MakeFaceContext(renderer, map, index, &miptex);
    batch = &renderer->batches[miptex];

    vertices = malloc(sizeof(Vector3) * face->numedges);

    if (!vertices)
        return;

    count = BSP_GetFaceVertices(map, face, vertices);
    first = batch->count;

    for (i = 1; i + 1 < count; i++)
        AppendTriangle(batch, &context, &vertices[0], &vertices[i], &vertices[i + 1]);

    free(vertices);

    if (batch->count == first)
        return;

    if (!GrowBatchFaces(batch))
        return;

    renderer->face_first[index] = first;
    renderer->face_count[index] = batch->count - first;
    batch->faces[batch->face_count++] = index;
}

static bool GrowSky(BSP_Sky *sky)
{
    int capacity;
    float *grown;

    if (sky->count * 3 < sky->capacity)
        return true;

    capacity = sky->capacity == 0 ? 768 : sky->capacity * 2;
    grown = realloc(sky->vertices, sizeof(float) * (size_t)capacity);

    if (!grown)
        return false;

    sky->vertices = grown;
    sky->capacity = capacity;

    return true;
}

static void AppendSkyVertex(BSP_Sky *sky, const Vector3 *vertex)
{
    float *out;

    if (!GrowSky(sky))
        return;

    out = &sky->vertices[sky->count * 3];
    out[0] = vertex->x;
    out[1] = vertex->z;
    out[2] = -vertex->y;
    sky->count++;
}

static void AddSkyFace(BSP_Sky *sky, const BSP_Map *map, const BSP_Face *face)
{
    Vector3 *vertices;
    int count;
    int i;

    if (face->numedges < 3)
        return;

    vertices = malloc(sizeof(Vector3) * (size_t)face->numedges);

    if (!vertices)
        return;

    count = BSP_GetFaceVertices(map, face, vertices);

    for (i = 1; i + 1 < count; i++)
    {
        AppendSkyVertex(sky, &vertices[0]);
        AppendSkyVertex(sky, &vertices[i + 1]);
        AppendSkyVertex(sky, &vertices[i]);
    }

    free(vertices);
}

static bool IsSkyFace(const BSP_Map *map, const BSP_Face *face)
{
    int miptex;

    if (face->texinfo < 0 || face->texinfo >= map->num_texinfo)
        return false;

    miptex = map->texinfo[face->texinfo].miptex;

    if (miptex < 0 || miptex >= map->num_textures)
        return false;

    return IsSkyTexture(map->textures[miptex].name);
}

static bool LoadSkyShader(BSP_Sky *sky)
{
    sky->shader = LoadShaderFromMemory(SkyVertexShader, SkyFragmentShader);

    if (sky->shader.id == 0)
        return false;

    sky->mvp_loc = GetShaderLocation(sky->shader, "mvp");
    sky->sky_color_loc = GetShaderLocation(sky->shader, "skyColor");
    sky->camera_position_loc = GetShaderLocation(sky->shader, "cameraPosition");
    sky->horizon_color_loc = GetShaderLocation(sky->shader, "horizonColor");
    sky->fade_loc = GetShaderLocation(sky->shader, "skyFade");
    sky->sun_direction_loc = GetShaderLocation(sky->shader, "sunDirection");

    return true;
}

static void UploadSky(BSP_Sky *sky)
{
    if (sky->count > 0)
    {
        sky->vao = rlLoadVertexArray();
        rlEnableVertexArray(sky->vao);

        sky->vbo = rlLoadVertexBuffer(sky->vertices, sky->count * 3 * (int)sizeof(float), false);
        rlEnableVertexBuffer(sky->vbo);

        rlSetVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_POSITION, 3, RL_FLOAT, false, 3 * (int)sizeof(float), 0);
        rlEnableVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_POSITION);

        rlDisableVertexArray();
    }

    free(sky->vertices);
    sky->vertices = NULL;
    sky->capacity = 0;
}

static void BuildSky(BSP_Renderer *renderer, const BSP_Map *map)
{
    unsigned char r = 255;
    unsigned char g = 255;
    unsigned char b = 255;
    BSP_SkyGradient gradient;
    int i;

    renderer->sky.horizon_color[0] = 1.0f;
    renderer->sky.horizon_color[1] = 1.0f;
    renderer->sky.horizon_color[2] = 1.0f;
    renderer->sky.sun_direction[0] = 0.0f;
    renderer->sky.sun_direction[1] = 1.0f;
    renderer->sky.sun_direction[2] = 0.0f;
    renderer->sky.fade = 0.45f;

    if (BSP_GetSunlightColor(map, &r, &g, &b))
    {
        renderer->sky.color[0] = (float)r / 255.0f;
        renderer->sky.color[1] = (float)g / 255.0f;
        renderer->sky.color[2] = (float)b / 255.0f;
    }

    if (BSP_GetSkyGradient(map, &gradient))
    {
        for (i = 0; i < 3; i++)
        {
            renderer->sky.color[i] = gradient.top[i];
            renderer->sky.horizon_color[i] = gradient.bottom[i];
            renderer->sky.sun_direction[i] = gradient.sun_direction[i];
        }

        renderer->sky.fade = gradient.fade;
    }

    for (i = 0; i < map->num_faces; i++)
    {
        if (IsSkyFace(map, &map->faces[i]))
            AddSkyFace(&renderer->sky, map, &map->faces[i]);
    }

    if (renderer->sky.count == 0)
        return;

    if (!LoadSkyShader(&renderer->sky))
    {
        free(renderer->sky.vertices);
        renderer->sky.vertices = NULL;
        renderer->sky.capacity = 0;
        renderer->sky.count = 0;
        return;
    }

    UploadSky(&renderer->sky);
}

static void SetVertexAttributes(void)
{
    const int stride = (int)sizeof(BSP_RenderVertex);

    rlSetVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_POSITION, 3, RL_FLOAT, false, stride, (int)offsetof(BSP_RenderVertex, x));
    rlSetVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD, 2, RL_FLOAT, false, stride, (int)offsetof(BSP_RenderVertex, s));
    rlSetVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD2, 2, RL_FLOAT, false, stride, (int)offsetof(BSP_RenderVertex, lm_s));

    rlEnableVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_POSITION);
    rlEnableVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD);
    rlEnableVertexAttribute(RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD2);
}

static bool UploadBatch(BSP_Batch *batch)
{
    if (batch->count > 0)
    {
        batch->vao = rlLoadVertexArray();
        rlEnableVertexArray(batch->vao);

        batch->vbo = rlLoadVertexBuffer(batch->vertices, batch->count * (int)sizeof(BSP_RenderVertex), false);
        rlEnableVertexBuffer(batch->vbo);
        SetVertexAttributes();

        rlDisableVertexArray();
    }

    free(batch->vertices);
    batch->vertices = NULL;
    batch->capacity = 0;

    return batch->count == 0 || (batch->vao != 0 && batch->vbo != 0);
}

static bool UploadBatches(BSP_Renderer *renderer)
{
    int i;

    for (i = 0; i < renderer->num_batches; i++)
    {
        if (!UploadBatch(&renderer->batches[i]))
            return false;
    }

    return true;
}

static void CreateBatchTextures(BSP_Renderer *renderer, const BSP_Map *map)
{
    int i;

    for (i = 0; i < map->num_textures; i++)
    {
        const BSP_Texture *texture = &map->textures[i];

        if (!texture->pixels || IsHiddenTexture(texture->name))
            continue;

        renderer->batches[i].texture_id = CreateTexture(texture);
    }
}

static inline void BuildBatches(BSP_Renderer *renderer, const BSP_Map *map)
{
    int i;

    for (i = 0; i < map->num_faces; i++)
        AddFace(renderer, map, i);
}

static bool LoadLightmapShader(BSP_Renderer *renderer)
{
    renderer->shader = LoadShaderFromMemory(LightmapVertexShader, LightmapFragmentShader);

    if (renderer->shader.id == 0)
        return false;

    renderer->mvp_loc = GetShaderLocation(renderer->shader, "mvp");
    renderer->lightmap_loc = GetShaderLocation(renderer->shader, "lightmap");

    return true;
}


static void BindLightmapTexture(const BSP_Renderer *renderer)
{
    const int unit = LIGHTMAP_TEXTURE_UNIT;

    SetShaderValue(renderer->shader, renderer->lightmap_loc, &unit, SHADER_UNIFORM_INT);

    rlActiveTextureSlot(LIGHTMAP_TEXTURE_UNIT);
    rlEnableTexture(renderer->lightmap.texture_id);
    rlActiveTextureSlot(DIFFUSE_TEXTURE_UNIT);
}

bool BSP_RendererInit(BSP_Renderer *renderer, const BSP_Map *map)
{
    if (!renderer || !map)
        return false;

    memset(renderer, 0, sizeof(*renderer));

    if (!BSP_LightmapInit(&renderer->lightmap, map) || !LoadLightmapShader(renderer))
    {
        BSP_RendererShutdown(renderer);
        return false;
    }

    renderer->num_faces = map->num_faces;
    renderer->leaf_vis_size = (map->num_leaves + 7) / 8;

    if (map->num_faces > 0)
    {
        renderer->face_first = calloc((size_t)map->num_faces, sizeof(int));
        renderer->face_count = calloc((size_t)map->num_faces, sizeof(int));
        renderer->face_visible = calloc((size_t)map->num_faces, sizeof(uint8_t));
    }

    if (renderer->leaf_vis_size > 0)
        renderer->leaf_vis = calloc((size_t)renderer->leaf_vis_size, sizeof(uint8_t));

    if ((map->num_faces > 0 && (!renderer->face_first || !renderer->face_count || !renderer->face_visible))
        || (renderer->leaf_vis_size > 0 && !renderer->leaf_vis))
    {
        BSP_RendererShutdown(renderer);
        return false;
    }

    if (map->num_textures == 0)
        return true;

    renderer->batches = calloc((size_t)map->num_textures, sizeof(BSP_Batch));

    if (!renderer->batches)
    {
        BSP_RendererShutdown(renderer);
        return false;
    }

    renderer->num_batches = map->num_textures;

    CreateBatchTextures(renderer, map);
    BuildBatches(renderer, map);
    BuildSky(renderer, map);

    if (!UploadBatches(renderer))
    {
        BSP_RendererShutdown(renderer);
        return false;
    }

    return true;
}

void BSP_RendererShutdown(BSP_Renderer *renderer)
{
    int i;

    if (!renderer)
        return;

    for (i = 0; i < renderer->num_batches; i++)
    {
        BSP_Batch *batch = &renderer->batches[i];

        if (batch->texture_id)
            rlUnloadTexture(batch->texture_id);

        if (batch->vbo)
            rlUnloadVertexBuffer(batch->vbo);

        if (batch->vao)
            rlUnloadVertexArray(batch->vao);

        free(batch->vertices);
        free(batch->faces);
    }

    free(renderer->batches);
    free(renderer->face_first);
    free(renderer->face_count);
    free(renderer->face_visible);
    free(renderer->leaf_vis);

    if (renderer->sky.vbo)
        rlUnloadVertexBuffer(renderer->sky.vbo);

    if (renderer->sky.vao)
        rlUnloadVertexArray(renderer->sky.vao);

    if (renderer->sky.shader.id)
        UnloadShader(renderer->sky.shader);

    free(renderer->sky.vertices);

    if (renderer->shader.id)
        UnloadShader(renderer->shader);

    BSP_LightmapShutdown(&renderer->lightmap);
    memset(renderer, 0, sizeof(*renderer));
}

typedef struct {
    Vector3 normal;
    float distance;
} RenderPlane;

static RenderPlane MakePlane(Vector3 normal, Vector3 point)
{
    RenderPlane plane;

    plane.normal = Vector3Normalize(normal);
    plane.distance = -Vector3DotProduct(plane.normal, point);

    return plane;
}

static void BuildFrustum(Camera camera, float aspect, RenderPlane planes[6])
{
    const float far = 8192.0f;
    const float near = 0.1f;

    Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
    Vector3 up = Vector3CrossProduct(right, forward);

    float halfHeight = tanf(camera.fovy * 0.5f * DEG2RAD) * far;
    float halfWidth = halfHeight * aspect;

    Vector3 farCenter = Vector3Add(camera.position, Vector3Scale(forward, far));
    Vector3 farTop = Vector3Scale(up, halfHeight);
    Vector3 farRight = Vector3Scale(right, halfWidth);

    Vector3 toTop = Vector3Add(Vector3Scale(forward, far), farTop);
    Vector3 toBottom = Vector3Subtract(Vector3Scale(forward, far), farTop);
    Vector3 toRight = Vector3Add(Vector3Scale(forward, far), farRight);
    Vector3 toLeft = Vector3Subtract(Vector3Scale(forward, far), farRight);

    planes[0] = MakePlane(forward, Vector3Add(camera.position, Vector3Scale(forward, near)));
    planes[1] = MakePlane(Vector3Negate(forward), farCenter);
    planes[2] = MakePlane(Vector3CrossProduct(toLeft, up), camera.position);
    planes[3] = MakePlane(Vector3CrossProduct(up, toRight), camera.position);
    planes[4] = MakePlane(Vector3CrossProduct(toTop, right), camera.position);
    planes[5] = MakePlane(Vector3CrossProduct(right, toBottom), camera.position);
}

static bool BoxVisible(const RenderPlane planes[6], Vector3 mins, Vector3 maxs)
{
    int i;

    for (i = 0; i < 6; i++)
    {
        Vector3 positive = {
            planes[i].normal.x >= 0.0f ? maxs.x : mins.x,
            planes[i].normal.y >= 0.0f ? maxs.y : mins.y,
            planes[i].normal.z >= 0.0f ? maxs.z : mins.z
        };

        if (Vector3DotProduct(planes[i].normal, positive) + planes[i].distance < 0.0f)
            return false;
    }

    return true;
}

static void LeafBounds(const BSP_Leaf *leaf, Vector3 *mins, Vector3 *maxs)
{
    mins->x = (float)leaf->mins[0];
    maxs->x = (float)leaf->maxs[0];
    mins->y = (float)leaf->mins[2];
    maxs->y = (float)leaf->maxs[2];
    mins->z = -(float)leaf->maxs[1];
    maxs->z = -(float)leaf->mins[1];
}

static void MarkLeafFaces(BSP_Renderer *renderer, const BSP_Map *map, const BSP_Leaf *leaf)
{
    int i;

    for (i = 0; i < leaf->nummarksurfaces; i++)
    {
        int mark = leaf->firstmarksurface + i;
        int face;

        if (mark < 0 || mark >= map->num_marksurfaces)
            continue;

        face = map->marksurfaces[mark];

        if (face >= 0 && face < renderer->num_faces)
            renderer->face_visible[face] = 1;
    }
}

static void ModelBounds(const BSP_Model *model, Vector3 *mins, Vector3 *maxs)
{
    mins->x = model->mins.x;
    maxs->x = model->maxs.x;
    mins->y = model->mins.z;
    maxs->y = model->maxs.z;
    mins->z = -model->maxs.y;
    maxs->z = -model->mins.y;
}

static void MarkModelFaces(BSP_Renderer *renderer, const BSP_Model *model)
{
    int i;

    for (i = 0; i < model->numfaces; i++)
    {
        int face = model->firstface + i;

        if (face >= 0 && face < renderer->num_faces)
            renderer->face_visible[face] = 1;
    }
}

static void MarkBrushModels(BSP_Renderer *renderer, const BSP_Map *map, const RenderPlane planes[6])
{
    int i;

    if (!map->models)
        return;

    for (i = 1; i < map->num_models; i++)
    {
        const BSP_Model *model = &map->models[i];
        Vector3 mins;
        Vector3 maxs;

        if (model->numfaces <= 0)
            continue;

        ModelBounds(model, &mins, &maxs);

        if (!BoxVisible(planes, mins, maxs))
            continue;

        MarkModelFaces(renderer, model);
    }
}

static void MarkVisibleFaces(BSP_Renderer *renderer, const BSP_Map *map, Camera camera, float aspect)
{
    RenderPlane planes[6];
    Vector3 quakeEye = { camera.position.x, -camera.position.z, camera.position.y };
    int viewLeaf = BSP_FindLeaf(map, quakeEye);
    bool hasVis = BSP_DecompressVis(map, viewLeaf, renderer->leaf_vis, renderer->leaf_vis_size);
    int i;

    memset(renderer->face_visible, 0, (size_t)renderer->num_faces);
    BuildFrustum(camera, aspect, planes);

    for (i = 1; i < map->num_leaves; i++)
    {
        const BSP_Leaf *leaf = &map->leaves[i];
        Vector3 mins;
        Vector3 maxs;

        if (leaf->contents == BSP_CONTENTS_SOLID || leaf->nummarksurfaces == 0)
            continue;

        if (hasVis && !(renderer->leaf_vis[(i - 1) >> 3] & (1 << ((i - 1) & 7))))
            continue;

        LeafBounds(leaf, &mins, &maxs);

        if (!BoxVisible(planes, mins, maxs))
            continue;

        MarkLeafFaces(renderer, map, leaf);
    }

    MarkBrushModels(renderer, map, planes);
}

static void DrawVisibleBatch(BSP_Renderer *renderer, const BSP_Batch *batch)
{
    int runFirst = -1;
    int runCount = 0;
    int i;

    if (batch->vao == 0 || batch->face_count == 0)
        return;

    rlEnableTexture(batch->texture_id);
    rlEnableVertexArray(batch->vao);

    for (i = 0; i < batch->face_count; i++)
    {
        const int face = batch->faces[i];
        const int first = renderer->face_first[face];
        const int count = renderer->face_count[face];

        if (!renderer->face_visible[face])
            continue;

        renderer->drawn_faces++;

        if (runFirst >= 0 && first == runFirst + runCount)
        {
            runCount += count;
            continue;
        }

        if (runFirst >= 0)
        {
            rlDrawVertexArray(runFirst, runCount);
            renderer->draw_calls++;
        }

        runFirst = first;
        runCount = count;
    }

    if (runFirst >= 0)
    {
        rlDrawVertexArray(runFirst, runCount);
        renderer->draw_calls++;
    }

    rlDisableVertexArray();
}

void BSP_Render(BSP_Renderer *renderer, const BSP_Map *map, Camera camera, float aspect)
{
    Matrix mvp;
    int i;

    if (!renderer || !map || renderer->shader.id == 0)
        return;

    renderer->drawn_faces = 0;
    renderer->draw_calls = 0;

    if (renderer->face_visible && renderer->num_faces > 0)
        MarkVisibleFaces(renderer, map, camera, aspect);

    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();

    BeginShaderMode(renderer->shader);

    mvp = MatrixMultiply(rlGetMatrixModelview(), rlGetMatrixProjection());
    SetShaderValueMatrix(renderer->shader, renderer->mvp_loc, mvp);
    BindLightmapTexture(renderer);

    for (i = 0; i < renderer->num_batches; i++)
        DrawVisibleBatch(renderer, &renderer->batches[i]);

    rlSetTexture(0);
    EndShaderMode();
}

void BSP_RenderSky(const BSP_Renderer *renderer, Vector3 cameraPosition, float time)
{
    Matrix mvp;

    float cameraPositionValue[3];

    (void)time;

    if (!renderer || renderer->sky.vao == 0 || renderer->sky.shader.id == 0)
        return;

    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();

    BeginShaderMode(renderer->sky.shader);

    mvp = MatrixMultiply(rlGetMatrixModelview(), rlGetMatrixProjection());
    SetShaderValueMatrix(renderer->sky.shader, renderer->sky.mvp_loc, mvp);
    SetShaderValue(renderer->sky.shader, renderer->sky.sky_color_loc, renderer->sky.color, SHADER_UNIFORM_VEC3);

    cameraPositionValue[0] = cameraPosition.x;
    cameraPositionValue[1] = cameraPosition.y;
    cameraPositionValue[2] = cameraPosition.z;
    SetShaderValue(renderer->sky.shader, renderer->sky.camera_position_loc, cameraPositionValue, SHADER_UNIFORM_VEC3);
    SetShaderValue(renderer->sky.shader, renderer->sky.horizon_color_loc, renderer->sky.horizon_color, SHADER_UNIFORM_VEC3);
    SetShaderValue(renderer->sky.shader, renderer->sky.fade_loc, &renderer->sky.fade, SHADER_UNIFORM_FLOAT);
    SetShaderValue(renderer->sky.shader, renderer->sky.sun_direction_loc, renderer->sky.sun_direction, SHADER_UNIFORM_VEC3);

    rlEnableVertexArray(renderer->sky.vao);
    rlDrawVertexArray(0, renderer->sky.count);
    rlDisableVertexArray();

    rlSetTexture(0);
    EndShaderMode();
    rlEnableBackfaceCulling();
}
