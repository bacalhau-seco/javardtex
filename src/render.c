#include "../include/render.h"
#include "../include/cvar.h"
#include "../include/resource.h"
#include "../include/settings.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

#define DIFFUSE_TEXTURE_UNIT 0
#define LIGHTMAP_TEXTURE_UNIT 1
#define DETAIL_TEXTURE_UNIT 2

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
    "uniform sampler2D detail;\n"
    "uniform vec2 detailScale;\n"
    "out vec4 finalColor;\n"
    "void main()\n"
    "{\n"
    "    vec4 diffuse = texture(texture0, fragTexCoord);\n"
    "    if (diffuse.a < 0.5) discard;\n"
    "    vec3 light = texture(lightmap, fragLightmapCoord).rgb * 2.0;\n"
    "    vec3 detailColor = texture(detail, fragTexCoord * detailScale).rgb * 2.0;\n"
    "    finalColor = vec4(diffuse.rgb * light * detailColor, 1.0);\n"
    "}\n";

typedef struct {
    const BSP_TexInfo *texinfo;
    const BSP_Texture *texture;
    const BSP_FaceLightmap *lightmap;
} FaceContext;

static bool IsHiddenTexture(const char *name)
{
    return strncasecmp(name, "sky", 3) == 0
        || strncasecmp(name, "clip", 4) == 0
        || strncasecmp(name, "trigger", 7) == 0
        || strncasecmp(name, "aaatrigger", 10) == 0
        || strcasecmp(name, "null") == 0
        || strcasecmp(name, "hint") == 0
        || strcasecmp(name, "skip") == 0
        || strcasecmp(name, "origin") == 0;
}

static inline void ApplyTextureFilter(uint32_t id, bool mipmaps)
{
    if (id == 0)
        return;

    rlTextureParameters(id, RL_TEXTURE_MAG_FILTER, RL_TEXTURE_FILTER_NEAREST);
    rlTextureParameters(
        id,
        RL_TEXTURE_MIN_FILTER,
        mipmaps ? RL_TEXTURE_FILTER_NEAREST_MIP_LINEAR : RL_TEXTURE_FILTER_NEAREST
    );
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

    GenTextureMipmaps(&gpu);
    ApplyTextureFilter(gpu.id, Settings_Mipmaps());

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
    out->y = vertex->y;
    out->z = vertex->z;
    out->s = s / (float)context->texture->width;
    out->t = t / (float)context->texture->height;
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

    renderer->mipmaps = Settings_Mipmaps();
}

static void RefreshTextureFilters(BSP_Renderer *renderer)
{
    const bool mipmaps = Settings_Mipmaps();
    int i;

    if (renderer->mipmaps == mipmaps)
        return;

    renderer->mipmaps = mipmaps;

    for (i = 0; i < renderer->num_batches; i++)
        ApplyTextureFilter(renderer->batches[i].texture_id, mipmaps);
}

static uint32_t CreateNeutralDetailTexture(void)
{
    const Color neutral = { 128, 128, 128, 255 };
    Image image = GenImageColor(1, 1, neutral);
    Texture2D gpu = LoadTextureFromImage(image);

    UnloadImage(image);

    return gpu.id;
}

static uint32_t FindOrLoadDetailTexture(BSP_Renderer *renderer, const BSP_Map *map, const char *name)
{
    char relative[RESOURCE_PATH_SIZE];
    char path[RESOURCE_PATH_SIZE];
    Texture2D gpu;
    int slot;
    int i;

    for (i = 0; i < renderer->num_detail_textures; i++)
    {
        if (strcmp(renderer->detail_paths[i], name) == 0)
            return renderer->detail_ids[i];
    }

    if (renderer->num_detail_textures >= BSP_MAX_DETAIL_TEXTURES)
        return 0;

    snprintf(relative, sizeof(relative), "gfx/%s.tga", name);

    if (!Resource_FindFile(map->path, relative, path, (int)sizeof(path)))
    {
        TraceLog(LOG_WARNING, "DETAIL: missing %s", relative);
        return 0;
    }

    gpu = LoadTexture(path);

    if (gpu.id == 0)
    {
        TraceLog(LOG_WARNING, "DETAIL: failed to load %s", path);
        return 0;
    }

    GenTextureMipmaps(&gpu);
    SetTextureFilter(gpu, TEXTURE_FILTER_TRILINEAR);
    SetTextureWrap(gpu, TEXTURE_WRAP_REPEAT);

    slot = renderer->num_detail_textures++;
    renderer->detail_ids[slot] = gpu.id;
    snprintf(renderer->detail_paths[slot], sizeof(renderer->detail_paths[slot]), "%s", name);

    return gpu.id;
}

