#ifndef ARGS_H
#define ARGS_H

#include <stdbool.h>

#define ARGS_MAP_PATH_MAX 512

typedef struct Args
{
    char mapPath[ARGS_MAP_PATH_MAX];
    bool hasMap;
} Args;

bool Args_ResolveMapPath(char *dest, int size, const char *value);
void Args_Init(Args *args);
bool Args_Parse(Args *args, int argc, char **argv);
void Args_PrintUsage(const char *exeName);

#endif
