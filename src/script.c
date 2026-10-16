#include <string.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "../include/script.h"
#include "../include/game.h"
#include "../include/title.h"
#include "../include/settings.h"

static lua_State *lua = NULL;

static UI_Config ui = {
    .button = {
        .widthPct = MENU_BUTTON_WIDTH_PCT,
        .heightPct = MENU_BUTTON_HEIGHT_PCT,
        .spacingPct = MENU_BUTTON_SPACING_PCT,
        .textPct = MENU_BUTTON_TEXT_PCT,
        .borderPct = MENU_BORDER_PCT,
        .fill = { 200, 200, 200, 255 },
        .hoverFill = { 130, 130, 130, 255 },
        .border = { 80, 80, 80, 255 },
        .text = { 0, 0, 0, 255 }
    },
    .gameTitle = {
        .text = GAMENAME,
        .textPct = MENU_TITLE_TEXT_PCT,
        .offsetPct = MENU_TITLE_OFFSET_PCT,
        .color = { 0, 0, 0, 255 }
    },
    .mapTitle = {
        .textPct = MAPNAME_TEXT_PCT,
        .fadeTime = MAPNAME_FADE_TIME,
        .holdTime = MAPNAME_HOLD_TIME,
        .color = { 245, 245, 245, 255 }
    },
    .version = {
        .textPct = MENU_VERSION_TEXT_PCT,
        .marginPct = MENU_MARGIN_PCT,
        .textColor = { 0, 0, 0, 255 },
        .pauseTextColor = { 245, 245, 245, 255 }
    },
    .settings = {
        .rowHeightPct = SETTINGS_ROW_HEIGHT_PCT,
        .rowSpacingPct = SETTINGS_ROW_SPACING_PCT,
        .labelWidthPct = SETTINGS_LABEL_WIDTH_PCT,
        .labelTextPct = SETTINGS_LABEL_TEXT_PCT,
        .keyWidthPct = SETTINGS_KEY_WIDTH_PCT,
        .tabWidthPct = SETTINGS_TAB_WIDTH_PCT,
        .tabHeightPct = SETTINGS_TAB_HEIGHT_PCT,
        .aspectWidthPct = SETTINGS_ASPECT_WIDTH_PCT,
        .titleOffsetPct = SETTINGS_TITLE_OFFSET_PCT,
        .tabsOffsetPct = SETTINGS_TABS_OFFSET_PCT,
        .columnSpacingPct = MENU_MARGIN_PCT,
        .stepperWidthScale = SETTINGS_STEPPER_WIDTH_SCALE,
        .labelColor = { 0, 0, 0, 255 }
    },
    .fontUi = {
        .path = FONT_UI_PATH,
        .size = FONT_UI_SIZE
    },
    .fontMenu = {
        .path = FONT_MENU_PATH,
        .size = FONT_MENU_SIZE
    }
};

static inline unsigned char ClampChannel(lua_Number value)
{
    if (value <= 0.0)
        return 0;

    if (value >= 255.0)
        return 255;

    return (unsigned char)value;
}

static float ReadNumber(lua_State *state, const char *key, float fallback)
{
    float value = fallback;

    lua_getfield(state, -1, key);

    if (lua_isnumber(state, -1))
        value = (float)lua_tonumber(state, -1);

    lua_pop(state, 1);

    return value;
}

static Color ReadColor(lua_State *state, const char *key, Color fallback)
{
    Color color = fallback;
    int i;

    lua_getfield(state, -1, key);

    if (!lua_istable(state, -1))
    {
        lua_pop(state, 1);
        return color;
    }

    unsigned char *channels[4] = { &color.r, &color.g, &color.b, &color.a };

    for (i = 0; i < 4; i++)
    {
        lua_rawgeti(state, -1, i + 1);

        if (lua_isnumber(state, -1))
            *channels[i] = ClampChannel(lua_tonumber(state, -1));

        lua_pop(state, 1);
    }

    lua_pop(state, 1);

    return color;
}

static void ReadText(lua_State *state, const char *key, char *dst, int size)
{
    lua_getfield(state, -1, key);

    if (lua_isstring(state, -1))
    {
        const char *text = lua_tostring(state, -1);

        if (text)
        {
            strncpy(dst, text, (size_t)size - 1);
            dst[size - 1] = '\0';
        }
    }

    lua_pop(state, 1);
}

static bool PushUiField(lua_State *state, const char *key)
{
    lua_getfield(state, -1, key);

    if (lua_istable(state, -1))
        return true;

    lua_pop(state, 1);

    return false;
}

static void ReadButtonConfig(lua_State *state)
{
    if (!PushUiField(state, "button"))
        return;

    ui.button.widthPct = ReadNumber(state, "width", ui.button.widthPct);
    ui.button.heightPct = ReadNumber(state, "height", ui.button.heightPct);
    ui.button.spacingPct = ReadNumber(state, "spacing", ui.button.spacingPct);
    ui.button.textPct = ReadNumber(state, "textSize", ui.button.textPct);
    ui.button.borderPct = ReadNumber(state, "borderSize", ui.button.borderPct);
    ui.button.fill = ReadColor(state, "fill", ui.button.fill);
    ui.button.hoverFill = ReadColor(state, "hoverFill", ui.button.hoverFill);
    ui.button.border = ReadColor(state, "border", ui.button.border);
    ui.button.text = ReadColor(state, "textColor", ui.button.text);

    lua_pop(state, 1);
}

