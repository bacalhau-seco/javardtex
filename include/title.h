#ifndef TITLE_H
#define TITLE_H

#include <stdbool.h>

#include "raylib.h"

#define BUTTON_WIDTH 260
#define BUTTON_HEIGHT 44
#define BUTTON_SPACING 12
#define BUTTON_TEXT_SIZE 24

void Menu_DrawText(Font font, const char *text, int centerX, int y, float size, Color color);
bool Menu_DrawButton(Rectangle button, const char *text);

void Title_Draw(void);
void Pause_Draw(void);

#endif
