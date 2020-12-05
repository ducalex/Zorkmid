#include "Utilities.h"
#include "Scripting.h"
#include "Inventory.h"
#include "Controls.h"
#include "Loader.h"
#include "Render.h"
#include "Anims.h"
#include "Mouse.h"
#include "Game.h"

static int FocusInput = 0;
static bool pushChangeMouse = false;

static void DrawImageToGameScreen(SDL_Surface *scr, int x, int y)
{
    if (Rend_GetRenderer() == RENDER_TILT)
        y = y + GAMESCREEN_H_2 - GetgVarInt(SLOT_VIEW_POS);

    Rend_BlitSurfaceXY(scr, Rend_GetGameScreen(), x, y);
}

static void ctrl_setvenus(ctrlnode_t *nod)
{
    if (nod->venus >= 0)
    {
        if (GetgVarInt(nod->venus) > 0)
            SetgVarInt(SLOT_VENUS, nod->venus);
    }
}

static bool ctrl_eligeblity(int obj, slotnode_t *slot)
{
    for (int i = 0; i < slot->eligable_cnt; i++)
        if (obj == slot->eligible_objects[i])
            return true;
    return false;
}

static bool ctrl_eligeblity_slots(int obj, int32_t *slots, int32_t count)
{
    for (int i = 0; i < count; i++)
        if (obj == slots[i])
            return true;
    return false;
}

static void control_slot_draw(ctrlnode_t *nod)
{
    slotnode_t *slut = nod->node.slot;
    int tmp1 = GetgVarInt(nod->slot);
    char buf[MINIBUFSIZE];

    if (tmp1 == 0 || tmp1 != slut->loaded_img)
    {
        if (slut->srf)
            SDL_FreeSurface(slut->srf);
        slut->srf = NULL;
        slut->loaded_img = -1;
    }

    if (tmp1 == 0)
        return;

    if (!slut->srf)
    {
        if (CURRENT_GAME == GAME_ZGI)
            sprintf(buf, "g0z%1.1su%2.2x1.tga", slut->distance_id, tmp1);
        else
            sprintf(buf, "%d%sOBJ.TGA", tmp1, slut->distance_id);

        slut->srf = Loader_LoadGFX(buf, 0, 0x0000);

        slut->loaded_img = tmp1;
    }

    if (slut->srf)
    {
        int32_t drawx = slut->rectangle.x;
        int32_t drawy = slut->rectangle.y;

        if ((slut->rectangle.w - slut->rectangle.x) > slut->srf->w)
            drawx = slut->rectangle.x + ((slut->rectangle.w - slut->rectangle.x) - slut->srf->w) / 2;

        if ((slut->rectangle.h - slut->rectangle.y) > slut->srf->h)
            drawy = slut->rectangle.y + ((slut->rectangle.h - slut->rectangle.y) - slut->srf->h) / 2;

        DrawImageToGameScreen(slut->srf, drawx, drawy);
    }
}

static void control_input_draw(ctrlnode_t *ct)
{
    inputnode_t *inp = ct->node.inp;

    if (!str_empty(inp->text))
    {
        if (inp->textchanged)
        {
            Rend_FillRect(inp->rect, NULL, 0, 0, 0);

            if (!inp->readonly || !inp->focused)
                inp->textwidth = Text_Draw(inp->text, &inp->string_init, inp->rect);
            else
                inp->textwidth = Text_Draw(inp->text, &inp->string_chooser_init, inp->rect);

            inp->textchanged = false;
        }
        DrawImageToGameScreen(inp->rect, inp->rectangle.x, inp->rectangle.y);
    }
    else
        inp->textwidth = 0;

    if (FocusInput == ct->slot)
    {
        if (!inp->readonly)
        {
            if (inp->cursor != NULL)
            {
                Rend_BlitSurfaceXY(
                    inp->cursor->img[inp->frame],
                    Rend_GetGameScreen(),
                    inp->rectangle.x + inp->textwidth,
                    inp->rectangle.y);

                inp->period -= Game_GetDTime();

                if (inp->period <= 0)
                {
                    inp->frame++;
                    inp->period = inp->cursor->info.time;
                }

                int32_t max_frames = inp->cursor->info.frames;

                if (inp->frame >= max_frames)
                    inp->frame = 0;
            }
        }
    }
}

static void control_lever_draw(ctrlnode_t *ct)
{
    levernode_t *lev = ct->node.lev;
    if (lev->curfrm == lev->rendfrm)
        return;

    if ((lev->rendfrm > lev->curfrm) && lev->mirrored)
        Anim_RenderFrame(lev->anm, lev->AnimCoords.x, lev->AnimCoords.y, lev->AnimCoords.w, lev->AnimCoords.h, lev->frames * 2 - 1 - lev->curfrm);
    else
        Anim_RenderFrame(lev->anm, lev->AnimCoords.x, lev->AnimCoords.y, lev->AnimCoords.w, lev->AnimCoords.h, lev->curfrm);

    lev->rendfrm = lev->curfrm;
}

static void control_input(ctrlnode_t *ct)
{
    inputnode_t *inp = ct->node.inp;
    bool mousein = false;

    if (inp->rectangle.x <= Rend_GetMouseGameX() &&
        inp->rectangle.w >= Rend_GetMouseGameX() &&
        inp->rectangle.y <= Rend_GetMouseGameY() &&
        inp->rectangle.h >= Rend_GetMouseGameY())
        mousein = true;
    if (inp->hotspot.x <= Rend_GetMouseGameX() &&
        inp->hotspot.w >= Rend_GetMouseGameX() &&
        inp->hotspot.y <= Rend_GetMouseGameY() &&
        inp->hotspot.h >= Rend_GetMouseGameY())
        mousein = true;

    if (FocusInput == ct->slot)
    {
        if (!inp->readonly)
        {
            if (KeyAnyHit())
            {
                SDLKey key = GetLastKey();
                int tmplen = strlen(inp->text);

                if ((key >= SDLK_0 && key <= SDLK_9) ||
                    (key >= SDLK_a && key <= SDLK_z) ||
                    (key == SDLK_SPACE))
                {
                    if (tmplen < SAVE_NAME_MAX_LEN)
                    {
                        inp->text[tmplen] = key;
                        inp->textchanged = true;
                    }
                }
                else if (key == SDLK_BACKSPACE)
                {
                    if (tmplen > 0)
                    {
                        inp->text[tmplen - 1] = 0x0;
                        inp->textchanged = true;
                    }
                }
                else if (key == SDLK_RETURN)
                {
                    // if (tmplen > 0)
                    inp->enterkey = true;
                }
                else if (key == SDLK_TAB)
                {
                    if (inp->next_tab > 0)
                        FocusInput = inp->next_tab;

                    FlushKeybKey(SDLK_TAB);
                }
                else if (key == SDLK_ESCAPE)
                {
                    inp->text[0] = 0;
                    inp->textchanged = true;
                }
            }
        }
        else
        {
            if (KeyHit(SDLK_TAB))
            {
                if (inp->next_tab > 0)
                {
                    int32_t cycle = inp->next_tab;
                    ctrlnode_t *c_tmp = ct;
                    while (cycle > 0 && cycle != ct->slot && c_tmp != NULL)
                    {
                        c_tmp = Controls_GetControl(cycle);

                        if (c_tmp)
                            if (c_tmp->type == CTRL_INPUT)
                            {
                                if (!str_empty(c_tmp->node.inp->text))
                                {
                                    FocusInput = cycle;
                                    break;
                                }
                                else
                                    cycle = c_tmp->node.inp->next_tab;
                            }
                    }
                }

                FlushKeybKey(SDLK_TAB);
            }
            else if (KeyHit(SDLK_RETURN))
            {
                inp->enterkey = true;
            }
        }
    }

    if (inp->readonly)
    {
        if (FocusInput == ct->slot && !inp->focused)
        {
            inp->textchanged = true;
            inp->focused = true;
        }
        else if (FocusInput != ct->slot && inp->focused)
        {
            inp->textchanged = true;
            inp->focused = false;
        }
    }

    if (mousein)
    {
        if (!inp->readonly)
        {
            if (Mouse_IsCurrentCur(CURSOR_IDLE))
                Mouse_SetCursor(CURSOR_ACTIVE);

            if (MouseUp(MOUSE_BTN_LEFT))
            {
                ctrl_setvenus(ct);
                FocusInput = ct->slot;
                FlushMouseBtn(MOUSE_BTN_LEFT);
            }
        }
        else if (inp->text[0] >= ' ')
        {
            if (Mouse_IsCurrentCur(CURSOR_IDLE))
                Mouse_SetCursor(CURSOR_ACTIVE);

            if (MouseMove())
                FocusInput = ct->slot;

            if (MouseUp(MOUSE_BTN_LEFT))
            {
                ctrl_setvenus(ct);
                FocusInput = 0;
                inp->enterkey = true;
            }
        }
    }
}

