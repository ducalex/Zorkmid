#include "System.h"

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

static SDL_Surface *RenderUTF8(TTF_Font *fnt, char *text, txt_style_t *style)
{
    Text_SetStyle(fnt, style);
    SDL_Color clr = {style->red, style->green, style->blue, 255};
    return TTF_RenderUTF8_Solid(fnt, text, clr);
}

static int8_t txt_parse_txt_params(txt_style_t *style, const char *strin, int32_t len)
{
    char buf[TXT_CFG_BUF_MAX_LEN];
    memcpy(buf, strin, len);
    buf[len] = 0x0;

    int8_t retval = TXT_RET_NOTHING;

    char *token;

    const char *find = " ";

    //font with "item what i want"
    const char *fontitem = str_find(buf, "font");
    if (fontitem != NULL)
    {
        fontitem += 5; //to next item
        if (fontitem[0] == '"')
        {
            fontitem++;

            int32_t len = 0;
            while (fontitem[len] != '"' && fontitem[len] >= ' ')
            {
                style->fontname[len] = fontitem[len];
                len++;
            }
            style->fontname[len] = 0;
        }
        else
        {
            int32_t len = 0;
            while (fontitem[len] > ' ')
            {
                style->fontname[len] = fontitem[len];
                len++;
            }
            style->fontname[len] = 0;
        }
        retval |= TXT_RET_FNTCHG;
    }

    token = strtok(buf, find);

    bool gooood;

    while (token != NULL)
    {
        gooood = true;

        //        if ( strCMP(token,"font") == 0 )
        //        {
        //            token = strtok(NULL,find);
        //            if (token == NULL)
        //                gooood = false;
        //            else
        //            {
        //                if (strCMP(style->fontname,token) != 0)
        //                {
        //                    strcpy(style->fontname,token);
        //                    retval |= TXT_RET_FNTCHG;
        //                }
        //
        //            }
        //
        //        }
        //        else
        if (str_starts_with(token, "blue"))
        {
            token = strtok(NULL, find);
            if (token == NULL)
                gooood = false;
            else
            {
                int32_t tmp = atoi(token);
                if (style->blue != tmp)
                {
                    style->blue = tmp;
                    retval |= TXT_RET_FNTSTL;
                }
            }
        }
        else if (str_starts_with(token, "red"))
        {
            token = strtok(NULL, find);
            if (token == NULL)
                gooood = false;
            else
            {
                int32_t tmp = atoi(token);
                if (style->red != tmp)
                {
                    style->red = tmp;
                    retval |= TXT_RET_FNTSTL;
                }
            }
        }
        else if (str_starts_with(token, "green"))
        {
            token = strtok(NULL, find);
            if (token == NULL)
                gooood = false;
            else
            {
                int32_t tmp = atoi(token);
                if (style->green != tmp)
                {
                    style->green = tmp;
                    retval |= TXT_RET_FNTSTL;
                }
            }
        }
        else if (str_starts_with(token, "newline"))
        {
            if ((retval & TXT_RET_NEWLN) == 0)
                style->newline = 0;

            style->newline++;
            retval |= TXT_RET_NEWLN;
        }
        else if (str_starts_with(token, "point"))
        {
            token = strtok(NULL, find);
            if (token == NULL)
                gooood = false;
            else
            {
                int32_t tmp = atoi(token);
                if (style->size != tmp)
                {
                    style->size = tmp;
                    retval |= TXT_RET_FNTCHG;
                }
            }
        }
        else if (str_starts_with(token, "escapement"))
        {
            token = strtok(NULL, find);
            if (token == NULL)
                gooood = false;
            else
                style->escapement = atoi(token);
        }
        else if (str_starts_with(token, "italic"))
        {
            token = strtok(NULL, find);
            if (token == NULL)
                gooood = false;
            else if (str_starts_with(token, "on"))
            {
                if (style->italic != TXT_STYLE_VAR_TRUE)
                {
                    style->italic = TXT_STYLE_VAR_TRUE;
                    retval |= TXT_RET_FNTSTL;
                }
            }
            else if (str_starts_with(token, "off"))
            {
                if (style->italic != TXT_STYLE_VAR_FALSE)
                {
                    style->italic = TXT_STYLE_VAR_FALSE;
                    retval |= TXT_RET_FNTSTL;
                }
            }
            else
                gooood = false;
        }
        else if (str_starts_with(token, "underline"))
        {
            token = strtok(NULL, find);
            if (token == NULL)
                gooood = false;
            else if (str_starts_with(token, "on"))
            {
                if (style->underline != TXT_STYLE_VAR_TRUE)
                {
                    style->underline = TXT_STYLE_VAR_TRUE;
                    retval |= TXT_RET_FNTSTL;
                }
            }
            else if (str_starts_with(token, "off"))
            {
                if (style->underline != TXT_STYLE_VAR_FALSE)
                {
                    style->underline = TXT_STYLE_VAR_FALSE;
                    retval |= TXT_RET_FNTSTL;
                }
            }
            else
                gooood = false;
        }
        else if (str_starts_with(token, "strikeout"))
        {
            token = strtok(NULL, find);
            if (token == NULL)
                gooood = false;
            else if (str_starts_with(token, "on"))
            {
                if (style->strikeout != TXT_STYLE_VAR_TRUE)
                {
                    style->strikeout = TXT_STYLE_VAR_TRUE;
                    retval |= TXT_RET_FNTSTL;
                }
            }
            else if (str_starts_with(token, "off"))
            {
                if (style->strikeout != TXT_STYLE_VAR_FALSE)
                {
                    style->strikeout = TXT_STYLE_VAR_FALSE;
                    retval |= TXT_RET_FNTSTL;
                }
            }
            else
                gooood = false;
        }
        else if (str_starts_with(token, "bold"))
        {
            token = strtok(NULL, find);
            if (token == NULL)
                gooood = false;
            else if (str_starts_with(token, "on"))
            {
                if (style->bold != TXT_STYLE_VAR_TRUE)
                {
                    style->bold = TXT_STYLE_VAR_TRUE;
                    retval |= TXT_RET_FNTSTL;
                }
            }
            else if (str_starts_with(token, "off"))
            {
                if (style->bold != TXT_STYLE_VAR_FALSE)
                {
                    style->bold = TXT_STYLE_VAR_FALSE;
                    retval |= TXT_RET_FNTSTL;
                }
            }
            else
                gooood = false;
        }
        else if (str_starts_with(token, "skipcolor"))
        {
            token = strtok(NULL, find);
            if (token == NULL)
                gooood = false;
            else if (str_starts_with(token, "on"))
                style->skipcolor = TXT_STYLE_VAR_TRUE;
            else if (str_starts_with(token, "off"))
                style->skipcolor = TXT_STYLE_VAR_FALSE;
            else
                gooood = false;
        }
        else if (str_starts_with(token, "image"))
        {
            //token = strtok(NULL,find);
        }
        else if (str_starts_with(token, "statebox"))
        {
            token = strtok(NULL, find);
            if (token == NULL)
                gooood = false;
            else
            {
                style->statebox = atoi(token);
                retval |= TXT_RET_HASSTBOX;
            }
        }
        else if (str_starts_with(token, "justify"))
        {
            token = strtok(NULL, find);
            if (token == NULL)
                gooood = false;
            else if (str_starts_with(token, "center"))
                style->justify = TXT_JUSTIFY_CENTER;
            else if (str_starts_with(token, "left"))
                style->justify = TXT_JUSTIFY_LEFT;
            else if (str_starts_with(token, "right"))
                style->justify = TXT_JUSTIFY_RIGHT;
            else
                gooood = false;
        }

        if (gooood)
            token = strtok(NULL, find);
    }
    return retval;
}

