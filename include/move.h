#import "raylib.h"

#define GRAVITY         32.0f
#define MAX_SPEED       20.0f
#define JUMP_FORCE      12.0f
#define FRICTION         0.85f
#define STAND_HEIGHT     1.0f
#define BOTTOM_HEIGHT    0.5f
#define CROUCH_HEIGHT    0.5f
#define ACCEL           90.0f
#define AIR_ACCEL       50.0f 

// Body structure
typedef struct {
    Vector3 position;
    Vector3 velocity;
    Vector3 dir;
    bool isGrounded;
} Body;

void UpdateBody(Body *body, float rot, char side, char forward, bool jumpPressed, bool crouchHold);
