#include "../include/title.h"
#include "../include/settings.h"
#include "../include/bsp.h"

#include <stdio.h>

static char mapNameText[BSP_MAP_NAME_SIZE] = { 0 };
static float mapNameTimer = 0.0f;

void MapName_Show(const char *name)
{
    mapNameText[0] = '\0';
    mapNameTimer = 0.0f;

    if (!name || name[0] == '\0')
        return;

    snprintf(mapNameText, sizeof(mapNameText), "%s", name);
    mapNameTimer = MAPNAME_TOTAL_TIME;
}

void MapName_Draw(float delta)
{
    if (mapNameText[0] == '\0' || mapNameTimer <= 0.0f)
        return;

    mapNameTimer -= delta;

    if (mapNameTimer <= 0.0f)
    {
        mapNameTimer = 0.0f;
        mapNameText[0] = '\0';
        return;
    }

    const float fadeIn = (MAPNAME_TOTAL_TIME - mapNameTimer) / MAPNAME_FADE_TIME;
    const float fadeOut = mapNameTimer / MAPNAME_FADE_TIME;
    float fade = (fadeIn < fadeOut) ? fadeIn : fadeOut;

    if (fade > 1.0f)
        fade = 1.0f;
    if (fade < 0.0f)
        fade = 0.0f;

    const float size = Menu_Height(MAPNAME_TEXT_PCT);

    Color color = RAYWHITE;
    color.a = (unsigned char)(fade * 255.0f);

    Menu_DrawText(
        Fonts_Get(FONT_MENU),
        mapNameText,
        GetRenderWidth() / 2,
        (int)(GetRenderHeight() / 2 - size / 2.0f),
        size,
        color
    );
}

float Menu_Width(float percent)
{
    return (float)GetRenderWidth() * percent;
}

float Menu_Height(float percent)
{
    return (float)GetRenderHeight() * percent;
}

void Menu_DrawText(Font font, const char *text, int centerX, int y, float size, Color color)
{
    const Vector2 textSize = MeasureTextEx(font, text, size, 1.0f);

    const Vector2 position = {
        centerX - textSize.x / 2.0f,
        y
    };

    DrawTextEx(font, text, position, size, 1.0f, color);
}

bool Menu_IsHovered(Rectangle button)
{
    return CheckCollisionPointRec(GetMousePosition(), button);
}

bool Menu_DrawButton(Rectangle button, const char *text)
{
    const bool hovered = Menu_IsHovered(button);

    const float textSize = Menu_Height(MENU_BUTTON_TEXT_PCT);

    DrawRectangleRec(button, hovered ? GRAY : LIGHTGRAY);
    DrawRectangleLinesEx(button, Menu_Height(MENU_BORDER_PCT), DARKGRAY);

    Menu_DrawText(
        Fonts_Get(FONT_MENU),
        text,
        button.x + button.width / 2,
        button.y + (button.height - textSize) / 2,
        textSize,
        BLACK
    );

    return (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT));
}

