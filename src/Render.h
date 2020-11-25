#ifndef RENDER_H_INCLUDED
#define RENDER_H_INCLUDED

#define RENDER_SURFACE SDL_SWSURFACE // SDL_HWSURFACE

extern int GAME_W;
extern int GAME_H;
extern int GAME_BPP;
extern int GAMESCREEN_W;
extern int GAMESCREEN_P;
extern int GAMESCREEN_H;
extern int GAMESCREEN_X;
extern int GAMESCREEN_Y;
extern int GAMESCREEN_FLAT_X;

#define GAMESCREEN_H_2 (GAMESCREEN_H >> 1)
#define GAMESCREEN_W_2 (GAMESCREEN_W >> 1)

#define RENDER_FLAT 0
#define RENDER_PANA 1
#define RENDER_TILT 2

#define COLOR_SPLIT_RGBA888(a, r, g, b) \
    r = a & 0xFF;                \
    g = (a >> 8) & 0xFF;         \
    b = (a >> 16) & 0xFF;

#define COLOR_SPLIT_RGB565(in, r, g, b) \
    r = (in) & 0x1F;                   \
    g = ((in) >> 5) & 0x3F;            \
    b = ((in) >> 11) & 0x1F;

#define COLOR_SPLIT_RGB555(a, r, g, b) \
    r = a & 0x1F;                     \
    g = (a >> 5) & 0x1F;              \
    b = (a >> 10) & 0x1F;

#define COLOR_JOIN_RGBA888(r, g, b) ((r) & 0xFF) | (((g) & 0xFF) << 8) | (((b) & 0xFF) << 16) | (0xFF << 24)
#define COLOR_JOIN_RGB565(r, g, b) ((r) & 0x1F) | (((g) & 0x3F) << 5) | (((b) & 0x1F) << 11)
#define COLOR_JOIN_RGB555(r, g, b) ((r) & 0x1F) | (((g) & 0x1F) << 5) | (((b) & 0x1F) << 10)

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

typedef struct
{
    SDL_Surface **img;
    struct info
    {
        uint32_t w;
        uint32_t h;
        uint32_t time;
        uint32_t frames;
    } info;
} anim_surf_t;

typedef struct
{
    SDL_Surface *img;
    avi_file_t *av;
    bool pld;
    bool loop;
    int32_t lastfrm;
} anim_avi_t;

typedef struct
{
    int32_t *offsets;
    uint16_t w;
    uint16_t h;
    SDL_Surface *surf;
} scaler_t;

void Rend_DrawImageToScr(SDL_Surface *scr, int x, int y);
void Rend_DrawImageToGamescr(SDL_Surface *scr, int x, int y);
void Rend_DrawAnimImageToGamescr(anim_surf_t *scr, int x, int y, int frame);
bool Rend_LoadGamescr(const char *file);
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
void Rend_DrawImageUpGamescr(SDL_Surface *scr, int x, int y);
void Rend_DrawAnimImageUpGamescr(anim_surf_t *scr, int x, int y, int frame);
void Rend_DrawScalerToGamescr(scaler_t *scl, int16_t x, int16_t y);
int Rend_GetRenderer();
void Rend_MouseInteractOfRender();
void Rend_RenderFunc();
void Rend_InitGraphics(bool fullscreen, bool widescreen);
void Rend_SwitchFullscreen();
void Rend_SetDelay(int32_t delay);
SDL_Surface *Rend_CreateSurface(uint16_t w, uint16_t h);
SDL_Surface *Rend_GetLocationScreenImage();
SDL_Surface *Rend_GetGameScreen();
subrect_t *Rend_CreateSubRect(int x, int y, int w, int h);
void Rend_DeleteSubRect(subrect_t *erect);
void Rend_ProcessSubs();
void Rend_DelaySubDelete(subrect_t *sub, int32_t time);
uint32_t Rend_MapScreenRGB(int r, int g, int b);
void Rend_ScreenFlip();
void Rend_Delay(uint32_t delay_ms);
action_res_t *Rend_CreateDistortNode();
int32_t Rend_ProcessDistortNode(action_res_t *nod);
int32_t Rend_DeleteDistortNode(action_res_t *nod);
int Rend_DeleteRegion(action_res_t *nod);
void Rend_SetGamma(float val);
float Rend_GetGamma();
void Rend_ConvertImage(SDL_Surface **tmp);
void Rend_DrawImage(SDL_Surface *surf, int16_t x, int16_t y);
void Rend_DrawImageToSurf(SDL_Surface *surf, int16_t x, int16_t y, SDL_Surface *dest);
void Rend_SetColorKey(SDL_Surface *surf, int8_t r, int8_t g, int8_t b);
void Rend_DrawAnimImageToSurf(anim_surf_t *anim, int x, int y, int frame, SDL_Surface *surf);
void Rend_FreeAnimImage(anim_surf_t *anim);
scaler_t *Rend_CreateScaler(SDL_Surface *src, uint16_t w, uint16_t h);
void Rend_DeleteScaler(scaler_t *scal);
void Rend_DrawScaler(scaler_t *scal, int16_t x, int16_t y, SDL_Surface *dst);
void Rend_DrawScalerToScreen(scaler_t *scal, int16_t x, int16_t y);

int32_t Rend_EF_Wave_Setup(int32_t delay, int32_t frames, int32_t s_x, int32_t s_y, float apml, float waveln, float spd);
int32_t Rend_EF_Light_Setup(char *string, int32_t x, int32_t y, int32_t w, int32_t h, int32_t delay, int32_t steps);
int32_t Rend_EF_9_Setup(char *mask, char *clouds, int32_t delay, int32_t x, int32_t y, int32_t w, int32_t h);

static inline uint32_t Rend_GetPixel(SDL_Surface *surf, int x, int y)
{
    uint8_t *pixels = ((uint8_t *)surf->pixels) + y * surf->pitch;

    if (surf->format->BitsPerPixel == 32)
        return ((uint32_t *)pixels)[x];
    else
        return ((uint16_t *)pixels)[x];
}

static inline void Rend_SetPixel(SDL_Surface *surf, int x, int y, uint8_t color)
{
    uint8_t *pixels = (uint8_t *)surf->pixels + y * surf->pitch;

    if (surf->format->BitsPerPixel == 32)
        ((uint32_t *)pixels)[x] = color;
    else
        ((uint16_t *)pixels)[x] = color;
}

#endif // RENDER_H_INCLUDED