static void control_slot(ctrlnode_t *ct)
{
    slotnode_t *slot = ct->node.slot;
    bool mousein = false;

    if (slot->hotspot.x <= Rend_GetMouseGameX() &&
        slot->hotspot.w >= Rend_GetMouseGameX() &&
        slot->hotspot.y <= Rend_GetMouseGameY() &&
        slot->hotspot.h >= Rend_GetMouseGameY())
        mousein = true;

    if (mousein)
    {

        //  if (GetgVarInt(ct->slot)!=0)
        if (Mouse_IsCurrentCur(CURSOR_IDLE))
            Mouse_SetCursor(slot->cursor);

        if (MouseUp(MOUSE_BTN_LEFT))
        {

            ctrl_setvenus(ct);

            int32_t item = GetgVarInt(ct->slot);
            int32_t mouse_item = GetgVarInt(SLOT_INVENTORY_MOUSE);
            if (item != 0)
            {
                if (mouse_item != 0)
                {
                    if (ctrl_eligeblity(mouse_item, slot))
                    {
                        Inventory_Drop(mouse_item);
                        Inventory_Add(item);
                        SetgVarInt(ct->slot, mouse_item);
                    }
                }
                else
                {
                    Inventory_Add(item);
                    SetgVarInt(ct->slot, 0);
                }
            }
            else if (mouse_item == 0)
            {
                if (ctrl_eligeblity(0, slot))
                {
                    Inventory_Drop(0);
                    SetgVarInt(ct->slot, 0);
                }
            }
            else if (ctrl_eligeblity(mouse_item, slot))
            {
                SetgVarInt(ct->slot, mouse_item);
                Inventory_Drop(mouse_item);
            }

            FlushMouseBtn(MOUSE_BTN_LEFT);
        }
    }
}

static void control_paint(ctrlnode_t *ct)
{
    paintnode_t *paint = ct->node.paint;

    if (!Rend_MouseInGamescr())
        return;

    int32_t mX = Rend_GetMouseGameX();
    int32_t mY = Rend_GetMouseGameY();

    bool mousein = (mX >= paint->rectangle.x &&
                    mX < paint->rectangle.x + paint->rectangle.w &&
                    mY >= paint->rectangle.y &&
                    mY < paint->rectangle.y + paint->rectangle.h);

    if (!mousein)
        return;

    if (!ctrl_eligeblity_slots(GetgVarInt(SLOT_INVENTORY_MOUSE), paint->eligible_objects, paint->eligable_cnt))
        return;

    if (Mouse_IsCurrentCur(CURSOR_IDLE))
        Mouse_SetCursor(paint->cursor);

    if (!MouseDown(MOUSE_BTN_LEFT))
        return;

    ctrl_setvenus(ct);

    if (mX == paint->last_x || mY == paint->last_y)
        return;

    SDL_Surface *scrn = Rend_GetLocationScreenImage();
    SDL_LockSurface(scrn);
    SDL_LockSurface(paint->paint);

    int32_t cen_x = 0; // paint->b_w / 2;
    int32_t cen_y = 0; //paint->b_h / 2;

    int32_t d_x = mX - paint->rectangle.x;
    int32_t d_y = mY - paint->rectangle.y;

    for (int y = 0; y < paint->b_h; y++)
    {
        for (int x = 0; x < paint->b_w; x++)
        {
            if (paint->brush[x + y * paint->b_w] == 0)
                continue;

            if ((d_x - cen_x) + x >= 0 && (d_x - cen_x) + x < paint->paint->w &&
                (d_y - cen_y) + y >= 0 && (d_y - cen_y) + y < paint->paint->h)
            {
                color_t col = Rend_GetPixel(paint->paint, (d_x - cen_x) + x, (d_y - cen_y) + y);
                Rend_SetPixel(scrn, (mX - cen_x) + x, (mY - cen_y) + y, col.r, col.g, col.b);
            }
        }
    }

    SDL_UnlockSurface(scrn);
    SDL_UnlockSurface(paint->paint);

    paint->last_x = mX;
    paint->last_y = mY;
}

static void control_fist(ctrlnode_t *ct)
{
    bool mousein = false;
    fistnode_t *fist = ct->node.fist;
    int32_t n_fist = -1;

    if (!Rend_MouseInGamescr())
        return;

    int mX = Rend_GetMouseGameX();
    int mY = Rend_GetMouseGameY();

    if (fist->order != 0)
    {
        for (int i = 0; i < fist->fistnum; i++)
        {
            if (((fist->fiststatus >> i) & 1) == 1)
            {
                for (int j = 0; j < fist->fists_dwn[i].num_box; j++)
                    if (fist->fists_dwn[i].boxes[j].x <= mX &&
                        fist->fists_dwn[i].boxes[j].x2 >= mX &&
                        fist->fists_dwn[i].boxes[j].y <= mY &&
                        fist->fists_dwn[i].boxes[j].y2 >= mY)
                    {
                        mousein = true;
                        n_fist = i;
                        break;
                    }
            }
            else
            {
                for (int j = 0; j < fist->fists_up[i].num_box; j++)
                    if (fist->fists_up[i].boxes[j].x <= mX &&
                        fist->fists_up[i].boxes[j].x2 >= mX &&
                        fist->fists_up[i].boxes[j].y <= mY &&
                        fist->fists_up[i].boxes[j].y2 >= mY)
                    {
                        mousein = true;
                        n_fist = i;
                        break;
                    }
            }

            if (mousein)
                break;
        }
    }
    else
    {
        for (int i = fist->fistnum - 1; i >= 0; i--)
        {
            if (((fist->fiststatus >> i) & 1) == 1)
            {
                for (int j = 0; j < fist->fists_dwn[i].num_box; j++)
                    if (fist->fists_dwn[i].boxes[j].x <= mX &&
                        fist->fists_dwn[i].boxes[j].x2 >= mX &&
                        fist->fists_dwn[i].boxes[j].y <= mY &&
                        fist->fists_dwn[i].boxes[j].y2 >= mY)
                    {
                        mousein = true;
                        n_fist = i;
                        break;
                    }
            }
            else
            {
                for (int j = 0; j < fist->fists_up[i].num_box; j++)
                    if (fist->fists_up[i].boxes[j].x <= mX &&
                        fist->fists_up[i].boxes[j].x2 >= mX &&
                        fist->fists_up[i].boxes[j].y <= mY &&
                        fist->fists_up[i].boxes[j].y2 >= mY)
                    {
                        mousein = true;
                        n_fist = i;
                        break;
                    }
            }

            if (mousein)
                break;
        }
    }

    if (mousein)
    {
        if (Mouse_IsCurrentCur(CURSOR_IDLE))
            Mouse_SetCursor(CURSOR_ACTIVE);

        if (MouseUp(MOUSE_BTN_LEFT))
        {

            ctrl_setvenus(ct);

            uint32_t old_status = fist->fiststatus;
            fist->fiststatus ^= (1 << n_fist);

            for (int i = 0; i < fist->num_entries; i++)
                if (fist->entries[i].strt == old_status &&
                    fist->entries[i].send == fist->fiststatus)
                {
                    fist->frame_cur = fist->entries[i].anm_str;
                    fist->frame_end = fist->entries[i].anm_end;
                    fist->frame_time = 0;

                    SetgVarInt(fist->animation_id, 1);
                    SetgVarInt(fist->soundkey, fist->entries[i].sound);
                    break;
                }

            SetgVarInt(ct->slot, fist->fiststatus);

            FlushMouseBtn(MOUSE_BTN_LEFT);
        }
    }
}

static void control_fist_draw(ctrlnode_t *ct)
{
    fistnode_t *fist = ct->node.fist;

    if (fist->frame_cur >= 0 && fist->frame_end >= 0 && fist->frame_cur <= fist->frame_end)
    {
        fist->frame_time -= Game_GetDTime();

        if (fist->frame_time <= 0)
        {
            fist->frame_time = fist->anm->framerate;

            Anim_RenderFrame(fist->anm, fist->anm_rect.x, fist->anm_rect.y,
                                    fist->anm->rel_w, fist->anm->rel_h,
                                    fist->frame_cur);

            fist->frame_cur++;
            if (fist->frame_cur > fist->frame_end)
                SetgVarInt(fist->animation_id, 2);
        }
    }
}

static void control_hotmv(ctrlnode_t *ct)
{
    bool mousein = false;

    hotmvnode_t *hotm = ct->node.hotmv;

    if (hotm->cycle < hotm->num_cycles)
    {
        hotm->frame_time -= Game_GetDTime();
        if (hotm->frame_time <= 0)
        {
            hotm->frame_time = hotm->anm->framerate;
            hotm->cur_frame++;
            if (hotm->cur_frame >= hotm->num_frames)
            {
                hotm->cycle++;
                hotm->cur_frame = 0;
                if (hotm->cycle == hotm->num_cycles)
                {
                    SetgVarInt(ct->slot, 2);
#ifdef TRACE
                    printf("Max Cycles reached HotMov %d(Slot)\n", ct->slot);
#endif
                }
            }
        }
    }

    if (hotm->cycle < hotm->num_cycles)
    {
        if (!Rend_MouseInGamescr())
            return;

        int32_t mX = Rend_GetMouseGameX();
        int32_t mY = Rend_GetMouseGameY();
        int32_t curfr = hotm->cur_frame;

        if (hotm->rect.x + hotm->frame_list[curfr].x <= mX &&
            hotm->rect.x + hotm->frame_list[curfr].x2 >= mX &&
            hotm->rect.y + hotm->frame_list[curfr].y <= mY &&
            hotm->rect.y + hotm->frame_list[curfr].y2 >= mY)
            mousein = true;

        if (mousein)
        {
            if (Mouse_IsCurrentCur(CURSOR_IDLE))
                Mouse_SetCursor(CURSOR_ACTIVE);

            if (MouseUp(MOUSE_BTN_LEFT))
            {
                ctrl_setvenus(ct);

                FlushMouseBtn(MOUSE_BTN_LEFT);

                SetgVarInt(ct->slot, 1);
            }
        }
    }
}