static void Text_DrawWithJustify(char *txt, TTF_Font *fnt, SDL_Color clr, SDL_Surface *dst, int lineY, int justify)
{
    SDL_Surface *aaa = TTF_RenderUTF8_Solid(fnt, txt, clr);

    if (justify == TXT_JUSTIFY_LEFT)

        DrawImageToSurf(aaa, 0, lineY - aaa->h, dst);

    else if (justify == TXT_JUSTIFY_CENTER)

        DrawImageToSurf(aaa, (dst->w - aaa->w) / 2, lineY - aaa->h, dst);

    else if (justify == TXT_JUSTIFY_RIGHT)

        DrawImageToSurf(aaa, dst->w - aaa->w, lineY - aaa->h, dst);

    SDL_FreeSurface(aaa);
}

static void ttynewline(ttytext_t *tty)
{
    tty->dy += tty->style.size;
    tty->dx = 0;
}

static void ttyscroll(ttytext_t *tty)
{
    int32_t scroll = 0;
    while (tty->dy - scroll > tty->h - tty->style.size)
        scroll += tty->style.size;
    SDL_Surface *tmp = CreateSurface(tty->w, tty->h);
    DrawImageToSurf(tty->img, 0, -scroll, tmp);
    SDL_FreeSurface(tty->img);
    tty->img = tmp;
    tty->dy -= scroll;
}

