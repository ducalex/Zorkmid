#include "System.h"

int truemotion1_decode_init(avi_file_t *fil);
int truemotion1_decode_frame(avi_file_t *avctx, void *pkt, int pkt_sz);
int truemotion1_decode_end(avi_file_t *avctx);
void avi_build_vlist(avi_file_t *av);

#define MKTAG(a0, a1, a2, a3) ((uint32_t)(((a3) << 24) | ((a2) << 16) | ((a1) << 8) | ((a0) << 0)))

#define ID_RIFF MKTAG('R', 'I', 'F', 'F')
#define ID_AVI MKTAG('A', 'V', 'I', ' ')
#define ID_LIST MKTAG('L', 'I', 'S', 'T')
#define ID_HDRL MKTAG('h', 'd', 'r', 'l')
#define ID_AVIH MKTAG('a', 'v', 'i', 'h')
#define ID_STRL MKTAG('s', 't', 'r', 'l')
#define ID_STRH MKTAG('s', 't', 'r', 'h')
#define ID_VIDS MKTAG('v', 'i', 'd', 's')
#define ID_AUDS MKTAG('a', 'u', 'd', 's')
#define ID_MIDS MKTAG('m', 'i', 'd', 's')
#define ID_TXTS MKTAG('t', 'x', 't', 's')
#define ID_JUNK MKTAG('J', 'U', 'N', 'K')
#define ID_STRF MKTAG('s', 't', 'r', 'f')
#define ID_MOVI MKTAG('m', 'o', 'v', 'i')
#define ID_REC MKTAG('r', 'e', 'c', ' ')
#define ID_VEDT MKTAG('v', 'e', 'd', 't')
#define ID_IDX1 MKTAG('i', 'd', 'x', '1')
#define ID_STRD MKTAG('s', 't', 'r', 'd')
#define ID_00AM MKTAG('0', '0', 'A', 'M')

#define MKTAG16(a0, a1) ((uint16_t)((a0) | ((a1) << 8)))

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

static uint16_t getStreamType(uint32_t tag)
{
    return (tag >> 16) & 0xffff;
}

uint32_t readu32(FILE *file)
{
    uint32_t tmp = 0;
    fread(&tmp, 4, 1, file);
    return tmp;
}

uint16_t readu16(FILE *file)
{
    uint16_t tmp = 0;
    fread(&tmp, 2, 1, file);
    return tmp;
}

static void adpcm8_decode(void *in, void *out, int8_t stereo, int32_t n, adpcm_context_t *ctx)
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

