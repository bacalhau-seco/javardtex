#include <stddef.h>

#include "../include/settings.h"
#include "../include/title.h"
#include "../include/input.h"
#include "../include/cvar.h"
#include "../include/script.h"

static inline const UI_ButtonConfig *UiButton(void)
{
    return &UI_Get()->button;
}

static inline const UI_SettingsConfig *UiSettings(void)
{
    return &UI_Get()->settings;
}

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
static int capturingSlot = 0;
static bool captureCancelled = false;
static GameState returnState = GAME_STATE_TITLE;
static SettingsTab activeTab = SETTINGS_TAB_VIDEO;
static SettingsAspect activeAspect = SETTINGS_ASPECT_16_9;
static int visibleResolutions[SETTINGS_RESOLUTION_COUNT];
static int visibleCount = 0;

static int renderWidth = 1280;
static int renderHeight = 960;
static bool stretchScaling = false;
static float fieldOfView = 90.0f;
static float sensitivity = 0.001f;
static int maxFps = 90;

int Settings_NativeWidth(void)
{
    const int width = GetMonitorWidth(GetCurrentMonitor());

    return width > 0 ? width : GetScreenWidth();
}

int Settings_NativeHeight(void)
{
    const int height = GetMonitorHeight(GetCurrentMonitor());

    return height > 0 ? height : GetScreenHeight();
}

int Settings_RenderWidth(void)
{
    return renderWidth;
}

int Settings_RenderHeight(void)
{
    return renderHeight;
}

bool Settings_Stretch(void)
{
    return stretchScaling;
}

float Settings_Fov(void)
{
    return fieldOfView;
}

float Settings_Sensitivity(void)
{
    return sensitivity;
}

int Settings_MaxFps(void)
{
    return maxFps;
}

void Settings_Init(void)
{
    const int refresh = GetMonitorRefreshRate(GetCurrentMonitor());

    renderWidth = Settings_NativeWidth();
    renderHeight = Settings_NativeHeight();

    if (refresh >= SETTINGS_FPS_MIN && refresh <= SETTINGS_FPS_MAX)
        maxFps = refresh;
}

static inline int NativeWidth(void)
{
    return Settings_NativeWidth();
}

static inline int NativeHeight(void)
{
    return Settings_NativeHeight();
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
    return (activeTab == SETTINGS_TAB_CONTROLS) ? BIND_COUNT + 1 : visibleCount + 5;
}

static int DrawStepperRow(const char *label, const char *value, int centerX, int y)
{
    const float height = Menu_Height(UiSettings()->rowHeightPct);
    const float stepWidth = Menu_Height(UiSettings()->rowHeightPct) * UiSettings()->stepperWidthScale;
    const float valueWidth = Menu_Width(UiSettings()->keyWidthPct);
    const float spacing = Menu_Width(UiSettings()->columnSpacingPct);
    const float startX = centerX + spacing;

    const Rectangle minus = { startX, y, stepWidth, height };
    const Rectangle valueBox = { startX + stepWidth + spacing, y, valueWidth, height };
    const Rectangle plus = { startX + stepWidth + valueWidth + spacing * 2.0f, y, stepWidth, height };

    int delta = 0;

    Menu_DrawText(
        Fonts_Get(FONT_UI),
        label,
        centerX - (int)(Menu_Width(UiSettings()->labelWidthPct) / 2.0f),
        y + (int)((height - Menu_Height(UiSettings()->labelTextPct)) / 2.0f),
        Menu_Height(UiSettings()->labelTextPct),
        UiSettings()->labelColor
    );

    if (Menu_DrawButton(minus, "-"))
        delta--;

    Menu_DrawButton(valueBox, value);

    if (Menu_DrawButton(plus, "+"))
        delta++;

    return delta;
}

static float ClampFloatRange(float value, float min, float max)
{
    if (value < min)
        return min;

    return value > max ? max : value;
}

static void DrawFovRow(int centerX, int y)
{
    const int delta = DrawStepperRow("FOV", TextFormat("%d", (int)(fieldOfView + 0.5f)), centerX, y);

    if (delta != 0)
        fieldOfView = ClampFloatRange(fieldOfView + delta * SETTINGS_FOV_STEP, SETTINGS_FOV_MIN, SETTINGS_FOV_MAX);
}

