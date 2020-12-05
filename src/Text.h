#ifndef TEXT_H_INCLUDED
#define TEXT_H_INCLUDED

typedef struct struct_action_res action_res_t;

typedef struct
{
    char fontname[64];
    uint8_t red;   //0-255
    uint8_t green; //0-255
    uint8_t blue;  //0-255
    int statebox;
    int newline;
    int escapement;
    int justify; //0 - center, 1-left, 2-right
    int size;
    bool italic;
    bool bold;
    bool underline;
    bool strikeout;
    bool skipcolor;
    void *image;
} txt_style_t;

typedef struct
{
    int x, y;
    int w, h;
    int txtpos;
    int txtsize;
    int delay;
    int nexttime;
    int dx, dy;
    char *txtbuf;
    txt_style_t style;
    TTF_Font *fnt;
    SDL_Surface *img;
} ttytext_t;

typedef struct
{
    int start;
    int stop;
    int sub;
    char *txt;
} subtitle_t;

typedef struct
{
    subrect_t *SubRect;
    subtitle_t *subs;
    int count;
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
