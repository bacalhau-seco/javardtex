#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "server.h"
#include "client.h"

int main(int argc, char *argv[])
{
    if (argc > 1) {
        char *ip = "127.0.0.1";
        int port = 6969;
        int playercount = 16;
        int gamemode = 0; // 0 = singleplayer, 1 = deathmatch, 2 = coop
        char *arg = argv[1];
        if (strcmp(argv[1], "server") == 0) {
            if(argv[2]!= NULL) {
                ip = argv[2];
            }
            if(argv[3]!= NULL){
                port = atoi(argv[3]);
            }
            if(argv[4]!= NULL){
                playercount = atoi(argv[4]);
            }
            if(argv[5]!= NULL){
                gamemode = atoi(argv[5]);
            }
            SV_main(ip, port, playercount, gamemode);   
            printf("starting the server %s:%i with %i and gamemode %i\n", ip, port, playercount, gamemode);

        }
        else if (strcmp(argv[1], "join") == 0) {
            if (argv[2]!= NULL) {
                ip = argv[2];
            }
            if(argv[3]!= NULL){
                port = atoi(argv[3]);
            }
            CL_main(ip, port);
            printf("connecting to the server on %s:%i\n", ip, port);
        }
        else {
            printf("server usage: server ip port playercount gamemode\n");
            printf("client usage: join ip port\n");
        }
    }else {
        const char *addr = "127.0.0.1";
        const int port = 6969;
        CL_main(addr, port);
    }
    return 0;
}

