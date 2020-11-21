#ifndef GAME_H_INCLUDED
#define GAME_H_INCLUDED

#include "System.h"

#define SYSTEM_STR_SAVEEXIST 23
#define SYSTEM_STR_SAVED 4
#define SYSTEM_STR_SAVEEMPTY 21
#define SYSTEM_STR_EXITPROMT 6

#define SYSTEM_STRINGS_NUM 64
#define CHANGELOCATIONDELAY 2

typedef struct
{
    uint8_t World;
    uint8_t Room;
    uint8_t Node;
    uint8_t View;
    int16_t X;
} Location_t;

void GameLoop();
void GameUpdate();
void GameQuit();
void GameInit();
void SetNeedLocate(uint8_t w, uint8_t r, uint8_t v1, uint8_t v2, int32_t X);
const char *GetGameTitle();

char *GetSystemString(int32_t indx);

void game_timed_debug_message(int32_t milsecs, const char *str);
void game_timed_message(int32_t milsecs, const char *str);
void game_delay_message(int32_t milsecs, const char *str);
bool game_question_message(const char *str);
void game_try_quit();

#endif // GAME_H_INCLUDED
