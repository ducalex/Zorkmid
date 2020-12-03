#include "System.h"

#define TXT_JUSTIFY_CENTER 0
#define TXT_JUSTIFY_LEFT 1
#define TXT_JUSTIFY_RIGHT 2

#define TXT_RET_NOTHING 0x0
#define TXT_RET_FNTCHG 0x1
#define TXT_RET_FNTSTL 0x2
#define TXT_RET_NEWLN 0x4
#define TXT_RET_HASSTBOX 0x8

#define TXT_CFG_BUF_MAX_LEN 512
#define TXT_CFG_TEXTURES_LINES 256
#define TXT_CFG_TEXTURES_PER_LINE 6

static dynlist_t subs_list;

static SDL_Surface *RenderUTF8(TTF_Font *fnt, const char *text, txt_style_t *style)
{
    Text_SetStyle(fnt, style);
    SDL_Color clr = {style->red, style->green, style->blue, 255};
    return TTF_RenderUTF8_Solid(fnt, text, clr);
}

static uint8_t txt_parse_txt_params(txt_style_t *style, const char *str, size_t len)
{
    char buf[TXT_CFG_BUF_MAX_LEN];
    memcpy(buf, str, len);
    buf[len] = 0x0;

    const char *find = " ";
    const char *token = strtok(buf, find);
    uint8_t retval = TXT_RET_NOTHING;

    while (token != NULL)
    {
        if (str_equals(token, "font"))
        {
            if ((token = strtok(NULL, find)))
            {
                if (token[0] == '"')
                {
                    strcpy(style->fontname, token + 1);
                    while (token && token[strlen(token)-1] != '"')
                    {
                        if ((token = strtok(NULL, find)))
                        {
                            strcat(style->fontname, " ");
                            strcat(style->fontname, token);
                        }
                    }
                    style->fontname[strlen(style->fontname)-1] = 0;
                }
                else
                {
                    strcpy(style->fontname, token);
                }
                retval |= TXT_RET_FNTCHG;
            }
        }
        else if (str_equals(token, "blue"))
        {
            if ((token = strtok(NULL, find)))
            {
                int tmp = atoi(token);
                if (style->blue != tmp)
                {
                    style->blue = tmp;
                    retval |= TXT_RET_FNTSTL;
                }
            }
        }
        else if (str_equals(token, "red"))
        {
            if ((token = strtok(NULL, find)))
            {
                int tmp = atoi(token);
                if (style->red != tmp)
                {
                    style->red = tmp;
                    retval |= TXT_RET_FNTSTL;
                }
            }
        }
        else if (str_equals(token, "green"))
        {
            if ((token = strtok(NULL, find)))
            {
                int tmp = atoi(token);
                if (style->green != tmp)
                {
                    style->green = tmp;
                    retval |= TXT_RET_FNTSTL;
                }
            }
        }
        else if (str_equals(token, "newline"))
        {
            if ((retval & TXT_RET_NEWLN) == 0)
                style->newline = 0;

            style->newline++;
            retval |= TXT_RET_NEWLN;
        }
        else if (str_equals(token, "point"))
        {
            if ((token = strtok(NULL, find)))
            {
                int tmp = atoi(token);
                if (style->size != tmp)
                {
                    style->size = tmp;
                    retval |= TXT_RET_FNTCHG;
                }
            }
        }
        else if (str_equals(token, "escapement"))
        {
            if ((token = strtok(NULL, find)))
                style->escapement = atoi(token);
        }
        else if (str_equals(token, "italic"))
        {
            if ((token = strtok(NULL, find)))
            {
                int status = str_equals(token, "on");
                if (style->italic != status)
                {
                    style->italic = status;
                    retval |= TXT_RET_FNTSTL;
                }
            }
        }
        else if (str_equals(token, "underline"))
        {
            if ((token = strtok(NULL, find)))
            {
                int status = str_equals(token, "on");
                if (style->underline != status)
                {
                    style->underline = status;
                    retval |= TXT_RET_FNTSTL;
                }
            }
        }
        else if (str_equals(token, "strikeout"))
        {
            if ((token = strtok(NULL, find)))
            {
                int status = str_equals(token, "on");
                if (style->strikeout != status)
                {
                    style->strikeout = status;
                    retval |= TXT_RET_FNTSTL;
                }
            }
        }
        else if (str_equals(token, "bold"))
        {
            if ((token = strtok(NULL, find)))
            {
                int status = str_equals(token, "on");
                if (style->bold != status)
                {
                    style->bold = status;
                    retval |= TXT_RET_FNTSTL;
                }
            }
        }
        else if (str_equals(token, "skipcolor"))
        {
            if ((token = strtok(NULL, find)))
            {
                int status = str_equals(token, "on");
                if (style->skipcolor != status)
                {
                    style->skipcolor = status;
                    retval |= TXT_RET_FNTSTL;
                }
            }
        }
        else if (str_equals(token, "image"))
        {
            //token = strtok(NULL,find);
        }
        else if (str_equals(token, "statebox"))
        {
            if ((token = strtok(NULL, find)))
            {
                style->statebox = atoi(token);
                retval |= TXT_RET_HASSTBOX;
            }
        }
        else if (str_equals(token, "justify"))
        {
            if ((token = strtok(NULL, find)))
            {
                if (str_equals(token, "center"))
                    style->justify = TXT_JUSTIFY_CENTER;
                else if (str_equals(token, "left"))
                    style->justify = TXT_JUSTIFY_LEFT;
                else if (str_equals(token, "right"))
                    style->justify = TXT_JUSTIFY_RIGHT;
                else
                    LOG_WARN("Invalid token following justify: '%s'\n", token);
            }
        }

        // We should skip this if the previous token wasn't a valid parameter above...
        // (though that would mean a parse error, so consuming it is equally valid I guess)
        token = strtok(NULL, find);
    }
    return retval;
}

