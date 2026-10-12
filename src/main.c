#include "../include/defs.h"
#include "../include/title.h"
#include "../include/bsp.h"
#include "../include/render.h"
#include "../include/collision.h"

static Body player = { 0 };
static BSP_Map bsp = { 0 };
static BSP_Renderer bspRenderer = { 0 };
static Vector2 lookRotation = { 0 };
GameState gameState = GAME_STATE_TITLE;
static GameState oldGameState = GAME_STATE_TITLE;

int main(void)
{
    Cvar_Init();

    const int screenWidth = Cvar_Get("hres")->value;
    const int screenHeight = Cvar_Get("vres")->value;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "javardtex");
    SetTargetFPS((int)Cvar_Get("maxfps")->value);

    if (!BSP_Load(&bsp, "game/maps/test.bsp"))
    {
        CloseWindow();
        return 1;
    }

    if (!BSP_RendererInit(&bspRenderer, &bsp))
    {
        BSP_Free(&bsp);
        CloseWindow();
        return 1;
    }

    Vector3 spawn;

    if (Collision_GetSpawn(&bsp, &spawn))
        player.position = spawn;

    Camera camera;
    InitCamera(&camera, &player);

    while (!WindowShouldClose())
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
                break;
            }

            oldGameState = gameState;
        }

        InputState in = GetInputState();
        float delta = GetFrameTime();

        switch (gameState)
        {
        case GAME_STATE_TITLE:
            break;

        case GAME_STATE_PAUSE:
            break;

        case GAME_STATE_MENU:
            break;

        case GAME_STATE_GAME:
            lookRotation.x -= in.mouse.x * Cvar_Get("mouseh")->value;
            lookRotation.y += in.mouse.y * Cvar_Get("mousev")->value;
            lookRotation.y = Clamp(lookRotation.y, -PI/2+0.001f, PI/2-0.001f);

            UpdatePlayer(
                &bsp,
                &player,
                lookRotation.x,
                in.side,
                in.forward,
                in.jump,
                in.crouch,
                delta
            );

            Camera_Update(&camera, &player, lookRotation);
            break;
        }

        BeginDrawing();
            ClearBackground(RAYWHITE);

            switch (gameState)
            {
            case GAME_STATE_TITLE:
                Title_Draw();
                break;

            case GAME_STATE_PAUSE:
                break;

            case GAME_STATE_MENU:
                break;

            case GAME_STATE_GAME:
                BeginMode3D(camera);
                    BSP_Render(&bspRenderer);
                EndMode3D();
                break;
            }

        EndDrawing();
    }

    BSP_RendererShutdown(&bspRenderer);
    BSP_Free(&bsp);

    CloseWindow();

    return 0;
}
