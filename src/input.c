#include <stdio.h>

#include "../include/input.h"

typedef struct {
    const char *label;
    int keys[INPUT_BIND_SLOTS];
} InputBindSlot;

static InputBindSlot binds[BIND_COUNT] = {
    { "MOVE FORWARD", { KEY_W, KEY_UP } },
    { "MOVE BACK", { KEY_S, KEY_DOWN } },
    { "MOVE LEFT", { KEY_A, KEY_LEFT } },
    { "MOVE RIGHT", { KEY_D, KEY_RIGHT } },
    { "JUMP", { KEY_SPACE, KEY_NULL } },
    { "CROUCH", { KEY_LEFT_CONTROL, KEY_NULL } }
};

static inline bool IsValidBind(int bind)
{
    return bind >= 0 && bind < BIND_COUNT;
}

static inline bool IsValidSlot(int slot)
{
    return slot >= 0 && slot < INPUT_BIND_SLOTS;
}

const char *Input_BindLabel(InputBind bind)
{
    if (!IsValidBind(bind))
        return "";

    return binds[bind].label;
}

int Input_GetBindKey(InputBind bind, int slot)
{
    if (!IsValidBind(bind) || !IsValidSlot(slot))
        return KEY_NULL;

    return binds[bind].keys[slot];
}

void Input_SetBindKey(InputBind bind, int slot, int key)
{
    int i;
    int j;

    if (!IsValidBind(bind) || !IsValidSlot(slot))
        return;

    if (key != KEY_NULL)
    {
        for (i = 0; i < BIND_COUNT; i++)
        {
            for (j = 0; j < INPUT_BIND_SLOTS; j++)
            {
                if ((i != (int)bind || j != slot) && binds[i].keys[j] == key)
                    binds[i].keys[j] = KEY_NULL;
            }
        }
    }

    binds[bind].keys[slot] = key;
}

void Input_ClearBindKey(InputBind bind, int slot)
{
    Input_SetBindKey(bind, slot, KEY_NULL);
}

static bool IsBindDown(int key)
{
    if (key == INPUT_BIND_MWHEELUP)
        return GetMouseWheelMove() > 0.0f;

    if (key == INPUT_BIND_MWHEELDOWN)
        return GetMouseWheelMove() < 0.0f;

    return IsKeyDown(key);
}

static inline bool IsBindPressed(int key)
{
    if (key == INPUT_BIND_MWHEELUP || key == INPUT_BIND_MWHEELDOWN)
        return IsBindDown(key);

    return IsKeyPressed(key);
}

static bool IsBindActionDown(InputBind bind)
{
    int slot;

    for (slot = 0; slot < INPUT_BIND_SLOTS; slot++)
    {
        if (binds[bind].keys[slot] != KEY_NULL && IsBindDown(binds[bind].keys[slot]))
            return true;
    }

    return false;
}

static bool IsBindActionPressed(InputBind bind)
{
    int slot;

    for (slot = 0; slot < INPUT_BIND_SLOTS; slot++)
    {
        if (binds[bind].keys[slot] != KEY_NULL && IsBindPressed(binds[bind].keys[slot]))
            return true;
    }

    return false;
}

int Input_PollBindKey(void)
{
    float wheel = GetMouseWheelMove();

    if (wheel > 0.0f)
        return INPUT_BIND_MWHEELUP;

    if (wheel < 0.0f)
        return INPUT_BIND_MWHEELDOWN;

    return GetKeyPressed();
}

#define INPUT_NAME_BUFFERS 4
#define INPUT_NAME_SIZE 16

const char *Input_KeyName(int key)
{
    static char names[INPUT_NAME_BUFFERS][INPUT_NAME_SIZE];
    static int next = 0;

    char *name = names[next];

    next = (next + 1) % INPUT_NAME_BUFFERS;

    if (key == KEY_NULL)
        return "---";

    if (key == INPUT_BIND_MWHEELUP)
        return "MWHEELUP";

    if (key == INPUT_BIND_MWHEELDOWN)
        return "MWHEELDOWN";

    if ((key >= KEY_A && key <= KEY_Z) || (key >= KEY_ZERO && key <= KEY_NINE))
    {
        name[0] = (char)key;
        name[1] = '\0';

        return name;
    }

    if (key >= KEY_F1 && key <= KEY_F12)
    {
        snprintf(name, INPUT_NAME_SIZE, "F%d", key - KEY_F1 + 1);

        return name;
    }

    if (key >= KEY_KP_0 && key <= KEY_KP_9)
    {
        snprintf(name, INPUT_NAME_SIZE, "KP %d", key - KEY_KP_0);

        return name;
    }

    switch (key)
    {
    case KEY_SPACE: return "SPACE";
    case KEY_TAB: return "TAB";
    case KEY_ENTER: return "ENTER";
    case KEY_KP_ENTER: return "KP ENTER";
    case KEY_BACKSPACE: return "BACKSPACE";
    case KEY_LEFT_SHIFT: return "LSHIFT";
    case KEY_RIGHT_SHIFT: return "RSHIFT";
    case KEY_LEFT_CONTROL: return "LCTRL";
    case KEY_RIGHT_CONTROL: return "RCTRL";
    case KEY_LEFT_ALT: return "LALT";
    case KEY_RIGHT_ALT: return "RALT";
    case KEY_LEFT: return "LEFT";
    case KEY_RIGHT: return "RIGHT";
    case KEY_UP: return "UP";
    case KEY_DOWN: return "DOWN";
    case KEY_COMMA: return ",";
    case KEY_PERIOD: return ".";
    case KEY_SLASH: return "/";
    case KEY_SEMICOLON: return ";";
    case KEY_APOSTROPHE: return "'";
    case KEY_LEFT_BRACKET: return "[";
    case KEY_RIGHT_BRACKET: return "]";
    case KEY_MINUS: return "-";
    case KEY_EQUAL: return "=";
    case KEY_GRAVE: return "`";
    case KEY_INSERT: return "INS";
    case KEY_DELETE: return "DEL";
    case KEY_HOME: return "HOME";
    case KEY_END: return "END";
    case KEY_PAGE_UP: return "PGUP";
    case KEY_PAGE_DOWN: return "PGDN";
    default: break;
    }

    snprintf(name, INPUT_NAME_SIZE, "KEY %d", key);

    return name;
}

InputState GetInputState(void)
{
    InputState in = {0};

    in.mouse = GetMouseDelta();
    in.side = IsBindActionDown(BIND_RIGHT) - IsBindActionDown(BIND_LEFT);
    in.forward = IsBindActionDown(BIND_FORWARD) - IsBindActionDown(BIND_BACK);
    in.jump = IsBindActionPressed(BIND_JUMP);
    in.crouch = IsBindActionDown(BIND_CROUCH);

    return in;
}
