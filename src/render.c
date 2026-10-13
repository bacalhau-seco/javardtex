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
    "out vec3 worldPosition;\n"
    "void main()\n"
    "{\n"
    "    worldPosition = vertexPosition;\n"
    "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
    "}\n";

static const char *SkyFragmentShader =
    "#version 330\n"
    "in vec3 worldPosition;\n"
    "uniform vec3 cameraPosition;\n"
    "uniform vec3 skyColor;\n"
    "uniform float time;\n"
    "uniform float cloudScale;\n"
    "uniform float cloudHeight;\n"
    "uniform float cloudSpeed;\n"
    "uniform float cloudPixels;\n"
    "uniform float cloudOpacity;\n"
    "out vec4 finalColor;\n"
    "vec2 Hash2(vec2 cell)\n"
    "{\n"
    "    vec3 p = fract(vec3(cell.xyx) * vec3(0.1031, 0.1030, 0.0973));\n"
    "    p += dot(p, p.yzx + 33.33);\n"
    "    return fract((p.xx + p.yz) * p.zy) * 2.0 - 1.0;\n"
    "}\n"
    "float Simplex(vec2 point)\n"
    "{\n"
    "    const float F2 = 0.3660254;\n"
    "    const float G2 = 0.2113249;\n"
    "    vec2 cell = floor(point + (point.x + point.y) * F2);\n"
    "    vec2 a = point - cell + (cell.x + cell.y) * G2;\n"
    "    vec2 step1 = a.x > a.y ? vec2(1.0, 0.0) : vec2(0.0, 1.0);\n"
    "    vec2 b = a - step1 + G2;\n"
    "    vec2 c = a - 1.0 + 2.0 * G2;\n"
    "    vec3 weight = max(0.5 - vec3(dot(a, a), dot(b, b), dot(c, c)), 0.0);\n"
    "    weight = weight * weight * weight * weight;\n"
    "    vec3 gradient = vec3(dot(Hash2(cell), a), dot(Hash2(cell + step1), b), dot(Hash2(cell + 1.0), c));\n"
    "    return dot(weight, gradient) * 35.0 + 0.5;\n"
    "}\n"
    "float Fbm(vec2 point, int octaves)\n"
    "{\n"
    "    float value = 0.0;\n"
    "    float amplitude = 0.5;\n"
    "    float frequency = 1.0;\n"
    "    int i;\n"
    "    for (i = 0; i < octaves; i++)\n"
    "    {\n"
    "        value += Simplex(point * frequency) * amplitude;\n"
    "        frequency *= 2.0;\n"
    "        amplitude *= 0.5;\n"
    "    }\n"
    "    return value;\n"
    "}\n"
    "float Clouds(vec2 point)\n"
    "{\n"
    "    vec2 warp = vec2(Fbm(point + vec2(1.7, 9.2), 2), Fbm(point + vec2(8.3, 2.8), 2));\n"
    "    return Fbm(point + warp * 1.2, 4);\n"
    "}\n"
    "vec4 Layer(vec3 direction, float height, float scale, float speed, float seed, float threshold)\n"
    "{\n"
    "    float up = max(abs(direction.y), 0.03);\n"
    "    vec2 plane = cameraPosition.xz + direction.xz * (height / up);\n"
    "    vec2 point = plane * scale + vec2(seed, time * speed * scale);\n"
    "    float pixels = cloudPixels / (cloudScale / scale);\n"
    "    point = floor(point * pixels) / pixels;\n"
    "    float density = Clouds(point);\n"
    "    float mass = clamp((density - threshold) / (1.0 - threshold), 0.0, 1.0);\n"
    "    if (mass <= 0.0)\n"
    "        return vec4(0.0);\n"
    "    float alpha = ceil(mass * 5.0) / 5.0;\n"
    "    float shade = 0.55 + floor(mass * 4.0) / 4.0 * 0.45;\n"
    "    return vec4(vec3(shade), alpha);\n"
    "}\n"
    "void main()\n"
    "{\n"
    "    vec3 direction = normalize(worldPosition - cameraPosition);\n"
    "    vec3 color = skyColor;\n"
    "    float fade = smoothstep(0.02, 0.10, abs(direction.y));\n"
    "    fade = ceil(fade * 4.0) / 4.0;\n"
    "    if (fade <= 0.0)\n"
    "    {\n"
    "        finalColor = vec4(color, 1.0);\n"
    "        return;\n"
    "    }\n"
    "    vec4 low = Layer(direction, cloudHeight, cloudScale, cloudSpeed, 3.0, 0.48);\n"
    "    float cover = low.a * cloudOpacity * fade;\n"
    "    if (cover < 0.99)\n"
    "    {\n"
    "        vec4 high = Layer(direction, cloudHeight * 2.2, cloudScale * 0.45, cloudSpeed * 0.55, 31.0, 0.58);\n"
    "        vec4 mid = Layer(direction, cloudHeight * 1.4, cloudScale * 0.75, cloudSpeed * 0.8, 17.0, 0.52);\n"
    "        color = mix(color, high.rgb * 0.85, high.a * cloudOpacity * 0.6 * fade);\n"
    "        color = mix(color, mid.rgb * 0.92, mid.a * cloudOpacity * 0.8 * fade);\n"
    "    }\n"
    "    color = mix(color, low.rgb, cover);\n"
    "    finalColor = vec4(color, 1.0);\n"
    "}\n";

typedef struct {
    const BSP_TexInfo *texinfo;
    const BSP_Texture *texture;
    const BSP_FaceLightmap *lightmap;
} FaceContext;

static bool IsSkyTexture(const char *name)
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
    AppendVertex(batch, context, b);
    AppendVertex(batch, context, c);

    AppendVertex(batch, context, c);
    AppendVertex(batch, context, b);
    AppendVertex(batch, context, a);
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

