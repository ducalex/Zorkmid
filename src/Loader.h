#ifndef LOADER_H_INCLUDED
#define LOADER_H_INCLUDED

#define MAGIC_ZCR 0x3152435A
#define MAGIC_ZFS 0x4653465A
#define MAGIC_TGA 0x005A4754
#define MAGIC_RLF 0x524C4546

typedef struct
{
    FILE *fl;
    uint32_t xor_key;
} zfs_arch_t;

typedef struct
{
    uint32_t offset;
    uint32_t size;
    zfs_arch_t *archive;
} zfs_file_t;

typedef struct
{
    int32_t t[4];
    uint32_t j;
} adpcm_context_t;

typedef struct
{
    char *name;
    char *path;
    zfs_file_t *zfs;
} FManNode_t;

typedef struct
{
    char *buf;
    int32_t size;
    int32_t pos;
} mfile_t;

mfile_t *mfopen(FManNode_t *nod);
int32_t mfsize(FManNode_t *nod);
int32_t mftell(mfile_t *fil);
void mfclose(mfile_t *fil);
bool mfeof(mfile_t *fil);
bool mfread(void *buf, int32_t bytes, mfile_t *file);
void mfseek(mfile_t *fil, int32_t pos);
bool mfread_ptr(void **buf, int32_t bytes, mfile_t *file);
char *mfgets(char *str, int32_t num, mfile_t *stream);
void m_wide_to_utf8(mfile_t *file);

Mix_Chunk *loader_LoadChunk(const char *file);
SDL_Surface *loader_LoadFile(const char *file, int8_t transpose);
SDL_Surface *loader_LoadFile_key(const char *file, int8_t transpose, uint32_t key);
anim_surf_t *loader_LoadRlf(const char *file, int8_t transpose, int32_t mask);
char **loader_loadStr(const char *file);
char **loader_loadStr_m(mfile_t *mfp);
void loader_LoadZcr(const char *file, Cursor_t *cur);
void loader_openzfs(const char *file, MList *list);
void adpcm8_decode(void *in, void *out, int8_t stereo, int32_t n, adpcm_context_t *ctx);

void LoadSystemStrings(void);
const char *GetSystemString(int32_t indx);


#endif // LOADER_H_INCLUDED