static int32_t avi_parse_handle(avi_file_t *av, uint32_t tag)
{
    switch (tag)
    {
    case ID_LIST:
    {
        uint32_t listSize = readu32(av->file) - 4;
        uint32_t listType = readu32(av->file);
        uint32_t curPos = ftell(av->file);

        if (listType == ID_STRL || listType == ID_HDRL)
        {
            while ((ftell(av->file) - curPos) < listSize)
                avi_parse_handle(av, readu32(av->file));
        }
        else if (listType == ID_MOVI)
        {
            if (av->movi_cnt < MAX_MOVI)
            {
                uint32_t ofmx = 0;
                if (av->movi_cnt > 0)
                    ofmx = av->movi[av->movi_cnt - 1].offset + av->movi[av->movi_cnt - 1].ssize;

                av->movi[av->movi_cnt].fofset = ftell(av->file);
                av->movi[av->movi_cnt].offset = ofmx;
                av->movi[av->movi_cnt].ssize = listSize;
                av->movi_cnt++;
                fseek(av->file, listSize, SEEK_CUR);
            }
        }
        else
        {
            fseek(av->file, listSize, SEEK_CUR);
        }
    }
    break;

    case ID_AVIH:
        av->header.size = readu32(av->file);
        av->header.mcrSecPframe = readu32(av->file);
        av->header.maxbitstream = readu32(av->file);
        av->header.padding = readu32(av->file);
        av->header.flags = readu32(av->file);
        av->header.frames = readu32(av->file);
        av->header.iframes = readu32(av->file);
        av->header.streams = readu32(av->file);
        av->header.buffsize = readu32(av->file);
        av->header.width = readu32(av->file);
        av->header.height = readu32(av->file);
        fseek(av->file, 16, SEEK_CUR);
        break;

    case ID_STRH:
    {
        avi_strm_hdr_t hdr;
        hdr.size = readu32(av->file);
        hdr.streamType = readu32(av->file);
        hdr.streamHandler = readu32(av->file);
        hdr.flags = readu32(av->file);
        hdr.priority = readu16(av->file);
        hdr.language = readu16(av->file);
        hdr.initialFrames = readu32(av->file);
        hdr.scale = readu32(av->file);
        hdr.rate = readu32(av->file);
        hdr.start = readu32(av->file);
        hdr.length = readu32(av->file);
        hdr.bufferSize = readu32(av->file);
        hdr.quality = readu32(av->file);
        hdr.sampleSize = readu32(av->file);
        fseek(av->file, hdr.size - 48, SEEK_CUR);

        if (readu32(av->file) != ID_STRF)
            return -1;

        uint32_t strfSize = readu32(av->file);
        uint32_t startPos = ftell(av->file);

        if (hdr.streamType == ID_VIDS)
        {
            vid_trk_t *inf = NEW(vid_trk_t);
            inf->size = readu32(av->file);
            inf->width = readu32(av->file);
            inf->height = readu32(av->file);
            inf->planes = readu16(av->file);
            inf->bitCount = readu16(av->file);
            inf->compression = readu32(av->file);
            inf->sizeImage = readu32(av->file);
            inf->xPelsPerMeter = readu32(av->file);
            inf->yPelsPerMeter = readu32(av->file);
            inf->clrUsed = readu32(av->file);
            inf->clrImportant = readu32(av->file);

            inf->hdr = hdr;

            if (av->vtrk == NULL)
                av->vtrk = inf;
            else
                free(inf);
        }
        else if (hdr.streamType == ID_AUDS)
        {
            aud_trk_t *wv = NEW(aud_trk_t);
            wv->tag = readu16(av->file);
            wv->channels = readu16(av->file);
            wv->samplesPerSec = readu32(av->file);
            wv->avgBytesPerSec = readu32(av->file);
            wv->blockAlign = readu16(av->file);
            wv->size = readu16(av->file);

            if (wv->channels == 2)
                hdr.sampleSize /= 2;

            wv->hdr = hdr;
            if (av->atrk == NULL)
                av->atrk = wv;
            else
                free(wv);
        }

        fseek(av->file, startPos + strfSize, SEEK_SET);
    }
    break;

    case ID_STRD:
    case ID_VEDT:
    case ID_JUNK:
    {
        uint32_t junkSize = readu32(av->file);
        fseek(av->file, (junkSize + 1) & (~0x1), SEEK_CUR);
    }
    break;

    case ID_IDX1:
    {
        uint32_t cnt = readu32(av->file) / 16;
        av->idx_cnt = cnt;
        av->idx = NEW_ARRAY(vid_idx_t, cnt);

        for (uint32_t i = 0; i < cnt; i++)
        {
            av->idx[i].id = readu32(av->file);
            av->idx[i].flags = readu32(av->file);
            av->idx[i].offset = readu32(av->file);
            av->idx[i].size = readu32(av->file);
        }
    }
    break;
    }
    return 0;
}

static int avi_get_offset(avi_file_t *av, uint32_t virt)
{
    for (uint32_t i = 0; i < av->movi_cnt; i++)
    {
        if (virt >= av->movi[i].offset && virt < av->movi[i].offset + av->movi[i].ssize)
            return av->movi[i].fofset + (virt - av->movi[i].offset);
    }
    return 0;
}

avi_file_t *avi_openfile(const char *fle, uint8_t transl)
{
    FILE *file = fopen(fle, "rb");
    if (!file)
        return NULL;

    avi_file_t *tmp = NEW(avi_file_t);

    if (readu32(file) != ID_RIFF)
        goto ERROR;

    tmp->file = file;
    tmp->size = readu32(file);

    if (readu32(file) != ID_AVI)
        goto ERROR;

    while (!feof(file))
        avi_parse_handle(tmp, readu32(tmp->file));

    if (tmp->vtrk)
    {
        tmp->translate = transl;
        tmp->lastrnd = 0;
        avi_set_dem(tmp, tmp->header.width, tmp->header.height);
        truemotion1_decode_init(tmp);
        avi_build_vlist(tmp);
        tmp->buf = malloc(tmp->header.buffsize);
        return tmp;
    }

ERROR:
    fclose(file);
    free(tmp);
    return NULL;
}

void avi_set_dem(avi_file_t *av, int32_t w, int32_t h)
{
    if (av->frame)
        free(av->frame);

    av->frame = malloc(w * h * 4);
    av->header.width = w;
    av->header.height = h;
    if (av->translate)
    {
        av->w = h;
        av->h = w;
    }
    else
    {
        av->w = w;
        av->h = h;
    }
}

