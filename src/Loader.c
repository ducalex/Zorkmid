#include "System.h"

/*********************************** adpcm_Support *******************************/
static const uint8_t wavHeader[0x2C] =
    {
        'R', 'I', 'F', 'F',
        0, 0, 0, 0,
        'W', 'A', 'V', 'E',
        'f', 'm', 't', ' ',
        0x10, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        'd', 'a', 't', 'a',
        0, 0, 0, 0};

static const int32_t t1[] = {-1, -1, -1, 1, 4, 7, 10, 12};
static const int32_t t2[] = {0x0007, 0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E,
                      0x0010, 0x0011, 0x0013, 0x0015, 0x0017, 0x0019, 0x001C, 0x001F,
                      0x0022, 0x0025, 0x0029, 0x002D, 0x0032, 0x0037, 0x003C, 0x0042,
                      0x0049, 0x0050, 0x0058, 0x0061, 0x006B, 0x0076, 0x0082, 0x008F,
                      0x009D, 0x00AD, 0x00BE, 0x00D1, 0x00E6, 0x00FD, 0x0117, 0x0133,
                      0x0151, 0x0173, 0x0198, 0x01C1, 0x01EE, 0x0220, 0x0256, 0x0292,
                      0x02D4, 0x031C, 0x036C, 0x03C3, 0x0424, 0x048E, 0x0502, 0x0583,
                      0x0610, 0x06AB, 0x0756, 0x0812, 0x08E0, 0x09C3, 0x0ABD, 0x0BD0,
                      0x0CFF, 0x0E4C, 0x0FBA, 0x114C, 0x1307, 0x14EE, 0x1706, 0x1954,
                      0x1BDC, 0x1EA5, 0x21B6, 0x2515, 0x28CA, 0x2CDF, 0x315B, 0x364B,
                      0x3BB9, 0x41B2, 0x4844, 0x4F7E, 0x5771, 0x602F, 0x69CE, 0x7462,
                      0x7FFF};

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
static int znem_bits[4] = {8, 8, 0x10, 0x10};
static int znem_stereo[2] = {0, 1};

void adpcm8_decode(void *in, void *out, int8_t stereo, int32_t n, adpcm_context_t *ctx)
{
    uint8_t *m1;
    uint16_t *m2;
    m1 = (uint8_t *)in;
    m2 = (uint16_t *)out;
    uint32_t a, x, j = 0;
    int32_t b, i, t[4] = {0, 0, 0, 0};

    while (n)
    {
        a = *m1;
        i = ctx ? ctx->t[ctx->j + 2] : t[j + 2];
        x = t2[i];
        b = 0;

        if (a & 0x40)
            b += x;
        if (a & 0x20)
            b += x >> 1;
        if (a & 0x10)
            b += x >> 2;
        if (a & 8)
            b += x >> 3;
        if (a & 4)
            b += x >> 4;
        if (a & 2)
            b += x >> 5;
        if (a & 1)
            b += x >> 6;

        if (a & 0x80)
            b = -b;

        b += ctx ? ctx->t[ctx->j] : t[j];

        if (b > 32767)
            b = 32767;
        else if (b < -32768)
            b = -32768;

        i += t1[(a >> 4) & 7];

        if (i < 0)
            i = 0;
        else if (i > 88)
            i = 88;

        if (ctx)
        {
            ctx->t[ctx->j] = b;
            ctx->t[ctx->j + 2] = i;
            ctx->j = (ctx->j + 1) & stereo;
        }
        else
        {
            t[j] = b;
            t[j + 2] = i;
            j = (j + 1) & stereo;
        }
        *m2 = b;

        m1++;
        m2++;
        n--;
    }
}