static void control_titler(ctrlnode_t *ct)
{
    titlernode_t *titler = ct->node.titler;
    if (titler->current_string != titler->next_string && titler->next_string >= 0 && titler->next_string < CTRL_TITLER_MAX_STRINGS)
    {
        titler->current_string = titler->next_string;
        Rend_FillRect(titler->surface, NULL, 0, 0, 0);
        if (titler->strings[titler->current_string] != NULL && titler->surface != NULL)
            Text_DrawInOneLine(titler->strings[titler->current_string], titler->surface);
    }
}

static void control_titler_draw(ctrlnode_t *ct)
{
    titlernode_t *titler = ct->node.titler;

    if (titler->surface)
        DrawImageToGameScreen(titler->surface, titler->rectangle.x, titler->rectangle.y);
}

static void control_hotmv_draw(ctrlnode_t *ct)
{
    hotmvnode_t *hotm = ct->node.hotmv;
    if (hotm->cur_frame == hotm->rend_frame)
        return;

    hotm->rend_frame = hotm->cur_frame;

    if (hotm->cycle < hotm->num_cycles)
        Anim_RenderFrame(hotm->anm, hotm->rect.x, hotm->rect.y, hotm->rect.w, hotm->rect.h, hotm->cur_frame);
}

static void control_safe(ctrlnode_t *ct)
{
    bool mousein = false;
    safenode_t *safe = ct->node.safe;

    int32_t mX = Rend_GetMouseGameX();
    int32_t mY = Rend_GetMouseGameY();

    if (!Rend_MouseInGamescr())
        return;

    if (safe->rectangle.x <= mX && safe->rectangle.x + safe->rectangle.w >= mX &&
        safe->rectangle.y <= mY && safe->rectangle.y + safe->rectangle.h >= mY)
    {
        int32_t mR = (mX - safe->center_x) * (mX - safe->center_x) + (mY - safe->center_y) * (mY - safe->center_y);

        if (mR < safe->radius_outer_sq &&
            mR > safe->radius_inner_sq)
            mousein = true;
    }

    if (!mousein)
        return;

    if (Mouse_IsCurrentCur(CURSOR_IDLE))
        Mouse_SetCursor(CURSOR_ACTIVE);

    if (MouseUp(MOUSE_BTN_LEFT))
    {
        FlushMouseBtn(MOUSE_BTN_LEFT);

        ctrl_setvenus(ct);

        float raddeg = 57.29578; //180/3.1415926
        float dd = atan2(mX - safe->center_x, mY - safe->center_y) * raddeg;

        int32_t dp_state = 360 / safe->num_states;

        int32_t m_state = (safe->num_states - ((((int32_t)dd + 540) % 360) / dp_state)) % safe->num_states;

        int32_t v3 = (m_state + safe->cur_state - safe->zero_pointer + safe->num_states - 1) % safe->num_states;

        int32_t dbl = safe->num_states * 2;

        int32_t v11 = (dbl + v3) % safe->num_states;

        int32_t v8 = (v11 + safe->num_states - safe->start_pointer) % safe->num_states;

        safe->cur_state = v11;
        safe->to_frame = v8;

        SetgVarInt(ct->slot, v11);
    }
}

static void control_safe_draw(ctrlnode_t *ct)
{
    safenode_t *safe = ct->node.safe;
    if (safe->cur_frame == safe->to_frame)
        return;

    safe->frame_time -= Game_GetDTime();

    if (safe->frame_time <= 0)
    {
        if (safe->cur_frame < safe->to_frame)
        {
            safe->cur_frame++;
            Anim_RenderFrame(safe->anm, safe->rectangle.x, safe->rectangle.y, safe->rectangle.w, safe->rectangle.h, safe->cur_frame);
        }
        else
        {
            safe->cur_frame--;
            Anim_RenderFrame(safe->anm, safe->rectangle.x, safe->rectangle.y, safe->rectangle.w, safe->rectangle.h, safe->num_states * 2 - safe->cur_frame);
        }

        safe->frame_time = safe->anm->framerate;
    }
}

static void control_push(ctrlnode_t *ct)
{
    pushnode_t *psh = ct->node.push;

    if (!Rend_MouseInGamescr())
        return;

    bool mousein = (psh->x <= Rend_GetMouseGameX() && psh->x + psh->w >= Rend_GetMouseGameX() &&
                    psh->y <= Rend_GetMouseGameY() && psh->y + psh->h >= Rend_GetMouseGameY());

    if (!mousein)
        return;

    if (!pushChangeMouse)
        if (Mouse_IsCurrentCur(CURSOR_IDLE))
        {
            Mouse_SetCursor(psh->cursor);
            pushChangeMouse = true;
        }

    int8_t pushed = 0;

    switch (psh->event)
    {
    case CTRL_PUSH_EV_UP:
        if (MouseUp(MOUSE_BTN_LEFT))
            pushed = 1;
        break;
    case CTRL_PUSH_EV_DWN:
        if (MouseHit(MOUSE_BTN_LEFT))
            pushed = 1;
        break;
    case CTRL_PUSH_EV_DBL:
        if (MouseDblClk())
            pushed = 1;
        break;
    default:

        if (MouseUp(MOUSE_BTN_LEFT))
            pushed = 1;
        break;
    };

    if (pushed == 1)
    {
        ctrl_setvenus(ct);

        int32_t val = GetgVarInt(ct->slot);
        val++;
        val %= psh->count_to;
        SetgVarInt(ct->slot, val);

        FlushMouseBtn(MOUSE_BTN_LEFT);
    }
}

static void control_save(ctrlnode_t *ct)
{
    saveloadnode_t *sv = ct->node.svld;
    char fln[32];

    for (int i = 0; i < MAX_SAVES; i++)
        if (sv->inputslot[i] != -1)
            if (sv->input_nodes[i]->node.inp->enterkey)
            {
                if (sv->forsaving)
                {
                    if (!str_empty(sv->input_nodes[i]->node.inp->text))
                    {
                        bool tosave = true;

                        sprintf(fln, CTRL_SAVE_SAVES, i + 1);
                        if (Loader_FindFile(fln) != NULL)
                        {
                            tosave = game_question_message(Game_GetString(SYSTEM_STR_SAVEEXIST));
                        }

                        if (tosave)
                        {
                            FILE *f = fopen(CTRL_SAVE_FILE, "wb");

                            for (int j = 0; j < MAX_SAVES; j++)
                                if (j != i)
                                    fprintf(f, "%s\r\n", sv->Names[j]);
                                else
                                    fprintf(f, "%s\r\n", sv->input_nodes[i]->node.inp->text);

                            fclose(f);

                            ScrSys_SaveGame(fln);
                            game_delay_message(1500, Game_GetString(SYSTEM_STR_SAVED));
                            Game_Relocate('0', '0', '0', '0', 0);
                            break;
                        }
                    }
                    else
                    {
                        game_timed_message(2000, Game_GetString(SYSTEM_STR_SAVEEMPTY));
                    }
                }
                else
                {
                    sprintf(fln, CTRL_SAVE_SAVES, i + 1);
                    ScrSys_LoadGame(fln);
                    break;
                }

                sv->input_nodes[i]->node.inp->enterkey = false;
            }
}

