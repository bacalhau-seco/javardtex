#include <math.h>

#include "../include/move.h"
#include "../include/collision.h"
#include "../include/cvar.h"

#define MOVE_MAX_CLIP_PLANES 5
#define MOVE_MAX_BUMPS 4
#define MOVE_GROUND_NORMAL 0.7f
#define MOVE_GRIP_NORMAL 0.819f
#define MOVE_SLIDE_NORMAL 0.5f
#define MOVE_TRACTION_EPSILON 0.01f
#define MOVE_STOP_EPSILON 0.1f
#define MOVE_GROUND_CHECK 2.0f
#define MOVE_UNCROUCH_STEP 1.0f

static float SlopeTraction(float normalY)
{
    const float grip = Cvar_Value("slopegrip", MOVE_GRIP_NORMAL);
    const float slide = Cvar_Value("slopeslide", MOVE_SLIDE_NORMAL);

    float t;

    if (normalY >= grip)
        return 1.0f;

    if (normalY <= slide || grip <= slide)
        return 0.0f;

    t = (normalY - slide) / (grip - slide);

    return t * t * (3.0f - 2.0f * t);
}

static float GroundTraction(const BSP_Map *map, const Body *body)
{
    Collision_TraceResult trace;
    Vector3 end = body->position;

    end.y -= MOVE_GROUND_CHECK;

    if (!Collision_Trace(map, body->position, end, body->isCrouched, &trace))
        return 0.0f;

    if (trace.startsolid)
        return 1.0f;

    if (!trace.hit)
        return 0.0f;

    return SlopeTraction(trace.normal.y);
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
            if (!body->isGrounded)
            {
                body->position.y += heightDiff;
                body->crouchFeetOffset = heightDiff;
            }

            body->isCrouched = true;
        }
    }
    else if (body->isCrouched)
    {
        float drop;

        for (drop = body->crouchFeetOffset; drop <= heightDiff; drop += MOVE_UNCROUCH_STEP)
        {
            Vector3 stood = body->position;

            stood.y -= drop;

            if (Collision_IsSolid(map, stood, false))
                continue;

            body->position = stood;
            body->crouchFeetOffset = 0.0f;
            body->isCrouched = false;
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

static Collision_TraceResult PushBody(const BSP_Map *map, Body *body, Vector3 move)
{
    Collision_TraceResult trace;
    Vector3 end = Vector3Add(body->position, move);

    Collision_Trace(map, body->position, end, body->isCrouched, &trace);

    if (!trace.startsolid)
        body->position = trace.endpos;

    return trace;
}

static int SlideMove(const BSP_Map *map, Body *body, float delta)
{
    Vector3 planes[MOVE_MAX_CLIP_PLANES];
    Vector3 primalVelocity = body->velocity;
    Vector3 originalVelocity = body->velocity;
    float timeLeft = delta;
    int numPlanes = 0;
    int blocked = 0;
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
            return 3;
        }

        if (trace.fraction > 0.0f)
        {
            body->position = trace.endpos;
            originalVelocity = body->velocity;
            numPlanes = 0;
        }

        if (trace.fraction == 1.0f)
            break;

        if (trace.normal.y > MOVE_GROUND_NORMAL)
            blocked |= 1;
        else
            blocked |= 2;

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

    return blocked;
}

static void StayOnGround(const BSP_Map *map, Body *body)
{
    float stepHeight = Cvar_Value("stepheight", 16.0f);
    const float groundCheck = MOVE_GROUND_CHECK;
    Collision_TraceResult up;
    Collision_TraceResult down;
    Vector3 start = body->position;
    Vector3 end = body->position;

    start.y += groundCheck;
    end.y -= stepHeight + groundCheck;

    if (!Collision_Trace(map, body->position, start, body->isCrouched, &up))
        return;

    if (up.startsolid)
        return;

    if (!Collision_Trace(map, up.endpos, end, body->isCrouched, &down))
        return;

    if (down.startsolid || !down.hit || down.fraction == 0.0f || down.fraction == 1.0f)
        return;

    if (SlopeTraction(down.normal.y) <= MOVE_TRACTION_EPSILON)
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

static inline float HorizontalDistanceSquared(Vector3 from, Vector3 to)
{
    float dx = to.x - from.x;
    float dz = to.z - from.z;

    return dx * dx + dz * dz;
}

static void WalkMove(const BSP_Map *map, Body *body, float delta)
{
    float stepHeight = Cvar_Value("stepheight", 16.0f);
    Vector3 oldPosition = body->position;
    Vector3 oldVelocity = body->velocity;
    Vector3 flatPosition;
    Vector3 flatVelocity;
    Collision_TraceResult downTrace;
    bool wasGrounded = body->isGrounded;
    int blocked = SlideMove(map, body, delta);

    if (blocked == 0 || !wasGrounded)
        return;

    flatPosition = body->position;
    flatVelocity = body->velocity;

    body->position = oldPosition;
    body->velocity = oldVelocity;

    PushBody(map, body, (Vector3){ 0.0f, stepHeight, 0.0f });
    SlideMove(map, body, delta);

    downTrace = PushBody(map, body, (Vector3){ 0.0f, -stepHeight, 0.0f });

    if (!downTrace.hit || SlopeTraction(downTrace.normal.y) <= MOVE_TRACTION_EPSILON
        || HorizontalDistanceSquared(oldPosition, flatPosition) > HorizontalDistanceSquared(oldPosition, body->position))
    {
        body->position = flatPosition;
        body->velocity = flatVelocity;
        return;
    }

    if (blocked & 2)
        body->stepViewOffset += body->position.y - oldPosition.y;

    body->velocity.y = flatVelocity.y;
}

void Body_OnLand(Body *body)
{
    Vector3 hvel = { .x = body->velocity.x, .y = 0.0f, .z = body->velocity.z };
    float speed = Vector3Length(hvel);
    float penalty = Cvar_Value("jumppenalty", 0.0f);

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

    friction = Cvar_Value("friction", 4.0f);

    if (crouchHold)
        friction *= 2.0f;

    control = speed < Cvar_Value("stopspeed", 100.0f)
        ? Cvar_Value("stopspeed", 100.0f)
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
    bool jumped = false;

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

    if (body->isGrounded && jumpPressed)
    {
        body->velocity.y = Cvar_Value("jumpforce", 270.0f);
        body->traction = 0.0f;
        body->isGrounded = false;
        jumped = true;
    }

    body->velocity.y -= Cvar_Value("gravity", 800.0f) * delta * (1.0f - body->traction);

    if (body->traction > MOVE_TRACTION_EPSILON)
    {
        ApplyFriction(body, crouchHold, delta * body->traction);
        Accelerate(body, body->dir, wishSpeed, Cvar_Value("accel", 5.0f) * wishSpeed * delta * body->traction);
    }

    if (body->traction < 1.0f)
    {
        float airCap = Cvar_Value("airwishcap", 30.0f);
        float airSpeed = wishSpeed < airCap ? wishSpeed : airCap;
        float airAccel = Cvar_Value("airaccel", 100.0f) * Cvar_Value("maxspeed", 290.0f) * delta;

        Accelerate(body, body->dir, airSpeed, airAccel * (1.0f - body->traction));
    }

    WalkMove(map, body, delta);

    if (wasGrounded && !jumped && body->velocity.y <= 0.0f)
        StayOnGround(map, body);

    body->traction = jumped ? 0.0f : GroundTraction(map, body);
    body->isGrounded = body->traction > MOVE_TRACTION_EPSILON;

    body->velocity.y *= 1.0f - body->traction;

    DecayStepViewOffset(body, delta);

    if (!wasGrounded && body->isGrounded)
        Body_OnLand(body);
}
