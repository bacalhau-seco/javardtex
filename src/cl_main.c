#include "../include/defs.h"
#include "../include/title.h"

static Body player = { 0 };
static Vector2 lookRotation = { 0 };
static float headLerp;
GameState gameState = GAME_STATE_TITLE;
static GameState oldGameState = GAME_STATE_TITLE;

int main(void)
{
    headLerp = sv_standheight.value;

    const int screenWidth = cl_hres.value;
    const int screenHeight = cl_vres.value;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "javardtex");

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
            lookRotation.x -= in.mouse.x * cl_mouseh.value;
            lookRotation.y += in.mouse.y * cl_mousev.value;

            UpdatePlayer(&player, lookRotation.x, in.side, in.forward, in.jump, in.crouch, delta);

            headLerp = Lerp(
                headLerp,
                in.crouch ? sv_crouchheight.value : sv_standheight.value,
                20.0f * delta
            );

            camera.position = (Vector3){
                player.position.x,
                player.position.y + (sv_bottomheight.value + headLerp),
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
                break;
            }

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
