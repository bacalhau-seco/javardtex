#pragma once

#include "move.h"
#include "raylib.h"
#include "raymath.h"

#define FOV 90.0f

void Camera_Update(Camera *camera, const Body *player, Vector2 lookRotation, float headLerp);
