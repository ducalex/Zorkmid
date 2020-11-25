#include "System.h"

#define TRY_CHANNELS 16

static int CHANNELS = 0;

static uint32_t chantime[TRY_CHANNELS];
static bool ChanStatus[TRY_CHANNELS];

static int audio_rate = 44100;
static uint16_t audio_format = MIX_DEFAULT_FORMAT; /* 16-bit stereo */
static int audio_channels = MIX_DEFAULT_CHANNELS;
static int audio_buffers = 1024;

static const int SoundVol[101] = {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2,
                                  3, 3, 3, 4, 4, 4, 5, 5, 6, 7, 7, 8, 9, 10, 11, 12, 14, 15, 17,
                                  18, 20, 23, 26, 29, 32, 36, 41, 46, 51, 57, 64, 72, 81, 91, 102, 114, 127};

int Sound_GetLogVol(uint8_t linear)
{
    if (linear < 101)
        return SoundVol[linear];
    else if (linear > 100)
        return SoundVol[100];
    else
        return 0;
}

static uint32_t GetChanTime(int i)
{
    if (i >= 0 && i < CHANNELS)
        if (ChanStatus[i])
            return SDL_GetTicks() - chantime[i];
    return 0;
}

void Sound_Init()
{
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
    {
        Z_PANIC("Unable to init SDL: %s\n", SDL_GetError());
    }

    Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers);
    memset(ChanStatus, 0, sizeof(ChanStatus));
    CHANNELS = Mix_AllocateChannels(TRY_CHANNELS);
}

void Sound_DeInit()
{
    Mix_CloseAudio();
}

int Sound_GetFreeChannel()
{
    for (int i = 0; i < CHANNELS; i++)
        if (ChanStatus[i] == false)
            return i;

    return -1;
}

void Sound_LockChannel(int i)
{
    if (i >= 0 && i < CHANNELS)
    {
        ChanStatus[i] = true;
        chantime[i] = SDL_GetTicks();
    }
}

void Sound_UnLockChannel(int i)
{
    if (i >= 0 && i < CHANNELS)
        ChanStatus[i] = false;
}

#define pi180 0.0174

int Sound_ProcessWav(action_res_t *nod)
{
    if (nod->node_type != NODE_TYPE_MUSIC)
        return NODE_RET_OK;

    musicnode_t *mnod = nod->nodes.node_music;

    if (!Mix_Playing(mnod->chn))
    {
        Sound_DeleteWav(nod);
        return NODE_RET_DELETE;
    }

    if (mnod->crossfade)
    {
        if (mnod->crossfade_params.times > 0)
        {
            if (GetBeat())
            {
                mnod->volume += mnod->crossfade_params.deltavolume;
                if (Mix_Playing(mnod->chn))
                    Mix_Volume(mnod->chn, Sound_GetLogVol(mnod->volume));
                mnod->crossfade_params.times--;
            }
        }
        else
            mnod->crossfade = false;
    }

    if (Rend_GetRenderer() != RENDER_PANA)
    {
        mnod->pantrack = false;
        Mix_SetPosition(mnod->chn, 0, mnod->attenuate);
    }

    if (mnod->pantrack)
    {
        int PlX = GetgVarInt(SLOT_VIEW_POS);
        float pixangle = (360.0 / Rend_GetPanaWidth());
        int soundpos = floor((PlX - mnod->pantrack_X) * pixangle);
        if (soundpos < 0)
            soundpos += 360;

        soundpos = 360 - soundpos;
        mnod->pantrack_angle = soundpos;

        int tempdist = mnod->attenuate;
        if (soundpos > 90 && soundpos < 270)
            tempdist += (-cos(soundpos * pi180) * 96.0);

        if (tempdist > 255)
            tempdist = 255;

        Mix_SetPosition(mnod->chn, mnod->pantrack_angle, tempdist);
    }

    if (mnod->sub != NULL)
        Subtitles_Process(mnod->sub, GetChanTime(mnod->chn) / 100);

    return NODE_RET_OK;
}

int Sound_DeleteWav(action_res_t *nod)
{
    if (nod->node_type != NODE_TYPE_MUSIC)
        return NODE_RET_NO;

    if (nod->nodes.node_music->chn >= 0)
    {
        if (Mix_Playing(nod->nodes.node_music->chn))
            Mix_HaltChannel(nod->nodes.node_music->chn);

        Mix_UnregisterAllEffects(nod->nodes.node_music->chn);
        Sound_UnLockChannel(nod->nodes.node_music->chn);
    }
    Mix_FreeChunk(nod->nodes.node_music->chunk);
    if (nod->slot != 0)
        SetgVarInt(nod->slot, 2);

    if (nod->nodes.node_music->sub != NULL)
        Subtitles_Delete(nod->nodes.node_music->sub);

    setGNode(nod->slot, NULL);

    free(nod->nodes.node_music);
    free(nod);

    return NODE_RET_DELETE;
}

