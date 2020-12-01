#include "System.h"

typedef struct
{
    int32_t t[4];
    uint32_t j;
} adpcm_context_t;

int truemotion1_decode_init(avi_file_t *fil);
int truemotion1_decode_frame(avi_file_t *avctx, void *pkt, int pkt_sz);
int truemotion1_decode_end(avi_file_t *avctx);

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

static const uint8_t wavHeader[] =
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

static uint16_t isStreamType(uint32_t stream_tag, uint16_t type)
{
    return ((stream_tag >> 16) & 0xffff) == type;
}

uint32_t readu32(FILE *file)
{
    uint32_t tmp = 0;
    fread(&tmp, 4, 1, file);
    return tmp;
}

static void adpcm8_decode(const void *in, void *out, int8_t stereo, size_t n)
{
    const int16_t t1[] = {-1, -1, -1, 1, 4, 7, 10, 12};
    const uint16_t t2[] = {0x0007, 0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E,
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

    const uint8_t *input = in;
    int16_t *output = out;
    int32_t a, x, j = 0;
    int32_t b, i, t[4] = {0, 0, 0, 0};

    while (n)
    {
        a = *input;
        i = t[j + 2];
        x = t2[i];
        b = 0;

        if (a & 0x40) b += x;
        if (a & 0x20) b += x >> 1;
        if (a & 0x10) b += x >> 2;
        if (a & 0x08) b += x >> 3;
        if (a & 0x04) b += x >> 4;
        if (a & 0x02) b += x >> 5;
        if (a & 0x01) b += x >> 6;
        if (a & 0x80) b = -b;

        b += t[j];

        if (b > 32767)
            b = 32767;
        else if (b < -32768)
            b = -32768;

        i += t1[(a >> 4) & 7];

        if (i < 0)
            i = 0;
        else if (i > 88)
            i = 88;

        t[j] = b;
        t[j + 2] = i;
        j = (j + 1) & stereo;

        *output = b;

        input++;
        output++;
        n--;
    }
}

static void avi_parse_block(avi_file_t *av)
{
    switch (readu32(av->fp))
    {
    case ID_LIST:
    {
        uint32_t listSize = readu32(av->fp) - 4;
        uint32_t listType = readu32(av->fp);
        uint32_t curPos = ftell(av->fp);

        if (listType == ID_STRL || listType == ID_HDRL)
        {
            while ((ftell(av->fp) - curPos) < listSize)
                avi_parse_block(av);
        }
        else if (listType == ID_MOVI)
        {
            if (av->movi_cnt < MAX_MOVI)
            {
                uint32_t ofmx = 0;
                if (av->movi_cnt > 0)
                    ofmx = av->movi[av->movi_cnt - 1].offset + av->movi[av->movi_cnt - 1].size;

                av->movi[av->movi_cnt].foffset = ftell(av->fp);
                av->movi[av->movi_cnt].offset = ofmx;
                av->movi[av->movi_cnt].size = listSize;
                av->movi_cnt++;
                fseek(av->fp, listSize, SEEK_CUR);
            }
        }
        else
        {
            fseek(av->fp, listSize, SEEK_CUR);
        }
    }
    break;

    case ID_AVIH:
        fread(av, 44, 1, av->fp);
        fseek(av->fp, 16, SEEK_CUR);
        break;

    case ID_STRH:
    {
        avi_strm_hdr_t hdr;
        uint32_t type, size;

        fread(&hdr, 52, 1, av->fp);
        fseek(av->fp, hdr.size - 48, SEEK_CUR);
        fread(&type, 4, 1, av->fp);
        fread(&size, 4, 1, av->fp);

        if (type != ID_STRF)
            return;

        size_t next_block = ftell(av->fp) + size;

        if (hdr.streamType == ID_VIDS && !av->vtrk)
        {
            av->vtrk = NEW(vid_trk_t);
            fread(av->vtrk, 40, 1, av->fp);
        }
        else if (hdr.streamType == ID_AUDS && !av->atrk)
        {
            av->atrk = NEW(aud_trk_t);
            fread(av->atrk, 16, 1, av->fp);
        }

        fseek(av->fp, next_block, SEEK_SET);
    }
    break;

    case ID_STRD:
    case ID_VEDT:
    case ID_JUNK:
        fseek(av->fp, (readu32(av->fp) + 1) & (~0x1), SEEK_CUR);
        break;

    case ID_IDX1:
        av->idx_cnt = readu32(av->fp) / 16;
        av->idx = NEW_ARRAY(vid_idx_t, av->idx_cnt);
        fread(av->idx, 16, av->idx_cnt, av->fp);
        break;
    }
}

static int avi_get_offset(avi_file_t *av, uint32_t virt)
{
    for (uint32_t i = 0; i < av->movi_cnt; i++)
    {
        if (virt >= av->movi[i].offset && virt < av->movi[i].offset + av->movi[i].size)
            return av->movi[i].foffset + (virt - av->movi[i].offset);
    }
    return 0;
}

avi_file_t *avi_openfile(const char *filename, uint8_t translate)
{
    const char *path = Loader_FindFile(filename);
    if (!path)
    {
        LOG_WARN("AVI file '%s' not in assets list\n", filename);
        path = filename;
    }

    FILE *file = fopen(path, "rb");
    if (!file)
    {
        LOG_WARN("Unable to open avi file '%s'\n", path);
        return NULL;
    }

    avi_file_t *av = NEW(avi_file_t);
    uint32_t magic, size, type;

    fread(&magic, 4, 1, file);
    fread(&size, 4, 1, file);
    fread(&type, 4, 1, file);

    if (magic != ID_RIFF || type != ID_AVI)
        goto ERROR;

    av->fp = file;
    av->translate = translate;

    while (!feof(av->fp))
        avi_parse_block(av);

    if (!av->vtrk)
        goto ERROR;

    avi_set_dem(av, av->width, av->height);
    truemotion1_decode_init(av);

    av->buf = NEW_ARRAY(uint8_t, av->buffsize);

    av->vframes = NEW_ARRAY(avi_data_block_t, av->frames);
    av->achunks = NEW_ARRAY(avi_data_block_t, av->frames);

    for (size_t i = 0; i < av->idx_cnt; i++)
        if (isStreamType(av->idx[i].id, MKTAG16('d', 'c')))
        {
            av->vframes[av->vframes_cnt].foffset = avi_get_offset(av, av->idx[i].offset) + 4;
            av->vframes[av->vframes_cnt].size = av->idx[i].size;
            av->vframes[av->vframes_cnt].kfrm = (av->idx[i].flags & 0x10) != 0;
            av->vframes_cnt++;
        }
        else if (isStreamType(av->idx[i].id, MKTAG16('w', 'b')))
        {
            av->achunks[av->achunks_cnt].foffset = avi_get_offset(av, av->idx[i].offset) + 4;
            av->achunks[av->achunks_cnt].size = av->idx[i].size;
            av->achunks_cnt++;
        }

    return av;

ERROR:
    fclose(file);
    DELETE(av);
    return NULL;
}

void avi_set_dem(avi_file_t *av, int32_t w, int32_t h)
{
    if (av->surf)
        SDL_FreeSurface(av->surf);

    av->surf = Rend_CreateSurface(w, h, 0); // TO DO: Set correct pixel format!
    av->framebuffer = (void*)av->surf->pixels;
    av->width = w;
    av->height = h;
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

int8_t avi_renderframe(avi_file_t *av, int32_t frm)
{
    if (frm < 0 || frm >= av->frames)
        return -1;

    if (av->vframes[frm].size > av->buffsize)
    {
        LOG_WARN("Oversized frame: size:%d max:%d\n", av->vframes[frm].size, av->buffsize);
        return -1;
    }

    if (av->vframes[frm].kfrm || abs((int)(frm - av->rend_frame)) <= 1)
    {
        fseek(av->fp, av->vframes[frm].foffset, SEEK_SET);
        fread(av->buf, av->vframes[frm].size, 1, av->fp);
        truemotion1_decode_frame(av, av->buf, av->vframes[frm].size);
    }
    else
    {
        int32_t st = 0;
        for (st = frm; st > 0; st--)
            if (av->vframes[st].kfrm)
                break;
        for (; st <= frm; st++)
        {
            fseek(av->fp, av->vframes[st].foffset, SEEK_SET);
            fread(av->buf, av->vframes[st].size, 1, av->fp);
            truemotion1_decode_frame(av, av->buf, av->vframes[st].size);
        }
    }
    av->rend_frame = frm;
    return 0;
}

Mix_Chunk *avi_get_audio(avi_file_t *av)
{
    if (!av || !av->atrk)
        return NULL;

    if (av->atrk->tag != 0x1 && av->atrk->tag != 0x11)
    {
        LOG_WARN("Unsupported audio type: %x\n", av->atrk->tag);
        return NULL;
    }

    size_t buffer_size = 0;
    size_t buffer_pos = 0;
    for (int i = 0; i < av->achunks_cnt; i++)
        buffer_size += av->achunks[i].size;

    uint8_t *buffer = NEW_ARRAY(uint8_t, buffer_size);
    Mix_Chunk *chunk = NULL;

    for (int i = 0; i < av->achunks_cnt; i++)
    {
        fseek(av->fp, av->achunks[i].foffset, SEEK_SET);
        fread(buffer + buffer_pos, av->achunks[i].size, 1, av->fp);
        buffer_pos += av->achunks[i].size;
    }

    chunk = wav_create(
        buffer,
        buffer_size,
        av->atrk->channels,
        av->atrk->samplesPerSec,
        av->atrk->size,
        av->atrk->tag == 0x11);

    DELETE(buffer);

    return chunk;
}

void avi_play(avi_file_t *av)
{
    av->playing = true;
    av->cur_frame = 0;

    avi_renderframe(av, 0);

    av->start_time = Game_GetTime();
}

void avi_update(avi_file_t *av)
{
    if (av->playing)
    {
        float ss = Game_GetTime() - av->start_time;
        uint32_t nwf = (ss / ((float)(av->mcrSecPframe) / 1000.0));

        if (nwf >= av->frames)
        {
            av->playing = false;
            av->cur_frame = av->frames - 1;
        }
        else if (nwf != av->cur_frame)
        {
            av->cur_frame = nwf;
            avi_renderframe(av, nwf);
        }
    }
}

void avi_stop(avi_file_t *av)
{
    av->playing = false;
}

void avi_close(avi_file_t *av)
{
    truemotion1_decode_end(av);

    if (av->fp)
        fclose(av->fp);
    if (av->surf)
        SDL_FreeSurface(av->surf);
    DELETE(av->buf);
    DELETE(av->atrk);
    DELETE(av->vtrk);
    DELETE(av->idx);
    DELETE(av->vframes);
    DELETE(av->achunks);
    DELETE(av);
}

Mix_Chunk *wav_create(const void *data, size_t data_len, int channels, int freq, int bits, int adpcm)
{
    size_t final_size = sizeof(wavHeader) + (adpcm ? data_len * 2 : data_len);

    uint32_t *buffer = NEW_ARRAY(uint32_t, final_size / 4 + 1);

    if (adpcm)
        bits = 16;

    memcpy(buffer, wavHeader, sizeof(wavHeader));
    buffer[1] = final_size - 8;
    buffer[5] = (channels << 16) + 1; // (adpcm ? 0x11 : 0x01);
    buffer[6] = freq;
    buffer[7] = freq * channels * bits / 8;
    buffer[8] = (bits << 16) | (channels * bits / 8);
    buffer[10] = final_size - sizeof(wavHeader);

    if (adpcm)
        adpcm8_decode(data, &buffer[11], channels > 1, data_len);
    else
        memcpy((void*)buffer + sizeof(wavHeader), data, data_len);

    return Mix_LoadWAV_RW(SDL_RWFromMem(buffer, final_size), 1);
}
