#include "../include/cvar.h"

Cvar cl_fov = {
    .name = "cl_fov",
    .value = 90.0f
};

Cvar cl_mouseh = {
    .name = "cl_mouseh",
    .value = 0.001f
};

Cvar cl_mousev = {
    .name = "cl_mousev",
    .value = 0.001f
};

Cvar cl_maxfps = {
    .name = "cl_maxfps",
    .value = 90.0f
};

Cvar cl_hres = {
    .name = "cl_hres",
    .value = 1280
};

Cvar cl_vres = {
    .name = "cl_vres",
    .value = 960
};

Cvar sv_gravity = {
    .name = "sv_gravity",
    .value = 800.0f
};

Cvar sv_maxspeed = {
    .name = "sv_maxspeed",
    .value = 290.0f
};

Cvar sv_jumpforce = {
    .name = "sv_jumpforce",
    .value = 270.0f
};

Cvar sv_friction = {
    .name = "sv_friction",
    .value = 4.0f
};

Cvar sv_stopspeed = {
    .name = "sv_stopspeed",
    .value = 100.0f
};

Cvar sv_standheight = {
    .name = "sv_standheight",
    .value = 56.0f
};

Cvar sv_bottomheight = {
    .name = "sv_bottomheight",
    .value = 32.0f
};

Cvar sv_crouchheight = {
    .name = "sv_crouchheight",
    .value = 32.0f
};

Cvar sv_accel = {
    .name = "sv_accel",
    .value = 5.0f
};

Cvar sv_airaccel = {
    .name = "sv_airaccel",
    .value = 10.0f
};

Cvar sv_jumppenalty = {
    .name = "sv_jumppenalty",
    .value = 0.0f
};

void Cvar_Init(void)
{
    // register cvars here
}
