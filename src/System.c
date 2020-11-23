#include "System.h"
#include "VkKeys.h"

#define DBL_CLK_TIME 250
#define KEYBUFLEN 14

static uint8_t VkKeys[512];  // windows map vk keys
static uint8_t KeyHits[512]; // Array with hitted keys (once per press)
static bool AnyHit = false;  // it's indicate what any key was pressed
static uint8_t *Keys;        // Array with pressed keys (while pressed)
static SDLKey lastkey;
static int16_t keybbuf[KEYBUFLEN];
static int32_t Mx, My, LMx, LMy;
static uint8_t LMstate, Mstate;
static int32_t M_dbl_time;
static bool M_dbl_clk = false;

static MList *FMan;
static MList *FontList;
static char **SystemStrings;

static uint32_t CurrentTime = 0;
static uint32_t DeltaTime = 0;

static uint64_t mtime = 0;   // Game timer ticks [after ~23 milliards years will came overflow of this var, don't play so long]
static bool btime = false;   // Indicates new Tick
static uint64_t reltime = 0; // Realtime ticks for calculate game ticks
static int tofps = 0;

static uint32_t time = 0;
static int32_t frames = 0;
static int32_t fps = 1;

#define DELAY 10

typedef struct BinTreeNd
{
    struct BinTreeNd *zero;
    struct BinTreeNd *one;
    FManNode_t *nod;
} BinTreeNd_t;

static BinTreeNd_t *root = NULL;
static MList *BinNodesList = NULL;

//Reset state of key hits states
void FlushHits()
{
    AnyHit = false;
    memset(KeyHits, 0, 512);
    lastkey = SDLK_FIRST;
}

//Sets hit state for key
void SetHit(SDLKey key)
{
    AnyHit = true;
    KeyHits[key] = 1;
    lastkey = key;
    for (int16_t i = 0; i < KEYBUFLEN - 1; i++)
        keybbuf[i] = keybbuf[i + 1];
    keybbuf[KEYBUFLEN - 1] = key;
}

void FlushKeybKey(SDLKey key)
{
    KeyHits[key] = 0;
    Keys[key] = 0;
    if (lastkey == key)
        lastkey = SDLK_FIRST;
}

int GetKeyBuffered(int indx)
{
    if (indx > KEYBUFLEN)
        return 0;
    else
        return keybbuf[KEYBUFLEN - indx - 1];
}

bool CheckKeyboardMessage(const char *msg, int len)
{
    if (len > KEYBUFLEN)
        return false;
    for (int i = 0; i < len; i++)
    {
        int ki = GetWinKey((SDLKey)keybbuf[KEYBUFLEN - i - 1]);
        if (msg[len - i - 1] != ki && msg[len - i - 1] != '?')
            return false;
    }

    return true;
}

SDLKey GetLastKey()
{
    return lastkey;
}

//Returns hit state of the key
bool KeyHit(SDLKey key)
{
    return KeyHits[key] ? true : false;
}

//return true if any key was hitted(by key hit)
bool KeyAnyHit()
{
    return AnyHit;
}

//Update key pressed states
void UpdateKeyboard()
{
    Keys = SDL_GetKeyState(NULL);

    M_dbl_clk = false;
    LMstate = Mstate;
    LMx = Mx;
    LMy = My;
    Mstate = SDL_GetMouseState(&Mx, &My);

    if (MouseHit(SDL_BUTTON_LEFT))
    {
        if ((uint32_t)M_dbl_time < SDL_GetTicks())
        {
            M_dbl_time = SDL_GetTicks() + DBL_CLK_TIME;
        }
        else
        {
            M_dbl_time = 0;
            M_dbl_clk = true;
        }
    }
}

//return true if key was pressed(continously)
bool KeyDown(SDLKey key)
{
    return Keys[key] ? true : false;
}

