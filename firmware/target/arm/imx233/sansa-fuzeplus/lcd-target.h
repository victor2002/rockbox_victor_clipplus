/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: lcd-target.h 31335 2011-12-16 23:40:39Z jethead71 $
 *
 * Copyright (c) 2011 by Amaury Pouly
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
#ifndef LCD_TARGET_H
#define LCD_TARGET_H

#define LCD_FRAMEBUF_ADDR(col, row) ((fb_data *)FRAME + (row)*LCD_WIDTH + (col))

/* Not really optimized, but are unusual */
#define LCD_OPTIMIZED_UPDATE
#define LCD_OPTIMIZED_UPDATE_RECT

#endif /* LCD_TARGET_H */
