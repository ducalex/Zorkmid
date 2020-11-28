#include "System.h"

static const char *get_addition(const char *str)
{
    const char *s = str;
    int32_t len = strlen(str);
    int32_t pos = 0;
    int32_t num = 0;
    bool whitespace = true;

    while (pos < len)
    {
        if (whitespace)
        {
            if (str[pos] != ' ')
            {
                whitespace = false;
                num++;
            }
        }
        else
        {
            if (str[pos] == ' ')
            {
                whitespace = true;
                if (num >= 9)
                {
                    s = str + pos;
                    break;
                }
            }
        }
        pos++;
    }
    return s;
}

static int GetIntVal(char *chr)
{
    if (chr[0] == '[')
        return GetgVarInt(atoi(chr + 1));
    else
        return atoi(chr);
}

static void useart_recovery(char *str)
{
    int32_t len = strlen(str);
    int32_t pos = 0;
    bool bracket = false;

    while (pos < len)
    {
        if (str[pos] == '[')
            bracket = true;
        else if (str[pos] == ']')
            bracket = false;
        else if (str[pos] == ' ' && bracket)
            str[pos] = ',';
        pos++;
    }
}

static int action_set_screen(char *params, int aSlot, pzllst_t *owner)
{
    if (Rend_LoadGamescr(params) == 0)
        LOG_WARN("Can't find %s, screen load failed\n", params);

    return ACTION_NORMAL;
}

static int action_set_partial_screen(char *params, int aSlot, pzllst_t *owner)
{
    char xx[16], yy[16], tmp11[16], tmp22[16];
    char file[255];
    sscanf(params, "%s %s %s %s %s", xx, yy, file, tmp11, tmp22);

    int x = GetIntVal(xx);
    int y = GetIntVal(yy);
    int tmp2 = GetIntVal(tmp22);

    SDL_Surface *tmp = Loader_LoadGFX(file, Rend_GetRenderer() == RENDER_PANA, tmp2);
    if (tmp)
    {
        Rend_BlitSurfaceXY(tmp, Rend_GetLocationScreenImage(), x, y);
        SDL_FreeSurface(tmp);
    }
    else
    {
        LOG_WARN("Failed to load image '%s'\n", file);
    }
    return ACTION_NORMAL;
}

static int action_assign(char *params, int aSlot, pzllst_t *owner)
{
    char tmp1[16], tmp2[16];
    sscanf(params, "%s %s", tmp1, tmp2);

    SetgVarInt(GetIntVal(tmp1), GetIntVal(tmp2));
    return ACTION_NORMAL;
}

static int action_timer(char *params, int aSlot, pzllst_t *owner)
{
    int delay = CUR_GAME == GAME_ZGI ? 100 : 1000;

    char tmp2[16];
    sscanf(params, "%s", tmp2);

    if (GetGNode(aSlot) != NULL)
    {
        return ACTION_NORMAL;
    }

    action_res_t *nod = Timer_CreateNode();
    nod->slot = aSlot;
    nod->owner = owner;
    nod->nodes.node_timer = GetIntVal(PrepareString(tmp2)) * delay;

    SetGNode(aSlot, nod);

    ScrSys_AddToActionsList(nod);

    SetgVarInt(aSlot, 1);

    return ACTION_NORMAL;
}

static int action_change_location(char *params, int aSlot, pzllst_t *owner)
{
    char tmp[4], tmp2[4], tmp3[4], tmp4[16];
    sscanf(params, "%c %c %c%c %s", tmp, tmp2, tmp3, tmp3 + 1, tmp4);

    Game_Relocate(tolower(tmp[0]), tolower(tmp2[0]), tolower(tmp3[0]), tolower(tmp3[1]), GetIntVal(tmp4));

    Rend_SetDelay(2);

    return ACTION_NORMAL;
}

static int action_dissolve(char *params, int aSlot, pzllst_t *owner)
{
    Rend_FillRect(Rend_GetGameScreen(), NULL, 0, 0, 0);

    return ACTION_NORMAL;
}

static int action_disable_control(char *params, int aSlot, pzllst_t *owner)
{
    ScrSys_SetFlag(GetIntVal(params), FLAG_DISABLED);

    return ACTION_NORMAL;
}

static int action_enable_control(char *params, int aSlot, pzllst_t *owner)
{
    ScrSys_SetFlag(GetIntVal(params), 0);

    return ACTION_NORMAL;
}

static int action_add(char *params, int aSlot, pzllst_t *owner)
{
    char number[16];
    int slot;
    //    int tmp;
    sscanf(params, "%d %s", &slot, number);

    //tmp = GetIntVal(slot);
    SetgVarInt(slot, GetgVarInt(slot) + GetIntVal(number));

    return ACTION_NORMAL;
}

