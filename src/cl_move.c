#include "defs.h"
//
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
    
    Vector3 hvel = {
        body->velocity.x,
        0.0f,
        body->velocity.z
    };

    // RUNS ON GROUND
    if (body->isGrounded)
    {
        float decel = FRICTION;
        hvel = (Vector3){ body->velocity.x*decel, 0.0f, body->velocity.z*decel };
        float currentSpeed = Vector3DotProduct(hvel, body->dir);
        float addSpeed = MAX_SPEED - currentSpeed;
        float accel = ACCEL * delta;
        if (Vector3Length(hvel) <= 0.1) { hvel = (Vector3) {0}; }
        if (Vector3Length(wishDir) == 0.0f){ accel = 0; }
        if (accel > addSpeed)
            accel = addSpeed;
        if (crouchHold == true) {
            hvel.x += body->dir.x * accel/2;
            hvel.z += body->dir.z * accel/2;
            body->velocity.x = hvel.x/2;
            body->velocity.z = hvel.z/2;
        }
        else {
            hvel.x += body->dir.x * accel;
            hvel.z += body->dir.z * accel;
            body->velocity.x = hvel.x;
            body->velocity.z = hvel.z;
        }
    }
    // RUNS ON AIR
    else {
        // does the same as VectorNormalize() in quake's code base
        Vector3 wishveloc = Vector3Scale(body->dir, MAX_SPEED);
        float wishspd = Vector3Length(wishveloc);
        wishveloc = Vector3Normalize(wishveloc);

        // limits the wishspd size
        if (wishspd > 1)
            wishspd = 1;

        float currentSpeed = Vector3DotProduct(hvel, wishveloc);
        float addSpeed = wishspd - currentSpeed;

        // prevents negative add speed
        if (addSpeed > 0)
        {
            float accel = AIR_ACCEL * wishspd * delta;

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
    if (body->position.y <= 0.0f)
    {
        body->position.y = 0.0f;
        body->velocity.y = 0.0f;
        body->isGrounded = true;
    }
}
