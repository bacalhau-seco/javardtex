#include "../include/title.h"
#include "../include/settings.h"
#include "../include/bsp.h"
#include "../include/script.h"

#include <stdio.h>

static char mapNameText[BSP_MAP_NAME_SIZE] = { 0 };
static float mapNameTimer = 0.0f;

void MapName_Show(const char *name)
{
    const UI_MapTitleConfig *config = &UI_Get()->mapTitle;

    mapNameText[0] = '\0';
    mapNameTimer = 0.0f;

    if (!name || name[0] == '\0')
        return;

    snprintf(mapNameText, sizeof(mapNameText), "%s", name);
    mapNameTimer = config->fadeTime * 2.0f + config->holdTime;
}

void MapName_Draw(float delta)
{
    const UI_MapTitleConfig *config = &UI_Get()->mapTitle;

    if (mapNameText[0] == '\0' || mapNameTimer <= 0.0f)
        return;

    const float totalTime = config->fadeTime * 2.0f + config->holdTime;

    mapNameTimer -= delta;

    if (mapNameTimer <= 0.0f)
    {
        mapNameTimer = 0.0f;
        mapNameText[0] = '\0';
        return;
    }

    float fade = 1.0f;

    if (config->fadeTime > 0.0f)
    {
        const float fadeIn = (totalTime - mapNameTimer) / config->fadeTime;
        const float fadeOut = mapNameTimer / config->fadeTime;

        fade = (fadeIn < fadeOut) ? fadeIn : fadeOut;

        if (fade > 1.0f)
            fade = 1.0f;
        if (fade < 0.0f)
            fade = 0.0f;
    }

    const float size = Menu_Height(config->textPct);

    Color color = config->color;
    color.a = (unsigned char)(fade * (float)config->color.a);

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
    const UI_ButtonConfig *config = &UI_Get()->button;

    const bool hovered = Menu_IsHovered(button);

    const float textSize = Menu_Height(config->textPct);

    DrawRectangleRec(button, hovered ? config->hoverFill : config->fill);
    DrawRectangleLinesEx(button, Menu_Height(config->borderPct), config->border);

    Menu_DrawText(
        Fonts_Get(FONT_MENU),
        text,
        button.x + button.width / 2,
        button.y + (button.height - textSize) / 2,
        textSize,
        config->text
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

    const UI_Config *ui = UI_Get();

    const float buttonWidth = Menu_Width(ui->button.widthPct);
    const float buttonHeight = Menu_Height(ui->button.heightPct);
    const float buttonSpacing = Menu_Height(ui->button.spacingPct);
    const float titleSize = Menu_Height(ui->gameTitle.textPct);
    const float titleOffset = Menu_Height(ui->gameTitle.offsetPct);

    const float menuHeight = buttonCount * buttonHeight + (buttonCount - 1) * buttonSpacing;
    const float menuY = centerY - menuHeight / 2.0f + titleOffset;

    Menu_DrawText(Fonts_Get(FONT_MENU), ui->gameTitle.text, centerX, menuY - titleSize - titleOffset, titleSize, ui->gameTitle.color);

    DrawTextEx(
        Fonts_Get(FONT_UI),
        VERSION,
        (Vector2){ Menu_Width(ui->version.marginPct), GetRenderHeight() - Menu_Height(ui->version.marginPct) - Menu_Height(ui->version.textPct) },
        Menu_Height(ui->version.textPct),
        1.0f,
        ui->version.pauseTextColor
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

    const UI_Config *ui = UI_Get();

    const float buttonWidth = Menu_Width(ui->button.widthPct);
    const float buttonHeight = Menu_Height(ui->button.heightPct);
    const float buttonSpacing = Menu_Height(ui->button.spacingPct);
    const float titleSize = Menu_Height(ui->gameTitle.textPct);
    const float titleOffset = Menu_Height(ui->gameTitle.offsetPct);

    const float menuHeight = buttonCount * buttonHeight + (buttonCount - 1) * buttonSpacing;
    const float menuY = centerY - menuHeight / 2.0f + titleOffset;

    Menu_DrawText(Fonts_Get(FONT_MENU), ui->gameTitle.text, centerX, menuY - titleSize - titleOffset, titleSize, ui->gameTitle.color);
    DrawTextEx(
        Fonts_Get(FONT_UI),
        VERSION,
        (Vector2){ Menu_Width(ui->version.marginPct), GetRenderHeight() - Menu_Height(ui->version.marginPct) - Menu_Height(ui->version.textPct) },
        Menu_Height(ui->version.textPct),
        1.0f,
        ui->version.textColor
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
