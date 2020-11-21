#ifndef SOUND_H_INCLUDED
#define SOUND_H_INCLUDED

void InitSound();
void DeinitMusic();
int GetFreeChannel();
void LockChan(int i);
void UnlockChan(int i);
void SaveVol();
void SilenceVol();
void RestoreVol();
uint32_t GetChanTime(int i);
int GetLogVol(uint8_t linear);

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

void snd_DeleteLoopedWavsByOwner(pzllst_t *owner);
void snd_DeleteNoUniverse(pzllst_t *owner);

action_res_t *snd_CreateWavNode();
int snd_ProcessWav(action_res_t *nod);
int snd_DeleteWav(action_res_t *nod);

action_res_t *snd_CreateSyncNode();
int snd_ProcessSync(action_res_t *nod);
int snd_DeleteSync(action_res_t *nod);

action_res_t *snd_CreatePanTrack();
int snd_ProcessPanTrack(action_res_t *nod);
int snd_DeletePanTrack(action_res_t *nod);

#endif // SOUND_H_INCLUDED
