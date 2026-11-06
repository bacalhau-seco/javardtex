#ifndef ENTITY_H
#define ENTITY_H

#include <stdbool.h>

#include "raylib.h"
#include "bsp.h"

#define ENTITY_MAX 512
#define ENTITY_NAME_SIZE 64

/* Builds the runtime entity table from the map entity lump. */
void Entity_Load(const BSP_Map *map);

/* Runs touch tests for the player and the pending/delayed fires. */
void Entity_Update(const BSP_Map *map, Vector3 feet, bool crouched, float delta);

/* Fires every entity with this targetname (target chains, scripting). */
void Entity_Fire(const char *targetname);

/* Consumes a queued level change, if any. */
bool Entity_TakeChangeLevel(char *out, int size);

#endif
