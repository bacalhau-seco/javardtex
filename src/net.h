#ifndef NET_H
#define NET_H

#define PORT 6969

/* port 0 = ephemeral, for the client */
int NET_StartSocket(int port);

#endif