static Mix_Chunk *Load_ZGI(FManNode_t *file, char type)
{
    int32_t freq;
    int32_t bits;
    int32_t stereo;
    int32_t pkd;
    int32_t size2;

    int32_t indx = -1;

    char low = tolower(type);

    for (int32_t i = 0; i < 24; i++)
        if (zg[i].chr == low)
        {
            indx = i;
            freq = zg[i].freq;
            bits = zg[i].bits;
            stereo = zg[i].stereo;
            pkd = zg[i].pkd;

            break;
        }

    if (indx == -1)
        return NULL;

    mfile_t *f = mfopen(file);

    if (pkd == 1)
        size2 = f->size * 2 + 0x2C;
    else
        size2 = f->size + 0x2C;

    void *raw_w = malloc(size2);

    memcpy(raw_w, wavHeader, 0x2C);

    uint32_t *raw_i = (uint32_t *)raw_w;
    raw_i[1] = size2 - 8;
    raw_i[5] = ((stereo + 1) << 16) + 1;
    raw_i[6] = freq;
    if (bits == 16)
    {
        raw_i[7] = freq << (stereo + 1);
        raw_i[8] = 0x100000 + (stereo + 1) * 2;
    }
    else
    {
        raw_i[7] = freq << stereo;
        raw_i[8] = 0x100000 + stereo * 2;
    }
    raw_i[10] = size2 - 0x2C;

    if (pkd == 1)
        adpcm8_decode(f->buf, &raw_i[11], stereo, f->size, NULL);
    else
        memcpy(&raw_i[11], f->buf, f->size);

    mfclose(f);

    return Mix_LoadWAV_RW(SDL_RWFromMem(raw_w, size2), 1);
}

static Mix_Chunk *Load_ZNEM(FManNode_t *file, char type)
{
    int32_t freq;
    int32_t bits;
    int32_t stereo;
    int32_t pkd;
    int32_t size2;

    char low = tolower(type);

    if (low < 'p')
    {
        if (low >= 'j')
            low--;
    }
    else
    {
        low -= 2;
    }
    if (low > '9')
        low -= '\'';
    low -= '0';

    if (str_ends_with(file->name, "ifp") || type == '6')
        pkd = 0;
    else
        pkd = 1;

    freq = znem_freq[low % 16];
    bits = znem_bits[low % 4];
    stereo = znem_stereo[low % 2];

    mfile_t *f = mfopen(file);

    if (pkd == 1)
        size2 = f->size * 2 + 0x2C;
    else
        size2 = f->size + 0x2C;

    void *raw_w = malloc(size2);

    memcpy(raw_w, wavHeader, 0x2C);

    uint32_t *raw_i = (uint32_t *)raw_w;
    raw_i[1] = size2 - 8;
    raw_i[5] = ((stereo + 1) << 16) + 1;
    raw_i[6] = freq;
    if (bits == 16)
    {
        raw_i[7] = freq << (stereo + 1);
        raw_i[8] = 0x100000 + (stereo + 1) * 2;
    }
    else
    {
        raw_i[7] = freq << stereo;
        raw_i[8] = 0x100000 + stereo * 2;
    }
    raw_i[10] = size2 - 0x2C;

    if (pkd == 1)
        adpcm8_decode(f->buf, &raw_i[11], stereo, f->size, NULL);
    else
        memcpy(&raw_i[11], f->buf, f->size);

    mfclose(f);

    return Mix_LoadWAV_RW(SDL_RWFromMem(raw_w, size2), 1);
}

Mix_Chunk *loader_LoadChunk(const char *file)
{
    FManNode_t *mfil = FindInBinTree(file);
    const char *fil = NULL;

    if (!mfil)
        fil = GetFilePath(file);
    else
        fil = mfil->name;

    if (!fil)
        return NULL;

    if (mfsize(mfil) < 10)
        return NULL;

    Mix_Chunk *chunk = NULL;

    if ((str_ends_with(fil, "src") || str_ends_with(fil, "raw") || str_ends_with(fil, "ifp")) && (mfil != NULL))
    {
        if (CUR_GAME == GAME_ZGI)
            chunk = Load_ZGI(mfil, fil[strlen(fil) - 5]);
        else
            chunk = Load_ZNEM(mfil, fil[strlen(fil) - 6]);
    }
    else if (!mfil)
    {
        chunk = Mix_LoadWAV(fil);
    }

    return chunk;
}
/*********************************** END adpcm_Support *******************************/


/***********************************TGZ_Support *******************************/
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

static void flip_vertical(SDL_Surface *srf)
{
    SDL_LockSurface(srf);

    uint8_t *a = (uint8_t *)srf->pixels;

    int32_t num = srf->h / 2;
    for (int32_t i = 0; i < num; i++)
        for (int32_t j = 0; j < srf->pitch; j++)
        {
            a[i * srf->pitch + j] ^= a[(srf->h - i - 1) * srf->pitch + j];
            a[(srf->h - i - 1) * srf->pitch + j] ^= a[i * srf->pitch + j];
            a[i * srf->pitch + j] ^= a[(srf->h - i - 1) * srf->pitch + j];
        }

    SDL_UnlockSurface(srf);
}

