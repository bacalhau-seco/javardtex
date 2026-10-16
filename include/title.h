#ifndef TITLE_H
#define TITLE_H

#include <stdbool.h>

#include "raylib.h"
#include "game.h"

#define MENU_BUTTON_WIDTH_PCT 0.22f
#define MENU_BUTTON_HEIGHT_PCT 0.055f
#define MENU_BUTTON_SPACING_PCT 0.015f
#define MENU_BUTTON_TEXT_PCT 0.028f
#define MENU_TITLE_TEXT_PCT 0.065f
#define MENU_VERSION_TEXT_PCT 0.022f
#define MENU_BORDER_PCT 0.0025f
#define MENU_MARGIN_PCT 0.012f
#define MAPNAME_TEXT_PCT 0.05f
#define MAPNAME_HOLD_TIME 3.0f
#define MAPNAME_FADE_TIME 1.0f
#define MAPNAME_TOTAL_TIME (MAPNAME_FADE_TIME + MAPNAME_HOLD_TIME + MAPNAME_FADE_TIME)

float Menu_Width(float percent);
float Menu_Height(float percent);
void Menu_DrawText(Font font, const char *text, int centerX, int y, float size, Color color);
bool Menu_DrawButton(Rectangle button, const char *text);
bool Menu_IsHovered(Rectangle button);

void Title_Draw(void);
void Pause_Draw(void);

void MapName_Show(const char *name);
void MapName_Draw(float delta);

#endif
