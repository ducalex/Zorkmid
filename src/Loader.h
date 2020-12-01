#ifndef LOADER_H_INCLUDED
#define LOADER_H_INCLUDED

#define MAGIC_ZCR 0x3152435A
#define MAGIC_ZFS 0x4653465A
#define MAGIC_TGA 0x005A4754
#define MAGIC_RLF 0x524C4546
#define MAGIC_SAV 0x47534E5A

typedef struct
{
    FILE *fl;
    uint32_t xor_key;
} zfs_arch_t;

typedef struct
{
    char *name;
    char *path;
    size_t size;
    struct {
        zfs_arch_t *archive;
        size_t offset;
    } zfs;
} FManNode_t;

typedef struct
{
    char *buffer;
    size_t size;
    size_t pos;
    FILE *fp;
    FManNode_t *node;
} mfile_t;

void Loader_Init(const char *dir);
const char *Loader_FindFile(const char *name);
TTF_Font *Loader_LoadFont(const char *name, int size);
Mix_Chunk *Loader_LoadSound(const char *file);
SDL_Surface *Loader_LoadGFX(const char *file, bool transpose, int32_t key_555);
anim_surf_t *Loader_LoadRLF(const char *file, bool transpose, int32_t mask_555);
char **Loader_LoadSTR(const char *file);
void Loader_LoadZCR(const char *file, Cursor_t *cur);

mfile_t *mfopen(const char *filename);
mfile_t *mfopen_txt(const char *filename);
const void *mfbuffer(mfile_t *file);
void mfclose(mfile_t *file);
bool mfeof(mfile_t *file);
int32_t mftell(mfile_t *file);
void mfseek(mfile_t *file, size_t pos);
bool mfread(void *buf, size_t bytes, mfile_t *file);
char *mfgets(char *buf, size_t max_bytes, mfile_t *file);

#endif // LOADER_H_INCLUDED
