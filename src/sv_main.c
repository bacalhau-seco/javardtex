#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>

#include "net.h"
#include "server.h"

void SV_main(void)
{
    char buf[1400];
    int sockfd = NET_StartSocket(PORT);
    if (sockfd < 0) return;

    printf("server opened on port %d\n", PORT);

    for (;;) {
        ssize_t n = recv(sockfd, buf, sizeof buf, 0);
        printf("%.*s\n", (int)n, buf);
    }
    close(sockfd);
}

