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
    int x, y /*,tmp1*/, tmp2;
    char xx[16], yy[16], tmp11[16], tmp22[16];
    char file[255];
    sscanf(params, "%s %s %s %s %s", xx, yy, file, tmp11, tmp22);

    x = GetIntVal(xx);
    y = GetIntVal(yy);
    tmp2 = GetIntVal(tmp22);

    SDL_Surface *tmp = NULL;

    if (tmp2 > 0)
    {
        int r, g, b;
        b = ((tmp2 >> 10) & 0x1F) * 8;
        g = ((tmp2 >> 5) & 0x1F) * 8;
        r = (tmp2 & 0x1F) * 8;
        tmp = loader_LoadFile_key(file, Rend_GetRenderer() == RENDER_PANA, Rend_MapScreenRGB(r, g, b));
    }
    else
        tmp = loader_LoadFile(file, Rend_GetRenderer() == RENDER_PANA);

    if (!tmp)
        LOG_WARN("IMG_Load(%s): %s\n", params, IMG_GetError());
    else
    {
        Rend_DrawImageToGamescr(tmp, x, y);
        SDL_FreeSurface(tmp);
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
    char tmp2[16];
    sscanf(params, "%s", tmp2);

    if (getGNode(aSlot) != NULL)
    {
        return ACTION_NORMAL;
    }

    action_res_t *nod = NEW(action_res_t);
    nod->nodes.node_timer = 0;

    nod->slot = aSlot;
    nod->owner = owner;
    nod->node_type = NODE_TYPE_TIMER;
    nod->need_delete = false;
    nod->nodes.node_timer = GetIntVal(PrepareString(tmp2)) * TIMER_DELAY;

    setGNode(aSlot, nod);

    ScrSys_AddToActResList(nod);

    SetgVarInt(aSlot, 1);

    return ACTION_NORMAL;
}

static int action_change_location(char *params, int aSlot, pzllst_t *owner)
{
    char tmp[4], tmp2[4], tmp3[4], tmp4[16];
    sscanf(params, "%c %c %c%c %s", tmp, tmp2, tmp3, tmp3 + 1, tmp4);

    SetNeedLocate(tolower(tmp[0]), tolower(tmp2[0]), tolower(tmp3[0]), tolower(tmp3[1]), GetIntVal(tmp4));

    Rend_SetDelay(2);

    return ACTION_NORMAL;
}

