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
        int maxplayers = 16;
        int gamemode = 0; // 0 = singleplayer, 1 = deathmatch, 2 = coop
        if (strcmp(argv[1], "server") == 0) {
            if (argc > 2) {
                ip = argv[2];
            }
            if (argc > 3) {
                port = atoi(argv[3]);
            }
            if (argc > 4) {
                maxplayers = atoi(argv[4]);
            }
            if (argc > 5) {
                gamemode = atoi(argv[5]);
            }
            printf("starting the server %s:%i with %i and gamemode %i\n", ip, port, maxplayers, gamemode);
            SV_main(ip, port, maxplayers, gamemode);
        }
        else if (strcmp(argv[1], "join") == 0) {
            if (argc > 2) {
                ip = argv[2];
            }
            if (argc > 3) {
                port = atoi(argv[3]);
            }
            printf("connecting to the server on %s:%i\n", ip, port);
            CL_main(ip, port);
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

