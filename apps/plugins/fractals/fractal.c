/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: fractal.c 29820 2011-05-04 06:11:59Z Buschel $
 *
 * Copyright (C) 2004 Matthias Wientapper
 * Heavily extended 2005 Jens Arnold
 * Extended 2009 Tomer Shalev
 *
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
#include "plugin.h"

#include "fractal.h"
#include "fractal_rect.h"
#include "fractal_sets.h"
#include "mandelbrot_set.h"
#include "lib/pluginlib_exit.h"

#ifdef USEGSLIB
GREY_INFO_STRUCT
static unsigned char *gbuf;
static size_t gbuf_size = 0;
#endif

#define REDRAW_NONE         0
#define REDRAW_PARTIAL      1
#define REDRAW_FULL         2
#define REDRAW_FULL_OVERLAY 3



/* returns 1 if a button has been pressed, 0 otherwise */
static int button_yield(void *ctx)
{
    long *button = (long *)ctx;

    *button = rb->button_get(false);
    switch (*button)
    {
        case FRACTAL_QUIT:
        case FRACTAL_UP:
        case FRACTAL_DOWN:
        case FRACTAL_LEFT:
        case FRACTAL_RIGHT:
        case FRACTAL_ZOOM_IN:
        case FRACTAL_ZOOM_OUT:
        case FRACTAL_PRECISION_INC:
        case FRACTAL_PRECISION_DEC:
        case FRACTAL_RESET:
#ifdef FRACTAL_ZOOM_IN2
        case FRACTAL_ZOOM_IN2:
#endif
#ifdef FRACTAL_ZOOM_IN_PRE
        case FRACTAL_ZOOM_IN_PRE:
#endif
#if defined(FRACTAL_ZOOM_OUT_PRE) && \
            (FRACTAL_ZOOM_OUT_PRE != FRACTAL_ZOOM_IN_PRE)
        case FRACTAL_ZOOM_OUT_PRE:
#endif
#ifdef FRACTAL_PRECISION_INC_PRE
        case FRACTAL_PRECISION_INC_PRE:
#endif
#if defined(FRACTAL_PRECISION_DEC_PRE) && \
            (FRACTAL_PRECISION_DEC_PRE != FRACTAL_PRECISION_INC_PRE)
        case FRACTAL_PRECISION_DEC_PRE:
#endif
            return 1;
        default:
            *button = BUTTON_NONE;
            return 0;
    }
}

static void cleanup(void)
{
#ifdef USEGSLIB
    grey_release();
#endif
}

enum plugin_status plugin_start(const void* parameter)
{
#if defined(FRACTAL_ZOOM_OUT_PRE) || \
    defined(FRACTAL_ZOOM_IN_PRE) || \
    defined(FRACTAL_PRECISION_DEC_PRE) || \
    defined(FRACTAL_PRECISION_INC_PRE)
    long lastbutton = BUTTON_NONE;
#endif
    int redraw = REDRAW_FULL;
    struct fractal_ops *ops = &mandelbrot_ops;

    (void)parameter;

#ifdef USEGSLIB
    /* get the remainder of the plugin buffer */
    gbuf = (unsigned char *)rb->plugin_get_buffer(&gbuf_size);

    /* initialize the greyscale buffer.*/
    if (!grey_init(gbuf, gbuf_size, GREY_ON_COP, LCD_WIDTH, LCD_HEIGHT, NULL))
    {
        rb->splash(HZ, "Couldn't init greyscale display");
        return PLUGIN_ERROR;
    }
    grey_show(true); /* switch on greyscale overlay */
#endif

    /* release greylib on exit */
    atexit(cleanup);
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif

    ops->init();

