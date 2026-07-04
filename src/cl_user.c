#include "../include/defs.h"
#include <stdio.h>

void Body_OnLand(Body *body)
{
    Vector3 hvel = {.x=body->velocity.x, .y=0.0f, .z=body->velocity.z};
    float speed = Vector3Length(hvel);
    if (speed > JUMP_PENALTY)
    {
        hvel = Vector3Scale(hvel, (speed - JUMP_PENALTY) / speed);

        body->velocity.x = hvel.x;
        body->velocity.z = hvel.z;
    }
}

// Update body considering current world state
void UpdateBody(Body *body, float rot, char side, char forward, bool jumpPressed, bool crouchHold)
{
    Vector2 input = (Vector2){ (float)side, (float)-forward }; // side = horizontal input / forward = vertical input
    if ((side != 0) && (forward != 0)) input = Vector2Normalize(input); // normalizes input

    float delta = GetFrameTime();

    if (!body->isGrounded) body->velocity.y -= GRAVITY*delta;

    if (body->isGrounded && jumpPressed)
    {
        body->velocity.y = JUMP_FORCE;
        body->isGrounded = false;
    }
    Vector3 front = { sinf(rot), 0.f, cosf(rot) };
    Vector3 right = { cosf(rot), 0.f, -sinf(rot) };

    Vector3 wishDir = (Vector3){ input.x*right.x + input.y*front.x, 0.0f, input.x*right.z + input.y*front.z, }; // essentially "wishDir" from quake
    wishDir = Vector3Normalize(wishDir);
    body->dir = wishDir;
    
    Vector3 hvel = {body->velocity.x, 0.0f, body->velocity.z};

    // RUNS ON GROUND
    if (body->isGrounded)
    {
        Vector3 wishveloc = Vector3Scale(body->dir, MAX_SPEED);
        float wishspd = Vector3Length(wishveloc);
        wishveloc = Vector3Normalize(wishveloc);


        // FRICTION
        float speed = Vector3Length(hvel);

        if (speed > 0.001f)
        {
            float friction = FRICTION;

            if (crouchHold)
                friction *= 2.0f;

            float control = speed < STOPSPEED ? STOPSPEED : speed;

            float newspeed = speed - delta * control * friction;

            if (newspeed < 0)
                newspeed = 0;

            float scale = newspeed / speed;

            body->velocity.x *= scale;
            body->velocity.z *= scale;
        }


        // ACCELERATION
        hvel = (Vector3){ body->velocity.x, 0.0f, body->velocity.z };

        float currentSpeed = Vector3DotProduct(hvel, wishveloc);

        float addSpeed = wishspd - currentSpeed;

        if (addSpeed > 0)
        {
            float accelSpeed = ACCEL * delta * wishspd;

            if (accelSpeed > addSpeed)
                accelSpeed = addSpeed;

            body->velocity.x += wishveloc.x * accelSpeed;
            body->velocity.z += wishveloc.z * accelSpeed;
        }
    }
    // RUNS ON AIR
    else {
        // does the same as VectorNormalize() in quake's code base
        Vector3 wishveloc = Vector3Scale(body->dir, MAX_SPEED);
        float wishspd = Vector3Length(wishveloc);
        wishveloc = Vector3Normalize(wishveloc);

        // limits the wishspd size
        if (wishspd > 30.0f)
            wishspd = 30.0f;

        float currentSpeed = Vector3DotProduct(hvel, wishveloc);
        float addSpeed = wishspd - currentSpeed;

        // prevents negative add speed
        if (addSpeed > 0)
        {
            float accel = AIR_ACCEL * MAX_SPEED * delta;

            if (accel > addSpeed)
                accel = addSpeed;

            hvel.x += wishveloc.x * accel;
            hvel.z += wishveloc.z * accel;
        }

        body->velocity.x = hvel.x;
        body->velocity.z = hvel.z;
    }
    body->position.x += body->velocity.x * delta;
    body->position.y += body->velocity.y * delta;
    body->position.z += body->velocity.z * delta;

    bool wasGrounded = body->isGrounded;

    if (body->position.y <= 0.0f)
    {
        body->position.y = 0.0f;
        body->velocity.y = 0.0f;
        body->isGrounded = true;
    }

    if (!wasGrounded && body->isGrounded)
    {
        Body_OnLand(body);
    }
}
