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
#include <SDL/SDL_rotozoom.h>

#define STRBUFSIZE 0x400
#define MINIBUFSZ  32
#define PATHBUFSIZ 1024
#define MLIST_STACK 0x10

#define NEW(type) (type *)calloc(1, sizeof(type))
#define NEW_ARRAY(type, n) (type *)calloc((n), sizeof(type))

#define Z_PANIC(x...)                                           \
    {                                                           \
        fprintf(stderr, "[PANIC] IN FUNCTION %s:\n", __func__); \
        fprintf(stderr, "[PANIC] " x);                          \
        abort();                                               \
    }

#ifdef ENABLE_TRACING
#define LOG_DEBUG(f, ...) fprintf(stderr, "[DEBUG] (%s) " f, __func__, ##__VA_ARGS__)
#else
#define LOG_DEBUG(f, ...)
#endif

#define LOG_INFO(f, ...) fprintf(stderr, "[INFO] (%s) " f, __func__, ##__VA_ARGS__)
#define LOG_WARN(f, ...) fprintf(stderr, "[WARNING] (%s) " f, __func__, ##__VA_ARGS__)

typedef struct struct_action_res action_res_t;
typedef struct pzllst pzllst_t;
typedef struct ctrlnode ctrlnode_t;

//node structure
typedef struct MList_node_s
{
    struct MList_node_s *next; //pointer to next node
    struct MList_node_s *prev; //pointer to next node
    void *data;                //pointer to data
    size_t idx;
} MList_node;

//List structure
typedef struct
{
    MList_node *CurNode; //pointer to current node
    MList_node *Head;    //pointer to first node
    MList_node *Tail;    //pointer to last node
    size_t count;  //count of elements
    size_t indx;   //count of elements
    MList_node *Stack[MLIST_STACK];
    size_t stkpos;
    bool dontstp;
} MList;

//Linked-list functions
MList *CreateMList();
MList_node *AddToMList(MList *lst, void *item);
void StartMList(MList *lst);
void LastMList(MList *lst);
void NextMList(MList *lst);
void PrevMList(MList *lst);
void *DataMList(MList *lst);
void DeleteMList(MList *lst);
void FlushMList(MList *lst);
void DeleteCurrent(MList *lst);
bool EndOfMList(MList *lst);
bool PushMList(MList *lst);
bool PopMList(MList *lst);

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

#include "Decoder.h"
#include "Anims.h"
#include "Game.h"
#include "Render.h"
#include "Text.h"
#include "Sound.h"
#include "Mouse.h"
#include "Loader.h"
#include "Puzzle.h"
#include "Controls.h"
#include "Timer.h"
#include "Scripting.h"
#include "Inventory.h"
#include "Actions.h"
#include "Menu.h"

#endif // SYSTEM_H_INCLUDED
