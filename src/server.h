#ifndef SERVER_H
#define SERVER_H

#define PORT 6969
#define MAXPLAYERS 32
#define IP "127.0.0.1"
#define GAMEMODE 0 // 0 = singleplayer, 1 = deathmatch, 2 = coop

void SV_main(char *ip, int port, int playercount, int gamemode);
void SV_Connect(void);

#endif

