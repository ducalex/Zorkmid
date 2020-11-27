#include "System.h"

#include <SDL/SDL_rotozoom.h>

//Graphics
int WINDOW_W = 0;
int WINDOW_H = 0;
int GAMESCREEN_W = 0;
int GAMESCREEN_P = 0;
int GAMESCREEN_H = 0;
int GAMESCREEN_X = 0;
int GAMESCREEN_Y = 0;
int GAMESCREEN_FLAT_X = 0;
int WIDESCREEN = 0;
int FULLSCREEN = 0;

static float tilt_angle = 60.0;
static float tilt_linscale = 1.0;
static int32_t tilt_gap = 0;

static float mgamma = 1.0;

static uint8_t Renderer = RENDER_FLAT;

static MList *sublist = NULL;
static int32_t subid = 0;

static SDL_Surface *screen;      // Game window surface
static SDL_Surface *scrbuf;      // Surface loaded by setscreen, all changes by setpartialscreen and other similar modify this surface.
static SDL_Surface *tempbuf;     // This surface used for effects(region action) and control draws.
static SDL_Surface *viewportbuf; // This surface used for rendered viewport image with renderer processing.

static int32_t RenderDelay = 0;

static struct
{
    int32_t x;
    int32_t y;
} render_table[1024][1024];

static int32_t new_render_table[1024 * 1024];

static int32_t *view_X;

static int32_t pana_PanaWidth = 1800;
static bool pana_ReversePana = false;
static float pana_angle = 60.0, pana_linscale = 1.00;
static int32_t pana_Zero = 0;

static const int FiveBitToEightBitLookupTable[32] = {
    0, 8, 16, 24, 32, 41, 49, 57, 65, 74, 82, 90, 98, 106, 115, 123,
    131, 139, 148, 156, 164, 172, 180, 189, 197, 205, 213, 222, 230, 238, 246, 255};

static const float sin_tab[] = {
    0.000000, 0.012272, 0.024541, 0.036807,
    0.049068, 0.061321, 0.073565, 0.085797,
    0.098017, 0.110222, 0.122411, 0.134581,
    0.146730, 0.158858, 0.170962, 0.183040,
    0.195090, 0.207111, 0.219101, 0.231058,
    0.242980, 0.254866, 0.266713, 0.278520,
    0.290285, 0.302006, 0.313682, 0.325310,
    0.336890, 0.348419, 0.359895, 0.371317,
    0.382683, 0.393992, 0.405241, 0.416430,
    0.427555, 0.438616, 0.449611, 0.460539,
    0.471397, 0.482184, 0.492898, 0.503538,
    0.514103, 0.524590, 0.534998, 0.545325,
    0.555570, 0.565732, 0.575808, 0.585798,
    0.595699, 0.605511, 0.615232, 0.624860,
    0.634393, 0.643832, 0.653173, 0.662416,
    0.671559, 0.680601, 0.689541, 0.698376,
    0.707107, 0.715731, 0.724247, 0.732654,
    0.740951, 0.749136, 0.757209, 0.765167,
    0.773010, 0.780737, 0.788346, 0.795837,
    0.803208, 0.810457, 0.817585, 0.824589,
    0.831470, 0.838225, 0.844854, 0.851355,
    0.857729, 0.863973, 0.870087, 0.876070,
    0.881921, 0.887640, 0.893224, 0.898674,
    0.903989, 0.909168, 0.914210, 0.919114,
    0.923880, 0.928506, 0.932993, 0.937339,
    0.941544, 0.945607, 0.949528, 0.953306,
    0.956940, 0.960431, 0.963776, 0.966976,
    0.970031, 0.972940, 0.975702, 0.978317,
    0.980785, 0.983105, 0.985278, 0.987301,
    0.989177, 0.990903, 0.992480, 0.993907,
    0.995185, 0.996313, 0.997290, 0.998118,
    0.998795, 0.999322, 0.999699, 1.000000};

static inline float fastSin(float x)
{
    int idx = 0;
    if (x < 0)
        idx = x * (-81.487332253);
    else
        idx = x * 81.487332253; //512/(2*pi)

    idx += 1024;
    idx %= 512;
    //int32_t idx2 = idx_untrimmed % 512;

    if (idx < 128)
        return sin_tab[idx];
    else if (idx < 256)
        return sin_tab[255 - idx];
    else if (idx < 384)
        return -sin_tab[idx - 256];
    else if (idx < 512)
        return -sin_tab[511 - idx];
    else
        return 0;
}

//Method using Log Base 2 Approximation
static inline float fastSqrt(float x)
{
    union
    {
        int i;
        float x;
    } u;
    u.x = x;
    u.i = (1 << 29) + (u.i >> 1) - (1 << 22);
    return u.x;
}

#define EFFECTS_MAX_CNT 32
#define EFFECT_WAVE 1
#define EFFECT_LIGH 2
#define EFFECT_9 4

typedef struct //water
{
    int32_t frame;
    int32_t frame_cnt;
    int8_t **ampls;
    SDL_Surface *surface;
} effect0_t;

typedef struct //lightning
{
    int8_t *map; // 0 - no; !0 - draw
    int8_t sign;
    int32_t stp;
    int32_t maxstp;
    SDL_Surface *surface;
} effect1_t;

typedef struct
{
    int8_t *cloud_mod;
    SDL_Surface *cloud;
    SDL_Surface *mask;
    SDL_Surface *mapping;
} effect9_t;

typedef struct
{
    int32_t type;
    int32_t delay;
    int32_t time;
    int32_t w;
    int32_t h;
    int32_t x;
    int32_t y;
    union effect
    {
        effect0_t ef0;
        effect1_t ef1;
        effect9_t ef9;
    } effect;
} effect_t;

static effect_t Effects[EFFECTS_MAX_CNT];

static void DeleteEffect(uint32_t index);
static void Rend_EF_9_Draw(effect_t *ef);
static void Rend_EF_Wave_Draw(effect_t *ef);
static void Rend_EF_Light_Draw(effect_t *ef);

float Rend_GetRendererAngle()
{
    if (Renderer == RENDER_PANA)
        return pana_angle;
    else if (Renderer == RENDER_TILT)
        return tilt_angle;
    else
        return 1.0;
}

float Rend_GetRendererLinscale()
{
    if (Renderer == RENDER_PANA)
        return pana_linscale;
    else if (Renderer == RENDER_TILT)
        return tilt_linscale;
    else
        return 1.0;
}

static void SetRendererAngle(float angle)
{
    if (Renderer == RENDER_PANA)
        pana_angle = angle;
    else if (Renderer == RENDER_TILT)
        tilt_angle = angle;
}

static void SetRendererLinscale(float lin)
{
    if (Renderer == RENDER_PANA)
        pana_linscale = lin;
    else if (Renderer == RENDER_TILT)
        tilt_linscale = lin;
}

