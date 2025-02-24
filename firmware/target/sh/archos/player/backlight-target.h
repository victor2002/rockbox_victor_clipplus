/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: backlight-target.h 17847 2008-06-28 18:10:04Z bagder $
 *
 * Copyright (C) 2007 by Jens Arnold
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
#ifndef BACKLIGHT_TARGET_H
#define BACKLIGHT_TARGET_H

#include "config.h"
#include "cpu.h"

static inline bool _backlight_init(void)
{
    PACR1 &= ~0x3000;    /* Set PA14 (backlight control) to GPIO */
    and_b(~0x40, &PADRH); /* drive and set low */
    or_b(0x40, &PAIORH); /* ..and output */
    return true;
}

static inline void _backlight_on(void)
{
    and_b(~0x40, &PADRH); /* drive and set low */
    or_b(0x40, &PAIORH);
}

static inline void _backlight_off(void)
{
    and_b(~0x40, &PAIORH); /* let it float (up) */
}

#endif
