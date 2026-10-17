#include <math.h>

#include "../include/move.h"
#include "../include/collision.h"
#include "../include/cvar.h"

#define MOVE_MAX_CLIP_PLANES 5
#define MOVE_MAX_BUMPS 4
#define MOVE_GROUND_NORMAL 0.7f
#define MOVE_MAX_GROUND_RISE 180.0f
#define MOVE_STOP_EPSILON 0.1f
#define MOVE_MIN_WALK_SPEED 1.0f
#define MOVE_GROUND_CHECK 2.0f
#define MOVE_UNCROUCH_STEP 1.0f

static inline float HorizontalSpeed(Vector3 velocity)
{
    return sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);
}

static inline float HorizontalDistanceSquared(Vector3 from, Vector3 to)
{
    float dx = to.x - from.x;
    float dz = to.z - from.z;

    return dx * dx + dz * dz;
}

static inline float EyePosition(const Body *body)
{
    return body->position.y + Collision_EyeHeight(body->isCrouched);
}

static inline bool IsWalkableNormal(const Collision_TraceResult *trace)
{
    return trace->hit && trace->normal.y >= MOVE_GROUND_NORMAL;
}

static void CategorizePosition(const BSP_Map *map, Body *body)
{
    Collision_TraceResult trace;
    Vector3 point = body->position;

    point.y -= MOVE_GROUND_CHECK;
    body->isGrounded = false;

    if (body->velocity.y > MOVE_MAX_GROUND_RISE)
        return;

    if (!Collision_Trace(map, body->position, point, body->isCrouched, &trace))
        return;

    if (trace.startsolid || trace.allsolid)
    {
        body->isGrounded = true;
        return;
    }

    if (!IsWalkableNormal(&trace))
        return;

    body->isGrounded = true;
    body->position = trace.endpos;
}

static void UpdateCrouch(const BSP_Map *map, Body *body, bool crouchHold)
{
    float heightDiff = Collision_PlayerHeight(false) - Collision_PlayerHeight(true);

    if (body->isGrounded)
        body->crouchFeetOffset = 0.0f;

    if (crouchHold)
    {
        if (!body->isCrouched)
        {
            const bool airborne = !body->isGrounded;
            const float eye = EyePosition(body);

            if (airborne)
            {
                body->position.y += heightDiff;
                body->crouchFeetOffset = heightDiff;
            }

            body->isCrouched = true;

            if (airborne)
                body->stepViewOffset += EyePosition(body) - eye;
        }
    }
    else if (body->isCrouched)
    {
        float drop;

        for (drop = body->crouchFeetOffset; drop <= heightDiff; drop += MOVE_UNCROUCH_STEP)
        {
            const bool airborne = !body->isGrounded;
            const float eye = EyePosition(body);
            Vector3 stood = body->position;

            stood.y -= drop;

            if (Collision_IsSolid(map, stood, false))
                continue;

            body->position = stood;
            body->crouchFeetOffset = 0.0f;
            body->isCrouched = false;

            if (airborne)
                body->stepViewOffset += EyePosition(body) - eye;

            break;
        }
    }
}

static Vector3 ClipVelocity(Vector3 velocity, Vector3 normal, float overbounce)
{
    float backoff = Vector3DotProduct(velocity, normal) * overbounce;
    Vector3 out = Vector3Subtract(velocity, Vector3Scale(normal, backoff));

    if (fabsf(out.x) < MOVE_STOP_EPSILON)
        out.x = 0.0f;

    if (fabsf(out.y) < MOVE_STOP_EPSILON)
        out.y = 0.0f;

    if (fabsf(out.z) < MOVE_STOP_EPSILON)
        out.z = 0.0f;

    return out;
}

static void PushBody(const BSP_Map *map, Body *body, Vector3 move)
{
    Collision_TraceResult trace;
    Vector3 end = Vector3Add(body->position, move);

    if (!Collision_Trace(map, body->position, end, body->isCrouched, &trace))
        return;

    if (!trace.startsolid && !trace.allsolid)
        body->position = trace.endpos;
}

