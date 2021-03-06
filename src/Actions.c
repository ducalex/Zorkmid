#include "Utilities.h"
#include "Scripting.h"
#include "Inventory.h"
#include "Controls.h"
#include "Actions.h"
#include "Loader.h"
#include "Render.h"
#include "Anims.h"
#include "Sound.h"
#include "Timer.h"
#include "Game.h"
#include "Menu.h"

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

static inline void useart_recovery(char *str)
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

static inline int GetIntVal(char *chr)
{
    if (chr[0] == '[')
        return GetgVarInt(atoi(chr + 1));
    else
        return atoi(chr);
}

static inline int action_set_screen(char *params, int aSlot, int owner)
{
    Rend_LoadGamescr(params);
    return ACTION_NORMAL;
}

static inline int action_set_partial_screen(char *params, int aSlot, int owner)
{
    char x[16], y[16], tmp11[16], key[16], file[255];
    sscanf(params, "%s %s %s %s %s", x, y, file, tmp11, key);

    SDL_Surface *tmp = Loader_LoadGFX(file, Rend_GetRenderer() == RENDER_PANA, GetIntVal(key));
    if (tmp)
    {
        Rend_BlitSurfaceXY(tmp, Rend_GetLocationScreenImage(), GetIntVal(x), GetIntVal(y));
        SDL_FreeSurface(tmp);
    }
    else
    {
        LOG_WARN("Failed to load image '%s'\n", file);
    }
    return ACTION_NORMAL;
}

static inline int action_assign(char *params, int aSlot, int owner)
{
    char tmp1[16], tmp2[16];
    sscanf(params, "%s %s", tmp1, tmp2);

    SetgVarInt(GetIntVal(tmp1), GetIntVal(tmp2));
    return ACTION_NORMAL;
}

static inline int action_timer(char *params, int aSlot, int owner)
{
    int delay = CURRENT_GAME == GAME_ZGI ? 100 : 1000;

    char param[16];
    sscanf(params, "%s", param); // Get first word

    if (GetGNode(aSlot) != NULL)
    {
        return ACTION_NORMAL;
    }

    action_res_t *nod = Timer_CreateNode();
    nod->slot = aSlot;
    nod->owner = owner;
    nod->nodes.node_timer = GetIntVal(param) * delay;

    SetGNode(aSlot, nod);

    ScrSys_AddToActionsList(nod);

    SetgVarInt(aSlot, 1);

    return ACTION_NORMAL;
}

static inline int action_change_location(char *params, int aSlot, int owner)
{
    char w[4], r[4], v[4], x[16];
    sscanf(params, "%c %c %c%c %s", w, r, v, v + 1, x);

    Game_Relocate(tolower(w[0]), tolower(r[0]), tolower(v[0]), tolower(v[1]), GetIntVal(x));

    Rend_SetDelay(2);

    return ACTION_NORMAL;
}

static inline int action_dissolve(char *params, int aSlot, int owner)
{
    Rend_FillRect(Rend_GetGameScreen(), NULL, 0, 0, 0);

    return ACTION_NORMAL;
}

static inline int action_disable_control(char *params, int aSlot, int owner)
{
    ScrSys_SetFlag(GetIntVal(params), FLAG_DISABLED);

    return ACTION_NORMAL;
}

static inline int action_enable_control(char *params, int aSlot, int owner)
{
    ScrSys_SetFlag(GetIntVal(params), 0);

    return ACTION_NORMAL;
}

static inline int action_add(char *params, int aSlot, int owner)
{
    char tmp[16];
    int slot;
    sscanf(params, "%d %s", &slot, tmp);

    SetgVarInt(slot, GetgVarInt(slot) + GetIntVal(tmp));

    return ACTION_NORMAL;
}

static inline int action_debug(char *params, int aSlot, int owner)
{
    char txt[STRBUFSIZE], tmp[16];

    sscanf(params, "%s %s", txt, tmp);
    LOG_WARN("DEBUG :%s\t: %d \n", txt, GetIntVal(tmp));

    return ACTION_NORMAL;
}

static inline int action_random(char *params, int aSlot, int owner)
{
    char param[16];
    sscanf(params, "%s", param);

    SetgVarInt(aSlot, rand() % (GetIntVal(param) + 1));

    return ACTION_NORMAL;
}

