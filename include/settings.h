#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdbool.h>

#include "game.h"

#define SETTINGS_FOV_MIN 60.0f
#define SETTINGS_FOV_MAX 130.0f
#define SETTINGS_FOV_STEP 5.0f

#define SETTINGS_SENSITIVITY_MIN 0.0002f
#define SETTINGS_SENSITIVITY_MAX 0.0060f
#define SETTINGS_SENSITIVITY_STEP 0.0002f

#define SETTINGS_LABEL_TEXT_PCT 0.024f
#define SETTINGS_ROW_HEIGHT_PCT 0.045f
#define SETTINGS_ROW_SPACING_PCT 0.010f
#define SETTINGS_LABEL_WIDTH_PCT 0.18f
#define SETTINGS_KEY_WIDTH_PCT 0.16f
#define SETTINGS_TAB_WIDTH_PCT 0.14f
#define SETTINGS_TAB_HEIGHT_PCT 0.045f
#define SETTINGS_ASPECT_WIDTH_PCT 0.085f
#define SETTINGS_TITLE_OFFSET_PCT 0.14f
#define SETTINGS_TABS_OFFSET_PCT 0.07f
#define SETTINGS_STEPPER_WIDTH_SCALE 1.6f

#define SETTINGS_FPS_MIN 30
#define SETTINGS_FPS_MAX 500
#define SETTINGS_FPS_STEP 10

void Settings_Init(void);
void Settings_Open(void);
void Settings_OpenFrom(GameState back);
void Settings_Draw(void);

int Settings_RenderWidth(void);
int Settings_RenderHeight(void);
int Settings_NativeWidth(void);
int Settings_NativeHeight(void);
bool Settings_Stretch(void);
float Settings_Fov(void);
float Settings_Sensitivity(void);
int Settings_MaxFps(void);

#endif