static void control_lever(ctrlnode_t *ct)
{
    levernode_t *lev = ct->node.lev;
    if (lev->curfrm < CTRL_LEVER_MAX_FRAMES)
    {
        if (!lev->mouse_captured)
        {
            if (lev->hotspots[lev->curfrm].x <= Rend_GetMouseGameX() &&
                lev->hotspots[lev->curfrm].x + lev->delta_x >= Rend_GetMouseGameX() &&
                lev->hotspots[lev->curfrm].y <= Rend_GetMouseGameY() &&
                lev->hotspots[lev->curfrm].y + lev->delta_y >= Rend_GetMouseGameY())
            {
                Mouse_SetCursor(lev->cursor);

                if (MouseDown(MOUSE_BTN_LEFT))
                {
                    ctrl_setvenus(ct);

                    lev->mouse_captured = true;
                    lev->mouse_count = CTRL_LEVER_ANGL_TIME;
                    lev->mouse_angle = -1;
                    lev->last_mouse_x = MouseX();
                    lev->last_mouse_y = MouseY();
                    lev->autoout = false;
                }
            }

            //if (!lev->mouse_captured) /* if still not pressed*/
            if (lev->autoout)
            {
                if (lev->autoseq_frm < lev->hasout[lev->autoseq])
                {
                    lev->autoseq_time -= Game_GetDTime();

                    if (lev->autoseq_time < 0)
                    {
                        lev->curfrm = lev->outproc[lev->autoseq][lev->autoseq_frm];
                        SetgVarInt(ct->slot, lev->curfrm);
                        lev->autoseq_frm++;
                        lev->autoseq_time = CTRL_LEVER_AUTO_DELAY;
                    }
                }
                else
                    lev->autoout = false;
            }
        }
        else
        {
            if (!MouseDown(MOUSE_BTN_LEFT))
            {
                lev->mouse_captured = false;

                if (!lev->autoout) /* not initiated */
                {
                    if (lev->hasout[lev->curfrm] > 0) /* if has animation */
                    {
                        lev->autoseq = lev->curfrm;
                        lev->autoseq_frm = 0;
                        lev->autoseq_time = CTRL_LEVER_AUTO_DELAY;
                        lev->autoout = true;
                    }
                }
            }
            else
            {
                Mouse_SetCursor(lev->cursor);

                if (lev->mouse_angle != -1)
                    for (int j = 0; j < CTRL_LEVER_MAX_DIRECTS; j++)
                        if (lev->hotspots[lev->curfrm].directions[j].toframe != -1)
                        {
                            int16_t angl = lev->hotspots[lev->curfrm].directions[j].angle;

                            if (angl + CTRL_LEVER_ANGL_DELTA > lev->mouse_angle &&
                                angl - CTRL_LEVER_ANGL_DELTA < lev->mouse_angle)
                            {
                                lev->curfrm = lev->hotspots[lev->curfrm].directions[j].toframe;
                                SetgVarInt(ct->slot, lev->curfrm);

                                lev->mouse_angle = -1;

                                break;
                            }
                        }
            }
        }
    }

    if (lev->mouse_count <= 0)
    {
        lev->mouse_count = CTRL_LEVER_ANGL_TIME;
        lev->mouse_angle = Mouse_GetAngle(lev->last_mouse_x, lev->last_mouse_y, MouseX(), MouseY());
        lev->last_mouse_x = MouseX();
        lev->last_mouse_y = MouseY();
    }

    lev->mouse_count -= Game_GetDTime();
}

static pushnode_t *CreatePushNode()
{
    pushnode_t *tmp = NEW(pushnode_t);
    tmp->cursor = CURSOR_IDLE;
    tmp->count_to = 2;
    tmp->event = CTRL_PUSH_EV_UP;
    return tmp;
}

static inputnode_t *CreateInputNode()
{
    inputnode_t *tmp = NEW(inputnode_t);
    tmp->textchanged = true;
    Text_InitStyle(&tmp->string_init);
    Text_InitStyle(&tmp->string_chooser_init);
    return tmp;
}

static slotnode_t *CreateSlotNode()
{
    slotnode_t *tmp = NEW(slotnode_t);
    tmp->cursor = CURSOR_IDLE;
    tmp->loaded_img = -1;
    return tmp;
}

static fistnode_t *CreateFistNode()
{
    fistnode_t *tmp = NEW(fistnode_t);
    tmp->cursor = CURSOR_IDLE;
    tmp->frame_cur = -1;
    tmp->frame_end = -1;
    return tmp;
}

static hotmvnode_t *CreateHotMovieNode()
{
    hotmvnode_t *tmp = NEW(hotmvnode_t);
    tmp->cur_frame = -1;
    tmp->rend_frame = -1;
    return tmp;
}

static levernode_t *CreateLeverNode()
{
    levernode_t *tmp = NEW(levernode_t);
    tmp->cursor = CURSOR_IDLE;
    tmp->rendfrm = -1;
    tmp->autoseq = -1;
    for (int i = 0; i < CTRL_LEVER_MAX_FRAMES; i++)
        for (int j = 0; j < CTRL_LEVER_MAX_DIRECTS; j++)
            tmp->hotspots[i].directions[j].toframe = -1;
    return tmp;
}

static saveloadnode_t *CreateSaveNode()
{
    saveloadnode_t *tmp = NEW(saveloadnode_t);
    for (int i = 0; i < MAX_SAVES; i++)
        tmp->inputslot[i] = -1;
    return tmp;
}

static safenode_t *CreateSafeNode()
{
    safenode_t *tmp = NEW(safenode_t);
    tmp->cur_frame = -1;
    return tmp;
}

static paintnode_t *CreatePaintNode()
{
    paintnode_t *tmp = NEW(paintnode_t);
    tmp->cursor = CURSOR_IDLE;
    return tmp;
}

static titlernode_t *CreateTitlerNode()
{
    return NEW(titlernode_t);
}

static ctrlnode_t *Ctrl_CreateNode(int type)
{
    ctrlnode_t *tmp;
    tmp = NEW(ctrlnode_t);
    tmp->venus = -1;
    tmp->type = CTRL_UNKN;

    switch (type)
    {
    case CTRL_PUSH:
        tmp->type = CTRL_PUSH;
        tmp->node.push = CreatePushNode();
        tmp->func = control_push;
        break;
    case CTRL_INPUT:
        tmp->type = CTRL_INPUT;
        tmp->node.inp = CreateInputNode();
        tmp->func = control_input;
        break;
    case CTRL_SLOT:
        tmp->type = CTRL_SLOT;
        tmp->node.slot = CreateSlotNode();
        tmp->func = control_slot;
        break;
    case CTRL_SAVE:
        tmp->type = CTRL_SAVE;
        tmp->node.svld = CreateSaveNode();
        tmp->func = control_save;
        break;
    case CTRL_LEVER:
        tmp->type = CTRL_LEVER;
        tmp->node.lev = CreateLeverNode();
        tmp->func = control_lever;
        break;
    case CTRL_SAFE:
        tmp->type = CTRL_SAFE;
        tmp->node.safe = CreateSafeNode();
        tmp->func = control_safe;
        break;
    case CTRL_FIST:
        tmp->type = CTRL_FIST;
        tmp->node.fist = CreateFistNode();
        tmp->func = control_fist;
        break;
    case CTRL_HOTMV:
        tmp->type = CTRL_HOTMV;
        tmp->node.hotmv = CreateHotMovieNode();
        tmp->func = control_hotmv;
        break;
    case CTRL_PAINT:
        tmp->type = CTRL_PAINT;
        tmp->node.paint = CreatePaintNode();
        tmp->func = control_paint;
        break;
    case CTRL_TITLER:
        tmp->type = CTRL_TITLER;
        tmp->node.titler = CreateTitlerNode();
        tmp->func = control_titler;
        break;
    };
    return tmp;
}

static int Parse_Control_Flat()
{
    Rend_SetRenderer(RENDER_FLAT);
    return 1;
}

static int Parse_Control_Lever(dynlist_t *controls, mfile_t *fl, uint32_t slot)
{
    char tmpbuf[STRBUFSIZE];
    char filename[MINIBUFSIZE];
    int32_t t1, t2, t3, t4;

    ctrlnode_t *ctnode = Ctrl_CreateNode(CTRL_LEVER);
    levernode_t *lev = ctnode->node.lev;

    AddToList(controls, ctnode);

    ctnode->slot = slot;

    while (!mfeof(fl))
    {
        mfgets(tmpbuf, STRBUFSIZE, fl);
        char *str = PrepareString(tmpbuf);

        if (str[0] == '}')
        {
            break;
        }
        else if (str_starts_with(str, "descfile"))
        {
            strcpy(filename, GetParams(str));
        }
        else if (str_starts_with(str, "cursor"))
        {
            lev->cursor = Mouse_GetCursorIndex(GetParams(str));
        }
        else if (str_starts_with(str, "venus_id"))
        {
            ctnode->venus = atoi(GetParams(str));
        }
    }

    mfile_t *descfile = mfopen_txt(filename);
    if (!descfile)
        return 0;

    while (!mfeof(descfile))
    {
        mfgets(tmpbuf, STRBUFSIZE, descfile);
        char *str = PrepareString(tmpbuf);

        if (str_starts_with(str, "animation_id"))
        {
            //sscanf(str,"animation_id:%d",);
        }
        else if (str_starts_with(str, "filename"))
        {
            sscanf(str, "filename:%s", tmpbuf);
            size_t ln = strlen(tmpbuf);
            if (tmpbuf[ln - 1] == '~')
                tmpbuf[ln - 1] = 0;
            lev->anm = NEW(animnode_t);
            Anim_Load(lev->anm, tmpbuf, 0, 0, 0, 0);
        }
        else if (str_starts_with(str, "skipcolor"))
        {
        }
        else if (str_starts_with(str, "anim_coords"))
        {
            sscanf(str, "anim_coords:%d %d %d %d~", &t1, &t2, &t3, &t4);
            lev->AnimCoords.x = t1;
            lev->AnimCoords.y = t2;
            lev->AnimCoords.w = t3 - t1 + 1;
            lev->AnimCoords.h = t4 - t2 + 1;
        }
        else if (str_starts_with(str, "mirrored"))
        {
            sscanf(str, "mirrored:%d", &t1);
            if (t1 == 1)
                lev->mirrored = true;
            else
                lev->mirrored = false;
        }
        else if (str_starts_with(str, "frames"))
        {
            sscanf(str, "frames:%d", &t1);
            lev->frames = t1;
        }
        else if (str_starts_with(str, "elsewhere"))
        {
        }
        else if (str_starts_with(str, "out_of_control"))
        {
        }
        else if (str_starts_with(str, "start_pos"))
        {
            sscanf(str, "start_pos:%d", &t1);
            lev->startpos = t1;
            lev->curfrm = lev->startpos;
        }
        else if (str_starts_with(str, "hotspot_deltas"))
        {
            sscanf(str, "hotspot_deltas:%d %d", &t1, &t2);
            lev->delta_x = t1;
            lev->delta_y = t2;
        }
        else if (sscanf(str, "%d:%d %d", &t1, &t2, &t3) == 3)
        {
            if (t1 < CTRL_LEVER_MAX_FRAMES)
            {
                lev->hotspots[t1].x = t2;
                lev->hotspots[t1].y = t3;
                char *token;
                const char *find = " ";
                strcpy(tmpbuf, str);
                token = strtok(tmpbuf, find);
                while (token != NULL)
                {
                    if (tolower(token[0]) == 'd')
                    {
                        int32_t t4, t5;
                        sscanf(token, "d=%d,%d", &t4, &t5);
                        if (lev->hotspots[t1].angles < CTRL_LEVER_MAX_DIRECTS)
                        {
                            int16_t angles = lev->hotspots[t1].angles;
                            lev->hotspots[t1].directions[angles].toframe = t4;
                            lev->hotspots[t1].directions[angles].angle = t5;
                            lev->hotspots[t1].angles++;
                        }
                    }
                    token = strtok(NULL, find);
                }
                lev->hasout[t1] = 0;
                size_t len = strlen(str);
                for (size_t g = 0; g < len; g++)
                    if (tolower(str[g]) == 'p')
                    {
                        int32_t tr1, tr2;
                        int8_t num = sscanf(str + g + 1, "(%d to %d)", &tr1, &tr2);
                        if (num == 2)
                        {
                            lev->outproc[t1][lev->hasout[t1]] = tr2;
                            lev->hasout[t1]++;
                        }
                    }
            }
        }
    }

    mfclose(descfile);

    lev->curfrm = GetgVarInt(ctnode->slot);

    return 1;
}

