#pragma once

#include "move.h"
#include "raylib.h"
#include "raymath.h"

#define CAMERA_MAX_PITCH (80.0f * DEG2RAD)

void Camera_Update(Camera *camera, const Body *player, Vector2 lookRotation);

void InitCamera(Camera *camera, const Body *player);
