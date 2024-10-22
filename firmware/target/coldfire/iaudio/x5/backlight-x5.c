/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: backlight-x5.c 20793 2009-04-26 01:23:39Z kugel $
 *
 * Copyright (C) 2006 by Linus Nielsen Feltzing
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
#include "cpu.h"
#include "system.h"
#include "backlight.h"
#include "backlight-target.h"
#include "pcf50606.h"
#include "lcd.h"

bool _backlight_init(void)
{
    _backlight_set_brightness(DEFAULT_BRIGHTNESS_SETTING);
    _backlight_on();
    
    return true; /* Backlight always ON after boot. */
}

void _backlight_on(void)
{
    int level;
#ifdef HAVE_LCD_ENABLE
    lcd_enable(true); /* power on lcd + visible display */
#endif
    level = disable_irq_save();
    pcf50606_write(0x38, 0xb0); /* Backlight ON, GPO1INV=1, GPO1ACT=011 */
    restore_irq(level);
}

void _backlight_off(void)
{
    int level = disable_irq_save();
    pcf50606_write(0x38, 0x80); /* Backlight OFF, GPO1INV=1, GPO1ACT=000 */
    restore_irq(level);
#ifdef HAVE_LCD_ENABLE
    lcd_enable(false); /* power off visible display */
#endif
}

/* set brightness by changing the PWM */
void _backlight_set_brightness(int val)
{
    /* disable IRQs while bitbanging */
    int old_irq_level = disable_irq_save();
    pcf50606_write(0x35, (val << 1) | 0x01); /* 512Hz, Enable PWM */
    /* enable IRQs again */
    restore_irq(old_irq_level);
}

void _remote_backlight_on(void)
{
    and_l(~0x00200000, &GPIO_OUT);
}

void _remote_backlight_off(void)
{
    or_l(0x00200000, &GPIO_OUT);
}