void InitVkKeys()
{
    memset(VkKeys, 0, 512 * sizeof(uint8_t));
    VkKeys[SDLK_BACKSPACE] = VK_BACK;
    VkKeys[SDLK_TAB] = VK_TAB;
    VkKeys[SDLK_CLEAR] = VK_CLEAR;
    VkKeys[SDLK_RETURN] = VK_RETURN;
    VkKeys[SDLK_MENU] = VK_MENU;
    VkKeys[SDLK_CAPSLOCK] = VK_CAPITAL;
    VkKeys[SDLK_ESCAPE] = VK_ESCAPE;
    VkKeys[SDLK_SPACE] = VK_SPACE;
    VkKeys[SDLK_PAGEUP] = VK_PRIOR;
    VkKeys[SDLK_PAGEDOWN] = VK_NEXT;
    VkKeys[SDLK_END] = VK_END;
    VkKeys[SDLK_HOME] = VK_HOME;
    VkKeys[SDLK_LEFT] = VK_LEFT;
    VkKeys[SDLK_UP] = VK_UP;
    VkKeys[SDLK_RIGHT] = VK_RIGHT;
    VkKeys[SDLK_DOWN] = VK_DOWN;
    VkKeys[SDLK_PRINT] = VK_PRINT;
    VkKeys[SDLK_INSERT] = VK_INSERT;
    VkKeys[SDLK_DELETE] = VK_DELETE;
    VkKeys[SDLK_HELP] = VK_HELP;

    for (int i = 0; i <= 9; i++)
        VkKeys[SDLK_0 + i] = VK_0 + i;
    for (int i = 0; i <= 25; i++)
        VkKeys[SDLK_a + i] = VK_A + i;

    VkKeys[SDLK_KP0] = VK_NUMPAD0;
    VkKeys[SDLK_KP1] = VK_NUMPAD1;
    VkKeys[SDLK_KP2] = VK_NUMPAD2;
    VkKeys[SDLK_KP3] = VK_NUMPAD3;
    VkKeys[SDLK_KP4] = VK_NUMPAD4;
    VkKeys[SDLK_KP5] = VK_NUMPAD5;
    VkKeys[SDLK_KP6] = VK_NUMPAD6;
    VkKeys[SDLK_KP7] = VK_NUMPAD7;
    VkKeys[SDLK_KP8] = VK_NUMPAD8;
    VkKeys[SDLK_KP9] = VK_NUMPAD9;
    VkKeys[SDLK_KP_MULTIPLY] = VK_MULTIPLY;
    VkKeys[SDLK_KP_PLUS] = VK_ADD;
    VkKeys[SDLK_KP_MINUS] = VK_SUBTRACT;
    VkKeys[SDLK_KP_PERIOD] = VK_DECIMAL;
    VkKeys[SDLK_KP_DIVIDE] = VK_DIVIDE;

    for (int i = 0; i < 15; i++)
        VkKeys[SDLK_F1 + i] = VK_F1 + i;

    VkKeys[SDLK_NUMLOCK] = VK_NUMLOCK;
    VkKeys[SDLK_SCROLLOCK] = VK_SCROLL;
    VkKeys[SDLK_LSHIFT] = VK_LSHIFT;
    VkKeys[SDLK_RSHIFT] = VK_RSHIFT;
    VkKeys[SDLK_LCTRL] = VK_LCONTROL;
    VkKeys[SDLK_RCTRL] = VK_RCONTROL;
    VkKeys[SDLK_MENU] = VK_RMENU;
    VkKeys[SDLK_LEFTBRACKET] = VK_OEM_4;
    VkKeys[SDLK_RIGHTBRACKET] = VK_OEM_6;
    VkKeys[SDLK_SEMICOLON] = VK_OEM_1;
    VkKeys[SDLK_BACKSLASH] = VK_OEM_5;
    VkKeys[SDLK_QUOTE] = VK_OEM_7;
    VkKeys[SDLK_SLASH] = VK_OEM_2;
    VkKeys[SDLK_BACKSLASH] = VK_OEM_3;
    VkKeys[SDLK_COMMA] = VK_OEM_COMMA;
    VkKeys[SDLK_PERIOD] = VK_OEM_PERIOD;
    VkKeys[SDLK_MINUS] = VK_OEM_MINUS;
    VkKeys[SDLK_PLUS] = VK_OEM_PLUS;
}

uint8_t GetWinKey(SDLKey key)
{
    return VkKeys[key];
}

int MouseX()
{
    return Mx;
}

int MouseY()
{
    return My;
}

bool MouseDown(int btn)
{
    if (Mstate & SDL_BUTTON(btn))
        return true;
    else
        return false;
}

bool MouseHit(int btn)
{
    if ((LMstate & SDL_BUTTON(btn)) == 0 && (Mstate & SDL_BUTTON(btn)) == SDL_BUTTON(btn))
        return true;
    else
        return false;
}

bool MouseUp(int btn)
{
    if ((Mstate & SDL_BUTTON(btn)) == 0 && (LMstate & SDL_BUTTON(btn)) == SDL_BUTTON(btn))
        return true;
    else
        return false;
}

bool MouseDblClk()
{
    return M_dbl_clk;
}

void FlushMouseBtn(int btn)
{
    Mstate &= ~SDL_BUTTON(btn);
    LMstate &= ~SDL_BUTTON(btn);
    if (btn == SDL_BUTTON_LEFT)
        M_dbl_clk = false;
}

bool MouseMove()
{
    if (LMx != Mx || LMy != My)
        return true;
    return false;
}

int32_t GetFps()
{
    return fps;
}

