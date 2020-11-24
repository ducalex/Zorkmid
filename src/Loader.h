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

typedef struct BinTreeNd
{
    struct BinTreeNd *zero;
    struct BinTreeNd *one;
    FManNode_t *nod;
} BinTreeNd_t;

typedef struct
{
    char *buf;
    int32_t size;
    int32_t pos;
} mfile_t;

void Loader_Init(const char *dir);
FManNode_t *Loader_FindNode(const char *chr);
const char *Loader_GetPath(const char *chr);
TTF_Font *Loader_LoadFont(char *name, int size);
Mix_Chunk *Loader_LoadChunk(const char *file);
SDL_Surface *Loader_LoadFile(const char *file, int8_t transpose);
SDL_Surface *Loader_LoadFile_key(const char *file, int8_t transpose, uint32_t key);
anim_surf_t *Loader_LoadRLF(const char *file, int8_t transpose, int32_t mask);
char **Loader_LoadSTR(const char *file);
char **Loader_LoadSTR_m(mfile_t *mfp);
void Loader_LoadZCR(const char *file, Cursor_t *cur);
void Loader_OpenZFS(const char *file);
void adpcm8_decode(void *in, void *out, int8_t stereo, int32_t n, adpcm_context_t *ctx);

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

bool isDirectory(const char *);
bool FileExists(const char *);

#endif // LOADER_H_INCLUDED
