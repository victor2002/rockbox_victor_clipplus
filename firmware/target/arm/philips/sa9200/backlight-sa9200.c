/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: backlight-sa9200.c 21583 2009-07-01 02:18:49Z lowlight $
 *
 * Copyright (C) 2006 by Barry Wardell
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
#include "config.h"
#include "backlight-target.h"
#include "system.h"
#include "backlight.h"
#include "ascodec.h"
#include "as3514.h"
#include "synaptics-mep.h"
#include "lcd.h"

void _backlight_set_brightness(int brightness)
{
    ascodec_write(AS3514_DCDC15, brightness);
}

void _backlight_on(void)
{
#ifdef HAVE_LCD_ENABLE
    lcd_enable(true); /* power on lcd + visible display */
#endif
#if (CONFIG_BACKLIGHT_FADING != BACKLIGHT_FADING_SW_SETTING) /* in bootloader/sim */
    /* if we set the brightness to the settings value, then fading up
     * is glitchy */
    _backlight_set_brightness(backlight_brightness);
#endif
}

void _backlight_off(void)
{
    _backlight_set_brightness(0);
#ifdef HAVE_LCD_ENABLE
    lcd_enable(false); /* power off visible display */
#endif
}

#ifdef HAVE_BUTTON_LIGHT

#define BUTTONLIGHT_MASK 0x7f
#define BUTTONLIGHT_MAX  0x0f
static unsigned short buttonlight_status = 0;

void _buttonlight_on(void)
{
    if (!buttonlight_status)
    {
        touchpad_set_buttonlights(BUTTONLIGHT_MASK, BUTTONLIGHT_MAX);
        GPIOD_OUTPUT_VAL &= ~(0x40 | 0x20 | 0x04); /* REW/FFWD/MENU */
        buttonlight_status = 1;
    }
}

void _buttonlight_off(void)
{
    if (buttonlight_status)
    {
        touchpad_set_buttonlights(BUTTONLIGHT_MASK, 0);
        GPIOD_OUTPUT_VAL |= (0x40 | 0x20 | 0x04); /* REW/FFWD/MENU */
        buttonlight_status = 0;
    }
}
#endif
