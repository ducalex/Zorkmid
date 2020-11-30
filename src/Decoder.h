#ifndef SIMPLE_AVI_H_INCLUDED
#define SIMPLE_AVI_H_INCLUDED

#define MAX_MOVI 10

typedef struct __attribute__((packed))
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
} avi_strm_hdr_t;

typedef struct __attribute__((packed))
{
    uint32_t id;
    uint32_t flags;
    uint32_t offset;
    uint32_t size;
} vid_idx_t;

typedef struct __attribute__((packed))
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
} vid_trk_t;

typedef struct __attribute__((packed))
{
    uint16_t tag;
    uint16_t channels;
    uint32_t samplesPerSec;
    uint32_t avgBytesPerSec;
    uint16_t blockAlign;
    uint16_t size;
} aud_trk_t;

typedef struct
{
    size_t foffset;
    size_t offset;
    size_t size;
    bool kfrm;
} avi_data_block_t;

typedef struct __attribute__((packed))
{
    /* Packed header */
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
    /* end packed header */

    avi_data_block_t *vframes;
    size_t vframes_cnt;

    avi_data_block_t *achunks;
    size_t achunks_cnt;

    avi_data_block_t movi[MAX_MOVI];
    size_t movi_cnt;

    size_t idx_cnt;
    vid_idx_t *idx;
    vid_trk_t *vtrk;
    aud_trk_t *atrk;

    void *ctx;
    void *buf;

    void *framebuffer;
    SDL_Surface *surf;
    uint32_t pixel_format;

    FILE *fp;

    /* Status */
    bool playing;
    bool translate;
    uint32_t start_time;
    long cur_frame;
    long rend_frame;
    long w, h;

} avi_file_t;

avi_file_t *avi_openfile(const char *filename, uint8_t transl);
void avi_set_dem(avi_file_t *av, int32_t w, int32_t h);
int8_t avi_renderframe(avi_file_t *av, int32_t frm);
void avi_play(avi_file_t *av);
Mix_Chunk *avi_get_audio(avi_file_t *av);
void avi_stop(avi_file_t *av);
void avi_update(avi_file_t *av);
void avi_close(avi_file_t *av);

Mix_Chunk *wav_create(const void *data, size_t data_len, int channels, int freq, int bits, int adpcm);

#endif // SIMPLE_AVI_H_INCLUDED
