#ifndef GAME_H
#define GAME_H

#define GAMENAME "demo"
#define VERSION "0.0.1"
#define AUTHOR "bacalhau"

#define FONT_UI_PATH "game/fonts/ui.ttf"
#define FONT_UI_SIZE 64

#define FONT_MENU_PATH "game/fonts/menu.ttf"
#define FONT_MENU_SIZE 96

typedef enum {
    FONT_UI,
    FONT_MENU,
    FONT_COUNT
} GameFontId;

typedef enum {
    GAME_STATE_TITLE,
    GAME_STATE_MENU,
    GAME_STATE_GAME,
    GAME_STATE_PAUSE
} GameState;

#define STARTMAP "game/maps/01.bsp"

extern GameState gameState;
extern bool newGameRequested;
extern bool quitRequested;

void Fonts_Load(void);
void Fonts_Unload(void);
Font Fonts_Get(GameFontId id);

#endif
