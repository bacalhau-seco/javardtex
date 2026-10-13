#include "../include/defs.h"

#define BUTTON_WIDTH 260
#define BUTTON_HEIGHT 44
#define BUTTON_SPACING 12
#define BUTTON_TEXT_SIZE 24

static void DrawTitleText(Font font, const char *text, int centerX, int y, float size, Color color)
{
    const Vector2 textSize = MeasureTextEx(font, text, size, 1.0f);

    const Vector2 position = {
        centerX - textSize.x / 2.0f,
        y
    };

    DrawTextEx(font, text, position, size, 1.0f, color);
}

static bool DrawMenuButton(Rectangle button, const char *text)
{
    const bool hovered = CheckCollisionPointRec(GetMousePosition(), button);

    DrawRectangleRec(button, hovered ? GRAY : LIGHTGRAY);
    DrawRectangleLinesEx(button, 2.0f, DARKGRAY);

    DrawTitleText(
        Fonts_Get(FONT_MENU),
        text,
        button.x + button.width / 2,
        button.y + (button.height - BUTTON_TEXT_SIZE) / 2,
        BUTTON_TEXT_SIZE,
        BLACK
    );

    return (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT));
}

void Title_Draw(void)
{
    enum {
        BUTTON_NEW_GAME,
        BUTTON_LOAD_GAME,
        BUTTON_SETTINGS,
        BUTTON_QUIT,
        BUTTON_COUNT
    };

    static const char *buttons[BUTTON_COUNT] = {
        "START NEW GAME",
        "LOAD GAME",
        "SETTINGS",
        "QUIT GAME"
    };

    const int buttonCount = BUTTON_COUNT;

    const int centerX = GetScreenWidth() / 2;
    const int centerY = GetScreenHeight() / 2;

    const int menuHeight = buttonCount * BUTTON_HEIGHT + (buttonCount - 1) * BUTTON_SPACING;
    const int menuY = centerY - menuHeight / 2 + 40;

    DrawTitleText(Fonts_Get(FONT_MENU), GAMENAME, centerX, menuY - 130, 48, BLACK);
    DrawTitleText(Fonts_Get(FONT_UI), VERSION, centerX, menuY - 70, 20, DARKGRAY);

    for (int i = 0; i < buttonCount; i++)
    {
        Rectangle button = {
            centerX - BUTTON_WIDTH / 2,
            menuY + i * (BUTTON_HEIGHT + BUTTON_SPACING),
            BUTTON_WIDTH,
            BUTTON_HEIGHT
        };

        if (!DrawMenuButton(button, buttons[i]))
            continue;

        switch (i)
        {
        case BUTTON_NEW_GAME:
            newGameRequested = true;
            break;

        case BUTTON_QUIT:
            quitRequested = true;
            break;

        case BUTTON_LOAD_GAME:
        case BUTTON_SETTINGS:
            break;
        }
    }
}