void avi_build_vlist(avi_file_t *av)
{
    int32_t cnt = 0;
    int32_t acnt = 0;

    for (uint32_t i = 0; i < av->idx_cnt; i++)
        if (getStreamType(av->idx[i].id) == MKTAG16('d', 'c'))
            cnt++;
        else if (getStreamType(av->idx[i].id) == MKTAG16('w', 'b'))
            acnt++;

    av->vfrm = NEW_ARRAY(vframes_t, cnt);
    if (acnt > 0)
        av->achunk = NEW_ARRAY(vframes_t, acnt);

    av->vfrm_cnt = cnt;
    av->achunk_cnt = acnt;
    cnt = 0;
    acnt = 0;
    for (uint32_t i = 0; i < av->idx_cnt; i++)
        if (getStreamType(av->idx[i].id) == MKTAG16('d', 'c'))
        {
            av->vfrm[cnt].fof = avi_get_offset(av, av->idx[i].offset) + 4;
            av->vfrm[cnt].sz = av->idx[i].size;
            av->vfrm[cnt].kfrm = (av->idx[i].flags & 0x10) != 0;
            cnt++;
        }
        else if (getStreamType(av->idx[i].id) == MKTAG16('w', 'b'))
        {
            av->achunk[acnt].fof = avi_get_offset(av, av->idx[i].offset) + 4;
            av->achunk[acnt].sz = av->idx[i].size;
            acnt++;
        }
}

int8_t avi_renderframe(avi_file_t *av, int32_t frm)
{
    if (frm >= 0 && frm < av->vfrm_cnt)
    {
        if (av->vfrm[frm].sz <= av->header.buffsize)
        {
            if (av->vfrm[frm].kfrm || abs((int)(frm - av->lastrnd)) <= 1)
            {
                fseek(av->file, av->vfrm[frm].fof, SEEK_SET);
                fread(av->buf, av->vfrm[frm].sz, 1, av->file);
                truemotion1_decode_frame(av, av->buf, av->vfrm[frm].sz);
            }
            else
            {
                int32_t st = 0;
                for (st = frm; st > 0; st--)
                    if (av->vfrm[st].kfrm)
                        break;
                for (; st <= frm; st++)
                {
                    fseek(av->file, av->vfrm[st].fof, SEEK_SET);
                    fread(av->buf, av->vfrm[st].sz, 1, av->file);
                    truemotion1_decode_frame(av, av->buf, av->vfrm[st].sz);
                }
            }
            av->lastrnd = frm;
        }
        else
        {
            printf("oversize\n");
        }
        return 0;
    }
    return -1;
}

Mix_Chunk *avi_get_audio(avi_file_t *av)
{
    if (!av || !av->atrk)
        return NULL;

    uint32_t asz = 0;
    uint32_t tmp = 0;
    for (int i = 0; i < av->achunk_cnt; i++)
        asz += av->achunk[i].sz;

    if (av->atrk->tag == 0x1) // PCM
    {
        size_t buffer_size = asz + sizeof(wavHeader);
        uint32_t *raw = (uint32_t *)malloc(buffer_size);
        memcpy(raw, wavHeader, sizeof(wavHeader));
        raw[7] = av->atrk->channels * raw[6] * av->atrk->size / 8;
        raw[8] = (av->atrk->size << 16) | (av->atrk->size * av->atrk->channels / 8);
        raw[10] = asz;
        tmp = 0;
        for (int i = 0; i < av->achunk_cnt; i++)
        {
            uint8_t *rw = (uint8_t *)(&raw[11]);
            fseek(av->file, av->achunk[i].fof, SEEK_SET);
            fread(rw + tmp, av->achunk[i].sz, 1, av->file);
            tmp += av->achunk[i].sz;
        }

        return Mix_LoadWAV_RW(SDL_RWFromMem(raw, buffer_size), 1);
    }
    else if (av->atrk->tag == 0x11) // Intel's DVI ADPCM
    {
        size_t buffer_size = asz * 2 + sizeof(wavHeader);
        uint32_t *raw = (uint32_t *)malloc(buffer_size);
        memcpy(raw, wavHeader, sizeof(wavHeader));
        raw[1] = buffer_size - 8;
        raw[5] = (av->atrk->channels << 16) | 0x01;
        raw[6] = av->atrk->samplesPerSec;
        raw[7] = av->atrk->channels * raw[6] * av->atrk->size / 4;
        raw[8] = (av->atrk->size * av->atrk->channels << 16) | (av->atrk->size * av->atrk->channels / 4);
        //raw[8] = 0x100000 | (av->atrk->size*av->atrk->channels/8);
        raw[10] = asz * 2;
        tmp = 0;

        adpcm_context_t ctx;
        memset(&ctx, 0, sizeof(ctx));

        for (int i = 0; i < av->achunk_cnt; i++)
        {
            uint8_t *rw = (uint8_t *)(&raw[11]);
            fseek(av->file, av->achunk[i].fof, SEEK_SET);
            fread(av->buf, av->achunk[i].sz, 1, av->file);
            adpcm8_decode(av->buf, rw + tmp, (av->atrk->channels - 1), av->achunk[i].sz, &ctx);
            tmp += av->achunk[i].sz * 2;
        }

        return Mix_LoadWAV_RW(SDL_RWFromMem(raw, buffer_size), 1);
    }

    printf("auds tag: %x\n", av->atrk->tag);
    return NULL;
}