static void flip_horizont(SDL_Surface *srf)
{
    SDL_LockSurface(srf);

    if (srf->format->BitsPerPixel == 32)
    {
        int32_t num = srf->w / 2;
        int32_t maxw = srf->w - 1;
        for (int32_t i = 0; i < num; i++)
            for (int32_t j = 0; j < srf->h; j++)
            {
                uint32_t *a = (uint32_t *)((uint8_t *)srf->pixels + j * srf->pitch);

                a[i] ^= a[maxw - i];
                a[maxw - i] ^= a[i];
                a[i] ^= a[maxw - i];
            }
    }
    else if (srf->format->BitsPerPixel == 16)
    {
        int32_t num = srf->w / 2;
        int32_t maxw = srf->w - 1;
        for (int32_t i = 0; i < num; i++)
            for (int32_t j = 0; j < srf->h; j++)
            {
                uint16_t *a = (uint16_t *)((uint8_t *)srf->pixels + j * srf->pitch);

                a[i] ^= a[maxw - i];
                a[maxw - i] ^= a[i];
                a[i] ^= a[maxw - i];
            }
    }
    else if (srf->format->BitsPerPixel == 8)
    {
        int32_t num = srf->w / 2;
        int32_t maxw = srf->w - 1;
        for (int32_t i = 0; i < num; i++)
            for (int32_t j = 0; j < srf->h; j++)
            {
                uint8_t *a = (uint8_t *)srf->pixels + j * srf->pitch;

                a[i] ^= a[maxw - i];
                a[maxw - i] ^= a[i];
                a[i] ^= a[maxw - i];
            }
    }

    SDL_UnlockSurface(srf);
}

static SDL_Surface *buf_to_surf(void *buf, int32_t w, int32_t h, int8_t transpose)
{
    SDL_Surface *srf = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 16, 0x7C00, 0x3E0, 0x1F, 0);

    SDL_LockSurface(srf);

    bool need_correction = ((srf->w * srf->format->BytesPerPixel) != srf->pitch);
    int32_t vpitch = srf->pitch / 2;

    if (transpose == 1)
    {
        uint16_t *tmp = (uint16_t *)buf;
        uint16_t *tmp2 = (uint16_t *)srf->pixels;

        if (need_correction)
        {
            int32_t idx = 0;

            for (int32_t j = 0; j < h; j++)
                for (int32_t i = 0; i < w; i++)
                {
                    int32_t real_idx = ((idx / w) * vpitch) + (idx % w);
                    tmp2[real_idx] = tmp[i * h + j];
                    idx++;
                }
        }
        else
        {
            for (int32_t j = 0; j < h; j++)
                for (int32_t i = 0; i < w; i++)
                {
                    *tmp2 = tmp[i * h + j];
                    tmp2++;
                }
        }
    }
    else
    {
        if (need_correction)
        {
            uint16_t *tmp = (uint16_t *)buf;
            uint16_t *tmp2 = (uint16_t *)srf->pixels;

            int32_t idx = 0;

            for (int32_t j = 0; j < h; j++)
                for (int32_t i = 0; i < w; i++)
                {
                    int32_t real_idx = ((idx / w) * vpitch) + (idx % w);
                    tmp2[real_idx] = tmp[j * w + i];
                    idx++;
                }
        }
        else
            memcpy(srf->pixels, buf, w * h * 2);
    }

    SDL_UnlockSurface(srf);

    return srf;
}

static SDL_Surface *Load_gfx_File(const char *file, int8_t transpose, int8_t key, uint32_t ckey)
{
    TRACE_LOADER("Load GFX file '%s'\n", file);

    FManNode_t *mfil = FindInBinTree(file);

    if (!mfil)
    {
        LOG_WARN("GFX File '%s' not found\n", file);
        return NULL;
    }

    SDL_Surface *srf = NULL;

    mfile_t *mfp = mfopen(mfil);

    uint32_t magic = ((uint32_t*)mfp->buf)[0];

    if (magic == MAGIC_TGA)
    {
        int32_t wi = ((int32_t*)mfp->buf)[2];
        int32_t hi = ((int32_t*)mfp->buf)[3];

        if (transpose)
        {
            wi ^= hi;
            hi ^= wi;
            wi ^= hi;
        }

        srf = SDL_CreateRGBSurface(SDL_SWSURFACE, wi, hi, 16, 0x7C00, 0x3E0, 0x1F, 0);

        de_lz(srf, mfp->buf + 0x10, mfp->size - 0x10, transpose);
    }
    else
    {
        LOG_WARN("File '%s' is not valid TGA\n", file);

        srf = IMG_Load_RW(SDL_RWFromMem(mfp->buf, mfp->size), 0);
        if (!srf)
        {
            uint16_t wi = ((uint16_t*)mfp->buf)[7];
            uint16_t hi = ((uint16_t*)mfp->buf)[8];

            if (transpose == 1)
            {
                srf = buf_to_surf(mfp->buf + 18, hi, wi, transpose);
                flip_horizont(srf);
            }
            else
            {
                srf = buf_to_surf(mfp->buf + 18, wi, hi, transpose);
                flip_vertical(srf);
            }
        }
    }

    mfclose(mfp);

    if (srf)
    {
        ConvertImage(&srf);

        if (key != 0)
            SDL_SetColorKey(srf, SDL_SRCCOLORKEY, ckey);
    }

    return srf;
}