static void ReadGameTitleConfig(lua_State *state)
{
    if (!PushUiField(state, "gametitle"))
        return;

    ReadText(state, "text", ui.gameTitle.text, UI_TEXT_SIZE);
    ui.gameTitle.textPct = ReadNumber(state, "textSize", ui.gameTitle.textPct);
    ui.gameTitle.offsetPct = ReadNumber(state, "offset", ui.gameTitle.offsetPct);
    ui.gameTitle.color = ReadColor(state, "textColor", ui.gameTitle.color);

    lua_pop(state, 1);
}

static void ReadMapTitleConfig(lua_State *state)
{
    if (!PushUiField(state, "maptitle"))
        return;

    ui.mapTitle.textPct = ReadNumber(state, "textSize", ui.mapTitle.textPct);
    ui.mapTitle.fadeTime = ReadNumber(state, "fadeTime", ui.mapTitle.fadeTime);
    ui.mapTitle.holdTime = ReadNumber(state, "holdTime", ui.mapTitle.holdTime);
    ui.mapTitle.color = ReadColor(state, "textColor", ui.mapTitle.color);

    lua_pop(state, 1);
}

static void ReadVersionConfig(lua_State *state)
{
    if (!PushUiField(state, "version"))
        return;

    ui.version.textPct = ReadNumber(state, "textSize", ui.version.textPct);
    ui.version.marginPct = ReadNumber(state, "margin", ui.version.marginPct);
    ui.version.textColor = ReadColor(state, "textColor", ui.version.textColor);
    ui.version.pauseTextColor = ReadColor(state, "pauseTextColor", ui.version.pauseTextColor);

    lua_pop(state, 1);
}

static void ReadSettingsConfig(lua_State *state)
{
    if (!PushUiField(state, "settings"))
        return;

    ui.settings.rowHeightPct = ReadNumber(state, "rowHeight", ui.settings.rowHeightPct);
    ui.settings.rowSpacingPct = ReadNumber(state, "rowSpacing", ui.settings.rowSpacingPct);
    ui.settings.labelWidthPct = ReadNumber(state, "labelWidth", ui.settings.labelWidthPct);
    ui.settings.labelTextPct = ReadNumber(state, "labelTextSize", ui.settings.labelTextPct);
    ui.settings.keyWidthPct = ReadNumber(state, "keyWidth", ui.settings.keyWidthPct);
    ui.settings.tabWidthPct = ReadNumber(state, "tabWidth", ui.settings.tabWidthPct);
    ui.settings.tabHeightPct = ReadNumber(state, "tabHeight", ui.settings.tabHeightPct);
    ui.settings.aspectWidthPct = ReadNumber(state, "aspectWidth", ui.settings.aspectWidthPct);
    ui.settings.titleOffsetPct = ReadNumber(state, "titleOffset", ui.settings.titleOffsetPct);
    ui.settings.tabsOffsetPct = ReadNumber(state, "tabsOffset", ui.settings.tabsOffsetPct);
    ui.settings.columnSpacingPct = ReadNumber(state, "columnSpacing", ui.settings.columnSpacingPct);
    ui.settings.stepperWidthScale = ReadNumber(state, "stepperWidthScale", ui.settings.stepperWidthScale);
    ui.settings.labelColor = ReadColor(state, "labelColor", ui.settings.labelColor);

    lua_pop(state, 1);
}

static void ReadFontConfig(lua_State *state, const char *key, UI_FontConfig *font)
{
    if (!PushUiField(state, key))
        return;

    ReadText(state, "path", font->path, UI_PATH_SIZE);
    font->size = (int)ReadNumber(state, "size", (float)font->size);

    if (font->size < 1)
        font->size = 1;

    lua_pop(state, 1);
}

static void ReadUiConfig(lua_State *state)
{
    lua_getglobal(state, "ui");

    if (!lua_istable(state, -1))
    {
        TraceLog(LOG_WARNING, "SCRIPT: %s has no 'ui' table, using defaults", SCRIPT_CONFIG_PATH);
        lua_pop(state, 1);
        return;
    }

    ReadButtonConfig(state);
    ReadGameTitleConfig(state);
    ReadMapTitleConfig(state);
    ReadVersionConfig(state);
    ReadSettingsConfig(state);
    ReadFontConfig(state, "uifont", &ui.fontUi);
    ReadFontConfig(state, "menufont", &ui.fontMenu);

    lua_pop(state, 1);
}

bool Script_Init(void)
{
    if (lua)
        return true;

    lua = luaL_newstate();

    if (!lua)
    {
        TraceLog(LOG_WARNING, "SCRIPT: failed to create lua state, using defaults");
        return false;
    }

    luaL_openlibs(lua);

    if (luaL_dofile(lua, SCRIPT_CONFIG_PATH) != LUA_OK)
    {
        TraceLog(LOG_WARNING, "SCRIPT: %s", lua_tostring(lua, -1));
        lua_pop(lua, 1);
        return false;
    }

    ReadUiConfig(lua);

    return true;
}

void Script_Shutdown(void)
{
    if (!lua)
        return;

    lua_close(lua);
    lua = NULL;
}

const UI_Config *UI_Get(void)
{
    return &ui;
}
