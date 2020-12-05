#include "Utilities.h"
#include "Scripting.h"
#include "Inventory.h"
#include "Loader.h"
#include "Render.h"
#include "Mouse.h"
#include "Game.h"
#include "Menu.h"

#define menu_ITEM 0
#define menu_MAGIC 1
#define menu_MAIN 2

#define menu_MAIN_IMAGE_SAVE 0
#define menu_MAIN_IMAGE_REST 1
#define menu_MAIN_IMAGE_PREF 2
#define menu_MAIN_IMAGE_EXIT 3

#define menu_MAIN_EL_W 135
#define menu_MAIN_EL_H 32

#define menu_MAGIC_SPACE 28
#define menu_MAGIC_ITEM_W 47

#define menu_ITEM_SPACE 28

#define menu_SCROLL_TIME_INV 1.0
#define menu_SCROLL_TIME 0.5

#define menu_MAIN_CENTER (WINDOW_W >> 1)

#define menu_zgi_inv_hot_w 28
#define menu_zgi_inv_h 32
#define menu_zgi_inv_w 600

#define menu_znemesis_butanim 200
#define menu_znemesis_but_frames 6
#define menu_znemesis_but_max_frm 4
#define menu_znemesis_but_clk_frm 5

#define znem_but1_w 120
#define znem_but1_x 0
#define znem_but2_w 144
#define znem_but2_x 120
#define znem_but3_w 128
#define znem_but3_x 264
#define znem_but4_w 120
#define znem_but4_x 392

static uint16_t menu_bar_flag = 0xFFFF;
static bool menu_Scrolled[3] = {false, false, false};
static float menu_ScrollPos[3] = {0, 0, 0};
static int menu_mousefocus = -1;
static int mouse_on_item = -1;
static bool inmenu = false;

static SDL_Surface *menubar[4][2];
static SDL_Surface *menuback[3][2];
static SDL_Surface *menupicto[256][2];
static SDL_Surface *menubar_znem;
static SDL_Surface *menubut_znem[4][menu_znemesis_but_frames];

static float scrollpos_znem;
static bool scrolled_znem;

static int16_t butframe_znem[4];
static int16_t lastbut_znem = -1;

static int32_t znem_but_anim = menu_znemesis_butanim;

#define BlitSurfaceToScreen(surf, x, y) Rend_BlitSurfaceXY(surf, Rend_GetScreen(), x, y)

void Menu_SetVal(uint16_t val)
{
    menu_bar_flag = val;
}

uint16_t Menu_GetVal()
{
    return menu_bar_flag;
}

void Menu_Init()
{
    char buf[MINIBUFSIZE];

    if (CURRENT_GAME == GAME_ZGI)
    {
        for (int i = 1; i < 4; i++)
        {
            sprintf(buf, "gmzau%2.2x1.tga", i);
            menuback[i - 1][0] = Loader_LoadGFX(buf, false, -1);
            sprintf(buf, "gmzau%2.2x1.tga", i + 0x10);
            menuback[i - 1][1] = Loader_LoadGFX(buf, false, -1);
        }
        for (int i = 0; i < 4; i++)
        {
            sprintf(buf, "gmzmu%2.2x1.tga", i);
            menubar[i][0] = Loader_LoadGFX(buf, false, -1);
            sprintf(buf, "gmznu%2.2x1.tga", i);
            menubar[i][1] = Loader_LoadGFX(buf, false, -1);
        }

        memset(menupicto, 0, sizeof(menupicto));
    }
    else
    {
        for (int j = 1; j <= 4; j++)
            for (int i = 0; i < menu_znemesis_but_frames; i++)
            {
                sprintf(buf, "butfrm%d%d.tga", j, i);
                menubut_znem[j - 1][i] = Loader_LoadGFX(buf, false, -1);
            }
        menubar_znem = Loader_LoadGFX("bar.tga", false, -1);
    }
}

