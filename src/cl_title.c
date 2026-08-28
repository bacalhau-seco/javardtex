#include "../include/defs.h"

void CL_Title(void)
{
    const int titleSize = 48;
    const int versionSize = 20;

    const int titleWidth = MeasureText(GAMENAME, titleSize);
    const int versionWidth = MeasureText(VERSION, versionSize);

    const int centerX = GetScreenWidth() / 2;
    const int centerY = GetScreenHeight() / 2;

    DrawText(
        GAMENAME,
        centerX - titleWidth / 2,
        centerY - 100,
        titleSize,
        BLACK
    );

    DrawText(
        VERSION,
        centerX - versionWidth / 2,
        centerY - 40,
        versionSize,
        BLACK
    );

    Rectangle playButton = {
        centerX - 100,
        centerY + 30,
        200,
        50
    };

    DrawRectangleRec(playButton, LIGHTGRAY);

    const char *playText = "PLAY";
    const int playWidth = MeasureText(playText, 20);

    DrawText(
        playText,
        centerX - playWidth / 2,
        centerY + 45,
        20,
        BLACK
    );

    if (CheckCollisionPointRec(GetMousePosition(), playButton))
    {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            gameState = GAME_STATE_GAME;
    }
}
