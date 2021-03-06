#ifndef RENDER_H_INCLUDED
#define RENDER_H_INCLUDED

extern int WINDOW_W;
extern int WINDOW_H;
extern int GAMESCREEN_W;
extern int GAMESCREEN_P;
extern int GAMESCREEN_H;
extern int GAMESCREEN_X;
extern int GAMESCREEN_Y;
extern int FULLSCREEN;

#define GAMESCREEN_H_2 (GAMESCREEN_H >> 1)
#define GAMESCREEN_W_2 (GAMESCREEN_W >> 1)

#define RENDER_FLAT 0
#define RENDER_PANA 1
#define RENDER_TILT 2

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint32_t pixel;
} color_t;

typedef struct
{
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
    SDL_Surface *img;
    int32_t id;
    int32_t timer;
    bool todelete;
} subrect_t;

typedef struct
{
    int32_t speed;
    float st_angl;
    float end_angl;
    float dif_angl;
    float rend_angl;
    float st_lin;
    float end_lin;
    float dif_lin;
    float rend_lin;
    int32_t frames;
    int32_t cur_frame;
    bool increase;
    float param1;
} distort_t;

void Rend_LoadGamescr(const char *file);
int Rend_GetMouseGameX();
int Rend_GetMouseGameY();
bool Rend_MouseInGamescr();
int Rend_GetPanaWidth();
void Rend_SetReversePana(bool pana);
void Rend_SetRenderer(int meth);
void Rend_pana_SetTable();
void Rend_pana_SetAngle(float angle);
void Rend_pana_SetLinscale(float linscale);
void Rend_pana_SetZeroPoint(int32_t point);
void Rend_tilt_SetTable();
void Rend_tilt_SetAngle(float angle);
void Rend_tilt_SetLinscale(float linscale);
float Rend_GetRendererAngle();
float Rend_GetRendererLinscale();
int Rend_GetRenderer();
void Rend_MouseInteractOfRender();
void Rend_RenderFrame();
void Rend_SetVideoMode(int w, int h, int full);
void Rend_Init(int full);
void Rend_InitWindow();
void Rend_SetDelay(int32_t delay);
void Rend_ScreenFlip();
void Rend_SetGamma(float val);
float Rend_GetGamma();

action_res_t *Rend_CreateNode(int type);
int Rend_ProcessNode(action_res_t *nod);
int Rend_DeleteNode(action_res_t *nod);

SDL_Surface *Rend_CreateSurface(int w, int h, int mode);
SDL_Surface *Rend_GetLocationScreenImage();
SDL_Surface *Rend_GetGameScreen();
SDL_Surface *Rend_GetScreen();

void Rend_BlitSurfaceXY(SDL_Surface *src, SDL_Surface *dst, int x, int y);
void Rend_BlitSurface(SDL_Surface *src, SDL_Rect *src_rct, SDL_Surface *dst, SDL_Rect *dst_rct);
void Rend_SetColorKey(SDL_Surface *surf, uint8_t r, uint8_t g, uint8_t b);
void Rend_FillRect(SDL_Surface *surf, SDL_Rect *dst_rct, uint8_t r, uint8_t g, uint8_t b);

int32_t Rend_EF_Wave_Setup(int32_t delay, int32_t frames, int32_t s_x, int32_t s_y, float apml, float waveln, float spd);
int32_t Rend_EF_Light_Setup(char *string, int32_t x, int32_t y, int32_t w, int32_t h, int32_t delay, int32_t steps);
int32_t Rend_EF_9_Setup(char *mask, char *clouds, int32_t delay, int32_t x, int32_t y, int32_t w, int32_t h);

static inline color_t Rend_GetPixel(SDL_Surface *surf, int x, int y)
{
    uint8_t *pixels = ((uint8_t *)surf->pixels) + y * surf->pitch;
    color_t ret;

    if (surf->format->BitsPerPixel == 32)
        ret.pixel = ((uint32_t *)pixels)[x];
    else
        ret.pixel = ((uint16_t *)pixels)[x];

    SDL_GetRGB(ret.pixel, surf->format, &ret.r, &ret.g, &ret.b);

    return ret;
}

static inline void Rend_SetPixel(SDL_Surface *surf, int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t *pixels = (uint8_t *)surf->pixels + y * surf->pitch;
    uint32_t color = SDL_MapRGB(surf->format, r, g, b);

    if (surf->format->BitsPerPixel == 32)
        ((uint32_t *)pixels)[x] = color;
    else
        ((uint16_t *)pixels)[x] = color;
}

#endif // RENDER_H_INCLUDED
