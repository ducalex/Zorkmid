#ifndef RENDER_H_INCLUDED
#define RENDER_H_INCLUDED

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

#define COLOR_RGBA32(a, r, g, b) \
    r = a & 0xFF;                \
    g = (a >> 8) & 0xFF;         \
    b = (a >> 16) & 0xFF;

#define COLOR_RGB16_565(a, r, g, b) \
    r = a & 0x1F;                   \
    g = (a >> 5) & 0x3F;            \
    b = (a >> 11) & 0x1F;

#define COLOR_RGBA16_5551(a, r, g, b) \
    r = a & 0x1F;                     \
    g = (a >> 5) & 0x1F;              \
    b = (a >> 10) & 0x1F;

#define COLOR_RGBA16_4444(a, r, g, b) \
    r = a & 0xF;                      \
    g = (a >> 4) & 0xF;               \
    b = (a >> 8) & 0xF;

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

struct anim_surf
{
    SDL_Surface **img;
    struct info {
        uint32_t w;
        uint32_t h;
        uint32_t time;
        uint32_t frames;
    } info;
};

#ifdef SMPEG_SUPPORT
struct anim_mpg
{
    SDL_Surface *img;
    SMPEG *mpg;
    SMPEG_Info inf;
    bool pld;
    bool loop;
    int32_t lastfrm;
};
#endif

struct anim_avi
{
    SDL_Surface *img;
    avi_file_t *av;
    bool pld;
    bool loop;
    int32_t lastfrm;
};

struct scaler
{
    int32_t *offsets;
    uint16_t w;
    uint16_t h;
    SDL_Surface *surf;
};

void Rend_DrawImageToScr(SDL_Surface *scr, int x, int y);
void Rend_DrawImageToGamescr(SDL_Surface *scr, int x, int y);
void Rend_DrawAnimImageToGamescr(anim_surf *scr, int x, int y, int frame);
int8_t Rend_LoadGamescr(const char *file);
void Rend_ProcessCursor();
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
void Rend_DrawTilt();
void Rend_DrawTilt_pre();
float Rend_GetRendererAngle();
float Rend_GetRendererLinscale();
void Rend_SetRendererAngle(float angle);
void Rend_SetRendererLinscale(float lin);
void Rend_SetRendererTable();
void Rend_DrawImageUpGamescr(SDL_Surface *scr, int x, int y);
void Rend_DrawAnimImageUpGamescr(anim_surf *scr, int x, int y, int frame);
void Rend_DrawScalerToGamescr(scaler *scl, int16_t x, int16_t y);
int Rend_GetRenderer();
void Rend_ProcessCursor();
void Rend_PanaMouseInteract();
void Rend_MouseInteractOfRender();
void Rend_tilt_MouseInteract();
void Rend_RenderFunc();
void Rend_InitGraphics(bool fullscreen, bool widescreen);
void Rend_SwitchFullscreen();
void Rend_SetDelay(int32_t delay);
SDL_Surface *Rend_GetLocationScreenImage();
subrect_t *Rend_CreateSubRect(int x, int y, int w, int h);
void Rend_DeleteSubRect(subrect_t *erect);
void Rend_ProcessSubs();
void Rend_DelaySubDelete(subrect_t *sub, int32_t time);
SDL_Surface *Rend_GetGameScreen();
uint32_t Rend_MapScreenRGB(int r, int g, int b);
void Rend_ScreenFlip();
void Rend_Delay(uint32_t delay_ms);
action_res_t *Rend_CreateDistortNode();
int32_t Rend_ProcessDistortNode(action_res_t *nod);
int32_t Rend_DeleteDistortNode(action_res_t *nod);
int Rend_DeleteRegion(action_res_t *nod);
int8_t Rend_GetScreenPart(int32_t *x, int32_t *y, int32_t w, int32_t h, SDL_Surface *dst);

int32_t Rend_EF_Wave_Setup(int32_t delay, int32_t frames, int32_t s_x, int32_t s_y, float apml, float waveln, float spd);
int32_t Rend_EF_Light_Setup(char *string, int32_t x, int32_t y, int32_t w, int32_t h, int32_t delay, int32_t steps);
int32_t Rend_EF_9_Setup(char *mask, char *clouds, int32_t delay, int32_t x, int32_t y, int32_t w, int32_t h);


SDL_Surface *CreateSurface(uint16_t w, uint16_t h);
void ConvertImage(SDL_Surface **tmp);
void DrawImage(SDL_Surface *surf, int16_t x, int16_t y);
void DrawImageToSurf(SDL_Surface *surf, int16_t x, int16_t y, SDL_Surface *dest);
void SetColorKey(SDL_Surface *surf, int8_t r, int8_t g, int8_t b);
void DrawAnimImageToSurf(anim_surf *anim, int x, int y, int frame, SDL_Surface *surf);
anim_surf *LoadAnimImage(const char *file, int32_t mask);
void FreeAnimImage(anim_surf *anim);
scaler *CreateScaler(SDL_Surface *src, uint16_t w, uint16_t h);
void DeleteScaler(scaler *scal);
void DrawScaler(scaler *scal, int16_t x, int16_t y, SDL_Surface *dst);
void DrawScalerToScreen(scaler *scal, int16_t x, int16_t y);
void setGamma(float val);
float getGamma();

#endif // RENDER_H_INCLUDED
