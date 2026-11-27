#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <stdint.h>

typedef enum {
    LEAVE = 0,
    JOIN = 1,
    MOVE = 2, 
    SNAPSHOT = 3,
} pcktid_t;

typedef struct {
    uint8_t type;
    char name[32];
} pckt_join_t;

typedef struct {
    uint8_t type;
} pckt_leave_t;

typedef struct {
   uint8_t  type;
   uint8_t  buttons; // buttons pressed (jump, crouch and whatever else i add in the future)
   int8_t   side; // goes from 1 to -1
   int8_t   forwrd; // same as above
   uint32_t seq; // the order of the packet
   float    yaw;  // camera shit
   float    pitch;
} pckt_move_t;

#endif
