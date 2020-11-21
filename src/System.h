#ifndef SYSTEM_H_INCLUDED
#define SYSTEM_H_INCLUDED

#define ZORKMID_VER "1.0"
#define CUR_GAME GAME_ZGI
// extern int CUR_GAME;

#define TRACE_ACTION() printf("[ACTION] %s(%s)\n", __func__, params);
//#define TRACE_ACTION()
#define TRACE_LOADER(x...) printf("[LOADER] " x);
//#define TRACE_LOADER(x...)
#define TRACE_PUZZLE(x...) printf(x)
// #define TRACE_PUZZLE(x...)

//if you plan to build engine with smpeg support
//#define SMPEG_SUPPORT

#define SFTYPE SDL_SWSURFACE // SDL_HWSURFACE

#define SYS_STRINGS_FILE (CUR_GAME == GAME_ZGI ? "INQUIS.STR" : "NEMESIS.STR")
#define CTRL_SAVE_FILE (CUR_GAME == GAME_ZGI ? "inquis.sav" : "nemesis.sav")
#define CTRL_SAVE_SAVES (CUR_GAME == GAME_ZGI ? "inqsav%d.sav" : "nemsav%d.sav")
#define TIMER_DELAY (CUR_GAME == GAME_ZGI ? 100 : 1000)

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
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
#define FPS_DELAY 66 //millisecs  1000/FPS

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
        exit(-1);                                               \
    }

char *findstr(const char *h, const char *n);

#ifdef SMPEG_SUPPORT
#include <smpeg/smpeg.h>
#endif

#include "avi_duck/simple_avi.h"

typedef struct anim_preplay_node anim_preplay_node_t;
typedef struct struct_action_res action_res_t;
typedef struct pzllst pzllst_t;
typedef struct ctrlnode ctrlnode_t;
typedef struct animnode animnode_t;

#include "mylist.h"
#include "Render.h"
#include "Text.h"
#include "Subtitles.h"
#include "Sound.h"
#include "Mouse.h"
#include "Loader.h"
#include "Puzzle.h"
#include "Control.h"
#include "Scripting.h"
#include "Inventory.h"
#include "Actions.h"
#include "Anims.h"
#include "Menu.h"
#include "Game.h"

//Game timer functions
void TimerInit(float fps);
void TimerTick();
int32_t GetFps();
bool GetBeat();
void Delay(uint32_t ms);

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

void InitFileManager(const char *dir);
const char *GetFilePath(const char *chr);
const char *GetExactFilePath(const char *chr);
TTF_Font *GetFontByName(char *name, int size);
const char *GetSystemString(int32_t indx);

int GetKeyBuffered(int indx);
bool CheckKeyboardMessage(const char *msg, int len);

void FindAssets(const char *dir);
bool isDirectory(const char *);
bool FileExist(const char *);
int32_t FileSize(const char *);

uint32_t GetDTime();

char *PrepareString(char *buf);
char *TrimLeft(char *buf);
char *TrimRight(char *buf);
char *GetParams(char *str);
int GetIntVal(char *chr);

#define strCMP(X, Y) strncasecmp(X, Y, strlen(Y))

void AddToBinTree(FManNode_t *nod);
FManNode_t *FindInBinTree(const char *chr);

double round(double r);

void SetGamePath(const char *pth);
const char *GetGamePath();
const char *GetGameTitle();

#endif // SYSTEM_H_INCLUDED
