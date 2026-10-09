#include <math.h>

#include "../include/defs.h"
#include "../include/collision.h"

#define MOVE_STEP 1.0f
#define GROUND_CHECK 2.0f
#define AIR_WISH_SPEED_CAP 30.0f

static float *Component(Vector3 *v, int axis)
{
    if (axis == 0)
        return &v->x;

    if (axis == 1)
        return &v->y;

    return &v->z;
}

static bool IsStanding(const BSP_Map *map, Vector3 feet)
{
    feet.y -= GROUND_CHECK;
    return Collision_IsSolid(map, feet);
}

static bool MoveAxis(const BSP_Map *map, Body *body, int axis, float distance)
{
    int steps = (int)ceilf(fabsf(distance) / MOVE_STEP);
    float step;
    int i;

    if (steps == 0)
        return true;

    step = distance / (float)steps;

    for (i = 0; i < steps; i++)
    {
        Vector3 next = body->position;

        *Component(&next, axis) += step;

        if (Collision_IsSolid(map, next))
            return false;

        body->position = next;
    }

    return true;
}

void Body_OnLand(Body *body)
{
    Vector3 hvel = { .x = body->velocity.x, .y = 0.0f, .z = body->velocity.z };
    float speed = Vector3Length(hvel);
    float penalty = Cvar_Get("sv_jumppenalty")->value;

    if (speed > penalty)
    {
        hvel = Vector3Scale(hvel, (speed - penalty) / speed);
        body->velocity.x = hvel.x;
        body->velocity.z = hvel.z;
    }
}

static void ApplyFriction(Body *body, bool crouchHold, float delta)
{
    float speed = sqrtf(body->velocity.x * body->velocity.x + body->velocity.z * body->velocity.z);
    float friction;
    float control;
    float newSpeed;
    float scale;

    if (speed <= 0.001f)
        return;

    friction = Cvar_Get("sv_friction")->value;

    if (crouchHold)
        friction *= 2.0f;

    control = speed < Cvar_Get("sv_stopspeed")->value
        ? Cvar_Get("sv_stopspeed")->value
        : speed;

    newSpeed = speed - delta * control * friction;

    if (newSpeed < 0.0f)
        newSpeed = 0.0f;

    scale = newSpeed / speed;

    body->velocity.x *= scale;
    body->velocity.z *= scale;
}

static void Accelerate(Body *body, Vector3 wishDir, float wishSpeed, float accel)
{
    float currentSpeed = body->velocity.x * wishDir.x + body->velocity.z * wishDir.z;
    float addSpeed = wishSpeed - currentSpeed;

    if (addSpeed <= 0.0f)
        return;

    if (accel > addSpeed)
        accel = addSpeed;

    body->velocity.x += wishDir.x * accel;
    body->velocity.z += wishDir.z * accel;
}

void UpdatePlayer(const BSP_Map *map, Body *body, float rot, char side, char forward, bool jumpPressed, bool crouchHold, float delta)
{
    Vector2 input = { (float)side, (float)-forward };
    Vector3 front = { sinf(rot), 0.0f, cosf(rot) };
    Vector3 right = { cosf(rot), 0.0f, -sinf(rot) };
    Vector3 wish;
    float wishSpeed;
    bool wasGrounded = body->isGrounded;

    if (side != 0 && forward != 0)
        input = Vector2Normalize(input);

    wish = (Vector3){
        input.x * right.x + input.y * front.x,
        0.0f,
        input.x * right.z + input.y * front.z
    };

    body->dir = Vector3Normalize(wish);
    wishSpeed = Vector3Length(body->dir) * Cvar_Get("sv_maxspeed")->value;

    if (body->isGrounded && jumpPressed)
    {
        body->velocity.y = Cvar_Get("sv_jumpforce")->value;
        body->isGrounded = false;
    }

    if (!body->isGrounded)
        body->velocity.y -= Cvar_Get("sv_gravity")->value * delta;

    if (body->isGrounded)
    {
        ApplyFriction(body, crouchHold, delta);
        Accelerate(body, body->dir, wishSpeed, Cvar_Get("sv_accel")->value * wishSpeed * delta);
    }
    else
    {
        float airSpeed = wishSpeed < AIR_WISH_SPEED_CAP ? wishSpeed : AIR_WISH_SPEED_CAP;

        Accelerate(body, body->dir, airSpeed, Cvar_Get("sv_airaccel")->value * Cvar_Get("sv_maxspeed")->value * delta);
    }

    if (!MoveAxis(map, body, 0, body->velocity.x * delta))
        body->velocity.x = 0.0f;

    if (!MoveAxis(map, body, 2, body->velocity.z * delta))
        body->velocity.z = 0.0f;

    if (!MoveAxis(map, body, 1, body->velocity.y * delta))
        body->velocity.y = 0.0f;

    body->isGrounded = body->velocity.y <= 0.0f && IsStanding(map, body->position);

    if (body->isGrounded)
        body->velocity.y = 0.0f;

    if (!wasGrounded && body->isGrounded)
        Body_OnLand(body);
}
