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

void snd_DeleteLoopedWavsByOwner(pzllst *owner);
void snd_DeleteNoUniverse(pzllst *owner);

struct_action_res *snd_CreateWavNode();
int snd_ProcessWav(struct_action_res *nod);
int snd_DeleteWav(struct_action_res *nod);

struct_action_res *snd_CreateSyncNode();
int snd_ProcessSync(struct_action_res *nod);
int snd_DeleteSync(struct_action_res *nod);

struct_action_res *snd_CreatePanTrack();
int snd_ProcessPanTrack(struct_action_res *nod);
int snd_DeletePanTrack(struct_action_res *nod);

#endif // SOUND_H_INCLUDED
