#include <sys/stat.h>
#include <dirent.h>

#include "System.h"

/******************************* Assets management *******************************/
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
    uint32_t magic;
    uint32_t unk1;
    uint32_t unk2;
    uint32_t files_perblock;
    uint32_t files_cnt;
    uint32_t xor_key;
    uint32_t Offset_files;
} zfs_header_t;

typedef struct
{
    char name[0x10];
    uint32_t offset;
    uint32_t id;
    uint32_t size;
    uint32_t time;
    uint32_t unk2;
} zfs_file_header_t;

typedef struct
{
    char *name;
    char *path;
    size_t size;
    zfs_file_t *zfs;
} FManNode_t;

typedef struct BinTreeNd_s
{
    struct BinTreeNd_s *zero;
    struct BinTreeNd_s *one;
    FManNode_t *nod;
} BinTreeNd_t;

static BinTreeNd_t *root = NULL;
static MList *BinNodesList = NULL;
static MList *FontList;

static void AddFManNode(FManNode_t *nod)
{
    char buffer[255];
    int32_t t_len = strlen(nod->name);

    for (int i = 0; i < t_len; i++)
        buffer[i] = tolower(nod->name[i]);

    buffer[t_len] = 0x0;

    BinTreeNd_t **treenod = &root;
    t_len = strlen(buffer);
    for (int j = 0; j < t_len; j++)
        for (int i = 0; i < 8; i++)
        {
            int bit = ((buffer[j]) >> i) & 1;
            if (bit)
                treenod = &((*treenod)->one);
            else
                treenod = &((*treenod)->zero);

            if (*treenod == NULL)
            {
                *treenod = NEW(BinTreeNd_t);
                AddToMList(BinNodesList, *treenod);
            }
        }
    if ((*treenod)->nod == NULL) //we don't need to reSet nodes (ADDON and patches don't work without it)
        (*treenod)->nod = nod;
    else if ((*treenod)->nod->size < 10 && nod->size >= 10)
        (*treenod)->nod = nod;
}

static FManNode_t *FindFManNode(const char *chr)
{
    char buffer[255];
    int32_t t_len = strlen(chr);
    for (int i = 0; i < t_len; i++)
        buffer[i] = tolower(chr[i]);

    buffer[t_len] = 0x0;

    BinTreeNd_t *treenod = root;

    t_len = strlen(buffer);
    for (int j = 0; j < t_len; j++)
        for (int i = 0; i < 8; i++)
        {
            int bit = ((buffer[j]) >> i) & 1;
            if (bit)
                treenod = treenod->one;
            else
                treenod = treenod->zero;

            if (treenod == NULL)
                return NULL;
        }

    return treenod->nod;
}

static void OpenZFS(const char *file)
{
    LOG_DEBUG("Opening ZFS file '%s'\n", file);

    FILE *fl = fopen(file, "rb");
    if (!fl)
        Z_PANIC("Error: File '%s' not found!\n", file);

    zfs_header_t hdr;
    fread(&hdr, sizeof(hdr), 1, fl);

    if (hdr.magic != MAGIC_ZFS)
    {
        LOG_WARN("File '%s' is not valid ZFS\n", file);
        return;
    }

    zfs_arch_t *tmp = NEW(zfs_arch_t);
    tmp->fl = fl;
    tmp->xor_key = hdr.xor_key;

    uint32_t nextpos = hdr.Offset_files;

    while (nextpos != 0)
    {
        fseek(fl, nextpos, 0);
        fread(&nextpos, 4, 1, fl);

        for (uint32_t i = 0; i < hdr.files_perblock; i++)
        {
            //fseek(f,pos,0);
            zfs_file_header_t fil;
            fread(&fil, sizeof(fil), 1, fl);

            if (!str_empty(fil.name))
            {
                LOG_DEBUG("Adding file from zfs : %s\n", fil.name);

                FManNode_t *nod = NEW(FManNode_t);
                nod->path = strdup(fil.name);
                nod->name = nod->path;
                nod->zfs = NEW(zfs_file_t);
                nod->zfs->archive = tmp;
                nod->zfs->offset = fil.offset;
                nod->zfs->size = fil.size;
                nod->size = fil.size;
                AddFManNode(nod);
            }
        }
    }
}

