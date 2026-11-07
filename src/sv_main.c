#include <stdio.h>
#include "net.h"
#include "server.h"

void SV_main(void)
{
    int sockfd = NET_StartSocket(PORT);
    printf("server opened on port %d\n", PORT);
}
