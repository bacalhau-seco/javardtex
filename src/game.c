#include "../include/game.h"
#include "../include/script.h"

bool newGameRequested = false;
bool quitRequested = false;

static Font fonts[FONT_COUNT];
static bool fontsLoaded = false;

static Font LoadGameFont(const char *path, int size)
{
    Font font = { 0 };

    if (FileExists(path))
        font = LoadFontEx(path, size, 0, 0);

    if (font.texture.id == 0)
        font = GetFontDefault();
    else
        SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);

    return font;
}

void Fonts_Load(void)
{
    if (fontsLoaded)
        return;

    const UI_Config *ui = UI_Get();

    fonts[FONT_UI] = LoadGameFont(ui->fontUi.path, ui->fontUi.size);
    fonts[FONT_MENU] = LoadGameFont(ui->fontMenu.path, ui->fontMenu.size);

    fontsLoaded = true;
}

void Fonts_Unload(void)
{
    if (!fontsLoaded)
        return;

    for (int i = 0; i < FONT_COUNT; i++)
        UnloadFont(fonts[i]);

    fontsLoaded = false;
}

Font Fonts_Get(GameFontId id)
{
    if (!fontsLoaded || id < 0 || id >= FONT_COUNT)
        return GetFontDefault();

    return fonts[id];
}