action_res_t *Sound_CreateWavNode()
{
    action_res_t *tmp = ScrSys_CreateActRes(NODE_TYPE_MUSIC);

    tmp->nodes.node_music = NEW(musicnode_t);
    tmp->nodes.node_music->chn = -1;
    tmp->nodes.node_music->chunk = NULL;
    tmp->nodes.node_music->volume = 100;
    tmp->nodes.node_music->looped = false;
    tmp->nodes.node_music->crossfade = false;
    tmp->nodes.node_music->pantrack = false;
    tmp->nodes.node_music->crossfade_params.deltavolume = 0;
    tmp->nodes.node_music->crossfade_params.times = 0;
    tmp->nodes.node_music->pantrack_X = 0;
    tmp->nodes.node_music->pantrack_angle = 0;
    tmp->nodes.node_music->attenuate = 0;
    tmp->nodes.node_music->universe = false;
    tmp->nodes.node_music->sub = NULL;

    return tmp;
}

action_res_t *Sound_CreateSyncNode()
{
    action_res_t *tmp = ScrSys_CreateActRes(NODE_TYPE_SYNCSND);

    tmp->nodes.node_sync = NEW(syncnode_t);
    tmp->nodes.node_sync->chn = -1;
    tmp->nodes.node_sync->chunk = NULL;
    tmp->nodes.node_sync->syncto = 0;
    tmp->nodes.node_sync->sub = NULL;

    return tmp;
}

int Sound_DeleteSync(action_res_t *nod)
{
    if (nod->node_type != NODE_TYPE_SYNCSND)
        return NODE_RET_NO;

    if (nod->nodes.node_sync->chn >= 0)
    {
        if (Mix_Playing(nod->nodes.node_sync->chn))
            Mix_HaltChannel(nod->nodes.node_sync->chn);

        Mix_UnregisterAllEffects(nod->nodes.node_sync->chn);
        Sound_UnLockChannel(nod->nodes.node_sync->chn);
    }
    Mix_FreeChunk(nod->nodes.node_sync->chunk);

    if (nod->nodes.node_sync->sub != NULL)
        Subtitles_Delete(nod->nodes.node_sync->sub);

    free(nod->nodes.node_sync);
    free(nod);

    return NODE_RET_DELETE;
}

int Sound_ProcessSync(action_res_t *nod)
{
    if (nod->node_type != NODE_TYPE_SYNCSND)
        return NODE_RET_OK;

    syncnode_t *mnod = nod->nodes.node_sync;

    if (mnod->sub != NULL)
        Subtitles_Process(mnod->sub, GetChanTime(mnod->chn) / 100);

    if (!Mix_Playing(mnod->chn) || getGNode(mnod->syncto) == NULL)
    {
#ifdef TRACE
        if (!Mix_Playing(mnod->chn))
            printf("Not Played chan %d syncto %d \n", mnod->chn, mnod->syncto);
        else
            printf("NULL \n");
#endif
        Sound_DeleteSync(nod);
        return NODE_RET_DELETE;
    }

    return NODE_RET_OK;
}

//// Pantracking
action_res_t *Sound_CreatePanTrack()
{
    action_res_t *tmp;
    tmp = ScrSys_CreateActRes(NODE_TYPE_PANTRACK);

    tmp->nodes.node_pantracking = 0;
    return tmp;
}

int Sound_ProcessPanTrack(action_res_t *nod)
{
    if (nod->node_type != NODE_TYPE_PANTRACK)
        return NODE_RET_OK;

    action_res_t *tr_nod = getGNode(nod->nodes.node_pantracking);
    if (tr_nod == NULL)
    {
        Sound_DeletePanTrack(nod);
        return NODE_RET_DELETE;
    }

    return NODE_RET_OK;
}

int Sound_DeletePanTrack(action_res_t *nod)
{
    if (nod->node_type != NODE_TYPE_PANTRACK)
        return NODE_RET_NO;

    action_res_t *tr_nod = getGNode(nod->nodes.node_pantracking);
    if (tr_nod != NULL)
        if (tr_nod->node_type == NODE_TYPE_MUSIC)
        {
            tr_nod->nodes.node_music->pantrack = false;
            Mix_SetPosition(tr_nod->nodes.node_music->chn, 0, tr_nod->nodes.node_music->attenuate);
            tr_nod->need_delete = true;
        }

    if (nod->slot > 0)
        setGNode(nod->slot, NULL);

    free(nod);

    return NODE_RET_DELETE;
}
