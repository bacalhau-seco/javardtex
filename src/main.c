#include "raylib.h"
#include "raymath.h"
#include <stdio.h>

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
// Movement constants
#define GRAVITY         32.0f
#define MAX_SPEED       20.0f
#define JUMP_FORCE      12.0f
#define FRICTION         0.85f
#define STAND_HEIGHT     1.0f
#define BOTTOM_HEIGHT    0.5f
#define ACCEL           90.0f
#define AIR_ACCEL       50.0f 

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
// Body structure
typedef struct {
    Vector3 position;
    Vector3 velocity;
    Vector3 dir;
    bool isGrounded;
} Body;

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
static void UpdateBody(Body *body, float rot, char side, char forward, bool jumpPressed); //, bool crouchHold

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


    InitWindow(screenWidth, screenHeight, "raylib [core] example - 3d camera fps");

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
        // Update
        //----------------------------------------------------------------------------------
        Vector2 mouseDelta = GetMouseDelta();
        lookRotation.x -= mouseDelta.x*sensitivity.x;
        lookRotation.y += mouseDelta.y*sensitivity.y;

        char sideway = (IsKeyDown(KEY_D) - IsKeyDown(KEY_A));
        char forward = (IsKeyDown(KEY_W) - IsKeyDown(KEY_S));
        //bool crouching = IsKeyDown(KEY_LEFT_CONTROL);
        UpdateBody(&player, lookRotation.x, sideway, forward, IsKeyPressed(KEY_SPACE)); //, crouching

        float delta = GetFrameTime();
        headLerp = Lerp(headLerp, (/*crouching ? CROUCH_HEIGHT : */STAND_HEIGHT), 20.0f*delta);
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

    const Vector3 towerSize = (Vector3){ 16.0f, 32.0f, 16.0f };
    const Color towerColor = (Color){ 150, 200, 200, 255 };

    Vector3 towerPos = (Vector3){ 16.0f, 16.0f, 16.0f };
    DrawCubeV(towerPos, towerSize, towerColor);
    DrawCubeWiresV(towerPos, towerSize, DARKBLUE);

    towerPos.x *= -1;
    DrawCubeV(towerPos, towerSize, towerColor);
    DrawCubeWiresV(towerPos, towerSize, DARKBLUE);

    towerPos.z *= -1;
    DrawCubeV(towerPos, towerSize, towerColor);
    DrawCubeWiresV(towerPos, towerSize, DARKBLUE);

    towerPos.x *= -1;
    DrawCubeV(towerPos, towerSize, towerColor);
    DrawCubeWiresV(towerPos, towerSize, DARKBLUE);

    // Red sun
    DrawSphere((Vector3){ 300.0f, 300.0f, 0.0f }, 100.0f, (Color){ 255, 0, 0, 255 });
}

// Update body considering current world state
void UpdateBody(Body *body, float rot, char side, char forward, bool jumpPressed) //, bool crouchHold
{
    Vector2 input = (Vector2){ (float)side, (float)-forward }; // side = horizontal input / forward = vertical input
    if ((side != 0) && (forward != 0)) input = Vector2Normalize(input); // normalizes input

    float delta = GetFrameTime();

    if (!body->isGrounded) body->velocity.y -= GRAVITY*delta;

    if (body->isGrounded && jumpPressed)
    {
        body->velocity.y = JUMP_FORCE;
        body->isGrounded = false;
    }
    Vector3 front = { sinf(rot), 0.f, cosf(rot) };
    Vector3 right = { cosf(rot), 0.f, -sinf(rot) };

    Vector3 wishDir = (Vector3){ input.x*right.x + input.y*front.x, 0.0f, input.x*right.z + input.y*front.z, }; // essentially "wishDir" from quake
    wishDir = Vector3Normalize(wishDir);
    body->dir = wishDir;
    
    Vector3 hvel = {
        body->velocity.x,
        0.0f,
        body->velocity.z
    };

    // RUNS ON GROUND
    if (body->isGrounded)
    {
        float decel = FRICTION;
        hvel = (Vector3){ body->velocity.x*decel, 0.0f, body->velocity.z*decel };
        float currentSpeed = Vector3DotProduct(hvel, body->dir);
        float addSpeed = MAX_SPEED - currentSpeed;
        float accel = ACCEL * delta;
        if (Vector3Length(hvel) <= 0.1) { hvel = (Vector3) {0}; }
        if (Vector3Length(wishDir) == 0.0f){ accel = 0; }
        if (accel > addSpeed)
            accel = addSpeed;
        hvel.x += body->dir.x * accel;
        hvel.z += body->dir.z * accel;
        body->velocity.x = hvel.x;
        body->velocity.z = hvel.z;
    }
    // RUNS ON AIR
    else {
        // does the same as VectorNormalize() in quake's code base
        Vector3 wishveloc = Vector3Scale(body->dir, MAX_SPEED);
        float wishspd = Vector3Length(wishveloc);
        wishveloc = Vector3Normalize(wishveloc);

        // limits the wishspd size
        if (wishspd > 1)
            wishspd = 1;

        float currentSpeed = Vector3DotProduct(hvel, wishveloc);
        float addSpeed = wishspd - currentSpeed;

        // prevents negative add speed
        if (addSpeed > 0)
        {
            float accel = AIR_ACCEL * wishspd * delta;

            if (accel > addSpeed)
                accel = addSpeed;

            hvel.x += wishveloc.x * accel;
            hvel.z += wishveloc.z * accel;
        }

        body->velocity.x = hvel.x;
        body->velocity.z = hvel.z;
    }
    body->position.x += body->velocity.x * delta;
    body->position.y += body->velocity.y * delta;
    body->position.z += body->velocity.z * delta;
    if (body->position.y <= 0.0f)
    {
        body->position.y = 0.0f;
        body->velocity.y = 0.0f;
        body->isGrounded = true;
    }
}