static void FindAssets(const char *dir)
{
    char path[PATHBUFSIZE];
    struct stat statbuf;
    int len = strlen(dir);

    strcpy(path, dir);

    while (path[len - 1] == '/' || path[len - 1] == '\\')
    {
        path[len - 1] = 0;
        len--;
    }

    LOG_DEBUG("Listing dir: %s\n", path);

    DIR *dr = opendir(path);
    struct dirent *de;

    if (!dr)
        return;

    while ((de = readdir(dr)))
    {
        if (strlen(de->d_name) < 3)
            continue;

        sprintf(path + len, "/%s", de->d_name);

        if (stat(path, &statbuf) != 0)
        {
            // LOG_WARN("Error stat file '%s'\n", path);
            continue;
        }
        else if (S_ISDIR(statbuf.st_mode))
        {
            FindAssets(path);
        }
        else if (str_ends_with(path, ".ZFS"))
        {
            OpenZFS(path);
        }
        else if (str_ends_with(path, ".TTF"))
        {
            LOG_DEBUG("Adding font : %s\n", path);
            TTF_Font *fnt = TTF_OpenFont(path, 10);
            if (fnt != NULL)
            {
                graph_font_t *tmpfnt = NEW(graph_font_t);
                strncpy(tmpfnt->name, TTF_FontFaceFamilyName(fnt), sizeof(tmpfnt->name)-1);
                strncpy(tmpfnt->path, path, sizeof(path));
                AddToMList(FontList, tmpfnt);
                TTF_CloseFont(fnt);
            }
        }
        else
        {
            LOG_DEBUG("Adding game file : %s\n", path);
            FManNode_t *nod = NEW(FManNode_t);
            nod->path = strdup(path);
            nod->name = nod->path + len + 1;
            nod->zfs = NULL;
            nod->size = statbuf.st_size;
            AddFManNode(nod);
        }
    }
    closedir(dr);
}

void Loader_Init(const char *dir)
{
    FontList = CreateMList();
    BinNodesList = CreateMList();
    root = NEW(BinTreeNd_t);
    AddToMList(BinNodesList, root);
    FindAssets(Game_GetPath());
    // Load working dir (this would allow us to override without touching the original assets)
    // FindAssets(".");
}

const char *Loader_FindFile(const char *filename)
{
    FManNode_t *nod = FindFManNode(filename);

    if (nod && nod->zfs == NULL)
        return nod->path;

    struct stat statbuf;
    if (stat(filename, &statbuf) == 0)
        return filename;

    return NULL;
}
/***************************** END Assets management *****************************/


/*********************************** Music/Sound *******************************/
static const struct
{
    int8_t pkd;
    uint16_t freq;
    int8_t bits;
    int8_t stereo;
    uint8_t chr;
} zg[24] = {
    {0, 0x2B11, 0x8, 0x0, '4'},
    {0, 0x2B11, 0x8, 0x1, '5'},
    {0, 0x2B11, 0x10, 0x0, '6'},
    {0, 0x2B11, 0x10, 0x1, '7'},
    {0, 0x5622, 0x8, 0x0, '8'},
    {0, 0x5622, 0x8, 0x1, '9'},
    {0, 0x5622, 0x10, 0x0, 'a'},
    {0, 0x5622, 0x10, 0x1, 'b'},
    {0, 0xAC44, 0x8, 0x0, 'c'},
    {0, 0xAC44, 0x8, 0x1, 'd'},
    {0, 0xAC44, 0x10, 0x0, 'e'},
    {0, 0xAC44, 0x10, 0x1, 'f'},
    {1, 0x2B11, 0x8, 0x0, 'g'},
    {1, 0x2B11, 0x8, 0x1, 'h'},
    {1, 0x2B11, 0x10, 0x0, 'j'},
    {1, 0x2B11, 0x10, 0x1, 'k'},
    {1, 0x5622, 0x8, 0x0, 'm'},
    {1, 0x5622, 0x8, 0x1, 'n'},
    {1, 0x5622, 0x10, 0x0, 'p'},
    {1, 0x5622, 0x10, 0x1, 'q'},
    {1, 0xAC44, 0x8, 0x0, 'r'},
    {1, 0xAC44, 0x8, 0x1, 's'},
    {1, 0xAC44, 0x10, 0x0, 't'},
    {1, 0xAC44, 0x10, 0x1, 'u'}};

static int znem_freq[16] = {0x1F40, 0x1F40, 0x1F40, 0x1F40, 0x2B11, 0x2B11,
                            0x2B11, 0x2B11, 0x5622, 0x5622, 0x5622, 0x5622,
                            0xAC44, 0xAC44, 0xAC44, 0xAC44};