static int Parse_Control_HotMov(dynlist_t *controls, mfile_t *fl, uint32_t slot)
{
    char tmpbuf[STRBUFSIZE];
    char filename[MINIBUFSIZE];
    char *str;
    int32_t t1, t2, t3, t4, tt;

    ctrlnode_t *ctnode = Ctrl_CreateNode(CTRL_HOTMV);
    hotmvnode_t *hotm = ctnode->node.hotmv;

    AddToList(controls, ctnode);

    ctnode->slot = slot;
    SetDirectgVarInt(slot, 0);

    while (!mfeof(fl))
    {
        mfgets(tmpbuf, STRBUFSIZE, fl);
        str = PrepareString(tmpbuf);

        if (str[0] == '}')
        {
            break;
        }
        else if (str_starts_with(str, "hs_frame_list"))
        {
            strcpy(filename, GetParams(str));
        }
        else if (str_starts_with(str, "num_frames"))
        {
            hotm->num_frames = atoi(GetParams(str)) + 1;
            hotm->frame_list = NEW_ARRAY(Rect_t, hotm->num_frames);
        }
        else if (str_starts_with(str, "num_cycles"))
        {
            hotm->num_cycles = atoi(GetParams(str));
        }
        else if (str_starts_with(str, "animation"))
        {
            sscanf(GetParams(str), "%s", tmpbuf);
            hotm->anm = NEW(animnode_t);
            Anim_Load(hotm->anm, tmpbuf, 0, 0, 0, 0);
        }
        else if (str_starts_with(str, "rectangle"))
        {
            sscanf(GetParams(str), "%d %d %d %d", &t1, &t2, &t3, &t4);
            hotm->rect.x = t1;
            hotm->rect.y = t2;
            hotm->rect.w = t3 - t1 + 1;
            hotm->rect.h = t4 - t2 + 1;
        }
        else if (str_starts_with(str, "venus_id"))
        {
            ctnode->venus = atoi(GetParams(str));
        }
    }

    mfile_t *frmfile = mfopen_txt(filename);
    if (!frmfile)
        return 0;

    while (!mfeof(frmfile))
    {
        mfgets(tmpbuf, STRBUFSIZE, frmfile);
        sscanf(tmpbuf, "%d:%d %d %d %d~", &tt, &t1, &t2, &t3, &t4);
        if (tt >= 0 && tt < hotm->num_frames)
        {
            hotm->frame_list[tt].x = t1;
            hotm->frame_list[tt].y = t2;
            hotm->frame_list[tt].x2 = t3;
            hotm->frame_list[tt].y2 = t4;
        }
    }

    mfclose(frmfile);

    return 1;
}

static int Parse_Control_Panorama(mfile_t *fl)
{
    char buf[STRBUFSIZE];
    char *str;
    int good = 0;

    double angle = 27.0;
    double k = 0.55;
    int tmp = 0;

    Rend_SetRenderer(RENDER_PANA);

    while (!mfeof(fl))
    {
        mfgets(buf, STRBUFSIZE, fl);
        str = PrepareString(buf);

        if (str[0] == '}')
        {
            good = 1;
            break;
        }
        else if (str_starts_with(str, "angle"))
        {
            str = GetParams(str);
            angle = atof(str);
            Rend_pana_SetAngle(angle);
        }
        else if (str_starts_with(str, "linscale"))
        {
            str = GetParams(str);
            k = atof(str);
            Rend_pana_SetLinscale(k);
        }
        else if (str_starts_with(str, "reversepana"))
        {
            str = GetParams(str);
            tmp = atoi(str);
            if (tmp == 1)
                Rend_SetReversePana(true);
        }
        else if (str_starts_with(str, "zeropoint"))
        {
            str = GetParams(str);
            tmp = atoi(str);
            Rend_pana_SetZeroPoint(tmp);
        }
    }

    Rend_pana_SetTable();
    return good;
}

static int Parse_Control_Tilt(mfile_t *fl)
{
    char buf[STRBUFSIZE];
    char *str;
    int good = 0;

    double angle = 27.0;
    double k = 0.55;
    int tmp = 0;

    Rend_SetRenderer(RENDER_TILT);

    while (!mfeof(fl))
    {
        mfgets(buf, STRBUFSIZE, fl);
        str = PrepareString(buf);

        if (str[0] == '}')
        {
            good = 1;
            break;
        }
        else if (str_starts_with(str, "angle") == 0)
        {
            str = GetParams(str);
            angle = atof(str);
            Rend_tilt_SetAngle(angle);
        }
        else if (str_starts_with(str, "linscale") == 0)
        {
            str = GetParams(str);
            k = atof(str);
            Rend_tilt_SetLinscale(k);
        }
        else if (str_starts_with(str, "reversepana") == 0)
        {
            str = GetParams(str);
            tmp = atoi(str);
            if (tmp == 1)
                Rend_SetReversePana(true);
        }
    }

    Rend_tilt_SetTable();
    return good;
}

static int Parse_Control_Save(dynlist_t *controls, mfile_t *fl, uint32_t slot)
{
    char buf[STRBUFSIZE];
    char *str;
    int good = 0;

    ctrlnode_t *ctnode = Ctrl_CreateNode(CTRL_SAVE);
    saveloadnode_t *sv = ctnode->node.svld;

    AddToList(controls, ctnode);

    ctnode->slot = slot;
    SetDirectgVarInt(slot, 0);

    memset(sv->Names, 0, sizeof(sv->Names));

    mfile_t *mfp = mfopen_txt(CTRL_SAVE_FILE);
    if (mfp)
    {
        size_t count = 0;
        while (!mfeof(mfp))
        {
            mfgets(sv->Names[count++], SAVE_NAME_MAX_LEN, mfp);
        }
        mfclose(mfp);
    }

    while (!mfeof(fl))
    {
        mfgets(buf, STRBUFSIZE, fl);
        str = PrepareString(buf);

        if (str[0] == '}')
        {
            good = 1;
            break;
        }
        else if (str_starts_with(str, "savebox"))
        {
            str = GetParams(str);
            int ctrlslot, saveslot;
            sscanf(str, "%d %d", &saveslot, &ctrlslot);

            saveslot--;

            ctrlnode_t *nd = Controls_GetControl(ctrlslot);
            if (nd && nd->type == CTRL_INPUT)
            {
                strcpy(nd->node.inp->text, sv->Names[saveslot]);
                nd->node.inp->textchanged = true;
                sv->inputslot[saveslot] = ctrlslot;
                sv->input_nodes[saveslot] = nd;
            }
        }
        else if (str_starts_with(str, "control_type"))
        {
            str = GetParams(str);
            if (str_starts_with(str, "save"))
                sv->forsaving = true;
            else if (str_starts_with(str, "restore"))
                sv->forsaving = false;
            else
                LOG_WARN("Unknown control_type: %s\n", str);
        }
        else
        {
            LOG_WARN("Unknown parameter for save control: %s\n", str);
        }

    }

    for (int i = 0; i < MAX_SAVES; i++)
        if (sv->inputslot[i] != -1)
        {
            ctrlnode_t *nd = sv->input_nodes[i];
            if (nd && nd->type == CTRL_INPUT)
                nd->node.inp->readonly = !sv->forsaving;
        }
    return good;
}

