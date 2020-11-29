#ifndef SIMPLE_AVI_H_INCLUDED
#define SIMPLE_AVI_H_INCLUDED

#include <SDL/SDL_audio.h>
#include <SDL/SDL_mixer.h>

#define AVI_PLAY 1
#define AVI_STOP 0

typedef struct
{
    uint32_t size;
    uint32_t streamType;
    uint32_t streamHandler;
    uint32_t flags;
    uint16_t priority;
    uint16_t language;
    uint32_t initialFrames;
    uint32_t scale;
    uint32_t rate;
    uint32_t start;
    uint32_t length;
    uint32_t bufferSize;
    uint32_t quality;
    uint32_t sampleSize;
    //Common::Rect frame;
} avi_strm_hdr_t;

typedef struct
{
    uint32_t id;
    uint32_t flags;
    uint32_t offset;
    uint32_t size;
} vid_idx_t;

typedef struct
{
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint16_t planes;
    uint16_t bitCount;
    uint32_t compression;
    uint32_t sizeImage;
    uint32_t xPelsPerMeter;
    uint32_t yPelsPerMeter;
    uint32_t clrUsed;
    uint32_t clrImportant;
    avi_strm_hdr_t hdr;
} vid_trk_t;

typedef struct
{
    uint16_t tag;
    uint16_t channels;
    uint32_t samplesPerSec;
    uint32_t avgBytesPerSec;
    uint16_t blockAlign;
    uint16_t size;
    avi_strm_hdr_t hdr;
} aud_trk_t;

typedef struct
{
    uint32_t fof;
    uint32_t sz;
    bool kfrm;
} vframes_t;

#define MAX_MOVI 10

typedef struct
{
    int8_t status;
    uint32_t stime;

    FILE *file;
    uint32_t size;
    uint32_t cframe;

    int32_t w, h;

    struct header
    {
        uint32_t size;
        uint32_t mcrSecPframe;
        uint32_t maxbitstream;
        uint32_t padding;
        uint32_t flags;
        uint32_t frames;
        uint32_t iframes;
        uint32_t streams;
        uint32_t buffsize;
        int32_t width;
        int32_t height;
    } header;

    struct movi
    {
        uint32_t fofset;
        uint32_t offset;
        uint32_t ssize;
    } movi[MAX_MOVI];

    uint32_t movi_cnt;

    uint32_t idx_cnt;
    vid_idx_t *idx;
    vid_trk_t *vtrk;
    aud_trk_t *atrk;

    int32_t pix_fmt;

    void *frame;

    void *priv_data;

    vframes_t *vfrm;
    int32_t vfrm_cnt;

    vframes_t *achunk;
    int32_t achunk_cnt;

    uint32_t lastrnd;

    void *buf;

    uint8_t translate;
} avi_file_t;

typedef struct
{
    int32_t t[4];
    uint32_t j;
} adpcm_context_t;

avi_file_t *avi_openfile(const char *filename, uint8_t transl);
void avi_set_dem(avi_file_t *av, int32_t w, int32_t h);
int8_t avi_renderframe(avi_file_t *av, int32_t frm);
void avi_play(avi_file_t *av);
Mix_Chunk *avi_get_audio(avi_file_t *av);
void avi_blit(avi_file_t *av, SDL_Surface *srf);
void avi_stop(avi_file_t *av);
void avi_update(avi_file_t *av);
void avi_close(avi_file_t *av);

Mix_Chunk *wav_create(const void *data, size_t data_len, int channels, int freq, int bits, int adpcm);

#endif // SIMPLE_AVI_H_INCLUDED
