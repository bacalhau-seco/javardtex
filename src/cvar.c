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

Cvar cl_hres = {
    .name = "width",
    .value = 1280
};

Cvar cl_vres = {
    .name = "height",
    .value = 960
};

Cvar sv_gravity = {
    .name = "gravity",
    .value = 800.0f
};

Cvar sv_maxspeed = {
    .name = "max speed",
    .value = 290.0f
};

Cvar sv_jumpforce = {
    .name = "jump force",
    .value = 270.0f
};

Cvar sv_friction = {
    .name = "friction",
    .value = 4.0f
};

Cvar sv_stopspeed = {
    .name = "stop speed",
    .value = 100.0f
};

Cvar sv_standheight = {
    .name = "stand height",
    .value = 56.0f
};

Cvar sv_bottomheight = {
    .name = "bottom height",
    .value = 32.0f
};

Cvar sv_crouchheight = {
    .name = "crouch height",
    .value = 32.0f
};

Cvar sv_accel = {
    .name = "acceleration",
    .value = 5.0f
};

Cvar sv_airaccel = {
    .name = "air acceleration",
    .value = 10.0f
};

Cvar sv_jumppenalty = {
    .name = "jump penalty",
    .value = 0.0f
};

void Cvar_Init(void)
{
    // this will initialize cvars
}