static void DrawFpsRow(int centerX, int y)
{
    const int delta = DrawStepperRow("MAX FPS", TextFormat("%d", maxFps), centerX, y);

    if (delta == 0)
        return;

    maxFps += delta * SETTINGS_FPS_STEP;

    if (maxFps < SETTINGS_FPS_MIN)
        maxFps = SETTINGS_FPS_MIN;

    if (maxFps > SETTINGS_FPS_MAX)
        maxFps = SETTINGS_FPS_MAX;
}

static void DrawSensitivityRow(int centerX, int y)
{
    const int delta = DrawStepperRow("SENSITIVITY", TextFormat("%.1f", sensitivity * 1000.0f), centerX, y);

    if (delta != 0)
        sensitivity = ClampFloatRange(
            sensitivity + delta * SETTINGS_SENSITIVITY_STEP,
            SETTINGS_SENSITIVITY_MIN,
            SETTINGS_SENSITIVITY_MAX
        );
}

static void ApplyResolution(int width, int height)
{
    if (width <= 0 || height <= 0)
        return;

    renderWidth = width;
    renderHeight = height;
}

static bool IsCurrentResolution(int width, int height)
{
    return renderWidth == width && renderHeight == height;
}

static void DrawResolutionRow(int index, int centerX, int y)
{
    const SettingsResolution *res = &resolutions[visibleResolutions[index]];

    const Rectangle button = {
        centerX - Menu_Width(UiButton()->widthPct) / 2.0f,
        y,
        Menu_Width(UiButton()->widthPct),
        Menu_Height(UiSettings()->rowHeightPct)
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
        centerX - Menu_Width(UiButton()->widthPct) / 2.0f,
        y,
        Menu_Width(UiButton()->widthPct),
        Menu_Height(UiSettings()->rowHeightPct)
    };

    const char *text = TextFormat(
        "%sNATIVE (%dx%d)",
        IsCurrentResolution(NativeWidth(), NativeHeight()) ? "> " : "",
        NativeWidth(),
        NativeHeight()
    );

    if (Menu_DrawButton(button, text))
        ApplyResolution(NativeWidth(), NativeHeight());
}

static void DrawScalingRow(int centerX, int y)
{
    const Rectangle button = {
        centerX - Menu_Width(UiButton()->widthPct) / 2.0f,
        y,
        Menu_Width(UiButton()->widthPct),
        Menu_Height(UiSettings()->rowHeightPct)
    };

    const bool stretched = stretchScaling;

    if (!Menu_DrawButton(button, stretched ? "SCALING: STRETCH" : "SCALING: BORDERS"))
        return;

    stretchScaling = !stretched;
}