static void ttynewline(ttytext_t *tty)
{
    tty->dy += tty->style.size;
    tty->dx = 0;
}

static void ttyscroll(ttytext_t *tty)
{
    int scroll = 0;
    while (tty->dy - scroll > tty->h - tty->style.size)
        scroll += tty->style.size;
    SDL_Surface *tmp = Rend_CreateSurface(tty->w, tty->h, 0);
    Rend_BlitSurfaceXY(tty->img, tmp, 0, -scroll);
    SDL_FreeSurface(tty->img);
    tty->img = tmp;
    tty->dy -= scroll;
}

static void outchartotty(uint16_t chr, ttytext_t *tty)
{
    SDL_Color clr = {tty->style.red, tty->style.green, tty->style.blue, 255};

    SDL_Surface *tmp_surf = TTF_RenderGlyph_Solid(tty->fnt, chr, clr);

    int minx, maxx, miny, maxy, advice;
    TTF_GlyphMetrics(tty->fnt, chr, &minx, &maxx, &miny, &maxy, &advice);

    if (tty->dx + advice > tty->w)
        ttynewline(tty);

    if (tty->dy + tty->style.size + tty->style.size / 4 > tty->h)
        ttyscroll(tty);

    Rend_BlitSurfaceXY(tmp_surf, tty->img, tty->dx, tty->dy + tty->style.size - maxy);

    tty->dx += advice;

    SDL_FreeSurface(tmp_surf);
}

static int getglyphwidth(TTF_Font *fnt, uint16_t chr)
{
    int minx, maxx, miny, maxy, advice;
    TTF_GlyphMetrics(fnt, chr, &minx, &maxx, &miny, &maxy, &advice);
    return advice;
}

void Text_Init()
{
    //
}

void Text_InitStyle(txt_style_t *style)
{
    if (style == NULL)
        Z_PANIC("style is NULL")

    style->blue = 255;
    style->green = 255;
    style->red = 255;
    strcpy(style->fontname, "Arial");
    style->escapement = 0;
    style->justify = TXT_JUSTIFY_LEFT;
    style->newline = 0;
    style->size = 12;
    style->bold = false;
    style->italic = false;
    style->strikeout = false;
    style->underline = false;
    style->skipcolor = false;
    style->statebox = 0;
}

