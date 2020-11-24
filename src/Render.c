#include "System.h"

//Graphics
int GAME_W = 640;
int GAME_H = 480;
int GAME_BPP = 32;
int GAMESCREEN_W = 640;
int GAMESCREEN_P = 60;
int GAMESCREEN_H = 344;
int GAMESCREEN_X = 0;
int GAMESCREEN_Y = 68;
int GAMESCREEN_FLAT_X = 0;

static float tilt_angle = 60.0;
static float tilt_linscale = 1.0;
static bool tilt_Reverse = false;
static int32_t tilt_gap = 0;

static float mgamma = 1.0;

static uint8_t Renderer = RENDER_FLAT;

static MList *sublist = NULL;
static int32_t subid = 0;

static SDL_Surface *screen;      // Game window surface
static SDL_Surface *scrbuf;      // Surface loaded by setscreen, all changes by setpartialscreen and other similar modify this surface.
static SDL_Surface *tempbuf;     // This surface used for effects(region action) and control draws.
static SDL_Surface *viewportbuf; //This surface used for rendered viewport image with renderer processing.

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

const int FiveBitToEightBitLookupTable[32] = {
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

//static const float PITWO = (3.14159265*2);

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
} struct_effect_t;

#define EFFECTS_MAX_CNT 32

static struct_effect_t Effects[EFFECTS_MAX_CNT];

