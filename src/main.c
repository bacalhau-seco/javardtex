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
        if (strcmp(argv[1], "server") == 0) {
            printf("starting the server...\n");
            if (strcmp(argv[2], "") == 0) {
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

        }
        else if (strcmp(argv[1], "join") == 0) {
            printf("connecting to the server...\n");
            if (strcmp(argv[2], "") == 0) {
                ip = argv[2];
            }
            if(argv[3]!= NULL){
                port = atoi(argv[3]);
            }
            CL_main(ip, port);
        }
    }else {
        const char *addr = "127.0.0.1";
        const int port = 6969;
        CL_main(addr, port);
    }
    return 0;
}