static inline int action_streamvideo(char *params, int aSlot, int owner)
{
    char file[PATHBUFSIZE];
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

    avi_file_t *avi = avi_openfile(file, 0);
    if (!avi)
        return ACTION_NORMAL;

    Mix_Chunk *chunk = avi_get_audio(avi);
    int channel = Sound_Play(-1, chunk, 0, 100);
    int frame_delay = MIN(200, avi->mcrSecPframe / 2000);

    subtitles_t *subs = NULL;

    if (GetgVarInt(SLOT_SUBTITLE_FLAG) == 1)
    {
        strcpy(file + (strlen(file) - 3), "sub");
        subs = Text_LoadSubtitles(file);
    }

    SDL_Rect dst_rct = {GAMESCREEN_X + xx, GAMESCREEN_Y + yy, ww, hh};

    avi_play(avi);

    while (avi->playing && !KeyHit(SDLK_SPACE))
    {
        Game_Update();

        avi_update(avi);

        Rend_BlitSurface(avi->surf, NULL, Rend_GetScreen(), &dst_rct);

        if (subs)
            Text_ProcessSubtitles(subs, avi->cur_frame);

        Text_DrawSubtitles();
        Rend_ScreenFlip();

        Game_Delay(frame_delay);
    }

    avi_close(avi);

    if (channel >= 0) Sound_Stop(channel);
    if (chunk) Mix_FreeChunk(chunk);
    if (subs) Text_DeleteSubtitles(subs);

    return ACTION_NORMAL;
}

