/***************************************************************************
 *             __________               __   ___.                  
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \  
 *                     \/            \/     \/    \/            \/ 
 * $Id: checkwps.c 31040 2011-11-21 10:13:36Z jdgordon $
 *
 * Copyright (C) 2008 by Dave Chapman
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "checkwps.h"
#include "resize.h"
#include "wps.h"
#include "skin_buffer.h"
#include "skin_debug.h"
#include "skin_engine.h"
#include "wps_internals.h"
#include "settings.h"
#include "viewport.h"
#include "file.h"
#include "font.h"

bool debug_wps = true;
int wps_verbose_level = 0;
char *skin_buffer;

int errno;

const struct settings_list *settings;
const int nb_settings = 0;

/* static endianness conversion */
#define SWAP_16(x) ((typeof(x))(unsigned short)(((unsigned short)(x) >> 8) | \
                                                ((unsigned short)(x) << 8)))

#define SWAP_32(x) ((typeof(x))(unsigned long)( ((unsigned long)(x) >> 24) | \
                                               (((unsigned long)(x) & 0xff0000ul) >> 8) | \
                                               (((unsigned long)(x) & 0xff00ul) << 8) | \
                                                ((unsigned long)(x) << 24)))

#ifndef letoh16
unsigned short letoh16(unsigned short x)
{
    unsigned short n = 0x1234;
    unsigned char* ch = (unsigned char*)&n;

    if (*ch == 0x34)
    {
        /* Little-endian */
        return x;
    } else {
        return SWAP_16(x);
    }
}
#endif

#ifndef letoh32
unsigned short letoh32(unsigned short x)
{
    unsigned short n = 0x1234;
    unsigned char* ch = (unsigned char*)&n;

    if (*ch == 0x34)
    {
        /* Little-endian */
        return x;
    } else {
        return SWAP_32(x);
    }
}
#endif

#ifndef htole32
unsigned int htole32(unsigned int x)
{
    unsigned short n = 0x1234;
    unsigned char* ch = (unsigned char*)&n;

    if (*ch == 0x34)
    {
        /* Little-endian */
        return x;
    } else {
        return SWAP_32(x);
    }
}
#endif

int recalc_dimension(struct dim *dst, struct dim *src)
{
    return 0;
}

#ifdef HAVE_ALBUMART
int playback_claim_aa_slot(struct dim *dim)
{
    return 0;
}

void playback_release_aa_slot(int slot)
{
    return;
}
#endif

int resize_on_load(struct bitmap *bm, bool dither,
                   struct dim *src, struct rowset *tmp_row,
                   unsigned char *buf, unsigned int len,
                   const struct custom_format *cformat,
                   IF_PIX_FMT(int format_index,)
                   struct img_part* (*store_part)(void *args),
                   void *args)
{
    return 0;
}

static char pluginbuf[PLUGIN_BUFFER_SIZE];

static unsigned dummy_func2(void)
{
    return 0;
}

void* plugin_get_buffer(size_t *buffer_size)
{
    *buffer_size = PLUGIN_BUFFER_SIZE;
    return pluginbuf;
}

struct user_settings global_settings = {
    .statusbar = STATUSBAR_TOP,
#ifdef HAVE_LCD_COLOR
    .bg_color = LCD_DEFAULT_BG,
    .fg_color = LCD_DEFAULT_FG,
#endif
};

struct system_status global_status;

int getwidth(void) { return LCD_WIDTH; }
int getheight(void) { return LCD_HEIGHT; }
int getuifont(void) { return 0; }
#ifdef HAVE_REMOTE_LCD
int remote_getwidth(void) { return LCD_REMOTE_WIDTH; }
int remote_getheight(void) { return LCD_REMOTE_HEIGHT; }
#endif

static inline bool backdrop_load(const char *filename, char* backdrop_buffer) 	 
{ 	 
 (void)filename; (void)backdrop_buffer; return true; 	 
}

