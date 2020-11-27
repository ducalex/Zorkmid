#include "System.h"

subtitles_t *Subtitles_Load(char *filename)
{
    char buf[STRBUFSIZE];
    char *str1;        // without left spaces, paramname
    char *str2 = NULL; // params

    int subscount = 0;

    subtitles_t *tmp;

    FManNode_t *fil = Loader_FindNode(filename);
    if (!fil)
        return NULL;

    tmp = NEW(subtitles_t);
    tmp->currentsub = -1;

    mfile_t *f = mfopen(fil);
    while (!mfeof(f))
    {
        mfgets(buf, STRBUFSIZE, f);

        str1 = (char*)str_ltrim(buf);
        str2 = NULL;

        int32_t t_len = strlen(str1);

        for (int i = 0; i < t_len; i++)
            if (str1[i] == ':')
            {
                str1[i] = 0x0;
                str2 = str1 + i + 1;
                break;
            }

        if (str_empty(str2))
            continue;

        for (int i = strlen(str2) - 1; i >= 0; i--)
            if (str2[i] == '~' || str2[i] == 0x0A || str2[i] == 0x0D)
                str2[i] = 0x0;

        if (str_starts_with(str1, "Initialization"))
        {
            ;
        }
        else if (str_starts_with(str1, "Rectangle"))
        {
            int x, y, x2, y2;
            sscanf(str2, "%d %d %d %d", &x, &y, &x2, &y2);
            tmp->SubRect = Rend_CreateSubRect(x + GAMESCREEN_X + SUB_CORRECT_HORIZ + GAMESCREEN_FLAT_X,
                                                y + GAMESCREEN_Y + SUB_CORRECT_VERT,
                                                x2 - x + 1,
                                                y2 - y + 1);
        }
        else if (str_starts_with(str1, "TextFile"))
        {
            FManNode_t *fil2 = Loader_FindNode(str2);
            if (fil2 == NULL)
            {
                free(tmp);
                return NULL;
            }

            tmp->txt = Loader_LoadSTR_m(mfopen(fil2));
            subscount = 0;
            while (tmp->txt[subscount])
                subscount++;
            tmp->subs = NEW_ARRAY(subtitle_t, subscount);
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
                tmp->subs[tmp->count].start = st;
                tmp->subs[tmp->count].stop = en;
                tmp->subs[tmp->count].sub = sb;

                tmp->count++;
            }
        }
    }
    mfclose(f);

    return tmp;
}

void Subtitles_Process(subtitles_t *sub, int subtime)
{
    int j = -1;
    for (int i = 0; i < sub->count; i++)
        if (subtime >= sub->subs[i].start && subtime <= sub->subs[i].stop)
        {
            j = i;
            break;
        }

    if (j == -1 && sub->currentsub != -1)
    {
        Rend_FillRect(sub->SubRect->img, NULL, 0, 0, 0);
        sub->currentsub = -1;
    }

    if (j != -1 && j != sub->currentsub)
    {
        char *sss = sub->txt[sub->subs[j].sub];
        if (!str_empty(sss))
        {
            Rend_FillRect(sub->SubRect->img, NULL, 0, 0, 0);
            Text_DrawInOneLine(sss, sub->SubRect->img);
        }
        sub->currentsub = j;
    }
}

void Subtitles_Delete(subtitles_t *sub)
{
    if (sub->txt)
    {
        for (int i = 0; i < sub->count; i++)
            free(sub->txt[i]);
    }
    Rend_DeleteSubRect(sub->SubRect);
    free(sub->txt);
    free(sub->subs);
    free(sub);
}
