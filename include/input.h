#pragma once

#include "raylib.h"
#include <stdbool.h>

typedef struct {
    int side;
    int forward;
    bool jump;
    bool crouch;
    Vector2 mouse;
} InputState;

InputState GetInputState(void);
