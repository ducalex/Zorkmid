#include "System.h"

#define TRY_CHANNELS 16

static int CHANNELS = 0;

static uint8_t chanvol[TRY_CHANNELS];
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

int GetLogVol(uint8_t linear)
{
    if (linear < 101)
        return SoundVol[linear];
    else if (linear > 100)
        return SoundVol[100];
    else
        return 0;
}

void InitMusic()
{
    Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers);
    memset(ChanStatus, 0, sizeof(ChanStatus));
    CHANNELS = Mix_AllocateChannels(TRY_CHANNELS);
}

void DeinitMusic()
{
    Mix_CloseAudio();
}

int GetFreeChannel()
{
    for (int i = 0; i < CHANNELS; i++)
        if (ChanStatus[i] == false)
            return i;

    return -1;
}

void LockChan(int i)
{
    if (i >= 0 && i < CHANNELS)
    {
        ChanStatus[i] = true;
        chantime[i] = SDL_GetTicks();
    }
}

void UnlockChan(int i)
{
    if (i >= 0 && i < CHANNELS)
        ChanStatus[i] = false;
}

void SaveVol()
{
    for (int i = 0; i < CHANNELS; i++)
        chanvol[i] = Mix_Volume(i, -1);
}

void SilenceVol()
{
    for (int i = 0; i < CHANNELS; i++)
        Mix_Volume(i, 0);
}

void RestoreVol()
{
    for (int i = 0; i < CHANNELS; i++)
        Mix_Volume(i, chanvol[i]);
}

uint32_t GetChanTime(int i)
{
    if (i >= 0 && i < CHANNELS)
        if (ChanStatus[i])
            return SDL_GetTicks() - chantime[i];
    return 0;
}

#define pi180 0.0174

void snd_DeleteNoUniverse(pzllst *owner)
{
    MList *allres = GetAction_res_List();
    StartMList(allres);
    while (!eofMList(allres))
    {
        struct_action_res *nod = (struct_action_res *)DataMList(allres);
        if (nod->node_type == NODE_TYPE_MUSIC)
            if (nod->owner == owner && nod->nodes.node_music->universe == false)
            {
                snd_DeleteWav(nod);
                DeleteCurrent(allres);
            }
        NextMList(allres);
    }
}

void snd_DeleteLoopedWavsByOwner(pzllst *owner)
{
    MList *allres = GetAction_res_List();
    StartMList(allres);
    while (!eofMList(allres))
    {
        struct_action_res *nod = (struct_action_res *)DataMList(allres);
        if (nod->node_type == NODE_TYPE_MUSIC)
            if (nod->owner == owner && nod->nodes.node_music->looped)
            {
                snd_DeleteWav(nod);
                DeleteCurrent(allres);
            }
        NextMList(allres);
    }
}

int snd_ProcessWav(struct_action_res *nod)
{
    if (nod->node_type != NODE_TYPE_MUSIC)
        return NODE_RET_OK;

    musicnode *mnod = nod->nodes.node_music;

    if (!Mix_Playing(mnod->chn))
    {
        snd_DeleteWav(nod);
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
                    Mix_Volume(mnod->chn, GetLogVol(mnod->volume));
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
        sub_ProcessSub(mnod->sub, GetChanTime(mnod->chn) / 100);

    return NODE_RET_OK;
}

int snd_DeleteWav(struct_action_res *nod)
{
    if (nod->node_type != NODE_TYPE_MUSIC)
        return NODE_RET_NO;

    if (nod->nodes.node_music->chn >= 0)
    {
        if (Mix_Playing(nod->nodes.node_music->chn))
            Mix_HaltChannel(nod->nodes.node_music->chn);

        Mix_UnregisterAllEffects(nod->nodes.node_music->chn);
        UnlockChan(nod->nodes.node_music->chn);
    }
    Mix_FreeChunk(nod->nodes.node_music->chunk);
    if (nod->slot != 0)
        SetgVarInt(nod->slot, 2);

    if (nod->nodes.node_music->sub != NULL)
        sub_DeleteSub(nod->nodes.node_music->sub);

    setGNode(nod->slot, NULL);

    delete nod->nodes.node_music;
    free(nod);

    return NODE_RET_DELETE;
}

struct_action_res *snd_CreateWavNode()
{
    struct_action_res *tmp;
    tmp = ScrSys_CreateActRes(NODE_TYPE_MUSIC);

    tmp->nodes.node_music = NEW(musicnode);
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

/// SoundSync

struct_action_res *snd_CreateSyncNode()
{
    struct_action_res *tmp;
    tmp = ScrSys_CreateActRes(NODE_TYPE_SYNCSND);

    tmp->nodes.node_sync = NEW(struct_syncnode);
    tmp->nodes.node_sync->chn = -1;
    tmp->nodes.node_sync->chunk = NULL;
    tmp->nodes.node_sync->syncto = 0;
    tmp->nodes.node_sync->sub = NULL;

    return tmp;
}

int snd_DeleteSync(struct_action_res *nod)
{
    if (nod->node_type != NODE_TYPE_SYNCSND)
        return NODE_RET_NO;

    if (nod->nodes.node_sync->chn >= 0)
    {
        if (Mix_Playing(nod->nodes.node_sync->chn))
            Mix_HaltChannel(nod->nodes.node_sync->chn);

        Mix_UnregisterAllEffects(nod->nodes.node_sync->chn);
        UnlockChan(nod->nodes.node_sync->chn);
    }
    Mix_FreeChunk(nod->nodes.node_sync->chunk);

    if (nod->nodes.node_sync->sub != NULL)
        sub_DeleteSub(nod->nodes.node_sync->sub);

    delete nod->nodes.node_sync;
    free(nod);

    return NODE_RET_DELETE;
}

int snd_ProcessSync(struct_action_res *nod)
{
    if (nod->node_type != NODE_TYPE_SYNCSND)
        return NODE_RET_OK;

    struct_syncnode *mnod = nod->nodes.node_sync;

    if (mnod->sub != NULL)
        sub_ProcessSub(mnod->sub, GetChanTime(mnod->chn) / 100);

    if (!Mix_Playing(mnod->chn) || getGNode(mnod->syncto) == NULL)
    {
#ifdef TRACE
        if (!Mix_Playing(mnod->chn))
            printf("Not Played chan %d syncto %d \n", mnod->chn, mnod->syncto);
        else
            printf("NULL \n");
#endif
        snd_DeleteSync(nod);
        return NODE_RET_DELETE;
    }

    return NODE_RET_OK;
}

//// Pantracking
struct_action_res *snd_CreatePanTrack()
{
    struct_action_res *tmp;
    tmp = ScrSys_CreateActRes(NODE_TYPE_PANTRACK);

    tmp->nodes.node_pantracking = 0;
    return tmp;
}

int snd_ProcessPanTrack(struct_action_res *nod)
{
    if (nod->node_type != NODE_TYPE_PANTRACK)
        return NODE_RET_OK;

    struct_action_res *tr_nod = getGNode(nod->nodes.node_pantracking);
    if (tr_nod == NULL)
    {
        snd_DeletePanTrack(nod);
        return NODE_RET_DELETE;
    }

    return NODE_RET_OK;
}

int snd_DeletePanTrack(struct_action_res *nod)
{
    if (nod->node_type != NODE_TYPE_PANTRACK)
        return NODE_RET_NO;

    struct_action_res *tr_nod = getGNode(nod->nodes.node_pantracking);
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
