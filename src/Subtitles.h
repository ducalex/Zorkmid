#ifndef SUBTITLES_H_INCLUDED
#define SUBTITLES_H_INCLUDED

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

subtitles_t *Subtitles_Load(char *filename);
void Subtitles_Process(subtitles_t *sub, int subtime);
void Subtitles_Delete(subtitles_t *sub);

#endif // SUBTITLES_H_INCLUDED
