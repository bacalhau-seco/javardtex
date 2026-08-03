#pragma once

#include "move.h"
#include "raylib.h"
#include "raymath.h"

void Camera_Update(Camera *camera, const Body *player, Vector2 lookRotation, float headLerp);

void InitCamera(Camera *camera, const Body *player, float headLerp);
