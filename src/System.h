#ifndef SYSTEM_H_INCLUDED
#define SYSTEM_H_INCLUDED

#define ZORKMID_VER "1.0"
#define CUR_GAME GAME_ZGI
// extern int CUR_GAME;

// #define ENABLE_TRACING

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_audio.h>
#include <SDL/SDL_mixer.h>

#define GAME_AUTO 0
#define GAME_ZGI 1
#define GAME_NEM 2

#define STRBUFSIZE 0x400
#define MINIBUFSZ 32
#define PATHBUFSIZ 1024

//speed tune
#define FPS 15
#define FPS_DELAY (1000 / FPS)

//Script names
#define SystemWorld 'g'
#define SystemRoom 'j'

#define SaveWorld SystemWorld
#define SaveRoom SystemRoom
#define SaveNode 's'
#define SaveView 'e'

#define LoadWorld SystemWorld
#define LoadRoom SystemRoom
#define LoadNode 'r'
#define LoadView 'e'

#define PrefWorld SystemWorld
#define PrefRoom SystemRoom
#define PrefNode 'p'
#define PrefView 'e'

#define InitWorld SystemWorld
#define InitRoom 'a'
#define InitNode 'r'
#define InitView 'y'

#define NEW(type) (type *)calloc(1, sizeof(type))
#define NEW_ARRAY(type, n) (type *)calloc((n), sizeof(type))

#define Z_PANIC(x...)                                           \
    {                                                           \
        fprintf(stderr, "[PANIC] IN FUNCTION %s:\n", __func__); \
        fprintf(stderr, "[PANIC] " x);                          \
        abort();                                               \
    }

#ifdef ENABLE_TRACING
#define TRACE(x...) printf(x)
#else
#define TRACE(x...)
#endif

#define TRACE_LOADER(x...) TRACE("[LOADER] " x)
#define TRACE_PUZZLE(x...) TRACE("[PUZZLE] " x)
#define TRACE_ACTION(x...) TRACE("[ACTION] " x)
#define TRACE_CONTROL(x...) TRACE("[CONTROL] " x)

#define LOG_INFO(f, x...) printf("[INFO] %s : " f, __func__, x)
#define LOG_WARN(f, x...) fprintf(stderr, "[WARNING] %s : " f, __func__, x)

typedef struct anim_preplay_node anim_preplay_node_t;
typedef struct struct_action_res action_res_t;
typedef struct pzllst pzllst_t;
typedef struct ctrlnode ctrlnode_t;
typedef struct animnode animnode_t;

#include "avi_duck/simple_avi.h"
#include "mylist.h"
#include "Render.h"
#include "Text.h"
#include "Subtitles.h"
#include "Sound.h"
#include "Mouse.h"
#include "Loader.h"
#include "Puzzle.h"
#include "Controls.h"
#include "Scripting.h"
#include "Inventory.h"
#include "Actions.h"
#include "Anims.h"
#include "Menu.h"
#include "Game.h"

//Keyboard functions
void FlushKeybKey(SDLKey key);
void FlushHits();
void UpdateKeyboard();
bool KeyDown(SDLKey key);
bool KeyAnyHit();
int MouseX();
int MouseY();
bool MouseDown(int btn);
bool MouseHit(int btn);
bool MouseUp(int btn);
bool MouseDblClk();
bool MouseMove();
void FlushMouseBtn(int btn);
void SetHit(SDLKey key);
bool KeyHit(SDLKey key);

void InitVkKeys();
uint8_t GetWinKey(SDLKey key);
SDLKey GetLastKey();

int GetKeyBuffered(int indx);
bool CheckKeyboardMessage(const char *msg, int len);

char *PrepareString(char *buf);
char *GetParams(char *str);

const char *str_find(const char *haystack, const char *needle);
bool str_starts_with(const char *haystack, const char *needle);
bool str_ends_with(const char *haystack, const char *needle);
bool str_equals(const char *str1, const char *str2);
bool str_empty(const char *str);
const char *str_ltrim(const char *str);
char *str_trim(const char *str);
char **str_split(const char *str, const char *delim);

#endif // SYSTEM_H_INCLUDED
