#include "net.h"
#include "client.h"
#include <unistd.h>

void CL_main() {
    int sockfd = NET_StartSocket(0);
    close(sockfd);

}
