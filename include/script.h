#ifndef SCRIPT_H
#define SCRIPT_H

#include <stdbool.h>

#include "raylib.h"

#define SCRIPT_DIR "game/scripts"
#define SCRIPT_CONFIG_PATH SCRIPT_DIR "/config.lua"
#define UI_TEXT_SIZE 64
#define UI_PATH_SIZE 128

typedef struct {
    float widthPct;
    float heightPct;
    float spacingPct;
    float textPct;
    float borderPct;
    Color fill;
    Color hoverFill;
    Color border;
    Color text;
} UI_ButtonConfig;

typedef struct {
    char text[UI_TEXT_SIZE];
    float textPct;
    float offsetPct;
    Color color;
} UI_GameTitleConfig;

typedef struct {
    float textPct;
    float fadeTime;
    float holdTime;
    Color color;
} UI_MapTitleConfig;

typedef struct {
    float textPct;
    float marginPct;
    Color textColor;
    Color pauseTextColor;
} UI_VersionConfig;

typedef struct {
    float rowHeightPct;
    float rowSpacingPct;
    float labelWidthPct;
    float labelTextPct;
    float keyWidthPct;
    float tabWidthPct;
    float tabHeightPct;
    float aspectWidthPct;
    float titleOffsetPct;
    float tabsOffsetPct;
    float columnSpacingPct;
    float stepperWidthScale;
    Color labelColor;
} UI_SettingsConfig;

typedef struct {
    char path[UI_PATH_SIZE];
    int size;
} UI_FontConfig;

typedef struct {
    UI_ButtonConfig button;
    UI_GameTitleConfig gameTitle;
    UI_MapTitleConfig mapTitle;
    UI_VersionConfig version;
    UI_SettingsConfig settings;
    UI_FontConfig fontUi;
    UI_FontConfig fontMenu;
} UI_Config;

bool Script_Init(void);
void Script_Shutdown(void);

const UI_Config *UI_Get(void);

#endif
