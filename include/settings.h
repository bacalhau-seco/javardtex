#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdbool.h>

#include "raylib.h"
#include "game.h"

void Settings_Open(void);
void Settings_OpenFrom(GameState back);
void Settings_Draw(void);

#endif