    /* main loop */
    while (true)
    {
        long button = BUTTON_NONE;

        if (redraw != REDRAW_NONE)
        {
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
            rb->cpu_boost(true);
#endif
            switch (redraw)
            {
                case REDRAW_FULL:
                    mylcd_ub_clear_display();
                    mylcd_ub_update();
                    /* fall-through */
                case REDRAW_FULL_OVERLAY:
                    rects_queue_init();
                    break;
                default:
                    break;
            }

            /* paint all rects */
            rects_calc_all(ops->calc, button_yield, (void *)&button);
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
            rb->cpu_boost(false);
#endif
            /* not interrupted by button press - screen is fully drawn */
            redraw = (button == BUTTON_NONE) ? REDRAW_NONE : REDRAW_PARTIAL;
        }

        if (button == BUTTON_NONE)
            button = rb->button_get(true);

        switch (button)
        {
#ifdef FRACTAL_RC_QUIT
        case FRACTAL_RC_QUIT:
#endif
        case FRACTAL_QUIT:
            return PLUGIN_OK;

        case FRACTAL_ZOOM_OUT:
#ifdef FRACTAL_ZOOM_OUT_PRE
            if (lastbutton != FRACTAL_ZOOM_OUT_PRE)
                break;
#endif
            if (!ops->zoom(-1))
                redraw = REDRAW_FULL;
            break;


        case FRACTAL_ZOOM_IN:
#ifdef FRACTAL_ZOOM_IN_PRE
            if (lastbutton != FRACTAL_ZOOM_IN_PRE)
                break;
#endif
#ifdef FRACTAL_ZOOM_IN2
        case FRACTAL_ZOOM_IN2:
#endif
            if (!ops->zoom(1))
                redraw = REDRAW_FULL;
            break;

        case FRACTAL_UP:
            ops->move(0, +1);
            mylcd_ub_scroll_down(LCD_SHIFT_Y);
            mylcd_ub_update();
            if (redraw != REDRAW_FULL)
                redraw = rects_move_down() ? REDRAW_FULL : REDRAW_PARTIAL;
            break;

        case FRACTAL_DOWN:
            ops->move(0, -1);
            mylcd_ub_scroll_up(LCD_SHIFT_Y);
            mylcd_ub_update();
            if (redraw != REDRAW_FULL)
                redraw = rects_move_up() ? REDRAW_FULL : REDRAW_PARTIAL;
            break;

        case FRACTAL_LEFT:
            ops->move(-1, 0);
            mylcd_ub_scroll_right(LCD_SHIFT_X);
            mylcd_ub_update();
            if (redraw != REDRAW_FULL)
                redraw = rects_move_right() ? REDRAW_FULL : REDRAW_PARTIAL;
            break;

        case FRACTAL_RIGHT:
            ops->move(+1, 0);
            mylcd_ub_scroll_left(LCD_SHIFT_X);
            mylcd_ub_update();
            if (redraw != REDRAW_FULL)
                redraw = rects_move_left() ? REDRAW_FULL : REDRAW_PARTIAL;
            break;

        case FRACTAL_PRECISION_DEC:
#ifdef FRACTAL_PRECISION_DEC_PRE
            if (lastbutton != FRACTAL_PRECISION_DEC_PRE)
                break;
#endif
            if (ops->precision(-1))
                redraw = REDRAW_FULL_OVERLAY;

            break;

        case FRACTAL_PRECISION_INC:
#ifdef FRACTAL_PRECISION_INC_PRE
            if (lastbutton != FRACTAL_PRECISION_INC_PRE)
                break;
#endif
            if (ops->precision(+1))
                redraw = REDRAW_FULL_OVERLAY;

            break;

        case FRACTAL_RESET:
            ops->init();
            redraw = REDRAW_FULL;
            break;

        default:
            exit_on_usb(button);
            break;
        }
#if defined(FRACTAL_ZOOM_OUT_PRE) || \
    defined(FRACTAL_ZOOM_IN_PRE) || \
    defined(FRACTAL_PRECISION_DEC_PRE) || \
    defined(FRACTAL_PRECISION_INC_PRE)
        if (button != BUTTON_NONE)
            lastbutton = button;
#endif
    }
    return PLUGIN_OK;
}