SDL_Surface *loader_LoadFile(const char *file, int8_t transpose)
{
    return Load_gfx_File(file, transpose, 0, 0);
}

SDL_Surface *loader_LoadFile_key(const char *file, int8_t transpose, uint32_t key)
{
    return Load_gfx_File(file, transpose, 1, key);
}
/*********************************** END TGZ_Support *******************************/


/*********************************** RLF ***************************************/
typedef struct
{
    uint32_t magic; //FELR 0x524C4546
    uint32_t size;  // from begin
    uint32_t unk1;
    uint32_t unk2;
    uint32_t frames; //number of frames
} Header;

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
} Cinf;

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
} MinF;

typedef struct
{
    uint32_t magic; //EMIT
    uint32_t size;
    uint32_t unk1;
    uint32_t microsecs;
} mTime;

typedef struct
{
    uint32_t magic; //MARF
    uint32_t size;
    uint32_t unk1;
    uint32_t unk2;
    uint32_t TYPE;   // ELRH or  ELHD
    uint32_t offset; //from begin of frame to data
    uint32_t unk3;
} Frame;

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

anim_surf_t *loader_LoadRlf(const char *file, int8_t transpose, int32_t mask)
{
    TRACE_LOADER("Load RLF file '%s'\n", file);

    FManNode_t *fil = FindInBinTree(file);
    if (!fil)
        Z_PANIC("File %s not found\n", file);

    mfile_t *f = mfopen(fil);

    Header hd;
    Cinf cin;
    MinF mn;
    mTime tm;
    Frame frm;

    mfread(&hd, sizeof(Header), f);
    mfread(&cin, sizeof(Cinf), f);
    mfread(&mn, sizeof(MinF), f);
    mfread(&tm, sizeof(mTime), f);

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

    int32_t sz_frame = mn.height * mn.width * 2;
    int8_t *buf2 = calloc(sz_frame, 1);
    void *buf  = NULL;

    for (uint16_t i = 0; i < hd.frames; i++)
    {
        if (mfread(&frm, sizeof(Frame), f))
            if (frm.size > 0 && frm.size < 0x40000000)
            {
                if (mfread_ptr(&buf, frm.size - frm.offset, f))
                {
                    if (frm.TYPE == 0x44484C45)
                        DHLE(buf2, buf, frm.size - frm.offset, sz_frame);
                    else if (frm.TYPE == 0x48524C45)
                        HRLE(buf2, buf, frm.size - frm.offset, sz_frame);
                }
            }

        atmp->img[i] = buf_to_surf(buf2, atmp->info.w, atmp->info.h, transpose);
        ConvertImage(&atmp->img[i]);

        if (mask != 0 && mask != -1)
            SDL_SetColorKey(atmp->img[i], SDL_SRCCOLORKEY, mask);
    }

    mfclose(f);

    free(buf2);

    return atmp;
}
/*********************************** END RLF ***************************************/