static int znem_bits[4] = {0x08, 0x08, 0x10, 0x10};
static int znem_stereo[2] = {0, 1};

Mix_Chunk *Loader_LoadSound(const char *file)
{
    mfile_t *mfp = mfopen(file);
    Mix_Chunk *chunks = NULL;

    if (!mfp)
    {
        LOG_WARN("File '%s' not found\n", file);
    }
    else if (str_ends_with(file, "wav"))
    {
        chunks = Mix_QuickLoad_WAV((uint8_t*)mfbuffer(mfp));
    }
    else if (CUR_GAME == GAME_ZGI)
    {
        char type = tolower(file[strlen(file) - 5]);
        int index = -1;

        for (int i = 0; i < 24; i++)
            if (zg[i].chr == type)
            {
                index = i;
                break;
            }

        if (index == -1)
            return NULL;

        chunks = wav_create(
            mfbuffer(mfp),
            mfp->size,
            zg[index].stereo + 1,
            zg[index].freq,
            zg[index].bits,
            zg[index].pkd);
    }
    else if (CUR_GAME == GAME_NEM)
    {
        char type = tolower(file[strlen(file) - 6]);

        if (type < 'p')
        {
            if (type >= 'j')
                type--;
        }
        else
            type -= 2;

        if (type > '9')
            type -= '\'';

        type -= '0';

        chunks = wav_create(
            mfbuffer(mfp),
            mfp->size,
            znem_stereo[type % 2] + 1,
            znem_freq[type % 16],
            znem_bits[type % 4],
            !(str_ends_with(file, "ifp") || type == '6'));
    }

    mfclose(mfp);

    return chunks;
}
/********************************* END Music/Sound *****************************/


/******************************* Bitmaps *******************************/
static void de_lz(SDL_Surface *srf, uint8_t *src, uint32_t size, int32_t transpose)
{
    uint8_t lz[0x1000];
    uint32_t lz_pos = 0x0fee;
    uint32_t cur = 0, d_cur = 0, otsk;
    uint8_t bl, mk, i, j, lw, hi, loops;

    memset(&lz[0], 0, 0x1000);

    SDL_LockSurface(srf);

    uint8_t *dst = (uint8_t *)srf->pixels;

    bool need_correction = (srf->w != (srf->pitch / srf->format->BytesPerPixel));
    int32_t vpitch = srf->w * 2;

    while (cur < size)
    {
        bl = src[cur];
        mk = 1;

        for (i = 0; i < 8; i++)
        {
            if ((bl & mk) == mk)
            {
                cur++;
                if (cur >= size)
                    break;

                lz[lz_pos] = src[cur];

                int32_t index = 0;

                if (transpose == 0)
                    index = d_cur;
                else
                {
                    int32_t dx = d_cur / 2;
                    int32_t ddx = d_cur & 1;
                    int32_t hh = dx % srf->h;
                    int32_t ww = dx / srf->h;
                    index = (hh * srf->w + ww) * 2 + ddx;
                }

                if (need_correction)
                {
                    int32_t cary = index / vpitch;
                    int32_t cary2 = index % vpitch;
                    index = cary * srf->pitch + cary2;
                }

                dst[index] = src[cur];

                d_cur++;
                lz_pos = (lz_pos + 1) & 0xfff;
            }
            else
            {
                cur++;
                if (cur >= size)
                    break;
                lw = src[cur];

                cur++;
                if (cur >= size)
                    break;
                hi = src[cur];

                loops = (hi & 0xf) + 2;

                otsk = lw | ((hi & 0xf0) << 4);

                for (j = 0; j <= loops; j++)
                {
                    lz[lz_pos] = lz[(otsk + j) & 0xfff];

                    int32_t index = 0;

                    if (transpose == 0)
                        index = d_cur;
                    else
                    {
                        int32_t dx = d_cur / 2;
                        int32_t ddx = d_cur & 1;
                        int32_t hh = dx % srf->h;
                        int32_t ww = dx / srf->h;
                        index = (hh * srf->w + ww) * 2 + ddx;
                    }

                    if (need_correction)
                    {
                        int32_t cary = index / vpitch;
                        int32_t cary2 = index % vpitch;
                        index = cary * srf->pitch + cary2;
                    }

                    dst[index] = lz[lz_pos];

                    lz_pos = (lz_pos + 1) & 0xfff;
                    d_cur++;
                }
            };

            mk = mk << 1;
        }

        cur++;
    }

    SDL_UnlockSurface(srf);
}