static void Effects_Delete(uint32_t index);
static struct_effect_t *Effects_GetEf(uint32_t index);
static int32_t Effects_AddEffect(int32_t type);
static int32_t Effects_GetColor(uint32_t x, uint32_t y);
static void Rend_EF_9_Draw(struct_effect_t *ef);
static void Rend_EF_Wave_Draw(struct_effect_t *ef);
static void Rend_EF_Light_Draw(struct_effect_t *ef);

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

    for (int32_t ff = 1; ff < GAMESCREEN_H * GAMESCREEN_W; ff++)
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

    for (int32_t x = 0; x < ww; x++)
    {
        double poX = (double)x - half_w + 0.01; //0.01 - for zero tan/atan issue (vertical line on half of screen)

        double tmx = atan(poX * tandhh);
        double nX = k * hhdtan * tmx;
        double nn = cos(tmx);
        double nhw = half_h * nn * hhdtan * tandhh * 2.0;

        int32_t relx = floor(nX); // + half_w);
        double yk = nhw / (double)yy;

        double et2 = ((double)yy - nhw) / 2.0;

        for (int32_t y = 0; y < yy; y++)
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

void Rend_InitGraphics(bool fullscreen, bool widescreen)
{
    // To do : Use a nice struct instead
    GAME_W = 640;
    GAME_H = 480;

    if (CUR_GAME == GAME_ZGI)
    {
        GAMESCREEN_W = 640;
        GAMESCREEN_P = 60;
        GAMESCREEN_H = 344;
        GAMESCREEN_X = 0;
        GAMESCREEN_Y = 68;
        GAMESCREEN_FLAT_X = 0;
        if (widescreen)
        {
            GAME_W = 854;
            GAMESCREEN_W = 854;
            GAMESCREEN_FLAT_X = 107;
        }
    }
    else
    {
        GAMESCREEN_W = 512;
        GAMESCREEN_P = 60;
        GAMESCREEN_H = 320;
        GAMESCREEN_X = 64; //(640-512)/2
        GAMESCREEN_Y = 80; //(480-320)/2
        GAMESCREEN_FLAT_X = 0;
    }

    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
    {
        Z_PANIC("Unable to init SDL: %s\n", SDL_GetError());
    }

    uint32_t flags = RENDER_SURFACE | SDL_RESIZABLE;

    if (fullscreen)
        flags |= SDL_FULLSCREEN;

    screen = SDL_SetVideoMode(GAME_W, GAME_H, GAME_BPP, flags);
    tempbuf = CreateSurface(GAMESCREEN_W, GAMESCREEN_H);
    viewportbuf = CreateSurface(GAMESCREEN_W, GAMESCREEN_H);
    sublist = CreateMList();
    view_X = GetDirectgVarInt(SLOT_VIEW_POS);
    tilt_gap = GAMESCREEN_H_2;

    char buffer[128];
    sprintf(buffer, "Zorkmid: %s [build: " __DATE__ " " __TIME__ "]", GetGameTitle());
    SDL_WM_SetCaption(buffer, NULL);
    SDL_ShowCursor(SDL_DISABLE);
    TTF_Init();

    memset(Effects, 0, sizeof(Effects));
}

void Rend_SwitchFullscreen()
{
    int32_t flags = screen->flags;

    screen = SDL_SetVideoMode(0, 0, 0, flags ^ SDL_FULLSCREEN);
    if (screen == NULL)
        screen = SDL_SetVideoMode(0, 0, 0, flags);
    else
        flags ^= SDL_FULLSCREEN;
}

void Rend_DrawImageToGamescr(SDL_Surface *scr, int x, int y)
{
    if (scrbuf)
        DrawImageToSurf(scr, x, y, scrbuf);
}

void Rend_DrawAnimImageToGamescr(anim_surf_t *scr, int x, int y, int frame)
{
    if (scrbuf)
        DrawAnimImageToSurf(scr, x, y, frame, scrbuf);
}

void Rend_DrawImageUpGamescr(SDL_Surface *scr, int x, int y)
{
    if (tempbuf)
    {
        if (Renderer == RENDER_FLAT)
            DrawImageToSurf(scr, x, y, tempbuf);
        else if (Renderer == RENDER_PANA)
            DrawImageToSurf(scr, x, y, tempbuf);
        else if (Renderer == RENDER_TILT)
            DrawImageToSurf(scr, x, y + GAMESCREEN_H_2 - *view_X, tempbuf);
    }
}

void Rend_DrawAnimImageUpGamescr(anim_surf_t *scr, int x, int y, int frame)
{
    DrawAnimImageToSurf(scr, x, y, frame, tempbuf);
}

void Rend_DrawImageToScr(SDL_Surface *scr, int x, int y)
{
    DrawImageToSurf(scr, x, y, screen);
}

void Rend_DrawTilt_pre()
{
    DrawImageToSurf(scrbuf, 0, GAMESCREEN_H_2 - *view_X, tempbuf);
}

void Rend_DrawTilt()
{
    SDL_LockSurface(tempbuf);
    SDL_LockSurface(viewportbuf);
    if (GAME_BPP == 32)
    {
        uint32_t *nww = (uint32_t *)viewportbuf->pixels;
        uint32_t *old = (uint32_t *)tempbuf->pixels;
        int32_t *ofs = new_render_table;
        for (int32_t ai = 0; ai < GAMESCREEN_H * GAMESCREEN_W; ai++)
        {
            old += *ofs;
            *nww = *old;
            nww++;
            ofs++;
        }
    }
    else if (GAME_BPP == 16)
    {
        uint16_t *nww = (uint16_t *)viewportbuf->pixels;
        uint16_t *old = (uint16_t *)tempbuf->pixels;
        int32_t *ofs = new_render_table;
        for (int32_t ai = 0; ai < GAMESCREEN_H * GAMESCREEN_W; ai++)
        {
            old += *ofs;
            *nww = *old;
            nww++;
            ofs++;
        }
    }
    else
    {
        Z_PANIC("Bit depth %d not supported!\n", GAME_BPP);
    }
    SDL_UnlockSurface(tempbuf);
    SDL_UnlockSurface(viewportbuf);
}

int8_t Rend_LoadGamescr(const char *file)
{
    int8_t good = 1;
    if (scrbuf)
        SDL_FreeSurface(scrbuf);

    scrbuf = Loader_LoadFile(file, Rend_GetRenderer() == RENDER_PANA);

    SDL_FillRect(tempbuf, 0, 0);

    if (!scrbuf)
        LOG_WARN("IMG_Load(%s): %s\n", file, SDL_GetError());

    if (!scrbuf) // no errors if no screen
    {
        good = 0;
        scrbuf = CreateSurface(GAMESCREEN_W, GAMESCREEN_H);
    }

    if (Renderer != RENDER_TILT)
        pana_PanaWidth = scrbuf->w;
    else
        pana_PanaWidth = scrbuf->h;

    return good;
}

void Rend_ProcessCursor()
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

void Rend_SetRenderer(int meth)
{
    Renderer = meth;
    pana_ReversePana = false;
    Rend_tilt_SetLinscale(0.65);
    Rend_tilt_SetAngle(60.0);
    Rend_pana_SetLinscale(0.55);
    Rend_pana_SetAngle(60.0);
}

void Rend_SetReversePana(bool pana)
{
    pana_ReversePana = pana;
}

int Rend_GetRenderer()
{
    return Renderer;
}

void Rend_FlatRender()
{
    DrawImageToSurf(tempbuf, 0, 0, viewportbuf);
}

void Rend_FlatRender_pre()
{
    DrawImageToSurf(scrbuf, GAMESCREEN_FLAT_X, 0, tempbuf);
}

void Rend_DrawPanorama_pre()
{
    DrawImageToSurf(scrbuf, GAMESCREEN_W_2 - *view_X, 0, tempbuf);
    if (*view_X < GAMESCREEN_W_2)
        DrawImageToSurf(scrbuf, GAMESCREEN_W_2 - (*view_X + pana_PanaWidth), 0, tempbuf);
    else if (pana_PanaWidth - *view_X < GAMESCREEN_W_2)
        DrawImageToSurf(scrbuf, GAMESCREEN_W_2 + pana_PanaWidth - *view_X, 0, tempbuf);
}

void Rend_DrawPanorama()
{
    SDL_LockSurface(tempbuf);
    SDL_LockSurface(viewportbuf);
    if (GAME_BPP == 32)
    {
        uint32_t *nww = (uint32_t *)viewportbuf->pixels;
        uint32_t *old = (uint32_t *)tempbuf->pixels;
        int32_t *ofs = new_render_table;
        for (int32_t ai = 0; ai < GAMESCREEN_H * GAMESCREEN_W; ai++)
        {
            old += *ofs;
            *nww = *old;
            nww++;
            ofs++;
        }
    }
    else if (GAME_BPP == 16)
    {
        uint16_t *nww = (uint16_t *)viewportbuf->pixels;
        uint16_t *old = (uint16_t *)tempbuf->pixels;
        int32_t *ofs = new_render_table;
        for (int32_t ai = 0; ai < GAMESCREEN_H * GAMESCREEN_W; ai++)
        {
            old += *ofs;
            *nww = *old;
            nww++;
            ofs++;
        }
    }
    else
    {
        Z_PANIC("Bit depth %d not supported!\n", GAME_BPP);
    }
    SDL_UnlockSurface(tempbuf);
    SDL_UnlockSurface(viewportbuf);
}

void Rend_PanaMouseInteract()
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

void Rend_MouseInteractOfRender()
{
    if (GetgVarInt(SLOT_PANAROTATE_SPEED) == 0)
        SetgVarInt(SLOT_PANAROTATE_SPEED, 700);
    if (Renderer == RENDER_PANA)
        Rend_PanaMouseInteract();
    else if (Renderer == RENDER_TILT)
        Rend_tilt_MouseInteract();
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

    SDL_FillRect(screen, 0, 0);

    //pre-rendered
    if (Renderer == RENDER_FLAT)
        Rend_FlatRender_pre();
    else if (Renderer == RENDER_PANA)
        Rend_DrawPanorama_pre();
    else if (Renderer == RENDER_TILT)
        Rend_DrawTilt_pre();

    //draw dynamic controls
    Controls_Draw();

    //effect-processor
    for (int32_t i = 0; i < EFFECTS_MAX_CNT; i++)
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
        Rend_FlatRender();
    else if (Renderer == RENDER_PANA)
        Rend_DrawPanorama();
    else if (Renderer == RENDER_TILT)
        Rend_DrawTilt();

    //output viewport
    DrawImage(viewportbuf, GAMESCREEN_X, GAMESCREEN_Y);

    Rend_ProcessSubs();

    Menu_Draw();

    Rend_ProcessCursor();
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
    tmp->img = CreateSurface(w, h);

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
    while (!eofMList(sublist))
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
            DrawImage(subrec->img, subrec->x + GAMESCREEN_FLAT_X, subrec->y);

        NextMList(sublist);
    }
}

