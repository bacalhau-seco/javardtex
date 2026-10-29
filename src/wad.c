#include "../include/wad.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>

static inline int32_t ReadInt32(const uint8_t *data)
{
    return (int32_t)((uint32_t)data[0]
        | ((uint32_t)data[1] << 8)
        | ((uint32_t)data[2] << 16)
        | ((uint32_t)data[3] << 24));
}

static inline long FileSize(FILE *file)
{
    long size;

    if (fseek(file, 0, SEEK_END) != 0)
        return -1;

    size = ftell(file);

    if (fseek(file, 0, SEEK_SET) != 0)
        return -1;

    return size;
}

static bool ReadLumpTable(Wad *wad, int32_t count, int32_t table_offset)
{
    uint8_t *table;
    int i;

    if (count <= 0 || count > WAD_MAX_LUMPS)
        return false;

    if (table_offset < 12 || (long)table_offset + (long)count * WAD_LUMP_ENTRY_SIZE > wad->file_size)
        return false;

    table = malloc((size_t)count * WAD_LUMP_ENTRY_SIZE);

    if (!table)
        return false;

    if (fseek(wad->file, table_offset, SEEK_SET) != 0
        || fread(table, WAD_LUMP_ENTRY_SIZE, (size_t)count, wad->file) != (size_t)count)
    {
        free(table);
        return false;
    }

    wad->lumps = calloc((size_t)count, sizeof(WadLump));

    if (!wad->lumps)
    {
        free(table);
        return false;
    }

    for (i = 0; i < count; i++)
    {
        const uint8_t *entry = table + (size_t)i * WAD_LUMP_ENTRY_SIZE;
        WadLump *lump = &wad->lumps[i];

        lump->filepos = ReadInt32(entry);
        lump->disksize = ReadInt32(entry + 4);
        lump->type = entry[12];

        memcpy(lump->name, entry + 16, WAD_NAME_SIZE);
        lump->name[WAD_NAME_SIZE - 1] = '\0';
    }

    wad->num_lumps = count;

    free(table);

    return true;
}

bool Wad_Open(Wad *wad, const char *path)
{
    uint8_t header[12];

    if (!wad || !path)
        return false;

    memset(wad, 0, sizeof(*wad));

    wad->file = fopen(path, "rb");

    if (!wad->file)
        return false;

    wad->file_size = FileSize(wad->file);

    if (wad->file_size < (long)sizeof(header)
        || fread(header, 1, sizeof(header), wad->file) != sizeof(header)
        || memcmp(header, "WAD3", 4) != 0
        || !ReadLumpTable(wad, ReadInt32(header + 4), ReadInt32(header + 8)))
    {
        Wad_Close(wad);
        return false;
    }

    return true;
}

void Wad_Close(Wad *wad)
{
    if (!wad)
        return;

    if (wad->file)
        fclose(wad->file);

    free(wad->lumps);
    memset(wad, 0, sizeof(*wad));
}

bool Wad_ReadMipTexture(Wad *wad, const char *name, uint8_t **data, int *size)
{
    int i;

    if (!wad || !wad->file || !name || !data || !size)
        return false;

    for (i = 0; i < wad->num_lumps; i++)
    {
        const WadLump *lump = &wad->lumps[i];
        uint8_t *bytes;

        if (lump->type != WAD_TYPE_MIPTEX || strcasecmp(lump->name, name) != 0)
            continue;

        if (lump->disksize <= 0 || lump->filepos < 0
            || (long)lump->filepos + (long)lump->disksize > wad->file_size)
            return false;

        bytes = malloc((size_t)lump->disksize);

        if (!bytes)
            return false;

        if (fseek(wad->file, lump->filepos, SEEK_SET) != 0
            || fread(bytes, 1, (size_t)lump->disksize, wad->file) != (size_t)lump->disksize)
        {
            free(bytes);
            return false;
        }

        *data = bytes;
        *size = lump->disksize;

        return true;
    }

    return false;
}
