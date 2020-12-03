#ifndef SYSTEM_H_INCLUDED
#define SYSTEM_H_INCLUDED

#define ZORKMID_VER "1.0"

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

#define STRBUFSIZE  0x400
#define MINIBUFSIZE 0x20
#define PATHBUFSIZE 0x400
#define MLIST_STACK 0x10

#define Z_ALLOC(n) ({LOG_DEBUG("Allocating %d bytes\n", n); calloc(1, n);})
#define Z_FREE(obj) {LOG_DEBUG("Freeing bytes\n"); free((void*)obj); /*obj = NULL;*/}
#define NEW(type) (type *)Z_ALLOC(sizeof(type))
#define NEW_ARRAY(type, n) (type *)Z_ALLOC(sizeof(type) * (n))
#define DELETE(obj) Z_FREE(obj)

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

#define ASSERT(expr) {if (!(expr)) Z_PANIC("Assertion failed");}

#define MIN(a,b) ({__typeof__(a) _a = (a); __typeof__(b) _b = (b);_a < _b ? _a : _b; })
#define MAX(a,b) ({__typeof__(a) _a = (a); __typeof__(b) _b = (b);_a > _b ? _a : _b; })

typedef struct struct_action_res action_res_t;

typedef struct
{
    void **items;       // Items. Always check for NULL values (deleted items may leave holes)
    size_t length;      // Length of the list including deleted items (to be used in loops)
    size_t capacity;    // Max length before we need to realloc
    size_t blocksize;   // Block size to increase on realloc (in items)
    bool   is_heap;     // Tells us if the dynlist was created with CreateList() so we can free it
} dynlist_t;

dynlist_t *CreateList(size_t blocksize);
void ResizeList(dynlist_t *list);
void AddToList(dynlist_t *list, void *item);
void DeleteFromList(dynlist_t *list, int index);
void FlushList(dynlist_t *list);
void DeleteList(dynlist_t *list);

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
#include "Controls.h"
#include "Timer.h"
#include "Scripting.h"
#include "Inventory.h"
#include "Actions.h"
#include "Menu.h"

#endif // SYSTEM_H_INCLUDED
