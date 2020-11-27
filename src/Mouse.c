#include "System.h"

enum {
    CURSOR_STATE_UP = 0,
    CURSOR_STATE_DW,
    CURSOR_STATE_CT
};

static const char *CurNames[] = {
    "active", "arrow", "backward", "downarrow", "forward", "handpt",
    "handpu", "hdown", "hleft", "hright", "hup", "idle", "leftarrow",
    "rightarrow", "suggest_surround", "suggest_tilt", "turnaround",
    "zuparrow"};

static const char *CurFiles_zgi[] = {
    "g0gbc011.zcr", "g0gac001.zcr", "g0gac021.zcr", "g0gac031.zcr", "g0gac041.zcr", "g0gac051.zcr",
    "g0gac061.zcr", "g0gac071.zcr", "g0gac081.zcr", "g0gac091.zcr", "g0gac101.zcr", "g0gac011.zcr",
    "g0gac111.zcr", "g0gac121.zcr", "g0gac131.zcr", "g0gac141.zcr", "g0gac151.zcr", "g0gac161.zcr"};

static const char *CurFiles_znemesis[] = {
    "00act", "arrow", "back", "down", "forw", "handpt", "handpu", "hdown", "hleft",
    "hright", "hup", "00idle", "left", "right", "ssurr", "stilt", "turn", "up"};

static Cursor_t DefCursors[NUM_CURSORS][CURSOR_STATE_CT];
static Cursor_t ObjCursors[2][CURSOR_STATE_CT];
static Cursor_t *cur;

static int8_t cursor_index = 0;
static int current_obj_cur = 0;
static bool DrawCursor = true;

static void Free_Cursor(Cursor_t *cur)
{
    if (cur && cur->img)
    {
        SDL_FreeSurface(cur->img);
        cur->img = NULL;
    }
}

void Mouse_LoadCursors()
{
    char buffer[MINIBUFSZ];

    for (int i = 0; i < 18; i++)
    {
        Free_Cursor(&DefCursors[i][CURSOR_STATE_UP]);

        if (CUR_GAME == GAME_ZGI)
        {
            Loader_LoadZCR(CurFiles_zgi[i], &DefCursors[i][CURSOR_STATE_UP]);
            strcpy(buffer, CurFiles_zgi[i]);
            buffer[3] += 2;
            Loader_LoadZCR(buffer, &DefCursors[i][CURSOR_STATE_DW]);
        }
        else
        {
            sprintf(buffer, "%sa.zcr", CurFiles_znemesis[i]);
            Loader_LoadZCR(buffer, &DefCursors[i][CURSOR_STATE_UP]);
            sprintf(buffer, "%sb.zcr", CurFiles_znemesis[i]);
            Loader_LoadZCR(buffer, &DefCursors[i][CURSOR_STATE_DW]);
        }
    }

    cur = &DefCursors[CURSOR_IDLE][0];
}

void Mouse_LoadObjCursor(int num)
{
    Free_Cursor(&ObjCursors[0][CURSOR_STATE_UP]);
    Free_Cursor(&ObjCursors[0][CURSOR_STATE_DW]);
    Free_Cursor(&ObjCursors[1][CURSOR_STATE_UP]);
    Free_Cursor(&ObjCursors[1][CURSOR_STATE_DW]);

    current_obj_cur = num;

    char buf[MINIBUFSZ];

    if (CUR_GAME == GAME_ZGI)
    {
        sprintf(buf, "g0b%cc%2.2x1.zcr", 'a', current_obj_cur);
        Loader_LoadZCR(buf, &ObjCursors[0][CURSOR_STATE_UP]);
        sprintf(buf, "g0b%cc%2.2x1.zcr", 'b', current_obj_cur);
        Loader_LoadZCR(buf, &ObjCursors[1][CURSOR_STATE_UP]);
        sprintf(buf, "g0b%cc%2.2x1.zcr", 'c', current_obj_cur);
        Loader_LoadZCR(buf, &ObjCursors[0][CURSOR_STATE_DW]);
        sprintf(buf, "g0b%cc%2.2x1.zcr", 'd', current_obj_cur);
        Loader_LoadZCR(buf, &ObjCursors[1][CURSOR_STATE_DW]);
    }
    else
    {
        sprintf(buf, "%2.2didle%c.zcr", current_obj_cur, 'a');
        Loader_LoadZCR(buf, &ObjCursors[0][CURSOR_STATE_UP]);
        sprintf(buf, "%2.2didle%c.zcr", current_obj_cur, 'b');
        Loader_LoadZCR(buf, &ObjCursors[0][CURSOR_STATE_DW]);
        sprintf(buf, "%2.2dact%c.zcr", current_obj_cur, 'a');
        Loader_LoadZCR(buf, &ObjCursors[1][CURSOR_STATE_UP]);
        sprintf(buf, "%2.2dact%c.zcr", current_obj_cur, 'b');
        Loader_LoadZCR(buf, &ObjCursors[1][CURSOR_STATE_DW]);
    }
}

