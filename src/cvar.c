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

void Cvar_Init(void)
{
    Cvar_Register("cl_fov", 90.0f);
    Cvar_Register("cl_mouseh", 0.001f);
    Cvar_Register("cl_mousev", 0.001f);
    Cvar_Register("cl_maxfps", 90.0f);
    Cvar_Register("cl_hres", 1280.0f);
    Cvar_Register("cl_vres", 960.0f);

    Cvar_Register("sv_gravity", 800.0f);
    Cvar_Register("sv_maxspeed", 290.0f);
    Cvar_Register("sv_jumpforce", 270.0f);
    Cvar_Register("sv_friction", 4.0f);
    Cvar_Register("sv_stopspeed", 100.0f);
    Cvar_Register("sv_standheight", 56.0f);
    Cvar_Register("sv_bottomheight", 32.0f);
    Cvar_Register("sv_crouchheight", 32.0f);
    Cvar_Register("sv_accel", 5.0f);
    Cvar_Register("sv_airaccel", 10.0f);
    Cvar_Register("sv_jumppenalty", 0.0f);
}