void avi_play(avi_file_t *av)
{
    av->status = AVI_PLAY;
    av->cframe = 0;

    avi_renderframe(av, 0);

    av->stime = Game_GetTime();
}

void avi_update(avi_file_t *av)
{
    if (av->status == AVI_PLAY)
    {
        float ss = Game_GetTime() - av->stime;
        uint32_t nwf = (ss / ((float)(av->header.mcrSecPframe) / 1000.0));

        if (nwf >= av->header.frames)
        {
            av->status = AVI_STOP;
            av->cframe = av->header.frames - 1;
        }
        else if (nwf != av->cframe)
        {
            av->cframe = nwf;
            avi_renderframe(av, nwf);
        }
    }
}

void avi_stop(avi_file_t *av)
{
    av->status = AVI_STOP;
}

void avi_to_surf(avi_file_t *av, SDL_Surface *srf)
{
    if (av->pix_fmt != 16)
    {
        Z_PANIC("AVI Bit depth %d not supported!\n", av->pix_fmt);
    }

    SDL_LockSurface(srf);

    bool fullscreen = av->w == srf->w && av->h == srf->h;

    uint16_t *img = (uint16_t *)av->frame;
    uint16_t color;

    float xperc = (float)av->w / (float)srf->w;
    float yperc = (float)av->h / (float)srf->h;

    for (int y = 0; y < srf->h; y++)
    {
        for (int x = 0; x < srf->w; x++)
        {
            if (fullscreen)
            {
                if (av->translate == 0)
                    color = *img++;
                else
                    color = img[x * av->h + y];
            }
            else
            {
                if (av->translate == 0)
                    color = img[av->w * (int32_t)(y * yperc) + (int32_t)(x * xperc)];
                else
                    color = img[(int32_t)(x * yperc) * av->h + (int32_t)(y * xperc)];
            }

            // Video is 555 format
            uint8_t r = (color >> 10) & 0x1F;
            uint8_t g = (color >> 5) & 0x1F;
            uint8_t b = (color >> 0) & 0x1F;

            Rend_SetPixel(srf, x, y, r << 3, g << 3, b << 3);
        }
    }

    SDL_UnlockSurface(srf);
}

void avi_close(avi_file_t *av)
{
    if (av->file)
        fclose(av->file);

    truemotion1_decode_end(av);

    free(av->frame);
    free(av->buf);
    free(av->atrk);
    free(av->vtrk);
    free(av->idx);
    free(av->vfrm);
    free(av->achunk);
    free(av);
}

Mix_Chunk *wav_create(void *data, size_t data_len, int channels, int freq, int bits, int adpcm)
{
    size_t final_size = sizeof(wavHeader) + (adpcm ? data_len * 2 : data_len);

    uint32_t *buffer = malloc(final_size);

    memcpy(buffer, wavHeader, sizeof(wavHeader));

    buffer[1] = final_size - 8;
    buffer[5] = (channels << 16) + 1;
    buffer[6] = freq;
    if (bits == 16)
    {
        buffer[7] = freq << channels;
        buffer[8] = 0x100000 + channels * 2;
    }
    else
    {
        buffer[7] = freq << (channels-1);
        buffer[8] = 0x100000 + (channels-1) * 2;
    }
    buffer[10] = final_size - sizeof(wavHeader);

    if (adpcm)
        adpcm8_decode(data, &buffer[11], channels > 1, data_len, NULL);
    else
        memcpy(&buffer[11], data, data_len);

    return Mix_LoadWAV_RW(SDL_RWFromMem(buffer, final_size), 1);
}
