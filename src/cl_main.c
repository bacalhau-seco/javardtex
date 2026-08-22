#include "../include/defs.h"

static Body player = { 0 };
static Vector2 lookRotation = { 0 };
static float headLerp;

int main(void)
{
    headLerp = sv_standheight.value;

    const int screenWidth = cl_hres.value;
    const int screenHeight = cl_vres.value;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "javardtex");

    DisableCursor();

    Camera camera;
    InitCamera(&camera, &player, headLerp);

    while (!WindowShouldClose())
    {
        InputState in = GetInputState();

        lookRotation.x -= in.mouse.x * cl_mouseh.value;
        lookRotation.y += in.mouse.y * cl_mousev.value;

        float delta = GetFrameTime();

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

        BeginDrawing();
            ClearBackground(RAYWHITE);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
