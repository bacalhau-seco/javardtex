#include <stdio.h>
#include <string.h>
#include "server.h"

int main(int argc, char *argv[])
{
    if (argc > 1) {
        if (strcmp(argv[1], "server") == 0) {
            printf("starting the server...\n");
            SV_main();
        }
    }else {
        printf("starting the clinet");
    }
    return 0;
}
