#include <stddef.h>

#include "../include/defs.h"
#include "../include/title.h"
#include "../include/settings.h"

#define SETTINGS_LABEL_TEXT_PCT 0.024f
#define SETTINGS_ROW_HEIGHT_PCT 0.045f
#define SETTINGS_ROW_SPACING_PCT 0.010f
#define SETTINGS_LABEL_WIDTH_PCT 0.18f
#define SETTINGS_KEY_WIDTH_PCT 0.16f

#define SETTINGS_TAB_WIDTH_PCT 0.14f
#define SETTINGS_TAB_HEIGHT_PCT 0.045f
#define SETTINGS_ASPECT_WIDTH_PCT 0.085f

typedef enum {
    SETTINGS_TAB_VIDEO,
    SETTINGS_TAB_CONTROLS,
    SETTINGS_TAB_COUNT
} SettingsTab;

typedef enum {
    SETTINGS_ASPECT_4_3,
    SETTINGS_ASPECT_16_9,
    SETTINGS_ASPECT_16_10,
    SETTINGS_ASPECT_COUNT
} SettingsAspect;

typedef struct {
    int width;
    int height;
    SettingsAspect aspect;
} SettingsResolution;

static const SettingsResolution resolutions[] = {
    {  640,  480, SETTINGS_ASPECT_4_3 },
    {  800,  600, SETTINGS_ASPECT_4_3 },
    { 1024,  768, SETTINGS_ASPECT_4_3 },
    { 1280,  960, SETTINGS_ASPECT_4_3 },
    { 1600, 1200, SETTINGS_ASPECT_4_3 },

    { 1280,  720, SETTINGS_ASPECT_16_9 },
    { 1366,  768, SETTINGS_ASPECT_16_9 },
    { 1600,  900, SETTINGS_ASPECT_16_9 },
    { 1920, 1080, SETTINGS_ASPECT_16_9 },
    { 2560, 1440, SETTINGS_ASPECT_16_9 },

    { 1280,  800, SETTINGS_ASPECT_16_10 },
    { 1440,  900, SETTINGS_ASPECT_16_10 },
    { 1680, 1050, SETTINGS_ASPECT_16_10 },
    { 1920, 1200, SETTINGS_ASPECT_16_10 },
    { 2560, 1600, SETTINGS_ASPECT_16_10 }
};

#define SETTINGS_RESOLUTION_COUNT ((int)(sizeof(resolutions) / sizeof(resolutions[0])))

static int capturingBind = -1;
static GameState returnState = GAME_STATE_TITLE;
static SettingsTab activeTab = SETTINGS_TAB_VIDEO;
static SettingsAspect activeAspect = SETTINGS_ASPECT_16_9;
static int visibleResolutions[SETTINGS_RESOLUTION_COUNT];
static int visibleCount = 0;

static inline int NativeWidth(void)
{
    return (int)Cvar_Value("nativehres", 1920.0f);
}

static inline int NativeHeight(void)
{
    return (int)Cvar_Value("nativevres", 1080.0f);
}

static void RebuildVisibleResolutions(void)
{
    const int nativeWidth = NativeWidth();
    const int nativeHeight = NativeHeight();

    int i;

    visibleCount = 0;

    for (i = 0; i < SETTINGS_RESOLUTION_COUNT; i++)
    {
        if (resolutions[i].aspect != activeAspect)
            continue;

        if (resolutions[i].width > nativeWidth || resolutions[i].height > nativeHeight)
            continue;

        visibleResolutions[visibleCount++] = i;
    }
}

static inline int RowsCount(void)
{
    return (activeTab == SETTINGS_TAB_CONTROLS) ? BIND_COUNT : visibleCount + 3;
}

static void ApplyResolution(int width, int height)
{
    Cvar *hres = Cvar_Get("hres");
    Cvar *vres = Cvar_Get("vres");

    if (hres == NULL || vres == NULL || width <= 0 || height <= 0)
        return;

    hres->value = (float)width;
    vres->value = (float)height;
}

static bool IsCurrentResolution(int width, int height)
{
    return (int)Cvar_Value("hres", 0.0f) == width
        && (int)Cvar_Value("vres", 0.0f) == height;
}