static void AddFace(BSP_Renderer *renderer, const BSP_Map *map, int index)
{
    const BSP_Face *face = &map->faces[index];
    FaceContext context;
    BSP_Batch *batch;
    Vector3 *vertices;
    int miptex;
    int count;
    int i;

    if (!IsFaceDrawable(renderer, map, index))
        return;

    context = MakeFaceContext(renderer, map, index, &miptex);
    batch = &renderer->batches[miptex];

    vertices = malloc(sizeof(Vector3) * face->numedges);

    if (!vertices)
        return;

    count = BSP_GetFaceVertices(map, face, vertices);

    for (i = 1; i + 1 < count; i++)
        AppendTriangle(batch, &context, &vertices[0], &vertices[i], &vertices[i + 1]);

    free(vertices);
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
        AppendSkyVertex(sky, &vertices[i]);
        AppendSkyVertex(sky, &vertices[i + 1]);

        AppendSkyVertex(sky, &vertices[i + 1]);
        AppendSkyVertex(sky, &vertices[i]);
        AppendSkyVertex(sky, &vertices[0]);
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
    sky->camera_loc = GetShaderLocation(sky->shader, "cameraPosition");
    sky->time_loc = GetShaderLocation(sky->shader, "time");
    sky->sky_color_loc = GetShaderLocation(sky->shader, "skyColor");
    sky->scale_loc = GetShaderLocation(sky->shader, "cloudScale");
    sky->height_loc = GetShaderLocation(sky->shader, "cloudHeight");
    sky->speed_loc = GetShaderLocation(sky->shader, "cloudSpeed");
    sky->pixels_loc = GetShaderLocation(sky->shader, "cloudPixels");
    sky->opacity_loc = GetShaderLocation(sky->shader, "cloudOpacity");

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
    int i;

    if (BSP_GetSunlightColor(map, &r, &g, &b))
    {
        renderer->sky.color[0] = (float)r / 255.0f;
        renderer->sky.color[1] = (float)g / 255.0f;
        renderer->sky.color[2] = (float)b / 255.0f;
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

static void BuildBatches(BSP_Renderer *renderer, const BSP_Map *map)
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

static void DrawBatch(const BSP_Batch *batch)
{
    if (batch->vao == 0)
        return;

    rlEnableTexture(batch->texture_id);
    rlEnableVertexArray(batch->vao);
    rlDrawVertexArray(0, batch->count);
    rlDisableVertexArray();
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
    }

    free(renderer->batches);

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

void BSP_Render(const BSP_Renderer *renderer)
{
    Matrix mvp;
    int i;

    if (!renderer || renderer->shader.id == 0)
        return;

    rlDrawRenderBatchActive();
    rlDisableBackfaceCulling();

    BeginShaderMode(renderer->shader);

    mvp = MatrixMultiply(rlGetMatrixModelview(), rlGetMatrixProjection());
    SetShaderValueMatrix(renderer->shader, renderer->mvp_loc, mvp);
    BindLightmapTexture(renderer);

    for (i = 0; i < renderer->num_batches; i++)
        DrawBatch(&renderer->batches[i]);

    rlSetTexture(0);
    EndShaderMode();
    rlEnableBackfaceCulling();
}

void BSP_RenderSky(const BSP_Renderer *renderer, Vector3 cameraPosition, float time)
{
    Matrix mvp;
    float camera[3];
    float scale;
    float height;
    float speed;
    float pixels;
    float opacity;

    if (!renderer || renderer->sky.vao == 0 || renderer->sky.shader.id == 0)
        return;

    scale = 1.0f / Cvar_Value("cloudsize", 2048.0f);
    height = Cvar_Value("cloudheight", 1024.0f);
    speed = Cvar_Value("cloudspeed", 120.0f);
    pixels = Cvar_Value("cloudpixels", 128.0f);
    opacity = Cvar_Value("cloudopacity", 0.8f);

    camera[0] = cameraPosition.x;
    camera[1] = cameraPosition.y;
    camera[2] = cameraPosition.z;

    rlDrawRenderBatchActive();
    rlDisableBackfaceCulling();

    BeginShaderMode(renderer->sky.shader);

    mvp = MatrixMultiply(rlGetMatrixModelview(), rlGetMatrixProjection());
    SetShaderValueMatrix(renderer->sky.shader, renderer->sky.mvp_loc, mvp);
    SetShaderValue(renderer->sky.shader, renderer->sky.camera_loc, camera, SHADER_UNIFORM_VEC3);
    SetShaderValue(renderer->sky.shader, renderer->sky.sky_color_loc, renderer->sky.color, SHADER_UNIFORM_VEC3);
    SetShaderValue(renderer->sky.shader, renderer->sky.time_loc, &time, SHADER_UNIFORM_FLOAT);
    SetShaderValue(renderer->sky.shader, renderer->sky.scale_loc, &scale, SHADER_UNIFORM_FLOAT);
    SetShaderValue(renderer->sky.shader, renderer->sky.height_loc, &height, SHADER_UNIFORM_FLOAT);
    SetShaderValue(renderer->sky.shader, renderer->sky.speed_loc, &speed, SHADER_UNIFORM_FLOAT);
    SetShaderValue(renderer->sky.shader, renderer->sky.pixels_loc, &pixels, SHADER_UNIFORM_FLOAT);
    SetShaderValue(renderer->sky.shader, renderer->sky.opacity_loc, &opacity, SHADER_UNIFORM_FLOAT);

    rlEnableVertexArray(renderer->sky.vao);
    rlDrawVertexArray(0, renderer->sky.count);
    rlDisableVertexArray();

    rlSetTexture(0);
    EndShaderMode();
    rlEnableBackfaceCulling();
}