static int action_debug(char *params, int aSlot, pzllst_t *owner)
{
    char txt[256], number[16];

    sscanf(params, "%s %s", txt, number);
    LOG_WARN("DEBUG :%s\t: %d \n", txt, GetIntVal(number));

    return ACTION_NORMAL;
}

static int action_random(char *params, int aSlot, pzllst_t *owner)
{
    int number;
    char chars[16];
    sscanf(params, "%s", chars);
    number = GetIntVal(chars);

    SetgVarInt(aSlot, rand() % (number + 1));

    return ACTION_NORMAL;
}

static int action_streamvideo(char *params, int aSlot, pzllst_t *owner)
{
    char file[255];
    char x[16];
    char y[16];
    char w[16];
    char h[16];
    char u1[16]; //bit-flags
    // 0x1 - in original aspect ratio and size, without scale
    // 0x2 - unknown
    char u2[16]; // skipline 0-off any other - on

    sscanf(params, "%s %s %s %s %s %s %s", file, x, y, w, h, u1, u2);

    int xx = GetIntVal(x);
    int yy = GetIntVal(y);
    int ww = GetIntVal(w) - xx + 1;
    int hh = GetIntVal(h) - yy + 1;

    anim_avi_t *anm = NEW(anim_avi_t);
    subtitles_t *subs = NULL;

    const char *fil = Loader_GetPath(file);
    if (fil == NULL)
        return ACTION_NORMAL;

    anm->av = avi_openfile(fil, 0);
    anm->img = Rend_CreateSurface(anm->av->w, anm->av->h, 0);

    if (GetgVarInt(SLOT_SUBTITLE_FLAG) == 1)
    {
        strcpy(file + (strlen(file) - 3), "sub");
        subs = Text_LoadSubtitles(file);
    }

    Mix_Chunk *aud = avi_get_audio(anm->av);
    int tmp = Sound_Play(-1, aud, 0, 100);

    SDL_Rect dst_rct = {GAMESCREEN_X + xx + GAMESCREEN_FLAT_X, GAMESCREEN_Y + yy, ww, hh};
    int frame_delay = anm->av->header.mcrSecPframe / 2000;

    if (frame_delay > 200 || frame_delay < 0)
        frame_delay = 15;

    avi_play(anm->av);

    while (anm->av->status == AVI_PLAY)
    {
        GameUpdate();

        if (KeyHit(SDLK_SPACE))
            avi_stop(anm->av);

        avi_blit(anm->av, anm->img);

        Rend_BlitSurface(anm->img, NULL, Rend_GetScreen(), &dst_rct);

        avi_update(anm->av);

        if (subs)
            Text_ProcessSubtitles(subs, anm->av->cframe);

        Rend_ProcessSubs();
        Rend_ScreenFlip();

        Game_Delay(frame_delay);
    }

    if (tmp >= 0) Sound_Stop(tmp);
    if (aud) Mix_FreeChunk(aud);
    if (subs) Text_DeleteSubtitles(subs);

    avi_close(anm->av);
    free(anm);

    return ACTION_NORMAL;
}

static int action_animplay(char *params, int aSlot, pzllst_t *owner)
{
    char file[255];
    char x[16];
    char y[16];
    char w[16];
    char h[16];
    char st[16];
    char en[16];
    char un1[16];
    char loop[16];
    char un2[16];
    char mask[16];
    char framerate[16]; //framerate or 0 (default)
    sscanf(params, "%s %s %s %s %s %s %s %s %s %s %s %s", file, x, y, w, h, st, en, loop, un1, un2, mask, framerate);

    MList *all = GetActionsList();
    StartMList(all);
    while (!EndOfMList(all))
    {
        action_res_t *nd = (action_res_t *)DataMList(all);

        if (nd->node_type == NODE_TYPE_ANIMPLAY)
            if (nd->slot == aSlot)
            {
                Anim_DeleteNode(nd);
                DeleteCurrent(all);
            }

        NextMList(all);
    }

    action_res_t *glob = Anim_CreateNode(NODE_TYPE_ANIMPLAY);
    animnode_t *nod = glob->nodes.node_anim;

    ScrSys_AddToActionsList(glob);

    if (aSlot > 0)
    {
        SetgVarInt(aSlot, 1);
        SetGNode(aSlot, glob);
    }

    glob->slot = aSlot;
    glob->owner = owner;

    Anim_Load(nod, file, 0, 0, GetIntVal(mask), GetIntVal(framerate));

    Anim_Play(nod, GetIntVal(x),
                  GetIntVal(y),
                  GetIntVal(w) - GetIntVal(x) + 1,
                  GetIntVal(h) - GetIntVal(y) + 1,
                  GetIntVal(st),
                  GetIntVal(en),
                  GetIntVal(loop));

    return ACTION_NORMAL;
}

