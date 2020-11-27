#include "Anims.h"

action_res_t *Anim_CreateAnimPlayNode()
{
    action_res_t *tmp = ScrSys_CreateActRes(NODE_TYPE_ANIMPLAY);
    tmp->nodes.node_anim = NEW(animnode_t);
    return tmp;
}

action_res_t *Anim_CreateAnimPreNode()
{
    action_res_t *tmp = ScrSys_CreateActRes(NODE_TYPE_ANIMPRE);
    tmp->nodes.node_animpre = NEW(animnode_t);
    return tmp;
}

action_res_t *Anim_CreateAnimPlayPreNode()
{
    action_res_t *tmp = ScrSys_CreateActRes(NODE_TYPE_ANIMPRPL);
    tmp->nodes.node_animpreplay = NEW(anim_preplay_node_t);
    return tmp;
}

void Anim_Load(animnode_t *nod, char *filename, int u1, int u2, int32_t mask, int framerate)
{
    if (framerate != 0)
        nod->framerate = 1000.0 / framerate;
    else
        nod->framerate = 0;

    nod->nexttick = 0;
    nod->loops = 0;

    if (str_ends_with(filename, ".avi")) // AVI
    {
        nod->anim_avi = NEW(anim_avi_t);
        nod->anim_avi->av = avi_openfile(Loader_GetPath(filename), Rend_GetRenderer() == RENDER_PANA);
        nod->anim_avi->lastfrm = -1;

        int w = nod->anim_avi->av->w;
        int h = nod->anim_avi->av->h;

        nod->anim_avi->img = Rend_CreateSurface(w, h, 0);

        if (nod->framerate == 0)
            nod->framerate = nod->anim_avi->av->header.mcrSecPframe / 1000; //~15fps

        nod->rel_h = h;
        nod->rel_w = w;
    }
    else if (str_ends_with(filename, ".rlf"))
    {
        nod->anim_rlf = Loader_LoadRLF(filename, Rend_GetRenderer() == RENDER_PANA, mask);

        if (nod->framerate == 0)
            nod->framerate = nod->anim_rlf->info.time;

        nod->rel_h = nod->anim_rlf->info.h;
        nod->rel_w = nod->anim_rlf->info.w;
    }
    else
    {
        Z_PANIC("Unknown animation format '%s'", filename);
    }
}

void Anim_Process(animnode_t *mnod)
{
    if (!mnod->playing || !mnod)
        return;

    mnod->nexttick -= GetDTime();

    if (mnod->nexttick > 0)
        return;

    mnod->nexttick = mnod->framerate;

    // if (mnod->CurFr < mnod->end)

    if (mnod->anim_avi)
    {
        avi_renderframe(mnod->anim_avi->av, mnod->CurFr);
        avi_to_surf(mnod->anim_avi->av, mnod->anim_avi->img);

        SDL_Rect rect = {mnod->x, mnod->y, mnod->w, mnod->h};
        Rend_BlitSurface(mnod->anim_avi->img, NULL, Rend_GetLocationScreenImage(), &rect);
    }
    else if (mnod->anim_rlf)
    {
        Rend_BlitSurfaceXY(
            mnod->anim_rlf->img[mnod->CurFr],
            Rend_GetLocationScreenImage(),
            mnod->x,
            mnod->y);
    }

    mnod->CurFr++;

    if (mnod->CurFr >= mnod->end)
    {
        mnod->loops++;

        if (mnod->loops < mnod->loopcnt || mnod->loopcnt == 0)
        {
            mnod->CurFr = mnod->start;
        }
        else
        {
            mnod->playing = false;
        }
    }
}

int Anim_ProcessPlayNode(action_res_t *nod)
{
    if (nod->node_type != NODE_TYPE_ANIMPLAY)
        return NODE_RET_OK;

    Anim_Process(nod->nodes.node_anim);

    if (!nod->nodes.node_anim->playing)
    {
        anim_DeleteAnimPlay(nod);
        return NODE_RET_DELETE;
    }

    return NODE_RET_OK;
}

int Anim_ProcessPreNode(action_res_t *nod)
{
    if (nod->node_type != NODE_TYPE_ANIMPRE)
        return NODE_RET_OK;

    Anim_Process(nod->nodes.node_animpre);

    return NODE_RET_OK;
}

