#ifndef GAME_H
#define GAME_H

#define GAMENAME "fodasse"
#define VERSION "0.0.0"
#define AUTHOR "bacalhau"

typedef enum {
    GAME_STATE_TITLE,
    GAME_STATE_MENU,
    GAME_STATE_GAME,
    GAME_STATE_PAUSE
} GameState;

extern GameState gameState;

#endif