void Text_GetStyle(txt_style_t *style, const char *strin)
{
    int strt = -1;
    int endt = -1;

    size_t t_len = strlen(strin);

    for (size_t i = 0; i < t_len; i++)
    {
        if (strin[i] == '<')
            strt = i;
        else if (strin[i] == '>')
        {
            endt = i;
            if (strt != -1 && (endt - strt - 1) > 0)
                txt_parse_txt_params(style, strin + strt + 1, endt - strt - 1);
        }
    }
}

void Text_SetStyle(TTF_Font *font, txt_style_t *fnt_stl)
{
    int style = 0;

    if (fnt_stl->bold)
        style |= TTF_STYLE_BOLD;
    if (fnt_stl->italic)
        style |= TTF_STYLE_ITALIC;
    if (fnt_stl->underline)
        style |= TTF_STYLE_UNDERLINE;
    if (fnt_stl->strikeout)
        style |= TTF_STYLE_STRIKETHROUGH;

    if (TTF_GetFontStyle(font) != style)
    {
        TTF_SetFontStyle(font, style);
    }
}

size_t Text_Draw(const char *txt, txt_style_t *fnt_stl, SDL_Surface *dst)
{
    TTF_Font *fnt = Loader_LoadFont(fnt_stl->fontname, fnt_stl->size);
    if (!fnt)
        Z_PANIC("TTF_OpenFont: %s\n", TTF_GetError());

    Rend_FillRect(dst, NULL, 0, 0, 0);

    SDL_Surface *aaa = RenderUTF8(fnt, txt, fnt_stl);
    size_t width = aaa->w;

    if (fnt_stl->justify == TXT_JUSTIFY_LEFT)
    {
        Rend_BlitSurfaceXY(aaa, dst, 0, fnt_stl->size - aaa->h);
    }
    else if (fnt_stl->justify == TXT_JUSTIFY_CENTER)
    {
        Rend_BlitSurfaceXY(aaa, dst, (dst->w - aaa->w) / 2, fnt_stl->size - aaa->h);
    }
    else if (fnt_stl->justify == TXT_JUSTIFY_RIGHT)
    {
        Rend_BlitSurfaceXY(aaa, dst, dst->w - aaa->w, fnt_stl->size - aaa->h);
    }

    SDL_FreeSurface(aaa);
    TTF_CloseFont(fnt);

    return width;
}

