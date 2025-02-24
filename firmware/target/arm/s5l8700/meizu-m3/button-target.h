/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: button-target.h 21820 2009-07-12 19:36:27Z bertrik $
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
#ifndef _BUTTON_TARGET_H_
#define _BUTTON_TARGET_H_

#include <stdbool.h>

#define HAS_BUTTON_HOLD

bool button_hold(void);
void button_init_device(void);
int button_read_device(void);

/* Meizu M3 specific button codes */
#define BUTTON_PREVNEXT     0x00000001
#define BUTTON_MENU         0x00000002
#define BUTTON_PLAY         0x00000004

/* there are no LEFT/RIGHT buttons, but other parts of the code expect them */
#define BUTTON_LEFT         0
#define BUTTON_RIGHT        0

#define BUTTON_MAIN (BUTTON_PREVNEXT|BUTTON_MENU|BUTTON_PLAY)

#define BUTTON_REMOTE       0

#define POWEROFF_BUTTON BUTTON_PLAY
#define POWEROFF_COUNT 10

#endif /* _BUTTON_TARGET_H_ */

