#include "../include/camera.h"
#include "../include/defs.h"

void Camera_Update(Camera *camera,
                   const Body *player,
                   Vector2 lookRotation,
                   float headLerp)
{
    camera->position = (Vector3){
        player->position.x,
        player->position.y + (sv_bottomheight.value + headLerp),
        player->position.z,
    };

    const Vector3 up = { 0.0f, 1.0f, 0.0f };
    const Vector3 forward = { 0.0f, 0.0f, -1.0f };

    Vector3 yaw = Vector3RotateByAxisAngle(forward, up, lookRotation.x);

    Vector3 right = Vector3Normalize(Vector3CrossProduct(yaw, up));

    float pitch = Clamp(-lookRotation.y,
                        -PI/2 + 0.0001f,
                         PI/2 - 0.0001f);

    Vector3 dir = Vector3RotateByAxisAngle(yaw, right, pitch);

    camera->up = up;
    camera->target = Vector3Add(camera->position, dir);
}

void InitCamera(Camera *camera, const Body *player, float headLerp)
{
    *camera = (Camera){0};

    camera->fovy = cl_fov.value;
    camera->projection = CAMERA_PERSPECTIVE;
    camera->position = (Vector3){
        player->position.x,
        player->position.y + (sv_bottomheight.value + headLerp),
        player->position.z,
    };
}
