#include "System.h"

static uint32_t channel_start_time[SOUND_CHANNELS];
static int num_channels = 0;

static const uint8_t log_volume[101] = {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2,
                                  3, 3, 3, 4, 4, 4, 5, 5, 6, 7, 7, 8, 9, 10, 11, 12, 14, 15, 17,
                                  18, 20, 23, 26, 29, 32, 36, 41, 46, 51, 57, 64, 72, 81, 91, 102, 114, 127};

static uint32_t GetChanElapsedTime(int i)
{
    if (i >= 0 && i < num_channels && channel_start_time[i] > 0)
        return Game_GetTime() - channel_start_time[i];
    return 0;
}

void Sound_Init()
{
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
    {
        Z_PANIC("Unable to init SDL: %s\n", SDL_GetError());
    }

    Mix_OpenAudio(SOUND_FREQUENCY, AUDIO_S16LSB, MIX_DEFAULT_CHANNELS, 1024);
    memset(channel_start_time, 0, sizeof(channel_start_time));
    num_channels = Mix_AllocateChannels(SOUND_CHANNELS);
}

void Sound_DeInit()
{
    Mix_CloseAudio();
}

int Sound_Play(int channel, Mix_Chunk *chunk, int loops, int volume)
{
    if (channel < 0) // Find free channel
    {
        for (int i = 0; i < num_channels; i++)
            if (channel_start_time[i] == 0)
                channel = i;
    }

    if (channel < 0 || channel >= num_channels)
    {
        LOG_WARN("Unable to play sound, invalid channel: %d!\n", channel);
        return -1;
    }

    if (chunk == NULL)
    {
        LOG_WARN("Unable to play sound, chunk is NULL!\n");
        return -2;
    }

    Mix_UnregisterAllEffects(channel);
    if (volume >= 0)
        Sound_SetVolume(channel, volume);
    Mix_PlayChannel(channel, chunk, loops);

    channel_start_time[channel] = Game_GetTime();

    return channel;
}

int Sound_Stop(int channel)
{
    if (channel < 0 || channel >= num_channels)
    {
        LOG_WARN("Unable to stop sound, invalid channel: %d!\n", channel);
        return -1;
    }

    if (channel_start_time[channel] == 0)
    {
        LOG_WARN("Unable to stop sound, channel not playing!\n");
        return -2;
    }

    if (Mix_Playing(channel))
        Mix_HaltChannel(channel);

    Mix_UnregisterAllEffects(channel);

    channel_start_time[channel] = 0;
    return 0;
}

int Sound_Playing(int channel)
{
    return Mix_Playing(channel);
}

int Sound_Pause(int channel)
{
    Mix_Pause(channel);
    return 0;
}

int Sound_SetVolume(int channel, int volume)
{
    return Mix_Volume(channel, log_volume[(volume > 100) ? 100 : volume]);
}

int Sound_SetPosition(int channel, int angle, int distance)
{
    return Mix_SetPosition(channel, angle, distance);
}

action_res_t *Sound_CreateNode(int type)
{
    action_res_t *tmp = NEW(action_res_t);

    tmp->node_type = type;

    switch (type)
    {
    case NODE_TYPE_MUSIC:
        tmp->nodes.node_music = NEW(musicnode_t);
        tmp->nodes.node_music->chn = -1;
        tmp->nodes.node_music->volume = 100;
        break;

    case NODE_TYPE_SYNCSND:
        tmp->nodes.node_sync = NEW(syncnode_t);
        tmp->nodes.node_sync->chn = -1;
        break;

    case NODE_TYPE_PANTRACK:
        break;

    default:
        Z_PANIC("Invalid sound node type %d\n", type);
    }

    return tmp;
}

int Sound_DeleteNode(action_res_t *nod)
{
    action_res_t *tr_nod;

    switch (nod->node_type)
    {
    case NODE_TYPE_MUSIC:
        if (nod->nodes.node_music->chn >= 0)
        {
            Sound_Stop(nod->nodes.node_music->chn);
        }

        Mix_FreeChunk(nod->nodes.node_music->chunk);

        if (nod->slot != 0)
            SetgVarInt(nod->slot, 2);

        if (nod->nodes.node_music->sub != NULL)
            Text_DeleteSubtitles(nod->nodes.node_music->sub);

        SetGNode(nod->slot, NULL);

        DELETE(nod->nodes.node_music);
        DELETE(nod);

        return NODE_RET_DELETE;

    case NODE_TYPE_SYNCSND:
        if (nod->nodes.node_sync->chn >= 0)
        {
            Sound_Stop(nod->nodes.node_sync->chn);
        }
        Mix_FreeChunk(nod->nodes.node_sync->chunk);

        if (nod->nodes.node_sync->sub != NULL)
            Text_DeleteSubtitles(nod->nodes.node_sync->sub);

        DELETE(nod->nodes.node_sync);
        DELETE(nod);

        return NODE_RET_DELETE;

    case NODE_TYPE_PANTRACK:
        tr_nod = GetGNode(nod->nodes.node_pantracking);
        if (tr_nod && tr_nod->node_type == NODE_TYPE_MUSIC)
        {
            tr_nod->nodes.node_music->pantrack = false;
            Mix_SetPosition(tr_nod->nodes.node_music->chn, 0, tr_nod->nodes.node_music->attenuate);
            tr_nod->need_delete = true;
        }

        if (nod->slot > 0)
            SetGNode(nod->slot, NULL);

        DELETE(nod);

        return NODE_RET_DELETE;

    default:
        return NODE_RET_NO;
    }
}

int Sound_ProcessNode(action_res_t *nod)
{
    if (nod->node_type == NODE_TYPE_MUSIC)
    {
        musicnode_t *mnod = nod->nodes.node_music;

        if (!Sound_Playing(mnod->chn))
        {
            Sound_DeleteNode(nod);
            return NODE_RET_DELETE;
        }

        #define pi180 0.0174

        if (mnod->crossfade)
        {
            mnod->crossfade = (mnod->crossfade_params.times > 0);
            if (mnod->crossfade && Game_GetBeat())
            {
                mnod->volume += mnod->crossfade_params.deltavolume;
                if (Sound_Playing(mnod->chn))
                    Sound_SetVolume(mnod->chn, mnod->volume);
                mnod->crossfade_params.times--;
            }
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
            Text_ProcessSubtitles(mnod->sub, GetChanElapsedTime(mnod->chn) / 100);

        return NODE_RET_OK;
    }

    else if (nod->node_type == NODE_TYPE_SYNCSND)
    {
        syncnode_t *mnod = nod->nodes.node_sync;

        if (mnod->sub != NULL)
            Text_ProcessSubtitles(mnod->sub, GetChanElapsedTime(mnod->chn) / 100);

        if (!Sound_Playing(mnod->chn) || GetGNode(mnod->syncto) == NULL)
        {
            Sound_DeleteNode(nod);
            return NODE_RET_DELETE;
        }

        return NODE_RET_OK;
    }

    else if (nod->node_type == NODE_TYPE_PANTRACK)
    {
        action_res_t *tr_nod = GetGNode(nod->nodes.node_pantracking);
        if (tr_nod == NULL)
        {
            Sound_DeleteNode(nod);
            return NODE_RET_DELETE;
        }

        return NODE_RET_OK;
    }

    return NODE_RET_NO;
}
