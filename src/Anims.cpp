#include "Anims.h"

action_res_t *anim_CreateAnimPlayNode()
{
    action_res_t *tmp = ScrSys_CreateActRes(NODE_TYPE_ANIMPLAY);
    tmp->nodes.node_anim = NEW(animnode_t);
    return tmp;
}

action_res_t *anim_CreateAnimPreNode()
{
    action_res_t *tmp = ScrSys_CreateActRes(NODE_TYPE_ANIMPRE);
    tmp->nodes.node_animpre = NEW(animnode_t);
    return tmp;
}

action_res_t *anim_CreateAnimPlayPreNode()
{
    action_res_t *tmp = ScrSys_CreateActRes(NODE_TYPE_ANIMPRPL);
    tmp->nodes.node_animpreplay = NEW(anim_preplay_node_t);
    return tmp;
}

void anim_LoadAnim(animnode_t *nod, char *filename, int u1, int u2, int32_t mask, int framerate)
{
    if (framerate != 0)
        nod->framerate = 1000.0 / framerate;
    else
        nod->framerate = 0;

    nod->nexttick = 0;
    nod->loops = 0;

    if (strcasestr(filename, ".avi") != NULL)
    {
        nod->anim.avi = NEW(anim_avi);
        nod->vid = 1;

        nod->anim.avi->av = avi_openfile(GetFilePath(filename), Rend_GetRenderer() == RENDER_PANA);
        int16_t w, h;

        w = nod->anim.avi->av->w;
        h = nod->anim.avi->av->h;

        nod->anim.avi->img = CreateSurface(w, h);

        nod->anim.avi->lastfrm = -1;

        if (nod->framerate == 0)
            nod->framerate = nod->anim.avi->av->header.mcrSecPframe / 1000; //~15fps

        nod->rel_h = h;
        nod->rel_w = w;
    }
#ifdef SMPEG_SUPPORT
    else if (strcasestr(filename, ".mpg") != NULL)
    {
        nod->anim.mpg = NEW(anim_mpg);
        nod->vid = 2;

        nod->anim.mpg->mpg = SMPEG_new(GetFilePath(filename), &nod->anim.mpg->inf, 0);
        int16_t w, h;

        w = nod->anim.mpg->inf.width;
        h = nod->anim.mpg->inf.height;

        nod->anim.mpg->img = CreateSurface(w, h);

        SMPEG_setdisplay(nod->anim.mpg->mpg, nod->anim.mpg->img, 0, 0);
        SMPEG_setdisplayregion(nod->anim.mpg->mpg, 0, 0, nod->anim.mpg->inf.width, nod->anim.mpg->inf.height);

        nod->anim.mpg->lastfrm = -1;

        if (nod->framerate == 0)
            nod->framerate = FPS_DELAY; //~15fps

        nod->rel_h = h;
        nod->rel_w = w;
    }
#endif
    else
    {
        if (strcasestr(filename, ".rlf") != NULL)
            nod->anim.rlf = loader_LoadRlf(filename, Rend_GetRenderer() == RENDER_PANA, mask);
        else
            nod->anim.rlf = LoadAnimImage(filename, mask);

        nod->vid = 0;

        if (nod->framerate == 0)
            nod->framerate = nod->anim.rlf->info.time;

        nod->rel_h = nod->anim.rlf->info.h;
        nod->rel_w = nod->anim.rlf->info.w;
    }
}

