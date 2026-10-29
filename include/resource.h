#ifndef RESOURCE_H
#define RESOURCE_H

#include <stdbool.h>

#define RESOURCE_PATH_SIZE 1024

bool Resource_FindFile(const char *mapPath, const char *relative, char *out, int size);
bool Resource_MapBaseName(const char *mapPath, char *out, int size);

#endif
