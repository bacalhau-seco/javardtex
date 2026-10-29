#include "../include/game.h"
#include "../include/move.h"
#include "../include/input.h"
#include "../include/camera.h"
#include "../include/cvar.h"
#include "../include/title.h"
#include "../include/settings.h"
#include "../include/bsp.h"
#include "../include/render.h"
#include "../include/collision.h"
#include "../include/args.h"
#include "../include/script.h"
#include "../include/world.h"

static BSP_Map bsp = { 0 };
static BSP_Renderer bspRenderer = { 0 };
static Vector2 lookRotation = { 0 };
static Camera camera = { 0 };
static Args args;
static bool mapLoaded = false;
static bool ignoreMouseDelta = false;
static bool insideNextLevel = false;
static Color skyColor = { 0, 0, 0, 255 };
GameState gameState = GAME_STATE_TITLE;
static GameState oldGameState = GAME_STATE_TITLE;
static RenderTexture2D screenTarget = { 0 };
static int targetWidth = 0;
static int targetHeight = 0;
static Vector2 mouseScale = { 1.0f, 1.0f };

static void UpdateTargetFps(void)
{
    static int appliedFps = 0;

    if (appliedFps == Settings_MaxFps())
        return;

    appliedFps = Settings_MaxFps();
    SetTargetFPS(appliedFps);
}

static void UpdateScreenTarget(void)
{
    const int width = Settings_RenderWidth();
    const int height = Settings_RenderHeight();

    if (width <= 0 || height <= 0)
        return;

    if (width == targetWidth && height == targetHeight && screenTarget.id != 0)
        return;

    if (screenTarget.id != 0)
        UnloadRenderTexture(screenTarget);

    screenTarget = LoadRenderTexture(width, height);

    if (screenTarget.id == 0)
        return;

    SetTextureFilter(screenTarget.texture, TEXTURE_FILTER_BILINEAR);

    targetWidth = width;
    targetHeight = height;
}

static Rectangle ScreenTargetDestination(void)
{
    const float windowWidth = (float)GetScreenWidth();
    const float windowHeight = (float)GetScreenHeight();

    float scale;

    if (targetWidth <= 0 || targetHeight <= 0)
        return (Rectangle){ 0.0f, 0.0f, windowWidth, windowHeight };

    if (Settings_Stretch())
        return (Rectangle){ 0.0f, 0.0f, windowWidth, windowHeight };

    scale = fminf(windowWidth / (float)targetWidth, windowHeight / (float)targetHeight);

    return (Rectangle){
        (windowWidth - targetWidth * scale) * 0.5f,
        (windowHeight - targetHeight * scale) * 0.5f,
        targetWidth * scale,
        targetHeight * scale
    };
}

static void UnloadMap(void)
{
    if (!mapLoaded)
        return;

    BSP_RendererShutdown(&bspRenderer);
    BSP_Free(&bsp);

    mapLoaded = false;
}

static bool LoadMap(const char *path)
{
    BSP_Map loaded = { 0 };
    BSP_Renderer loadedRenderer = { 0 };

    if (path == 0 || path[0] == '\0')
        return false;

    if (!BSP_Load(&loaded, path))
        return false;

    if (!BSP_RendererInit(&loadedRenderer, &loaded))
    {
        BSP_Free(&loaded);
        return false;
    }

    UnloadMap();

    bsp = loaded;
    bspRenderer = loadedRenderer;
    mapLoaded = true;
    insideNextLevel = true;

    lookRotation.x = 0;
    lookRotation.y = 0;
    ignoreMouseDelta = true;

    Vector3 spawn = { 0.0f, 0.0f, 0.0f };

    Collision_GetSpawn(&bsp, &spawn);
    World_Reset(spawn);

    float spawnYaw;

    if (Collision_GetSpawnYaw(&bsp, &spawnYaw))
        lookRotation.x = spawnYaw;

    unsigned char sr;
    unsigned char sg;
    unsigned char sb;

    if (BSP_GetSunlightColor(&bsp, &sr, &sg, &sb))
    {
        skyColor.r = sr;
        skyColor.g = sg;
        skyColor.b = sb;
    }

    char mapName[BSP_MAP_NAME_SIZE];

    if (BSP_GetMapName(&bsp, mapName, (int)sizeof(mapName)))
        MapName_Show(mapName);
    else
        MapName_Show(0);

    Body view = World_ViewBody();

    InitCamera(&camera, &view);

    return true;
}

static void UpdateMouseMapping(void)
{
    const Rectangle dest = ScreenTargetDestination();

    mouseScale.x = (dest.width > 0.0f) ? targetWidth / dest.width : 1.0f;
    mouseScale.y = (dest.height > 0.0f) ? targetHeight / dest.height : 1.0f;

    SetMouseOffset((int)-dest.x, (int)-dest.y);
    SetMouseScale(mouseScale.x, mouseScale.y);
}