static int action_dissolve(char *params, int aSlot, pzllst_t *owner)
{
    SDL_FillRect(Rend_GetGameScreen(), NULL, Rend_MapScreenRGB(0, 0, 0));

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

    int xx, yy, ww, hh, tmp;

    const char *fil;

    sscanf(params, "%s %s %s %s %s %s %s", file, x, y, w, h, u1, u2);

    xx = GetIntVal(x);
    yy = GetIntVal(y);
    ww = GetIntVal(w) - xx + 1;
    hh = GetIntVal(h) - yy + 1;

    char *ext = file + (strlen(file) - 3);

    anim_avi_t *anm = NEW(anim_avi_t);
    Mix_Chunk *aud = NULL;
    subtitles_t *subs = NULL;

    fil = GetFilePath(file);

    if (fil == NULL)
        return ACTION_NORMAL;

    anm->av = avi_openfile(fil, 0);

    anm->img = CreateSurface(anm->av->w, anm->av->h);
    scaler_t *scl = CreateScaler(anm->img, ww, hh);

    if (GetgVarInt(SLOT_SUBTITLE_FLAG) == 1)
    {
        strcpy(ext, "sub");
        subs = sub_LoadSubtitles(file);
    }

    aud = avi_get_audio(anm->av);
    if (aud != NULL)
    {
        tmp = GetFreeChannel();
        Mix_UnregisterAllEffects(tmp);
        Mix_PlayChannel(tmp, aud, 0);
        Mix_Volume(tmp, 127);
    }

    avi_play(anm->av);

    while (anm->av->status == AVI_PLAY)
    {
        GameUpdate();

        if (KeyHit(SDLK_SPACE))
            avi_stop(anm->av);

        avi_to_surf(anm->av, anm->img);
        //DrawImage(anm->img,0,0);
        DrawScalerToScreen(scl, GAMESCREEN_X + xx + GAMESCREEN_FLAT_X, GAMESCREEN_Y + yy); //it's direct rendering without game screen update

        avi_update(anm->av);

        if (subs != NULL)
            sub_ProcessSub(subs, anm->av->cframe);

        Rend_ProcessSubs();
        Rend_ScreenFlip();

        int32_t delay = anm->av->header.mcrSecPframe / 2000;

        if (delay > 200 || delay < 0)
            delay = 15;

        Rend_Delay(delay);
    }

    if (aud != NULL)
    {
        //if (u2 == 0)
        //   RestoreVol();
        Mix_HaltChannel(tmp);
        Mix_FreeChunk(aud);
    }

    if (subs != NULL)
        sub_DeleteSub(subs);

    avi_close(anm->av);
    free(anm);
    DeleteScaler(scl);

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

    MList *all = GetAction_res_List();
    StartMList(all);
    while (!eofMList(all))
    {
        action_res_t *nd = (action_res_t *)DataMList(all);

        if (nd->node_type == NODE_TYPE_ANIMPLAY)
            if (nd->slot == aSlot)
            {
                anim_DeleteAnimPlay(nd);
                DeleteCurrent(all);
            }

        NextMList(all);
    }

    action_res_t *glob = anim_CreateAnimPlayNode();
    animnode_t *nod = glob->nodes.node_anim;

    ScrSys_AddToActResList(glob);

    if (aSlot > 0)
    {
        SetgVarInt(aSlot, 1);
        setGNode(aSlot, glob);
    }

    glob->slot = aSlot;
    glob->owner = owner;

    int mask2, r, g, b;

    mask2 = GetIntVal(mask);

    if (mask2 != -1 && mask2 != 0)
    {
        b = ((mask2 >> 10) & 0x1F) * 8;
        g = ((mask2 >> 5) & 0x1F) * 8;
        r = (mask2 & 0x1F) * 8;
        mask2 = r | g << 8 | b << 16;
    }

    anim_LoadAnim(nod, file, 0, 0, mask2, GetIntVal(framerate));

    anim_PlayAnim(nod, GetIntVal(x),
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
    char fn[PATHBUFSIZ];
    int8_t read_params = sscanf(params, "%d %s %s %s", &type, file, loop, vol);

    if (getGNode(aSlot) != NULL)
        return ACTION_NORMAL;

    action_res_t *nod = snd_CreateWavNode();

    nod->slot = aSlot;
    nod->owner = owner;

    setGNode(nod->slot, nod);

    if (type == 4)
    {
        int32_t instr = atoi(file);
        int32_t pitch = atoi(loop);
        sprintf(fn, "%s/MIDI/%d/%d.wav", GetGamePath(), instr, pitch);
        if (FileExists(fn))
        {
            nod->nodes.node_music->universe = universe;
            nod->nodes.node_music->chunk = Mix_LoadWAV(fn);

            nod->nodes.node_music->chn = GetFreeChannel();
            if (nod->nodes.node_music->chn == -1)
            {
                LOG_WARN("ERROR: NO CHANNELS! %s\n", params);
                snd_DeleteWav(nod);
                return ACTION_NORMAL;
            }

            Mix_UnregisterAllEffects(nod->nodes.node_music->chn);

            if (read_params == 4)
                nod->nodes.node_music->volume = GetIntVal(vol);
            else
                nod->nodes.node_music->volume = 100;

            Mix_Volume(nod->nodes.node_music->chn, GetLogVol(nod->nodes.node_music->volume));

            if (instr == 0)
            {
                Mix_PlayChannel(nod->nodes.node_music->chn, nod->nodes.node_music->chunk, 0);
                nod->nodes.node_music->looped = false;
            }
            else
            {
                Mix_PlayChannel(nod->nodes.node_music->chn, nod->nodes.node_music->chunk, -1);
                nod->nodes.node_music->looped = true;
            }

            LockChan(nod->nodes.node_music->chn);
        }
        else
        {
            snd_DeleteWav(nod);
            return ACTION_NORMAL;
        }
    }
    else
    {
        nod->nodes.node_music->universe = universe;

        char *ext = file + (strlen(file) - 3);

        nod->nodes.node_music->chunk = loader_LoadChunk(file);

        if (nod->nodes.node_music->chunk == NULL)
        {
            strcpy(ext, "raw");
            nod->nodes.node_music->chunk = loader_LoadChunk(file);

            if (nod->nodes.node_music->chunk == NULL)
            {
                strcpy(ext, "ifp");
                nod->nodes.node_music->chunk = loader_LoadChunk(file);

                if (nod->nodes.node_music->chunk == NULL)
                {
                    strcpy(ext, "src");
                    nod->nodes.node_music->chunk = loader_LoadChunk(file);
                }
            }
        }

        if (nod->nodes.node_music->chunk == NULL)
        {
            snd_DeleteWav(nod);
            return ACTION_NORMAL;
        }

        if (GetgVarInt(SLOT_SUBTITLE_FLAG) == 1)
        {
            strcpy(ext, "sub");
            nod->nodes.node_music->sub = sub_LoadSubtitles(file);
        }

        nod->nodes.node_music->chn = GetFreeChannel();
        if (nod->nodes.node_music->chn == -1)
        {
            LOG_WARN("ERROR: NO CHANNELS! %s\n", params);
            snd_DeleteWav(nod);
            return ACTION_NORMAL;
        }

        Mix_UnregisterAllEffects(nod->nodes.node_music->chn);

        if (read_params == 4)
            nod->nodes.node_music->volume = GetIntVal(vol);
        else
            nod->nodes.node_music->volume = 100;

        Mix_Volume(nod->nodes.node_music->chn, GetLogVol(nod->nodes.node_music->volume));

        if (GetIntVal(loop) == 1)
        {
            Mix_PlayChannel(nod->nodes.node_music->chn, nod->nodes.node_music->chunk, -1);
            nod->nodes.node_music->looped = true;
        }
        else
        {
            Mix_PlayChannel(nod->nodes.node_music->chn, nod->nodes.node_music->chunk, 0);
            nod->nodes.node_music->looped = false;
        }

        LockChan(nod->nodes.node_music->chn);
    }

    ScrSys_AddToActResList(nod);

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

    if (getGNode(syncto) == NULL)
        return ACTION_NORMAL;

    action_res_t *tmp = snd_CreateSyncNode();

    tmp->owner = owner;
    tmp->slot = -1;
    //tmp->slot  = aSlot;

    tmp->nodes.node_sync->chn = GetFreeChannel();

    tmp->nodes.node_sync->syncto = syncto;

    if (getGNode(syncto)->node_type == NODE_TYPE_ANIMPRE)
    {
        getGNode(syncto)->nodes.node_animpre->framerate = FPS_DELAY; //~15fps hack
    }

    tmp->nodes.node_sync->chunk = loader_LoadChunk(a3);

    if (tmp->nodes.node_sync->chn == -1 || tmp->nodes.node_sync->chunk == NULL)
    {
        LOG_WARN("ERROR: NO CHANNELS OR FILE! %s\n", params);
        snd_DeleteSync(tmp);
        return ACTION_NORMAL;
    }

    char *ext = a3 + (strlen(a3) - 3);
    strcpy(ext, "sub");

    if (GetgVarInt(SLOT_SUBTITLE_FLAG) == 1)
        tmp->nodes.node_sync->sub = sub_LoadSubtitles(a3);

    Mix_UnregisterAllEffects(tmp->nodes.node_sync->chn);

    Mix_Volume(tmp->nodes.node_sync->chn, GetLogVol(100));

    Mix_PlayChannel(tmp->nodes.node_sync->chn, tmp->nodes.node_sync->chunk, 0);

    LockChan(tmp->nodes.node_sync->chn);

    ScrSys_AddToActResList(tmp);

    return ACTION_NORMAL;
}

static int action_animpreload(char *params, int aSlot, pzllst_t *owner)
{
    if (getGNode(aSlot) != NULL)
        return ACTION_NORMAL;

    char name[64];
    char u1[16];
    char u2[16];
    char u3[16];
    char u4[16];

    action_res_t *pre = anim_CreateAnimPreNode();

    //%s %d %d %d %f
    //name     ? ? mask framerate
    //in zgi   0 0 0
    sscanf(params, "%s %s %s %s %s", name, u1, u2, u3, u4);

    anim_LoadAnim(pre->nodes.node_animpre,
                  name,
                  0, 0,
                  GetIntVal(u3),
                  GetIntVal(u4));

    pre->slot = aSlot;
    pre->owner = owner;

    ScrSys_AddToActResList(pre);

    SetgVarInt(pre->slot, 2);

    setGNode(aSlot, pre);

    return ACTION_NORMAL;
}

static int action_playpreload(char *params, int aSlot, pzllst_t *owner)
{
    char sl[16];
    uint32_t slot;
    int x, y, w, h, start, end, loop;
    sscanf(params, "%s %d %d %d %d %d %d %d", sl, &x, &y, &w, &h, &start, &end, &loop);

    slot = GetIntVal(sl);

    action_res_t *pre = getGNode(slot);

    if (pre == NULL)
    {
        return ACTION_NORMAL;
    }

    if (pre->node_type != NODE_TYPE_ANIMPRE)
        return ACTION_NORMAL;

    action_res_t *nod = anim_CreateAnimPlayPreNode();

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

    ScrSys_AddToActResList(nod);

    if (aSlot > 0)
    {
        setGNode(aSlot, nod);
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

    FManNode_t *fil = FindInBinTree(chars);

    if (fil == NULL)
    {
        SetgVarInt(aSlot, 2);
        return ACTION_NORMAL;
    }

    action_res_t *nod = txt_CreateTTYtext();

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

    txt_get_font_style(&nod->nodes.tty_text->style, nod->nodes.tty_text->txtbuf);
    nod->nodes.tty_text->fnt = GetFontByName(nod->nodes.tty_text->style.fontname, nod->nodes.tty_text->style.size);
    nod->nodes.tty_text->w = w;
    nod->nodes.tty_text->h = h;
    nod->nodes.tty_text->x = x;
    nod->nodes.tty_text->y = y;
    nod->nodes.tty_text->delay = delay;

    nod->nodes.tty_text->img = CreateSurface(w, h);

    SetgVarInt(nod->slot, 1);
    setGNode(nod->slot, nod);

    ScrSys_AddToActResList(nod);

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
            ScrSys_FlushActResList();
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

    MList *all = GetAction_res_List();

    StartMList(all);
    while (!eofMList(all))
    {
        action_res_t *nod = (action_res_t *)DataMList(all);

        if (nod->slot == slot)
        {
            if (nod->node_type == NODE_TYPE_ANIMPRE)
            {
                if (nod->nodes.node_animpre->playing)
                {
                    if (iskillfunc)
                        ScrSys_DeleteNode(nod);
                    else
                    {
                        nod->nodes.node_animpre->playing = false;
                        break;
                    }
                }
                else
                {
                    if (iskillfunc)
                        ScrSys_DeleteNode(nod);
                    else
                        break;
                }
            }
            else
                ScrSys_DeleteNode(nod);

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
        inv_add(item);
    }
    else if (str_equals(cmd, "addi"))
    {
        item = GetgVarInt(item);
        inv_add(item);
    }
    else if (str_equals(cmd, "drop"))
    {
        if (item >= 0)
            inv_drop(item);
        else
            inv_drop(GetgVarInt(SLOT_INVENTORY_MOUSE));
    }
    else if (str_equals(cmd, "dropi"))
    {
        item = GetgVarInt(item);
        inv_drop(item);
    }
    else if (str_equals(cmd, "cycle"))
    {
        inv_cycle();
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
        action_res_t *tmp = getGNode(item);

        if (tmp != NULL)
            if (tmp->node_type == NODE_TYPE_MUSIC)
            {
                action_res_t *tnod = tmp;

                tnod->nodes.node_music->crossfade = true;

                if (GetIntVal(frVol1) >= 0)
                    tnod->nodes.node_music->volume = GetIntVal(frVol1);

                tnod->nodes.node_music->crossfade_params.times = ceil(GetIntVal(tim) / 33.3);
                tnod->nodes.node_music->crossfade_params.deltavolume = ceil((GetIntVal(toVol) - tnod->nodes.node_music->volume) / (float)tnod->nodes.node_music->crossfade_params.times);

                if (Mix_Playing(tnod->nodes.node_music->chn))
                    Mix_Volume(tnod->nodes.node_music->chn, GetLogVol(tnod->nodes.node_music->volume));
            }
    }

    if (item2 > 0)
    {
        action_res_t *tmp = getGNode(item2);

        if (tmp != NULL)
            if (tmp->node_type == NODE_TYPE_MUSIC)
            {
                action_res_t *tnod = tmp;

                tnod->nodes.node_music->crossfade = true;

                if (GetIntVal(frVol2) >= 0)
                    tnod->nodes.node_music->volume = GetIntVal(frVol2);

                tnod->nodes.node_music->crossfade_params.times = ceil(GetIntVal(tim) / 33.3);
                tnod->nodes.node_music->crossfade_params.deltavolume = ceil((GetIntVal(toVol2) - tnod->nodes.node_music->volume) / (float)tnod->nodes.node_music->crossfade_params.times);

                if (Mix_Playing(tnod->nodes.node_music->chn))
                    Mix_Volume(tnod->nodes.node_music->chn, GetLogVol(tnod->nodes.node_music->volume));
            }
    }

    /* MList *wavs = snd_GetWavsList();
     StartMList(wavs);
     while(!eofMList(wavs))
     {
         musicnode *nod = (musicnode *)DataMList(wavs);

    //        if (nod->slot == item)
    //            Mix_Volume(nod->chn , GetLogVol(GetIntVal(toVol)));

     //       if (nod->slot == item2)
     //           Mix_Volume(nod->chn , GetLogVol(GetIntVal(toVol2)));

         NextMList(wavs);
     }*/

    return ACTION_NORMAL;
}

static int action_menu_bar_enable(char *params, int aSlot, pzllst_t *owner)
{
    menu_SetMenuBarVal(GetIntVal(params));

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
        action_res_t *nod = snd_CreatePanTrack();
        nod->nodes.node_pantracking = slot;

        nod->owner = owner;
        nod->slot = aSlot;

        if (nod->slot > 0)
            setGNode(nod->slot, nod);

        ScrSys_AddToActResList(nod);

        action_res_t *tmp = getGNode(slot);

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
        action_res_t *tmp = getGNode(slot);

        if (tmp != NULL)
            if (tmp->node_type == NODE_TYPE_MUSIC)
            {

                tmp->nodes.node_music->attenuate = att;
                Mix_SetPosition(tmp->nodes.node_music->chn,
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

    action_res_t *nod = getGNode(slot);

    if (nod != NULL)
        if (nod->node_type == NODE_TYPE_ANIMPRE)
            nod->need_delete = true;

    return ACTION_NORMAL;
}

static int action_flush_mouse_events(char *params, int aSlot, pzllst_t *owner)
{
    FlushMouseBtn(SDL_BUTTON_LEFT);
    FlushMouseBtn(SDL_BUTTON_RIGHT);

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

    for (int32_t i = 0; i <= time; i++)
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
        Rend_Delay(500 / time);
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

    if (getGNode(slot) != NULL)
        return ACTION_NORMAL;

    action_res_t *act = Rend_CreateDistortNode();
    act->slot = slot;
    act->owner = owner;

    if (slot > 0)
    {
        setGNode(act->slot, act);
        SetgVarInt(act->slot, 1);
    }

    ScrSys_AddToActResList(act);

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
            action_res_t *nod = NEW(action_res_t);

            nod->slot = aSlot;
            nod->owner = owner;
            nod->node_type = NODE_TYPE_REGION;
            nod->need_delete = false;

            setGNode(aSlot, nod);

            ScrSys_AddToActResList(nod);

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
            action_res_t *nod = NEW(action_res_t);

            nod->slot = aSlot;
            nod->owner = owner;
            nod->node_type = NODE_TYPE_REGION;
            nod->need_delete = false;

            setGNode(aSlot, nod);

            ScrSys_AddToActResList(nod);

            int32_t d;

            sscanf(addition, "%d", &d);

            nod->nodes.node_region = Rend_EF_Light_Setup(art, x, y, w, h, delay, d);
        }

        break;

        case 9:
        {
            action_res_t *nod = NEW(action_res_t);

            nod->slot = aSlot;
            nod->owner = owner;
            nod->node_type = NODE_TYPE_REGION;
            nod->need_delete = false;

            setGNode(aSlot, nod);

            ScrSys_AddToActResList(nod);

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
        ctrlnode_t *ct = GetControlByID(p1);
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


int action_exec(const char* name, char *params, int aSlot, pzllst_t *owner)
{
    TRACE_ACTION("Running %s %s\n", name, params);

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