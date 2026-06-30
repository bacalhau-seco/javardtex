#include "../include/defs.h"

InputState GetInputState(void)
{
    InputState in = {0};

    in.mouse = GetMouseDelta();
    in.side = IsKeyDown(KEY_D) - IsKeyDown(KEY_A);
    in.forward = IsKeyDown(KEY_W) - IsKeyDown(KEY_S);
    in.jump = IsKeyPressed(KEY_SPACE);
    in.crouch = IsKeyDown(KEY_LEFT_CONTROL);

    return in;
}
