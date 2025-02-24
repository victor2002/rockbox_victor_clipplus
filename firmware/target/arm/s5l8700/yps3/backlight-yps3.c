/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: backlight-yps3.c 22327 2009-08-15 15:59:39Z bertrik $
 *
 * Copyright (C) 2009 by Bertrik Sikken
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
#include <stdbool.h>

#include "config.h"
#include "backlight.h"
#include "backlight-target.h"
#include "system.h"

/*
    Backlight driver using the PWM mode of a hardware timer.
    
    The PWM duty cycle depends exponentially on the configured brightness
    level. This makes the brightness curve more linear to the human eye.
    
    The button LEDs are all activated at the same time (even though there
    are three individually controllable groups: menu/back, cursor, middle).
 */

void _backlight_set_brightness(int brightness)
{
    /* pwm = round(16 * 16**(x/16)), where brightness level x = 1..16 */
    static const unsigned int logtable[] =
        {19, 23, 27, 32, 38, 45, 54, 64, 76, 91, 108, 128, 152, 181, 215, 256};

    /* set PWM width */
    TADATA0 = logtable[brightness];
}

void _backlight_on(void)
{
    /* configure backlight pin P0.0 as timer PWM output */
    PCON0 = ((PCON0 & ~(3 << 0)) | (2 << 0));

    _backlight_set_brightness(backlight_brightness);
}

void _backlight_off(void)
{
    /* configure backlight pin P0.0 as GPIO and switch off */
    PCON0 = ((PCON0 & ~(3 << 0)) | (1 << 0));
    PDAT0 &= ~(1 << 0);
}

void _buttonlight_on(void)
{
    PDAT3 |= (3 << 2);
    PDAT4 |= (1 << 2);
}

void _buttonlight_off(void)
{
    PDAT3 &= ~(3 << 2);
    PDAT4 &= ~(1 << 2);
}

bool _backlight_init(void)
{
    /* Enable button LEDs: P3.2 (menu/back), P3.3 (cursor), P4.2 (middle) */
    PCON3 = (PCON3 & ~0x0000FF00) | 0x00001100;
    PCON4 = (PCON4 & ~0x00000F00) | 0x00000100;
    _buttonlight_off();

    /* enable timer clock */
    PWRCON &= ~(1 << 4);

    /* configure timer */
    TACMD = (1 << 1);   /* TA_CLR */
    TACON = (0 << 13) | /* TA_INT1_EN */
            (0 << 12) | /* TA_INT0_EN */
            (1 << 11) | /* TA_START */
            (3 << 8) |  /* TA_CS = PCLK / 64 */
            (1 << 4);   /* TA_MODE_SEL = PWM mode */
    TADATA1 = 256;      /* set PWM period */
    TAPRE = 20;         /* prescaler */
    TACMD = (1 << 0);   /* TA_EN */
   
    _backlight_on();

    return true;
}

