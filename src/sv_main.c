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
    int64_t tc; // tick count
    int playercount;
    

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

        struct sockaddr_in from;
        socklen_t fromlen = sizeof(from);
        ssize_t n = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&from, &fromlen);
        if (n < 0) {
            perror("recvfrom");
            break;
        }

        for (int i = 0; i < maxplayers; i++) {
            if (slots[i].state == CONNECTED) {
                playercount ++;
            }
        }

        printf("%.*s", (int)n, buf); // basically what i said in CL_init

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &from.sin_addr, ip, sizeof ip);
        printf("from %s:%d\n", ip, ntohs(from.sin_port));
        sendto(sockfd, "pong", 4, 0, (struct sockaddr *)&from, fromlen);

        int64_t tl = get_time_ns();
        int64_t delta = tl - tf;
        int64_t remaning = TICK_NS - delta;

        if (remaning > 0) {
            tick.tv_sec = remaning / 1000000000;
            tick.tv_nsec = remaning % 1000000000;
            printf("tick time: %.3f ms | waiting time: %.3f ms\n | playercount: %i, maxplayers: %i", delta / 1e6, remaning / 1e6, playercount, maxplayers);
            nanosleep(&tick, NULL);
        }else {
            printf("compra um server novo, seu pobre\n");
        }
        tc ++;
    }
    close(sockfd);
}

