#include "Game.h"

#define DELAY 10
#define DBL_CLK_TIME 250
#define KEYBUFLEN 14

static Location_t Need_Locate;
static bool NeedToLoadScript = false;
static int8_t NeedToLoadScriptDelay = CHANGELOCATIONDELAY;
static char *GamePath = "./";

static const char **GameStrings;

static uint32_t LastTick = 0;
static uint32_t DeltaTime = 0;

static bool beat = false;   // Indicates new Tick
static uint64_t reltime = 0; // Realtime ticks for calculate game ticks
static int frame_time = 0;

static uint32_t time = 0;
static int32_t frames = 0;
static int32_t fps = 1;

/* Input variables */
static bool KeyHits[SDLK_LAST]; // Array with hitted keys (once per press)
static bool AnyHit = false;  // it's indicate what any key was pressed
static uint8_t *Keys;        // Array with pressed keys (while pressed)
static SDLKey lastkey;
static SDLKey keybbuf[KEYBUFLEN];
static int32_t Mx, My, LMx, LMy;
static uint8_t LMstate, Mstate;
static int32_t M_dbl_time;
static bool M_dbl_clk = false;

//Resets game timer and set next realtime point to incriment game timer
static void TimerInit(float throttle)
{
    beat = false;
    frames = 0;
    fps = 1;
    frame_time = ceil((1000.0 - (float)(DELAY << 1)) / (throttle));
    reltime = Game_GetTime() + frame_time;
}

//Process game timer.
static void TimerTick()
{
    uint32_t cur_time = Game_GetTime();

    beat = reltime < cur_time; // New tick
    if (beat)
        reltime = cur_time + frame_time;

    Game_Delay(DELAY);

    cur_time = Game_GetTime();

    //
    if (LastTick != 0)
        DeltaTime = cur_time - LastTick;
    LastTick = cur_time;

    // FPS
    if (cur_time > time)
    {
        fps = frames;
        if (fps == 0)
            fps = 1;
        frames = 0;
        time = cur_time + 1000;
    }
    frames++;
}

//Resturn true if new tick appeared
bool Game_GetBeat()
{
    return beat;
}

uint32_t Game_GetDTime()
{
    if (DeltaTime == 0)
        DeltaTime = 1;
    return DeltaTime;
}

float Game_GetFps()
{
    return fps;
}

void FlushKeybKey(SDLKey key)
{
    KeyHits[key] = 0;
    Keys[key] = 0;
    if (lastkey == key)
        lastkey = SDLK_UNKNOWN;
}

SDLKey GetLastKey()
{
    return lastkey;
}

bool KeyHit(SDLKey key)
{
    return key < SDLK_LAST ? KeyHits[key] : false;
}

bool KeyAnyHit()
{
    return AnyHit;
}

bool KeyDown(SDLKey key)
{
    return key < SDLK_LAST ? Keys[key] : false;
}

int MouseX()
{
    return Mx;
}

int MouseY()
{
    return My;
}

bool MouseUp(int btn)
{
    return ((Mstate & btn) == 0 && (LMstate & btn) == btn);
}

bool MouseDown(int btn)
{
    return (Mstate & btn);
}

bool MouseHit(int btn)
{
    return ((LMstate & btn) == 0 && (Mstate & btn) == btn);
}

bool MouseDblClk()
{
    return M_dbl_clk;
}

bool MouseMove()
{
    return (LMx != Mx || LMy != My);
}

void FlushMouseBtn(int btn)
{
    Mstate &= ~btn;
    LMstate &= ~btn;
    if (btn == SDL_BUTTON_LEFT)
        M_dbl_clk = false;
}

static SDLKey GetKeyBuffered(int indx)
{
    if (indx > KEYBUFLEN)
        return 0;
    return keybbuf[KEYBUFLEN - indx - 1];
}

static void FlushKeyHits()
{
    AnyHit = false;
    memset(KeyHits, 0, sizeof(KeyHits));
    lastkey = SDLK_UNKNOWN;
}

static void SetKeyHit(SDLKey key)
{
    AnyHit = true;
    KeyHits[key] = 1;
    lastkey = key;
    for (int i = 0; i < KEYBUFLEN - 1; i++)
        keybbuf[i] = keybbuf[i + 1];
    keybbuf[KEYBUFLEN - 1] = key;
}

