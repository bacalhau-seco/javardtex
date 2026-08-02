typedef struct
{
    const char *name;
    float value;
} Cvar;

extern Cvar cl_fov;
extern Cvar cl_mouseh;
extern Cvar cl_mousev;
extern Cvar cl_maxfps;

void Cvar_Init(void);