static void SetRendererTable()
{
    if (Renderer == RENDER_PANA)
        Rend_pana_SetTable();
    else if (Renderer == RENDER_TILT)
        Rend_tilt_SetTable();
}

void Rend_SetRenderer(int meth)
{
    Renderer = meth;
    pana_ReversePana = false;
    Rend_tilt_SetLinscale(0.65);
    Rend_tilt_SetAngle(60.0);
    Rend_pana_SetLinscale(0.55);
    Rend_pana_SetAngle(60.0);
}

int Rend_GetRenderer()
{
    return Renderer;
}

static void ProcessCursor()
{
    if (GetgVarInt(SLOT_INVENTORY_MOUSE) != 0)
    {
        if (GetgVarInt(SLOT_INVENTORY_MOUSE) != Mouse_GetCurrentObjCur())
            Mouse_LoadObjCursor(GetgVarInt(SLOT_INVENTORY_MOUSE));

        if (Mouse_IsCurrentCur(CURSOR_ACTIVE) || Mouse_IsCurrentCur(CURSOR_HANDPU) || Mouse_IsCurrentCur(CURSOR_IDLE))
        {
            if (Mouse_IsCurrentCur(CURSOR_ACTIVE) || Mouse_IsCurrentCur(CURSOR_HANDPU))
                Mouse_SetCursor(CURSOR_OBJ_1);
            else
                Mouse_SetCursor(CURSOR_OBJ_0);
        }
    }

    if (Renderer == RENDER_PANA)
        if (Rend_MouseInGamescr())
        {
            if (MouseX() < GAMESCREEN_X + GAMESCREEN_P)
                Mouse_SetCursor(CURSOR_LEFT);
            if (MouseX() > GAMESCREEN_X + GAMESCREEN_W - GAMESCREEN_P)
                Mouse_SetCursor(CURSOR_RIGH);
        }
    if (Renderer == RENDER_TILT)
        if (Rend_MouseInGamescr())
        {
            if (MouseY() < GAMESCREEN_Y + GAMESCREEN_P)
                Mouse_SetCursor(CURSOR_UPARR);
            if (MouseY() > GAMESCREEN_Y + GAMESCREEN_H - GAMESCREEN_P)
                Mouse_SetCursor(CURSOR_DWNARR);
        }

    Mouse_DrawCursor(MouseX(), MouseY());
}

static void PanaMouseInteract()
{
    int32_t tt = *view_X;

    if (KeyDown(SDLK_LEFT))
    {
        int32_t speed = GetgVarInt(SLOT_KBD_ROTATE_SPEED) / 10;
        *view_X -= (pana_ReversePana == false ? speed : -speed);
    }
    else if (KeyDown(SDLK_RIGHT))
    {
        int32_t speed = GetgVarInt(SLOT_KBD_ROTATE_SPEED) / 10;
        *view_X += (pana_ReversePana == false ? speed : -speed);
    }

    if (Rend_MouseInGamescr())
    {
        if (MouseX() > GAMESCREEN_X + GAMESCREEN_W - GAMESCREEN_P)
        {
            int32_t mspeed = GetgVarInt(SLOT_PANAROTATE_SPEED) >> 4;
            int32_t param = (((MouseX() - GAMESCREEN_X - GAMESCREEN_W + GAMESCREEN_P) << 7) / GAMESCREEN_P * mspeed) >> 7;

            *view_X += (pana_ReversePana == false ? param : -param);
        }

        if (MouseX() < GAMESCREEN_X + GAMESCREEN_P)
        {
            int32_t mspeed = GetgVarInt(SLOT_PANAROTATE_SPEED) >> 4;
            int32_t param = (((GAMESCREEN_X + GAMESCREEN_P - MouseX()) << 7) / GAMESCREEN_P * mspeed) >> 7;

            *view_X -= (pana_ReversePana == false ? param : -param);
        }
    }

    if (tt < pana_Zero)
    {
        if (*view_X >= pana_Zero)
            SetgVarInt(SLOT_ROUNDS, GetgVarInt(SLOT_ROUNDS) + 1);
    }
    else if (tt > pana_Zero)
    {
        if (*view_X <= pana_Zero)
            SetgVarInt(SLOT_ROUNDS, GetgVarInt(SLOT_ROUNDS) - 1);
    }
    else if (tt == pana_Zero)
    {
        if (*view_X < pana_Zero)
            SetgVarInt(SLOT_ROUNDS, GetgVarInt(SLOT_ROUNDS) - 1);
        else if (*view_X > pana_Zero)
            SetgVarInt(SLOT_ROUNDS, GetgVarInt(SLOT_ROUNDS) + 1);
    }

    if (*view_X >= pana_PanaWidth)
        *view_X %= pana_PanaWidth;
    if (*view_X < 0)
        *view_X += pana_PanaWidth;
}

static void TiltMouseInteract()
{
    if (KeyDown(SDLK_UP))
    {
        int32_t speed = GetgVarInt(SLOT_KBD_ROTATE_SPEED) / 10;
        *view_X -= (pana_ReversePana == false ? speed : -speed);
    }
    else if (KeyDown(SDLK_DOWN))
    {
        int32_t speed = GetgVarInt(SLOT_KBD_ROTATE_SPEED) / 10;
        *view_X += (pana_ReversePana == false ? speed : -speed);
    }

    if (Rend_MouseInGamescr())
    {
        if (MouseY() > GAMESCREEN_Y + GAMESCREEN_H - GAMESCREEN_P)
        {
            int32_t mspeed = GetgVarInt(SLOT_PANAROTATE_SPEED) >> 4;
            int32_t param = (((MouseY() - GAMESCREEN_Y - GAMESCREEN_H + GAMESCREEN_P) << 7) / GAMESCREEN_P * mspeed) >> 7;

            *view_X += (pana_ReversePana == false ? param : -param);
        }
        if (MouseY() < GAMESCREEN_Y + GAMESCREEN_P)
        {
            int32_t mspeed = GetgVarInt(SLOT_PANAROTATE_SPEED) >> 4;
            int32_t param = (((GAMESCREEN_Y + GAMESCREEN_P - MouseY()) << 7) / GAMESCREEN_P * mspeed) >> 7;

            *view_X -= (pana_ReversePana == false ? param : -param);
        }
    }

    if (*view_X >= (pana_PanaWidth - tilt_gap))
        *view_X = pana_PanaWidth - tilt_gap;
    if (*view_X <= tilt_gap)
        *view_X = tilt_gap;
}

void Rend_pana_SetAngle(float angle)
{
    pana_angle = angle;
}

void Rend_pana_SetLinscale(float linscale)
{
    pana_linscale = linscale;
}

void Rend_pana_SetZeroPoint(int32_t point)
{
    pana_Zero = point;
}

void Rend_SetDelay(int32_t delay)
{
    RenderDelay = delay;
}

