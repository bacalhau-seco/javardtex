#include "../include/camera.h"
#include "../include/collision.h"
#include "../include/defs.h"

static inline Vector3 EyePosition(const Body *player)
{
    return (Vector3){
        player->position.x,
        player->position.y + Collision_EyeHeight(player->isCrouched) - player->stepViewOffset,
        player->position.z,
    };
}

void Camera_Update(Camera *camera,
                   const Body *player,
                   Vector2 lookRotation)
{
    camera->position = EyePosition(player);

    const Vector3 up = { 0.0f, 1.0f, 0.0f };
    const Vector3 forward = { 0.0f, 0.0f, -1.0f };

    Vector3 yaw = Vector3RotateByAxisAngle(forward, up, lookRotation.x);

    Vector3 right = Vector3Normalize(Vector3CrossProduct(yaw, up));

    float pitch = -lookRotation.y;

    Vector3 dir = Vector3RotateByAxisAngle(yaw, right, pitch);

    camera->up = up;
    camera->target = Vector3Add(camera->position, dir);
}

void InitCamera(Camera *camera, const Body *player)
{
    *camera = (Camera){0};

    camera->fovy = Cvar_Get("fov")->value;
    camera->projection = CAMERA_PERSPECTIVE;
    camera->position = EyePosition(player);
}
