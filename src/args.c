#include "../include/defs.h"
#include "../include/args.h"

#include <stdio.h>
#include <string.h>

#define ARGS_DEFAULT_MAP "game/maps/test.bsp"

static bool Args_StringEndsWith(const char *text, const char *suffix)
{
    if (text == NULL || suffix == NULL)
        return false;

    size_t textLength = strlen(text);
    size_t suffixLength = strlen(suffix);

    if (suffixLength > textLength)
        return false;

    return strcmp(text + (textLength - suffixLength), suffix) == 0;
}

static bool Args_SetMapPath(Args *args, const char *value)
{
    if (args == NULL || value == NULL || value[0] == '\0')
        return false;

    bool hasDirectory = (strchr(value, '/') != NULL);
    bool hasExtension = Args_StringEndsWith(value, ".bsp");

    int written;

    if (hasDirectory || hasExtension)
        written = snprintf(args->mapPath, sizeof(args->mapPath), "%s", value);
    else
        written = snprintf(args->mapPath, sizeof(args->mapPath), "game/maps/%s.bsp", value);

    if (written < 0 || (size_t)written >= sizeof(args->mapPath))
    {
        Args_Init(args);
        return false;
    }

    return true;
}

void Args_Init(Args *args)
{
    if (args == NULL)
        return;

    snprintf(args->mapPath, sizeof(args->mapPath), "%s", ARGS_DEFAULT_MAP);
}

void Args_PrintUsage(const char *exeName)
{
    const char *name = (exeName != NULL && exeName[0] != '\0') ? exeName : "javardtex";

    printf("usage: %s [options]\n", name);
    printf("  -map <name|path>   load the given map (default: %s)\n", ARGS_DEFAULT_MAP);
    printf("  -help              show this message\n");
}

bool Args_Parse(Args *args, int argc, char **argv)
{
    if (args == NULL)
        return false;

    Args_Init(args);

    if (argc <= 1 || argv == NULL)
        return true;

    const char *exeName = (argv[0] != NULL) ? argv[0] : "javardtex";

    for (int i = 1; i < argc; i++)
    {
        const char *option = argv[i];

        if (option == NULL)
            continue;

        if (strcmp(option, "-map") == 0)
        {
            if (i + 1 >= argc || argv[i + 1] == NULL)
            {
                printf("error: -map requires a value\n");
                Args_PrintUsage(exeName);
                return false;
            }

            if (!Args_SetMapPath(args, argv[i + 1]))
            {
                printf("error: invalid map '%s'\n", argv[i + 1]);
                return false;
            }

            i++;
            continue;
        }

        if (strcmp(option, "-help") == 0 || strcmp(option, "--help") == 0 || strcmp(option, "-h") == 0)
        {
            Args_PrintUsage(exeName);
            return false;
        }

        printf("error: unknown option '%s'\n", option);
        Args_PrintUsage(exeName);
        return false;
    }

    return true;
}