static uint8_t GetWinKey(SDLKey key)
{
    if (key >= SDLK_0 && key <= SDLK_9)
        return 0x30 + (key - SDLK_0);

    if (key >= SDLK_a && key <= SDLK_z)
        return 0x41 + (key - SDLK_a);

    if (key >= SDLK_KP0 && key <= SDLK_KP9)
        return 0x60 + (key - SDLK_KP0);

    if (key >= SDLK_F1 && key <= SDLK_F12)
        return 0x70 + (key - SDLK_F1);

    switch (key)
    {
    case SDLK_BACKSPACE: return 8;
    case SDLK_TAB: return 9;
    case SDLK_CLEAR: return 12;
    case SDLK_RETURN: return 13;
    // case SDLK_MENU: return 18;
    case SDLK_CAPSLOCK: return 20;
    case SDLK_ESCAPE: return 27;
    case SDLK_SPACE: return 32;
    case SDLK_PAGEUP: return 33;
    case SDLK_PAGEDOWN: return 34;
    case SDLK_END: return 35;
    case SDLK_HOME: return 36;
    case SDLK_LEFT: return 37;
    case SDLK_UP: return 38;
    case SDLK_RIGHT: return 39;
    case SDLK_DOWN: return 40;
    case SDLK_PRINT: return 42;
    case SDLK_INSERT: return 45;
    case SDLK_DELETE: return 46;
    case SDLK_HELP: return 47;
    case SDLK_KP_MULTIPLY: return 0x6A;
    case SDLK_KP_PLUS: return 0x6B;
    case SDLK_KP_MINUS: return 0x6D;
    case SDLK_KP_PERIOD: return 0x6E;
    case SDLK_KP_DIVIDE: return 0x6F;
    case SDLK_NUMLOCK: return 0x90;
    case SDLK_SCROLLOCK: return 0x91;
    case SDLK_LSHIFT: return 0xA0;
    case SDLK_RSHIFT: return 0xA1;
    case SDLK_LCTRL: return 0xA2;
    case SDLK_RCTRL: return 0xA3;
    case SDLK_MENU: return 0xA5;
    case SDLK_LEFTBRACKET: return 0xDB;
    case SDLK_RIGHTBRACKET: return 0xDD;
    case SDLK_SEMICOLON: return 0xBA;
    // case SDLK_BACKSLASH: return 0xDC;
    case SDLK_QUOTE: return 0xDE;
    case SDLK_SLASH: return 0xBF;
    case SDLK_BACKSLASH: return 0xC0;
    case SDLK_COMMA: return 0xBC;
    case SDLK_PERIOD: return 0xBE;
    case SDLK_MINUS: return 0xBD;
    case SDLK_PLUS: return 0xBB;
    default: return 0;
    }
}

static bool CheckKeyboardMessage(const char *msg, int len)
{
    if (len > KEYBUFLEN)
        return false;

    for (int i = 0; i < len; i++)
    {
        int ki = GetWinKey(keybbuf[KEYBUFLEN - i - 1]);
        if (msg[len - i - 1] != ki && msg[len - i - 1] != '?')
            return false;
    }

    return true;
}

static void UpdateKeyboard()
{
    Keys = SDL_GetKeyState(NULL);

    M_dbl_clk = false;
    LMstate = Mstate;
    LMx = Mx;
    LMy = My;
    Mstate = SDL_GetMouseState(&Mx, &My);

    if (MouseHit(MOUSE_BTN_LEFT))
    {
        if ((uint32_t)M_dbl_time < Game_GetTime())
        {
            M_dbl_time = Game_GetTime() + DBL_CLK_TIME;
        }
        else
        {
            M_dbl_time = 0;
            M_dbl_clk = true;
        }
    }
}

static void LoadGameStrings(void)
{
    const char *file = (CUR_GAME == GAME_ZGI ? "INQUIS.STR" : "NEMESIS.STR");
    char filename[PATHBUFSIZ];
    sprintf(filename, "%s/%s", Game_GetPath(), file);
    GameStrings = (const char **)Loader_LoadSTR(filename);
}

static void SetGamePath(const char *path)
{
    GamePath = strdup(path);
    while (GamePath[strlen(GamePath - 1)] == '/' || GamePath[strlen(GamePath - 1)] == '\\')
        GamePath[strlen(GamePath - 1)] = 0;
}