static void ApplyDetailLine(BSP_Renderer *renderer, const BSP_Map *map, const char *line)
{
    char texture[BSP_TEXTURE_NAME_SIZE];
    char detail[64];
    float xscale;
    float yscale;
    uint32_t detail_id;
    int i;

    if (sscanf(line, "%15s %63s %f %f", texture, detail, &xscale, &yscale) != 4)
        return;

    if (texture[0] == '/' || texture[0] == '#')
        return;

    if (xscale <= 0.0f || yscale <= 0.0f)
        return;

    detail_id = FindOrLoadDetailTexture(renderer, map, detail);

    if (detail_id == 0)
        return;

    for (i = 0; i < map->num_textures && i < renderer->num_batches; i++)
    {
        if (strcasecmp(map->textures[i].name, texture) != 0)
            continue;

        renderer->batches[i].detail_id = detail_id;
        renderer->batches[i].detail_scale[0] = xscale;
        renderer->batches[i].detail_scale[1] = yscale;
    }
}

static void LoadDetailTextures(BSP_Renderer *renderer, const BSP_Map *map)
{
    char base[BSP_MAP_NAME_SIZE];
    char relative[RESOURCE_PATH_SIZE];
    char path[RESOURCE_PATH_SIZE];
    char line[256];
    FILE *file;
    int i;

    renderer->neutral_detail_id = CreateNeutralDetailTexture();

    for (i = 0; i < renderer->num_batches; i++)
    {
        renderer->batches[i].detail_id = renderer->neutral_detail_id;
        renderer->batches[i].detail_scale[0] = 1.0f;
        renderer->batches[i].detail_scale[1] = 1.0f;
    }

    if (!Resource_MapBaseName(map->path, base, (int)sizeof(base)))
        return;

    snprintf(relative, sizeof(relative), "maps/%s_detail.txt", base);

    if (!Resource_FindFile(map->path, relative, path, (int)sizeof(path)))
        return;

    file = fopen(path, "r");

    if (!file)
        return;

    while (fgets(line, sizeof(line), file))
        ApplyDetailLine(renderer, map, line);

    fclose(file);
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
    renderer->detail_loc = GetShaderLocation(renderer->shader, "detail");
    renderer->detail_scale_loc = GetShaderLocation(renderer->shader, "detailScale");

    return true;
}


static void BindLightmapTexture(const BSP_Renderer *renderer)
{
    const int lightmapUnit = LIGHTMAP_TEXTURE_UNIT;
    const int detailUnit = DETAIL_TEXTURE_UNIT;

    SetShaderValue(renderer->shader, renderer->lightmap_loc, &lightmapUnit, SHADER_UNIFORM_INT);
    SetShaderValue(renderer->shader, renderer->detail_loc, &detailUnit, SHADER_UNIFORM_INT);

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

    Sky_Load(&renderer->sky, map);

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
    LoadDetailTextures(renderer, map);
    BuildBatches(renderer, map);

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

    for (i = 0; i < renderer->num_detail_textures; i++)
    {
        if (renderer->detail_ids[i])
            rlUnloadTexture(renderer->detail_ids[i]);
    }

    if (renderer->neutral_detail_id)
        rlUnloadTexture(renderer->neutral_detail_id);

    Sky_Unload(&renderer->sky);

    free(renderer->batches);
    free(renderer->face_first);
    free(renderer->face_count);
    free(renderer->face_visible);
    free(renderer->leaf_vis);

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
    mins->y = (float)leaf->mins[1];
    maxs->y = (float)leaf->maxs[1];
    mins->z = (float)leaf->mins[2];
    maxs->z = (float)leaf->maxs[2];
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
    mins->y = model->mins.y;
    maxs->y = model->maxs.y;
    mins->z = model->mins.z;
    maxs->z = model->maxs.z;
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
    int viewLeaf = BSP_FindLeaf(map, camera.position);
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

    rlActiveTextureSlot(DETAIL_TEXTURE_UNIT);
    rlEnableTexture(batch->detail_id ? batch->detail_id : renderer->neutral_detail_id);
    rlActiveTextureSlot(DIFFUSE_TEXTURE_UNIT);

    SetShaderValue(renderer->shader, renderer->detail_scale_loc, batch->detail_scale, SHADER_UNIFORM_VEC2);

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

    RefreshTextureFilters(renderer);

    if (renderer->face_visible && renderer->num_faces > 0)
        MarkVisibleFaces(renderer, map, camera, aspect);

    rlDrawRenderBatchActive();
    Sky_Draw(&renderer->sky, camera.position);
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