static void DrawResolutionRow(int index, int centerX, int y)
{
    const SettingsResolution *res = &resolutions[visibleResolutions[index]];

    const Rectangle button = {
        centerX - Menu_Width(MENU_BUTTON_WIDTH_PCT) / 2.0f,
        y,
        Menu_Width(MENU_BUTTON_WIDTH_PCT),
        Menu_Height(SETTINGS_ROW_HEIGHT_PCT)
    };

    const char *text = TextFormat(
        "%s%dx%d",
        IsCurrentResolution(res->width, res->height) ? "> " : "",
        res->width,
        res->height
    );

    if (Menu_DrawButton(button, text))
        ApplyResolution(res->width, res->height);
}

static void DrawNativeRow(int centerX, int y)
{
    const Rectangle button = {
        centerX - Menu_Width(MENU_BUTTON_WIDTH_PCT) / 2.0f,
        y,
        Menu_Width(MENU_BUTTON_WIDTH_PCT),
        Menu_Height(SETTINGS_ROW_HEIGHT_PCT)
    };

    const char *text = TextFormat(
        "%sNATIVO (%dx%d)",
        IsCurrentResolution(NativeWidth(), NativeHeight()) ? "> " : "",
        NativeWidth(),
        NativeHeight()
    );

    if (Menu_DrawButton(button, text))
        ApplyResolution(NativeWidth(), NativeHeight());
}

static void DrawScalingRow(int centerX, int y)
{
    Cvar *stretch = Cvar_Get("stretch");

    const Rectangle button = {
        centerX - Menu_Width(MENU_BUTTON_WIDTH_PCT) / 2.0f,
        y,
        Menu_Width(MENU_BUTTON_WIDTH_PCT),
        Menu_Height(SETTINGS_ROW_HEIGHT_PCT)
    };

    const bool stretched = (stretch != NULL && stretch->value != 0.0f);

    if (!Menu_DrawButton(button, stretched ? "SCALING: STRETCH" : "SCALING: BORDAS"))
        return;

    if (stretch != NULL)
        stretch->value = stretched ? 0.0f : 1.0f;
}

static void DrawAspectRow(int centerX, int y)
{
    static const char *labels[SETTINGS_ASPECT_COUNT] = {
        "4:3",
        "16:9",
        "16:10"
    };

    const float itemWidth = Menu_Width(SETTINGS_ASPECT_WIDTH_PCT);
    const float spacing = Menu_Width(SETTINGS_ROW_SPACING_PCT);
    const float totalWidth = SETTINGS_ASPECT_COUNT * itemWidth + (SETTINGS_ASPECT_COUNT - 1) * spacing;

    int i;

    for (i = 0; i < SETTINGS_ASPECT_COUNT; i++)
    {
        const Rectangle button = {
            centerX - totalWidth / 2.0f + i * (itemWidth + spacing),
            y,
            itemWidth,
            Menu_Height(SETTINGS_ROW_HEIGHT_PCT)
        };

        const char *text = (i == (int)activeAspect) ? TextFormat("> %s", labels[i]) : labels[i];

        if (!Menu_DrawButton(button, text))
            continue;

        activeAspect = (SettingsAspect)i;
        RebuildVisibleResolutions();
    }
}

static void DrawTabs(int centerX, int y)
{
    static const char *labels[SETTINGS_TAB_COUNT] = {
        "VIDEO",
        "CONTROLS"
    };

    const float tabWidth = Menu_Width(SETTINGS_TAB_WIDTH_PCT);
    const float spacing = Menu_Width(SETTINGS_ROW_SPACING_PCT);
    const float totalWidth = SETTINGS_TAB_COUNT * tabWidth + (SETTINGS_TAB_COUNT - 1) * spacing;

    int i;

    for (i = 0; i < SETTINGS_TAB_COUNT; i++)
    {
        const Rectangle tab = {
            centerX - totalWidth / 2.0f + i * (tabWidth + spacing),
            y,
            tabWidth,
            Menu_Height(SETTINGS_TAB_HEIGHT_PCT)
        };

        const char *text = (i == (int)activeTab) ? TextFormat("> %s", labels[i]) : labels[i];

        if (!Menu_DrawButton(tab, text))
            continue;

        capturingBind = -1;
        activeTab = (SettingsTab)i;
    }
}

