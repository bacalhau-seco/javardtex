typedef struct
{
    const char *name;
    float value;
} Cvar;

extern Cvar cl_fov;
extern Cvar cl_mouseh;
extern Cvar cl_mousev;
extern Cvar cl_maxfps;
extern Cvar cl_hres;
extern Cvar cl_vres;

extern Cvar sv_gravity;
extern Cvar sv_maxspeed;
extern Cvar sv_jumpforce;
extern Cvar sv_friction;
extern Cvar sv_stopspeed;
extern Cvar sv_standheight;
extern Cvar sv_bottomheight;
extern Cvar sv_crouchheight;
extern Cvar sv_accel;
extern Cvar sv_airaccel;
extern Cvar sv_jumppenalty;

void Cvar_Init(void);