void Rend_DelaySubDelete(subrect_t *sub, int32_t time)
{
    if (time > 0)
        sub->timer = time;
}

SDL_Surface *Rend_GetGameScreen()
{
    return tempbuf;
}

SDL_Surface *Rend_GetLocationScreenImage()
{
    return scrbuf;
}

uint32_t Rend_MapScreenRGB(int r, int g, int b)
{
    return SDL_MapRGB(screen->format, r, g, b);
}

void Rend_ScreenFlip()
{
    SDL_Flip(screen);
}

void Rend_Delay(uint32_t delay_ms)
{
    SDL_Delay(delay_ms);
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

    for (int32_t y = 0; y < yy; y++)
    {
        double poY = (double)y - half_h + 0.01; //0.01 - for zero tan/atan issue (vertical line on half of screen)

        double tmx = atan(poY * tandhh);
        double nX = k * hhdtan * tmx;
        double nn = cos(tmx);
        double nhw = half_w * nn * hhdtan * tandhh * 2.0;

        int32_t rely = floor(nX); // + half_w);
        double xk = nhw / (double)xx;

        double et2 = ((double)xx - nhw) / 2.0;

        for (int32_t x = 0; x < xx; x++)
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

void Rend_tilt_MouseInteract()
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

action_res_t *Rend_CreateDistortNode()
{
    action_res_t *act = ScrSys_CreateActRes(NODE_TYPE_DISTORT);

    act->nodes.distort = NEW(distort_t);
    act->nodes.distort->increase = true;

    return act;
}

int32_t Rend_ProcessDistortNode(action_res_t *nod)
{
    if (nod->node_type != NODE_TYPE_DISTORT)
        return NODE_RET_OK;

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
    return NODE_RET_OK;
}

int32_t Rend_DeleteDistortNode(action_res_t *nod)
{
    if (nod->node_type != NODE_TYPE_DISTORT)
        return NODE_RET_NO;

    SetRendererAngle(nod->nodes.distort->rend_angl);
    SetRendererLinscale(nod->nodes.distort->rend_lin);
    SetRendererTable();

    if (nod->slot > 0)
    {
        setGNode(nod->slot, NULL);
        SetgVarInt(nod->slot, 2);
    }

    free(nod->nodes.distort);
    free(nod);

    return NODE_RET_DELETE;
}

void Rend_DrawScalerToGamescr(scaler_t *scl, int16_t x, int16_t y)
{
    if (scrbuf)
        DrawScaler(scl, x, y, scrbuf);
}

int Rend_DeleteRegion(action_res_t *nod)
{
    if (nod->node_type != NODE_TYPE_REGION)
        return NODE_RET_NO;

    if (nod->nodes.node_region != -1)
        Effects_Delete(nod->nodes.node_region);

    if (nod->slot > 0)
    {
        SetgVarInt(nod->slot, 2);
        setGNode(nod->slot, NULL);
    }

    free(nod);

    return NODE_RET_DELETE;
}

//Function used to get region for apply post-process effects

int8_t Rend_GetScreenPart(int32_t *x, int32_t *y, int32_t w, int32_t h, SDL_Surface *dst)
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

static int32_t Effects_GetColor(uint32_t x, uint32_t y)
{
    int32_t color = 0;
    int32_t xx = x;
    int32_t yy = y;

    SDL_LockSurface(scrbuf);
    if (GAME_BPP == 32)
    {
        if (xx < scrbuf->w && yy < scrbuf->h)
        {
            int32_t *pixels = (int32_t *)scrbuf->pixels;

            color = pixels[scrbuf->w * yy + xx];
        }
    }
    else if (GAME_BPP == 16)
    {
        if (xx < scrbuf->w && yy < scrbuf->h)
        {
            int16_t *pixels = (int16_t *)scrbuf->pixels;

            color = pixels[scrbuf->w * yy + xx];
        }
    }
    else
    {
        LOG_WARN("Bit depth %d not supported!\n", GAME_BPP);
    }
    SDL_UnlockSurface(scrbuf);

    return color;
}

static int8_t *Effects_Map_Useart(int32_t color, int32_t color_dlta, int32_t x, int32_t y, int32_t w, int32_t h)
{
    int8_t *tmp = NEW_ARRAY(int8_t, w * h);

    SDL_LockSurface(scrbuf);
    if (GAME_BPP == 32)
    {
        int32_t *scrn = (int32_t *)scrbuf->pixels;
        for (int32_t j = y; j < y + h; j++)
            for (int32_t i = x; i < x + w; i++)
            {
                if (i >= 0 && i < scrbuf->w && j >= 0 && j < scrbuf->h)
                    //if ((color - color_dlta) < scrn[i+j*scrbuf->w] && (color + color_dlta) < scrn[i+j*scrbuf->w])
                    if ((color - color_dlta) <= scrn[i + j * scrbuf->w])
                        tmp[(i - x) + (j - y) * w] = 1;
            }
    }
    else if (GAME_BPP == 16)
    {
        int16_t *scrn = (int16_t *)scrbuf->pixels;
        for (int32_t j = y; j < y + h; j++)
            for (int32_t i = x; i < x + w; i++)
            {
                if (i >= 0 && i < scrbuf->w && j >= 0 && j < scrbuf->h)
                    if ((color - color_dlta) < scrn[i + j * scrbuf->w] && (color + color_dlta) > scrn[i + j * scrbuf->w])
                        tmp[(i - x) + (j - y) * w] = 1;
            }
    }
    else
    {
        LOG_WARN("Bit depth %d not supported!\n", GAME_BPP);
    }
    SDL_UnlockSurface(scrbuf);

    return tmp;
}

int32_t Rend_EF_Light_Setup(char *string, int32_t x, int32_t y, int32_t w, int32_t h, int32_t delay, int32_t steps)
{
    int32_t eftmp = Effects_AddEffect(EFFECT_LIGH);

    if (eftmp == -1)
        return -1;

    struct_effect_t *ef = Effects_GetEf(eftmp);

    if (ef == NULL)
    {
        Effects_Delete(eftmp);
        return -1;
    }

    if (str_starts_with(string, "useart"))
    {
        int32_t xx, yy, dlt;
        sscanf(string, "useart[%d,%d,%d]", &xx, &yy, &dlt);

        int32_t color = Effects_GetColor(xx, yy);

        if (GAME_BPP == 32)
        {
            dlt = (dlt & 0x1F) * 8;
            dlt = SDL_MapRGB(scrbuf->format, dlt, dlt, dlt);
        }
        else if (GAME_BPP == 16)
            dlt = ((dlt & 0x1F) << 10) | ((dlt & 0x1F) << 5) | (dlt & 0x1F);
        else
            LOG_WARN("Bit depth %d not supported!\n", GAME_BPP);

        ef->effect.ef1.map = Effects_Map_Useart(color, dlt, x, y, w, h);
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

    ef->effect.ef1.surface = CreateSurface(w, h);

    return eftmp;
}

static void Rend_EF_Light_Draw(struct_effect_t *ef)
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

    if (Rend_GetScreenPart(&x, &y, w, h, ef->effect.ef1.surface) == 1)
    {
        SDL_Surface *srf = ef->effect.ef1.surface;

        SDL_LockSurface(srf);

        if (GAME_BPP == 32)
        {
            int32_t *px = (int32_t *)srf->pixels;
            int32_t stp = ef->effect.ef1.stp;

            int32_t color = SDL_MapRGB(srf->format, 8, 8, 8);

            for (int32_t j = 0; j < srf->h; j++)
                for (int32_t i = 0; i < srf->w; i++)
                    if (ef->effect.ef1.map[i + j * w] == 1)
                    {

                        if (stp < 0)
                        {
                            int32_t pixel = px[i + j * w];
                            int32_t min = (pixel >> 16) & 0xFF;
                            if (((pixel >> 8) & 0xFF) < min)
                                min = (pixel >> 8) & 0xFF;
                            if ((pixel & 0xFF) < min)
                                min = pixel & 0xFF;

                            int32_t minstp = min >> 3; //  min / 8
                            if (minstp > -stp)
                            {
                                px[i + j * w] += stp * color;
                            }
                            else
                            {
                                px[i + j * w] -= minstp * color;
                            }
                        }
                        else
                        {
                            int32_t pixel = px[i + j * w];
                            int32_t max = (pixel >> 16) & 0xFF;
                            if (((pixel >> 8) & 0xFF) > max)
                                max = (pixel >> 8) & 0xFF;
                            if ((pixel & 0xFF) > max)
                                max = pixel & 0xFF;

                            int32_t maxstp = (0xFF - max) >> 3; //  (255-max) / 8
                            if (maxstp > stp)
                            {
                                px[i + j * w] += stp * color;
                            }
                            else
                            {
                                px[i + j * w] += maxstp * color;
                            }
                        }
                    }
        }
        else
        {
            LOG_WARN("Bit depth %d not supported!\n", GAME_BPP);
        }

        SDL_UnlockSurface(srf);

        DrawImageToSurf(srf, x, y, tempbuf);
    }
}

int32_t Rend_EF_Wave_Setup(int32_t delay, int32_t frames, int32_t s_x, int32_t s_y, float apml, float waveln, float spd)
{
    int32_t eftmp = Effects_AddEffect(EFFECT_WAVE);

    if (eftmp == -1)
        return -1;

    struct_effect_t *ef = Effects_GetEf(eftmp);

    if (ef == NULL)
    {
        Effects_Delete(eftmp);
        return -1;
    }

    if (ef->effect.ef0.ampls)
    {
        for (int32_t i = 0; i < ef->effect.ef0.frame_cnt; i++)
            free(ef->effect.ef0.ampls[i]);
        free(ef->effect.ef0.ampls);
    }

    if (!ef->effect.ef0.surface)
        ef->effect.ef0.surface = CreateSurface(GAMESCREEN_W, GAMESCREEN_H);

    ef->effect.ef0.frame = -1;
    ef->effect.ef0.frame_cnt = frames;
    ef->effect.ef0.ampls = NEW_ARRAY(int8_t *, frames);
    ef->delay = delay;
    ef->time = 0;

    int32_t frmsize = GAMESCREEN_H_2 * GAMESCREEN_W_2;
    int32_t w_4 = GAMESCREEN_W / 4;
    int32_t h_4 = GAMESCREEN_H / 4;
    float phase = 0;

    for (int32_t i = 0; i < ef->effect.ef0.frame_cnt; i++)
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

static void Rend_EF_Wave_Draw(struct_effect_t *ef)
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
        int32_t *abc = ((int32_t *)ef->effect.ef0.surface->pixels) + y * GAMESCREEN_W;
        int32_t *abc2 = ((int32_t *)ef->effect.ef0.surface->pixels) + (y + GAMESCREEN_H_2) * GAMESCREEN_W;
        int32_t *abc3 = ((int32_t *)ef->effect.ef0.surface->pixels) + y * GAMESCREEN_W + GAMESCREEN_W_2;
        int32_t *abc4 = ((int32_t *)ef->effect.ef0.surface->pixels) + (y + GAMESCREEN_H_2) * GAMESCREEN_W + GAMESCREEN_W_2;
        for (int x = 0; x < GAMESCREEN_W_2; x++)
        {
            int8_t amnt = ef->effect.ef0.ampls[ef->effect.ef0.frame][x + y * GAMESCREEN_W_2];
            int32_t n_x = x + amnt;
            int32_t n_y = y + amnt;

            if (n_x < 0)
                n_x = 0;
            if (n_x >= GAMESCREEN_W)
                n_x = GAMESCREEN_W - 1;
            if (n_y < 0)
                n_y = 0;
            if (n_y >= GAMESCREEN_H)
                n_y = GAMESCREEN_H - 1;
            *abc = ((int32_t *)tempbuf->pixels)[n_x + n_y * GAMESCREEN_W];

            n_x = x + amnt + GAMESCREEN_W_2;
            n_y = y + amnt;

            if (n_x < 0)
                n_x = 0;
            if (n_x >= GAMESCREEN_W)
                n_x = GAMESCREEN_W - 1;
            if (n_y < 0)
                n_y = 0;
            if (n_y >= GAMESCREEN_H)
                n_y = GAMESCREEN_H - 1;
            *abc3 = ((int32_t *)tempbuf->pixels)[n_x + n_y * GAMESCREEN_W];

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
            *abc2 = ((int32_t *)tempbuf->pixels)[n_x + n_y * GAMESCREEN_W];

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
            *abc4 = ((int32_t *)tempbuf->pixels)[n_x + n_y * GAMESCREEN_W];

            abc++;
            abc2++;
            abc3++;
            abc4++;
        }
    }

    SDL_UnlockSurface(ef->effect.ef0.surface);
    SDL_UnlockSurface(tempbuf);

    DrawImageToSurf(ef->effect.ef0.surface, 0, 0, tempbuf);
}

static int32_t Effects_AddEffect(int32_t type)
{
    struct_effect_t *effect = NULL;
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

static void Effects_Delete(uint32_t index)
{
    if (index < EFFECTS_MAX_CNT)
    {
        struct_effect_t *effect = &Effects[index];

        switch (effect->type)
        {
        case EFFECT_WAVE:
            if (effect->effect.ef0.ampls)
            {
                for (int32_t i = 0; i < effect->effect.ef0.frame_cnt; i++)
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
        memset(effect, 0, sizeof(struct_effect_t));
    }
}

static struct_effect_t *Effects_GetEf(uint32_t index)
{
    if (index < EFFECTS_MAX_CNT)
        return &Effects[index];
    return NULL;
}

int32_t Rend_EF_9_Setup(char *mask, char *clouds, int32_t delay, int32_t x, int32_t y, int32_t w, int32_t h)
{

    int32_t eftmp = Effects_AddEffect(EFFECT_9);

    if (eftmp == -1)
        return -1;

    struct_effect_t *ef = Effects_GetEf(eftmp);

    if (ef == NULL)
    {
        Effects_Delete(eftmp);
        return -1;
    }

    ef->effect.ef9.cloud = Loader_LoadFile(clouds, 0);
    ef->effect.ef9.mask = Loader_LoadFile(mask, 0);

    if (ef->effect.ef9.cloud == NULL || ef->effect.ef9.mask == NULL)
    {
        Effects_Delete(eftmp);
        return -1;
    }

    ef->effect.ef9.cloud_mod = NEW_ARRAY(int8_t, ef->effect.ef9.cloud->w * ef->effect.ef9.cloud->h);
    ef->effect.ef9.mapping = CreateSurface(w, h);

    ef->x = x;
    ef->y = y;
    ef->w = w;
    ef->h = h;
    ef->delay = delay;

    return eftmp;
}

static void Rend_EF_9_Draw(struct_effect_t *ef)
{
    if (!ef->effect.ef9.mapping)
        return;

    ef->time -= GetDTime();

    if (ef->time < 0)
    {
        ef->time = ef->delay;

        SDL_LockSurface(ef->effect.ef9.cloud);

        if (GAME_BPP == 32)
        {
            SDL_Surface *clouds = ef->effect.ef9.cloud;

            int8_t *mp = ef->effect.ef9.cloud_mod;

            uint32_t *aa = (uint32_t *)clouds->pixels;

            for (int32_t yy = 0; yy < clouds->h; yy++)
            {
                for (int32_t xx = 0; xx < clouds->w; xx++)
                {
                    if (mp[xx] == 0)
                    {
                        if ((aa[xx] & 0xFF) <= 0x10)
                            mp[xx] = 1;
                        else
                            aa[xx] -= 0x0080808;
                    }
                    else
                    {
                        if ((aa[xx] & 0xFF) >= 0xEF)
                            mp[xx] = 0;
                        else
                            aa[xx] += 0x0080808;
                    }
                }

                int32_t shift_num = GetgVarInt(SLOT_EF9_SPEED);

                for (int32_t sn = 0; sn < shift_num; sn++)
                {
                    uint32_t tmp1 = aa[0];
                    int8_t tmp2 = mp[0];

                    for (int32_t xx = 0; xx < clouds->w - 1; xx++)
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
        }
        else
        {
            LOG_WARN("Bit depth %d not supported!\n", GAME_BPP);
        }

        SDL_UnlockSurface(ef->effect.ef9.cloud);
    }

    int32_t x = ef->x;
    int32_t y = ef->y;
    int32_t w = ef->w;
    int32_t h = ef->h;

    if (Rend_GetScreenPart(&x, &y, w, h, ef->effect.ef9.mapping) == 1)
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

        if (GAME_BPP == 32)
        {
            uint32_t *srfpx = (uint32_t *)srf->pixels;
            uint32_t *mskpx = (uint32_t *)mask->pixels;
            uint32_t *cldpx = (uint32_t *)cloud->pixels;

            for (int32_t y = 0; y < minh; y++)
            {
                for (int32_t x = 0; x < minw; x++)
                    if (mskpx[x] != 0)
                    {
                        uint32_t m_r, m_g, m_b;
                        //COLOR_RGBA32(mskpx[x],m_r,m_g,m_b);

                        m_r = GetgVarInt(SLOT_EF9_R);
                        m_g = GetgVarInt(SLOT_EF9_G);
                        m_b = GetgVarInt(SLOT_EF9_B);

                        if (m_r >= 32)
                            m_r = 31;

                        if (m_g >= 32)
                            m_g = 31;

                        if (m_b >= 32)
                            m_b = 31;

                        m_r = FiveBitToEightBitLookupTable[m_r];
                        m_g = FiveBitToEightBitLookupTable[m_g];
                        m_b = FiveBitToEightBitLookupTable[m_b];

                        uint32_t c_r, c_g, c_b;
                        COLOR_RGBA32(cldpx[x], c_r, c_g, c_b);

                        uint32_t s_r, s_g, s_b;
                        COLOR_RGBA32(srfpx[x], s_r, s_g, s_b);

                        m_r = (m_r * c_r) / 0xFF;
                        m_g = (m_g * c_g) / 0xFF;
                        m_b = (m_b * c_b) / 0xFF;

                        s_r = ((s_r + m_r) > 0xFF) ? 0xFF : (s_r + m_r);
                        s_g = ((s_g + m_g) > 0xFF) ? 0xFF : (s_g + m_g);
                        s_b = ((s_b + m_b) > 0xFF) ? 0xFF : (s_b + m_b);

                        srfpx[x] = s_r | (s_g << 8) | (s_b << 16);
                    }
                srfpx += srf->w;
                mskpx += mask->w;
                cldpx += cloud->w;
            }
        }
        else
        {
            LOG_WARN("Bit depth %d not supported!\n", GAME_BPP);
        }

        SDL_UnlockSurface(srf);
        SDL_UnlockSurface(mask);
        SDL_UnlockSurface(cloud);

        DrawImageToSurf(srf, x, y, tempbuf);
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

void ConvertImage(SDL_Surface **tmp)
{
    SDL_Surface *tmp2 = SDL_ConvertSurface(*tmp, screen->format, RENDER_SURFACE);
    SDL_FreeSurface(*tmp);
    *tmp = tmp2;
}

SDL_Surface *CreateSurface(uint16_t w, uint16_t h)
{
    return SDL_CreateRGBSurface(RENDER_SURFACE, w, h, GAME_BPP, 0, 0, 0, 0);
}

void DrawAnimImageToSurf(anim_surf_t *anim, int x, int y, int frame, SDL_Surface *surf)
{
    if (!anim || !surf)
        return;

    if (frame >= anim->info.frames)
        return;

    DrawImageToSurf(anim->img[frame], x, y, surf);
}

void FreeAnimImage(anim_surf_t *anim)
{
    if (!anim)
        return;

    for (int i = 0; i < anim->info.frames; i++)
        if (anim->img[i])
            SDL_FreeSurface(anim->img[i]);

    free(anim->img);
    free(anim);
}

void DrawImage(SDL_Surface *surf, int16_t x, int16_t y)
{
    if (!surf)
        return;

    SDL_Rect tmp;
    tmp.x = x; //ceil(x*sc_fac);
    tmp.y = y; //ceil(y*sc_fac);
    tmp.w = 0;
    tmp.h = 0;
    SDL_BlitSurface(surf, 0, screen, &tmp);
}

void DrawImageToSurf(SDL_Surface *surf, int16_t x, int16_t y, SDL_Surface *dest)
{
    if (!surf || !dest)
        return;

    SDL_Rect tmp;
    tmp.x = x; //ceil(x*sc_fac);
    tmp.y = y; //ceil(y*sc_fac);
    tmp.w = 0;
    tmp.h = 0;
    //SDL_StretchSurfaceBlit(surf,0,dest,0);
    SDL_BlitSurface(surf, 0, dest, &tmp);
}

void SetColorKey(SDL_Surface *surf, int8_t r, int8_t g, int8_t b)
{
    SDL_SetColorKey(surf, SDL_SRCCOLORKEY, Rend_MapScreenRGB(r, g, b));
}

scaler_t *CreateScaler(SDL_Surface *src, uint16_t w, uint16_t h)
{
    scaler_t *tmp = NEW(scaler_t);

    tmp->surf = src;
    tmp->w = w;
    tmp->h = h;
    tmp->offsets = NULL;

    if (w == src->w && h == src->h)
        return tmp;

    tmp->offsets = NEW_ARRAY(int32_t, tmp->w * tmp->h);

    float xfac = tmp->surf->w / (float)tmp->w;
    float yfac = tmp->surf->h / (float)tmp->h;

    int32_t pos = 0;

    int32_t *tmpofs = tmp->offsets;

    for (int16_t yy = 0; yy < tmp->h; yy++)
        for (int16_t xx = 0; xx < tmp->w; xx++)
        {
            int32_t newx = xx * xfac;
            int32_t newy = yy * yfac;
            int32_t posofs = newx + newy * tmp->surf->w;

            *tmpofs = posofs - pos;

            tmpofs++;

            pos = posofs;
        }

    return tmp;
}

void DeleteScaler(scaler_t *scaler)
{
    if (scaler->offsets != NULL)
        free(scaler->offsets);

    free(scaler);
}

void DrawScaler(scaler_t *scaler, int16_t x, int16_t y, SDL_Surface *dst)
{
    if (!scaler)
        return;

    if (scaler->surf->format->BytesPerPixel != dst->format->BytesPerPixel)
        return;

    if (x >= dst->w || y >= dst->h || x <= -scaler->w || y <= -scaler->h)
        return;

    if (scaler->offsets == NULL)
    {
        DrawImageToSurf(scaler->surf, x, y, dst);
        return;
    }

    if (x >= 0 && y >= 0 && x + scaler->w <= dst->w && y + scaler->h <= dst->h)
    {
        int32_t oneline = dst->w - scaler->w;

        SDL_LockSurface(scaler->surf);
        SDL_LockSurface(dst);

        if (GAME_BPP == 32)
        {
            int32_t *ofs = (int32_t *)dst->pixels;

            ofs += y * dst->w + x;

            int32_t *ifs = (int32_t *)scaler->surf->pixels;
            int32_t *dlt = scaler->offsets;

            for (int16_t yy = 0; yy < scaler->h; yy++)
            {
                for (int16_t xx = 0; xx < scaler->w; xx++)
                {
                    ifs += *dlt;
                    *ofs = *ifs;
                    ofs++;
                    dlt++;
                }

                ofs += oneline;
            }
        }
        else if (GAME_BPP == 16)
        {
            int16_t *ofs = (int16_t *)dst->pixels;

            ofs += y * dst->w + x;

            int16_t *ifs = (int16_t *)scaler->surf->pixels;
            int32_t *dlt = scaler->offsets;

            for (int16_t yy = 0; yy < scaler->h; yy++)
            {
                for (int16_t xx = 0; xx < scaler->w; xx++)
                {
                    ifs += *dlt;
                    *ofs = *ifs;
                    ofs++;
                    dlt++;
                }

                ofs += oneline;
            }
        }
        else
        {
            LOG_WARN("Bit depth %d not supported!\n", GAME_BPP);
        }
        SDL_UnlockSurface(scaler->surf);
        SDL_UnlockSurface(dst);
    }
    else
    {
        int32_t oneline = dst->w - scaler->w;

        SDL_LockSurface(scaler->surf);
        SDL_LockSurface(dst);

        if (GAME_BPP == 32)
        {
            int32_t *ofs = (int32_t *)dst->pixels;
            int32_t *maxofs = ofs, *minofs = ofs;

            maxofs += dst->w * dst->h;

            ofs += y * dst->w + x;

            int16_t lx = 0, rx = scaler->w;

            if (x < 0)
                lx = -x;

            if (x + scaler->w >= dst->w)
                rx = dst->w - x;

            int32_t *ifs = (int32_t *)scaler->surf->pixels;
            int32_t *dlt = scaler->offsets;

            for (int16_t yy = 0; yy < scaler->h; yy++)
            {
                for (int16_t xx = 0; xx < scaler->w; xx++)
                {
                    ifs += *dlt;
                    if (ofs >= minofs && ofs < maxofs &&
                        xx >= lx && xx < rx)
                        *ofs = *ifs;
                    ofs++;
                    dlt++;
                }

                ofs += oneline;
            }
        }
        else if (GAME_BPP == 16)
        {
            int16_t *ofs = (int16_t *)dst->pixels;
            int16_t *maxofs = ofs, *minofs = ofs;

            maxofs += dst->w * dst->h;

            ofs += y * dst->w + x;

            int16_t lx = 0, rx = scaler->w;

            if (x < 0)
                lx = -x;

            if (x + scaler->w >= dst->w)
                rx = dst->w - x;

            int16_t *ifs = (int16_t *)scaler->surf->pixels;
            int32_t *dlt = scaler->offsets;

            for (int16_t yy = 0; yy < scaler->h; yy++)
            {
                for (int16_t xx = 0; xx < scaler->w; xx++)
                {
                    ifs += *dlt;
                    if (ofs >= minofs && ofs < maxofs &&
                        xx >= lx && xx < rx)
                        *ofs = *ifs;
                    ofs++;
                    dlt++;
                }

                ofs += oneline;
            }
        }
        else
        {
            LOG_WARN("Bit depth %d not supported!\n", GAME_BPP);
        }
        SDL_UnlockSurface(scaler->surf);
        SDL_UnlockSurface(dst);
    }
}

void DrawScalerToScreen(scaler_t *scaler, int16_t x, int16_t y)
{
    DrawScaler(scaler, x, y, screen);
}