void anim_ProcessAnim(animnode_t *mnod)
{
    if (!mnod->playing || !mnod)
        return;

    mnod->nexttick -= GetDTime();

    if (mnod->nexttick <= 0)
    {
        mnod->nexttick = mnod->framerate;

        if (mnod->vid == 1)
        {
            avi_renderframe(mnod->anim.avi->av, mnod->CurFr);
            avi_to_surf(mnod->anim.avi->av, mnod->anim.avi->img);
            Rend_DrawScalerToGamescr(mnod->scal,
                                        mnod->x,
                                        mnod->y);
        }
#ifdef SMPEG_SUPPORT
        else if (mnod->vid == 2)
        {
            SMPEG_renderFrame(mnod->anim.mpg->mpg,
                                mnod->CurFr + 1);

            Rend_DrawScalerToGamescr(mnod->scal,
                                        mnod->x,
                                        mnod->y);
            mnod->CurFr++;
        }
#endif
        else
            Rend_DrawAnimImageToGamescr(mnod->anim.rlf, mnod->x, mnod->y, mnod->CurFr);

        mnod->CurFr++;

        if (mnod->CurFr > mnod->end)
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
}

int anim_ProcessAnimPlayNode(action_res_t *nod)
{
    if (nod->node_type != NODE_TYPE_ANIMPLAY)
        return NODE_RET_OK;

    anim_ProcessAnim(nod->nodes.node_anim);

    if (!nod->nodes.node_anim->playing)
    {
        anim_DeleteAnimPlay(nod);
        return NODE_RET_DELETE;
    }

    return NODE_RET_OK;
}

int anim_ProcessAnimPreNode(action_res_t *nod)
{
    if (nod->node_type != NODE_TYPE_ANIMPRE)
        return NODE_RET_OK;

    anim_ProcessAnim(nod->nodes.node_animpre);

    return NODE_RET_OK;
}

int anim_ProcessAnimPrePlayNode(action_res_t *nod)
{
    if (nod->node_type != NODE_TYPE_ANIMPRPL)
        return NODE_RET_OK;

    anim_preplay_node_t *pre = nod->nodes.node_animpreplay;

    if (pre->playerid == 0)
    {
        pre->playerid = anim_PlayAnim(pre->point, pre->x,
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

int anim_PlayAnim(animnode_t *nod, int x, int y, int w, int h, int start, int end, int loop)
{
    static int AnimID = 1;

    nod->playing = true;
    nod->playID = AnimID++;
    nod->w = w;
    nod->h = h;
    nod->x = x;
    nod->y = y;

    if (nod->vid == 1)
    {
        if (nod->scal != NULL)
            DeleteScaler(nod->scal);

        nod->scal = CreateScaler(nod->anim.avi->img, nod->w, nod->h);

        nod->start = start;
        nod->end = end;

        avi_renderframe(nod->anim.avi->av, nod->start);
        avi_to_surf(nod->anim.avi->av, nod->anim.avi->img);
    }
    else if (nod->vid == 2)
    {
#ifdef SMPEG_SUPPORT
        if (nod->scal != NULL)
            DeleteScaler(nod->scal);

        nod->scal = CreateScaler(nod->anim.mpg->img, nod->w, nod->h);

        nod->start = (start + 1) * 2;
        nod->end = (end + 1) * 2;

        SMPEG_renderFrame(nod->anim.mpg->mpg, nod->start);
#endif
    }
    else
    {
        nod->start = start;
        nod->end = end;
    }

    nod->CurFr = nod->start;
    nod->loopcnt = loop;

    return nod->playID;
}

void anim_RenderAnimFrame(animnode_t *mnod, int16_t x, int16_t y, int16_t w, int16_t h, int16_t frame)
{
    if (!mnod)
        return;

    if (mnod->vid == 1)
    {
        if (mnod->anim.avi != NULL)
        {
            if (mnod->anim.avi->lastfrm != frame)
            {
                avi_renderframe(mnod->anim.avi->av, frame);
                avi_to_surf(mnod->anim.avi->av, mnod->anim.avi->img);
            }

            mnod->anim.avi->lastfrm = frame;

            if (mnod->scal)
                if (mnod->scal->w != w || mnod->scal->h != h)
                {
                    DeleteScaler(mnod->scal);
                    mnod->scal = NULL;
                }

            if (!mnod->scal)
                mnod->scal = CreateScaler(mnod->anim.avi->img, w, h);

            Rend_DrawScalerToGamescr(mnod->scal, x, y);
        }
    }
    else if (mnod->vid == 2)
    {
#ifdef SMPEG_SUPPORT
        if (mnod->anim.mpg != NULL)
        {
            if (mnod->anim.mpg->lastfrm != frame)
            {
                SMPEG_renderFrame(mnod->anim.mpg->mpg, frame * 2);
                SMPEG_renderFrame(mnod->anim.mpg->mpg, frame * 2 + 1);
                if (mnod->anim.mpg->lastfrm > frame)
                    SMPEG_renderFinal(mnod->anim.mpg->mpg, mnod->anim.mpg->img, 0, 0);
            }

            mnod->anim.mpg->lastfrm = frame;

            if (mnod->scal)
                if (mnod->scal->w != w || mnod->scal->h != h)
                {
                    DeleteScaler(mnod->scal);
                    mnod->scal = NULL;
                }

            if (!mnod->scal)
                mnod->scal = CreateScaler(mnod->anim.mpg->img, w, h);

            Rend_DrawScalerToGamescr(mnod->scal, x, y);

            //Rend_DrawImageToGamescr(mnod->anim.mpg->img, x, y);
        }
#endif
    }
    else if (mnod->anim.rlf)
    {
        Rend_DrawAnimImageToGamescr(mnod->anim.rlf, x, y, frame);
    }
}

void anim_DeleteAnim(animnode_t *nod)
{
    if (nod->scal != NULL)
        DeleteScaler(nod->scal);

    if (nod->vid == 1)
    {
        if (nod->anim.avi->img)
            SDL_FreeSurface(nod->anim.avi->img);

        avi_close(nod->anim.avi->av);
        free(nod->anim.avi);
    }
    else if (nod->vid == 2)
    {
#ifdef SMPEG_SUPPORT
        if (nod->anim.mpg->img)
            SDL_FreeSurface(nod->anim.mpg->img);
        SMPEG_stop(nod->anim.mpg->mpg);
        SMPEG_delete(nod->anim.mpg->mpg);
        free(nod->anim.mpg);
#endif
    }
    else
        FreeAnimImage(nod->anim.rlf);

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