static int music_music(char *params, int aSlot, pzllst_t *owner, bool universe)
{
    int type;
    char file[32];
    char loop[16];
    char vol[16];
    int read_params = sscanf(params, "%d %s %s %s", &type, file, loop, vol);

    Mix_Chunk *chunk = NULL;

    char *ext = file + (strlen(file) - 3);

    if (GetGNode(aSlot) != NULL)
        return ACTION_NORMAL;

    if (type == 4) // MIDI note
    {
        int instrument = atoi(file);
        int pitch = atoi(loop);
        char fn[PATHBUFSIZ];
        sprintf(fn, "%s/MIDI/%d/%d.wav", Game_GetPath(), instrument, pitch);
        chunk = Loader_LoadSound(fn);
        sprintf(loop, "%d", (instrument != 0));
    }
    else
    {
        if ((chunk = Loader_LoadSound(file)) == NULL)
        {
            strcpy(ext, "raw");
            if ((chunk = Loader_LoadSound(file)) == NULL)
            {
                strcpy(ext, "ifp");
                if ((chunk = Loader_LoadSound(file)) == NULL)
                {
                    strcpy(ext, "src");
                    chunk = Loader_LoadSound(file);
                }
            }
        }
    }

    int volume = (read_params == 4) ? GetIntVal(vol) : 100;
    int looped = GetIntVal(loop) == 1;
    int playing = Sound_Play(-1, chunk, looped ? -1 : 0, volume);

    if (playing < 0)
    {
        return ACTION_NORMAL;
    }

    action_res_t *nod = Sound_CreateNode(NODE_TYPE_MUSIC);

    nod->slot = aSlot;
    nod->owner = owner;

    SetGNode(nod->slot, nod);

    if (GetgVarInt(SLOT_SUBTITLE_FLAG) == 1)
    {
        strcpy(ext, "sub");
        nod->nodes.node_music->sub = Text_LoadSubtitles(file);
    }

    nod->nodes.node_music->universe = universe;
    nod->nodes.node_music->volume = volume;
    nod->nodes.node_music->looped = looped;
    nod->nodes.node_music->chunk = chunk;
    nod->nodes.node_music->chn = playing;

    ScrSys_AddToActionsList(nod);

    SetgVarInt(aSlot, 1);

    return ACTION_NORMAL;
}

static int action_music(char *params, int aSlot, pzllst_t *owner)
{
    return music_music(params, aSlot, owner, false);
}

static int action_universe_music(char *params, int aSlot, pzllst_t *owner)
{
    return music_music(params, aSlot, owner, true);
}

static int action_syncsound(char *params, int aSlot, pzllst_t *owner)
{
    //slot maybe 0
    //params:
    //1 - sync with
    //2 - type (0 - manual params, 1 from file)
    //3 - filename
    //4 - Freq
    //5 - bits
    //6 - 0 mono | 1 stereo
    //7 - unknown

    char a1[16];
    char a2[16];
    char a3[16];

    //sscanf(params,"%s %s %s %s %s %s %s",)
    sscanf(params, "%s %s %s", a1, a2, a3);

    int syncto = GetIntVal(a1);

    if (GetGNode(syncto) == NULL)
        return ACTION_NORMAL;

    Mix_Chunk *chunk = Loader_LoadSound(a3);
    int playing = Sound_Play(-1, chunk, 0, 100);

    if (playing < 0)
    {
        return ACTION_NORMAL;
    }

    action_res_t *tmp = Sound_CreateNode(NODE_TYPE_SYNCSND);

    tmp->owner = owner;
    tmp->slot = -1;
    //tmp->slot  = aSlot;
    tmp->nodes.node_sync->syncto = syncto;
    tmp->nodes.node_sync->chunk = chunk;
    tmp->nodes.node_sync->chn = playing;

    if (GetGNode(syncto)->node_type == NODE_TYPE_ANIMPRE)
        GetGNode(syncto)->nodes.node_animpre->framerate = FPS_DELAY; //~15fps hack

    char *ext = a3 + (strlen(a3) - 3);
    strcpy(ext, "sub");

    if (GetgVarInt(SLOT_SUBTITLE_FLAG) == 1)
        tmp->nodes.node_sync->sub = Text_LoadSubtitles(a3);

    ScrSys_AddToActionsList(tmp);

    return ACTION_NORMAL;
}

static int action_animpreload(char *params, int aSlot, pzllst_t *owner)
{
    if (GetGNode(aSlot) != NULL)
        return ACTION_NORMAL;

    char name[64];
    char u1[16];
    char u2[16];
    char u3[16];
    char u4[16];

    action_res_t *pre = Anim_CreateNode(NODE_TYPE_ANIMPRE);

    //%s %d %d %d %f
    //name     ? ? mask framerate
    //in zgi   0 0 0
    sscanf(params, "%s %s %s %s %s", name, u1, u2, u3, u4);

    Anim_Load(pre->nodes.node_animpre, name, 0, 0, GetIntVal(u3), GetIntVal(u4));

    pre->slot = aSlot;
    pre->owner = owner;

    ScrSys_AddToActionsList(pre);

    SetgVarInt(pre->slot, 2);

    SetGNode(aSlot, pre);

    return ACTION_NORMAL;
}

