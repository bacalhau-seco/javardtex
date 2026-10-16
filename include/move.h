#pragma once

#include <stdbool.h>

#include "raylib.h"
#include "raymath.h"
#include "bsp.h"

typedef struct {
    Vector3 position;
    Vector3 velocity;
    Vector3 dir;
    bool isGrounded;
    float traction;
    bool isCrouched;
    float crouchFeetOffset;
    float stepViewOffset;
} Body;

void UpdatePlayer(const BSP_Map *map, Body *body, float rot, char side, char forward, bool jumpPressed, bool crouchHold, float delta);
