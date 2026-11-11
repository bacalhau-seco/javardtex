#include <stdio.h>
#include <stdlib.h>
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

    client_t *slots = calloc(maxplayers, sizeof *slots); // one slot per allowed player, FREE == 0
    if (slots == NULL) {
        perror("calloc");
        close(sockfd);
        return;
    }

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
    }
    close(sockfd);
    free(slots);
}

void SV_connect() {

}