static int Parse_Control_Titler(dynlist_t *controls, mfile_t *fl, uint32_t slot)
{
    char buf[STRBUFSIZE];
    char *str;
    int good = 0;

    ctrlnode_t *ctnode = Ctrl_CreateNode(CTRL_TITLER);
    titlernode_t *titler = ctnode->node.titler;

    AddToList(controls, ctnode);
    ctnode->slot = slot;

    while (!mfeof(fl))
    {
        mfgets(buf, STRBUFSIZE, fl);
        str = PrepareString(buf);

        if (str[0] == '}')
        {
            good = 1;
            break;
        }
        else if (str_starts_with(str, "rectangle") == 0)
        {
            sscanf(GetParams(str), "%d %d %d %d",
                   &titler->rectangle.x,
                   &titler->rectangle.y,
                   &titler->rectangle.w,
                   &titler->rectangle.h);

            titler->surface = Rend_CreateSurface(titler->rectangle.w - titler->rectangle.x + 1, titler->rectangle.h - titler->rectangle.y + 1, 0);
            Rend_SetColorKey(titler->surface, 0, 0, 0);
        }
        else if (str_starts_with(str, "string_resource_file") == 0)
        {
            mfile_t *mfp = mfopen_txt(GetParams(str));
            if (mfp)
            {
                titler->num_strings = 0;

                while (!mfeof(mfp))
                {
                    mfgets(buf, STRBUFSIZE, mfp);
                    titler->strings[titler->num_strings++] = str_trim(buf);
                }

                mfclose(mfp);
            }
        }
    }
    return good;
}

static int Parse_Control_Input(dynlist_t *controls, mfile_t *fl, uint32_t slot)
{
    ctrlnode_t *ctnode = Ctrl_CreateNode(CTRL_INPUT);
    inputnode_t *inp = ctnode->node.inp;
    char buf[STRBUFSIZE];
    int good = 0;

    AddToList(controls, ctnode);

    ctnode->slot = slot;
    SetDirectgVarInt(slot, 0);

    while (!mfeof(fl))
    {
        mfgets(buf, STRBUFSIZE, fl);
        char *str = PrepareString(buf);

        if (str[0] == '}')
        {
            good = 1;
            break;
        }
        else if (str_starts_with(str, "rectangle"))
        {
            sscanf(GetParams(str), "%d %d %d %d",
                   &inp->rectangle.x,
                   &inp->rectangle.y,
                   &inp->rectangle.w,
                   &inp->rectangle.h);

            inp->rect = Rend_CreateSurface(inp->rectangle.w - inp->rectangle.x, inp->rectangle.h - inp->rectangle.y, 0);
            Rend_SetColorKey(inp->rect, 0, 0, 0);
        }
        else if (str_starts_with(str, "aux_hotspot"))
        {
            sscanf(GetParams(str), "%d %d %d %d",
                   &inp->hotspot.x,
                   &inp->hotspot.y,
                   &inp->hotspot.w,
                   &inp->hotspot.h);
        }
        else if (str_starts_with(str, "cursor_animation_frames"))
        {
            //  inp->frame = atoi(GetParams(str));
        }
        else if (str_starts_with(str, "next_tabstop"))
        {
            inp->next_tab = atoi(GetParams(str));
        }
        else if (str_starts_with(str, "cursor_animation"))
        {
            char file[16];
            sscanf(GetParams(str), "%s", file);
            inp->cursor = Loader_LoadRLF(file, 0, 0);
        }
        else if (str_starts_with(str, "focus"))
        {
            if (atoi(GetParams(str)) == 1)
                FocusInput = ctnode->slot;
        }
        else if (str_starts_with(str, "string_init"))
        {
            const char *tmp = Game_GetString(atoi(GetParams(str)));
            if (tmp != NULL)
                Text_GetStyle(&inp->string_init, tmp);
        }
        else if (str_starts_with(str, "chooser_init_string"))
        {
            const char *tmp = Game_GetString(atoi(GetParams(str)));
            if (tmp != NULL)
                Text_GetStyle(&inp->string_chooser_init, tmp);
        }
        else if (str_starts_with(str, "venus_id"))
        {
            ctnode->venus = atoi(GetParams(str));
        }

    }
    return good;
}

static int Parse_Control_Paint(dynlist_t *controls, mfile_t *fl, uint32_t slot)
{
    ctrlnode_t *ctnode = Ctrl_CreateNode(CTRL_PAINT);
    paintnode_t *paint = ctnode->node.paint;
    char filename[MINIBUFSIZE];
    char buf[STRBUFSIZE];
    int good = 0;

    AddToList(controls, ctnode);
    ctnode->slot = slot;

    while (!mfeof(fl))
    {
        mfgets(buf, STRBUFSIZE, fl);
        char *str = PrepareString(buf);

        if (str[0] == '}')
        {
            good = 1;
            break;
        }
        else if (str_starts_with(str, "rectangle"))
        {
            sscanf(GetParams(str), "%d %d %d %d",
                   &paint->rectangle.x,
                   &paint->rectangle.y,
                   &paint->rectangle.w,
                   &paint->rectangle.h);
        }
        else if (str_starts_with(str, "brush_file"))
        {
            SDL_Surface *tmp = Loader_LoadGFX(GetParams(str), false, -1);
            if (tmp)
            {
                paint->brush = NEW_ARRAY(uint8_t, tmp->w * tmp->h);
                paint->b_w = tmp->w;
                paint->b_h = tmp->h;

                SDL_LockSurface(tmp);

                for (int j = 0; j < paint->b_h; j++)
                    for (int i = 0; i < paint->b_w; i++)
                    {
                        color_t c = Rend_GetPixel(tmp, i, j);
                        paint->brush[i + j * tmp->w] = (c.r >= 0x7F && c.g >= 0x7F && c.b >= 0x7F);
                    }

                SDL_UnlockSurface(tmp);
                SDL_FreeSurface(tmp);
            }
        }
        else if (str_starts_with(str, "cursor"))
        {
            paint->cursor = Mouse_GetCursorIndex(GetParams(str));
        }
        else if (str_starts_with(str, "paint_file"))
        {
            strcpy(filename, GetParams(str));
        }
        else if (str_starts_with(str, "venus_id"))
        {
            ctnode->venus = atoi(GetParams(str));
        }
        else if (str_starts_with(str, "eligible_objects"))
        {
            str = GetParams(str);
            int tmpobj = 0;
            int strl = strlen(str);
            for (int i = 0; i < strl; i++)
                if (str[i] == ' ')
                    tmpobj++;

            tmpobj++;

            paint->eligable_cnt = tmpobj;
            paint->eligible_objects = NEW_ARRAY(int32_t, tmpobj);
            int i = 0;
            tmpobj = 0;

            for (;;)
            {
                if (i >= strl)
                    break;
                if (str[i] != ' ')
                {
                    paint->eligible_objects[tmpobj] = atoi(str + i);
                    tmpobj++;

                    while (i < strl && str[i] != ' ')
                        i++;
                }
                i++;
            }
        }
    }

    SDL_Surface *tmp = Loader_LoadGFX(filename, false, -1);
    if (!tmp)
        return 0;

    paint->paint = Rend_CreateSurface(paint->rectangle.w, paint->rectangle.h, 0);

    SDL_Rect rect = {
        paint->rectangle.x,
        paint->rectangle.y,
        paint->rectangle.w,
        paint->rectangle.h
    };
    Rend_BlitSurface(tmp, &rect, paint->paint, NULL);
    SDL_FreeSurface(tmp);

    return good;
}

static int Parse_Control_Slot(dynlist_t *controls, mfile_t *fl, uint32_t slot)
{
    ctrlnode_t *ctnode = Ctrl_CreateNode(CTRL_SLOT);
    slotnode_t *slotnode = ctnode->node.slot;
    char buf[STRBUFSIZE];
    int good = 0;

    AddToList(controls, ctnode);
    ctnode->slot = slot;
    slotnode->srf = NULL;

    while (!mfeof(fl))
    {
        mfgets(buf, STRBUFSIZE, fl);
        char *str = PrepareString(buf);

        if (str[0] == '}')
        {
            good = 1;
            break;
        }
        else if (str_starts_with(str, "rectangle"))
        {
            sscanf(GetParams(str), "%d %d %d %d",
                   &slotnode->rectangle.x,
                   &slotnode->rectangle.y,
                   &slotnode->rectangle.w,
                   &slotnode->rectangle.h);
        }
        else if (str_starts_with(str, "hotspot"))
        {
            sscanf(GetParams(str), "%d %d %d %d",
                   &slotnode->hotspot.x,
                   &slotnode->hotspot.y,
                   &slotnode->hotspot.w,
                   &slotnode->hotspot.h);
        }
        else if (str_starts_with(str, "cursor"))
        {
            slotnode->cursor = Mouse_GetCursorIndex(GetParams(str));
        }
        else if (str_starts_with(str, "distance_id"))
        {
            strcpy(slotnode->distance_id, GetParams(str));
        }
        else if (str_starts_with(str, "venus_id"))
        {
            ctnode->venus = atoi(GetParams(str));
        }
        else if (str_starts_with(str, "eligible_objects"))
        {
            str = GetParams(str);
            int tmpobj = 0;
            int strl = strlen(str);
            for (int i = 0; i < strl; i++)
                if (str[i] == ' ')
                    tmpobj++;

            tmpobj++;

            slotnode->eligable_cnt = tmpobj;
            slotnode->eligible_objects = NEW_ARRAY(int32_t, tmpobj);
            int i = 0;
            tmpobj = 0;

            for (;;)
            {
                if (i >= strl)
                    break;
                if (str[i] != ' ')
                {
                    slotnode->eligible_objects[tmpobj] = atoi(str + i);
                    tmpobj++;

                    while (i < strl && str[i] != ' ')
                        i++;
                }
                i++;
            }
        }
    }
    return good;
}