static int action_playpreload(char *params, int aSlot, pzllst_t *owner)
{
    char sl[16];
    uint32_t slot;
    int x, y, w, h, start, end, loop;
    sscanf(params, "%s %d %d %d %d %d %d %d", sl, &x, &y, &w, &h, &start, &end, &loop);

    slot = GetIntVal(sl);

    action_res_t *pre = GetGNode(slot);

    if (pre == NULL || pre->node_type != NODE_TYPE_ANIMPRE)
    {
        return ACTION_NORMAL;
    }

    action_res_t *nod = Anim_CreateNode(NODE_TYPE_ANIMPRPL);

    anim_preplay_node_t *tmp = nod->nodes.node_animpreplay;

    tmp->playerid = 0;
    tmp->point = pre->nodes.node_animpre;
    tmp->x = x;
    tmp->y = y;
    tmp->w = w - x + 1;
    tmp->h = h - y + 1;
    tmp->start = start;
    tmp->end = end;
    tmp->loop = loop;
    tmp->pointingslot = slot;

    nod->slot = aSlot;
    nod->owner = owner;

    ScrSys_AddToActionsList(nod);

    if (aSlot > 0)
    {
        SetGNode(aSlot, nod);
    }

    //SetgVarInt(GetIntVal(chars),2);

    return ACTION_NORMAL;
}

static int action_ttytext(char *params, int aSlot, pzllst_t *owner)
{
    char chars[16];
    int32_t delay;
    int32_t x, y, w, h;
    sscanf(params, "%d %d %d %d %s %d", &x, &y, &w, &h, chars, &delay);

    w -= x;
    h -= y;

    FManNode_t *fil = Loader_FindNode(chars);

    if (fil == NULL)
    {
        SetgVarInt(aSlot, 2);
        return ACTION_NORMAL;
    }

    action_res_t *nod = Text_CreateTTYText();

    nod->slot = aSlot;
    nod->owner = owner;

    mfile_t *fl = mfopen(fil);
    m_wide_to_utf8(fl);

    nod->nodes.tty_text->txtbuf = NEW_ARRAY(char, fl->size);

    size_t j = 0;
    for (size_t i = 0; i < fl->size; i++)
        if (fl->buf[i] != 0x0A && fl->buf[i] != 0x0D)
            nod->nodes.tty_text->txtbuf[j++] = (char)fl->buf[i];

    nod->nodes.tty_text->txtsize = j;

    mfclose(fl);

    Text_GetStyle(&nod->nodes.tty_text->style, nod->nodes.tty_text->txtbuf);
    nod->nodes.tty_text->fnt = Loader_LoadFont(nod->nodes.tty_text->style.fontname, nod->nodes.tty_text->style.size);
    nod->nodes.tty_text->img = Rend_CreateSurface(w, h, 0);
    nod->nodes.tty_text->w = w;
    nod->nodes.tty_text->h = h;
    nod->nodes.tty_text->x = x;
    nod->nodes.tty_text->y = y;
    nod->nodes.tty_text->delay = delay;

    SetgVarInt(nod->slot, 1);
    SetGNode(nod->slot, nod);

    ScrSys_AddToActionsList(nod);

    return ACTION_NORMAL;
}

static int stopkiller(char *params, int aSlot, pzllst_t *owner, bool iskillfunc)
{
    int slot;
    char chars[16];
    sscanf(params, "%s", chars);
    if (iskillfunc)
    {
        if (str_equals(chars, "\"all\""))
        {
            ScrSys_FlushActionsList();
            return ACTION_NORMAL;
        }

        if (str_equals(chars, "\"anim\""))
        {
            ScrSys_FlushResourcesByType(NODE_TYPE_ANIMPLAY);
            ScrSys_FlushResourcesByType(NODE_TYPE_ANIMPRPL);
            ScrSys_FlushResourcesByType(NODE_TYPE_ANIMPRE);
            return ACTION_NORMAL;
        }

        if (str_equals(chars, "\"audio\""))
        {
            ScrSys_FlushResourcesByType(NODE_TYPE_MUSIC);
            //ScrSys_FlushResourcesByType(NODE_TYPE_SYNCSND);
            return ACTION_NORMAL;
        }

        if (str_equals(chars, "\"distort\""))
        {
            ScrSys_FlushResourcesByType(NODE_TYPE_DISTORT);
            return ACTION_NORMAL;
        }

        if (str_equals(chars, "\"pantrack\""))
        {
            ScrSys_FlushResourcesByType(NODE_TYPE_PANTRACK);
            return ACTION_NORMAL;
        }

        if (str_equals(chars, "\"region\""))
        {
            ScrSys_FlushResourcesByType(NODE_TYPE_REGION);
            return ACTION_NORMAL;
        }

        if (str_equals(chars, "\"timer\""))
        {
            ScrSys_FlushResourcesByType(NODE_TYPE_TIMER);
            return ACTION_NORMAL;
        }

        if (str_equals(chars, "\"ttytext\""))
        {
            ScrSys_FlushResourcesByType(NODE_TYPE_TTYTEXT);
            return ACTION_NORMAL;
        }
    }

    slot = GetIntVal(chars);

    //if (getGNode(slot) == NULL)
    //return ACTION_NOT_FOUND;

    MList *all = GetActionsList();

    StartMList(all);
    while (!EndOfMList(all))
    {
        action_res_t *nod = (action_res_t *)DataMList(all);

        if (nod->slot == slot)
        {
            if (nod->node_type == NODE_TYPE_ANIMPRE)
            {
                if (nod->nodes.node_animpre->playing)
                {
                    if (iskillfunc)
                        ScrSys_DeleteActionNode(nod);
                    else
                    {
                        nod->nodes.node_animpre->playing = false;
                        break;
                    }
                }
                else
                {
                    if (iskillfunc)
                        ScrSys_DeleteActionNode(nod);
                    else
                        break;
                }
            }
            else
                ScrSys_DeleteActionNode(nod);

            DeleteCurrent(all);
            break;
        }

        NextMList(all);
    }

    return ACTION_NORMAL;
}

