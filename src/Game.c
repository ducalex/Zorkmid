#include "Game.h"

static Location_t Need_Locate;
static bool NeedToLoadScript = false;
static int8_t NeedToLoadScriptDelay = CHANGELOCATIONDELAY;

void SetNeedLocate(uint8_t w, uint8_t r, uint8_t v1, uint8_t v2, int32_t X)
{
    NeedToLoadScript = true;
    Need_Locate.World = tolower(w);
    Need_Locate.Room = tolower(r);
    Need_Locate.Node = tolower(v1);
    Need_Locate.View = tolower(v2);
    Need_Locate.X = X;

    if (Need_Locate.World == '0')
    {
        if (GetgVarInt(SLOT_WORLD) == tolower(SystemWorld) &&
            GetgVarInt(SLOT_ROOM) == tolower(SystemRoom))
        {
            Need_Locate.World = GetgVarInt(SLOT_MENU_LASTWORLD);
            Need_Locate.Room = GetgVarInt(SLOT_MENU_LASTROOM);
            Need_Locate.Node = GetgVarInt(SLOT_MENU_LASTNODE);
            Need_Locate.View = GetgVarInt(SLOT_MENU_LASTVIEW);
            Need_Locate.X = GetgVarInt(SLOT_MENU_LASTVIEW_POS);
        }
        else
        {
            Need_Locate.World = GetgVarInt(SLOT_LASTWORLD);
            Need_Locate.Room = GetgVarInt(SLOT_LASTROOM);
            Need_Locate.Node = GetgVarInt(SLOT_LASTNODE);
            Need_Locate.View = GetgVarInt(SLOT_LASTVIEW);
            Need_Locate.X = GetgVarInt(SLOT_LASTVIEW_POS);
        }
    }
}

void GameInit()
{
    InitScriptsEngine();
    LoadScriptFile(GetUni(), FindInBinTree("universe.scr"), false, NULL);

    ScrSys_ChangeLocation(InitWorld, InitRoom, InitNode, InitView, 0, true);

    TimerInit(35.0);

    //Hack
    SetDirectgVarInt(SLOT_LASTWORLD, InitWorld);
    SetDirectgVarInt(SLOT_LASTROOM, InitRoom);
    SetDirectgVarInt(SLOT_LASTNODE, InitNode);
    SetDirectgVarInt(SLOT_LASTVIEW, InitView);
    SetDirectgVarInt(SLOT_MENU_LASTWORLD, InitWorld);
    SetDirectgVarInt(SLOT_MENU_LASTROOM, InitRoom);
    SetDirectgVarInt(SLOT_MENU_LASTNODE, InitNode);
    SetDirectgVarInt(SLOT_MENU_LASTVIEW, InitView);
    if (GetgVarInt(SLOT_PANAROTATE_SPEED) == 0)
        SetDirectgVarInt(SLOT_PANAROTATE_SPEED, 570);
    if (GetgVarInt(SLOT_KBD_ROTATE_SPEED) == 0)
        SetDirectgVarInt(SLOT_KBD_ROTATE_SPEED, 60);

    //\Hack
}

