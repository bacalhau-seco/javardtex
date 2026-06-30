#include "../include/defs.h"

void Camera_Update(Camera *camera, Vector2 lookRotation)
{
    const Vector3 up = (Vector3){ 0.0f, 1.0f, 0.0f };
    const Vector3 targetOffset = (Vector3){ 0.0f, 0.0f, -1.0f };

    // Left and right
    Vector3 yaw = Vector3RotateByAxisAngle(targetOffset, up, lookRotation.x);

    // Up and down
    Vector3 right = Vector3Normalize(Vector3CrossProduct(yaw, up));

    // Rotate view vector around right axis
    float pitchAngle = -lookRotation.y;
    pitchAngle = Clamp(pitchAngle, -PI/2 + 0.0001f, PI/2 - 0.0001f);

    Vector3 pitch = Vector3RotateByAxisAngle(yaw, right, pitchAngle);

    camera->up = up;
    camera->target = Vector3Add(camera->position, pitch);
}
