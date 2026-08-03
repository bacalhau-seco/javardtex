#pragma once

#import "raylib.h"


// Body structure
typedef struct {
    Vector3 position;
    Vector3 velocity;
    Vector3 dir;
    bool isGrounded;
} Body;

void UpdateBody(Body *body, float rot, char side, char forward, bool jumpPressed, bool crouchHold);