static void ApplyCursorMode(void)
{
    if (gameState != oldGameState)
    {
        switch (gameState)
        {
        case GAME_STATE_TITLE:
        case GAME_STATE_MENU:
        case GAME_STATE_PAUSE:
            EnableCursor();
            break;

        case GAME_STATE_GAME:
            DisableCursor();
            ignoreMouseDelta = true;
            break;
        }

        oldGameState = gameState;
    }
}

static void RunState(InputState in, float delta)
{
    switch (gameState)
    {
    case GAME_STATE_TITLE:
        if (newGameRequested)
        {
            newGameRequested = false;

            if (LoadMap(args.mapPath))
                gameState = GAME_STATE_GAME;
        }
        break;

    case GAME_STATE_PAUSE:
        if (IsKeyPressed(KEY_ESCAPE))
            gameState = GAME_STATE_GAME;
        break;

    case GAME_STATE_MENU:
        break;

    case GAME_STATE_GAME:
        if (IsKeyPressed(KEY_ESCAPE))
        {
            gameState = GAME_STATE_PAUSE;
            World_Pause();
            break;
        }

        if (ignoreMouseDelta)
        {
            in.mouse.x = 0;
            in.mouse.y = 0;
            ignoreMouseDelta = false;
        }

        if (mouseScale.x != 0.0f)
            in.mouse.x /= mouseScale.x;

        if (mouseScale.y != 0.0f)
            in.mouse.y /= mouseScale.y;

        lookRotation.x -= in.mouse.x * Settings_Sensitivity();
        lookRotation.y += in.mouse.y * Settings_Sensitivity();
        lookRotation.y = Clamp(lookRotation.y, -CAMERA_MAX_PITCH, CAMERA_MAX_PITCH);

        World_Update(&bsp, in, lookRotation.x, delta);

        {
            Body view = World_ViewBody();

            Camera_Update(&camera, &view, lookRotation);
        }

        char nextMap[BSP_MAP_NAME_SIZE];

        if (!Collision_CheckNextLevel(&bsp, World_Body()->position, World_Body()->isCrouched, nextMap, (int)sizeof(nextMap)))
        {
            insideNextLevel = false;
        }
        else if (!insideNextLevel)
        {
            char nextPath[ARGS_MAP_PATH_MAX];

            insideNextLevel = true;

            if (Args_ResolveMapPath(nextPath, (int)sizeof(nextPath), nextMap))
                LoadMap(nextPath);
        }
        break;
    }
}

static void DrawWorldAndUi(float delta)
{
    BeginTextureMode(screenTarget);
        ClearBackground(((gameState == GAME_STATE_GAME || gameState == GAME_STATE_PAUSE) && mapLoaded) ? skyColor : BLACK);

        switch (gameState)
        {
        case GAME_STATE_TITLE:
            Title_Draw();
            break;

        case GAME_STATE_PAUSE:
            if (mapLoaded)
            {
                BeginMode3D(camera);
                    BSP_Render(&bspRenderer, &bsp, camera, (float)targetWidth / (float)targetHeight);
                EndMode3D();
            }

            Pause_Draw();
            break;

        case GAME_STATE_MENU:
            Settings_Draw();
            break;

        case GAME_STATE_GAME:
            if (mapLoaded)
            {
                BeginMode3D(camera);
                    BSP_Render(&bspRenderer, &bsp, camera, (float)targetWidth / (float)targetHeight);
                EndMode3D();

                MapName_Draw(delta);
            }
            break;
        }

    EndTextureMode();
}

static void PresentFrame(void)
{
    BeginDrawing();
        ClearBackground(BLACK);

        DrawTexturePro(
            screenTarget.texture,
            (Rectangle){ 0.0f, 0.0f, (float)targetWidth, (float)-targetHeight },
            ScreenTargetDestination(),
            (Vector2){ 0.0f, 0.0f },
            0.0f,
            WHITE
        );
    EndDrawing();
}

int main(int argc, char **argv)
{

    if (!Args_Parse(&args, argc, argv))
        return 1;

    Cvar_Init();
    Script_Init();

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 960, "javardtex");
    SetExitKey(KEY_NULL);

    Settings_Init();
    SetTargetFPS(Settings_MaxFps());

    SetWindowSize(Settings_NativeWidth(), Settings_NativeHeight());

    if (!IsWindowFullscreen())
        ToggleFullscreen();

    Fonts_Load();

    if (args.hasMap)
    {
        if (!LoadMap(args.mapPath))
        {
            Fonts_Unload();
            Script_Shutdown();
            CloseWindow();
            return 1;
        }

        gameState = GAME_STATE_GAME;
    }

    while (!WindowShouldClose() && !quitRequested)
    {
        UpdateTargetFps();
        UpdateScreenTarget();
        UpdateMouseMapping();
        ApplyCursorMode();

        InputState in = GetInputState();
        const float delta = GetFrameTime();

        RunState(in, delta);
        DrawWorldAndUi(delta);
        PresentFrame();
    }

    if (screenTarget.id != 0)
        UnloadRenderTexture(screenTarget);

    Fonts_Unload();
    Script_Shutdown();

    UnloadMap();

    CloseWindow();

    return 0;
}