static void SlideMove(const BSP_Map *map, Body *body, float delta)
{
    Vector3 planes[MOVE_MAX_CLIP_PLANES];
    Vector3 primalVelocity = body->velocity;
    Vector3 originalVelocity = body->velocity;
    float timeLeft = delta;
    int numPlanes = 0;
    int bump;

    for (bump = 0; bump < MOVE_MAX_BUMPS; bump++)
    {
        Collision_TraceResult trace;
        Vector3 end;
        int i;
        int j;

        if (body->velocity.x == 0.0f && body->velocity.y == 0.0f && body->velocity.z == 0.0f)
            break;

        end = Vector3Add(body->position, Vector3Scale(body->velocity, timeLeft));

        if (!Collision_Trace(map, body->position, end, body->isCrouched, &trace))
        {
            body->position = end;
            break;
        }

        if (trace.allsolid)
        {
            body->velocity = Vector3Zero();
            break;
        }

        if (trace.fraction > 0.0f)
        {
            body->position = trace.endpos;
            originalVelocity = body->velocity;
            numPlanes = 0;
        }

        if (trace.fraction == 1.0f)
            break;

        timeLeft -= timeLeft * trace.fraction;

        if (numPlanes >= MOVE_MAX_CLIP_PLANES)
        {
            body->velocity = Vector3Zero();
            break;
        }

        planes[numPlanes] = trace.normal;
        numPlanes++;

        for (i = 0; i < numPlanes; i++)
        {
            Vector3 clipped = ClipVelocity(originalVelocity, planes[i], 1.0f);

            for (j = 0; j < numPlanes; j++)
            {
                if (j != i && Vector3DotProduct(clipped, planes[j]) < 0.0f)
                    break;
            }

            if (j == numPlanes)
            {
                body->velocity = clipped;
                break;
            }
        }

        if (i == numPlanes)
        {
            Vector3 dir;

            if (numPlanes != 2)
            {
                body->velocity = Vector3Zero();
                break;
            }

            dir = Vector3CrossProduct(planes[0], planes[1]);
            body->velocity = Vector3Scale(dir, Vector3DotProduct(dir, body->velocity));
        }

        if (Vector3DotProduct(body->velocity, primalVelocity) <= 0.0f)
        {
            body->velocity = Vector3Zero();
            break;
        }
    }
}

static void StayOnGround(const BSP_Map *map, Body *body)
{
    float stepHeight = Cvar_Value("stepheight", 16.0f);
    Collision_TraceResult up;
    Collision_TraceResult down;
    Vector3 start = body->position;
    Vector3 end = body->position;

    start.y += MOVE_GROUND_CHECK;
    end.y -= stepHeight + MOVE_GROUND_CHECK;

    if (!Collision_Trace(map, body->position, start, body->isCrouched, &up) || up.startsolid)
        return;

    if (!Collision_Trace(map, up.endpos, end, body->isCrouched, &down) || down.startsolid)
        return;

    if (down.fraction == 0.0f || down.fraction == 1.0f || !IsWalkableNormal(&down))
        return;

    body->stepViewOffset += down.endpos.y - body->position.y;
    body->position = down.endpos;
}

static void DecayStepViewOffset(Body *body, float delta)
{
    float limit = Cvar_Value("stepheight", 16.0f) + MOVE_GROUND_CHECK;
    float move = Cvar_Value("stepsmooth", 150.0f) * delta;

    if (body->stepViewOffset > limit)
        body->stepViewOffset = limit;

    if (body->stepViewOffset < -limit)
        body->stepViewOffset = -limit;

    if (body->stepViewOffset > move)
        body->stepViewOffset -= move;
    else if (body->stepViewOffset < -move)
        body->stepViewOffset += move;
    else
        body->stepViewOffset = 0.0f;
}