static int Parse_Control_PushTgl(dynlist_t *controls, mfile_t *fl, uint32_t slot)
{
    ctrlnode_t *ctnode = Ctrl_CreateNode(CTRL_PUSH);
    pushnode_t *psh = ctnode->node.push;
    char buf[STRBUFSIZE];
    char *str;
    int good = 0;

    //    SetgVarInt(slot,0);
    ctnode->slot = slot;
    psh->cursor = CURSOR_IDLE;

    while (!mfeof(fl))
    {
        mfgets(buf, STRBUFSIZE, fl);
        str = PrepareString(buf);

        if (str[0] == '}')
        {
            good = 1;
            break;
        }
        else if (str_starts_with(str, "flat_hotspot"))
        {
            psh->flat = true;
            str = GetParams(str);
            sscanf(str, "%d %d %d %d", &psh->x, &psh->y, &psh->w, &psh->h);
        }
        else if (str_starts_with(str, "warp_hotspot"))
        {
            psh->flat = true;
            str = GetParams(str);
            sscanf(str, "%d %d %d %d", &psh->x, &psh->y, &psh->w, &psh->h);
        }
        else if (str_starts_with(str, "cursor"))
        {
            psh->cursor = Mouse_GetCursorIndex(GetParams(str));
        }
        else if (str_starts_with(str, "animation"))
        {
        }
        else if (str_starts_with(str, "mouse_event"))
        {
            str = GetParams(str);
            if (str_starts_with(str, "up"))
                psh->event = CTRL_PUSH_EV_UP;
            else if (str_starts_with(str, "down"))
                psh->event = CTRL_PUSH_EV_DWN;
            else if (str_starts_with(str, "double"))
                psh->event = CTRL_PUSH_EV_DBL;
        }
        else if (str_starts_with(str, "sound"))
        {
        }
        else if (str_starts_with(str, "count_to"))
        {
            psh->count_to = atoi(GetParams(str)) + 1;
        }
        else if (str_starts_with(str, "venus_id"))
        {
            ctnode->venus = atoi(GetParams(str));
        }
    }

    if (good == 1)
        AddToList(controls, ctnode);

    return good;
}

static int Parse_Control_Fist(dynlist_t *controls, mfile_t *fl, uint32_t slot)
{
    ctrlnode_t *ctnode = Ctrl_CreateNode(CTRL_FIST);
    fistnode_t *fist = ctnode->node.fist;
    char filename[MINIBUFSIZE];
    char buf[STRBUFSIZE];
    char *str;
    int good = 0;

    ctnode->slot = slot;

    while (!mfeof(fl))
    {
        mfgets(buf, STRBUFSIZE, fl);
        str = PrepareString(buf);

        if (str[0] == '}')
        {
            good = 1;
            break;
        }
        else if (str_starts_with(str, "sound_key"))
        {
            fist->soundkey = atoi(GetParams(str));
        }
        else if (str_starts_with(str, "cursor"))
        {
            fist->cursor = Mouse_GetCursorIndex(GetParams(str));
        }
        else if (str_starts_with(str, "descfile"))
        {
            strcpy(filename, GetParams(str));
        }
        else if (str_starts_with(str, "animation_id"))
        {
            fist->animation_id = atoi(GetParams(str));
        }
        else if (str_starts_with(str, "venus_id"))
        {
            ctnode->venus = atoi(GetParams(str));
        }
    }

    mfile_t *mfp = mfopen_txt(filename);
    if (mfp)
    {
        int32_t t1, t2, t3, t4, t5, t6;
        char s1[MINIBUFSIZE];
        char s2[MINIBUFSIZE];

        while (!mfeof(mfp))
        {
            mfgets(buf, STRBUFSIZE, mfp);
            str = PrepareString(buf);

            size_t ln = strlen(str);
            if (str[ln - 1] == '~')
                str[ln - 1] = 0;

            if (str_starts_with(str, "animation_id"))
            {
                //sscanf(str,"animation_id:%d",);
            }
            else if (str_starts_with(str, "animation"))
            {
                sscanf(str, "animation:%s", s1);
                fist->anm = NEW(animnode_t);
                Anim_Load(fist->anm, s1, 0, 0, 0, 0);
            }
            else if (str_starts_with(str, "anim_rect"))
            {
                sscanf(str, "anim_rect:%d %d %d %d", &t1, &t2, &t3, &t4);
                fist->anm_rect.x = t1;
                fist->anm_rect.y = t2;
                fist->anm_rect.w = t3 - t1 + 1;
                fist->anm_rect.h = t4 - t2 + 1;
            }
            else if (str_starts_with(str, "num_fingers"))
            {
                sscanf(str, "num_fingers:%d", &t1);
                if (t1 >= 0 && t1 <= CTRL_FIST_MAX_FISTS)
                    fist->fistnum = t1;
            }
            else if (str_starts_with(str, "entries"))
            {
                sscanf(str, "entries:%d", &t1);
                if (t1 >= 0 && t1 <= CTRL_FIST_MAX_ENTRS)
                    fist->num_entries = t1;
            }
            else if (str_starts_with(str, "eval_order_ascending"))
            {
                sscanf(str, "eval_order_ascending:%d", &t1);
                fist->order = t1;
            }
            else if (str_starts_with(str, "up_hs_num_"))
            {
                sscanf(str, "up_hs_num_%d:%d", &t1, &t2);
                if (t1 >= 0 && t1 < fist->fistnum)
                    if (t2 >= 0 && t2 <= CTRL_FIST_MAX_BOXES)
                        fist->fists_up[t1].num_box = t2;
            }
            else if (str_starts_with(str, "up_hs_"))
            {
                sscanf(str, "up_hs_%d_%d:%d %d %d %d", &t1, &t2, &t3, &t4, &t5, &t6);
                if (t1 >= 0 && t1 < fist->fistnum)
                    if (t2 >= 0 && t2 < fist->fists_up[t1].num_box)
                    {
                        fist->fists_up[t1].boxes[t2].x = t3;
                        fist->fists_up[t1].boxes[t2].y = t4;
                        fist->fists_up[t1].boxes[t2].x2 = t5;
                        fist->fists_up[t1].boxes[t2].y2 = t6;
                    }
            }
            else if (str_starts_with(str, "down_hs_num_"))
            {
                sscanf(str, "down_hs_num_%d:%d", &t1, &t2);
                if (t1 >= 0 && t1 < fist->fistnum)
                    if (t2 >= 0 && t2 <= CTRL_FIST_MAX_BOXES)
                        fist->fists_dwn[t1].num_box = t2;
            }
            else if (str_starts_with(str, "down_hs_"))
            {
                sscanf(str, "down_hs_%d_%d:%d %d %d %d", &t1, &t2, &t3, &t4, &t5, &t6);
                if (t1 >= 0 && t1 < fist->fistnum)
                    if (t2 >= 0 && t2 < fist->fists_dwn[t1].num_box)
                    {
                        fist->fists_dwn[t1].boxes[t2].x = t3;
                        fist->fists_dwn[t1].boxes[t2].y = t4;
                        fist->fists_dwn[t1].boxes[t2].x2 = t5;
                        fist->fists_dwn[t1].boxes[t2].y2 = t6;
                    }
            }
            else
            {
                if (sscanf(str, "%d:%s %s %d %d (%d)", &t1, s1, s2, &t2, &t3, &t4) == 6)
                {
                    if (t1 >= 0 && t1 < fist->num_entries)
                    {
                        size_t n1 = 0;
                        size_t len = strlen(s1);
                        for (size_t i = 0; i < len; i++)
                            if (s1[i] != '0')
                                n1 |= (1 << i);

                        size_t n2 = 0;
                        size_t tmp_len = strlen(s2);
                        for (size_t i = 0; i < tmp_len; i++)
                            if (s2[i] != '0')
                                n2 |= (1 << i);

                        fist->entries[t1].strt = n1;
                        fist->entries[t1].send = n2;
                        fist->entries[t1].anm_str = t2;
                        fist->entries[t1].anm_end = t3;
                        fist->entries[t1].sound = t4;
                    }
                }
            }
        }
        mfclose(mfp);
    }
    else
        good = 0;

    if (good == 1)
        AddToList(controls, ctnode);

    return good;
}

