#include <raylib.h>

const int width = 640;
const int height = 480;

int main(int argc, char *argv[])
{
  InitWindow(width, height, "nigger");

  while (!WindowShouldClose()) {
    BeginDrawing();
      ClearBackground(RAYWHITE);
      BeginMode3D(Camera3D camera);
      EndMode3D();
    EndDrawing();
  }
  CloseWindow();
  
  return 0;
}