void Text_DrawInOneLine(const char *text, SDL_Surface *dst)
{
    txt_style_t style, style2;
    int strt = -1;
    int endt = -1;
    int i = 0;
    int dx = 0, dy = 0;
    int txt_w, txt_h;
    int txtpos = 0;
    int prevbufspace = 0;
    int prevtxtspace = 0;
    int currentline = 0;
    int currentlineitm = 0;
    char buf[TXT_CFG_BUF_MAX_LEN];
    char buf2[TXT_CFG_BUF_MAX_LEN];
    int8_t TxtJustify[TXT_CFG_TEXTURES_LINES];
    int8_t TxtPoint[TXT_CFG_TEXTURES_LINES];
    SDL_Surface *TxtSurfaces[TXT_CFG_TEXTURES_LINES][TXT_CFG_TEXTURES_PER_LINE];
    size_t stringlen = strlen(text);
    TTF_Font *font = NULL;

    memset(TxtSurfaces, 0, sizeof(TxtSurfaces));
    memset(buf, 0, TXT_CFG_BUF_MAX_LEN);
    memset(buf2, 0, TXT_CFG_BUF_MAX_LEN);

    Text_InitStyle(&style);
    font = Loader_LoadFont(style.fontname, style.size);
    Text_SetStyle(font, &style);

    while (i < stringlen)
    {
        TxtJustify[currentline] = style.justify;
        TxtPoint[currentline] = style.size;
        if (text[i] == '<')
        {
            int32_t ret = 0;

            strt = i;
            while (i < stringlen && text[i] != '>')
                i++;
            endt = i;
            if (strt != -1)
                if ((endt - strt - 1) > 0)
                {
                    style2 = style;
                    ret = txt_parse_txt_params(&style, text + strt + 1, endt - strt - 1);
                }

            if (ret & (TXT_RET_FNTCHG | TXT_RET_FNTSTL | TXT_RET_NEWLN))
            {
                if (!str_empty(buf))
                {
                    TTF_SizeUTF8(font, buf, &txt_w, &txt_h);

                    TxtSurfaces[currentline][currentlineitm] = RenderUTF8(font, buf, &style2);

                    currentlineitm++;

                    memset(buf, 0, TXT_CFG_BUF_MAX_LEN);
                    prevbufspace = 0;
                    txtpos = 0;
                    dx += txt_w;
                }

                if (ret & TXT_RET_FNTCHG)
                {
                    TTF_CloseFont(font);
                    font = Loader_LoadFont(style.fontname, style.size);
                    Text_SetStyle(font, &style);
                }
                else if (ret & TXT_RET_FNTSTL)
                {
                    Text_SetStyle(font, &style);
                }

                if (ret & TXT_RET_NEWLN)
                {
                    currentline++;
                    currentlineitm = 0;
                    dx = 0;
                }
            }

            if (ret & TXT_RET_HASSTBOX)
            {
                char buf3[MINIBUFSIZE];
                sprintf(buf3, "%d", GetgVarInt(style.statebox));
                strcat(buf, buf3);
                txtpos += strlen(buf3);
            }
        }
        else
        {
            buf[txtpos++] = text[i];

            if (text[i] == ' ')
            {
                prevbufspace = txtpos - 1;
                prevtxtspace = i;
            }

            if (font != NULL)
            {
                TTF_SizeUTF8(font, buf, &txt_w, &txt_h);
                if (txt_w + dx > dst->w)
                {
                    if (prevbufspace == 0)
                    {
                        prevtxtspace = i;
                        prevbufspace = txtpos - 1;
                    }
                    memcpy(buf2, buf, prevbufspace + 1);
                    buf2[prevbufspace + 1] = 0x0;

                    if (!str_empty(buf2))
                        TxtSurfaces[currentline][currentlineitm] = RenderUTF8(font, buf2, &style);

                    memset(buf, 0, TXT_CFG_BUF_MAX_LEN);
                    i = prevtxtspace;
                    prevbufspace = 0;
                    txtpos = 0;
                    currentline++;
                    currentlineitm = 0;
                    dx = 0;
                }
            }
        }
        i++;
    }

    if (!str_empty(buf))
        TxtSurfaces[currentline][currentlineitm] = RenderUTF8(font, buf, &style);

    dy = 0;
    for (i = 0; i <= currentline; i++)
    {
        int j = 0, width = 0;
        while (TxtSurfaces[i][j] != NULL)
        {
            width += TxtSurfaces[i][j]->w;
            j++;
        }
        dx = 0;
        for (int jj = 0; jj < j; jj++)
        {
            if (TxtJustify[i] == TXT_JUSTIFY_LEFT)
            {
                Rend_BlitSurfaceXY(TxtSurfaces[i][jj], dst, dx, dy + TxtPoint[i] - TxtSurfaces[i][jj]->h);
            }
            else if (TxtJustify[i] == TXT_JUSTIFY_CENTER)
            {
                Rend_BlitSurfaceXY(TxtSurfaces[i][jj], dst, ((dst->w - width) >> 1) + dx, dy + TxtPoint[i] - TxtSurfaces[i][jj]->h);
            }
            else if (TxtJustify[i] == TXT_JUSTIFY_RIGHT)
            {
                Rend_BlitSurfaceXY(TxtSurfaces[i][jj], dst, dst->w - width + dx, dy + TxtPoint[i] - TxtSurfaces[i][jj]->h);
            }

            dx += TxtSurfaces[i][jj]->w;
        }

        dy += TxtPoint[i];
    }

    for (i = 0; i < TXT_CFG_TEXTURES_LINES; i++)
        for (int j = 0; j < TXT_CFG_TEXTURES_PER_LINE; j++)
            if (TxtSurfaces[i][j] != NULL)
                SDL_FreeSurface(TxtSurfaces[i][j]);
}

/************** TTY **************/