static int action_kill(char *params, int aSlot, pzllst_t *owner)
{
    stopkiller(params, aSlot, owner, true);

    return ACTION_NORMAL;
}

static int action_stop(char *params, int aSlot, pzllst_t *owner)
{
    stopkiller(params, aSlot, owner, false);

    return ACTION_NORMAL;
}

static int action_inventory(char *params, int aSlot, pzllst_t *owner)
{
    int item;
    char cmd[16];
    char chars[16];
    memset(chars, 0, 16);
    sscanf(params, "%s %s", cmd, chars);
    item = GetIntVal(chars);

    if (str_equals(cmd, "add"))
    {
        Inventory_Add(item);
    }
    else if (str_equals(cmd, "addi"))
    {
        item = GetgVarInt(item);
        Inventory_Add(item);
    }
    else if (str_equals(cmd, "drop"))
    {
        if (item >= 0)
            Inventory_Drop(item);
        else
            Inventory_Drop(GetgVarInt(SLOT_INVENTORY_MOUSE));
    }
    else if (str_equals(cmd, "dropi"))
    {
        item = GetgVarInt(item);
        Inventory_Drop(item);
    }
    else if (str_equals(cmd, "cycle"))
    {
        Inventory_Cycle();
    }
    else
        return ACTION_ERROR;

    return ACTION_NORMAL;
}

static int action_crossfade(char *params, int aSlot, pzllst_t *owner)
{
    //crossfade(%d %d %d %d %d %d %ld)
    // item1 item2 fromvol1 fromvol2 tovol1 tovol2 time_in_millisecs
    uint32_t item, item2;
    char slot[16];
    char slot2[16]; //unknown slot
    char frVol1[16];
    char frVol2[16];
    char toVol[16];
    char toVol2[16];
    char tim[16];
    sscanf(params, "%s %s %s %s %s %s %s", slot, slot2, frVol1, frVol2, toVol, toVol2, tim);
    item = GetIntVal(slot);
    item2 = GetIntVal(slot2);

    if (item > 0)
    {
        action_res_t *tmp = GetGNode(item);
        if (tmp && tmp->node_type == NODE_TYPE_MUSIC)
        {
            action_res_t *tnod = tmp;

            tnod->nodes.node_music->crossfade = true;

            if (GetIntVal(frVol1) >= 0)
                tnod->nodes.node_music->volume = GetIntVal(frVol1);

            tnod->nodes.node_music->crossfade_params.times = ceil(GetIntVal(tim) / 33.3);
            tnod->nodes.node_music->crossfade_params.deltavolume = ceil((GetIntVal(toVol) - tnod->nodes.node_music->volume) / (float)tnod->nodes.node_music->crossfade_params.times);

            if (Sound_Playing(tnod->nodes.node_music->chn))
                Sound_SetVolume(tnod->nodes.node_music->chn, tnod->nodes.node_music->volume);
        }
    }

    if (item2 > 0)
    {
        action_res_t *tmp = GetGNode(item2);
        if (tmp && tmp->node_type == NODE_TYPE_MUSIC)
        {
            action_res_t *tnod = tmp;

            tnod->nodes.node_music->crossfade = true;

            if (GetIntVal(frVol2) >= 0)
                tnod->nodes.node_music->volume = GetIntVal(frVol2);

            tnod->nodes.node_music->crossfade_params.times = ceil(GetIntVal(tim) / 33.3);
            tnod->nodes.node_music->crossfade_params.deltavolume = ceil((GetIntVal(toVol2) - tnod->nodes.node_music->volume) / (float)tnod->nodes.node_music->crossfade_params.times);

            if (Sound_Playing(tnod->nodes.node_music->chn))
                Sound_SetVolume(tnod->nodes.node_music->chn, tnod->nodes.node_music->volume);
        }
    }

    return ACTION_NORMAL;
}

