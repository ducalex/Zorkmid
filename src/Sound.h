#ifndef SOUND_H_INCLUDED
#define SOUND_H_INCLUDED

typedef struct
{
    Mix_Chunk *chunk;
    int32_t chn;
    int32_t volume;
    bool looped;

    bool crossfade;
    struct crossfade_params
    {
        int deltavolume;
        int times;
    } crossfade_params;

    int attenuate;

    bool pantrack;
    int pantrack_X;

    int pantrack_angle;

    bool universe; //universe_music or music

    subtitles_t *sub;
} musicnode_t;

typedef struct
{
    int syncto;
    Mix_Chunk *chunk;
    int chn;
    subtitles_t *sub;
} syncnode_t;

void Sound_Init();
void Sound_DeInit();
int Sound_GetFreeChannel();
void Sound_LockChannel(int i);
void Sound_UnLockChannel(int i);
int Sound_GetLogVol(uint8_t linear);

action_res_t *Sound_CreateWavNode();
int Sound_ProcessWav(action_res_t *nod);
int Sound_DeleteWav(action_res_t *nod);

action_res_t *Sound_CreateSyncNode();
int Sound_ProcessSync(action_res_t *nod);
int Sound_DeleteSync(action_res_t *nod);

action_res_t *Sound_CreatePanTrack();
int Sound_ProcessPanTrack(action_res_t *nod);
int Sound_DeletePanTrack(action_res_t *nod);

#endif // SOUND_H_INCLUDED
