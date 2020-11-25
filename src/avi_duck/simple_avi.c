#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <ctype.h>

#include "../System.h"

#include "simple_avi.h"

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

static char char2num(char c)
{
    c = tolower((char)c);
    return (c >= 'a' && c <= 'f') ? c - 'a' + 10 : c - '0';
}

char getStreamIndex(uint32_t tag)
{
    return char2num((tag >> 24) & 0xFF) << 4 | char2num((tag >> 16) & 0xFF);
}

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

int32_t reads32(FILE *file)
{
    int32_t tmp = 0;
    fread(&tmp, 4, 1, file);
    return tmp;
}

uint16_t readu16(FILE *file)
{
    uint16_t tmp = 0;
    fread(&tmp, 2, 1, file);
    return tmp;
}

int16_t reads16(FILE *file)
{
    int16_t tmp = 0;
    fread(&tmp, 2, 1, file);
    return tmp;
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

avi_file_t *avi_openfile(const char *fle, uint8_t transl)
{
    FILE *file = fopen(fle, "rb");
    if (!file)
        return NULL;

    avi_file_t *tmp = NEW(avi_file_t);

    tmp->file = file;

    uint32_t tch = 0;
    //    uint32_t ntag;

    tch = readu32(file);

    if (tch != ID_RIFF)
        goto ERROR;

    tmp->size = readu32(file);

    tch = readu32(file);

    if (tch != ID_AVI)
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

int32_t avi_get_off(avi_file_t *av, uint32_t virt)
{
    for (uint32_t i = 0; i < av->movi_cnt; i++)
    {
        if (virt >= av->movi[i].offset && virt < av->movi[i].offset + av->movi[i].ssize)
            return av->movi[i].fofset + (virt - av->movi[i].offset);
    }
    return 0;
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
            av->vfrm[cnt].fof = avi_get_off(av, av->idx[i].offset) + 4;
            av->vfrm[cnt].sz = av->idx[i].size;
            av->vfrm[cnt].kfrm = (av->idx[i].flags & 0x10) != 0;
            cnt++;
        }
        else if (getStreamType(av->idx[i].id) == MKTAG16('w', 'b'))
        {
            av->achunk[acnt].fof = avi_get_off(av, av->idx[i].offset) + 4;
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

Mix_Chunk *avi_get_audio(avi_file_t *av)
{
    if (av->atrk)
    {
        uint32_t asz = 0;
        uint32_t tmp = 0;
        for (int i = 0; i < av->achunk_cnt; i++)
            tmp += av->achunk[i].sz;
        asz = tmp;

        if (av->atrk->tag == 0x1) // PCM
        {
            uint32_t *raw = (uint32_t *)malloc(asz + 0x2C);
            memcpy(raw, wavHeader, 0x2C);
            raw[1] = asz - 8 + 0x2C;
            raw[5] = (av->atrk->channels << 16) | 0x01;
            raw[6] = av->atrk->samplesPerSec;
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

            return Mix_LoadWAV_RW(SDL_RWFromMem(raw, asz + 0x2C), 1);
        }
        else if (av->atrk->tag == 0x11) // Intel's DVI ADPCM
        {
            uint32_t *raw = (uint32_t *)malloc(asz * 2 + 0x2C);
            memcpy(raw, wavHeader, 0x2C);
            raw[1] = asz * 2 - 8 + 0x2C;
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

            return Mix_LoadWAV_RW(SDL_RWFromMem(raw, asz * 2 + 0x2C), 1);
        }
        else
        {
            printf("auds tag: %x\n", av->atrk->tag);
        }
    }
    return NULL;
}

void avi_play(avi_file_t *av)
{
    av->status = AVI_PLAY;
    av->cframe = 0;

    avi_renderframe(av, 0);

    av->stime = SDL_GetTicks();
}

void avi_update(avi_file_t *av)
{
    if (av->status == AVI_PLAY)
    {
        float ss = SDL_GetTicks() - av->stime;
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
