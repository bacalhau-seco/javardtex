#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "net.h"
#include "server.h"

void SV_main(void)
{
    char buf[1400];
    int sockfd = NET_StartSocket(PORT);
    if (sockfd < 0) return;

    printf("server opened on port %d\n", PORT);

    for (;;) {
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

