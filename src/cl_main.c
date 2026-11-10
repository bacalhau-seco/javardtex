#include "net.h"
#include "client.h"
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>

void CL_main(const char *addr, const int port) {
    char buf[1400];
    char *packet = "wassup server";
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

    for (;;) {
        send(sockfd, packet, strlen(packet), 0);
        size_t n = recv(sockfd, buf, sizeof(buf), 0);
        printf("%.*s", (int)n, buf); // basically this will print exactly the lenght of buf
                                             // so it will read the precision size as n and print buf
                                             // vim is so cool that it lets me right comments
                                             // like this
                                             // now this is a big ass comment
    }
    close(sockfd);
}
