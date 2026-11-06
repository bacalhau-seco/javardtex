#include "../include/entity.h"
#include "../include/collision.h"

#include <stdio.h>
#include <string.h>

#define ENTITY_DEFAULT_WAIT 0.2f

typedef enum {
    ENTITY_KIND_CHANGELEVEL,
    ENTITY_KIND_TRIGGER_ONCE,
    ENTITY_KIND_TRIGGER_MULTIPLE
} EntityKind;

typedef struct {
    EntityKind kind;
    int model;
    Vector3 origin;
    char targetname[ENTITY_NAME_SIZE];
    char target[ENTITY_NAME_SIZE];
    char map[BSP_MAP_NAME_SIZE];
    float delay;
    float wait;
    float cooldown;
    float fireTimer;
    bool firePending;
    bool enabled;
    bool touching;
} Entity;

static Entity entities[ENTITY_MAX];
static int numEntities = 0;
static char pendingMap[BSP_MAP_NAME_SIZE] = { 0 };

static bool KindForClass(const char *classname, EntityKind *kind)
{
    if (strcmp(classname, "func_next_level") == 0 || strcmp(classname, "trigger_changelevel") == 0)
        *kind = ENTITY_KIND_CHANGELEVEL;
    else if (strcmp(classname, "trigger_once") == 0)
        *kind = ENTITY_KIND_TRIGGER_ONCE;
    else if (strcmp(classname, "trigger_multiple") == 0)
        *kind = ENTITY_KIND_TRIGGER_MULTIPLE;
    else
        return false;

    return true;
}

static void ReadString(const BSP_Entity *source, const char *key, char *out, int size)
{
    char value[BSP_ENTITY_VALUE_SIZE];

    out[0] = '\0';

    if (BSP_EntityValue(source, key, value, (int)sizeof(value)))
        snprintf(out, (size_t)size, "%s", value);
}

static float ReadFloat(const BSP_Entity *source, const char *key, float fallback)
{
    char value[BSP_ENTITY_VALUE_SIZE];
    float parsed;

    if (!BSP_EntityValue(source, key, value, (int)sizeof(value)))
        return fallback;

    if (sscanf(value, "%f", &parsed) != 1)
        return fallback;

    return parsed;
}

void Entity_Load(const BSP_Map *map)
{
    BSP_Entity source;
    int index;

    numEntities = 0;
    pendingMap[0] = '\0';

    for (index = 0; BSP_GetEntity(map, index, &source); index++)
    {
        EntityKind kind;
        Entity *entity;

        if (!KindForClass(source.classname, &kind))
            continue;

        if (numEntities >= ENTITY_MAX)
        {
            printf("Entity: table full (%d), ignoring the rest\n", ENTITY_MAX);
            break;
        }

        entity = &entities[numEntities++];

        *entity = (Entity){ 0 };
        entity->kind = kind;
        entity->model = source.model;
        entity->origin = source.origin;
        entity->enabled = true;

        /* The player may spawn inside a trigger: require leaving it first. */
        entity->touching = true;

        ReadString(&source, "targetname", entity->targetname, (int)sizeof(entity->targetname));
        ReadString(&source, "target", entity->target, (int)sizeof(entity->target));
        ReadString(&source, "map", entity->map, (int)sizeof(entity->map));

        entity->delay = ReadFloat(&source, "delay", 0.0f);
        entity->wait = ReadFloat(&source, "wait", ENTITY_DEFAULT_WAIT);
    }
}

static void Activate(Entity *entity)
{
    switch (entity->kind)
    {
    case ENTITY_KIND_CHANGELEVEL:
        if (entity->map[0] != '\0')
            snprintf(pendingMap, sizeof(pendingMap), "%s", entity->map);
        break;

    case ENTITY_KIND_TRIGGER_ONCE:
    case ENTITY_KIND_TRIGGER_MULTIPLE:
        break;
    }

    if (entity->target[0] != '\0')
        Entity_Fire(entity->target);

    if (entity->kind == ENTITY_KIND_TRIGGER_MULTIPLE)
        entity->cooldown = entity->wait;
    else
        entity->enabled = false;
}

static void Trigger(Entity *entity)
{
    if (!entity->enabled || entity->cooldown > 0.0f || entity->firePending)
        return;

    if (entity->delay > 0.0f)
    {
        entity->firePending = true;
        entity->fireTimer = entity->delay;

        return;
    }

    Activate(entity);
}

void Entity_Fire(const char *targetname)
{
    int i;

    if (!targetname || targetname[0] == '\0')
        return;

    for (i = 0; i < numEntities; i++)
    {
        if (strcmp(entities[i].targetname, targetname) == 0)
            Trigger(&entities[i]);
    }
}

void Entity_Update(const BSP_Map *map, Vector3 feet, bool crouched, float delta)
{
    int i;

    for (i = 0; i < numEntities; i++)
    {
        Entity *entity = &entities[i];
        bool touching;

        if (entity->cooldown > 0.0f)
            entity->cooldown -= delta;

        if (entity->firePending)
        {
            entity->fireTimer -= delta;

            if (entity->fireTimer <= 0.0f)
            {
                entity->firePending = false;

                Activate(entity);
            }
        }

        if (entity->model < 0)
            continue;

        touching = Collision_TouchesModel(map, entity->model, entity->origin, feet, crouched);

        if (touching && !entity->touching)
            Trigger(entity);

        entity->touching = touching;
    }
}

bool Entity_TakeChangeLevel(char *out, int size)
{
    if (!out || size <= 0 || pendingMap[0] == '\0')
        return false;

    snprintf(out, (size_t)size, "%s", pendingMap);
    pendingMap[0] = '\0';

    return true;
}