static int Parse_Control_Safe(dynlist_t *controls, mfile_t *fl, uint32_t slot)
{
    ctrlnode_t *ctnode = Ctrl_CreateNode(CTRL_SAFE);
    safenode_t *safe = ctnode->node.safe;
    char buf[STRBUFSIZE];
    char *str;
    int good = 0;

    ctnode->slot = slot;

    while (!mfeof(fl))
    {
        mfgets(buf, STRBUFSIZE, fl);
        str = PrepareString(buf);

        if (str[0] == '}')
        {
            good = 1;
            break;
        }
        else if (str_starts_with(str, "animation"))
        {
            safe->anm = NEW(animnode_t);
            Anim_Load(safe->anm, GetParams(str), 0, 0, 0, 0);
        }
        else if (str_starts_with(str, "rectangle"))
        {
            sscanf(GetParams(str), "%d %d %d %d",
                &safe->rectangle.x, &safe->rectangle.y, &safe->rectangle.w, &safe->rectangle.h);
            safe->rectangle.w -= (safe->rectangle.x - 1);
            safe->rectangle.h -= (safe->rectangle.y - 1);
        }
        else if (str_starts_with(str, "center"))
        {
            sscanf(GetParams(str), "%d %d", &safe->center_x, &safe->center_y);
        }
        else if (str_starts_with(str, "num_states"))
        {
            safe->num_states = atoi(GetParams(str));
        }
        else if (str_starts_with(str, "dial_inner_radius"))
        {
            safe->radius_inner = atoi(GetParams(str));
            safe->radius_inner_sq = safe->radius_inner * safe->radius_inner;
        }
        else if (str_starts_with(str, "radius"))
        {
            safe->radius_outer = atoi(GetParams(str));
            safe->radius_outer_sq = safe->radius_outer * safe->radius_outer;
        }
        else if (str_starts_with(str, "zero_radians_offset"))
        {
            safe->zero_pointer = atoi(GetParams(str));
        }
        else if (str_starts_with(str, "pointer_offset"))
        {
            safe->start_pointer = atoi(GetParams(str));
            safe->cur_state = safe->start_pointer;
        }
        else if (str_starts_with(str, "cursor"))
        {
            //not needed
        }
        else if (str_starts_with(str, "mirrored"))
        {
            //not needed
        }
        else if (str_starts_with(str, "venus_id"))
        {
            ctnode->venus = atoi(GetParams(str));
        }
    }

    if (good == 1)
        AddToList(controls, ctnode);

    return good;
}

static void ctrl_Delete_PushNode(ctrlnode_t *nod)
{
    DELETE(nod->node.push);
}

static void ctrl_Delete_SlotNode(ctrlnode_t *nod)
{
    if (nod->node.slot->srf)
        SDL_FreeSurface(nod->node.slot->srf);
    if (nod->node.slot->eligible_objects)
        DELETE(nod->node.slot->eligible_objects);
    DELETE(nod->node.slot);
}

static void ctrl_Delete_InputNode(ctrlnode_t *nod)
{
    if (nod->node.inp->cursor)
        Anim_DeleteAnimImage(nod->node.inp->cursor);
    if (nod->node.inp->rect)
        SDL_FreeSurface(nod->node.inp->rect);
    DELETE(nod->node.inp);
}

static void ctrl_Delete_SaveNode(ctrlnode_t *nod)
{
    DELETE(nod->node.svld);
}

static void ctrl_Delete_LeverNode(ctrlnode_t *nod)
{
    if (nod->node.lev->anm != NULL)
        Anim_DeleteAnim(nod->node.lev->anm);
    DELETE(nod->node.lev);
}

static void ctrl_Delete_SafeNode(ctrlnode_t *nod)
{
    if (nod->node.safe->anm != NULL)
        Anim_DeleteAnim(nod->node.safe->anm);
    DELETE(nod->node.safe);
}

static void ctrl_Delete_FistNode(ctrlnode_t *nod)
{
    if (nod->node.fist->anm != NULL)
        Anim_DeleteAnim(nod->node.fist->anm);
    DELETE(nod->node.fist);
}

static void ctrl_Delete_HotmovNode(ctrlnode_t *nod)
{
    if (nod->node.hotmv->anm != NULL)
        Anim_DeleteAnim(nod->node.hotmv->anm);
    if (nod->node.hotmv->frame_list != NULL)
        DELETE(nod->node.hotmv->frame_list);
    DELETE(nod->node.hotmv);
}

static void ctrl_Delete_PaintNode(ctrlnode_t *nod)
{
    if (nod->node.paint->brush != NULL)
        DELETE(nod->node.paint->brush);
    if (nod->node.paint->eligible_objects != NULL)
        DELETE(nod->node.paint->eligible_objects);
    if (nod->node.paint->paint != NULL)
        SDL_FreeSurface(nod->node.paint->paint);
    DELETE(nod->node.paint);
}

static void ctrl_Delete_TitlerNode(ctrlnode_t *nod)
{
    if (nod->node.titler->surface != NULL)
        SDL_FreeSurface(nod->node.titler->surface);
    for (int i = 0; i < CTRL_TITLER_MAX_STRINGS; i++)
        if (nod->node.titler->strings[i] != NULL)
            DELETE(nod->node.titler->strings[i]);
    DELETE(nod->node.titler);
}

void Controls_Draw()
{
    dynlist_t *list = GetControlsList();
    for (int i = 0; i < list->length; i++)
    {
        ctrlnode_t *nod = (ctrlnode_t *)list->items[i];
        if (!nod) continue;

        if (!(ScrSys_GetFlag(nod->slot) & FLAG_DISABLED))
        {
            if (nod->type == CTRL_SLOT)
                control_slot_draw(nod);
            else if (nod->type == CTRL_INPUT)
                control_input_draw(nod);
            else if (nod->type == CTRL_LEVER)
                control_lever_draw(nod);
            else if (nod->type == CTRL_SAFE)
                control_safe_draw(nod);
            else if (nod->type == CTRL_HOTMV)
                control_hotmv_draw(nod);
            else if (nod->type == CTRL_FIST)
                control_fist_draw(nod);
            else if (nod->type == CTRL_TITLER)
                control_titler_draw(nod);
        }
    }
}

void Control_Parse(dynlist_t *controls, mfile_t *fl, char *ctstr)
{
    char type[100];
    int slot;

    if (sscanf(ctstr, "control:%d %s", &slot, type) == 2)
    {
        LOG_DEBUG("Creating control:%d %s\n", slot, type);

        if (str_equals(type, "flat"))             Parse_Control_Flat();
        else if (str_equals(type, "pana"))        Parse_Control_Panorama(fl);
        else if (str_equals(type, "tilt"))        Parse_Control_Tilt(fl);
        else if (str_equals(type, "push_toggle")) Parse_Control_PushTgl(controls, fl, slot);
        else if (str_equals(type, "input"))       Parse_Control_Input(controls, fl, slot);
        else if (str_equals(type, "save"))        Parse_Control_Save(controls, fl, slot);
        else if (str_equals(type, "slot"))        Parse_Control_Slot(controls, fl, slot);
        else if (str_equals(type, "lever"))       Parse_Control_Lever(controls, fl, slot);
        else if (str_equals(type, "safe"))        Parse_Control_Safe(controls, fl, slot);
        else if (str_equals(type, "fist"))        Parse_Control_Fist(controls, fl, slot);
        else if (str_equals(type, "hotmovie"))    Parse_Control_HotMov(controls, fl, slot);
        else if (str_equals(type, "paint"))       Parse_Control_Paint(controls, fl, slot);
        else if (str_equals(type, "titler"))      Parse_Control_Titler(controls, fl, slot);
    }
}

void Controls_ProcessList(dynlist_t *list)
{
    pushChangeMouse = false;

    for (int i = list->length - 1; i >= 0 ; i--)
    {
        ctrlnode_t *nod = (ctrlnode_t *)list->items[i];
        if (!nod) continue;

        LOG_DEBUG("Running control %d (%d)\n", nod->slot, i);

        if (!(ScrSys_GetFlag(nod->slot) & FLAG_DISABLED)) //(nod->enable)
            if (nod->func != NULL)
                nod->func(nod);
    }
}

void Controls_FlushList(dynlist_t *list)
{
    for (int i = 0; i < list->length; i++)
    {
        ctrlnode_t *nod = (ctrlnode_t *)list->items[i];
        if (!nod) continue;

        switch (nod->type)
        {
        case CTRL_PUSH:
            ctrl_Delete_PushNode(nod);
            break;
        case CTRL_SLOT:
            ctrl_Delete_SlotNode(nod);
            break;
        case CTRL_INPUT:
            ctrl_Delete_InputNode(nod);
            break;
        case CTRL_SAVE:
            ctrl_Delete_SaveNode(nod);
            break;
        case CTRL_LEVER:
            ctrl_Delete_LeverNode(nod);
            break;
        case CTRL_SAFE:
            ctrl_Delete_SafeNode(nod);
            break;
        case CTRL_HOTMV:
            ctrl_Delete_HotmovNode(nod);
            break;
        case CTRL_FIST:
            ctrl_Delete_FistNode(nod);
            break;
        case CTRL_PAINT:
            ctrl_Delete_PaintNode(nod);
            break;
        case CTRL_TITLER:
            ctrl_Delete_TitlerNode(nod);
            break;
        }
        DeleteFromList(list, i);
        DELETE(nod);
    }

    FlushList(list);
}

ctrlnode_t *Controls_GetControl(int id)
{
    dynlist_t *list = GetControlsList();

    for (int i = 0; i < list->length; i++)
    {
        ctrlnode_t *nod = (ctrlnode_t *)list->items[i];
        if (nod && nod->slot == id)
            return nod;
    }

    return NULL;
}
