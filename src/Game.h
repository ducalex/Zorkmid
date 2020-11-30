#ifndef GAME_H_INCLUDED
#define GAME_H_INCLUDED

#include "System.h"

#define GAME_AUTO 0
#define GAME_ZGI 1
#define GAME_NEM 2

#define SYSTEM_STR_SAVEEXIST 23
#define SYSTEM_STR_SAVED 4
#define SYSTEM_STR_SAVEEMPTY 21
#define SYSTEM_STR_EXITPROMT 6

#define CHANGELOCATIONDELAY 2

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

#define MOUSE_BTN_LEFT  SDL_BUTTON(SDL_BUTTON_LEFT)
#define MOUSE_BTN_RIGHT SDL_BUTTON(SDL_BUTTON_RIGHT)

typedef struct
{
    uint8_t World;
    uint8_t Room;
    uint8_t Node;
    uint8_t View;
    int16_t X;
} Location_t;

void Game_Loop();
void Game_Update();
void Game_Quit();
void Game_Init(const char *path);
void Game_Relocate(uint8_t w, uint8_t r, uint8_t v1, uint8_t v2, int32_t X);

const char *Game_GetPath();
const char *GetGameTitle();
const char *Game_GetString(int indx);

float Game_GetFps();
bool Game_GetBeat();
uint32_t Game_GetDTime();

#define Game_Delay(ms) SDL_Delay(ms)
#define Game_GetTime() SDL_GetTicks()

void FlushKeybKey(SDLKey key);
void FlushMouseBtn(int btn);
bool KeyDown(SDLKey key);
bool KeyAnyHit();
bool KeyHit(SDLKey key);
SDLKey GetLastKey();
int MouseX();
int MouseY();
bool MouseDown(int btn);
bool MouseHit(int btn);
bool MouseUp(int btn);
bool MouseDblClk();
bool MouseMove();

void game_timed_debug_message(int32_t milsecs, const char *str);
void game_timed_message(int32_t milsecs, const char *str);
void game_delay_message(int32_t milsecs, const char *str);
bool game_question_message(const char *str);
void game_try_quit();

#endif // GAME_H_INCLUDED
