#ifndef SKY_H
#define SKY_H

#include <stdbool.h>

#include "raylib.h"
#include "bsp.h"

#define SKY_FACE_COUNT 6

typedef struct {
    Texture2D faces[SKY_FACE_COUNT];
    bool loaded;
} Sky;

bool Sky_Load(Sky *sky, const BSP_Map *map);
void Sky_Unload(Sky *sky);
void Sky_Draw(const Sky *sky, Vector3 origin);

#endif
