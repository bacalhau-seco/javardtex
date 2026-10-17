#include "../include/world.h"
#include "../include/collision.h"
#include "../include/cvar.h"

static Body body = { 0 };
static Body previousBody = { 0 };
static float tickAccumulator = 0.0f;
static bool jumpQueued = false;

static inline float TickDelta(void)
{
    float rate = Cvar_Value("tickrate", 128.0f);

    if (rate < WORLD_TICK_RATE_MIN)
        rate = WORLD_TICK_RATE_MIN;
    else if (rate > WORLD_TICK_RATE_MAX)
        rate = WORLD_TICK_RATE_MAX;

    return 1.0f / rate;
}

static inline float TickAlpha(float tickDelta)
{
    float alpha = tickDelta > 0.0f ? tickAccumulator / tickDelta : 0.0f;

    if (alpha < 0.0f)
        return 0.0f;

    return alpha > 1.0f ? 1.0f : alpha;
}

float World_TickDelta(void)
{
    return TickDelta();
}

void World_Reset(Vector3 feet)
{
    Body fresh = { 0 };

    fresh.position = feet;

    body = fresh;
    previousBody = fresh;
    tickAccumulator = 0.0f;
    jumpQueued = false;
}

void World_Pause(void)
{
    previousBody = body;
    tickAccumulator = 0.0f;
    jumpQueued = false;
}

void World_Update(const BSP_Map *map, InputState input, float yaw, float delta)
{
    const float tickDelta = TickDelta();

    if (input.jump)
        jumpQueued = true;

    tickAccumulator += delta;

    if (tickAccumulator > tickDelta * WORLD_TICK_MAX_STEPS)
        tickAccumulator = tickDelta * WORLD_TICK_MAX_STEPS;

    while (tickAccumulator >= tickDelta)
    {
        tickAccumulator -= tickDelta;
        previousBody = body;

        UpdatePlayer(map, &body, yaw, input.side, input.forward, jumpQueued, input.crouch, tickDelta);

        jumpQueued = false;
    }
}

const Body *World_Body(void)
{
    return &body;
}

Body World_ViewBody(void)
{
    const float alpha = TickAlpha(TickDelta());
    const float eyeHeight = Collision_EyeHeight(body.isCrouched);
    const float previousEye = previousBody.position.y + Collision_EyeHeight(previousBody.isCrouched);
    const float currentEye = body.position.y + eyeHeight;
    Body view = body;

    view.position = Vector3Lerp(previousBody.position, body.position, alpha);
    view.position.y = previousEye + (currentEye - previousEye) * alpha - eyeHeight;
    view.stepViewOffset = previousBody.stepViewOffset + (body.stepViewOffset - previousBody.stepViewOffset) * alpha;

    return view;
}
