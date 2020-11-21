#ifndef SYSTEM_H_INCLUDED
#define SYSTEM_H_INCLUDED

#define ZORKMID_VER "1.0"
#define CUR_GAME GAME_ZGI
// extern int CUR_GAME;

//if you plan to build engine with smpeg support
//#define SMPEG_SUPPORT

#define PREFERENCES_FILE (CUR_GAME == GAME_ZGI ? "INQUIS.INI" : "NEMESIS.INI")
#define SYS_STRINGS_FILE (CUR_GAME == GAME_ZGI ? "INQUIS.STR" : "NEMESIS.STR")
#define CTRL_SAVE_FILE   (CUR_GAME == GAME_ZGI ? "inquis.sav" : "nemesis.sav")
#define CTRL_SAVE_SAVES  (CUR_GAME == GAME_ZGI ? "inqsav%d.sav" : "nemsav%d.sav")
#define TIMER_DELAY      (CUR_GAME == GAME_ZGI ? 100 : 1000)

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
#define GAME_ZGI  1
#define GAME_NEM  2

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

// Default files if Zork.dir not found
#define ZORK_DIR_ZGI {"ADDON", "taunts", "ZASSETS1/GLOBAL", "ZASSETS1/CASTLE", "ZASSETS1/GRIFFQST", \
                  "ZASSETS1/JAIL", "ZASSETS1/LUCYQST", "ZASSETS1/MONAST1", "ZASSETS1/PORTFOOZ", \
                  "ZASSETS1/UNDERG1", "ZASSETS2/GLOBAL", "ZASSETS2/DMLAIR", "ZASSETS2/GRUEQST", \
                  "ZASSETS2/GUETECH", "ZASSETS2/HADES", "ZASSETS2/MONAST2", "ZASSETS2/UNDERG2", \
                  "ZGI/ZGI_MX", "ZGI/ZGI_MX/MINMAX", "zgi_mx", "CURSOR.ZFS", "SCRIPTS.ZFS", \
                  "DEATH.ZFS", "SUBTITLE.ZFS"}

// Default files if Zork.dir not found
#define ZORK_DIR_NEM {"Addon", "Addon/subpatch.zfs", "ZNEMMX", "ZNEMSCR", "ZNEMSCR/GSCR.ZFS", \
                  "ZNEMSCR/TSCR.ZFS", "ZNEMSCR/MSCR.ZFS", "ZNEMSCR/VSCR.ZFS", "ZNEMSCR/ASCR.ZFS", \
                  "ZNEMSCR/CSCR.ZFS", "ZNEMSCR/ESCR.ZFS", "ZNEMSCR/CURSOR.ZFS", "DATA1/ZASSETS", \
                  "DATA1/ZASSETS/GLOBAL", "DATA1/ZASSETS/GLOBAL/VENUS", "DATA3/ZASSETS/TEMPLE", \
                  "DATA1/ZASSETS/TEMPLE", "DATA2/ZASSETS", "DATA2/ZASSETS/GLOBAL2", \
                  "DATA2/ZASSETS/CONSERV", "DATA2/ZASSETS/MONAST", "DATA3/ZASSETS/GLOBAL3", \
                  "DATA3/ZASSETS/ASYLUM", "DATA3/ZASSETS/CASTLE", "DATA3/ZASSETS/ENDGAME" }

#define NEW(type) (type*)calloc(1, sizeof(type))
#define NEW_ARRAY(type, n) (type*)calloc((n), sizeof(type))

//System time functions
#define millisec SDL_GetTicks

#define SDL_TTF_NUM_VERSION (SDL_TTF_MAJOR_VERSION * 10000 + SDL_TTF_MINOR_VERSION * 100 + SDL_TTF_PATCHLEVEL)

#ifdef WIN32
static char *strcasestr(const char *h, const char *n)
{ /* h="haystack", n="needle" */
    const char *a = h, *e = n;

    if (!h || !*h || !n || !*n)
    {
        return 0;
    }
    while (*a && *e)
    {
        if ((toupper(((unsigned char)(*a)))) != (toupper(((unsigned char)(*e)))))
        {
            ++h;
            a = h;
            e = n;
        }
        else
        {
            ++a;
            ++e;
        }
    }
    return (char *)(*e ? 0 : h);
}
#endif

struct puzzlenode;
struct pzllst;
struct anim_preplay_node;
struct animnode;
struct struct_action_res;
struct struct_SubRect;
struct FManNode;
struct ctrlnode;

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
void InitMTime(float fps);
void ProcMTime();
bool GetBeat();

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

void LoadFiles(const char *dir);

void InitVkKeys();
uint8_t GetWinKey(SDLKey key);
SDLKey GetLastKey();

void InitFileManager(const char *dir);
const char *GetFilePath(const char *chr);
const char *GetExactFilePath(const char *chr);
TTF_Font *GetFontByName(char *name, int size);

int GetKeyBuffered(int indx);
bool CheckKeyboardMessage(const char *msg, int len);

bool isDirectory(const char *);
bool FileExist(const char *);
int32_t FileSize(const char *);

void UpdateDTime();
uint32_t GetDTime();

char *PrepareString(char *buf);
char *TrimLeft(char *buf);
char *TrimRight(char *buf);
char *GetParams(char *str);
int GetIntVal(char *chr);

#define strCMP(X, Y) strncasecmp(X, Y, strlen(Y))

void AddToBinTree(FManNode *nod);
FManNode *FindInBinTree(const char *chr);

double round(double r);

void SetGamePath(const char *pth);
const char *GetGamePath();

#endif // SYSTEM_H_INCLUDED
