#include "net.h"
#include "client.h"
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>

void CL_main(const char *addr, const int port) {
    char *packet = "wassup niggers";
    int sockfd = NET_StartSocket(0);
    printf("connecting to %s\n", addr);

    // connect to the server
    struct sockaddr_in server = {0}; // idk what the 0 is btw
    server.sin_family = AF_INET; // use ipv4
    server.sin_port = htons(port); //  connects the specified port
    
    if (connect(sockfd, (struct sockaddr *) &server, sizeof(server)) < 0 ) { // handling errors
        perror("connect");
        close(sockfd);
        return;
    }

    send(sockfd, packet, strlen(packet), 0);
    close(sockfd);
}