void EasterEggsAndDebug()
{
    char message_buffer[STRBUFSIZE];

    if (!KeyAnyHit())
        return;

    if ((KeyDown(SDLK_LCTRL) || KeyDown(SDLK_RCTRL)) && (KeyHit(SDLK_EQUALS) || KeyHit(SDLK_PLUS)))
    {
        setGamma(getGamma() + 0.1);
        sprintf(message_buffer, "Gamma: %1.1f", getGamma());
        game_timed_debug_message(1500, message_buffer);
    }

    if ((KeyDown(SDLK_LCTRL) || KeyDown(SDLK_RCTRL)) && (KeyHit(SDLK_UNDERSCORE) || KeyHit(SDLK_MINUS)))
    {
        setGamma(getGamma() - 0.1);
        sprintf(message_buffer, "Gamma: %1.1f", getGamma());
        game_timed_debug_message(1500, message_buffer);
    }

    if (CUR_GAME == GAME_ZGI)
    {
        if (CheckKeyboardMessage("IMNOTDEAF", 9))
        {
            //TODO: unknown
        }

        if (CheckKeyboardMessage("3100OPB", 7))
        {
            sprintf(message_buffer, "Current location: %c%c%c%c", GetgVarInt(SLOT_WORLD),
                    GetgVarInt(SLOT_ROOM),
                    GetgVarInt(SLOT_NODE),
                    GetgVarInt(SLOT_VIEW));
            game_timed_debug_message(3000, message_buffer);
        }

        if (CheckKeyboardMessage("KILLMENOW", 9))
        {
            SetNeedLocate('g', 'j', 'd', 'e', 0);
            SetgVarInt(2201, 35);
        }

        if (CheckKeyboardMessage("MIKESPANTS", 10))
        {
            NeedToLoadScript = true;
            SetNeedLocate('g', 'j', 't', 'm', 0);
        }
    }

    if (CUR_GAME == GAME_NEM)
    {
        if (CheckKeyboardMessage("CHLOE", 5))
        {
            SetNeedLocate('t', 'm', '2', 'g', 0);
            SetgVarInt(224, 1);
        }

        if (CheckKeyboardMessage("77MASSAVE", 9))
        {
            sprintf(message_buffer, "Current location: %c%c%c%c", GetgVarInt(SLOT_WORLD),
                    GetgVarInt(SLOT_ROOM),
                    GetgVarInt(SLOT_NODE),
                    GetgVarInt(SLOT_VIEW));
            game_timed_debug_message(3000, message_buffer);
        }

        if (CheckKeyboardMessage("IDKFA", 5))
        {
            SetNeedLocate('t', 'w', '3', 'f', 0);
            SetgVarInt(249, 1);
        }

        if (CheckKeyboardMessage("309NEWDORMA", 11))
        {
            SetNeedLocate('g', 'j', 'g', 'j', 0);
        }

        if (CheckKeyboardMessage("HELLOSAILOR", 11))
        {
            if (GetgVarInt(SLOT_WORLD) == 'v' && GetgVarInt(SLOT_ROOM) == 'b' &&
                GetgVarInt(SLOT_NODE) == '1' && GetgVarInt(SLOT_VIEW) == '0')
                sprintf(message_buffer, "0 %s 0", "v000hpta.raw");
            else
                sprintf(message_buffer, "0 %s 0", "v000hnta.raw");

            action_universe_music(message_buffer, 0, GetUni());
        }
    }

    if (CheckKeyboardMessage("FRAME", 5))
    {
        sprintf(message_buffer, "FPS: %d", GetFps());
        game_timed_debug_message(3000, message_buffer);
    }

    if (CheckKeyboardMessage("COMPUTERARCH", 12))
    {
        //TODO: var-watcher
    }

    if (CheckKeyboardMessage("XYZZY", 5))
        SetDirectgVarInt(SLOT_DEBUGCHEATS, 1 - GetgVarInt(SLOT_DEBUGCHEATS));

    if (GetgVarInt(SLOT_DEBUGCHEATS) == 1)
        if (CheckKeyboardMessage("GO????", 6))
        {
            SetNeedLocate(GetKeyBuffered(3),
                          GetKeyBuffered(2),
                          GetKeyBuffered(1),
                          GetKeyBuffered(0), 0);
        }

    if (KeyDown(SDLK_v) && (KeyDown(SDLK_LCTRL) || KeyDown(SDLK_RCTRL)))
    {
        sprintf(message_buffer, "<FONT \"ZorkNormal\" BOLD on JUSTIFY center POINT 18 RED 150 GREEN 100 BLUE 50>Zengine %s:<RED 255 GREEN 255 BLUE 255><NEWLINE>%s", ZORKMID_VER, GetGameTitle());
        game_timed_message(3000, message_buffer);
    }
}

void GameLoop()
{
    Mouse_SetCursor(CURSOR_IDLE);

    if (GetgVarInt(SLOT_MOUSE_RIGHT_CLICK) != 0)
        SetgVarInt(SLOT_MOUSE_RIGHT_CLICK, 0);

    if (GetgVarInt(SLOT_MOUSE_DOWN) != 0)
        SetgVarInt(SLOT_MOUSE_DOWN, 0);

    if (KeyAnyHit())
        if (GetLastKey() != SDLK_FIRST)
            SetgVarInt(SLOT_KEY_PRESS, GetWinKey(GetLastKey()));

    if (Rend_MouseInGamescr())
    {
        if (MouseUp(SDL_BUTTON_RIGHT))
            SetgVarInt(SLOT_MOUSE_RIGHT_CLICK, 1);

        if (CUR_GAME == GAME_NEM)
            if (MouseUp(SDL_BUTTON_RIGHT))
                inv_cycle();

        if (GetgVarInt(SLOT_MOUSE_RIGHT_CLICK) != 1)
            if (MouseDown(SDL_BUTTON_LEFT))
                SetgVarInt(SLOT_MOUSE_DOWN, 1);
    }

    ScrSys_ProcessActResList();

    if (!ScrSys_BreakExec())
        ScrSys_ExecPuzzleList(Getworld());

    if (!ScrSys_BreakExec())
        ScrSys_ExecPuzzleList(Getroom());

    if (!ScrSys_BreakExec())
        ScrSys_ExecPuzzleList(Getview());

    if (!ScrSys_BreakExec())
        ScrSys_ExecPuzzleList(GetUni());

    if (!ScrSys_BreakExec())
        menu_UpdateMenuBar();

    if (!NeedToLoadScript)
    {
        if (!ScrSys_BreakExec())
            Rend_MouseInteractOfRender();

        if (!ScrSys_BreakExec())
            ProcessControls(Getctrl());

        if (!ScrSys_BreakExec())
            Rend_RenderFunc();
    }

    if (NeedToLoadScript)
    {
        if (NeedToLoadScriptDelay <= 0)
        {
            NeedToLoadScript = false;
            ScrSys_ChangeLocation(Need_Locate.World, Need_Locate.Room, Need_Locate.Node, Need_Locate.View, Need_Locate.X, false);
            NeedToLoadScriptDelay = CHANGELOCATIONDELAY;
        }
        else
            NeedToLoadScriptDelay--;
    }

    EasterEggsAndDebug();

    // Keyboard shortcuts
    if (menu_GetMenuBarVal())
    {
        if (KeyHit(SDLK_s) && (KeyDown(SDLK_LCTRL) || KeyDown(SDLK_RCTRL)))
            if (menu_GetMenuBarVal() & menu_BAR_SAVE)
                SetNeedLocate(SaveWorld, SaveRoom, SaveNode, SaveView, 0);

        if (KeyHit(SDLK_r) && (KeyDown(SDLK_LCTRL) || KeyDown(SDLK_RCTRL)))
            if (menu_GetMenuBarVal() & menu_BAR_RESTORE)
                SetNeedLocate(LoadWorld, LoadRoom, LoadNode, LoadView, 0);

        if (KeyHit(SDLK_p) && (KeyDown(SDLK_LCTRL) || KeyDown(SDLK_RCTRL)))
            if (menu_GetMenuBarVal() & menu_BAR_SETTINGS)
                SetNeedLocate(PrefWorld, PrefRoom, PrefNode, PrefView, 0);

        if (KeyHit(SDLK_q) && (KeyDown(SDLK_LCTRL) || KeyDown(SDLK_RCTRL)))
            if (menu_GetMenuBarVal() & menu_BAR_EXIT)
                game_try_quit();
    }

    Rend_ScreenFlip();
}

