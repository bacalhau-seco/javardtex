#ifndef CVAR_H
#define CVAR_H

typedef struct
{
    const char *name;
    float value;
} Cvar;

void Cvar_Init(void);
Cvar *Cvar_Register(const char *name, float value);
Cvar *Cvar_Get(const char *name);

#endif
