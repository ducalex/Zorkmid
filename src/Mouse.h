#ifndef MOUSE_H_INCLUDED
#define MOUSE_H_INCLUDED

typedef struct
{
    SDL_Surface *img;
    int16_t ox;
    int16_t oy;
} Cursor_t;

enum {
    CURSOR_ACTIVE = 0,
    CURSOR_ARROW,
    CURSOR_BACKWARD,
    CURSOR_DOWNARROW,
    CURSOR_FORWARD,
    CURSOR_HANDPT,
    CURSOR_HANDPU,
    CURSOR_HDOWN,
    CURSOR_HLEFT,
    CURSOR_HRIGHT,
    CURSOR_HUP,
    CURSOR_IDLE,
    CURSOR_LEFTARROW,
    CURSOR_RIGHTARROW,
    CURSOR_SUGGEST_SURROUND,
    CURSOR_SUGGEST_TILT,
    CURSOR_TURNAROUND,
    CURSOR_ZUPARROW,
    CURSOR_MAX
};

#define CURSOR_OBJ_0 -1
#define CURSOR_OBJ_1 -2

void Mouse_Init();
void Mouse_SetCursor(int indx);
bool Mouse_IsCurrentCur(int indx);
void Mouse_DrawCursor();
int Mouse_GetCursorIndex(char *name);
void Mouse_ShowCursor();
void Mouse_HideCursor();
int Mouse_GetAngle(int x, int y, int x2, int y2);
bool Mouse_InRect(int x, int y, int w, int h);

#endif // MOUSE_H_INCLUDED
