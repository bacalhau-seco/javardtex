#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "net.h"
#include "server.h"

void SV_main(char *ip, int port, int maxplayers, int gamemode)
{
    char buf[1400];
    char *command[256]; // commands sent by the client

    if (maxplayers < 1) {
        fprintf(stderr, "maxplayers must be >= 1\n");
        return;
    }

    int sockfd = NET_StartSocket(ip, port);
    if (sockfd < 0) return;

    client_t slots[maxplayers]; // one slot per allowed player
    for (int i = 0; i < maxplayers; i++) slots[i].state = FREE; // initializes the slots

    printf("server opened on: %s:%d\n", ip, port);
    printf("server opened with maximum of %d players\n", maxplayers);
    printf("server opened with gamemode: %d\n", gamemode);


    for (;;) {
        // TODO: 120hz tick system
        // connect clients to the server
        // implement server commands (i think this requires multithreading so the server runs on the background)

        struct sockaddr_in from;
        socklen_t fromlen = sizeof(from);
        ssize_t n = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&from, &fromlen);
        if (n < 0) {
            perror("recvfrom");
            break;
        }

        printf("%.*s", (int)n, buf); // basically what i said in CL_init
                                             //
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &from.sin_addr, ip, sizeof ip);
        printf("from %s:%d\n", ip, ntohs(from.sin_port));
        sendto(sockfd, "pong", 4, 0, (struct sockaddr *)&from, fromlen);

        // HOW TO DO THE TICKRATE SYSTEM:
        // i need to calculate the time it takes to comeplete this for loop
        // then wait the rest of the time to complete a 128Hz tick
        // if this value is then negative idk ig could print an error saying how behind it is or some shit
    }
    close(sockfd);
}

void SV_connect() {

}