/******************** ZCR ************************/
void loader_LoadZcr(const char *file, Cursor_t *cur)
{
    TRACE_LOADER("Load ZCR file '%s'\n", file);

    FManNode_t *fl = FindInBinTree(file);

    if (fl == NULL)
    {
        TRACE_LOADER("fallback-mechanism\n");
        char tmp[64];
        strcpy(tmp, file);
        int len = strlen(tmp);

        cur->img = loader_LoadFile_key(tmp, 0, Rend_MapScreenRGB(0, 0, 0));

        if (cur->img == NULL)
            return;

        tmp[len - 3] = 'p';
        tmp[len - 2] = 'o';
        tmp[len - 1] = 'i';
        tmp[len] = 'n';
        tmp[len + 1] = 't';
        tmp[len + 2] = 0x0;

        const char *tmp2 = GetFilePath(tmp);
        if (tmp2 == NULL)
            return;

        FILE *f = fopen(tmp2, "rb");
        fread(&cur->ox, 1, 2, f);
        fread(&cur->oy, 1, 2, f);
        fclose(f);
        return;
    }

    mfile_t *f = mfopen(fl);

    uint32_t magic = 0;
    mfread(&magic, 4, f);
    if (magic == MAGIC_ZCR)
    {
        uint16_t x, y, w, h;
        mfread(&x, 2, f);
        mfread(&y, 2, f);
        mfread(&w, 2, f);
        mfread(&h, 2, f);

        cur->ox = x;
        cur->oy = y;

        cur->img = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 16, 0x7C00, 0x3E0, 0x1F, 0);

        SDL_LockSurface(cur->img);

        mfread(cur->img->pixels, 2 * w * h, f);

        SDL_UnlockSurface(cur->img);

        ConvertImage(&cur->img);

        SDL_SetColorKey(cur->img, SDL_SRCCOLORKEY, 0);
    }
    else
    {
        LOG_WARN("File '%s' is not ZCR\n", file);
    }
    mfclose(f);
}
/******************** ZCR END************************/


/******************** ZFS Routines************************/
typedef struct
{
    uint32_t magic;
    uint32_t unk1;
    uint32_t unk2;
    uint32_t files_perblock;
    uint32_t files_cnt;
    uint32_t xor_key;
    uint32_t Offset_files;
} header_zfs;

typedef struct
{
    char name[0x10];
    uint32_t offset;
    uint32_t id;
    uint32_t size;
    uint32_t time;
    uint32_t unk2;
} file_header_zfs;

static MList *zfs_arch_list = NULL;

void loader_openzfs(const char *file, MList *list)
{
    TRACE_LOADER("Loading ZFS : %s\n", file);

    FILE *fl = fopen(file, "rb");
    if (!fl)
        Z_PANIC("Error: File '%s' not found!\n", file);

    header_zfs hdr;
    fread(&hdr, sizeof(hdr), 1, fl);

    if (hdr.magic != MAGIC_ZFS)
    {
        LOG_WARN("File '%s' is not valid ZFS\n", file);
        return;
    }

    zfs_arch_t *tmp = NEW(zfs_arch_t);
    tmp->fl = fl;
    tmp->xor_key = hdr.xor_key;

    if (!zfs_arch_list)
        zfs_arch_list = CreateMList();

    AddToMList(zfs_arch_list, tmp);

    uint32_t nextpos = hdr.Offset_files;

    while (nextpos != 0)
    {
        fseek(fl, nextpos, 0);
        fread(&nextpos, 4, 1, fl);

        for (uint32_t i = 0; i < hdr.files_perblock; i++)
        {
            //fseek(f,pos,0);
            file_header_zfs fil;
            fread(&fil, sizeof(fil), 1, fl);

            if (!str_empty(fil.name))
            {
                TRACE_LOADER("Adding zfs file : %s\n", fil.name);

                FManNode_t *nod = NEW(FManNode_t);
                nod->path = strdup(fil.name);
                nod->name = nod->path;
                nod->zfs = NEW(zfs_file_t);
                nod->zfs->archive = tmp;
                nod->zfs->offset = fil.offset;
                nod->zfs->size = fil.size;
                AddToMList(list, nod);
                AddToBinTree(nod);
            }
        }
    }
}
/******************** ZFS END************************/


/******************* STRINGS *****************/
char **loader_loadStr_m(mfile_t *mfp)
{
    char buffer[STRBUFSIZE];
    int lines = 1;

    if (!mfp)
        return NULL;

    m_wide_to_utf8(mfp);

    for (int i = strlen(mfp->buf); i > 0; i--)
    {
        if (mfp->buf[i] == '\n')
            lines++;
    }

    char **strings = NEW_ARRAY(char *, lines + 10);
    int pos = 0;

    while (!mfeof(mfp))
    {
        mfgets(buffer, STRBUFSIZE, mfp);
        strings[pos++] = str_trim(buffer);
    }
    strings[pos] = NULL;

    mfclose(mfp);

    return strings;
}

char **loader_loadStr(const char *path)
{
    FManNode_t file = {
        .name = (char*)path,
        .path = (char*)path,
        .zfs = NULL
    };
    return loader_loadStr_m(mfopen(&file));
}
/***************** STRINGS END ***************/


