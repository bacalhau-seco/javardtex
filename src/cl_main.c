#include "../include/defs.h"
#include "../include/title.h"
#include "../include/bsp.h"
#include "../include/render.h"

static Body player = { 0 };
static BSP_Map bsp = { 0 };
static BSP_Renderer bspRenderer = { 0 };
static Vector2 lookRotation = { 0 };
static float headLerp;
GameState gameState = GAME_STATE_TITLE;
static GameState oldGameState = GAME_STATE_TITLE;

int main(void)
{
    Cvar_Init();
    headLerp = Cvar_Get("sv_standheight")->value;

    const int screenWidth = Cvar_Get("cl_hres")->value;
    const int screenHeight = Cvar_Get("cl_vres")->value;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "javardtex");

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

    if (BSP_GetPlayerStart(&bsp, &spawn.x, &spawn.y, &spawn.z))
    {
        player.position.x = spawn.x;
        player.position.y = spawn.z;
        player.position.z = -spawn.y;
    }

    Camera camera;
    InitCamera(&camera, &player, headLerp);

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
            lookRotation.x -= in.mouse.x * Cvar_Get("cl_mouseh")->value;
            lookRotation.y += in.mouse.y * Cvar_Get("cl_mousev")->value;

            UpdatePlayer(
                &player,
                lookRotation.x,
                in.side,
                in.forward,
                in.jump,
                in.crouch,
                delta
            );

            headLerp = Lerp(
                headLerp,
                in.crouch
                    ? Cvar_Get("sv_crouchheight")->value
                    : Cvar_Get("sv_standheight")->value,
                20.0f * delta
            );

            camera.position = (Vector3){
                player.position.x,
                player.position.y + (Cvar_Get("sv_bottomheight")->value + headLerp),
                player.position.z
            };

            Camera_Update(&camera, &player, lookRotation, headLerp);
            break;
        }

        BeginDrawing();
            ClearBackground(RAYWHITE);

            switch (gameState)
            {
            case GAME_STATE_TITLE:
                CL_Title();
                break;

            case GAME_STATE_PAUSE:
                break;

            case GAME_STATE_MENU:
                break;

            case GAME_STATE_GAME:
                BeginMode3D(camera);
                    BSP_Render(&bsp, &bspRenderer);
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