int Anim_ProcessPrePlayNode(action_res_t *nod)
{
    if (nod->node_type != NODE_TYPE_ANIMPRPL)
        return NODE_RET_OK;

    anim_preplay_node_t *pre = nod->nodes.node_animpreplay;

    if (pre->playerid == 0)
    {
        pre->playerid = Anim_Play(pre->point, pre->x,
                                      pre->y,
                                      pre->w,
                                      pre->h,
                                      pre->start,
                                      pre->end,
                                      pre->loop);
        SetgVarInt(pre->pointingslot, 1);
        if (nod->slot > 0)
            SetgVarInt(nod->slot, 1);
    }
    else
    {
        if (!pre->point->playing)
        {
            SetgVarInt(nod->slot, 2);
            SetgVarInt(nod->nodes.node_animpreplay->pointingslot, 2);
            anim_DeleteAnimPrePlayNode(nod);
            return NODE_RET_DELETE;
        }
    }

    return NODE_RET_OK;
}

int Anim_Play(animnode_t *nod, int x, int y, int w, int h, int start, int end, int loop)
{
    static int AnimID = 1;

    nod->playing = true;
    nod->playID = AnimID++;
    nod->w = w;
    nod->h = h;
    nod->x = x;
    nod->y = y;

    nod->start = start;
    nod->end = end;

    if (nod->anim_avi)
    {
        avi_renderframe(nod->anim_avi->av, nod->start);
        avi_to_surf(nod->anim_avi->av, nod->anim_avi->img);
    }

    nod->CurFr = nod->start;
    nod->loopcnt = loop;

    return nod->playID;
}

void Anim_RenderFrame(animnode_t *mnod, int16_t x, int16_t y, int16_t w, int16_t h, int16_t frame)
{
    if (!mnod)
        return;

    if (mnod->anim_avi)
    {
        if (mnod->anim_avi->lastfrm != frame)
        {
            avi_renderframe(mnod->anim_avi->av, frame);
            avi_to_surf(mnod->anim_avi->av, mnod->anim_avi->img);
        }

        mnod->anim_avi->lastfrm = frame;

        SDL_Rect rect = {x, y, w, h};
        Rend_BlitSurface(mnod->anim_avi->img, NULL, Rend_GetLocationScreenImage(), &rect);
    }
    else if (mnod->anim_rlf)
    {
        Rend_BlitSurfaceXY(mnod->anim_rlf->img[frame], Rend_GetLocationScreenImage(), x, y);
    }
}

void anim_DeleteAnimImage(anim_surf_t *anim)
{
    if (!anim)
        return;

    for (int i = 0; i < anim->info.frames; i++)
        if (anim->img[i])
            SDL_FreeSurface(anim->img[i]);

    free(anim->img);
    free(anim);
}

void anim_DeleteAnim(animnode_t *nod)
{
    if (nod->anim_avi)
    {
        if (nod->anim_avi->img)
            SDL_FreeSurface(nod->anim_avi->img);

        avi_close(nod->anim_avi->av);
        free(nod->anim_avi);
    }

    if (nod->anim_rlf)
    {
        anim_DeleteAnimImage(nod->anim_rlf);
    }

    free(nod);
}

int anim_DeleteAnimPlay(action_res_t *nod)
{
    if (nod->node_type != NODE_TYPE_ANIMPLAY)
        return NODE_RET_NO;

    anim_DeleteAnim(nod->nodes.node_anim);

    if (nod->slot > 0)
    {
        SetgVarInt(nod->slot, 2);
        setGNode(nod->slot, NULL);
    }

    free(nod);

    return NODE_RET_DELETE;
}

int anim_DeleteAnimPreNod(action_res_t *nod)
{
    if (nod->node_type != NODE_TYPE_ANIMPRE)
        return NODE_RET_NO;

    MList *lst = GetAction_res_List();
    pushMList(lst);
    StartMList(lst);
    while (!eofMList(lst))
    {
        action_res_t *nod2 = (action_res_t *)DataMList(lst);

        if (nod2->node_type == NODE_TYPE_ANIMPRPL)
        {
            if (nod2->nodes.node_animpreplay->pointingslot == nod->slot)
                nod2->need_delete = true;
        }

        NextMList(lst);
    }
    popMList(lst);

    setGNode(nod->slot, NULL);

    anim_DeleteAnim(nod->nodes.node_animpre);
    free(nod);

    return NODE_RET_DELETE;
}

int anim_DeleteAnimPrePlayNode(action_res_t *nod)
{
    if (nod->node_type != NODE_TYPE_ANIMPRPL)
        return NODE_RET_NO;

    if (nod->slot > 0)
    {
        SetgVarInt(nod->slot, 2);
        setGNode(nod->slot, NULL);
    }

    SetgVarInt(nod->nodes.node_animpreplay->pointingslot, 2);

    free(nod->nodes.node_animpreplay);
    free(nod);

    return NODE_RET_DELETE;
}