static int action_menu_bar_enable(char *params, int aSlot, pzllst_t *owner)
{
    Menu_SetVal(GetIntVal(params));

    return ACTION_NORMAL;
}

static int action_delay_render(char *params, int aSlot, pzllst_t *owner)
{
    Rend_SetDelay(GetIntVal(params));

    return ACTION_NORMAL;
}

static int action_pan_track(char *params, int aSlot, pzllst_t *owner)
{
    if (Rend_GetRenderer() != RENDER_PANA)
        return ACTION_NORMAL;

    int slot;
    int XX = 0;

    sscanf(params, "%d %d", &slot, &XX);

    if (slot > 0)
    {
        action_res_t *nod = Sound_CreateNode(NODE_TYPE_PANTRACK);
        nod->nodes.node_pantracking = slot;

        nod->owner = owner;
        nod->slot = aSlot;

        if (nod->slot > 0)
            SetGNode(nod->slot, nod);

        ScrSys_AddToActionsList(nod);

        action_res_t *tmp = GetGNode(slot);

        if (tmp != NULL)
            if (tmp->node_type == NODE_TYPE_MUSIC)
            {
                action_res_t *tnod = tmp;

                tnod->nodes.node_music->pantrack = true;
                tnod->nodes.node_music->pantrack_X = XX;
            }
    }

    return ACTION_NORMAL;
}

static int action_attenuate(char *params, int aSlot, pzllst_t *owner)
{
    int slot;
    int att;

    sscanf(params, "%d %d", &slot, &att);

    att = floor(32767.0 / ((float)abs(att)) * 255.0);

    if (slot > 0)
    {
        action_res_t *tmp = GetGNode(slot);
        if (tmp && tmp->node_type == NODE_TYPE_MUSIC)
        {
            tmp->nodes.node_music->attenuate = att;
            Sound_SetPosition(tmp->nodes.node_music->chn,
                              tmp->nodes.node_music->pantrack_angle,
                              tmp->nodes.node_music->attenuate);
        }
    }

    return ACTION_NORMAL;
}

static int action_cursor(char *params, int aSlot, pzllst_t *owner)
{
    if (tolower(params[0]) == 'u')
        Mouse_ShowCursor();
    else if (tolower(params[0]) == 'h')
        Mouse_HideCursor();
    else
        return ACTION_ERROR;

    return ACTION_NORMAL;
}

static int action_animunload(char *params, int aSlot, pzllst_t *owner)
{
    int slot;

    sscanf(params, "%d", &slot);

    action_res_t *nod = GetGNode(slot);

    if (nod != NULL)
        if (nod->node_type == NODE_TYPE_ANIMPRE)
            nod->need_delete = true;

    return ACTION_NORMAL;
}

static int action_flush_mouse_events(char *params, int aSlot, pzllst_t *owner)
{
    FlushMouseBtn(MOUSE_BTN_LEFT|MOUSE_BTN_RIGHT);

    return ACTION_NORMAL;
}

static int action_save_game(char *params, int aSlot, pzllst_t *owner)
{
    ScrSys_PrepareSaveBuffer();
    ScrSys_SaveGame(params);

    return ACTION_NORMAL;
}

static int action_restore_game(char *params, int aSlot, pzllst_t *owner)
{
    ScrSys_LoadGame(params);

    return ACTION_NORMAL;
}

static int action_quit(char *params, int aSlot, pzllst_t *owner)
{
    if (atoi(params) == 1)
        GameQuit();
    else
        game_try_quit();

    return ACTION_NORMAL;
}

static int action_rotate_to(char *params, int aSlot, pzllst_t *owner)
{
    if (Rend_GetRenderer() != RENDER_PANA)
        return ACTION_NORMAL;

    int32_t topos;
    int32_t time;

    sscanf(params, "%d %d", &topos, &time);

    int32_t maxX = Rend_GetPanaWidth();
    int32_t curX = GetgVarInt(SLOT_VIEW_POS);
    int32_t oner = 0;

    if (curX == topos)
        return ACTION_NORMAL;

    if (curX > topos)
    {
        if (curX - topos > maxX / 2)
            oner = (topos + (maxX - curX)) / time;
        else
            oner = -(curX - topos) / time;
    }
    else
    {
        if (topos - curX > maxX / 2)
            oner = -((maxX - topos) + curX) / time;
        else
            oner = (topos - curX) / time;
    }

    for (int i = 0; i <= time; i++)
    {
        if (i == time)
            curX = topos;
        else
            curX += oner;

        if (curX < 0)
            curX = maxX - curX;
        else if (curX >= maxX)
            curX %= maxX;

        SetDirectgVarInt(SLOT_VIEW_POS, curX);

        Rend_RenderFunc();
        Rend_ScreenFlip();
        Game_Delay(500 / time);
    }

    return ACTION_NORMAL;
}