SDL_Surface *Loader_LoadGFX(const char *file, bool transpose, int32_t key_555)
{
    LOG_DEBUG("Loading GFX file '%s'\n", file);

    mfile_t *mfp = mfopen(file);
    if (!mfp)
    {
        LOG_WARN("GFX File '%s' not found\n", file);
        return NULL;
    }

    uint8_t *buffer = (uint8_t *)mfbuffer(mfp);

    uint32_t magic = ((uint32_t*)buffer)[0];
    int32_t wi = ((int32_t*)buffer)[2];
    int32_t hi = ((int32_t*)buffer)[3];

    if (magic != MAGIC_TGA)
        Z_PANIC("File '%s' is not valid TGA\n", file);

    if (transpose)
    {
        wi ^= hi;
        hi ^= wi;
        wi ^= hi;
    }

    SDL_Surface *srf = Rend_CreateSurface(wi, hi, 0);

    de_lz(srf, buffer + 0x10, mfp->size - 0x10, transpose);

    mfclose(mfp);

    if (srf && key_555 >= 0)
    {
        int r = (key_555 & 0x1F) * 8;
        int g = ((key_555 >> 5) & 0x1F) * 8;
        int b = ((key_555 >> 10) & 0x1F) * 8;
        Rend_SetColorKey(srf, r, g, b);
    }

    return srf;
}

void Loader_LoadZCR(const char *file, Cursor_t *cur)
{
    LOG_DEBUG("Loading ZCR file '%s'\n", file);

    mfile_t *f = mfopen(file);
    if (f)
    {
        uint32_t magic = 0;
        mfread(&magic, 4, f);
        if (magic == MAGIC_ZCR)
        {
            uint16_t x = 0, y = 0, w = 0, h = 0;
            mfread(&x, 2, f);
            mfread(&y, 2, f);
            mfread(&w, 2, f);
            mfread(&h, 2, f);

            cur->ox = x;
            cur->oy = y;

            cur->img = Rend_CreateSurface(w, h, 0);
            Rend_SetColorKey(cur->img, 0, 0, 0);

            SDL_LockSurface(cur->img);

            mfread(cur->img->pixels, 2 * w * h, f);

            SDL_UnlockSurface(cur->img);
        }
        else
        {
            LOG_WARN("File '%s' is not ZCR\n", file);
        }
        mfclose(f);
    }
    else
    {
        LOG_DEBUG("  > Using fallback mechanism\n");
        char tmp[64];
        strcpy(tmp, file);
        int len = strlen(tmp);

        cur->img = Loader_LoadGFX(tmp, false, 0x0000);

        if (cur->img == NULL)
            return;

        tmp[len - 3] = 'p';
        tmp[len - 2] = 'o';
        tmp[len - 1] = 'i';
        tmp[len] = 'n';
        tmp[len + 1] = 't';
        tmp[len + 2] = 0x0;

        mfile_t *f = mfopen(tmp);
        if (f)
        {
            mfread(&cur->ox, 2, f);
            mfread(&cur->oy, 2, f);
            mfclose(f);
        }
    }
}

TTF_Font *Loader_LoadFont(char *name, int size)
{
    graph_font_t *fnt = NULL;

    StartMList(FontList);
    while (!EndOfMList(FontList))
    {
        fnt = (graph_font_t *)DataMList(FontList);
        if (str_equals(fnt->name, name)) // str_starts_with
            break;

        NextMList(FontList);
    }

    if (fnt == NULL)
        return NULL;

    return TTF_OpenFont(fnt->path, size);
}
/***************************** END Bitmaps *****************************/


/******************************* Animations *******************************/
typedef struct
{
    uint32_t magic; //FELR 0x524C4546
    uint32_t size;  // from begin
    uint32_t unk1;
    uint32_t unk2;
    uint32_t frames; //number of frames
} rlf_header_t;

typedef struct
{
    uint32_t magic; //FNIC
    uint32_t size;
    uint32_t unk1;
    uint32_t unk2;
    uint32_t unk3;
    char VRLE[0x18];
    uint32_t LRVD;
    uint32_t unk4;
    char HRLE[0x18];
    uint32_t ELHD;
    uint32_t unk5;
    char HKEY[0x18];
    uint32_t ELRH;
} rlf_cinf_t;

