#include "../include/resource.h"

#include <stdio.h>
#include <string.h>

#include <raylib.h>

static inline bool CopyMapDirectory(const char *mapPath, char *out, int size)
{
    const char *slash = strrchr(mapPath, '/');
    int length;

    if (!slash)
    {
        if (size < 2)
            return false;

        out[0] = '.';
        out[1] = '\0';

        return true;
    }

    length = (int)(slash - mapPath);

    if (length <= 0)
        length = 1;

    if (length + 1 > size)
        return false;

    memcpy(out, mapPath, (size_t)length);
    out[length] = '\0';

    return true;
}

static inline bool StripMapsDirectory(const char *directory, char *out, int size)
{
    const char *slash = strrchr(directory, '/');
    const char *leaf = slash ? slash + 1 : directory;
    int length;

    if (strcmp(leaf, "maps") != 0)
        return false;

    if (!slash)
    {
        if (size < 2)
            return false;

        out[0] = '.';
        out[1] = '\0';

        return true;
    }

    length = (int)(slash - directory);

    if (length <= 0)
        length = 1;

    if (length + 1 > size)
        return false;

    memcpy(out, directory, (size_t)length);
    out[length] = '\0';

    return true;
}

bool Resource_FindFile(const char *mapPath, const char *relative, char *out, int size)
{
    char directory[RESOURCE_PATH_SIZE];
    char root[RESOURCE_PATH_SIZE];

    if (!mapPath || !relative || !out || size <= 0)
        return false;

    if (!CopyMapDirectory(mapPath, directory, (int)sizeof(directory)))
        return false;

    if (StripMapsDirectory(directory, root, (int)sizeof(root)))
    {
        if (snprintf(out, (size_t)size, "%s/%s", root, relative) < size && FileExists(out))
            return true;
    }

    if (snprintf(out, (size_t)size, "%s/%s", directory, relative) < size && FileExists(out))
        return true;

    out[0] = '\0';

    return false;
}

bool Resource_MapBaseName(const char *mapPath, char *out, int size)
{
    const char *slash;
    const char *leaf;
    const char *dot;
    int length;

    if (!mapPath || !out || size <= 0)
        return false;

    slash = strrchr(mapPath, '/');
    leaf = slash ? slash + 1 : mapPath;
    dot = strrchr(leaf, '.');
    length = dot ? (int)(dot - leaf) : (int)strlen(leaf);

    if (length <= 0 || length + 1 > size)
        return false;

    memcpy(out, leaf, (size_t)length);
    out[length] = '\0';

    return true;
}
