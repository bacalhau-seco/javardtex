#include "../include/world.h"
#include "../include/collision.h"

static Body body = { 0 };
static bool jumpQueued = false;

void World_Reset(Vector3 feet)
{
    Body fresh = { 0 };

    fresh.position = feet;

    body = fresh;
    jumpQueued = false;
}

void World_Pause(void)
{
    jumpQueued = false;
}

void World_Update(const BSP_Map *map, InputState input, float yaw, float delta)
{
    int steps;
    float step;

    if (input.jump)
        jumpQueued = true;

    if (delta <= 0.0f)
        return;

    if (delta > WORLD_MAX_FRAME_DELTA)
        delta = WORLD_MAX_FRAME_DELTA;

    steps = (int)(delta / WORLD_MAX_STEP_DELTA) + 1;
    step = delta / (float)steps;

    while (steps-- > 0)
    {
        PlayerMove(map, &body, yaw, input.side, input.forward, jumpQueued, input.crouch, step);

        jumpQueued = false;
    }
}

const Body *World_Body(void)
{
    return &body;
}

Body World_ViewBody(void)
{
    return body;
}