static inline int RowsTop(void)
{
    const float rowsHeight = RowsCount() * Menu_Height(SETTINGS_ROW_HEIGHT_PCT + SETTINGS_ROW_SPACING_PCT);

    return (int)(GetRenderHeight() / 2.0f - rowsHeight / 2.0f);
}

static void UpdateCapture(void)
{
    int key;

    if (capturingBind < 0)
        return;

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
    {
        capturingBind = -1;
        return;
    }

    key = Input_PollBindKey();

    if (key == KEY_NULL)
        return;

    if (key != KEY_ESCAPE)
        Input_SetBindKey((InputBind)capturingBind, key);

    capturingBind = -1;
}

static void DrawBindRow(int bind, int centerX, int y)
{
    const Rectangle keyButton = {
        centerX + Menu_Width(SETTINGS_LABEL_WIDTH_PCT) / 2.0f - Menu_Width(SETTINGS_KEY_WIDTH_PCT) / 2.0f + Menu_Width(MENU_MARGIN_PCT),
        y,
        Menu_Width(SETTINGS_KEY_WIDTH_PCT),
        Menu_Height(SETTINGS_ROW_HEIGHT_PCT)
    };

    const char *keyText = capturingBind == bind
        ? "..."
        : Input_KeyName(Input_GetBindKey((InputBind)bind));

    Menu_DrawText(
        Fonts_Get(FONT_UI),
        Input_BindLabel((InputBind)bind),
        centerX - (int)(Menu_Width(SETTINGS_LABEL_WIDTH_PCT) / 2.0f),
        y + (int)((Menu_Height(SETTINGS_ROW_HEIGHT_PCT) - Menu_Height(SETTINGS_LABEL_TEXT_PCT)) / 2.0f),
        Menu_Height(SETTINGS_LABEL_TEXT_PCT),
        BLACK
    );

    if (!Menu_DrawButton(keyButton, keyText))
        return;

    if (capturingBind < 0)
        capturingBind = bind;
}

void Settings_Open(void)
{
    Settings_OpenFrom(GAME_STATE_TITLE);
}

void Settings_OpenFrom(GameState back)
{
    capturingBind = -1;
    returnState = back;
    activeTab = SETTINGS_TAB_VIDEO;

    RebuildVisibleResolutions();
}

void Settings_Draw(void)
{
    const int centerX = GetRenderWidth() / 2;
    const int top = RowsTop();

    const int rows = RowsCount();

    const float rowStep = Menu_Height(SETTINGS_ROW_HEIGHT_PCT + SETTINGS_ROW_SPACING_PCT);

    const Rectangle backButton = {
        centerX - Menu_Width(MENU_BUTTON_WIDTH_PCT) / 2.0f,
        top + rows * rowStep + Menu_Height(MENU_BUTTON_SPACING_PCT * 2.0f),
        Menu_Width(MENU_BUTTON_WIDTH_PCT),
        Menu_Height(MENU_BUTTON_HEIGHT_PCT)
    };

    int i;

    UpdateCapture();

    Menu_DrawText(Fonts_Get(FONT_MENU), "SETTINGS", centerX, top - (int)Menu_Height(0.14f), Menu_Height(MENU_TITLE_TEXT_PCT), BLACK);

    DrawTabs(centerX, top - (int)Menu_Height(0.07f));

    if (activeTab == SETTINGS_TAB_CONTROLS)
    {
        for (i = 0; i < rows; i++)
            DrawBindRow(i, centerX, (int)(top + i * rowStep));
    }
    else
    {
        DrawAspectRow(centerX, top);

        for (i = 0; i < visibleCount; i++)
            DrawResolutionRow(i, centerX, (int)(top + (i + 1) * rowStep));

        DrawNativeRow(centerX, (int)(top + (visibleCount + 1) * rowStep));
        DrawScalingRow(centerX, (int)(top + (visibleCount + 2) * rowStep));
    }

    if (!Menu_DrawButton(backButton, "BACK"))
        return;

    if (capturingBind >= 0)
    {
        capturingBind = -1;
        return;
    }

    gameState = returnState;
}