void Rend_indexer()
{
    int32_t previndx = new_render_table[0];

    for (int ff = 1; ff < GAMESCREEN_H * GAMESCREEN_W; ff++)
    {
        int32_t curindx = new_render_table[ff];
        new_render_table[ff] = curindx - previndx;
        previndx = curindx;
    }
}

void Rend_pana_SetTable()
{
    float angl = pana_angle;
    float k = pana_linscale;
    memset(render_table, 0, sizeof(render_table));

    int32_t yy = GAMESCREEN_H;
    int32_t ww = GAMESCREEN_W;

    double half_w = (double)ww / 2.0;
    double half_h = (double)yy / 2.0;

    double angle = (angl * 3.14159265 / 180.0);
    double hhdtan = half_h / tan(angle);
    double tandhh = tan(angle) / half_h;

    for (int x = 0; x < ww; x++)
    {
        double poX = (double)x - half_w + 0.01; //0.01 - for zero tan/atan issue (vertical line on half of screen)

        double tmx = atan(poX * tandhh);
        double nX = k * hhdtan * tmx;
        double nn = cos(tmx);
        double nhw = half_h * nn * hhdtan * tandhh * 2.0;

        int32_t relx = floor(nX); // + half_w);
        double yk = nhw / (double)yy;

        double et2 = ((double)yy - nhw) / 2.0;

        for (int y = 0; y < yy; y++)
        {
            double et1 = (double)y * yk;

            int32_t newx = relx;
            render_table[x][y].x = newx; //pixel index

            newx += GAMESCREEN_W_2;

            int32_t newy = floor(et2 + et1);

            render_table[x][y].y = newy; //pixel index

            if (newx < 0)
                newx = 0;
            if (newx >= GAMESCREEN_W)
                newx = GAMESCREEN_W - 1;
            if (newy < 0)
                newy = 0;
            if (newy >= GAMESCREEN_H)
                newy = GAMESCREEN_H - 1;

            new_render_table[x + y * GAMESCREEN_W] = newx + newy * GAMESCREEN_W; //pixel index
        }
    }

    Rend_indexer();
}

void Rend_SetVideoMode(int w, int h, int full, int wide)
{
    wide = wide >= 0 ? wide : WIDESCREEN;
    full = full >= 0 ? full : FULLSCREEN;
    w = w >= 0 ? w : WINDOW_W;
    h = h >= 0 ? h : WINDOW_H;

    if (screen && (WINDOW_W == w && WINDOW_H == h && WIDESCREEN == wide && FULLSCREEN == full))
    {
        // No changes to make
        return;
    }

    LOG_INFO("Changing video mode to: Size: %dx%d, Wide: %d, Full: %d\n", w, h, full, wide);

    if (CUR_GAME == GAME_ZGI)
    {
        GAMESCREEN_W = 640;
        GAMESCREEN_H = 344;
        if (WIDESCREEN)
        {
            GAMESCREEN_W = 854;
            GAMESCREEN_FLAT_X = 107;
        }
    }
    else
    {
        GAMESCREEN_W = 512;
        GAMESCREEN_H = 320;
    }

    SDL_FreeSurface(tempbuf);
    SDL_FreeSurface(viewportbuf);

    screen = SDL_SetVideoMode(w, h, 0, SDL_SWSURFACE | (full ? SDL_FULLSCREEN : SDL_RESIZABLE));
    tempbuf = Rend_CreateSurface(GAMESCREEN_W, GAMESCREEN_H, 0);
    viewportbuf = Rend_CreateSurface(GAMESCREEN_W, GAMESCREEN_H, 0);

    WINDOW_W = screen->w;
    WINDOW_H = screen->h;
    FULLSCREEN = (screen->flags & SDL_FULLSCREEN) != 0;
    WIDESCREEN = GAMESCREEN_W > 640;
    GAMESCREEN_X = (WINDOW_W - GAMESCREEN_W) / 2;
    GAMESCREEN_Y = (WINDOW_H - GAMESCREEN_H) / 2;
    GAMESCREEN_FLAT_X = 0;
    GAMESCREEN_P = 60;
}

void Rend_InitGraphics(int full, int wide)
{
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
    {
        Z_PANIC("Unable to init SDL: %s\n", SDL_GetError());
    }

    sublist = CreateMList();
    view_X = GetDirectgVarInt(SLOT_VIEW_POS);
    tilt_gap = GAMESCREEN_H_2;

    Rend_SetVideoMode(640, 480, full, wide);

    char buffer[128];
    sprintf(buffer, "Zorkmid: %s [build: " __DATE__ " " __TIME__ "]", GetGameTitle());
    SDL_WM_SetCaption(buffer, NULL);
    SDL_ShowCursor(SDL_DISABLE);
    TTF_Init();
}

bool Rend_LoadGamescr(const char *file)
{
    if (scrbuf)
        SDL_FreeSurface(scrbuf);

    scrbuf = Loader_LoadGFX(file, Rend_GetRenderer() == RENDER_PANA, -1);

    Rend_FillRect(tempbuf, NULL, 0, 0, 0);

    if (!scrbuf) // no errors if no screen
    {
        LOG_WARN("Loader_LoadGFX(%s) failed: %s\n", file, SDL_GetError());
        scrbuf = Rend_CreateSurface(GAMESCREEN_W, GAMESCREEN_H, 0);
    }

    if (Renderer != RENDER_TILT)
        pana_PanaWidth = scrbuf->w;
    else
        pana_PanaWidth = scrbuf->h;

    return true;
}

bool Rend_MouseInGamescr()
{
    return Mouse_InRect(GAMESCREEN_X, GAMESCREEN_Y, GAMESCREEN_W, GAMESCREEN_H);
}

int Rend_GetMouseGameX()
{
    int32_t tmp, tmpl;

    switch (Renderer)
    {
    case RENDER_FLAT:
        return MouseX() - GAMESCREEN_X - GAMESCREEN_FLAT_X;

    case RENDER_PANA:
        tmp = MouseY() - GAMESCREEN_Y;
        tmpl = MouseX() - GAMESCREEN_X;

        if (tmp >= 0 && tmp < GAMESCREEN_H && tmpl >= 0 && tmpl < GAMESCREEN_W)
            tmpl = render_table[tmpl][tmp].x;
        else
            tmpl = 0;

        tmpl += *view_X;
        if (tmpl < 0)
            tmpl += pana_PanaWidth;
        else if (tmpl > pana_PanaWidth)
            tmpl -= pana_PanaWidth;

        return tmpl;

    case RENDER_TILT:
        tmp = MouseY() - GAMESCREEN_Y;
        tmpl = MouseX() - GAMESCREEN_X;

        if (tmp >= 0 && tmp < GAMESCREEN_H && tmpl >= 0 && tmpl < GAMESCREEN_W)
            tmpl = render_table[tmpl][tmp].x;
        else
            tmpl = 0;

        //tmpl += GAMESCREEN_W_2;

        if (tmpl < 0)
            tmpl += GAMESCREEN_W;
        else if (tmpl > GAMESCREEN_W)
            tmpl -= GAMESCREEN_W;

        return tmpl;

    default:
        return MouseX() - GAMESCREEN_X;
    }
}