typedef struct
{
    uint32_t magic; //FNIM
    uint32_t size;
    uint32_t OEDV; //OEDV
    uint32_t unk1;
    uint32_t unk2;
    uint32_t unk3;
    uint32_t width;
    uint32_t height;
} rlf_minf_t;

typedef struct
{
    uint32_t magic; //EMIT
    uint32_t size;
    uint32_t unk1;
    uint32_t microsecs;
} rlf_mtime_t;

typedef struct
{
    uint32_t magic; //MARF
    uint32_t size;
    uint32_t unk1;
    uint32_t unk2;
    uint32_t type;   // ELRH or  ELHD
    uint32_t offset; //from begin of frame to data
    uint32_t unk3;
} rlf_frame_t;

static void DHLE(int8_t *dst, int8_t *src, int32_t size, int32_t size2)
{
    int8_t tmp;
    int32_t off1, off2;
    int16_t tmp2;
    off1 = 0;
    off2 = 0;

    while (off1 < size && off2 < size2)
    {
        tmp = src[off1];
        off1++;

        if (tmp < 0)
        {
            if (tmp < 0)
            {
                tmp = abs(tmp);
                while (tmp != 0)
                {
                    if (off1 + 1 >= size || off2 + 1 >= size2)
                        return;
                    tmp2 = *(int16_t *)(&src[off1]);

                    *(int16_t *)&dst[off2] = tmp2;
                    off2 += 2;
                    off1 += 2;
                    tmp--;
                }
            }
        }
        else
            off2 += tmp * 2 + 2;
    }
}

static void HRLE(int8_t *dst, int8_t *src, int32_t size, int32_t size2)
{
    int8_t tmp;
    int32_t off1, off2;
    int16_t tmp2;
    int32_t tmp4;
    off1 = 0;
    off2 = 0;

    while (off1 < size)
    {
        tmp = src[off1];
        off1++;

        if (tmp < 0)
        {
            if (tmp < 0)
            {
                tmp = abs(tmp);
                while (tmp > 0)
                {
                    if (off1 + 1 >= size || off2 + 1 >= size2)
                        return;
                    tmp2 = *(int16_t *)(src + off1);
                    *(int16_t *)&dst[off2] = tmp2;
                    off2 += 2;
                    off1 += 2;
                    tmp--;
                }
            }
        }
        else
        {
            if (off1 + 1 >= size)
                return;

            tmp2 = *(int16_t *)&src[off1];
            off1 += 2;
            tmp4 = tmp + 2;

            while (tmp4 > 0)
            {
                if (off2 + 1 >= size2)
                    return;
                *(int16_t *)&dst[off2] = tmp2;
                off2 += 2;
                tmp4--;
            }
        }
    }
}

anim_surf_t *Loader_LoadRLF(const char *file, bool transpose, int32_t mask_555)
{
    LOG_DEBUG("Loading RLF file '%s'\n", file);

    mfile_t *f = mfopen(file);
    if (!f)
        Z_PANIC("RLF File %s not found\n", file);

    rlf_header_t hd;
    rlf_cinf_t cin;
    rlf_minf_t mn;
    rlf_mtime_t tm;
    rlf_frame_t frm;

    mfread(&hd, sizeof(hd), f);
    mfread(&cin, sizeof(cin), f);
    mfread(&mn, sizeof(mn), f);
    mfread(&tm, sizeof(tm), f);

    if (hd.magic != MAGIC_RLF)
    {
        LOG_WARN("File '%s' is not valid RLF\n", file);
        return NULL;
    }

    if (transpose == 1)
    {
        mn.height ^= mn.width;
        mn.width ^= mn.height;
        mn.height ^= mn.width;
    }

    anim_surf_t *atmp = NEW(anim_surf_t);

    atmp->info.time = tm.microsecs / 10;
    atmp->info.frames = hd.frames;
    atmp->info.w = mn.width;
    atmp->info.h = mn.height;
    atmp->img = NEW_ARRAY(SDL_Surface*, atmp->info.frames);

    size_t sz_frame = mn.height * mn.width * 2;
    int8_t *buf2 = NEW_ARRAY(int8_t, sz_frame);

    for (int i = 0; i < hd.frames; i++)
    {
        if (!mfread(&frm, sizeof(frm), f))
        {
            Z_PANIC("mfread failed\n");
        }

        if (frm.size > 0 && frm.size < 0x40000000)
        {
            int8_t *frm_data = NEW_ARRAY(int8_t, frm.size - frm.offset);
            if (mfread(frm_data, frm.size - frm.offset, f))
            {
                if (frm.type == 0x44484C45)
                    DHLE(buf2, frm_data, frm.size - frm.offset, sz_frame);
                else if (frm.type == 0x48524C45)
                    HRLE(buf2, frm_data, frm.size - frm.offset, sz_frame);
            }
            free(frm_data);
        }

        SDL_Surface *srf = Rend_CreateSurface(atmp->info.w, atmp->info.h, 0);

        SDL_LockSurface(srf);

        if (transpose)
        {
            uint16_t *pixels = (uint16_t *)srf->pixels;

            for (int j = 0; j < atmp->info.h; j++)
                for (int i = 0; i < atmp->info.w; i++)
                {
                    *pixels = ((uint16_t *)buf2)[i * atmp->info.h + j];
                    pixels++;
                }
        }
        else
        {
            memcpy(srf->pixels, buf2, sz_frame);
        }

        SDL_UnlockSurface(srf);

        atmp->img[i] = srf;

        if (mask_555 > 0)
        {
            int b = ((mask_555 >> 10) & 0x1F) * 8;
            int g = ((mask_555 >> 5) & 0x1F) * 8;
            int r = (mask_555 & 0x1F) * 8;
            Rend_SetColorKey(atmp->img[i], r, g, b);
        }
    }

    mfclose(f);

    free(buf2);

    return atmp;
}
/***************************** END Animations *****************************/