const char *Game_GetString(int32_t indx)
{
    return GameStrings[indx & 0xFF];
}

const char *Game_GetPath()
{
    return GamePath;
}

const char *GetGameTitle()
{
    switch (CUR_GAME)
    {
    case GAME_ZGI:
        return "Zork: Grand Inquisitor";
    case GAME_NEM:
        return "Zork: Nemesis";
    default:
        return "Unknown";
    }
}

void Game_Relocate(uint8_t w, uint8_t r, uint8_t v1, uint8_t v2, int32_t X)
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

void GameInit(const char *path)
{
    SetGamePath(path);
    Loader_Init(path);
    LoadGameStrings();
    Mouse_LoadCursors();
    Menu_LoadGraphics();

    ScrSys_Init();
    ScrSys_LoadScript(GetUni(), Loader_FindNode("universe.scr"), false, NULL);

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
        Rend_SetGamma(Rend_GetGamma() + 0.1);
        sprintf(message_buffer, "Gamma: %1.1f", Rend_GetGamma());
        game_timed_debug_message(1500, message_buffer);
    }

    if ((KeyDown(SDLK_LCTRL) || KeyDown(SDLK_RCTRL)) && (KeyHit(SDLK_UNDERSCORE) || KeyHit(SDLK_MINUS)))
    {
        Rend_SetGamma(Rend_GetGamma() - 0.1);
        sprintf(message_buffer, "Gamma: %1.1f", Rend_GetGamma());
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
            Game_Relocate('g', 'j', 'd', 'e', 0);
            SetgVarInt(2201, 35);
        }

        if (CheckKeyboardMessage("MIKESPANTS", 10))
        {
            NeedToLoadScript = true;
            Game_Relocate('g', 'j', 't', 'm', 0);
        }
    }

    if (CUR_GAME == GAME_NEM)
    {
        if (CheckKeyboardMessage("CHLOE", 5))
        {
            Game_Relocate('t', 'm', '2', 'g', 0);
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
            Game_Relocate('t', 'w', '3', 'f', 0);
            SetgVarInt(249, 1);
        }

        if (CheckKeyboardMessage("309NEWDORMA", 11))
        {
            Game_Relocate('g', 'j', 'g', 'j', 0);
        }

        if (CheckKeyboardMessage("HELLOSAILOR", 11))
        {
            if (GetgVarInt(SLOT_WORLD) == 'v' && GetgVarInt(SLOT_ROOM) == 'b' &&
                GetgVarInt(SLOT_NODE) == '1' && GetgVarInt(SLOT_VIEW) == '0')
                sprintf(message_buffer, "0 %s 0", "v000hpta.raw");
            else
                sprintf(message_buffer, "0 %s 0", "v000hnta.raw");

            Actions_Run("universe_music", message_buffer, 0, GetUni());
        }
    }

    if (CheckKeyboardMessage("FRAME", 5))
    {
        sprintf(message_buffer, "FPS: %.2f", Game_GetFps());
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
            Game_Relocate(GetKeyBuffered(3),
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
        if (GetLastKey() != SDLK_UNKNOWN)
            SetgVarInt(SLOT_KEY_PRESS, GetWinKey(GetLastKey()));

    if (Rend_MouseInGamescr())
    {
        if (MouseUp(MOUSE_BTN_RIGHT))
            SetgVarInt(SLOT_MOUSE_RIGHT_CLICK, 1);

        if (CUR_GAME == GAME_NEM)
            if (MouseUp(MOUSE_BTN_RIGHT))
                Inventory_Cycle();

        if (GetgVarInt(SLOT_MOUSE_RIGHT_CLICK) != 1)
            if (MouseDown(MOUSE_BTN_LEFT))
                SetgVarInt(SLOT_MOUSE_DOWN, 1);
    }

    ScrSys_ProcessActionsList();

    if (!ScrSys_BreakExec())
        ScrSys_ExecPuzzleList(Getworld());

    if (!ScrSys_BreakExec())
        ScrSys_ExecPuzzleList(Getroom());

    if (!ScrSys_BreakExec())
        ScrSys_ExecPuzzleList(Getview());

    if (!ScrSys_BreakExec())
        ScrSys_ExecPuzzleList(GetUni());

    if (!ScrSys_BreakExec())
        Menu_Update();

    if (!NeedToLoadScript)
    {
        if (!ScrSys_BreakExec())
            Rend_MouseInteractOfRender();

        if (!ScrSys_BreakExec())
            Controls_ProcessList(GetControlsList());

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
    if (Menu_GetVal())
    {
        if (KeyHit(SDLK_s) && (KeyDown(SDLK_LCTRL) || KeyDown(SDLK_RCTRL)))
            if (Menu_GetVal() & MENU_BAR_SAVE)
                Game_Relocate(SaveWorld, SaveRoom, SaveNode, SaveView, 0);

        if (KeyHit(SDLK_r) && (KeyDown(SDLK_LCTRL) || KeyDown(SDLK_RCTRL)))
            if (Menu_GetVal() & MENU_BAR_RESTORE)
                Game_Relocate(LoadWorld, LoadRoom, LoadNode, LoadView, 0);

        if (KeyHit(SDLK_p) && (KeyDown(SDLK_LCTRL) || KeyDown(SDLK_RCTRL)))
            if (Menu_GetVal() & MENU_BAR_SETTINGS)
                Game_Relocate(PrefWorld, PrefRoom, PrefNode, PrefView, 0);

        if (KeyHit(SDLK_q) && (KeyDown(SDLK_LCTRL) || KeyDown(SDLK_RCTRL)))
            if (Menu_GetVal() & MENU_BAR_EXIT)
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
    FlushKeyHits();

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
            GameQuit();
            break;
        case SDL_VIDEORESIZE:
            Rend_SetVideoMode(event.resize.w, event.resize.h, -1, -1);
            break;
        case SDL_KEYDOWN:
            SetKeyHit(event.key.keysym.sym);
            break;
        }
    }

    UpdateKeyboard();

    if ((KeyDown(SDLK_RALT) || KeyDown(SDLK_LALT)) && KeyHit(SDLK_RETURN))
        Rend_SetVideoMode(640, 480, !FULLSCREEN, -1);
}

void game_timed_message(int32_t milsecs, const char *str)
{
    subrect_t *zzz = Rend_CreateSubRect(0, GAMESCREEN_H, GAMESCREEN_W, 68);
    Text_DrawInOneLine(str, zzz->img);
    Rend_DelaySubDelete(zzz, milsecs);
}

void game_timed_debug_message(int32_t milsecs, const char *str)
{
    int32_t tmp_up = 40;
    if (tmp_up < GAMESCREEN_Y)
        tmp_up = GAMESCREEN_Y;
    subrect_t *zzz = Rend_CreateSubRect(0, 0, GAMESCREEN_W, tmp_up);
    Text_DrawInOneLine(str, zzz->img);
    Rend_DelaySubDelete(zzz, milsecs);
}

void game_delay_message(int32_t milsecs, const char *str)
{
    subrect_t *zzz = Rend_CreateSubRect(0, GAMESCREEN_H, GAMESCREEN_W, 68);
    Text_DrawInOneLine(str, zzz->img);
    Rend_RenderFunc();
    Rend_ScreenFlip();

    int32_t cur_time = Game_GetTime();
    int32_t nexttime = cur_time + milsecs;

    FlushKeybKey(SDLK_RETURN);
    FlushKeybKey(SDLK_SPACE);
    FlushKeybKey(SDLK_ESCAPE);

    while (!KeyDown(SDLK_SPACE) && !KeyDown(SDLK_RETURN) && !KeyDown(SDLK_ESCAPE) && nexttime > cur_time)
    {
        GameUpdate();
        cur_time = Game_GetTime();
        Rend_RenderFunc();
        Rend_ScreenFlip();
        Game_Delay(DELAY);
    }

    Rend_DeleteSubRect(zzz);
}

bool game_question_message(const char *str)
{
    subrect_t *zzz = Rend_CreateSubRect(0, GAMESCREEN_H, GAMESCREEN_W, 68);
    Text_DrawInOneLine(str, zzz->img);
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
        Game_Delay(DELAY);
    }

    Rend_DeleteSubRect(zzz);

    return (KeyDown(SDLK_y) || KeyDown(SDLK_o));
}

void game_try_quit()
{
    if (game_question_message(Game_GetString(SYSTEM_STR_EXITPROMT)))
        GameQuit();
}
