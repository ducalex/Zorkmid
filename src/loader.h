#ifndef LOADER_H_INCLUDED
#define LOADER_H_INCLUDED

Mix_Chunk *loader_LoadChunk(const char *file);
SDL_Surface *loader_LoadFile(const char *file, int8_t transpose);
SDL_Surface *loader_LoadFile(const char *file, int8_t transpose, uint32_t key);
anim_surf *loader_LoadRlf(const char *file, int8_t transpose, int32_t mask);
void loader_LoadMouseCursor(const char *file, Cursor_t *cur);

struct zfs_arch
{
    FILE *fl;
    uint32_t xor_key;
};

struct zfs_file
{
    uint32_t offset;
    uint32_t size;
    zfs_arch *archive;
};

void loader_openzfs(const char *file, MList *list);
void *loader_zload(zfs_file *fil);

struct adpcm_context
{
    int32_t t[4];
    uint32_t j;
};

void adpcm8_decode(void *in, void *out, int8_t stereo, int32_t n, adpcm_context *ctx);

#endif // LOADER_H_INCLUDED
