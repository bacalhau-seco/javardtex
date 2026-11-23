#include "protocol.h"
#include "net.h"
#include "client.h"
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>

void CL_main(const char *addr, const int port) {
    char buf[1400];
    pckt_join_t join = {.type = JOIN, .name = "player"};
    int sockfd = NET_StartSocket(NULL, 0);
    if (sockfd < 0) return;
    printf("connecting to %s:%d\n", addr, port);

    // connect to the server
    struct sockaddr_in server = {0}; // idk what the 0 is btw
    server.sin_family = AF_INET; // use ipv4
    server.sin_port = htons(port); //  connects the specified port
    if (inet_pton(AF_INET, addr, &server.sin_addr) != 1) { // write the set IP to server.sin_addr so it can connect later in the code
        fprintf(stderr, "invalid server address: %s\n", addr);
        close(sockfd);
        return;
    }
    
    if (connect(sockfd, (struct sockaddr *) &server, sizeof(server)) < 0 ) { // connects to local socket which is pointing to the server
        perror("connect");
        close(sockfd);
        return;
    }

    send(sockfd, &join, sizeof(join), 0); // joins the game

    for (;;) { // TODO: actual game loop

        size_t n = recv(sockfd, buf, sizeof(buf), MSG_DONTWAIT); // recieves data from the server
        if (n>1) {
            switch ((uint8_t)buf[0]) { // will get the packet type
                case SNAPSHOT:
                    printf("snapshot recieved");
                    break;
            
            }
//            printf("%.*s", (int)n, buf); // basically this will print exactly the lenght of buf
                                                 // so it will read the precision size as n and print buf
                                                 // vim is so cool that it lets me right comments
                                                 // like this
                                                 // now this is a big ass comment

        }
    }
    close(sockfd);
}