struct screen screens[NB_SCREENS] =
{
    {
        .screen_type=SCREEN_MAIN,
        .lcdwidth=LCD_WIDTH,
        .lcdheight=LCD_HEIGHT,
        .depth=LCD_DEPTH,
#ifdef HAVE_LCD_COLOR
        .is_color=true,
#else
        .is_color=false,
#endif
        .getwidth = getwidth,
        .getheight = getheight,
#ifdef HAVE_LCD_BITMAP
        .getuifont = getuifont,
#endif
#if LCD_DEPTH > 1
        .get_foreground=dummy_func2,
        .get_background=dummy_func2,
        .backdrop_load=backdrop_load,
#endif
    },
#ifdef HAVE_REMOTE_LCD
    {
        .screen_type=SCREEN_REMOTE,
        .lcdwidth=LCD_REMOTE_WIDTH,
        .lcdheight=LCD_REMOTE_HEIGHT,
        .depth=LCD_REMOTE_DEPTH,
        .getuifont = getuifont,
        .is_color=false,/* No color remotes yet */
        .getwidth=remote_getwidth,
        .getheight=remote_getheight,
#if LCD_REMOTE_DEPTH > 1
        .get_foreground=dummy_func2,
        .get_background=dummy_func2,
        .backdrop_load=backdrop_load,
#endif
    }
#endif
};

#ifdef HAVE_LCD_BITMAP
void screen_clear_area(struct screen * display, int xstart, int ystart,
                       int width, int height)
{
    display->set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    display->fillrect(xstart, ystart, width, height);
    display->set_drawmode(DRMODE_SOLID);
}
#endif

#if CONFIG_TUNER
bool radio_hardware_present(void)
{
    return true;
}
#endif

#ifdef HAVE_LCD_BITMAP
static int loaded_fonts = 0;
static struct font _font;
int font_load(const char *path)
{
    int id = 2 + loaded_fonts;
    loaded_fonts++;
    return id;
}
    
void font_unload(int font_id)
{
    (void)font_id;
}

struct font* font_get(int font)
{
    return &_font;
}
#endif

/* This is no longer defined in ROCKBOX builds so just use a huge value */
#define SKIN_BUFFER_SIZE (200*1024)

int main(int argc, char **argv)
{
    int res;
    int filearg = 1;

    struct wps_data wps={0};
    enum screen_type screen = SCREEN_MAIN;
    struct screen* wps_screen;

    /* No arguments -> print the help text
     * Also print the help text upon -h or --help */
    if( (argc < 2) ||
        strcmp(argv[1],"-h") == 0 ||
        strcmp(argv[1],"--help") == 0 )
    {
        printf("Usage: checkwps [OPTIONS] filename.wps [filename2.wps]...\n");
        printf("\nOPTIONS:\n");
        printf("\t-v\t\tverbose\n");
        printf("\t-vv\t\tmore verbose\n");
        printf("\t-vvv\t\tvery verbose\n");
        printf("\t-h,\t--help\tshow this message\n");
        return 1;
    }

    if (argv[1][0] == '-') {
        filearg++;
        int i = 1;
        while (argv[1][i] && argv[1][i] == 'v') {
            i++;
            wps_verbose_level++;
        }
    }
    skin_buffer = malloc(SKIN_BUFFER_SIZE);
    if (!skin_buffer)
    {
        printf("mallloc fail!\n");
        return 1;
    }

    skin_buffer_init(skin_buffer, SKIN_BUFFER_SIZE);

    /* Go through every skin that was thrown at us, error out at the first
     * flawed wps */
    while (argv[filearg]) {
        printf("Checking %s...\n", argv[filearg]);
#ifdef HAVE_REMOTE_LCD
        if((strcmp(&argv[filearg][strlen(argv[filearg])-4], "rwps") == 0) || 
           (strcmp(&argv[filearg][strlen(argv[filearg])-4], "rsbs") == 0) ||
           (strcmp(&argv[filearg][strlen(argv[filearg])-4], "rfms") == 0))
            screen = SCREEN_REMOTE;
        else
            screen = SCREEN_MAIN;
#endif    
        wps_screen = &screens[screen];

        res = skin_data_load(screen, &wps, argv[filearg], true);

        if (!res) {
            printf("WPS parsing failure\n");
            skin_error_format_message();
            return 3;
        }

        printf("WPS parsed OK\n\n");
        if (wps_verbose_level>2)
            skin_debug_tree(SKINOFFSETTOPTR(skin_buffer, wps.tree));
        filearg++;
    }
    return 0;
}