static void outchartotty(uint16_t chr, ttytext_t *tty)
{
    SDL_Color clr = {tty->style.red, tty->style.green, tty->style.blue, 255};

    SDL_Surface *tmp_surf = TTF_RenderGlyph_Solid(tty->fnt, chr, clr);

    int32_t minx, maxx, miny, maxy, advice;
    TTF_GlyphMetrics(tty->fnt, chr, &minx, &maxx, &miny, &maxy, &advice);

    if (tty->dx + advice > tty->w)
        ttynewline(tty);

    if (tty->dy + tty->style.size + tty->style.size / 4 > tty->h)
        ttyscroll(tty);

    DrawImageToSurf(tmp_surf, tty->dx, tty->dy + tty->style.size - maxy, tty->img);

    tty->dx += advice;

    SDL_FreeSurface(tmp_surf);
}

static int32_t getglyphwidth(TTF_Font *fnt, uint16_t chr)
{
    int32_t minx, maxx, miny, maxy, advice;
    TTF_GlyphMetrics(fnt, chr, &minx, &maxx, &miny, &maxy, &advice);
    return advice;
}

void Text_InitStyle(txt_style_t *style)
{
    if (style == NULL)
        Z_PANIC("style is NULL")

    style->blue = 255;
    style->green = 255;
    style->red = 255;
    strcpy(style->fontname, "Arial");
    style->bold = TXT_STYLE_VAR_FALSE;
    style->escapement = 0;
    style->italic = TXT_STYLE_VAR_FALSE;
    style->justify = TXT_JUSTIFY_LEFT;
    style->newline = 0;
    style->size = 12;
    style->skipcolor = TXT_STYLE_VAR_FALSE;
    style->strikeout = TXT_STYLE_VAR_FALSE;
    style->underline = TXT_STYLE_VAR_FALSE;
    style->statebox = 0;
}

void Text_GetStyle(txt_style_t *style, const char *strin)
{
    int32_t strt = -1;
    int32_t endt = -1;

    int32_t t_len = strlen(strin);

    for (int32_t i = 0; i < t_len; i++)
    {
        if (strin[i] == '<')
            strt = i;
        else if (strin[i] == '>')
        {
            endt = i;
            if (strt != -1)
                if ((endt - strt - 1) > 0)
                    txt_parse_txt_params(style, strin + strt + 1, endt - strt - 1);
        }
    }
}

void Text_SetStyle(TTF_Font *font, txt_style_t *fnt_stl)
{
    int32_t temp_stl = 0;

    if (fnt_stl->bold == TXT_STYLE_VAR_TRUE)
        temp_stl |= TTF_STYLE_BOLD;

    if (fnt_stl->italic == TXT_STYLE_VAR_TRUE)
        temp_stl |= TTF_STYLE_ITALIC;

    if (fnt_stl->underline == TXT_STYLE_VAR_TRUE)
        temp_stl |= TTF_STYLE_UNDERLINE;

#ifdef TTF_STYLE_STRIKETHROUGH
    if (fnt_stl->strikeout == TXT_STYLE_VAR_TRUE)
        temp_stl |= TTF_STYLE_STRIKETHROUGH;
#endif

    TTF_SetFontStyle(font, temp_stl);
}

int32_t Text_Draw(char *txt, txt_style_t *fnt_stl, SDL_Surface *dst)
{
    TTF_Font *temp_font = Loader_LoadFont(fnt_stl->fontname, fnt_stl->size);
    if (!temp_font)
    {
        Z_PANIC("TTF_OpenFont: %s\n", TTF_GetError());
    }

    SDL_FillRect(dst, NULL, 0);

    SDL_Color clr = {fnt_stl->red, fnt_stl->green, fnt_stl->blue, 255};

    Text_SetStyle(temp_font, fnt_stl);

    int32_t w, h;

    TTF_SizeUTF8(temp_font, txt, &w, &h);

    Text_DrawWithJustify(txt, temp_font, clr, dst, fnt_stl->size, fnt_stl->justify);

    TTF_CloseFont(temp_font);

    return w;
}

