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

#define INPUT_BIND_MWHEELUP 1000
#define INPUT_BIND_MWHEELDOWN 1001
#define INPUT_BIND_SLOTS 2

typedef enum {
    BIND_FORWARD,
    BIND_BACK,
    BIND_LEFT,
    BIND_RIGHT,
    BIND_JUMP,
    BIND_CROUCH,
    BIND_COUNT
} InputBind;

InputState GetInputState(void);

const char *Input_BindLabel(InputBind bind);
int Input_GetBindKey(InputBind bind, int slot);
void Input_SetBindKey(InputBind bind, int slot, int key);
void Input_ClearBindKey(InputBind bind, int slot);
const char *Input_KeyName(int key);
int Input_PollBindKey(void);
