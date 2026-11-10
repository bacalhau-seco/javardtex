#ifndef NET_H
#define NET_H

#include <arpa/inet.h>

// here i want to define the packets used by the network

// TODO: peer2peer chalange to validade players 

typedef enum {FREE, CONNECTED} clstate_t; // possible client slot states
                                          // goal here is that the server will keep track of
                                          // which slots are free, which ones have players!
typedef struct {
    clstate_t state;
    struct sockaddr_in addr;
    char name[32];
    double lastmsg; // timestamp of the last time client was pinged
} client_t ; // this represents each server slot

int NET_StartSocket(int port);

#endif