static int action_distort(char *params, int aSlot, pzllst_t *owner)
{
    if (Rend_GetRenderer() != RENDER_PANA && Rend_GetRenderer() != RENDER_TILT)
        return ACTION_NORMAL;

    int32_t slot, speed;
    float st_angl, en_angl;
    float st_lin, en_lin;
    sscanf(params, "%d %d %f %f %f %f", &slot, &speed, &st_angl, &en_angl, &st_lin, &en_lin);

    if (GetGNode(slot) != NULL)
        return ACTION_NORMAL;

    action_res_t *act = Rend_CreateNode(NODE_TYPE_DISTORT);
    act->slot = slot;
    act->owner = owner;

    if (slot > 0)
    {
        SetGNode(act->slot, act);
        SetgVarInt(act->slot, 1);
    }

    ScrSys_AddToActionsList(act);

    act->nodes.distort->speed = speed;
    act->nodes.distort->increase = true;
    act->nodes.distort->rend_angl = Rend_GetRendererAngle();
    act->nodes.distort->rend_lin = Rend_GetRendererLinscale();
    act->nodes.distort->st_angl = st_angl;
    act->nodes.distort->st_lin = st_lin;
    act->nodes.distort->end_angl = en_angl;
    act->nodes.distort->end_lin = en_lin;
    act->nodes.distort->dif_angl = en_angl - st_angl;
    act->nodes.distort->dif_lin = en_lin - st_lin;
    act->nodes.distort->param1 = (float)speed / 15.0;
    act->nodes.distort->frames = ceil((5.0 - act->nodes.distort->param1 * 2.0) / (act->nodes.distort->param1));
    if (act->nodes.distort->frames <= 0)
        act->nodes.distort->frames = 1;

    return ACTION_NORMAL;
}

static int action_preferences(char *params, int aSlot, pzllst_t *owner)
{
    if (str_starts_with(params, "save"))
        ScrSys_SavePreferences();
    else
        ScrSys_LoadPreferences();

    return ACTION_NORMAL;
}

//Graphic effects
static int action_region(char *params, int aSlot, pzllst_t *owner)
{
    char art[64];
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
    int32_t delay;
    // type:
    // 0 - water waves
    // 1 - brightness
    // 9 - compositing
    int32_t type;
    int32_t unk1; //0 or 1
    int32_t unk2;
    char addition[128];

    useart_recovery(params);

    if (sscanf(params, "%s %d %d %d %d %d %d %d %d %s", art, &x, &y, &w, &h, &delay, &type, &unk1, &unk2, addition) == 10)
    {
        strcpy(addition, get_addition(params));
        w -= (x - 1);
        h -= (y - 1);
        switch (type)
        {
        case 0: //water effect
        {
            action_res_t *nod = Rend_CreateNode(NODE_TYPE_REGION);

            nod->slot = aSlot;
            nod->owner = owner;

            SetGNode(aSlot, nod);

            ScrSys_AddToActionsList(nod);

            int32_t s_x, s_y;
            int32_t frames;
            float amplitude,
                waveln,
                speed;
            sscanf(addition, "%d %d %d %f %f %f", &s_x, &s_y, &frames, &amplitude, &waveln, &speed);

            nod->nodes.node_region = Rend_EF_Wave_Setup(delay, frames, s_x, s_y, amplitude, waveln, speed);
        }
        break;

        case 1: //lightning effect
        {
            action_res_t *nod = Rend_CreateNode(NODE_TYPE_REGION);

            nod->slot = aSlot;
            nod->owner = owner;

            SetGNode(aSlot, nod);

            ScrSys_AddToActionsList(nod);

            int32_t d;

            sscanf(addition, "%d", &d);

            nod->nodes.node_region = Rend_EF_Light_Setup(art, x, y, w, h, delay, d);
        }

        break;

        case 9:
        {
            action_res_t *nod = Rend_CreateNode(NODE_TYPE_REGION);

            nod->slot = aSlot;
            nod->owner = owner;

            SetGNode(aSlot, nod);

            ScrSys_AddToActionsList(nod);

            int32_t d, d2;
            char buff[MINIBUFSZ];

            sscanf(addition, "%d %d %s", &d, &d2, buff);

            nod->nodes.node_region = Rend_EF_9_Setup(art, buff, delay, x, y, w, h);
        }

        break;
        }
    }

    return ACTION_NORMAL;
}

