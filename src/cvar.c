#include "../include/cvar.h"

Cvar cl_fov = {
    .name = "fov",
    .value = 90.0f
};

Cvar cl_mouseh = {
    .name = "horizontal sensitivity",
    .value = 0.001f
};

Cvar cl_mousev = {
    .name = "vertical sensitivity",
    .value = 0.001f
};

Cvar cl_maxfps = {
    .name = "max fps",
    .value = 90.0f
};

void Cvar_Init(void)
{
    // this will initialize cvars
}
