/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: button-target.h 31132 2011-12-03 23:13:17Z b0hoon $
 *
 * Copyright (C) 2008 by Mark Arigo
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

#ifndef _BUTTON_TARGET_H_
#define _BUTTON_TARGET_H_

#include <stdbool.h>
#include "config.h"

#define MEP_BUTTON_HEADER   0x19
#define MEP_BUTTON_ID       0x9
#define MEP_ABSOLUTE_HEADER 0x0b
#define MEP_FINGER          0x01
#define MEP_GESTURE         0x02

#define HAS_BUTTON_HOLD

bool button_hold(void);
void button_init_device(void);
int button_read_device(void);

#ifndef BOOTLOADER
void button_int(void);
#endif

/* Main unit's buttons */
#define BUTTON_POWER        0x00000001
#define BUTTON_MENU         0x00000002
#define BUTTON_VIEW         0x00000004
#define BUTTON_VOL_UP       0x00000008
#define BUTTON_VOL_DOWN     0x00000010
#define BUTTON_LEFT         0x00000020
#define BUTTON_RIGHT        0x00000040
#define BUTTON_UP           0x00000080
#define BUTTON_DOWN         0x00000100
#define BUTTON_NEXT         0x00000200
#define BUTTON_PREV         0x00000400
#define BUTTON_PLAY         0x00000800
#define BUTTON_TAP          0x00001000
#define BUTTON_MAIN         0x00001fff

/* No Remote control */
#define BUTTON_REMOTE 0

#define POWEROFF_BUTTON BUTTON_POWER
#define POWEROFF_COUNT 10

#endif /* _BUTTON_TARGET_H_ */
