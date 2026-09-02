#include "../include/defs.h"

void Body_OnLand(Body *body)
{
    Vector3 hvel = {.x=body->velocity.x, .y=0.0f, .z=body->velocity.z};
    float speed = Vector3Length(hvel);
    
    if (speed > Cvar_Get("sv_jumppenalty")->value)
    {
        hvel = Vector3Scale(hvel, (speed - Cvar_Get("sv_jumppenalty")->value) / speed);

        body->velocity.x = hvel.x;
        body->velocity.z = hvel.z;
    }
}

// Update body considering current world state
void UpdatePlayer(Body *body, float rot, char side, char forward, bool jumpPressed, bool crouchHold, float delta)
{
    Vector2 input = (Vector2){ (float)side, (float)-forward };
    if ((side != 0) && (forward != 0)) input = Vector2Normalize(input);

    if (!body->isGrounded)
        body->velocity.y -= Cvar_Get("sv_gravity")->value * delta;

    if (body->isGrounded && jumpPressed)
    {
        body->velocity.y = Cvar_Get("sv_jumpforce")->value;
        body->isGrounded = false;
    }

    Vector3 front = { sinf(rot), 0.f, cosf(rot) };
    Vector3 right = { cosf(rot), 0.f, -sinf(rot) };

    Vector3 wishDir = (Vector3){
        input.x * right.x + input.y * front.x,
        0.0f,
        input.x * right.z + input.y * front.z,
    };

    wishDir = Vector3Normalize(wishDir);
    body->dir = wishDir;
    
    Vector3 hvel = {body->velocity.x, 0.0f, body->velocity.z};

    // RUNS ON GROUND
    if (body->isGrounded)
    {
        Vector3 wishveloc = Vector3Scale(body->dir, Cvar_Get("sv_maxspeed")->value);
        float wishspd = Vector3Length(wishveloc);
        wishveloc = Vector3Normalize(wishveloc);

        // FRICTION
        float speed = Vector3Length(hvel);

        if (speed > 0.001f)
        {
            float friction = Cvar_Get("sv_friction")->value;

            if (crouchHold)
                friction *= 2.0f;

            float control = speed < Cvar_Get("sv_stopspeed")->value
                ? Cvar_Get("sv_stopspeed")->value
                : speed;

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
            float accelSpeed = Cvar_Get("sv_accel")->value * delta * wishspd;

            if (accelSpeed > addSpeed)
                accelSpeed = addSpeed;

            body->velocity.x += wishveloc.x * accelSpeed;
            body->velocity.z += wishveloc.z * accelSpeed;
        }
    }
    // RUNS ON AIR
    else
    {
        Vector3 wishveloc = Vector3Scale(body->dir, Cvar_Get("sv_maxspeed")->value);
        float wishspd = Vector3Length(wishveloc);
        wishveloc = Vector3Normalize(wishveloc);

        // limits the wishspd size
        if (wishspd > 30.0f)
            wishspd = 30.0f;

        float currentSpeed = Vector3DotProduct(hvel, wishveloc);
        float addSpeed = wishspd - currentSpeed;

        if (addSpeed > 0)
        {
            float accel = Cvar_Get("sv_airaccel")->value
                * Cvar_Get("sv_maxspeed")->value * delta;

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
