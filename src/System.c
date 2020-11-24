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