/******************************* STRINGS *******************************/
char **Loader_LoadSTR(const char *filename)
{
    char line_buffer[STRBUFSIZE];
    int lines = 1;

    mfile_t *mfp = mfopen_txt(filename);
    if (!mfp)
        return NULL;

    for (int i = 0; i < mfp->size; i++)
    {
        if (mfp->buffer[i] == '\n')
            lines++;
    }

    char **strings = NEW_ARRAY(char *, lines + 10);
    int pos = 0;

    while (!mfeof(mfp))
    {
        mfgets(line_buffer, STRBUFSIZE, mfp);
        strings[pos++] = str_trim(line_buffer);
    }
    strings[pos] = NULL;

    mfclose(mfp);

    return strings;
}
/***************************** STRINGS END *****************************/


/******************************* File access *******************************/
static void txt_wide_to_utf8(mfile_t *file)
{
    const char *file_buf = mfbuffer(file);
    size_t size = file->size * 2;
    size_t pos = 0;

    char *buf = NEW_ARRAY(char, size);

    file->pos = 0;
    while (file->pos < file->size && pos < size)
    {
        if (file_buf[file->pos] == 0xD)
        {
            buf[pos] = 0xD;
            pos++;
            file->pos++;
            if (file->pos < file->size)
                if (file_buf[file->pos] == 0xA)
                {
                    if (pos < size)
                    {
                        buf[pos] = 0xA;
                        pos++;
                    }
                    file->pos++;
                    if (file->pos < file->size)
                        if (file_buf[file->pos] == 0x0)
                            file->pos++;
                }
        }
        else
        {
            if (file->pos + 1 < file->size)
            {
                uint16_t ch = (file_buf[file->pos] & 0xFF) | ((file_buf[file->pos + 1] << 8) & 0xFF00);
                if (ch < 0x80)
                {
                    buf[pos] = ch & 0x7F;
                    pos++;
                }
                else if (ch >= 0x80 && ch < 0x800)
                {
                    if (pos + 1 < size)
                    {
                        buf[pos] = 0xC0 | ((ch >> 6) & 0x1F);
                        pos++;
                        buf[pos] = 0x80 | ((ch)&0x3F);
                        pos++;
                    }
                }
                else if (ch >= 0x800 && ch < 0x10000)
                {
                    if (pos + 2 < size)
                    {
                        buf[pos] = 0xE0 | ((ch >> 12) & 0xF);
                        pos++;
                        buf[pos] = 0x80 | ((ch >> 6) & 0x3F);
                        pos++;
                        buf[pos] = 0x80 | ((ch)&0x3F);
                        pos++;
                    }
                }
                else if (ch >= 0x10000 && ch < 0x200000)
                {
                    if (pos + 3 < size)
                    {
                        buf[pos] = 0xF0;
                        pos++;
                        buf[pos] = 0x80 | ((ch >> 12) & 0x3F);
                        pos++;
                        buf[pos] = 0x80 | ((ch >> 6) & 0x3F);
                        pos++;
                        buf[pos] = 0x80 | ((ch)&0x3F);
                        pos++;
                    }
                }

                file->pos += 2;
            }
        }
    }

    free((void*)file_buf);
    file->pos = 0;
    file->size = pos;
    file->buffer = buf;
}