//Resets game timer and set next realtime point to incriment game timer
void TimerInit(float throttle)
{
    mtime = 0;
    btime = false;
    frames = 0;
    fps = 1;
    tofps = ceil((1000.0 - (float)(DELAY << 1)) / (throttle));
    reltime = SDL_GetTicks() + tofps;
}

//Process game timer.
void TimerTick()
{
    if (reltime < SDL_GetTicks()) //New tick
    {
        mtime++;
        btime = true;
        reltime = SDL_GetTicks() + tofps;
    }
    else //No new tick
    {
        btime = false;
    }

    Rend_Delay(DELAY);

    //
    uint32_t tmptime = SDL_GetTicks();
    if (CurrentTime != 0)
        DeltaTime = tmptime - CurrentTime;
    CurrentTime = tmptime;

    // FPS
    if (SDL_GetTicks() > time)
    {
        fps = frames;
        if (fps == 0)
            fps = 1;
        frames = 0;
        time = SDL_GetTicks() + 1000;
    }
    frames++;
}

//Resturn true if new tick appeared
bool GetBeat()
{
    return btime;
}

uint32_t GetDTime()
{
    if (DeltaTime == 0)
        DeltaTime = 1;
    return DeltaTime;
}

void LoadSystemStrings(void)
{
    const char *file = (CUR_GAME == GAME_ZGI ? "INQUIS.STR" : "NEMESIS.STR");
    char filename[PATHBUFSIZ];
    sprintf(filename, "%s/%s", GetGamePath(), file);
    SystemStrings = loader_loadStr(filename);
}

const char *GetSystemString(int32_t indx)
{
    return SystemStrings[indx & 0xFF];
}

bool isDirectory(const char *path)
{
    struct stat statbuf;
    if (stat(path, &statbuf) != 0)
        return 0;
    return S_ISDIR(statbuf.st_mode);
}

bool FileExists(const char *path)
{
    struct stat statbuf;
    return stat(path, &statbuf) == 0;
}

void FindAssets(const char *dir)
{
    char path[PATHBUFSIZ];
    int len = strlen(dir);

    strcpy(path, dir);

    while (path[len - 1] == '/' || path[len - 1] == '\\')
    {
        path[len - 1] = 0;
        len--;
    }

    TRACE_LOADER("Listing dir: %s\n", path);

    DIR *dr = opendir(path);
    struct dirent *de;

    if (!dr)
        return;

    while ((de = readdir(dr)))
    {
        if (strlen(de->d_name) < 3)
            continue;

        sprintf(path + len, "/%s", de->d_name);

        if (isDirectory(path))
        {
            FindAssets(path);
        }
        else if (str_ends_with(path, ".ZFS"))
        {
            loader_openzfs(path, FMan);
        }
        else if (str_ends_with(path, ".TTF"))
        {
            TRACE_LOADER("Adding font : %s\n", path);
            TTF_Font *fnt = TTF_OpenFont(path, 10);
            if (fnt != NULL)
            {
                graph_font_t *tmpfnt = NEW(graph_font_t);
                strncpy(tmpfnt->Name, TTF_FontFaceFamilyName(fnt), 63);
                strncpy(tmpfnt->path, path, 255);
                AddToMList(FontList, tmpfnt);
                TTF_CloseFont(fnt);
            }
        }
        else
        {
            TRACE_LOADER("Adding game file : %s\n", path);
            FManNode_t *nod = NEW(FManNode_t);
            nod->path = strdup(path);
            nod->name = nod->path + len + 1;
            nod->zfs = NULL;
            AddToMList(FMan, nod);
            AddToBinTree(nod);
        }
    }
    closedir(dr);
}

void InitFileManager(const char *dir)
{
    FMan = CreateMList();
    FontList = CreateMList();
    BinNodesList = CreateMList();
    FindAssets(GetGamePath());
    LoadSystemStrings();
}

TTF_Font *GetFontByName(char *name, int size)
{
    graph_font_t *fnt = NULL;

    StartMList(FontList);
    while (!eofMList(FontList))
    {
        fnt = (graph_font_t *)DataMList(FontList);
        if (str_equals(fnt->Name, name)) // str_starts_with
            break;

        NextMList(FontList);
    }

    if (fnt == NULL)
        return NULL;

    return TTF_OpenFont(fnt->path, size);
}

char *PrepareString(char *buf)
{
    char *str = (char *)str_ltrim(buf);
    char *tmp;

    // Cut at newline or comment
    if ((tmp = strchr(str, 0x0A))) *tmp = 0;
    if ((tmp = strchr(str, 0x0D))) *tmp = 0;
    if ((tmp = strchr(str, '#'))) *tmp = 0;

    for (int i = 0, len = strlen(str); i < len; i++)
        str[i] = tolower(str[i]);

    return str;
}

