#include "../include/cvar.h"

#include <stdint.h>
#include <string.h>

#define CVAR_MAX 256

static Cvar cvars[CVAR_MAX];
static int cvar_used[CVAR_MAX];

static uint32_t Cvar_Hash(const char *str)
{
    uint32_t hash = 2166136261u;

    while (*str)
    {
        hash ^= (unsigned char)*str++;
        hash *= 16777619u;
    }

    return hash;
}

Cvar *Cvar_Register(const char *name, float value)
{
    uint32_t index = Cvar_Hash(name) % CVAR_MAX;

    for (int i = 0; i < CVAR_MAX; i++)
    {
        uint32_t pos = (index + i) % CVAR_MAX;

        if (!cvar_used[pos])
        {
            cvars[pos].name = name;
            cvars[pos].value = value;
            cvar_used[pos] = 1;

            return &cvars[pos];
        }

        if (!strcmp(cvars[pos].name, name))
            return &cvars[pos];
    }

    return NULL;
}

Cvar *Cvar_Get(const char *name)
{
    uint32_t index = Cvar_Hash(name) % CVAR_MAX;

    for (int i = 0; i < CVAR_MAX; i++)
    {
        uint32_t pos = (index + i) % CVAR_MAX;

        if (!cvar_used[pos])
            return NULL;

        if (!strcmp(cvars[pos].name, name))
            return &cvars[pos];
    }

    return NULL;
}

float Cvar_Value(const char *name, float fallback)
{
    const Cvar *cvar = Cvar_Get(name);

    return cvar ? cvar->value : fallback;
}


void Cvar_Init(void)
{

    Cvar_Register("gravity", 800.0f);
    Cvar_Register("maxspeed", 290.0f);
    Cvar_Register("jumpforce", 270.0f);
    Cvar_Register("friction", 4.0f);
    Cvar_Register("stopspeed", 100.0f);
    Cvar_Register("crouchspeedscale", 0.333f);
    Cvar_Register("maxvelocity", 2000.0f);
    Cvar_Register("accel", 5.0f);
    Cvar_Register("airaccel", 100.0f);
    Cvar_Register("airwishcap", 30.0f);
    Cvar_Register("jumppenalty", 0.0f);
    Cvar_Register("stepheight", 18.0f);
    Cvar_Register("stepsmooth", 150.0f);
}