int Rend_GetMouseGameY()
{
    int32_t tmp, tmpl;

    switch (Renderer)
    {
    case RENDER_FLAT:
        return MouseY() - GAMESCREEN_Y;

    case RENDER_PANA:
        tmp = MouseY() - GAMESCREEN_Y;
        tmpl = MouseX() - GAMESCREEN_X;
        if (tmp >= 0 && tmp < GAMESCREEN_H && tmpl >= 0 && tmpl < GAMESCREEN_W)
            return render_table[tmpl][tmp].y;
        return tmp;

    case RENDER_TILT:
        tmp = MouseY() - GAMESCREEN_Y;
        tmpl = MouseX() - GAMESCREEN_X;
        if (tmp >= 0 && tmp < GAMESCREEN_H && tmpl >= 0 && tmpl < GAMESCREEN_W)
            tmpl = render_table[tmpl][tmp].y;
        else
            tmpl = 0;

        tmpl += *view_X;

        if (tmpl > pana_PanaWidth)
            tmpl = pana_PanaWidth;
        else if (tmpl < 0)
            tmpl = 0;

        return tmpl;

    default:
        return MouseY() - GAMESCREEN_Y;
    }
}

void Rend_SetReversePana(bool pana)
{
    pana_ReversePana = pana;
}

void Rend_MouseInteractOfRender()
{
    if (GetgVarInt(SLOT_PANAROTATE_SPEED) == 0)
        SetgVarInt(SLOT_PANAROTATE_SPEED, 700);
    if (Renderer == RENDER_PANA)
        PanaMouseInteract();
    else if (Renderer == RENDER_TILT)
        TiltMouseInteract();
}

int Rend_GetPanaWidth()
{
    return pana_PanaWidth;
}

void Rend_RenderFunc()
{
    if (RenderDelay > 0)
    {
        RenderDelay--;
        return;
    }

    Rend_FillRect(screen, NULL, 0, 0, 0);

    //pre-rendered
    if (Renderer == RENDER_FLAT)
    {
        Rend_BlitSurfaceXY(scrbuf, tempbuf, GAMESCREEN_FLAT_X, 0);
    }
    else if (Renderer == RENDER_PANA)
    {
        Rend_BlitSurfaceXY(scrbuf, tempbuf, GAMESCREEN_W_2 - *view_X, 0);
        if (*view_X < GAMESCREEN_W_2)
        {
            Rend_BlitSurfaceXY(scrbuf, tempbuf, GAMESCREEN_W_2 - (*view_X + pana_PanaWidth), 0);
        }
        else if (pana_PanaWidth - *view_X < GAMESCREEN_W_2)
        {
            Rend_BlitSurfaceXY(scrbuf, tempbuf, GAMESCREEN_W_2 + pana_PanaWidth - *view_X, 0);
        }
    }
    else if (Renderer == RENDER_TILT)
    {
        Rend_BlitSurfaceXY(scrbuf, tempbuf, 0, GAMESCREEN_H_2 - *view_X);
    }

    //draw dynamic controls
    Controls_Draw();

    //effect-processor
    for (int i = 0; i < EFFECTS_MAX_CNT; i++)
    {
        if (Effects[i].type == EFFECT_WAVE)
            Rend_EF_Wave_Draw(&Effects[i]);
        else if (Effects[i].type == EFFECT_LIGH)
            Rend_EF_Light_Draw(&Effects[i]);
        else if (Effects[i].type == EFFECT_9)
            Rend_EF_9_Draw(&Effects[i]);
    }

    //Apply renderer distortion
    if (Renderer == RENDER_FLAT)
    {
        Rend_BlitSurfaceXY(tempbuf, viewportbuf, 0, 0);
    }
    else if (Renderer == RENDER_PANA || RenderDelay == RENDER_TILT)
    {
        SDL_LockSurface(tempbuf);
        SDL_LockSurface(viewportbuf);

        uint16_t *nww = (uint16_t *)viewportbuf->pixels;
        uint16_t *old = (uint16_t *)tempbuf->pixels;
        int32_t *ofs = new_render_table;
        for (int ai = 0; ai < GAMESCREEN_H * GAMESCREEN_W; ai++)
        {
            old += *ofs;
            *nww = *old;
            nww++;
            ofs++;
        }

        SDL_UnlockSurface(tempbuf);
        SDL_UnlockSurface(viewportbuf);
    }

    //output viewport
    Rend_BlitSurfaceToScreen(viewportbuf, GAMESCREEN_X, GAMESCREEN_Y);

    Rend_ProcessSubs();

    Menu_Draw();

    ProcessCursor();
}

subrect_t *Rend_CreateSubRect(int x, int y, int w, int h)
{
    subrect_t *tmp = NEW(subrect_t);

    tmp->h = h;
    tmp->w = w;
    tmp->x = x;
    tmp->y = y;
    tmp->todelete = false;
    tmp->id = subid++;
    tmp->timer = -1;
    tmp->img = Rend_CreateSurface(w, h, 0);

    AddToMList(sublist, tmp);

    return tmp;
}

void Rend_DeleteSubRect(subrect_t *erect)
{
    erect->todelete = true;
}

void Rend_ProcessSubs()
{
    StartMList(sublist);
    while (!EndOfMList(sublist))
    {
        subrect_t *subrec = (subrect_t *)DataMList(sublist);

        if (subrec->timer >= 0)
        {
            subrec->timer -= GetDTime();
            if (subrec->timer < 0)
                subrec->todelete = true;
        }

        if (subrec->todelete)
        {
            SDL_FreeSurface(subrec->img);
            free(subrec);
            DeleteCurrent(sublist);
        }
        else
            Rend_BlitSurfaceToScreen(
                subrec->img,
                subrec->x + GAMESCREEN_X + GAMESCREEN_FLAT_X,
                subrec->y + GAMESCREEN_Y - 5);

        NextMList(sublist);
    }
}

void Rend_DelaySubDelete(subrect_t *sub, int32_t time)
{
    if (time > 0)
        sub->timer = time;
}

SDL_Surface *Rend_CreateSurface(int w, int h, int rgb_mode)
{
    return SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 16, 0x1F << 10, 0x1F << 5, 0x1F, 0);
}

SDL_Surface *Rend_GetScreen()
{
    return screen;
}

SDL_Surface *Rend_GetGameScreen()
{
    return tempbuf;
}

SDL_Surface *Rend_GetLocationScreenImage()
{
    return scrbuf;
}

void Rend_ScreenFlip()
{
    SDL_Flip(screen);
}

void Rend_tilt_SetAngle(float angle)
{
    tilt_angle = angle;
}

