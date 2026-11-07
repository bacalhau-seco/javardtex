#include <sys/socket.h>
#include <arpa/inet.h>
#include "net.h"

int NET_StartSocket(int port)
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(sockfd, (struct sockaddr *)&addr, sizeof addr);

    return sockfd;
}
