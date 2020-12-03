#include "System.h"

#define MOUSE_UP     0
#define MOUSE_DOWN   1

static const struct
{
    const char *name;
    const char *zgi;
    const char *znem;
} Cursors[CURSOR_MAX] = {
    {"active",          "g0gbc011.zcr", "00act" },
    {"arrow",           "g0gac001.zcr", "arrow" },
    {"backward",        "g0gac021.zcr", "back"  },
    {"downarrow",       "g0gac031.zcr", "down"  },
    {"forward",         "g0gac041.zcr", "forw"  },
    {"handpt",          "g0gac051.zcr", "handpt"},
    {"handpu",          "g0gac061.zcr", "handpu"},
    {"hdown",           "g0gac071.zcr", "hdown" },
    {"hleft",           "g0gac081.zcr", "hleft" },
    {"hright",          "g0gac091.zcr", "hright"},
    {"hup",             "g0gac101.zcr", "hup"   },
    {"idle",            "g0gac011.zcr", "00idle"},
    {"leftarrow",       "g0gac111.zcr", "left"  },
    {"rightarrow",      "g0gac121.zcr", "right" },
    {"suggest_surround","g0gac131.zcr", "ssurr" },
    {"suggest_tilt",    "g0gac141.zcr", "stilt" },
    {"turnaround",      "g0gac151.zcr", "turn"  },
    {"zuparrow",        "g0gac161.zcr", "up"    },
};

static Cursor_t DefCursors[CURSOR_MAX][2];
static Cursor_t ObjCursors[2][2];
static Cursor_t *cursor;

static int current_cursor = 0;
static int current_obj_cursor = 0;
static bool draw_cursor = true;

static void FreeCursor(Cursor_t *cur)
{
    if (cur && cur->img)
    {
        SDL_FreeSurface(cur->img);
        cur->img = NULL;
    }
}

void Mouse_Init()
{
    char buffer[MINIBUFSIZE];

    for (int i = 0; i < 18; i++)
    {
        FreeCursor(&DefCursors[i][MOUSE_UP]);
        FreeCursor(&DefCursors[i][MOUSE_DOWN]);

        if (CURRENT_GAME == GAME_ZGI)
        {
            strcpy(buffer, Cursors[i].zgi);
            Loader_LoadZCR(buffer, &DefCursors[i][MOUSE_UP]);
            buffer[3] += 2;
            Loader_LoadZCR(buffer, &DefCursors[i][MOUSE_DOWN]);
        }
        else
        {
            sprintf(buffer, "%sa.zcr", Cursors[i].znem);
            Loader_LoadZCR(buffer, &DefCursors[i][MOUSE_UP]);
            sprintf(buffer, "%sb.zcr", Cursors[i].znem);
            Loader_LoadZCR(buffer, &DefCursors[i][MOUSE_DOWN]);
        }
    }

    cursor = &DefCursors[CURSOR_IDLE][MOUSE_UP];
}

static void LoadObjCursor(int index)
{
    FreeCursor(&ObjCursors[0][MOUSE_UP]);
    FreeCursor(&ObjCursors[0][MOUSE_DOWN]);
    FreeCursor(&ObjCursors[1][MOUSE_UP]);
    FreeCursor(&ObjCursors[1][MOUSE_DOWN]);

    current_obj_cursor = index;

    char buf[MINIBUFSIZE];

    if (CURRENT_GAME == GAME_ZGI)
    {
        sprintf(buf, "g0b%cc%2.2x1.zcr", 'a', current_obj_cursor);
        Loader_LoadZCR(buf, &ObjCursors[0][MOUSE_UP]);
        sprintf(buf, "g0b%cc%2.2x1.zcr", 'b', current_obj_cursor);
        Loader_LoadZCR(buf, &ObjCursors[1][MOUSE_UP]);
        sprintf(buf, "g0b%cc%2.2x1.zcr", 'c', current_obj_cursor);
        Loader_LoadZCR(buf, &ObjCursors[0][MOUSE_DOWN]);
        sprintf(buf, "g0b%cc%2.2x1.zcr", 'd', current_obj_cursor);
        Loader_LoadZCR(buf, &ObjCursors[1][MOUSE_DOWN]);
    }
    else
    {
        sprintf(buf, "%2.2didle%c.zcr", current_obj_cursor, 'a');
        Loader_LoadZCR(buf, &ObjCursors[0][MOUSE_UP]);
        sprintf(buf, "%2.2didle%c.zcr", current_obj_cursor, 'b');
        Loader_LoadZCR(buf, &ObjCursors[0][MOUSE_DOWN]);
        sprintf(buf, "%2.2dact%c.zcr", current_obj_cursor, 'a');
        Loader_LoadZCR(buf, &ObjCursors[1][MOUSE_UP]);
        sprintf(buf, "%2.2dact%c.zcr", current_obj_cursor, 'b');
        Loader_LoadZCR(buf, &ObjCursors[1][MOUSE_DOWN]);
    }
}

