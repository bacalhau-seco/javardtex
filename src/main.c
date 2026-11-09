#include <stdio.h>
#include <string.h>
#include "server.h"
#include "client.h"

int main(int argc, char *argv[])
{
    if (argc > 1) {
        if (strcmp(argv[1], "server") == 0) {
            printf("starting the server...\n");
            SV_main();
        }
    }else {
        const char *addr = "127.0.0.1";
        const int port = 6969;
        CL_main(addr, port);
    }
    return 0;
}

