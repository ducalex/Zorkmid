#ifndef MOUSE_H_INCLUDED
#define MOUSE_H_INCLUDED

typedef struct
{
    SDL_Surface *img;
    int16_t ox;
    int16_t oy;
} Cursor_t;

#define NUM_CURSORS 18

#define CURSOR_ACTIVE 0
#define CURSOR_DWNARR 3
#define CURSOR_HANDPU 6
#define CURSOR_IDLE 11
#define CURSOR_LEFT 12
#define CURSOR_RIGH 13
#define CURSOR_UPARR 17
#define CURSOR_OBJ_0 -1
#define CURSOR_OBJ_1 -2

void Mouse_LoadCursors();
void Mouse_SetCursor(int indx);
Cursor_t *Mouse_GetCursor(int indx);
bool Mouse_IsCurrentCur(int indx);
void Mouse_DrawCursor(int x, int y);
int Mouse_GetCursorIndex(char *name);
int Mouse_GetCurrentObjCur();
void Mouse_LoadObjCursor(int num);
void Mouse_ShowCursor();
void Mouse_HideCursor();
int16_t Mouse_GetAngle(int16_t x, int16_t y, int16_t x2, int16_t y2);
bool Mouse_InRect(int x, int y, int w, int h);

#endif // MOUSE_H_INCLUDED