static void DrawAspectRow(int centerX, int y)
{
    static const char *labels[SETTINGS_ASPECT_COUNT] = {
        "4:3",
        "16:9",
        "16:10"
    };

    const float itemWidth = Menu_Width(UiSettings()->aspectWidthPct);
    const float spacing = Menu_Width(UiSettings()->rowSpacingPct);
    const float totalWidth = SETTINGS_ASPECT_COUNT * itemWidth + (SETTINGS_ASPECT_COUNT - 1) * spacing;

    int i;

    for (i = 0; i < SETTINGS_ASPECT_COUNT; i++)
    {
        const Rectangle button = {
            centerX - totalWidth / 2.0f + i * (itemWidth + spacing),
            y,
            itemWidth,
            Menu_Height(UiSettings()->rowHeightPct)
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

    const float tabWidth = Menu_Width(UiSettings()->tabWidthPct);
    const float spacing = Menu_Width(UiSettings()->rowSpacingPct);
    const float totalWidth = SETTINGS_TAB_COUNT * tabWidth + (SETTINGS_TAB_COUNT - 1) * spacing;

    int i;

    for (i = 0; i < SETTINGS_TAB_COUNT; i++)
    {
        const Rectangle tab = {
            centerX - totalWidth / 2.0f + i * (tabWidth + spacing),
            y,
            tabWidth,
            Menu_Height(UiSettings()->tabHeightPct)
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
    const float rowsHeight = RowsCount() * Menu_Height(UiSettings()->rowHeightPct + UiSettings()->rowSpacingPct);

    return (int)(GetRenderHeight() / 2.0f - rowsHeight / 2.0f);
}

static void UpdateCapture(void)
{
    int key;

    captureCancelled = false;

    if (capturingBind < 0)
        return;

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
    {
        capturingBind = -1;
        captureCancelled = true;
        return;
    }

    key = Input_PollBindKey();

    if (key == KEY_NULL)
        return;

    if (key != KEY_ESCAPE)
        Input_SetBindKey((InputBind)capturingBind, capturingSlot, key);

    capturingBind = -1;
}

static void DrawBindRow(int bind, int centerX, int y)
{
    const float keyWidth = Menu_Width(UiSettings()->keyWidthPct);
    const float keySpacing = Menu_Width(UiSettings()->columnSpacingPct);
    const float startX = centerX + Menu_Width(UiSettings()->columnSpacingPct);

    int slot;

    Menu_DrawText(
        Fonts_Get(FONT_UI),
        Input_BindLabel((InputBind)bind),
        centerX - (int)(Menu_Width(UiSettings()->labelWidthPct) / 2.0f),
        y + (int)((Menu_Height(UiSettings()->rowHeightPct) - Menu_Height(UiSettings()->labelTextPct)) / 2.0f),
        Menu_Height(UiSettings()->labelTextPct),
        UiSettings()->labelColor
    );

    for (slot = 0; slot < INPUT_BIND_SLOTS; slot++)
    {
        const Rectangle keyButton = {
            startX + slot * (keyWidth + keySpacing),
            y,
            keyWidth,
            Menu_Height(UiSettings()->rowHeightPct)
        };

        const bool capturing = capturingBind == bind && capturingSlot == slot;

        const char *keyText = capturing
            ? "..."
            : Input_KeyName(Input_GetBindKey((InputBind)bind, slot));

        if (Menu_DrawButton(keyButton, keyText))
        {
            if (capturingBind < 0)
            {
                capturingBind = bind;
                capturingSlot = slot;
            }

            continue;
        }

        if (capturingBind >= 0 || captureCancelled)
            continue;

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && Menu_IsHovered(keyButton))
            Input_ClearBindKey((InputBind)bind, slot);
    }
}

void Settings_Open(void)
{
    Settings_OpenFrom(GAME_STATE_TITLE);
}

void Settings_OpenFrom(GameState back)
{
    capturingBind = -1;
    capturingSlot = 0;
    captureCancelled = false;
    returnState = back;
    activeTab = SETTINGS_TAB_VIDEO;

    RebuildVisibleResolutions();
}

void Settings_Draw(void)
{
    const int centerX = GetRenderWidth() / 2;
    const int top = RowsTop();

    const int rows = RowsCount();

    const float rowStep = Menu_Height(UiSettings()->rowHeightPct + UiSettings()->rowSpacingPct);

    const Rectangle backButton = {
        centerX - Menu_Width(UiButton()->widthPct) / 2.0f,
        top + rows * rowStep + Menu_Height(UiButton()->spacingPct * 2.0f),
        Menu_Width(UiButton()->widthPct),
        Menu_Height(UiButton()->heightPct)
    };

    int i;

    UpdateCapture();

    Menu_DrawText(Fonts_Get(FONT_MENU), "SETTINGS", centerX, top - (int)Menu_Height(UiSettings()->titleOffsetPct), Menu_Height(UI_Get()->gameTitle.textPct), UI_Get()->gameTitle.color);

    DrawTabs(centerX, top - (int)Menu_Height(UiSettings()->tabsOffsetPct));

    if (activeTab == SETTINGS_TAB_CONTROLS)
    {
        for (i = 0; i < BIND_COUNT; i++)
            DrawBindRow(i, centerX, (int)(top + i * rowStep));

        DrawSensitivityRow(centerX, (int)(top + BIND_COUNT * rowStep));
    }
    else
    {
        DrawAspectRow(centerX, top);

        for (i = 0; i < visibleCount; i++)
            DrawResolutionRow(i, centerX, (int)(top + (i + 1) * rowStep));

        DrawNativeRow(centerX, (int)(top + (visibleCount + 1) * rowStep));
        DrawScalingRow(centerX, (int)(top + (visibleCount + 2) * rowStep));
        DrawFovRow(centerX, (int)(top + (visibleCount + 3) * rowStep));
        DrawFpsRow(centerX, (int)(top + (visibleCount + 4) * rowStep));
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
