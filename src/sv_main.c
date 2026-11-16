#include <bits/time.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "net.h"
#include "server.h"

static int64_t get_time_ns() {
    struct timespec t; // time
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (int64_t)t.tv_sec*1000000000+t.tv_nsec;
} 

void SV_main(char *ip, int port, int maxplayers, int gamemode)
{
    char buf[1400];
    char *command[256]; // commands sent by the client
    struct timespec tick;
    uint64_t tc = 0; // tick count
    int playercount = 0;
    int playerexists = 0;
    

    if (maxplayers < 1) {
        fprintf(stderr, "maxplayers must be >= 1\n");
        return;
    }

    int sockfd = NET_StartSocket(ip, port);
    if (sockfd < 0) return;

    client_t slots[maxplayers]; // one slot per allowed player
    for (int i = 0; i < maxplayers; i++) slots[i].state = FREE; // initializes the slots

    for (;;) {
        // TODO: 
        // connect clients to the server slots
        // implement server commands (i think this requires multithreading so the server runs on the background)
        int64_t tf = get_time_ns();
        playerexists = 0;

        struct sockaddr_in from;
        socklen_t fromlen = sizeof(from);
        ssize_t n = recvfrom(sockfd, buf, sizeof(buf), MSG_DONTWAIT, (struct sockaddr *)&from, &fromlen); // gets message from the clients

        // JOIN A SERVER
        if (n != -1) { // if the buffer isnt empty
            for (int i = 0; i < maxplayers; i++) {
                if (slots[i].addr.sin_addr.s_addr == from.sin_addr.s_addr && slots[i].addr.sin_port == from.sin_port ) {
                    printf("player is already present");
                    playerexists = 1; // it seems like i LOVE to make 1 time use variables
                                      // i will find diferent logic for this
                                      // right now its fine ig
                    break;
                }
            }
            if (playerexists == 0) {
                for (int j = 0; j < maxplayers; j++){ 
                    if (slots[j].state == FREE) {
                        slots[j].state = CONNECTED; 
                        slots[j].addr = from; 
                        break;
                    }
                }
            }
        }
        
        // counts the players
        playercount = 0; 
        for (int i = 0; i < maxplayers; i++) {
            if (slots[i].state == CONNECTED) {
                playercount ++;
            }
        }


        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &from.sin_addr, ip, sizeof ip);
        sendto(sockfd, "pong", 4, 0, (struct sockaddr *)&from, fromlen);

        int64_t tl = get_time_ns();
        int64_t delta = tl - tf;
        int64_t remaning = TICK_NS - delta;

        if (remaning > 0) {
            tick.tv_sec = remaning / 1000000000;
            tick.tv_nsec = remaning % 1000000000;
            printf("tick time: %.3f ms | waiting time: %.3f ms | playercount: %i, maxplayers: %i | tick: %lu |\n", delta / 1e6, remaning / 1e6, playercount, maxplayers, tc);
            nanosleep(&tick, NULL);
        }else {
            printf("compra um server novo, seu pobre\n");
        }
        tc ++;
    }
    close(sockfd);
}