void Rend_tilt_SetLinscale(float linscale)
{
    tilt_linscale = fabs(linscale);
    if (tilt_linscale > 1.0 || tilt_linscale == 0)
        tilt_linscale = 1.0;
    tilt_gap = ((float)(GAMESCREEN_H_2)*tilt_linscale);
}

void Rend_tilt_SetTable()
{
    float angl = tilt_angle;
    float k = tilt_linscale;
    memset(render_table, 0, sizeof(render_table));

    int32_t yy = GAMESCREEN_H;
    int32_t xx = GAMESCREEN_W;

    double half_w = (double)xx / 2.0;
    double half_h = (double)yy / 2.0;

    double angle = (angl * 3.14159265 / 180.0);
    double hhdtan = half_w / tan(angle);
    double tandhh = tan(angle) / half_w;

    for (int y = 0; y < yy; y++)
    {
        double poY = (double)y - half_h + 0.01; //0.01 - for zero tan/atan issue (vertical line on half of screen)

        double tmx = atan(poY * tandhh);
        double nX = k * hhdtan * tmx;
        double nn = cos(tmx);
        double nhw = half_w * nn * hhdtan * tandhh * 2.0;

        int32_t rely = floor(nX); // + half_w);
        double xk = nhw / (double)xx;

        double et2 = ((double)xx - nhw) / 2.0;

        for (int x = 0; x < xx; x++)
        {
            double et1 = (double)x * xk;

            int32_t newy = rely;
            int32_t newx = floor(et2 + et1);

            render_table[x][y].y = newy;
            render_table[x][y].x = newx;

            newy += GAMESCREEN_H_2;

            if (newx < 0)
                newx = 0;
            if (newx >= GAMESCREEN_W)
                newx = GAMESCREEN_W - 1;
            if (newy < 0)
                newy = 0;
            if (newy >= GAMESCREEN_H)
                newy = GAMESCREEN_H - 1;

            new_render_table[x + y * GAMESCREEN_W] = newx + newy * GAMESCREEN_W; //pixel index
        }
    }

    Rend_indexer();
}

action_res_t *Rend_CreateNode(int type)
{
    action_res_t *tmp = NEW(action_res_t);

    tmp->node_type = NODE_TYPE_DISTORT;

    if (type == NODE_TYPE_DISTORT)
    {
        tmp->nodes.distort = NEW(distort_t);
        tmp->nodes.distort->increase = true;
    }
    else if (type == NODE_TYPE_REGION)
    {
        //
    }
    else
    {
        Z_PANIC("Invalid render node type %d\n", type);
    }

    return tmp;
}

int Rend_ProcessNode(action_res_t *nod)
{
    if (nod->node_type == NODE_TYPE_DISTORT)
    {
        if (Rend_GetRenderer() != RENDER_PANA && Rend_GetRenderer() != RENDER_TILT)
            return NODE_RET_OK;

        distort_t *dist = nod->nodes.distort;

        if (dist->increase)
            dist->cur_frame += rand() % dist->frames;
        else
            dist->cur_frame -= rand() % dist->frames;

        if (dist->cur_frame >= dist->frames)
        {
            dist->increase = false;
            dist->cur_frame = dist->frames;
        }
        else if (dist->cur_frame <= 1)
        {
            dist->cur_frame = 1;
            dist->increase = true;
        }

        float diff = (1.0 / (5.0 - ((float)dist->cur_frame * dist->param1))) / (5.0 - dist->param1);

        SetRendererAngle(dist->st_angl + diff * dist->dif_angl);
        SetRendererLinscale(dist->st_lin + diff * dist->dif_lin);
        SetRendererTable();
    }

    return NODE_RET_OK;
}

int Rend_DeleteNode(action_res_t *nod)
{
    if (nod->node_type == NODE_TYPE_DISTORT)
    {
        SetRendererAngle(nod->nodes.distort->rend_angl);
        SetRendererLinscale(nod->nodes.distort->rend_lin);
        SetRendererTable();

        if (nod->slot > 0)
        {
            SetGNode(nod->slot, NULL);
            SetgVarInt(nod->slot, 2);
        }

        free(nod->nodes.distort);
        free(nod);

        return NODE_RET_DELETE;
    }
    else if (nod->node_type == NODE_TYPE_REGION)
    {
        if (nod->nodes.node_region != -1)
            DeleteEffect(nod->nodes.node_region);

        if (nod->slot > 0)
        {
            SetgVarInt(nod->slot, 2);
            SetGNode(nod->slot, NULL);
        }

        free(nod);

        return NODE_RET_DELETE;
    }

    return NODE_RET_NO;
}

static bool GetScreenPart(int32_t *x, int32_t *y, int32_t w, int32_t h, SDL_Surface *dst)
{
    if (dst != NULL)
    {
        SDL_Rect rct = {*x, *y, w, h};
        SDL_BlitSurface(scrbuf, &rct, dst, NULL);
    }

    if (Renderer == RENDER_FLAT)
    {
        if ((*x + w) < 0 || (*x) >= scrbuf->w || (*y + h) < 0 || (*y) >= scrbuf->h)
            return 0;
        else
            return 1;
    }
    else if (Renderer == RENDER_PANA)
    {
        if ((*y + h) < 0 || (*y) >= scrbuf->h)
            return 0;
        else
        {
            int32_t xx = *x % pana_PanaWidth;

            if (xx < 0)
                xx = *x + pana_PanaWidth;

            if (xx + w > (*view_X - GAMESCREEN_W_2) && xx < (*view_X + GAMESCREEN_W_2))
            {
                *x -= *view_X - GAMESCREEN_W_2;
                return 1;
            }

            if (*view_X > (pana_PanaWidth - GAMESCREEN_W_2))
            {
                if (*x < (GAMESCREEN_W_2 - (pana_PanaWidth - *view_X)))
                {
                    *x += GAMESCREEN_W_2 + (pana_PanaWidth - *view_X);
                    return 1;
                }
            }
            else if (*view_X < GAMESCREEN_W_2)
            {
                if (*x + w > pana_PanaWidth - (GAMESCREEN_W_2 - *view_X))
                {
                    *x -= pana_PanaWidth - (GAMESCREEN_W_2 - *view_X);
                    return 1;
                }
            }
        }
    }
    else if (Renderer == RENDER_TILT)
    {
        printf("I HATE this shit! Write your code for tilt render in %s at %d line.\n", __FILE__, __LINE__);
    }

    return 0;
}

static effect_t *GetEffect(uint32_t index)
{
    return (index < EFFECTS_MAX_CNT && index >= 0) ? &Effects[index] : NULL;
}

