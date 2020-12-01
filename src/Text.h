#ifndef TEXT_H_INCLUDED
#define TEXT_H_INCLUDED

#define TXT_STYLE_VAR_TRUE 1
#define TXT_STYLE_VAR_FALSE 0

#define TXT_JUSTIFY_CENTER 0
#define TXT_JUSTIFY_LEFT 1
#define TXT_JUSTIFY_RIGHT 2

#define TXT_RET_NOTHING 0x0
#define TXT_RET_FNTCHG 0x1
#define TXT_RET_FNTSTL 0x2
#define TXT_RET_NEWLN 0x4
#define TXT_RET_HASSTBOX 0x8

#define TXT_CFG_FONTNAME_MAX_LEN 64
#define TXT_CFG_BUF_MAX_LEN 512
#define TXT_CFG_TEXTURES_LINES 256
#define TXT_CFG_TEXTURES_PER_LINE 6

typedef struct
{
    char fontname[TXT_CFG_FONTNAME_MAX_LEN];
    int8_t justify; //0 - center, 1-left, 2-right
    int16_t size;
    uint8_t red;   //0-255
    uint8_t green; //0-255
    uint8_t blue;  //0-255
    int8_t newline;
    int8_t escapement;
    int8_t italic;    //0 - OFF, 1 - ON
    int8_t bold;      //0 - OFF, 1 - ON
    int8_t underline; //0 - OFF, 1 - ON
    int8_t strikeout; //0 - OFF, 1 - ON
    int8_t skipcolor; //0 - OFF, 1 - ON
    int32_t statebox;
    //char image ??
} txt_style_t;

typedef struct
{
    int32_t x, y;
    int32_t w, h;
    txt_style_t style;
    TTF_Font *fnt;
    char *txtbuf;
    int32_t txtpos;
    int32_t txtsize;
    int32_t delay;
    int32_t nexttime;
    SDL_Surface *img;
    int32_t dx, dy;
} ttytext_t;

typedef struct
{
    int start;
    int stop;
    int sub;
} subtitle_t;

typedef struct
{
    subrect_t *SubRect;
    subtitle_t *subs; // Subtitle indices
    char **txt;           // Subtitles text
    int count;          // Subtittles count
    int currentsub;
} subtitles_t;

void Text_Init();
void Text_InitStyle(txt_style_t *style);
void Text_GetStyle(txt_style_t *style, const char *strin);
void Text_SetStyle(TTF_Font *font, txt_style_t *fnt_stl);
size_t Text_Draw(const char *txt, txt_style_t *fnt_stl, SDL_Surface *dst);
void Text_DrawInOneLine(const char *text, SDL_Surface *dst);
void Text_DrawSubtitles();

subrect_t *Text_CreateSubRect(int x, int y, int w, int h);
void Text_DeleteSubRect(subrect_t *rect);

action_res_t *Text_CreateTTYText();
int Text_DeleteTTYText(action_res_t *nod);
int Text_ProcessTTYText(action_res_t *nod);

subtitles_t *Text_LoadSubtitles(char *filename);
void Text_ProcessSubtitles(subtitles_t *sub, int subtime);
void Text_DeleteSubtitles(subtitles_t *sub);

#endif // TEXT_H_INCLUDED
