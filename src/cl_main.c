#include "../include/defs.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>

static Body player = { 0 };
static Vector2 lookRotation = { 0 };
static float headLerp;
static void DrawLevel(void);

int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        if (strcmp(argv[1], "-game") == 0) {
            DIR* dir = opendir(argv[2]);
            if (dir) {
                printf("game selected: %s\n", argv[2]);
                closedir(dir);
            }
            else {
                printf("game doesn't exist\n");
                // return 0;
            }
        }
        else {
            printf("no game defined\n");
            // return 0;
        }
    }

    // Initialization
    headLerp = sv_standheight.value;
    const int screenWidth = cl_hres.value;
    const int screenHeight = cl_vres.value;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "javardtex");

    DisableCursor();

    Camera camera;
    InitCamera(&camera, &player, headLerp);

    // Main game loop
    while (!WindowShouldClose())
    {
        InputState in = GetInputState();

        lookRotation.x -= in.mouse.x*cl_mouseh.value;
        lookRotation.y += in.mouse.y*cl_mousev.value;

        UpdateBody(&player,
                   lookRotation.x,
                   in.side,
                   in.forward,
                   in.jump,
                   in.crouch);

        float delta = GetFrameTime();
        headLerp = Lerp(headLerp, (in.crouch ? sv_crouchheight.value : sv_standheight.value), 20.0f*delta);
        camera.position = (Vector3){
            player.position.x,
            player.position.y + (sv_bottomheight.value + headLerp),
            player.position.z,
        };

        Camera_Update(&camera, &player, lookRotation, headLerp);

        // Draw
        BeginDrawing();
            ClearBackground(RAYWHITE);
            BeginMode3D(camera);
                DrawLevel();
            EndMode3D();
        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    CloseWindow();

    return 0;
}

// Draw game level
static void DrawLevel(void)
{
    const int floorExtent = 25;
    const float tileSize = 64.0f;
    const Color tileColor1 = (Color){ 150, 200, 200, 255 };

    // Floor tiles
    for (int y = -floorExtent; y < floorExtent; y++)
    {
        for (int x = -floorExtent; x < floorExtent; x++)
        {
            if ((y & 1) && (x & 1))
            {
                DrawPlane((Vector3){ x*tileSize, 0.0f, y*tileSize}, (Vector2){ tileSize, tileSize }, tileColor1);
            }
            else if (!(y & 1) && !(x & 1))
            {
                DrawPlane((Vector3){ x*tileSize, 0.0f, y*tileSize}, (Vector2){ tileSize, tileSize }, LIGHTGRAY);
            }
        }
    }
}

