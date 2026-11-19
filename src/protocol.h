#ifndef PROTOCOL_H
#define PROTOCOL_H
// here it will be defined the many packets sent and recieved by the server/client\o
#include <stdint.h>

typedef enum {
    LEAVE = 0,
    JOIN = 1,
} pcktid_t;

typedef struct {
    uint8_t type;
    char name[32];
} pckt_t;
#endif