void Menu_Update()
{
    if (CURRENT_GAME == GAME_ZGI)
    {
        int menu_MAIN_X = ((WINDOW_W - 580) >> 1);
        mouse_on_item = -1;
        inmenu = (MouseY() <= 40);

        if (inmenu)
        {
            switch (menu_mousefocus)
            {
            case menu_ITEM:
                if (menu_bar_flag & MENU_BAR_ITEM)
                {
                    SetgVarInt(SLOT_MENU_STATE, 1);

                    if (!menu_Scrolled[menu_ITEM])
                    {
                        float scrl = (menu_zgi_inv_w / Game_GetFps()) / menu_SCROLL_TIME_INV;

                        if (scrl == 0)
                            scrl = 1.0;

                        menu_ScrollPos[menu_ITEM] += scrl;
                    }

                    if (menu_ScrollPos[menu_ITEM] >= 0)
                    {
                        menu_Scrolled[menu_ITEM] = true;
                        menu_ScrollPos[menu_ITEM] = 0;
                    }

                    int item_count = GetgVarInt(SLOT_TOTAL_INV_AVAIL);
                    if (item_count == 0)
                        item_count = 20;

                    for (int i = 0; i < item_count; i++)
                    {
                        int itemspace = (menu_zgi_inv_w - menu_ITEM_SPACE) / item_count;

                        if (Mouse_InRect(menu_ScrollPos[menu_ITEM] + itemspace * i, 0, menu_zgi_inv_hot_w, menu_zgi_inv_h))
                        {
                            mouse_on_item = i;

                            if (MouseUp(MOUSE_BTN_LEFT))
                            {
                                int32_t mouse_item = GetgVarInt(SLOT_INVENTORY_MOUSE);
                                if (mouse_item >= 0 && mouse_item < 0xE0)
                                {
                                    Inventory_Drop(mouse_item);
                                    Inventory_Add(GetgVarInt(SLOT_START_SLOT + i));
                                    SetgVarInt(SLOT_START_SLOT + i, mouse_item);
                                }
                            }
                        }
                    }
                }
                break;

            case menu_MAGIC:
                if (menu_bar_flag & MENU_BAR_MAGIC)
                {
                    SetgVarInt(SLOT_MENU_STATE, 3);

                    if (!menu_Scrolled[menu_MAGIC])
                    {
                        float scrl = (menu_zgi_inv_w / Game_GetFps()) / menu_SCROLL_TIME_INV;

                        if (scrl == 0)
                            scrl = 1.0;

                        menu_ScrollPos[menu_MAGIC] += scrl;
                    }

                    if (menu_ScrollPos[menu_MAGIC] >= menuback[menu_MAGIC][0]->w)
                    {
                        menu_Scrolled[menu_MAGIC] = true;
                        menu_ScrollPos[menu_MAGIC] = menuback[menu_MAGIC][0]->w;
                    }

                    for (int i = 0; i < 12; i++)
                    {

                        int itemnum;
                        if (GetgVarInt(SLOT_REVERSED_SPELLBOOK) == 1)
                            itemnum = 0xEE + i;
                        else
                            itemnum = 0xE0 + i;

                        if (Mouse_InRect(WINDOW_W + menu_MAGIC_SPACE + menu_MAGIC_ITEM_W * i - menu_ScrollPos[menu_MAGIC], 0, menu_zgi_inv_hot_w, menu_zgi_inv_h))
                        {
                            mouse_on_item = i;
                            if (MouseUp(MOUSE_BTN_LEFT))
                                if (GetgVarInt(SLOT_INVENTORY_MOUSE) == 0 || GetgVarInt(SLOT_INVENTORY_MOUSE) >= 0xE0)
                                    if (GetgVarInt(SLOT_SPELL_1 + i) != 0)
                                        SetgVarInt(SLOT_USER_CHOSE_THIS_SPELL, itemnum);
                        }
                    }
                }
                break;

            case menu_MAIN:
                SetgVarInt(SLOT_MENU_STATE, 2);

                if (!menu_Scrolled[menu_MAIN])
                {
                    float scrl = (menu_MAIN_EL_H / Game_GetFps()) / menu_SCROLL_TIME;

                    if (scrl == 0)
                        scrl = 1.0;

                    menu_ScrollPos[menu_MAIN] += scrl;
                }

                if (menu_ScrollPos[menu_MAIN] >= 0)
                {
                    menu_Scrolled[menu_MAIN] = true;
                    menu_ScrollPos[menu_MAIN] = 0;
                }

                //EXIT
                if (menu_bar_flag & MENU_BAR_EXIT)
                    if (Mouse_InRect(menu_MAIN_CENTER + menu_MAIN_EL_W,
                                     menu_ScrollPos[menu_MAIN],
                                     menu_MAIN_EL_W,
                                     menu_MAIN_EL_H))
                    {
                        mouse_on_item = menu_MAIN_IMAGE_EXIT;
                        if (MouseUp(MOUSE_BTN_LEFT))
                            game_try_quit();
                    }

                //SETTINGS
                if (menu_bar_flag & MENU_BAR_SETTINGS)
                    if (Mouse_InRect(menu_MAIN_CENTER,
                                     menu_ScrollPos[menu_MAIN],
                                     menu_MAIN_EL_W,
                                     menu_MAIN_EL_H))
                    {
                        mouse_on_item = menu_MAIN_IMAGE_PREF;
                        if (MouseUp(MOUSE_BTN_LEFT))
                            Game_Relocate(PrefWorld, PrefRoom, PrefNode, PrefView, 0);
                    }

                //LOAD
                if (menu_bar_flag & MENU_BAR_RESTORE)
                    if (Mouse_InRect(menu_MAIN_CENTER - menu_MAIN_EL_W,
                                     menu_ScrollPos[menu_MAIN],
                                     menu_MAIN_EL_W,
                                     menu_MAIN_EL_H))
                    {
                        mouse_on_item = menu_MAIN_IMAGE_REST;
                        if (MouseUp(MOUSE_BTN_LEFT))
                            Game_Relocate(LoadWorld, LoadRoom, LoadNode, LoadView, 0);
                    }

                //SAVE
                if (menu_bar_flag & MENU_BAR_SAVE)
                    if (Mouse_InRect(menu_MAIN_CENTER - menu_MAIN_EL_W * 2,
                                     menu_ScrollPos[menu_MAIN],
                                     menu_MAIN_EL_W,
                                     menu_MAIN_EL_H))
                    {
                        mouse_on_item = menu_MAIN_IMAGE_SAVE;
                        if (MouseUp(MOUSE_BTN_LEFT))
                            Game_Relocate(SaveWorld, SaveRoom, SaveNode, SaveView, 0);
                    }
                break;

            default:
                if (Mouse_InRect(menu_MAIN_X, 0, menuback[menu_MAIN][1]->w, 8))
                {
                    menu_mousefocus = menu_MAIN;
                    menu_Scrolled[menu_MAIN] = false;
                    menu_ScrollPos[menu_MAIN] = menuback[menu_MAIN][1]->h - menuback[menu_MAIN][0]->h;
                }

                if (menu_bar_flag & MENU_BAR_MAGIC)
                    if (Mouse_InRect(WINDOW_W - menu_zgi_inv_hot_w, 0, menu_zgi_inv_hot_w, menu_zgi_inv_h))
                    {
                        menu_mousefocus = menu_MAGIC;
                        menu_Scrolled[menu_MAGIC] = false;
                        menu_ScrollPos[menu_MAGIC] = menu_zgi_inv_hot_w;
                    }

                if (menu_bar_flag & MENU_BAR_ITEM)
                    if (Mouse_InRect(0, 0, menu_zgi_inv_hot_w, menu_zgi_inv_h))
                    {
                        menu_mousefocus = menu_ITEM;
                        menu_Scrolled[menu_ITEM] = false;
                        menu_ScrollPos[menu_ITEM] = menu_zgi_inv_hot_w - menu_zgi_inv_w;
                    }
            }
        }
        else
        {
            SetDirectgVarInt(SLOT_MENU_STATE, 0);
            menu_mousefocus = -1;
        }
    }
    else if (CURRENT_GAME == GAME_ZNEM)
    {
        int menu_MAIN_X = ((WINDOW_W - 512) >> 1);
        mouse_on_item = -1;
        inmenu = (MouseY() <= 40);

        if (inmenu)
        {
            SetgVarInt(SLOT_MENU_STATE, 2);

            if (!scrolled_znem)
            {
                float scrl = (menubar_znem->h / Game_GetFps()) / menu_SCROLL_TIME;

                if (scrl == 0)
                    scrl = 1.0;

                scrollpos_znem += scrl;
            }

            if (scrollpos_znem >= 0)
            {
                scrolled_znem = true;
                scrollpos_znem = 0;
            }

            //EXIT
            if (menu_bar_flag & MENU_BAR_EXIT)
                if (Mouse_InRect(menu_MAIN_X + znem_but4_x,
                                 menu_ScrollPos[menu_MAIN],
                                 znem_but4_w,
                                 menu_MAIN_EL_H))
                {
                    mouse_on_item = menu_MAIN_IMAGE_EXIT;
                    if (MouseUp(MOUSE_BTN_LEFT))
                    {
                        butframe_znem[menu_MAIN_IMAGE_EXIT] = menu_znemesis_but_clk_frm;
                        game_try_quit();
                    }
                }

            //SETTINGS
            if (menu_bar_flag & MENU_BAR_SETTINGS)
                if (Mouse_InRect(menu_MAIN_X + znem_but3_x,
                                 menu_ScrollPos[menu_MAIN],
                                 znem_but3_w,
                                 menu_MAIN_EL_H))
                {
                    mouse_on_item = menu_MAIN_IMAGE_PREF;
                    if (MouseUp(MOUSE_BTN_LEFT))
                    {
                        butframe_znem[menu_MAIN_IMAGE_PREF] = menu_znemesis_but_clk_frm;
                        Game_Relocate(PrefWorld, PrefRoom, PrefNode, PrefView, 0);
                    }
                }

            //LOAD
            if (menu_bar_flag & MENU_BAR_RESTORE)
                if (Mouse_InRect(menu_MAIN_X + znem_but2_x,
                                 menu_ScrollPos[menu_MAIN],
                                 znem_but2_w,
                                 menu_MAIN_EL_H))
                {
                    mouse_on_item = menu_MAIN_IMAGE_REST;
                    if (MouseUp(MOUSE_BTN_LEFT))
                    {
                        butframe_znem[menu_MAIN_IMAGE_REST] = menu_znemesis_but_clk_frm;
                        Game_Relocate(LoadWorld, LoadRoom, LoadNode, LoadView, 0);
                    }
                }

            //SAVE
            if (menu_bar_flag & MENU_BAR_SAVE)
                if (Mouse_InRect(menu_MAIN_X + znem_but1_x,
                                 menu_ScrollPos[menu_MAIN],
                                 znem_but1_w,
                                 menu_MAIN_EL_H))
                {
                    mouse_on_item = menu_MAIN_IMAGE_SAVE;
                    if (MouseUp(MOUSE_BTN_LEFT))
                    {
                        butframe_znem[menu_MAIN_IMAGE_SAVE] = menu_znemesis_but_clk_frm;
                        Game_Relocate(SaveWorld, SaveRoom, SaveNode, SaveView, 0);
                    }
                }

            if (lastbut_znem != mouse_on_item && mouse_on_item != -1)
            {
                butframe_znem[mouse_on_item] = 0;
                znem_but_anim = 0;
            }
            else if (mouse_on_item != -1)
            {
                if (butframe_znem[mouse_on_item] < menu_znemesis_but_max_frm)
                {
                    znem_but_anim -= Game_GetDTime();

                    if (znem_but_anim < 0)
                    {
                        znem_but_anim = menu_znemesis_butanim;
                        butframe_znem[mouse_on_item]++;
                    }
                }
            }

            lastbut_znem = mouse_on_item;
        }
        else
        {
            SetDirectgVarInt(SLOT_MENU_STATE, 0);
            menu_mousefocus = -1;
            scrolled_znem = false;
            lastbut_znem = -1;
            mouse_on_item = -1;

            if (scrollpos_znem > -menubar_znem->h)
            {
                float scrl = (menubar_znem->h / Game_GetFps()) / menu_SCROLL_TIME;

                if (scrl == 0)
                    scrl = 1.0;

                scrollpos_znem -= scrl;
            }
            else
                scrollpos_znem = -menubar_znem->h;
        }
    }
}