static int action_display_message(char *params, int aSlot, pzllst_t *owner)
{
    int32_t p1, p2, p3, p4, p5, p6;

    if (sscanf(params, "%d %d %d %d %d %d", &p1, &p2, &p3, &p4, &p5, &p6) == 6)
    {
    }
    else if (sscanf(params, "%d %d", &p1, &p2) == 2)
    {
        ctrlnode_t *ct = Controls_GetControl(p1);
        if (ct)
            if (ct->type == CTRL_TITLER)
                ct->node.titler->next_string = p2;
    }

    return ACTION_NORMAL;
}

static int action_set_venus(char *params, int aSlot, pzllst_t *owner)
{
    int32_t p1;

    p1 = atoi(params);

    if (p1 > 0)
    {
        if (GetgVarInt(p1) > 0)
            SetgVarInt(SLOT_VENUS, p1);
    }

    return ACTION_NORMAL;
}

static int action_disable_venus(char *params, int aSlot, pzllst_t *owner)
{
    int32_t p1;

    p1 = atoi(params);

    if (p1 > 0)
        SetgVarInt(p1, 0);

    return ACTION_NORMAL;
}


int Actions_Run(const char* name, char *params, int aSlot, pzllst_t *owner)
{
    LOG_DEBUG("Running action %s %s\n", name, params);

    if (str_equals(name, "set_screen"))              return action_set_screen(params, aSlot, owner);
    else if (str_equals(name, "debug"))              return action_debug(params, aSlot, owner);
    else if (str_equals(name, "assign"))             return action_assign(params, aSlot, owner);
    else if (str_equals(name, "timer"))              return action_timer(params, aSlot, owner);
    else if (str_equals(name, "set_partial_screen")) return action_set_partial_screen(params, aSlot, owner);
    else if (str_equals(name, "change_location"))    return action_change_location(params, aSlot, owner);
    else if (str_equals(name, "dissolve"))           return action_dissolve(params, aSlot, owner);
    else if (str_equals(name, "disable_control"))    return action_disable_control(params, aSlot, owner);
    else if (str_equals(name, "enable_control"))     return action_enable_control(params, aSlot, owner);
    else if (str_equals(name, "add"))                return action_add(params, aSlot, owner);
    else if (str_equals(name, "random"))             return action_random(params, aSlot, owner);
    else if (str_equals(name, "animplay"))           return action_animplay(params, aSlot, owner);
    else if (str_equals(name, "universe_music"))     return action_universe_music(params, aSlot, owner);
    else if (str_equals(name, "music"))              return action_music(params, aSlot, owner);
    else if (str_equals(name, "kill"))               return action_kill(params, aSlot, owner);
    else if (str_equals(name, "stop"))               return action_stop(params, aSlot, owner);
    else if (str_equals(name, "inventory"))          return action_inventory(params, aSlot, owner);
    else if (str_equals(name, "crossfade"))          return action_crossfade(params, aSlot, owner);
    else if (str_equals(name, "streamvideo"))        return action_streamvideo(params, aSlot, owner);
    else if (str_equals(name, "animpreload"))        return action_animpreload(params, aSlot, owner);
    else if (str_equals(name, "playpreload"))        return action_playpreload(params, aSlot, owner);
    else if (str_equals(name, "syncsound"))          return action_syncsound(params, aSlot, owner);
    else if (str_equals(name, "menu_bar_enable"))    return action_menu_bar_enable(params, aSlot, owner);
    else if (str_equals(name, "delay_render"))       return action_delay_render(params, aSlot, owner);
    else if (str_equals(name, "ttytext"))            return action_ttytext(params, aSlot, owner);
    else if (str_equals(name, "cursor"))             return action_cursor(params, aSlot, owner);
    else if (str_equals(name, "attenuate"))          return action_attenuate(params, aSlot, owner);
    else if (str_equals(name, "pan_track"))          return action_pan_track(params, aSlot, owner);
    else if (str_equals(name, "animunload"))         return action_animunload(params, aSlot, owner);
    else if (str_equals(name, "flush_mouse_events")) return action_flush_mouse_events(params, aSlot, owner);
    else if (str_equals(name, "save_game"))          return action_save_game(params, aSlot, owner);
    else if (str_equals(name, "restore_game"))       return action_restore_game(params, aSlot, owner);
    else if (str_equals(name, "quit"))               return action_quit(params, aSlot, owner);
    else if (str_equals(name, "rotate_to"))          return action_rotate_to(params, aSlot, owner);
    else if (str_equals(name, "distort"))            return action_distort(params, aSlot, owner);
    else if (str_equals(name, "preferences"))        return action_preferences(params, aSlot, owner);
    else if (str_equals(name, "region"))             return action_region(params, aSlot, owner);
    else if (str_equals(name, "display_message"))    return action_display_message(params, aSlot, owner);
    else if (str_equals(name, "set_venus"))          return action_set_venus(params, aSlot, owner);
    else if (str_equals(name, "disable_venus"))      return action_disable_venus(params, aSlot, owner);

    LOG_WARN("Action '%s' not found!\n", name);

    return ACTION_ERROR; // return ACTION_NOT_FOUND;
}
