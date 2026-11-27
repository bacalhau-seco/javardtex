#include <bits/time.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "net.h"
#include "server.h"
#include "protocol.h"

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
        int64_t tf = get_time_ns();
        playerexists = 0;

        struct sockaddr_in from;
        socklen_t fromlen = sizeof(from);
        ssize_t n = recvfrom(sockfd, buf, sizeof(buf), MSG_DONTWAIT, (struct sockaddr *)&from, &fromlen); // gets message from the clients

        // JOIN A SERVER
        if (n != -1) { // if the buffer isnt empty
            switch ((uint8_t)buf[0]) {
                case JOIN:
                    pckt_join_t j;
                    memcpy(&j, buf, sizeof(j));
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
                        for (int i = 0; i < maxplayers; i++){ 
                            if (slots[i].state == FREE) {
                                slots[i].state = CONNECTED; 
                                slots[i].addr = from; 
                                snprintf(slots[i].name, sizeof slots[i].name, "%.*s", (int)sizeof j.name, j.name);
                                slots[i].lastmsg = tf;
                                break;
                            }
                        }
                    }

                    break;
                char ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &from.sin_addr, ip, sizeof ip);
                sendto(sockfd, "welcome player", 4, 0, (struct sockaddr *)&from, fromlen);
            
            }
        }
        
        // counts the players
        playercount = 0; 
        for (int i = 0; i < maxplayers; i++) {
            if (slots[i].state == CONNECTED) {
                playercount ++;
            }
        }

        for (int i = 0; i < maxplayers; i++) {
            if ( (tf - slots[i].lastmsg) > 5000000000 ){
                slots[i] = (client_t){0}; // bruh i could just have done this from the start
            }

        }

        // BELLOW THIS JUST ENDS THE TICK
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