void Menu_Draw()
{
    // Clear the menu area
    SDL_Rect menu_rect = {0, 0, WINDOW_W, GAMESCREEN_Y};
    Rend_FillRect(Rend_GetScreen(), &menu_rect, 0, 0, 0);

    // Draw the menu
    if (CURRENT_GAME == GAME_ZGI)
    {
        if (!inmenu)
            return;

        int menu_MAIN_X = ((WINDOW_W - 580) >> 1);
        char buf[MINIBUFSIZE];

        if ((menu_mousefocus == menu_ITEM) && (menu_bar_flag & MENU_BAR_ITEM))
        {
            BlitSurfaceToScreen(menuback[menu_ITEM][0], menu_ScrollPos[menu_ITEM], 0);

            int item_count = GetgVarInt(SLOT_TOTAL_INV_AVAIL);
            if (item_count == 0)
                item_count = 20;

            for (int i = 0; i < item_count; i++)
            {
                int itemspace = (menu_zgi_inv_w - menu_ITEM_SPACE) / item_count;
                bool inrect = (mouse_on_item == i);

                if (GetgVarInt(SLOT_START_SLOT + i) != 0)
                {
                    int itemnum = GetgVarInt(SLOT_START_SLOT + i);

                    if (menupicto[itemnum][0] == NULL)
                    {
                        sprintf(buf, "gmzwu%2.2x1.tga", itemnum);
                        menupicto[itemnum][0] = Loader_LoadGFX(buf, 0, 0);
                    }
                    if (menupicto[itemnum][1] == NULL)
                    {
                        sprintf(buf, "gmzxu%2.2x1.tga", itemnum);
                        menupicto[itemnum][1] = Loader_LoadGFX(buf, 0, 0);
                    }
                    BlitSurfaceToScreen(
                        menupicto[itemnum][inrect],
                        menu_ScrollPos[menu_ITEM] + itemspace * i,
                        0);
                }
            }
        }
        else if ((menu_mousefocus == menu_MAGIC) && (menu_bar_flag & MENU_BAR_MAGIC))
        {
            BlitSurfaceToScreen(menuback[menu_MAGIC][0], WINDOW_W - menu_ScrollPos[menu_MAGIC], 0);

            for (int i = 0; i < 12; i++)
            {
                int itemnum;
                if (GetgVarInt(SLOT_REVERSED_SPELLBOOK) == 1)
                    itemnum = 0xEE + i;
                else
                    itemnum = 0xE0 + i;

                bool inrect = (mouse_on_item == i);

                if (GetgVarInt(SLOT_SPELL_1 + i) != 0)
                {
                    if (menupicto[itemnum][0] == NULL)
                    {
                        sprintf(buf, "gmzwu%2.2x1.tga", itemnum);
                        menupicto[itemnum][0] = Loader_LoadGFX(buf, false, 0);
                    }
                    if (menupicto[itemnum][1] == NULL)
                    {
                        sprintf(buf, "gmzxu%2.2x1.tga", itemnum);
                        menupicto[itemnum][1] = Loader_LoadGFX(buf, false, 0);
                    }
                    BlitSurfaceToScreen(
                        menupicto[itemnum][inrect],
                        WINDOW_W + menu_MAGIC_SPACE + menu_MAGIC_ITEM_W * i - menu_ScrollPos[menu_MAGIC],
                        0);
                }
            }
        }
        else if (menu_mousefocus == menu_MAIN)
        {
            BlitSurfaceToScreen(menuback[menu_MAIN][0], menu_MAIN_X, menu_ScrollPos[menu_MAIN]);

            if (menu_bar_flag & MENU_BAR_EXIT)
            {
                bool hover = mouse_on_item == menu_MAIN_IMAGE_EXIT;

                BlitSurfaceToScreen(
                    menubar[menu_MAIN_IMAGE_EXIT][hover],
                    menu_MAIN_CENTER + menu_MAIN_EL_W,
                    menu_ScrollPos[menu_MAIN]);
            }

            if (menu_bar_flag & MENU_BAR_SETTINGS)
            {
                bool hover = mouse_on_item == menu_MAIN_IMAGE_PREF;

                BlitSurfaceToScreen(
                    menubar[menu_MAIN_IMAGE_PREF][hover],
                    menu_MAIN_CENTER,
                    menu_ScrollPos[menu_MAIN]);
            }

            if (menu_bar_flag & MENU_BAR_RESTORE)
            {
                bool hover = mouse_on_item == menu_MAIN_IMAGE_REST;

                BlitSurfaceToScreen(
                    menubar[menu_MAIN_IMAGE_REST][hover],
                    menu_MAIN_CENTER - menu_MAIN_EL_W,
                    menu_ScrollPos[menu_MAIN]);
            }

            if (menu_bar_flag & MENU_BAR_SAVE)
            {
                bool hover = mouse_on_item == menu_MAIN_IMAGE_SAVE;

                BlitSurfaceToScreen(
                    menubar[menu_MAIN_IMAGE_SAVE][hover],
                    menu_MAIN_CENTER - menu_MAIN_EL_W * 2,
                    menu_ScrollPos[menu_MAIN]);
            }
        }
        else
        {
            BlitSurfaceToScreen(menuback[menu_MAIN][1], menu_MAIN_X, 0);

            if (menu_bar_flag & MENU_BAR_ITEM)
                BlitSurfaceToScreen(menuback[menu_ITEM][1], 0, 0);

            if (menu_bar_flag & MENU_BAR_MAGIC)
                BlitSurfaceToScreen(menuback[menu_MAGIC][1], WINDOW_W - menu_zgi_inv_hot_w, 0);
        }
    }
    else
    {
        int menu_MAIN_X = ((WINDOW_W - 512) >> 1);
        if (inmenu)
        {
            BlitSurfaceToScreen(menubar_znem, menu_MAIN_X, scrollpos_znem);

            if (menu_bar_flag & MENU_BAR_EXIT)
            {
                if (mouse_on_item == menu_MAIN_IMAGE_EXIT)
                    BlitSurfaceToScreen(menubut_znem[menu_MAIN_IMAGE_EXIT][butframe_znem[menu_MAIN_IMAGE_EXIT]], menu_MAIN_X + znem_but4_x,
                              scrollpos_znem);
            }

            if (menu_bar_flag & MENU_BAR_SETTINGS)
            {
                if (mouse_on_item == menu_MAIN_IMAGE_PREF)
                    BlitSurfaceToScreen(menubut_znem[menu_MAIN_IMAGE_PREF][butframe_znem[menu_MAIN_IMAGE_PREF]], menu_MAIN_X + znem_but3_x,
                              scrollpos_znem);
            }

            if (menu_bar_flag & MENU_BAR_RESTORE)
            {
                if (mouse_on_item == menu_MAIN_IMAGE_REST)
                    BlitSurfaceToScreen(menubut_znem[menu_MAIN_IMAGE_REST][butframe_znem[menu_MAIN_IMAGE_REST]], menu_MAIN_X + znem_but2_x,
                              scrollpos_znem);
            }

            if (menu_bar_flag & MENU_BAR_SAVE)
            {
                if (mouse_on_item == menu_MAIN_IMAGE_SAVE)
                    BlitSurfaceToScreen(menubut_znem[menu_MAIN_IMAGE_SAVE][butframe_znem[menu_MAIN_IMAGE_SAVE]], menu_MAIN_X + znem_but1_x,
                              scrollpos_znem);
            }
        }
        else if (scrollpos_znem > -menubar_znem->h)
            BlitSurfaceToScreen(menubar_znem, menu_MAIN_X, scrollpos_znem);
    }
}