static int AddEffect(int32_t type)
{
    effect_t *effect = NULL;
    int s = 0;

    for (; s < EFFECTS_MAX_CNT; s++)
    {
        if (Effects[s].type == 0)
        {
            effect = &Effects[s];
            break;
        }
    }

    if (effect == NULL)
        return -1;

    effect->type = type;
    effect->delay = 100;
    effect->time = 0;
    effect->x = 0;
    effect->y = 0;
    effect->w = 0;
    effect->h = 0;

    if (type == EFFECT_WAVE)
    {
        effect->effect.ef0.ampls = NULL;
        effect->effect.ef0.frame = 0;
        effect->effect.ef0.frame_cnt = 0;
        effect->effect.ef0.surface = NULL;
    }
    else if (type == EFFECT_LIGH)
    {
        effect->effect.ef1.map = NULL;
        effect->effect.ef1.surface = NULL;
        effect->effect.ef1.maxstp = 0;
        effect->effect.ef1.sign = 1;
        effect->effect.ef1.stp = 0;
    }
    else if (type == EFFECT_9)
    {
        effect->effect.ef9.cloud = NULL;
        effect->effect.ef9.cloud_mod = NULL;
        effect->effect.ef9.mapping = NULL;
        effect->effect.ef9.mask = NULL;
    }
    return s;
}

static void DeleteEffect(uint32_t index)
{
    effect_t *effect = GetEffect(index);
    if (!effect)
        return;

    switch (effect->type)
    {
    case EFFECT_WAVE:
        if (effect->effect.ef0.ampls)
        {
            for (int i = 0; i < effect->effect.ef0.frame_cnt; i++)
                free(effect->effect.ef0.ampls[i]);
            free(effect->effect.ef0.ampls);
        }
        if (effect->effect.ef0.surface)
            SDL_FreeSurface(effect->effect.ef0.surface);
        break;
    case EFFECT_LIGH:
        if (effect->effect.ef1.map)
            free(effect->effect.ef1.map);

        if (effect->effect.ef1.surface)
            SDL_FreeSurface(effect->effect.ef1.surface);
        break;
    case EFFECT_9:
        if (effect->effect.ef9.cloud)
            SDL_FreeSurface(effect->effect.ef9.cloud);
        if (effect->effect.ef9.mapping)
            SDL_FreeSurface(effect->effect.ef9.mapping);
        if (effect->effect.ef9.mask)
            SDL_FreeSurface(effect->effect.ef9.mask);
        if (effect->effect.ef9.cloud_mod)
            free(effect->effect.ef9.cloud_mod);
        break;
    }

    memset(effect, 0, sizeof(effect_t));
}

static int8_t *Effects_Map_Useart(int32_t color, int32_t color_dlta, int32_t x, int32_t y, int32_t w, int32_t h)
{
    int16_t *pixels = (int16_t *)scrbuf->pixels;
    int8_t *tmp = NEW_ARRAY(int8_t, w * h);

    SDL_LockSurface(scrbuf);

    for (int j = y; j < y + h; j++)
        for (int i = x; i < x + w; i++)
            if (i >= 0 && i < scrbuf->w && j >= 0 && j < scrbuf->h)
                tmp[(i - x) + (j - y) * w] =
                    ((color - color_dlta) < pixels[i + j * scrbuf->w]
                        && (color + color_dlta) > pixels[i + j * scrbuf->w]);
                // ((color - color_dlta) <= pixels[i + j * scrbuf->w])

    SDL_UnlockSurface(scrbuf);

    return tmp;
}

int32_t Rend_EF_Light_Setup(char *string, int32_t x, int32_t y, int32_t w, int32_t h, int32_t delay, int32_t steps)
{
    int eftmp = AddEffect(EFFECT_LIGH);
    if (eftmp < 0)
        return -1;

    effect_t *ef = GetEffect(eftmp);

    if (str_starts_with(string, "useart"))
    {
        int32_t xx, yy, delta, color;
        sscanf(string, "useart[%d,%d,%d]", &xx, &yy, &delta);

        SDL_LockSurface(scrbuf);

        delta = SDL_MapRGB(scrbuf->format, delta * 8, delta * 8, delta * 8);
        color = Rend_GetPixel(scrbuf, xx, yy).pixel;

        SDL_UnlockSurface(scrbuf);

        ef->effect.ef1.map = Effects_Map_Useart(color, delta, x, y, w, h);
    }
    else
    {
        //WRITE CODE FOR IMAGES.... but not want %)
    }

    ef->delay = delay;
    ef->time = 0;

    ef->x = x;
    ef->w = w;
    ef->y = y;
    ef->h = h;
    ef->effect.ef1.sign = 1;

    ef->effect.ef1.maxstp = steps;
    ef->effect.ef1.stp = 0;

    ef->effect.ef1.surface = Rend_CreateSurface(w, h, 0);

    return eftmp;
}

static void Rend_EF_Light_Draw(effect_t *ef)
{
    if (!ef->effect.ef1.surface)
        return;

    ef->time -= GetDTime();

    if (ef->time < 0)
    {
        ef->time = ef->delay;
        ef->effect.ef1.stp += ef->effect.ef1.sign;
        if (ef->effect.ef1.stp == ef->effect.ef1.maxstp * ef->effect.ef1.sign)
            ef->effect.ef1.sign = -ef->effect.ef1.sign;
    }

    int32_t x = ef->x;
    int32_t y = ef->y;
    int32_t w = ef->w;
    int32_t h = ef->h;

    if (GetScreenPart(&x, &y, w, h, ef->effect.ef1.surface) != 1)
        return;

    SDL_Surface *srf = ef->effect.ef1.surface;

    SDL_LockSurface(srf);

    int stp = ef->effect.ef1.stp;

    for (int j = 0; j < srf->h; j++)
        for (int i = 0; i < srf->w; i++)
            if (ef->effect.ef1.map[i + j * w] == 1)
            {
                color_t c = Rend_GetPixel(srf, i, j);
                int min = 0xFF, max = 0, inc = 0;

                if (c.r < min) min = c.r;
                if (c.g < min) min = c.g;
                if (c.b < min) min = c.b;
                if (c.r > max) max = c.r;
                if (c.g > max) max = c.g;
                if (c.b > max) max = c.b;

                if (stp < 0)
                {
                    int32_t minstp = min >> 3; //  min / 8
                    inc = (minstp > -stp) ? (stp * 8) : -(minstp * 8);
                }
                else
                {
                    int32_t maxstp = (0xFF - max) >> 3; //  (255-max) / 8
                    inc = (maxstp > stp) ? (stp * 8) : (maxstp * 8);
                }

                Rend_SetPixel(srf, i, j, c.r + inc, c.g + inc, c.b + inc);
            }

    SDL_UnlockSurface(srf);

    Rend_BlitSurfaceXY(srf, tempbuf, x, y);
}

