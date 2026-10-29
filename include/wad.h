#ifndef WAD_H
#define WAD_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define WAD_NAME_SIZE 16
#define WAD_LUMP_ENTRY_SIZE 32
#define WAD_MAX_LUMPS 8192
#define WAD_TYPE_MIPTEX 0x43

typedef struct {
    char name[WAD_NAME_SIZE];
    int32_t filepos;
    int32_t disksize;
    uint8_t type;
} WadLump;

typedef struct {
    FILE *file;
    long file_size;
    WadLump *lumps;
    int num_lumps;
} Wad;

bool Wad_Open(Wad *wad, const char *path);
void Wad_Close(Wad *wad);
bool Wad_ReadMipTexture(Wad *wad, const char *name, uint8_t **data, int *size);

#endif
