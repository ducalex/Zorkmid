#ifndef SUBTITLES_H_INCLUDED
#define SUBTITLES_H_INCLUDED

#define SUB_DEF_RECT 0, 412, 640, 68
#define SUB_CORRECT_VERT (-14)
#define SUB_CORRECT_HORIZ 0

#include "Render.h"

typedef struct
{
    int start;
    int stop;
    int sub;
} one_subtitle_t;

typedef struct
{
    int count;
    char *buffer; //for all subs
    char **subs;  //for access to subs
} sub_textfile_t;

typedef struct
{
    struct_SubRect *SubRect;
    int subscount; //number of subs;
    one_subtitle_t *subs;
    sub_textfile_t *txt; //array
    int currentsub;
} subtitles_t;

subtitles_t *sub_LoadSubtitles(char *filename);
void sub_ProcessSub(subtitles_t *sub, int subtime);
void sub_DeleteSub(subtitles_t *sub);

#endif // SUBTITLES_H_INCLUDED
