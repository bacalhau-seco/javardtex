#include "../include/defs.h"
#include "../include/title.h"
#include "../include/settings.h"

#define SETTINGS_LABEL_SIZE 20
#define SETTINGS_ROW_HEIGHT 34
#define SETTINGS_ROW_SPACING 8
#define SETTINGS_LABEL_WIDTH 220
#define SETTINGS_KEY_WIDTH 200

static int capturingBind = -1;

static inline int RowsTop(void)
{
    const int rowsHeight = BIND_COUNT * (SETTINGS_ROW_HEIGHT + SETTINGS_ROW_SPACING);

    return GetScreenHeight() / 2 - rowsHeight / 2;
}

static void UpdateCapture(void)
{
    int key;

    if (capturingBind < 0)
        return;

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
    {
        capturingBind = -1;
        return;
    }

    key = Input_PollBindKey();

    if (key == KEY_NULL)
        return;

    if (key != KEY_ESCAPE)
        Input_SetBindKey((InputBind)capturingBind, key);

    capturingBind = -1;
}

static void DrawBindRow(int bind, int centerX, int y)
{
    const Rectangle keyButton = {
        centerX + SETTINGS_LABEL_WIDTH / 2 - SETTINGS_KEY_WIDTH / 2 + 20,
        y,
        SETTINGS_KEY_WIDTH,
        SETTINGS_ROW_HEIGHT
    };

    const char *keyText = capturingBind == bind
        ? "..."
        : Input_KeyName(Input_GetBindKey((InputBind)bind));

    Menu_DrawText(
        Fonts_Get(FONT_UI),
        Input_BindLabel((InputBind)bind),
        centerX - SETTINGS_LABEL_WIDTH / 2,
        y + (SETTINGS_ROW_HEIGHT - SETTINGS_LABEL_SIZE) / 2,
        SETTINGS_LABEL_SIZE,
        BLACK
    );

    if (!Menu_DrawButton(keyButton, keyText))
        return;

    if (capturingBind < 0)
        capturingBind = bind;
}

void Settings_Open(void)
{
    capturingBind = -1;
}

void Settings_Draw(void)
{
    const int centerX = GetScreenWidth() / 2;
    const int top = RowsTop();

    const Rectangle backButton = {
        centerX - BUTTON_WIDTH / 2,
        top + BIND_COUNT * (SETTINGS_ROW_HEIGHT + SETTINGS_ROW_SPACING) + BUTTON_SPACING * 2,
        BUTTON_WIDTH,
        BUTTON_HEIGHT
    };

    int i;

    UpdateCapture();

    Menu_DrawText(Fonts_Get(FONT_MENU), "SETTINGS", centerX, top - 80, 40, BLACK);

    for (i = 0; i < BIND_COUNT; i++)
        DrawBindRow(i, centerX, top + i * (SETTINGS_ROW_HEIGHT + SETTINGS_ROW_SPACING));

    if (!Menu_DrawButton(backButton, "BACK"))
        return;

    if (capturingBind >= 0)
    {
        capturingBind = -1;
        return;
    }

    gameState = GAME_STATE_TITLE;
}