static int8_t GetUtf8CharSize(char chr)
{
    if ((chr & 0x80) == 0)
        return 1;
    else if ((chr & 0xE0) == 0xC0)
        return 2;
    else if ((chr & 0xF0) == 0xE0)
        return 3;
    else if ((chr & 0xF8) == 0xF0)
        return 4;
    else if ((chr & 0xFC) == 0xF8)
        return 5;
    else if ((chr & 0xFE) == 0xFC)
        return 6;

    return 1;
}

static uint16_t ReadUtf8Char(char *chr)
{
    uint16_t result = 0;
    if ((chr[0] & 0x80) == 0)
        result = chr[0];
    else if ((chr[0] & 0xE0) == 0xC0)
        result = ((chr[0] & 0x1F) << 6) | (chr[1] & 0x3F);
    else if ((chr[0] & 0xF0) == 0xE0)
        result = ((chr[0] & 0x0F) << 12) | ((chr[1] & 0x3F) << 6) | (chr[2] & 0x3F);
    else
        result = chr[0];

    return result;
}

action_res_t *Text_CreateTTYText()
{
    action_res_t *tmp = NEW(action_res_t);

    tmp->node_type = NODE_TYPE_TTYTEXT;
    tmp->nodes.tty_text = NEW(ttytext_t);
    Text_InitStyle(&tmp->nodes.tty_text->style);

    return tmp;
}

int Text_DeleteTTYText(action_res_t *nod)
{
    if (nod->node_type != NODE_TYPE_TTYTEXT)
        return NODE_RET_NO;

    if (nod->nodes.tty_text->img != NULL)
        SDL_FreeSurface(nod->nodes.tty_text->img);

    if (nod->nodes.tty_text->fnt != NULL)
        TTF_CloseFont(nod->nodes.tty_text->fnt);

    if (nod->slot > 0)
    {
        SetgVarInt(nod->slot, 2);
        SetGNode(nod->slot, NULL);
    }

    DELETE(nod);

    return NODE_RET_DELETE;
}

int Text_ProcessTTYText(action_res_t *nod)
{
    if (nod->node_type != NODE_TYPE_TTYTEXT)
        return NODE_RET_NO;

    ttytext_t *tty = nod->nodes.tty_text;

    tty->nexttime -= Game_GetDTime();

    if (tty->nexttime >= 0)
    {
        return NODE_RET_OK;
    }

    if (tty->txtpos >= tty->txtsize)
    {
        Text_DeleteTTYText(nod);
        return NODE_RET_DELETE;
    }

    if (tty->txtbuf[tty->txtpos] == '<')
    {
        int32_t strt = tty->txtpos, endt = 0, ret = 0;
        while (tty->txtbuf[tty->txtpos] != '>' && tty->txtpos < tty->txtsize)
            tty->txtpos++;
        endt = tty->txtpos;
        if (strt != -1)
            if ((endt - strt - 1) > 0)
                ret = txt_parse_txt_params(&tty->style, tty->txtbuf + strt + 1, endt - strt - 1);

        if (ret & (TXT_RET_FNTCHG | TXT_RET_FNTSTL | TXT_RET_NEWLN))
        {
            if (ret & TXT_RET_FNTCHG)
            {
                TTF_CloseFont(tty->fnt);
                tty->fnt = Loader_LoadFont(tty->style.fontname, tty->style.size);
                Text_SetStyle(tty->fnt, &tty->style);
            }
            if (ret & TXT_RET_FNTSTL)
                Text_SetStyle(tty->fnt, &tty->style);

            if (ret & TXT_RET_NEWLN)
                ttynewline(tty);
        }

        if (ret & TXT_RET_HASSTBOX)
        {
            char buf[MINIBUFSIZE];
            int t_len = sprintf(buf, "%d", GetgVarInt(tty->style.statebox));
            for (int j = 0; j < t_len; j++)
                outchartotty(buf[j], tty);
        }

        tty->txtpos++;
    }
    else
    {
        int8_t charsz = GetUtf8CharSize(tty->txtbuf[tty->txtpos]);
        uint16_t chr = ReadUtf8Char(&tty->txtbuf[tty->txtpos]);

        if (chr == ' ')
        {
            int i = tty->txtpos + charsz;
            int width = getglyphwidth(tty->fnt, chr);

            while (i < tty->txtsize && tty->txtbuf[i] != ' ' && tty->txtbuf[i] != '<')
            {

                int8_t chsz = GetUtf8CharSize(tty->txtbuf[i]);
                uint16_t uchr = ReadUtf8Char(&tty->txtbuf[i]);

                width += getglyphwidth(tty->fnt, uchr);

                i += chsz;
            }
            if (tty->dx + width > tty->w)
                ttynewline(tty);
            else
                outchartotty(chr, tty);
        }
        else
            outchartotty(chr, tty);

        tty->txtpos += charsz;
    }
    tty->nexttime = tty->delay;
    Rend_BlitSurfaceXY(tty->img, Rend_GetLocationScreenImage(), tty->x, tty->y);

    return NODE_RET_OK;
}


