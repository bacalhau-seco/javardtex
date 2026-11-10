#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "net.h"
#include "server.h"

static client_t slots[MAXPLAYERS]; // this defines the slots used

void SV_main(char *ip, int port, int playercount, int gamemode)
{
    char buf[1400];
    int sockfd = NET_StartSocket(port);
    if (sockfd < 0) return;

    client_t slots[playercount] = {}; // initializes the slots

    printf("server opened on: %s:%d\n", ip, port);
    printf("server opened with maximum of %d players\n", playercount);
    printf("server opened with gamemode: %d\n", gamemode);



    for (;;) {
        // TODO: 120hz tick system
        // connect clients to the server
        // implement server commands (i think this requires multithreading so the server runs on the background)



        struct sockaddr_in from;
        socklen_t fromlen = sizeof(from);
        ssize_t n = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&from, &fromlen);
        printf("%.*s", (int)n, buf); // basically what i said in CL_init
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &from.sin_addr, ip, sizeof ip);
        printf("from %s:%d\n", ip, ntohs(from.sin_port));
        sendto(sockfd, "pong", 4, 0, (struct sockaddr *)&from, fromlen);
    }
    close(sockfd);
}

void SV_connect() {

}