mfile_t *mfopen(const char *filename)
{
    FManNode_t *node = FindFManNode(filename);
    if (node)
    {
        if (node->zfs)
        {
            LOG_DEBUG("Opening file '%s' from ZFS archive\n", filename);
            mfile_t *tmp = NEW(mfile_t);

            tmp->size = node->size;
            tmp->zfs = node->zfs;

            return tmp;
        }
        else
        {
            filename = node->path;
        }
    }

    FILE *fp = fopen(filename, "rb");
    if (fp)
    {
        LOG_DEBUG("Opening file '%s' from file system\n", filename);
        mfile_t *tmp = NEW(mfile_t);

        fseek(fp, 0, SEEK_END);
        tmp->size = ftell(fp);
        tmp->fp = fp;

        return tmp;
    }

    LOG_WARN("Unable to open file '%s'\n", filename);
    return NULL;
}

mfile_t *mfopen_txt(const char *filename)
{
    mfile_t *file = mfopen(filename);

    if (!file)
        return NULL;

    mfbuffer(file);

    for (size_t i = 0; i < file->size - 2; i++)
        if (file->buffer[i] == 0 && file->buffer[i + 2] == 0)
        {
            txt_wide_to_utf8(file);
            break;
        }

    return file;
}

const void *mfbuffer(mfile_t *file)
{
    if (!file->buffer)
    {
        file->buffer = NEW_ARRAY(char, file->size + 4);

        if (file->zfs)
        {
            zfs_file_t *zfile = (zfs_file_t *)file->zfs;
            file->zfs = NULL; // We no longer need the file pointer

            fseek(zfile->archive->fl, zfile->offset, SEEK_SET);
            fread(file->buffer, file->size, 1, zfile->archive->fl);

            if (zfile->archive->xor_key)
            {
                size_t count = file->size >> 2;
                for (size_t i = 0; i < count; i++)
                    ((uint32_t*)file->buffer)[i] ^= zfile->archive->xor_key;
            }
        }
        else
        {
            fseek(file->fp, 0, SEEK_SET);
            fread(file->buffer, file->size, 1, file->fp);
            fclose(file->fp); // We no longer need the file pointer
            file->fp = NULL;
        }
    }
    return file->buffer;
}

bool mfread(void *buf, size_t bytes, mfile_t *file)
{
    if (file->pos + bytes > file->size)
        return false;

    memcpy(buf, mfbuffer(file) + file->pos, bytes);

    // if (!file->buffer && !file->fp)
    //     mfbuffer(file);

    // if (file->buffer)
    //     memcpy(buf, file->buffer + file->pos, bytes);
    // else if (file->fp)
    // {
    //     fseek(file->fp, file->pos, SEEK_SET);
    //     fread(buf, bytes, 1, file->fp);
    // }

    file->pos += bytes;

    return true;
}

void mfseek(mfile_t *file, size_t pos)
{
    if (pos <= file->size)
        file->pos = pos;
}

int32_t mftell(mfile_t *file)
{
    return file ? file->pos : -1;
}

void mfclose(mfile_t *file)
{
    if (file)
    {
        if (file->fp)
            fclose(file->fp);
        if (file->buffer)
            free(file->buffer);
        free(file);
    }
}

bool mfeof(mfile_t *file)
{
    return file->pos >= file->size;
}

char *mfgets(char *buf, size_t max_bytes, mfile_t *file)
{
    int copied = 0;
    int max = max_bytes - 1;

    if (mfeof(file))
        return NULL;

    if (!file->buffer)
        mfbuffer(file);

    while (copied < max && file->pos < file->size && file->buffer[file->pos] != '\r' && file->buffer[file->pos] != '\n')
    {
        buf[copied++] = file->buffer[file->pos++];
    }

    buf[copied] = 0;

    if (file->pos < file->size && file->buffer[file->pos] == '\r')
    {
        file->pos++;
    }

    if (file->pos < file->size && file->buffer[file->pos] == '\n')
    {
        file->pos++;
    }

    return buf;
}
/***************************** END File access *****************************/
