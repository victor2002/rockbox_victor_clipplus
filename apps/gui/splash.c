/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: splash.c 30756 2011-10-15 19:35:02Z zagor $
 *
 * Copyright (C) Daniel Stenberg (2002)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "stdarg.h"
#include "string.h"
#include "rbunicode.h"
#include "stdio.h"
#include "kernel.h"
#include "screen_access.h"
#include "lang.h"
#include "settings.h"
#include "talk.h"
#include "splash.h"
#include "viewport.h"
#include "strtok_r.h"

#ifdef HAVE_LCD_BITMAP

#define MAXLINES  (LCD_HEIGHT/6)
#define MAXBUFFER 512
#define RECT_SPACING 2
#define SPLASH_MEMORY_INTERVAL (HZ)

#else /* HAVE_LCD_CHARCELLS */

#define MAXLINES  2
#define MAXBUFFER 64
#define RECT_SPACING 0

#endif


static void splash_internal(struct screen * screen, const char *fmt, va_list ap)
{
    char splash_buf[MAXBUFFER];
    char *lines[MAXLINES];

    char *next;
    char *lastbreak = NULL;
    char *store = NULL;
    int line = 0;
    int x = 0;
    int y, i;
    int space_w, w, h;
    struct viewport vp;
#ifdef HAVE_LCD_BITMAP
    int width, height;
    int maxw = 0;

    viewport_set_defaults(&vp, screen->screen_type);
    screen->set_viewport(&vp);
    
    screen->getstringsize(" ", &space_w, &h);
#else /* HAVE_LCD_CHARCELLS */
    vp.width = screen->lcdwidth;
    vp.height = screen->lcdheight;

    space_w = h = 1;
    screen->double_height (false);
#endif
    y = h;

    vsnprintf(splash_buf, sizeof(splash_buf), fmt, ap);
    va_end(ap);

    /* break splash string into display lines, doing proper word wrap */

    next = strtok_r(splash_buf, " ", &store);
    if (!next)
        goto end; /* nothing to display */

    lines[0] = next;
    while (true)
    {
#ifdef HAVE_LCD_BITMAP
        screen->getstringsize(next, &w, NULL);
#else
        w = utf8length(next);
#endif
        if (lastbreak)
        {
            if (x + (next - lastbreak) * space_w + w
                    > vp.width - RECT_SPACING*2)
            {   /* too wide, wrap */
#ifdef HAVE_LCD_BITMAP
                if (x > maxw)
                    maxw = x;
#endif
                if ((y + h > vp.height) || (line >= (MAXLINES-1)))
                    break;  /* screen full or out of lines */
                x = 0;
                y += h;
                lines[++line] = next;
            }
            else
            {
                /*  restore & calculate spacing */
                *lastbreak = ' ';
                x += (next - lastbreak) * space_w;
            }
        }
        x += w;
        lastbreak = next + strlen(next);
        next = strtok_r(NULL, " ", &store);
        if (!next)
        {   /* no more words */
#ifdef HAVE_LCD_BITMAP
            if (x > maxw)
                maxw = x;
#endif
            break;
        }
    }

    /* prepare viewport
     * First boundaries, then the grey filling, then the black border and finally
     * the text*/

    screen->stop_scroll();

#ifdef HAVE_LCD_BITMAP

    width = maxw + 2*RECT_SPACING;
    height = y + 2*RECT_SPACING;

    if (width > vp.width)
        width = vp.width;
    if (height > vp.height)
        height = vp.height;

    vp.x += (vp.width - width) / 2;
    vp.y += (vp.height - height) / 2;
    vp.width = width;
    vp.height = height;
    
    vp.flags |=  VP_FLAG_ALIGN_CENTER;
#if LCD_DEPTH > 1
    if (screen->depth > 1)
    {
        vp.drawmode = DRMODE_FG;
        /* can't do vp.fg_pattern here, since set_foreground does a bit more on
         * greyscale */
        screen->set_foreground(SCREEN_COLOR_TO_NATIVE(screen, LCD_LIGHTGRAY));
    }
    else
#endif
        vp.drawmode = (DRMODE_SOLID|DRMODE_INVERSEVID);

    screen->fill_viewport();

#if LCD_DEPTH > 1
    if (screen->depth > 1)
        /* can't do vp.fg_pattern here, since set_foreground does a bit more on
         * greyscale */
        screen->set_foreground(SCREEN_COLOR_TO_NATIVE(screen, LCD_BLACK));
    else
#endif
        vp.drawmode = DRMODE_SOLID;

    screen->draw_border_viewport();

    /* prepare putting the text */
    y = RECT_SPACING;
#else /* HAVE_LCD_CHARCELLS */
    y = 0;    /* vertical centering on 2 lines would be silly */
    screen->clear_display();
#endif

    /* print the message to screen */
    for (i = 0; i <= line; i++, y+=h)
    {
#ifdef HAVE_LCD_BITMAP
        screen->putsxy(0, y, lines[i]);
#else
        screen->puts(0, y, lines[i]);
#endif
    }
    screen->update_viewport();
end:
    screen->set_viewport(NULL);
}

void splashf(int ticks, const char *fmt, ...)
{
    va_list ap;

    /* If fmt is a lang ID then get the corresponding string (which
       still might contain % place holders). */
    fmt = P2STR((unsigned char *)fmt);
    FOR_NB_SCREENS(i)
    {
        va_start(ap, fmt);
        splash_internal(&(screens[i]), fmt, ap);
        va_end(ap);
    }
    if (ticks)
        sleep(ticks);
}

void splash(int ticks, const char *str)
{
#if !defined(SIMULATOR) || CONFIG_CODEC == SWCODEC
    long id;
    /* fmt may be a so called virtual pointer. See settings.h. */
    if((id = P2ID((const unsigned char*)str)) >= 0)
        /* If fmt specifies a voicefont ID, and voice menus are
           enabled, then speak it. */
        cond_talk_ids_fq(id);
#endif
    splashf(ticks, "%s", P2STR((const unsigned char*)str));
}
