#include <stddef.h>

#include "../include/settings.h"
#include "../include/title.h"
#include "../include/input.h"
#include "../include/cvar.h"
#include "raymath.h"
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
static bool mipmaps = true;
static bool vsync = false;
static float rowsScroll = 0.0f;

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

bool Settings_Mipmaps(void)
{
    return mipmaps;
}

bool Settings_Vsync(void)
{
    return vsync;
}

void Settings_Init(void)
{
    const int refresh = GetMonitorRefreshRate(GetCurrentMonitor());

    renderWidth = Settings_NativeWidth();
    renderHeight = Settings_NativeHeight();

    if (refresh >= SETTINGS_FPS_MIN && refresh <= SETTINGS_FPS_MAX)
        maxFps = refresh;
}

static void RebuildVisibleResolutions(void)
{
    const int nativeWidth = Settings_NativeWidth();
    const int nativeHeight = Settings_NativeHeight();

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
    return (activeTab == SETTINGS_TAB_CONTROLS) ? BIND_COUNT + 1 : visibleCount + 9;
}

static inline float RowStep(void)
{
    return Menu_Height(UiSettings()->rowHeightPct + UiSettings()->rowSpacingPct);
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

static int DrawStackedStepperRow(const char *label, const char *value, int centerX, int y)
{
    const float height = Menu_Height(UiSettings()->rowHeightPct);
    const float stepWidth = height * UiSettings()->stepperWidthScale;
    const float valueWidth = Menu_Width(UiSettings()->keyWidthPct);
    const float spacing = Menu_Width(UiSettings()->columnSpacingPct);
    const float totalWidth = stepWidth * 2.0f + valueWidth + spacing * 2.0f;
    const float startX = centerX - totalWidth / 2.0f;
    const float controlsY = y + RowStep();

    const Rectangle minus = { startX, controlsY, stepWidth, height };
    const Rectangle valueBox = { startX + stepWidth + spacing, controlsY, valueWidth, height };
    const Rectangle plus = { startX + stepWidth + valueWidth + spacing * 2.0f, controlsY, stepWidth, height };

    int delta = 0;

    Menu_DrawText(
        Fonts_Get(FONT_UI),
        label,
        centerX,
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

static void DrawFovRow(int centerX, int y)
{
    const int delta = DrawStackedStepperRow("FOV", TextFormat("%d", (int)(fieldOfView + 0.5f)), centerX, y);

    if (delta != 0)
        fieldOfView = Clamp(fieldOfView + delta * SETTINGS_FOV_STEP, SETTINGS_FOV_MIN, SETTINGS_FOV_MAX);
}

static void DrawFpsRow(int centerX, int y)
{
    const int delta = DrawStackedStepperRow("MAX FPS", TextFormat("%d", maxFps), centerX, y);

    if (delta != 0)
        maxFps = (int)Clamp((float)(maxFps + delta * SETTINGS_FPS_STEP), SETTINGS_FPS_MIN, SETTINGS_FPS_MAX);
}

static void DrawVsyncRow(int centerX, int y)
{
    const Rectangle button = {
        centerX - Menu_Width(UiButton()->widthPct) / 2.0f,
        y,
        Menu_Width(UiButton()->widthPct),
        Menu_Height(UiSettings()->rowHeightPct)
    };

    if (!Menu_DrawButton(button, vsync ? "VSYNC: ON" : "VSYNC: OFF"))
        return;

    vsync = !vsync;
}

static void DrawSensitivityRow(int centerX, int y)
{
    const int delta = DrawStepperRow("SENSITIVITY", TextFormat("%.1f", sensitivity * 1000.0f), centerX, y);

    if (delta != 0)
        sensitivity = Clamp(
            sensitivity + delta * SETTINGS_SENSITIVITY_STEP,
            SETTINGS_SENSITIVITY_MIN,
            SETTINGS_SENSITIVITY_MAX
        );
}

static void DrawMipmapsRow(int centerX, int y)
{
    const Rectangle button = {
        centerX - Menu_Width(UiButton()->widthPct) / 2.0f,
        y,
        Menu_Width(UiButton()->widthPct),
        Menu_Height(UiSettings()->rowHeightPct)
    };

    if (!Menu_DrawButton(button, mipmaps ? "MIPMAPS: ON" : "MIPMAPS: OFF"))
        return;

    mipmaps = !mipmaps;
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
        IsCurrentResolution(Settings_NativeWidth(), Settings_NativeHeight()) ? "> " : "",
        Settings_NativeWidth(),
        Settings_NativeHeight()
    );

    if (Menu_DrawButton(button, text))
        ApplyResolution(Settings_NativeWidth(), Settings_NativeHeight());
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
        rowsScroll = 0.0f;
    }
}

static inline float TabsY(void)
{
    return Menu_TitleY() + Menu_Height(UI_Get()->gameTitle.textPct) + Menu_Height(UiSettings()->tabsOffsetPct);
}

static inline float PanelTop(void)
{
    return floorf(TabsY() + Menu_Height(UiSettings()->tabHeightPct + UiSettings()->rowSpacingPct * 2.0f));
}

static inline float PanelHeight(void)
{
    const float reserved = Menu_Height(UiButton()->spacingPct * 2.0f + UiButton()->heightPct + UiSettings()->rowSpacingPct);
    const float available = (float)GetRenderHeight() - PanelTop() - reserved;
    const float wanted = Menu_Height(UiSettings()->panelHeightPct);

    return floorf(wanted < available ? wanted : available);
}

static inline float PanelBottom(void)
{
    return PanelTop() + PanelHeight();
}

static inline float RowsContentHeight(void)
{
    return RowsCount() * RowStep();
}

static inline float RowsMaxScroll(void)
{
    const float overflow = RowsContentHeight() - PanelHeight();

    return overflow > 0.0f ? overflow : 0.0f;
}

static void UpdateRowsScroll(void)
{
    if (capturingBind < 0)
        rowsScroll -= GetMouseWheelMove() * RowStep();

    rowsScroll = Clamp(rowsScroll, 0.0f, RowsMaxScroll());
}

static inline bool RowVisible(float y, float height)
{
    return y >= PanelTop() && y + height <= PanelBottom();
}

static inline int RowsTop(void)
{
    return (int)(PanelTop() - rowsScroll);
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
    rowsScroll = 0.0f;

    RebuildVisibleResolutions();
}

void Settings_Draw(void)
{
    const int centerX = GetRenderWidth() / 2;
    const int top = RowsTop();

    const float rowStep = RowStep();
    const float panelTop = PanelTop();

    const Rectangle backButton = {
        centerX - Menu_Width(UiButton()->widthPct) / 2.0f,
        PanelBottom() + Menu_Height(UiButton()->spacingPct * 2.0f),
        Menu_Width(UiButton()->widthPct),
        Menu_Height(UiButton()->heightPct)
    };

    int i;

    UpdateCapture();
    UpdateRowsScroll();

    Menu_DrawText(Fonts_Get(FONT_MENU), "SETTINGS", centerX, (int)Menu_TitleY(), Menu_Height(UI_Get()->gameTitle.textPct), UI_Get()->gameTitle.color);

    DrawTabs(centerX, (int)TabsY());

    BeginScissorMode(0, (int)panelTop, GetRenderWidth(), (int)PanelHeight());

    if (activeTab == SETTINGS_TAB_CONTROLS)
    {
        for (i = 0; i < BIND_COUNT; i++)
        {
            if (RowVisible(top + i * rowStep, rowStep))
                DrawBindRow(i, centerX, (int)(top + i * rowStep));
        }

        if (RowVisible(top + BIND_COUNT * rowStep, rowStep))
            DrawSensitivityRow(centerX, (int)(top + BIND_COUNT * rowStep));
    }
    else
    {
        if (RowVisible(top, rowStep))
            DrawAspectRow(centerX, top);

        for (i = 0; i < visibleCount; i++)
        {
            if (RowVisible(top + (i + 1) * rowStep, rowStep))
                DrawResolutionRow(i, centerX, (int)(top + (i + 1) * rowStep));
        }

        if (RowVisible(top + (visibleCount + 1) * rowStep, rowStep))
            DrawNativeRow(centerX, (int)(top + (visibleCount + 1) * rowStep));

        if (RowVisible(top + (visibleCount + 2) * rowStep, rowStep))
            DrawScalingRow(centerX, (int)(top + (visibleCount + 2) * rowStep));

        if (RowVisible(top + (visibleCount + 3) * rowStep, 2.0f * rowStep))
            DrawFovRow(centerX, (int)(top + (visibleCount + 3) * rowStep));

        if (RowVisible(top + (visibleCount + 5) * rowStep, 2.0f * rowStep))
            DrawFpsRow(centerX, (int)(top + (visibleCount + 5) * rowStep));

        if (RowVisible(top + (visibleCount + 7) * rowStep, rowStep))
            DrawMipmapsRow(centerX, (int)(top + (visibleCount + 7) * rowStep));

        if (RowVisible(top + (visibleCount + 8) * rowStep, rowStep))
            DrawVsyncRow(centerX, (int)(top + (visibleCount + 8) * rowStep));
    }

    EndScissorMode();

    if (!Menu_DrawButton(backButton, "BACK"))
        return;

    if (capturingBind >= 0)
    {
        capturingBind = -1;
        return;
    }

    gameState = returnState;
}