char *GetParams(char *str)
{
    for (int i = strlen(str) - 1; i > -1; i--)
    {
        if (str[i] == ')')
            str[i] = 0x0;
        else if (str[i] == '(')
        {
            return str + i + 1;
        }
        else if (str[i] == ',')
            str[i] = ' ';
    }
    return (char *)" ";
}

int GetIntVal(char *chr)
{
    if (chr[0] == '[')
        return GetgVarInt(atoi(chr + 1));
    else
        return atoi(chr);
}

void AddToBinTree(FManNode_t *nod)
{
    char buffer[255];
    int32_t t_len = strlen(nod->name);

    for (int i = 0; i < t_len; i++)
        buffer[i] = tolower(nod->name[i]);

    buffer[t_len] = 0x0;

    if (root == NULL)
    {
        root = NEW(BinTreeNd_t);
        AddToMList(BinNodesList, root);
    }

    BinTreeNd_t **treenod = &root;
    t_len = strlen(buffer);
    for (int j = 0; j < t_len; j++)
        for (int i = 0; i < 8; i++)
        {
            int bit = ((buffer[j]) >> i) & 1;
            if (bit)
                treenod = &((*treenod)->one);
            else
                treenod = &((*treenod)->zero);

            if (*treenod == NULL)
            {
                *treenod = NEW(BinTreeNd_t);
                AddToMList(BinNodesList, *treenod);
            }
        }
    if ((*treenod)->nod == NULL) //we don't need to reSet nodes (ADDON and patches don't work without it)
        (*treenod)->nod = nod;
    else if (mfsize((*treenod)->nod) < 10)
        if (mfsize(nod) >= 10)
            (*treenod)->nod = nod;
}

FManNode_t *FindInBinTree(const char *chr)
{
    char buffer[255];
    int32_t t_len = strlen(chr);
    for (int i = 0; i < t_len; i++)
        buffer[i] = tolower(chr[i]);

    buffer[t_len] = 0x0;

    BinTreeNd_t *treenod = root;

    t_len = strlen(buffer);
    for (int j = 0; j < t_len; j++)
        for (int i = 0; i < 8; i++)
        {
            int bit = ((buffer[j]) >> i) & 1;
            if (bit)
                treenod = treenod->one;
            else
                treenod = treenod->zero;

            if (treenod == NULL)
                return NULL;
        }

    return treenod->nod;
}

const char *GetFilePath(const char *chr)
{
    FManNode_t *nod = FindInBinTree(chr);

    if (nod && nod->zfs == NULL)
        return nod->path;

    LOG_WARN("Can't find file '%s'\n", chr);
    return NULL;
}

const char *str_find(const char *haystack, const char *needle)
{
    if (!haystack || !needle)
        return NULL;

    size_t h_len = strlen(haystack);
    size_t n_len = strlen(needle);

    if (n_len > h_len)
        return NULL;

    const char *a = haystack, *e = needle;

    while (*a && *e)
    {
        if (toupper((unsigned char)(*a)) != toupper((unsigned char)(*e)))
        {
            ++haystack;
            a = haystack;
            e = needle;
        }
        else
        {
            ++a;
            ++e;
        }
    }

    return *e ? NULL : haystack;
}

bool str_starts_with(const char *haystack, const char *needle)
{
    return str_find(haystack, needle) == haystack;
}

bool str_ends_with(const char *haystack, const char *needle)
{
    if (!haystack || !needle)
        return false;

    size_t h_len = strlen(haystack);
    size_t n_len = strlen(needle);

    if (n_len > h_len)
        return false;

    return str_find(haystack + h_len - n_len, needle) != NULL;
}

bool str_equals(const char *str1, const char *str2)
{
    return str1 && str2 && strcasecmp(str1, str2) == 0;
}

bool str_empty(const char *str)
{
    return str == NULL || str[0] == 0;
}

const char *str_ltrim(const char *str)
{
    if (!str_empty(str))
    {
        while (*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n')
            str++;
    }
    return str;
}

char *str_trim(const char *buffer)
{
    if (!str_empty(buffer))
    {
        const char *str = str_ltrim(buffer);
        const char *tmp = str + strlen(str) - 1;
        size_t trim = 0;

        while (*tmp == ' ' || *tmp == '\t' || *tmp == '\r' || *tmp == '\n')
        {
            tmp--;
            trim++;
        }

        int new_len = strlen(str) - trim;

        if (new_len > 0) {
            char *out = (char *)malloc(new_len + 1);
            memcpy(out, str, new_len);
            out[new_len] = 0;
            return out;
        }
    }

    return strdup("");
}
