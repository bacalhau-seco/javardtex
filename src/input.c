#include <stdio.h>

#include "../include/defs.h"

typedef struct {
    const char *label;
    int key;
} InputBindSlot;

static InputBindSlot binds[BIND_COUNT] = {
    { "MOVE FORWARD", KEY_W },
    { "MOVE BACK", KEY_S },
    { "MOVE LEFT", KEY_A },
    { "MOVE RIGHT", KEY_D },
    { "JUMP", KEY_SPACE },
    { "CROUCH", KEY_LEFT_CONTROL }
};

static inline bool IsValidBind(int bind)
{
    return bind >= 0 && bind < BIND_COUNT;
}

const char *Input_BindLabel(InputBind bind)
{
    if (!IsValidBind(bind))
        return "";

    return binds[bind].label;
}

int Input_GetBindKey(InputBind bind)
{
    if (!IsValidBind(bind))
        return KEY_NULL;

    return binds[bind].key;
}

void Input_SetBindKey(InputBind bind, int key)
{
    int i;

    if (!IsValidBind(bind))
        return;

    if (key != KEY_NULL)
    {
        for (i = 0; i < BIND_COUNT; i++)
        {
            if (i != (int)bind && binds[i].key == key)
                binds[i].key = KEY_NULL;
        }
    }

    binds[bind].key = key;
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

int Input_PollBindKey(void)
{
    float wheel = GetMouseWheelMove();

    if (wheel > 0.0f)
        return INPUT_BIND_MWHEELUP;

    if (wheel < 0.0f)
        return INPUT_BIND_MWHEELDOWN;

    return GetKeyPressed();
}

const char *Input_KeyName(int key)
{
    static char name[16];

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
        snprintf(name, sizeof(name), "F%d", key - KEY_F1 + 1);

        return name;
    }

    if (key >= KEY_KP_0 && key <= KEY_KP_9)
    {
        snprintf(name, sizeof(name), "KP %d", key - KEY_KP_0);

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

    snprintf(name, sizeof(name), "KEY %d", key);

    return name;
}

InputState GetInputState(void)
{
    InputState in = {0};

    in.mouse = GetMouseDelta();
    in.side = IsBindDown(binds[BIND_RIGHT].key) - IsBindDown(binds[BIND_LEFT].key);
    in.forward = IsBindDown(binds[BIND_FORWARD].key) - IsBindDown(binds[BIND_BACK].key);
    in.jump = IsBindPressed(binds[BIND_JUMP].key);
    in.crouch = IsBindDown(binds[BIND_CROUCH].key);

    return in;
}