void Mouse_SetCursor(int indx)
{
    int8_t stt = CURSOR_STATE_UP;

    if (MouseDown(MOUSE_BTN_LEFT) || MouseDown(MOUSE_BTN_RIGHT))
        stt = CURSOR_STATE_DW;

    if (indx == CURSOR_OBJ_0)
        cur = &ObjCursors[0][stt];
    else if (indx == CURSOR_OBJ_1)
        cur = &ObjCursors[1][stt];
    else
        cur = &DefCursors[indx][stt];

    cursor_index = indx;
}

Cursor_t *Mouse_GetCursor(int indx)
{
    int8_t stt = CURSOR_STATE_UP;

    if (MouseDown(MOUSE_BTN_LEFT) || MouseDown(MOUSE_BTN_RIGHT))
        stt = CURSOR_STATE_DW;

    if (indx == CURSOR_OBJ_0)
        return &ObjCursors[0][stt];
    else if (indx == CURSOR_OBJ_1)
        return &ObjCursors[1][stt];
    else
        return &DefCursors[indx][stt];
}

bool Mouse_IsCurrentCur(int indx)
{
    return indx == cursor_index;
}

void Mouse_DrawCursor(int x, int y)
{
    if (cur && DrawCursor)
    {
        Rend_BlitSurfaceXY(cur->img, Rend_GetScreen(), x - cur->ox, y - cur->oy);
    }
}

int Mouse_GetCursorIndex(char *name)
{
    for (int i = 0; i < NUM_CURSORS; i++)
        if (str_equals(name, CurNames[i]))
            return i;

    return CURSOR_IDLE;
}

int Mouse_GetCurrentObjCur()
{
    return current_obj_cur;
}

void Mouse_ShowCursor()
{
    DrawCursor = true;
}

void Mouse_HideCursor()
{
    DrawCursor = false;
}

bool Mouse_InRect(int x, int y, int w, int h)
{
    return (MouseX() >= x) && (MouseX() <= x + w) && (MouseY() >= y) && (MouseY() <= y + h);
}

int Mouse_GetAngle(int x, int y, int x2, int y2) //not exact but near and fast
{
    if (x == x2 && y == y2)
        return -1;
    else if (x2 == x)
    {
        if (y > y2)
            return 90;
        else
            return 270;
    }
    else if (y2 == y)
    {
        if (x < x2)
            return 0;
        else
            return 180;
    }

    // get angle in range (-90, 90) using arctangens
    int16_t dist_x = x2 - x;
    int16_t dist_y = y2 - y;
    int16_t angle = atan(dist_y / (float)abs(dist_x)) * 57;

    /* *(mul)57 ~ 180/3.1415 */

    // do quarter modifications
    int16_t quarter_index = (int16_t)(dist_x < 0) | ((int16_t)(dist_y > 0) << 1);

    switch (quarter_index)
    {
    case 0:
        angle = -angle;
        break;
    case 1:
        angle = angle + 180;
        break;
    case 2:
        angle = 360 - angle;
        break;
    case 3:
        angle = 180 + angle;
        break;
    }

    return angle;
}