void GameQuit()
{
    SDL_SetGamma(1.0, 1.0, 1.0);
    SDL_Quit();
    exit(0);
}

void GameUpdate()
{
    SDL_Event event;

    TimerTick();
    FlushHits();

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
            GameQuit();
            break;
        case SDL_KEYDOWN:
            SetHit(event.key.keysym.sym);
            break;
        }
    }

    UpdateKeyboard();

    if ((KeyDown(SDLK_RALT) || KeyDown(SDLK_LALT)) && KeyHit(SDLK_RETURN))
        Rend_SwitchFullscreen();
}

void game_timed_message(int32_t milsecs, const char *str)
{
    subrect_t *zzz = Rend_CreateSubRect(SUB_DEF_RECT);
    txt_DrawTxtInOneLine(str, zzz->img);
    Rend_DelaySubDelete(zzz, milsecs);
}

void game_timed_debug_message(int32_t milsecs, const char *str)
{
    int32_t tmp_up = 40;
    if (tmp_up < GAMESCREEN_Y)
        tmp_up = GAMESCREEN_Y;
    subrect_t *zzz = Rend_CreateSubRect(0, 0, GAMESCREEN_W, tmp_up);
    txt_DrawTxtInOneLine(str, zzz->img);
    Rend_DelaySubDelete(zzz, milsecs);
}

void game_delay_message(int32_t milsecs, const char *str)
{
    subrect_t *zzz = Rend_CreateSubRect(SUB_DEF_RECT);
    txt_DrawTxtInOneLine(str, zzz->img);
    Rend_RenderFunc();
    Rend_ScreenFlip();

    int32_t cur_time = SDL_GetTicks();
    int32_t nexttime = cur_time + milsecs;

    FlushKeybKey(SDLK_RETURN);
    FlushKeybKey(SDLK_SPACE);
    FlushKeybKey(SDLK_ESCAPE);

    while (!KeyDown(SDLK_SPACE) && !KeyDown(SDLK_RETURN) && !KeyDown(SDLK_ESCAPE) && nexttime > cur_time)
    {
        GameUpdate();
        cur_time = SDL_GetTicks();
        Rend_RenderFunc();
        Rend_ScreenFlip();
        Rend_Delay(5);
    }

    Rend_DeleteSubRect(zzz);
}

bool game_question_message(const char *str)
{
    subrect_t *zzz = Rend_CreateSubRect(SUB_DEF_RECT);
    txt_DrawTxtInOneLine(str, zzz->img);
    Rend_RenderFunc();
    Rend_ScreenFlip();

    FlushKeybKey(SDLK_y);
    FlushKeybKey(SDLK_n);
    FlushKeybKey(SDLK_o);

    while (!KeyDown(SDLK_y) && !KeyDown(SDLK_n) && !KeyDown(SDLK_o))
    {
        GameUpdate();
        Rend_RenderFunc();
        Rend_ScreenFlip();
        Rend_Delay(5);
    }

    Rend_DeleteSubRect(zzz);

    return (KeyDown(SDLK_y) || KeyDown(SDLK_o));
}

void game_try_quit()
{
    if (game_question_message(GetSystemString(SYSTEM_STR_EXITPROMT)))
        GameQuit();
}