int32_t Rend_EF_Wave_Setup(int32_t delay, int32_t frames, int32_t s_x, int32_t s_y, float apml, float waveln, float spd)
{
    int eftmp = AddEffect(EFFECT_WAVE);
    if (eftmp < 0)
        return -1;

    effect_t *ef = GetEffect(eftmp);

    if (ef->effect.ef0.ampls)
    {
        for (int i = 0; i < ef->effect.ef0.frame_cnt; i++)
            free(ef->effect.ef0.ampls[i]);
        free(ef->effect.ef0.ampls);
    }

    if (!ef->effect.ef0.surface)
        ef->effect.ef0.surface = Rend_CreateSurface(GAMESCREEN_W, GAMESCREEN_H, 0);

    ef->effect.ef0.frame = -1;
    ef->effect.ef0.frame_cnt = frames;
    ef->effect.ef0.ampls = NEW_ARRAY(int8_t *, frames);
    ef->delay = delay;
    ef->time = 0;

    int32_t frmsize = GAMESCREEN_H_2 * GAMESCREEN_W_2;
    int32_t w_4 = GAMESCREEN_W / 4;
    int32_t h_4 = GAMESCREEN_H / 4;
    float phase = 0;

    for (int i = 0; i < ef->effect.ef0.frame_cnt; i++)
    {
        ef->effect.ef0.ampls[i] = NEW_ARRAY(int8_t, frmsize);

        for (int y = 0; y < GAMESCREEN_H_2; y++)
            for (int x = 0; x < GAMESCREEN_W_2; x++)
            {
                int32_t dx = (x - w_4);
                int32_t dy = (y - h_4);

                ef->effect.ef0.ampls[i][x + y * GAMESCREEN_W_2] = apml * fastSin(fastSqrt(dx * dx / (float)s_x + dy * dy / (float)s_y) / (-waveln * 3.1415926) + phase);
            }
        phase += spd;
    }

    return eftmp;
}

static void Rend_EF_Wave_Draw(effect_t *ef)
{
    if (!ef->effect.ef0.surface)
        return;

    ef->time -= GetDTime();

    if (ef->time < 0)
    {
        ef->time = ef->delay;
        ef->effect.ef0.frame++;
        if (ef->effect.ef0.frame >= ef->effect.ef0.frame_cnt)
            ef->effect.ef0.frame = 0;
    }

    SDL_LockSurface(ef->effect.ef0.surface);
    SDL_LockSurface(tempbuf);

    for (int y = 0; y < GAMESCREEN_H_2; y++)
    {
        uint16_t *abc  = ((uint16_t *)ef->effect.ef0.surface->pixels) + y * GAMESCREEN_W;
        uint16_t *abc2 = ((uint16_t *)ef->effect.ef0.surface->pixels) + (y + GAMESCREEN_H_2) * GAMESCREEN_W;
        uint16_t *abc3 = ((uint16_t *)ef->effect.ef0.surface->pixels) + y * GAMESCREEN_W + GAMESCREEN_W_2;
        uint16_t *abc4 = ((uint16_t *)ef->effect.ef0.surface->pixels) + (y + GAMESCREEN_H_2) * GAMESCREEN_W + GAMESCREEN_W_2;
        for (int x = 0; x < GAMESCREEN_W_2; x++)
        {
            int8_t amnt = ef->effect.ef0.ampls[ef->effect.ef0.frame][x + y * GAMESCREEN_W_2];
            int32_t n_x = x + amnt;
            int32_t n_y = y + amnt;

            if (n_x < 0) n_x = 0;
            if (n_x >= GAMESCREEN_W) n_x = GAMESCREEN_W - 1;
            if (n_y < 0) n_y = 0;
            if (n_y >= GAMESCREEN_H) n_y = GAMESCREEN_H - 1;

            *abc = ((uint16_t *)tempbuf->pixels)[n_x + n_y * GAMESCREEN_W];

            n_x = x + amnt + GAMESCREEN_W_2;
            n_y = y + amnt;

            if (n_x < 0) n_x = 0;
            if (n_x >= GAMESCREEN_W)n_x = GAMESCREEN_W - 1;
            if (n_y < 0) n_y = 0;
            if (n_y >= GAMESCREEN_H) n_y = GAMESCREEN_H - 1;

            *abc3 = ((uint16_t *)tempbuf->pixels)[n_x + n_y * GAMESCREEN_W];

            n_x = x + amnt;
            n_y = y + amnt + GAMESCREEN_H_2;

            if (n_x < 0)
                n_x = 0;
            if (n_x >= GAMESCREEN_W)
                n_x = GAMESCREEN_W - 1;
            if (n_y < 0)
                n_y = 0;
            if (n_y >= GAMESCREEN_H)
                n_y = GAMESCREEN_H - 1;
            *abc2 = ((uint16_t *)tempbuf->pixels)[n_x + n_y * GAMESCREEN_W];

            n_x = x + amnt + GAMESCREEN_W_2;
            n_y = y + amnt + GAMESCREEN_H_2;

            if (n_x < 0)
                n_x = 0;
            if (n_x >= GAMESCREEN_W)
                n_x = GAMESCREEN_W - 1;
            if (n_y < 0)
                n_y = 0;
            if (n_y >= GAMESCREEN_H)
                n_y = GAMESCREEN_H - 1;
            *abc4 = ((uint16_t *)tempbuf->pixels)[n_x + n_y * GAMESCREEN_W];

            abc++;
            abc2++;
            abc3++;
            abc4++;
        }
    }

    SDL_UnlockSurface(ef->effect.ef0.surface);
    SDL_UnlockSurface(tempbuf);

    Rend_BlitSurfaceXY(ef->effect.ef0.surface, tempbuf, 0, 0);
}

int32_t Rend_EF_9_Setup(char *mask, char *clouds, int32_t delay, int32_t x, int32_t y, int32_t w, int32_t h)
{
    int eftmp = AddEffect(EFFECT_9);
    if (eftmp < 0)
        return -1;

    effect_t *ef = GetEffect(eftmp);

    ef->effect.ef9.cloud = Loader_LoadGFX(clouds, false, -1);
    ef->effect.ef9.mask = Loader_LoadGFX(mask, false, -1);

    if (ef->effect.ef9.cloud == NULL || ef->effect.ef9.mask == NULL)
    {
        DeleteEffect(eftmp);
        return -1;
    }

    ef->effect.ef9.cloud_mod = NEW_ARRAY(int8_t, ef->effect.ef9.cloud->w * ef->effect.ef9.cloud->h);
    ef->effect.ef9.mapping = Rend_CreateSurface(w, h, 0);

    ef->x = x;
    ef->y = y;
    ef->w = w;
    ef->h = h;
    ef->delay = delay;

    return eftmp;
}