static void ApplyFriction(Body *body, bool crouchHold, float delta)
{
    float speed = HorizontalSpeed(body->velocity);
    float stopSpeed = Cvar_Value("stopspeed", 100.0f);
    float friction = Cvar_Value("friction", 4.0f);
    float control;
    float newSpeed;
    float scale;

    if (speed <= 0.001f)
        return;

    if (crouchHold)
        friction *= 2.0f;

    control = speed < stopSpeed ? stopSpeed : speed;
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

static void WalkMove(const BSP_Map *map, Body *body, float delta)
{
    float stepHeight = Cvar_Value("stepheight", 16.0f);
    Collision_TraceResult trace;
    Vector3 original;
    Vector3 originalVelocity;
    Vector3 down;
    Vector3 downVelocity;
    Vector3 dest;

    body->velocity.y = 0.0f;

    if (HorizontalSpeed(body->velocity) < MOVE_MIN_WALK_SPEED)
    {
        body->velocity.x = 0.0f;
        body->velocity.z = 0.0f;
        return;
    }

    dest = body->position;
    dest.x += body->velocity.x * delta;
    dest.z += body->velocity.z * delta;

    if (!Collision_Trace(map, body->position, dest, body->isCrouched, &trace))
    {
        body->position = dest;
        StayOnGround(map, body);
        return;
    }

    if (trace.fraction == 1.0f)
    {
        body->position = trace.endpos;
        StayOnGround(map, body);
        return;
    }

    original = body->position;
    originalVelocity = body->velocity;

    SlideMove(map, body, delta);

    down = body->position;
    downVelocity = body->velocity;

    body->position = original;
    body->velocity = originalVelocity;

    PushBody(map, body, (Vector3){ 0.0f, stepHeight, 0.0f });
    SlideMove(map, body, delta);

    dest = body->position;
    dest.y -= stepHeight;

    if (Collision_Trace(map, body->position, dest, body->isCrouched, &trace) && IsWalkableNormal(&trace))
    {
        if (!trace.startsolid && !trace.allsolid)
            body->position = trace.endpos;

        if (HorizontalDistanceSquared(original, down) <= HorizontalDistanceSquared(original, body->position))
        {
            body->stepViewOffset += body->position.y - original.y;
            body->velocity.y = downVelocity.y;
            return;
        }
    }

    body->position = down;
    body->velocity = downVelocity;
}

static void AirMove(const BSP_Map *map, Body *body, Vector3 wishDir, float wishSpeed, float delta)
{
    float wishCap = Cvar_Value("airwishcap", 30.0f);
    float airSpeed = wishSpeed < wishCap ? wishSpeed : wishCap;
    float halfGravity = Cvar_Value("gravity", 800.0f) * delta * 0.5f;

    body->velocity.y -= halfGravity;

    Accelerate(body, wishDir, airSpeed, Cvar_Value("airaccel", 100.0f) * wishSpeed * delta);

    SlideMove(map, body, delta);

    body->velocity.y -= halfGravity;
}

void Body_OnLand(Body *body)
{
    float speed = HorizontalSpeed(body->velocity);
    float penalty = Cvar_Value("jumppenalty", 0.0f);
    float scale;

    if (speed <= penalty)
        return;

    scale = (speed - penalty) / speed;

    body->velocity.x *= scale;
    body->velocity.z *= scale;
}

void UpdatePlayer(const BSP_Map *map, Body *body, float rot, char side, char forward, bool jumpPressed, bool crouchHold, float delta)
{
    Vector2 input = { (float)side, (float)-forward };
    Vector3 front = { sinf(rot), 0.0f, cosf(rot) };
    Vector3 right = { cosf(rot), 0.0f, -sinf(rot) };
    Vector3 wish;
    float wishSpeed;
    bool wasGrounded;

    UpdateCrouch(map, body, crouchHold);

    if (side != 0 && forward != 0)
        input = Vector2Normalize(input);

    wish = (Vector3){
        input.x * right.x + input.y * front.x,
        0.0f,
        input.x * right.z + input.y * front.z
    };

    body->dir = Vector3Normalize(wish);
    wishSpeed = Vector3Length(body->dir) * Cvar_Value("maxspeed", 290.0f);

    CategorizePosition(map, body);

    wasGrounded = body->isGrounded;

    if (body->isGrounded && jumpPressed)
    {
        body->velocity.y = Cvar_Value("jumpforce", 270.0f);
        body->isGrounded = false;
    }

    if (body->isGrounded)
    {
        body->velocity.y = 0.0f;

        ApplyFriction(body, crouchHold, delta);
        Accelerate(body, body->dir, wishSpeed, Cvar_Value("accel", 5.0f) * wishSpeed * delta);
        WalkMove(map, body, delta);
    }
    else
    {
        AirMove(map, body, body->dir, wishSpeed, delta);
    }

    CategorizePosition(map, body);
    DecayStepViewOffset(body, delta);

    if (!wasGrounded && body->isGrounded)
        Body_OnLand(body);
}