void Pause_Draw(void)
{
    enum {
        PAUSE_BUTTON_CONTINUE,
        PAUSE_BUTTON_SAVE_GAME,
        PAUSE_BUTTON_LOAD_GAME,
        PAUSE_BUTTON_SETTINGS,
        PAUSE_BUTTON_QUIT,
        PAUSE_BUTTON_COUNT
    };

    static const char *buttons[PAUSE_BUTTON_COUNT] = {
        "CONTINUE",
        "SAVE GAME",
        "LOAD GAME",
        "SETTINGS",
        "QUIT GAME"
    };

    const int buttonCount = PAUSE_BUTTON_COUNT;

    const int centerX = GetRenderWidth() / 2;
    const int centerY = GetRenderHeight() / 2;

    const float buttonWidth = Menu_Width(MENU_BUTTON_WIDTH_PCT);
    const float buttonHeight = Menu_Height(MENU_BUTTON_HEIGHT_PCT);
    const float buttonSpacing = Menu_Height(MENU_BUTTON_SPACING_PCT);

    const float menuHeight = buttonCount * buttonHeight + (buttonCount - 1) * buttonSpacing;
    const float menuY = centerY - menuHeight / 2.0f + Menu_Height(0.06f);

    Menu_DrawText(Fonts_Get(FONT_MENU), GAMENAME, centerX, menuY - Menu_Height(MENU_TITLE_TEXT_PCT) - Menu_Height(0.06f), Menu_Height(MENU_TITLE_TEXT_PCT), BLACK);

    DrawTextEx(
        Fonts_Get(FONT_UI),
        VERSION,
        (Vector2){ Menu_Width(MENU_MARGIN_PCT), GetRenderHeight() - Menu_Height(MENU_MARGIN_PCT) - Menu_Height(MENU_VERSION_TEXT_PCT) },
        Menu_Height(MENU_VERSION_TEXT_PCT),
        1.0f,
        RAYWHITE
    );

    for (int i = 0; i < buttonCount; i++)
    {
        Rectangle button = {
            centerX - buttonWidth / 2.0f,
            menuY + i * (buttonHeight + buttonSpacing),
            buttonWidth,
            buttonHeight
        };

        if (!Menu_DrawButton(button, buttons[i]))
            continue;

        switch (i)
        {
        case PAUSE_BUTTON_CONTINUE:
            gameState = GAME_STATE_GAME;
            break;

        case PAUSE_BUTTON_QUIT:
            quitRequested = true;
            break;

        case PAUSE_BUTTON_SETTINGS:
            Settings_OpenFrom(GAME_STATE_PAUSE);
            gameState = GAME_STATE_MENU;
            break;

        case PAUSE_BUTTON_SAVE_GAME:
            break;

        case PAUSE_BUTTON_LOAD_GAME:
            break;
        }
    }
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

    const int centerX = GetRenderWidth() / 2;
    const int centerY = GetRenderHeight() / 2;

    const float buttonWidth = Menu_Width(MENU_BUTTON_WIDTH_PCT);
    const float buttonHeight = Menu_Height(MENU_BUTTON_HEIGHT_PCT);
    const float buttonSpacing = Menu_Height(MENU_BUTTON_SPACING_PCT);

    const float menuHeight = buttonCount * buttonHeight + (buttonCount - 1) * buttonSpacing;
    const float menuY = centerY - menuHeight / 2.0f + Menu_Height(0.06f);

    Menu_DrawText(Fonts_Get(FONT_MENU), GAMENAME, centerX, menuY - Menu_Height(MENU_TITLE_TEXT_PCT) - Menu_Height(0.06f), Menu_Height(MENU_TITLE_TEXT_PCT), BLACK);
    DrawTextEx(
        Fonts_Get(FONT_UI),
        VERSION,
        (Vector2){ Menu_Width(MENU_MARGIN_PCT), GetRenderHeight() - Menu_Height(MENU_MARGIN_PCT) - Menu_Height(MENU_VERSION_TEXT_PCT) },
        Menu_Height(MENU_VERSION_TEXT_PCT),
        1.0f,
        BLACK
    );

    for (int i = 0; i < buttonCount; i++)
    {
        Rectangle button = {
            centerX - buttonWidth / 2.0f,
            menuY + i * (buttonHeight + buttonSpacing),
            buttonWidth,
            buttonHeight
        };

        if (!Menu_DrawButton(button, buttons[i]))
            continue;

        switch (i)
        {
        case BUTTON_NEW_GAME:
            newGameRequested = true;
            break;

        case BUTTON_QUIT:
            quitRequested = true;
            break;

        case BUTTON_SETTINGS:
            Settings_Open();
            gameState = GAME_STATE_MENU;
            break;

        case BUTTON_LOAD_GAME:
            break;
        }
    }
}
