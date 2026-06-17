#include "../include/defs.h"

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
static Vector2 sensitivity = { 0.001f, 0.001f };

static Body player = { 0 };
static Vector2 lookRotation = { 0 };
static float headLerp = STAND_HEIGHT;

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
static void DrawLevel(void);
static void UpdateCameraFPS(Camera *camera);
//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 1024;
    const int screenHeight = 768;
    int cx = screenWidth / 2;
    int cy = screenHeight / 2;


    InitWindow(screenWidth, screenHeight, "My Awesome CS clone");

    // Initialize camera variables
    Camera camera = { 0 };
    camera.fovy = 90.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    camera.position = (Vector3){
        player.position.x,
        player.position.y + (BOTTOM_HEIGHT + headLerp),
        player.position.z,
    };

    UpdateCameraFPS(&camera); // Update camera parameters

    DisableCursor();        // Limit cursor to relative movement inside the window

    SetTargetFPS(60);       // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        InputState in = GetInputState();

        lookRotation.x -= in.mouse.x*sensitivity.x;
        lookRotation.y += in.mouse.y*sensitivity.y;

        UpdateBody(&player,
                   lookRotation.x,
                   in.side,
                   in.forward,
                   in.jump,
                   in.crouch);

        float delta = GetFrameTime();
        headLerp = Lerp(headLerp, (in.crouch ? CROUCH_HEIGHT : STAND_HEIGHT), 20.0f*delta);
        camera.position = (Vector3){
            player.position.x,
            player.position.y + (BOTTOM_HEIGHT + headLerp),
            player.position.z,
        };

        UpdateCameraFPS(&camera);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE);

            BeginMode3D(camera);
                DrawLevel();
            EndMode3D();

            Vector3 v = player.velocity;
            Vector3 hvel = { v.x, 0.0f, v.z };

            float speed = Vector3Length(hvel);
            float wishSpeed = Vector3DotProduct(hvel, player.dir);

            DrawRectangle(5, 90, 330, 105, Fade(DARKGRAY, 0.4f));
            DrawRectangleLines(5, 90, 330, 105, DARKGRAY);

            DrawText(TextFormat("Speed: %.2f", speed), 15, 100, 10, RAYWHITE);
            DrawText(TextFormat("WishSpeed: %.2f", wishSpeed), 15, 115, 10, RAYWHITE);
            DrawText(TextFormat("Vel Y: %.2f", v.y), 15, 130, 10, RAYWHITE);
            DrawText(TextFormat("Grounded: %s", player.isGrounded ? "yes" : "no"),
                15, 145, 10, RAYWHITE);

            DrawText(TextFormat("WishDir: (%.2f, %.2f, %.2f)",
                player.dir.x, player.dir.y, player.dir.z),
                15, 160, 10, RAYWHITE);
            int size = 6;
            int gap = 3;
            int thickness = 2;

            // horizontal
            DrawLineEx((Vector2){ cx - size - gap, cy }, (Vector2){ cx - gap, cy }, thickness, GREEN);
            DrawLineEx((Vector2){ cx + gap, cy }, (Vector2){ cx + size + gap, cy }, thickness, GREEN);

            // vertical
            DrawLineEx((Vector2){ cx, cy - size - gap }, (Vector2){ cx, cy - gap }, thickness, GREEN);
            DrawLineEx((Vector2){ cx, cy + gap }, (Vector2){ cx, cy + size + gap }, thickness, GREEN);      

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------
// Update camera for FPS behaviour
static void UpdateCameraFPS(Camera *camera)
{
    const Vector3 up = (Vector3){ 0.0f, 1.0f, 0.0f };
    const Vector3 targetOffset = (Vector3){ 0.0f, 0.0f, -1.0f };

    // Left and right
    Vector3 yaw = Vector3RotateByAxisAngle(targetOffset, up, lookRotation.x);

    // Up and down
    Vector3 right = Vector3Normalize(Vector3CrossProduct(yaw, up));

    // Rotate view vector around right axis
    float pitchAngle = -lookRotation.y;
    pitchAngle = Clamp(pitchAngle, -PI/2 + 0.0001f, PI/2 - 0.0001f);

    Vector3 pitch = Vector3RotateByAxisAngle(yaw, right, pitchAngle);

    camera->up = up;
    camera->target = Vector3Add(camera->position, pitch);
}

// Draw game level
static void DrawLevel(void)
{
    const int floorExtent = 25;
    const float tileSize = 5.0f;
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