static void Rend_EF_9_Draw(effect_t *ef)
{
    if (!ef->effect.ef9.mapping)
        return;

    ef->time -= GetDTime();

    if (ef->time < 0)
    {
        ef->time = ef->delay;

        SDL_LockSurface(ef->effect.ef9.cloud);

        SDL_Surface *clouds = ef->effect.ef9.cloud;
        int8_t *mp = ef->effect.ef9.cloud_mod;
        uint16_t *aa = (uint16_t *)clouds->pixels;
        uint8_t r, g, b;

        for (int yy = 0; yy < clouds->h; yy++)
        {
            for (int xx = 0; xx < clouds->w; xx++)
            {
                SDL_GetRGB(aa[xx], clouds->format, &r, &g, &b);

                if (mp[xx] == 0)
                {
                    if (b <= 0x10)
                        mp[xx] = 1;
                    else
                        aa[xx] = SDL_MapRGB(clouds->format, r - 8, g - 8, b - 8);
                }
                else
                {
                    if (b >= 0xEF)
                        mp[xx] = 0;
                    else
                        aa[xx] = SDL_MapRGB(clouds->format, r + 8, g + 8, b + 8);
                }
            }

            int shift_num = GetgVarInt(SLOT_EF9_SPEED);

            for (int sn = 0; sn < shift_num; sn++)
            {
                uint32_t tmp1 = aa[0];
                int8_t tmp2 = mp[0];

                for (int xx = 0; xx < clouds->w - 1; xx++)
                {
                    aa[xx] = aa[xx + 1];
                    mp[xx] = mp[xx + 1];
                }
                aa[clouds->w - 1] = tmp1;
                mp[clouds->w - 1] = tmp2;
            }

            aa += clouds->w;
            mp += clouds->w;
        }

        SDL_UnlockSurface(ef->effect.ef9.cloud);
    }

    int32_t x = ef->x;
    int32_t y = ef->y;
    int32_t w = ef->w;
    int32_t h = ef->h;

    if (GetScreenPart(&x, &y, w, h, ef->effect.ef9.mapping) == 1)
    {
        SDL_Surface *srf = ef->effect.ef9.mapping;
        SDL_Surface *mask = ef->effect.ef9.mask;
        SDL_Surface *cloud = ef->effect.ef9.cloud;

        int32_t minw = srf->w;
        if (minw > mask->w)
            minw = mask->w;
        if (minw > cloud->w)
            minw = cloud->w;

        int32_t minh = srf->h;
        if (minh > mask->h)
            minh = mask->h;
        if (minh > cloud->h)
            minh = cloud->h;

        SDL_LockSurface(srf);
        SDL_LockSurface(mask);
        SDL_LockSurface(cloud);

        for (int y = 0; y < minh; y++)
        {
            for (int x = 0; x < minw; x++)
            {
                if ((Rend_GetPixel(mask, x, y).pixel & 0xFFFFFF) == 0)
                    continue;

                color_t c = Rend_GetPixel(cloud, x, y);
                color_t s = Rend_GetPixel(srf, x, y);

                uint32_t m_r = GetgVarInt(SLOT_EF9_R) & 0x1F;
                uint32_t m_g = GetgVarInt(SLOT_EF9_G) & 0x1F;
                uint32_t m_b = GetgVarInt(SLOT_EF9_B) & 0x1F;

                m_r = (FiveBitToEightBitLookupTable[m_r] * c.r) / 0xFF;
                m_g = (FiveBitToEightBitLookupTable[m_g] * c.g) / 0xFF;
                m_b = (FiveBitToEightBitLookupTable[m_b] * c.b) / 0xFF;

                uint8_t s_r = ((m_r + s.r) > 0xFF) ? 0xFF : (m_r + s.r);
                uint8_t s_g = ((m_g + s.g) > 0xFF) ? 0xFF : (m_g + s.g);
                uint8_t s_b = ((m_b + s.b) > 0xFF) ? 0xFF : (m_b + s.b);

                Rend_SetPixel(srf, x, y, s_r, s_g, s_b);
            }
        }

        SDL_UnlockSurface(srf);
        SDL_UnlockSurface(mask);
        SDL_UnlockSurface(cloud);

        Rend_BlitSurfaceXY(srf, tempbuf, x, y);
    }
}

void Rend_SetGamma(float val)
{
    if (val > 0.4 && val < 2.1)
    {
        mgamma = val;
        SDL_SetGamma(mgamma, mgamma, mgamma);
    }
}

float Rend_GetGamma()
{
    return mgamma;
}

void Rend_SetColorKey(SDL_Surface *surf, uint8_t r, uint8_t g, uint8_t b)
{
    SDL_SetColorKey(surf, SDL_SRCCOLORKEY, SDL_MapRGB(surf->format, r, g, b));
}

void Rend_DrawImageToGameScreen(SDL_Surface *scr, int x, int y)
{
    SDL_Rect rect = {x, y, 0, 0};

    if (Renderer == RENDER_TILT)
        rect.y = y + GAMESCREEN_H_2 - *view_X;

    Rend_BlitSurface(scr, NULL, tempbuf, &rect);
}

void Rend_BlitSurfaceToScreen(SDL_Surface *surf, int x, int y)
{
    SDL_Rect rect = {x, y, 0, 0};
    Rend_BlitSurface(surf, NULL, screen, &rect);
}

void Rend_BlitSurfaceXY(SDL_Surface *surf, SDL_Surface *dest, int x, int y)
{
    SDL_Rect rect = {x, y, 0, 0};
    Rend_BlitSurface(surf, NULL, dest, &rect);
}

void Rend_BlitSurface(SDL_Surface *src, SDL_Rect *src_rct, SDL_Surface *dst, SDL_Rect *dst_rct)
{
    if (!dst || !src)
        return;

    int src_w = (src_rct && src_rct->w > 0) ? src_rct->w : src->w;
    int src_h = (src_rct && src_rct->h > 0) ? src_rct->h : src->h;
    int dst_w = (dst_rct && dst_rct->w > 0) ? dst_rct->w : src_w;
    int dst_h = (dst_rct && dst_rct->h > 0) ? dst_rct->h : src_h;

    if (dst_rct) LOG_DEBUG("dst_rct: %d %d %d %d\n", dst_rct->x, dst_rct->y, dst_rct->w, dst_rct->h);
    if (src_rct) LOG_DEBUG("src_rct: %d %d %d %d\n", dst_rct->x, dst_rct->y, dst_rct->w, dst_rct->h);
    LOG_DEBUG("src: %d %d    dst: %d %d\n", src->w, src->h, dst->w, dst->h);

    if (dst_h != src_h || dst_w != src_w)
    {
        // TO DO: Handle src x/y
        SDL_Rect dst_rect = {dst_rct ? dst_rct->x : 0, dst_rct ? dst_rct->y : 0, dst_w, dst_h};
        SDL_Surface *zsrc = zoomSurface(src, (float)dst_w / src_w, (float)dst_h / src_h, 0);
        SDL_BlitSurface(zsrc, NULL, dst, &dst_rect);
        SDL_FreeSurface(zsrc);
    }
    else
    {
        SDL_BlitSurface(src, src_rct, dst, dst_rct);
    }
}

void Rend_FillRect(SDL_Surface *surf, SDL_Rect *dst_rct, uint8_t r, uint8_t g, uint8_t b)
{
    SDL_FillRect(surf, dst_rct, SDL_MapRGB(surf->format, r, g, b));
}