subtitles_t *Text_LoadSubtitles(char *filename)
{
    char buf[STRBUFSIZE];
    int subscount = 0;

    mfile_t *f = mfopen(filename);
    if (!f) return NULL;

    subtitles_t *tmp = NEW(subtitles_t);
    tmp->currentsub = -1;

    while (!mfeof(f))
    {
        mfgets(buf, STRBUFSIZE, f);

        char *str1 = (char*)str_ltrim(buf); // without left spaces, paramname
        char *str2 = strchr(str1, ':');     // params

        if (str2)
        {
            *str2 = 0;
            str2++;
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
            tmp->SubRect = Text_CreateSubRect(x, y, x2 - x + 1, y2 - y + 1);
        }
        else if (str_starts_with(str1, "TextFile"))
        {
            tmp->txt = Loader_LoadSTR(str2);

            if (tmp->txt == NULL)
            {
                DELETE(tmp);
                return NULL;
            }

            subscount = 0;
            while (tmp->txt[subscount])
                subscount++;
            tmp->subs = NEW_ARRAY(subtitle_t, subscount);
        }
        else // it must be sub info
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

void Text_ProcessSubtitles(subtitles_t *sub, int subtime)
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

void Text_DeleteSubtitles(subtitles_t *sub)
{
    if (sub->txt)
    {
        for (int i = 0; i < sub->count; i++)
            DELETE(sub->txt[i]);
        DELETE(sub->txt);
    }
    Text_DeleteSubRect(sub->SubRect);
    DELETE(sub->subs);
    DELETE(sub);
}

subrect_t *Text_CreateSubRect(int x, int y, int w, int h)
{
    static int subid = 0;

    subrect_t *tmp = NEW(subrect_t);

    tmp->h = h;
    tmp->w = w;
    tmp->x = x;
    tmp->y = y;
    tmp->todelete = false;
    tmp->id = subid++;
    tmp->timer = -1;
    tmp->img = Rend_CreateSurface(w, h, 0);

    AddToList(&subs_list, tmp);

    return tmp;
}

void Text_DeleteSubRect(subrect_t *rect)
{
    rect->todelete = true;
}

void Text_DrawSubtitles()
{
    SDL_Surface *screen = Rend_GetScreen();

    SDL_Rect msg_rect = {
        0, GAMESCREEN_Y + GAMESCREEN_H,
        WINDOW_W, WINDOW_H - (GAMESCREEN_Y + GAMESCREEN_H)
    };
    Rend_FillRect(screen, &msg_rect, 0, 0, 0);

    for (int i = 0; i < subs_list.length; i++)
    {
        subrect_t *subrec = (subrect_t *)subs_list.items[i];
        if (!subrec) continue;

        if (subrec->timer >= 0)
        {
            subrec->timer -= Game_GetDTime();
            if (subrec->timer < 0)
                subrec->todelete = true;
        }

        if (subrec->todelete)
        {
            DeleteFromList(&subs_list, i);
            SDL_FreeSurface(subrec->img);
            DELETE(subrec);
        }
        else
        {
            SDL_Rect rect = {subrec->x + GAMESCREEN_X, subrec->y + GAMESCREEN_Y - 5, 0, 0};
            Rend_BlitSurface(subrec->img, NULL, screen, &rect);
        }
    }
}
