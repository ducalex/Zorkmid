#include "System.h"

static sub_textfile_t *sub_LoadTextFile(FManNode_t *file)
{
    mfile_t *f = mfopen(file);
    if (!f)
        return NULL;

    m_wide_to_utf8(f);

    int sz = f->size;

    sub_textfile_t *tmp = NEW(sub_textfile_t);

    tmp->buffer = (char *)calloc(sz + 1, 1);

    memcpy(tmp->buffer, f->buf, f->size);
    mfclose(f);

    int linescount = 1;

    for (int i = 0; i < sz; i++)
        if (tmp->buffer[i] == 0x0A)
            linescount++;

    tmp->count = linescount;

    tmp->subs = NEW_ARRAY(char *, linescount);

    linescount = 0;
    int i = 0;
    char *curline = NULL;

    while (i < sz)
    {
        curline = tmp->buffer + i;

        while (i < sz)
        {
            if (tmp->buffer[i] == 0xA || tmp->buffer[i] == 0xD)
                break;
            i++;
        }

        if (tmp->buffer[i] == 0xD)
        {
            if (tmp->buffer[i + 1] == 0xA) //windows
            {
                tmp->buffer[i] = 0;
                tmp->buffer[i + 1] = 0;
                i += 2;
            }
            else //macOS
            {
                tmp->buffer[i] = 0;
                i++;
            }
        }
        else if (tmp->buffer[i] == 0xA) //unix
        {
            tmp->buffer[i] = 0;
            i++;
        }

        if (linescount < tmp->count)
            tmp->subs[linescount] = curline;

        linescount++;
    }

    return tmp;
}

subtitles_t *sub_LoadSubtitles(char *filename)
{
    char buf[STRBUFSIZE];
    char *str1;        // without left spaces, paramname
    char *str2 = NULL; // params

    int subscount = 0;

    subtitles_t *tmp;

    FManNode_t *fil = FindInBinTree(filename);
    if (!fil)
        return NULL;

    tmp = NEW(subtitles_t);

    tmp->currentsub = -1;
    tmp->SubRect = NULL;
    tmp->subscount = 0;

    mfile_t *f = mfopen(fil);
    while (!mfeof(f))
    {
        mfgets(buf, STRBUFSIZE, f);
        str1 = TrimLeft(buf);

        str2 = NULL;

        int32_t t_len = strlen(str1);

        for (int i = 0; i < t_len; i++)
            if (str1[i] == ':')
            {
                str1[i] = 0x0;
                str2 = str1 + i + 1;
                break;
            }

        if (str2 != NULL)
        {
            for (int i = strlen(str2) - 1; i >= 0; i--)
                if (str2[i] == '~' || str2[i] == 0x0A || str2[i] == 0x0D)
                    str2[i] = 0x0;

            if (strCMP(str1, "Initialization") == 0)
            {
                ;
            }
            else if (strCMP(str1, "Rectangle") == 0)
            {
                int x, y, x2, y2;
                sscanf(str2, "%d %d %d %d", &x, &y, &x2, &y2);
                tmp->SubRect = Rend_CreateSubRect(x + GAMESCREEN_X + SUB_CORRECT_HORIZ + GAMESCREEN_FLAT_X,
                                                  y + GAMESCREEN_Y + SUB_CORRECT_VERT,
                                                  x2 - x + 1,
                                                  y2 - y + 1);
            }
            else if (strCMP(str1, "TextFile") == 0)
            {
                FManNode_t *fil2 = FindInBinTree(str2);
                if (fil2 == NULL)
                {
                    free(tmp);
                    return NULL;
                }

                tmp->txt = sub_LoadTextFile(fil2);
                tmp->subs = NEW_ARRAY(one_subtitle_t, tmp->txt->count);
                subscount = tmp->txt->count;
            }
            else //it's must be sub info
            {
                int st, en, sb;
                if (sscanf(str2, "(%d,%d)=%d", &st, &en, &sb) == 3)
                {
                    if (subscount == 0 || sb > subscount)
                    {
                        Z_PANIC("Error in subs %s\n", filename);
                    }
                    tmp->subs[tmp->subscount].start = st;
                    tmp->subs[tmp->subscount].stop = en;
                    tmp->subs[tmp->subscount].sub = sb;

                    tmp->subscount++;
                }
            }
        }
    }
    mfclose(f);

    return tmp;
}

void sub_ProcessSub(subtitles_t *sub, int subtime)
{
    int j = -1;
    for (int i = 0; i < sub->subscount; i++)
        if (subtime >= sub->subs[i].start && subtime <= sub->subs[i].stop)
        {
            j = i;
            break;
        }

    if (j == -1 && sub->currentsub != -1)
    {
        SDL_FillRect(sub->SubRect->img, NULL, SDL_MapRGBA(sub->SubRect->img->format, 0, 0, 0, 255));
        sub->currentsub = -1;
    }

    if (j != -1 && j != sub->currentsub)
    {
        char *sss = sub->txt->subs[sub->subs[j].sub];
        if (sss != NULL && strlen(sss) > 0)
        {
            SDL_FillRect(sub->SubRect->img, NULL, SDL_MapRGBA(sub->SubRect->img->format, 0, 0, 0, 255));
            txt_DrawTxtInOneLine(sss, sub->SubRect->img);
        }
        sub->currentsub = j;
    }
}

void sub_DeleteSub(subtitles_t *sub)
{
    Rend_DeleteSubRect(sub->SubRect);
    free(sub->txt->buffer);
    free(sub->txt->subs);
    free(sub->txt);
    free(sub->subs);
    free(sub);
}
