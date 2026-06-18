#import "raylib.h"

#define GRAVITY         800.0f
#define MAX_SPEED       250.0f
#define JUMP_FORCE      270.0f
#define FRICTION          4.0f
#define STOPSPEED       100.0f
#define STAND_HEIGHT     56.0f
#define BOTTOM_HEIGHT    32.0f
#define CROUCH_HEIGHT    32.0f
#define ACCEL            5.0f
#define AIR_ACCEL        10.0f 

// Body structure
typedef struct {
    Vector3 position;
    Vector3 velocity;
    Vector3 dir;
    bool isGrounded;
} Body;

void UpdateBody(Body *body, float rot, char side, char forward, bool jumpPressed, bool crouchHold);
