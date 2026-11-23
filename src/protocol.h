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

#endif