void Mouse_SetCursor(int index)
{
    int state = MouseDown(MOUSE_BTN_LEFT|MOUSE_BTN_RIGHT) ? MOUSE_DOWN : MOUSE_UP;

    if (index == CURSOR_OBJ_0)
        cursor = &ObjCursors[0][state];
    else if (index == CURSOR_OBJ_1)
        cursor = &ObjCursors[1][state];
    else
        cursor = &DefCursors[index][state];

    current_cursor = index;
}

bool Mouse_IsCurrentCur(int index)
{
    return index == current_cursor;
}

void Mouse_DrawCursor()
{
    if (GetgVarInt(SLOT_INVENTORY_MOUSE) != 0)
    {
        if (GetgVarInt(SLOT_INVENTORY_MOUSE) != current_obj_cursor)
            LoadObjCursor(GetgVarInt(SLOT_INVENTORY_MOUSE));

        if (Mouse_IsCurrentCur(CURSOR_ACTIVE) || Mouse_IsCurrentCur(CURSOR_HANDPU) || Mouse_IsCurrentCur(CURSOR_IDLE))
        {
            if (Mouse_IsCurrentCur(CURSOR_ACTIVE) || Mouse_IsCurrentCur(CURSOR_HANDPU))
                Mouse_SetCursor(CURSOR_OBJ_1);
            else
                Mouse_SetCursor(CURSOR_OBJ_0);
        }
    }

    if (Rend_MouseInGamescr())
    {
        if (Rend_GetRenderer() == RENDER_PANA)
        {
            if (MouseX() < GAMESCREEN_X + GAMESCREEN_P)
                Mouse_SetCursor(CURSOR_LEFTARROW);
            if (MouseX() > GAMESCREEN_X + GAMESCREEN_W - GAMESCREEN_P)
                Mouse_SetCursor(CURSOR_RIGHTARROW);
        }

        if (Rend_GetRenderer() == RENDER_TILT)
        {
            if (MouseY() < GAMESCREEN_Y + GAMESCREEN_P)
                Mouse_SetCursor(CURSOR_ZUPARROW);
            if (MouseY() > GAMESCREEN_Y + GAMESCREEN_H - GAMESCREEN_P)
                Mouse_SetCursor(CURSOR_DOWNARROW);
        }
    }

    if (cursor && draw_cursor)
    {
        Rend_BlitSurfaceXY(
            cursor->img,
            Rend_GetScreen(),
            MouseX() - cursor->ox,
            MouseY() - cursor->oy);
    }
}

int Mouse_GetCursorIndex(char *name)
{
    for (int i = 0; i < CURSOR_MAX; i++)
        if (str_equals(name, Cursors[i].name))
            return i;

    return CURSOR_IDLE;
}

void Mouse_ShowCursor()
{
    draw_cursor = true;
}

void Mouse_HideCursor()
{
    draw_cursor = false;
}

bool Mouse_InRect(int x, int y, int w, int h)
{
    return (MouseX() >= x) && (MouseX() <= x + w) && (MouseY() >= y) && (MouseY() <= y + h);
}

int Mouse_GetAngle(int x, int y, int x2, int y2) //not exact but near and fast
{
    if (x == x2 && y == y2)
        return -1;

    if (x2 == x)
        return (y > y2) ? 90 : 270;

    if (y2 == y)
        return (x < x2) ? 0 : 180;

    // get angle in range (-90, 90) using arctangens
    int dist_x = x2 - x;
    int dist_y = y2 - y;
    int angle = atan(dist_y / (float)abs(dist_x)) * 57;

    // do quarter modifications
    int quarter_index = (int)(dist_x < 0) | ((int)(dist_y > 0) << 1);

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