/**************** FS and ZFS ACCESS *****************/
mfile_t *mfopen(FManNode_t *nod)
{
    if (!nod)
        Z_PANIC("mfopen(NULL)");

    mfile_t *tmp = NEW(mfile_t);
    zfs_file_t *zfile = nod->zfs;

    if (zfile)
    {
        tmp->buf = NEW_ARRAY(char, zfile->size + 4);
        tmp->size = zfile->size;

        fseek(zfile->archive->fl, zfile->offset, SEEK_SET);
        fread(tmp->buf, zfile->size, 1, zfile->archive->fl);

        if (zfile->archive->xor_key)
        {
            size_t cnt = zfile->size >> 2;
            for (size_t i = 0; i < cnt; i++)
                ((uint32_t*)tmp->buf)[i] ^= zfile->archive->xor_key;
        }
    }
    else
    {
        FILE *fp = fopen(nod->path, "rb");
        if (!fp)
            Z_PANIC("File '%s' could not be opened\n", nod->path);

        fseek(fp, 0, SEEK_END);

        tmp->size = ftell(fp);
        tmp->buf = NEW_ARRAY(char, tmp->size);

        fseek(fp, 0, SEEK_SET);
        fread(tmp->buf, tmp->size, 1, fp);

        fclose(fp);
    }

    return tmp;
}

int32_t mfsize(FManNode_t *nod)
{
    struct stat statbuf;
    if (nod->zfs)
        return nod->zfs->size;
    if (stat(nod->path, &statbuf) == 0)
        return statbuf.st_size;
    return -1;
}

bool mfread(void *buf, int32_t bytes, mfile_t *file)
{
    if (file->pos + bytes > file->size)
        return false;

    memcpy(buf, file->buf + file->pos, bytes);

    file->pos += bytes;

    return true;
}

bool mfread_ptr(void **buf, int32_t bytes, mfile_t *file)
{
    if (file->pos + bytes > file->size)
        return false;

    *buf = file->buf + file->pos;

    file->pos += bytes;

    return true;
}

void mfseek(mfile_t *fil, int32_t pos)
{
    if (pos <= fil->size && pos >= 0)
        fil->pos = pos;
}

int32_t mftell(mfile_t *fil)
{
    return fil ? fil->pos : -1;
}

void mfclose(mfile_t *fil)
{
    free(fil->buf);
    free(fil);
}

bool mfeof(mfile_t *fil)
{
    return (fil->pos >= fil->size);
}

char *mfgets(char *str, int32_t num, mfile_t *stream)
{
    int32_t copied = 0;
    num--;

    if (mfeof(stream))
        return NULL;

    while ((copied < num) && (stream->pos < stream->size) && (stream->buf[stream->pos] != 0x0A) && (stream->buf[stream->pos] != 0x0D))
    {
        str[copied] = stream->buf[stream->pos];
        copied++;
        stream->pos++;
    }

    str[copied] = 0;

    if (stream->pos < stream->size)
    {
        if (stream->buf[stream->pos] == 0x0D)
        {
            stream->pos++;

            if (stream->pos < stream->size)
                if (stream->buf[stream->pos] == 0xA) //windows
                    stream->pos++;
        }
        else if (stream->buf[stream->pos] == 0x0D) //unix
            stream->pos++;
    }

    return str;
}

static bool m_is_wide_char(mfile_t *file)
{
    for (int32_t i = 0; i < file->size - 2; i++)
        if (file->buf[i] == 0 && file->buf[i + 2] == 0)
            return true;

    return false;
}

void m_wide_to_utf8(mfile_t *file)
{
    if (!m_is_wide_char(file))
        return;

    char *buf = (char *)malloc(file->size * 2);
    int32_t pos = 0, size = file->size * 2;
    file->pos = 0;
    while (file->pos < file->size && pos < size)
    {
        if (file->buf[file->pos] == 0xD)
        {
            buf[pos] = 0xD;
            pos++;
            file->pos++;
            if (file->pos < file->size)
                if (file->buf[file->pos] == 0xA)
                {
                    if (pos < size)
                    {
                        buf[pos] = 0xA;
                        pos++;
                    }
                    file->pos++;
                    if (file->pos < file->size)
                        if (file->buf[file->pos] == 0x0)
                            file->pos++;
                }
        }
        else
        {
            if (file->pos + 1 < file->size)
            {
                uint16_t ch = (file->buf[file->pos] & 0xFF) | ((file->buf[file->pos + 1] << 8) & 0xFF00);
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

    free(file->buf);
    file->pos = 0;
    file->size = pos;
    file->buf = buf;
}
/************** END FS and ZFS ACCESS ***************/
