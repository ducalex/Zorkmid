#ifndef SOUND_H_INCLUDED
#define SOUND_H_INCLUDED

#define SOUND_CHANNELS  16
#define SOUND_FREQUENCY 44100

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
int Sound_Play(int channel, Mix_Chunk *chunk, int loops, int volume);
int Sound_Playing(int channel);
int Sound_Pause(int channel);
int Sound_Stop(int channel);
int Sound_SetVolume(int channel, int volume);
int Sound_SetPosition(int channel, int angle, int distance);

action_res_t *Sound_CreateNode(int type);
int Sound_DeleteNode(action_res_t *nod);
int Sound_ProcessNode(action_res_t *nod);

#endif // SOUND_H_INCLUDED
