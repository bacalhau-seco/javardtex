#pragma once

#include "raylib.h"


// Body structure
typedef struct {
    Vector3 position;
    Vector3 velocity;
    Vector3 dir;
    bool isGrounded;
} Body;

void UpdatePlayer(Body *body, float rot, char side, char forward, bool jumpPressed, bool crouchHold, float delta);