void Text_DrawInOneLine(const char *text, SDL_Surface *dst)
{
    txt_style_t style, style2;
    Text_InitStyle(&style);
    int32_t strt = -1;
    int32_t endt = -1;
    int32_t i = 0;
    int32_t dx = 0, dy = 0;
    int32_t txt_w, txt_h;
    int32_t txtpos = 0;
    char buf[TXT_CFG_BUF_MAX_LEN];
    char buf2[TXT_CFG_BUF_MAX_LEN];
    memset(buf, 0, TXT_CFG_BUF_MAX_LEN);
    memset(buf2, 0, TXT_CFG_BUF_MAX_LEN);

    SDL_Surface *TxtSurfaces[TXT_CFG_TEXTURES_LINES][TXT_CFG_TEXTURES_PER_LINE];
    int32_t currentline = 0, currentlineitm = 0;

    int8_t TxtJustify[TXT_CFG_TEXTURES_LINES];
    int8_t TxtPoint[TXT_CFG_TEXTURES_LINES];

    memset(TxtSurfaces, 0, sizeof(TxtSurfaces));

    int32_t stringlen = strlen(text);

    TTF_Font *font = NULL;

    font = Loader_LoadFont(style.fontname, style.size);
    Text_SetStyle(font, &style);

    int32_t prevbufspace = 0, prevtxtspace = 0;

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
                if (ret & TXT_RET_FNTSTL)
                    Text_SetStyle(font, &style);

                if (ret & TXT_RET_NEWLN)
                {
                    currentline++;
                    currentlineitm = 0;
                    dx = 0;
                }
            }

            if (ret & TXT_RET_HASSTBOX)
            {
                char buf3[MINIBUFSZ];
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
        int32_t j = 0;
        int32_t width = 0;
        while (TxtSurfaces[i][j] != NULL)
        {
            width += TxtSurfaces[i][j]->w;
            j++;
        }
        dx = 0;
        for (int32_t jj = 0; jj < j; jj++)
        {
            if (TxtJustify[i] == TXT_JUSTIFY_LEFT)
                DrawImageToSurf(TxtSurfaces[i][jj], dx, dy + TxtPoint[i] - TxtSurfaces[i][jj]->h, dst);

            else if (TxtJustify[i] == TXT_JUSTIFY_CENTER)
                DrawImageToSurf(TxtSurfaces[i][jj], ((dst->w - width) >> 1) + dx, dy + TxtPoint[i] - TxtSurfaces[i][jj]->h, dst);

            else if (TxtJustify[i] == TXT_JUSTIFY_RIGHT)
                DrawImageToSurf(TxtSurfaces[i][jj], dst->w - width + dx, dy + TxtPoint[i] - TxtSurfaces[i][jj]->h, dst);

            dx += TxtSurfaces[i][jj]->w;
        }

        dy += TxtPoint[i];
    }

    for (i = 0; i < TXT_CFG_TEXTURES_LINES; i++)
        for (int32_t j = 0; j < TXT_CFG_TEXTURES_PER_LINE; j++)
            if (TxtSurfaces[i][j] != NULL)
                SDL_FreeSurface(TxtSurfaces[i][j]);
}

action_res_t *Text_CreateTTYText()
{
    action_res_t *tmp = ScrSys_CreateActRes(NODE_TYPE_TTYTEXT);
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
        setGNode(nod->slot, NULL);
    }

    free(nod);

    return NODE_RET_DELETE;
}

int Text_ProcessTTYText(action_res_t *nod)
{
    if (nod->node_type != NODE_TYPE_TTYTEXT)
        return NODE_RET_NO;

    ttytext_t *tty = nod->nodes.tty_text;
    char buf[MINIBUFSZ];

    tty->nexttime -= GetDTime();

    if (tty->nexttime < 0)
    {
        if (tty->txtpos < tty->txtsize)
        {
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
                    int32_t i = tty->txtpos + charsz;
                    int32_t width = getglyphwidth(tty->fnt, chr);

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
            Rend_DrawImageToGamescr(tty->img, tty->x, tty->y);
        }
        else
        {
            Text_DeleteTTYText(nod);
            return NODE_RET_DELETE;
        }
    }

    return NODE_RET_OK;
}