static inline int action_animplay(char *params, int aSlot, int owner)
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

    dynlist_t *all = GetActionsList();
    for (int i = 0; i < all->length; i++)
    {
        action_res_t *nod = (action_res_t *)all->items[i];
        if (!nod) continue;

        if (nod->slot == aSlot && nod->node_type == NODE_TYPE_ANIMPLAY)
        {
            Anim_DeleteNode(nod);
            DeleteFromList(all, i);
        }
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

static inline int action_music(char *params, int aSlot, int owner, bool universe)
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
        char filename[PATHBUFSIZE];

        sprintf(loop, "%d", (instrument != 0));
        sprintf(filename, "MIDI/%d/%d.wav", instrument, pitch);

        if (!(chunk = Loader_LoadSound(filename)))
        {
            sprintf(filename, "%s/MIDI/%d/%d.wav", Game_GetPath(), instrument, pitch);
            chunk = Loader_LoadSound(filename);
        }
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

static inline int action_syncsound(char *params, int aSlot, int owner)
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

    char a1[16], a2[16], a3[32];

    if (sscanf(params, "%s %s %s", a1, a2, a3) != 3)
    {
        LOG_WARN("Malformed params: '%s'\n", params);
        return ACTION_NORMAL;
    }

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
    tmp->slot = -1; //  aSlot;
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

static inline int action_animpreload(char *params, int aSlot, int owner)
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

static inline int action_playpreload(char *params, int aSlot, int owner)
{
    char sl[16];
    int x, y, w, h, start, end, loop, slot;
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

static inline int action_ttytext(char *params, int aSlot, int owner)
{
    char chars[16];
    int32_t x, y, w, h, delay;
    sscanf(params, "%d %d %d %d %s %d", &x, &y, &w, &h, chars, &delay);

    w -= x;
    h -= y;

    mfile_t *fl = mfopen_txt(chars);
    if (!fl)
    {
        SetgVarInt(aSlot, 2);
        return ACTION_NORMAL;
    }

    action_res_t *nod = Text_CreateTTYText();

    nod->slot = aSlot;
    nod->owner = owner;

    nod->nodes.tty_text->txtbuf = NEW_ARRAY(char, fl->size);

    size_t j = 0;
    for (size_t i = 0; i < fl->size; i++)
        if (fl->buffer[i] != 0x0A && fl->buffer[i] != 0x0D)
            nod->nodes.tty_text->txtbuf[j++] = (char)fl->buffer[i];

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

static int stopkiller(char *params, int aSlot, int owner, bool iskillfunc)
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

    dynlist_t *all = GetActionsList();
    for (int i = 0; i < all->length; i++)
    {
        action_res_t *nod = (action_res_t *)all->items[i];
        if (!nod) continue;

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

            DeleteFromList(all, i);
            break;
        }
    }

    return ACTION_NORMAL;
}

static inline int action_kill(char *params, int aSlot, int owner)
{
    stopkiller(params, aSlot, owner, true);

    return ACTION_NORMAL;
}

static inline int action_stop(char *params, int aSlot, int owner)
{
    stopkiller(params, aSlot, owner, false);

    return ACTION_NORMAL;
}

static inline int action_inventory(char *params, int aSlot, int owner)
{
    char cmd[MINIBUFSIZE];
    char param[MINIBUFSIZE];
    int item = 0;

    if (sscanf(params, "%s %s", cmd, param) == 2)
    {
        item = GetIntVal(param);
    }

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

static inline int action_crossfade(char *params, int aSlot, int owner)
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

static inline int action_menu_bar_enable(char *params, int aSlot, int owner)
{
    Menu_SetVal(GetIntVal(params));

    return ACTION_NORMAL;
}

static inline int action_delay_render(char *params, int aSlot, int owner)
{
    Rend_SetDelay(GetIntVal(params));

    return ACTION_NORMAL;
}

static inline int action_pan_track(char *params, int aSlot, int owner)
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

static inline int action_attenuate(char *params, int aSlot, int owner)
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

static inline int action_cursor(char *params, int aSlot, int owner)
{
    if (tolower(params[0]) == 'u')
        Mouse_ShowCursor();
    else if (tolower(params[0]) == 'h')
        Mouse_HideCursor();
    else
        return ACTION_ERROR;

    return ACTION_NORMAL;
}

static inline int action_animunload(char *params, int aSlot, int owner)
{
    int slot;

    sscanf(params, "%d", &slot);

    action_res_t *nod = GetGNode(slot);

    if (nod != NULL)
        if (nod->node_type == NODE_TYPE_ANIMPRE)
            nod->need_delete = true;

    return ACTION_NORMAL;
}

static inline int action_flush_mouse_events(char *params, int aSlot, int owner)
{
    FlushMouseBtn(MOUSE_BTN_LEFT|MOUSE_BTN_RIGHT);

    return ACTION_NORMAL;
}

static inline int action_save_game(char *params, int aSlot, int owner)
{
    ScrSys_PrepareSaveBuffer();
    ScrSys_SaveGame(params);

    return ACTION_NORMAL;
}

static inline int action_restore_game(char *params, int aSlot, int owner)
{
    ScrSys_LoadGame(params);

    return ACTION_NORMAL;
}

static inline int action_quit(char *params, int aSlot, int owner)
{
    if (atoi(params) == 1)
        Game_Quit();
    else
        game_try_quit();

    return ACTION_NORMAL;
}

static inline int action_rotate_to(char *params, int aSlot, int owner)
{
    if (Rend_GetRenderer() != RENDER_PANA)
        return ACTION_NORMAL;

    int topos;
    int time;
    int maxX = Rend_GetPanaWidth();
    int curX = GetgVarInt(SLOT_VIEW_POS);
    int oner = 0;

    sscanf(params, "%d %d", &topos, &time);

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

        Rend_RenderFrame();
        Game_Delay(500 / time);
    }

    return ACTION_NORMAL;
}

static inline int action_distort(char *params, int aSlot, int owner)
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

static inline int action_preferences(char *params, int aSlot, int owner)
{
    if (str_starts_with(params, "save"))
        ScrSys_SavePreferences();
    else
        ScrSys_LoadPreferences();

    return ACTION_NORMAL;
}

//Graphic effects
static inline int action_region(char *params, int aSlot, int owner)
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
            char buff[MINIBUFSIZE];

            sscanf(addition, "%d %d %s", &d, &d2, buff);

            nod->nodes.node_region = Rend_EF_9_Setup(art, buff, delay, x, y, w, h);
        }

        break;
        }
    }

    return ACTION_NORMAL;
}

static inline int action_display_message(char *params, int aSlot, int owner)
{
    int p1, p2, p3, p4, p5, p6;

    int count = sscanf(params, "%d %d %d %d %d %d", &p1, &p2, &p3, &p4, &p5, &p6);

    if (count == 2)
    {
        ctrlnode_t *ct = Controls_GetControl(p1);
        if (ct && ct->type == CTRL_TITLER)
            ct->node.titler->next_string = p2;
    }

    return ACTION_NORMAL;
}

static inline int action_set_venus(char *params, int aSlot, int owner)
{
    int p1 = atoi(params);

    if (p1 > 0)
    {
        if (GetgVarInt(p1) > 0)
            SetgVarInt(SLOT_VENUS, p1);
    }

    return ACTION_NORMAL;
}

static inline int action_disable_venus(char *params, int aSlot, int owner)
{
    int32_t p1 = atoi(params);

    if (p1 > 0)
        SetgVarInt(p1, 0);

    return ACTION_NORMAL;
}


int Actions_Run(const char* name, char *params, int aSlot, int owner)
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
    else if (str_equals(name, "universe_music"))     return action_music(params, aSlot, owner, true);
    else if (str_equals(name, "music"))              return action_music(params, aSlot, owner, false);
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
