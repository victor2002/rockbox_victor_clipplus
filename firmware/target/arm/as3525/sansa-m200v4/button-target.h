/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: button-target.h 19405 2008-12-12 19:50:49Z Domonoky $
 *
 * Copyright (C) 2007 by Dave Chapman
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

#define HAS_BUTTON_HOLD

void button_init_device(void);
int button_read_device(void);
bool button_hold(void);

/* Main unit's buttons */
#define BUTTON_POWER        0x00000001
#define BUTTON_VOL_UP       0x00000002
#define BUTTON_VOL_DOWN     0x00000004
#define BUTTON_UP           0x00000008
#define BUTTON_DOWN         0x00000010
#define BUTTON_LEFT         0x00000020
#define BUTTON_RIGHT        0x00000040
#define BUTTON_SELECT       0x00000080
#define BUTTON_HOLD         0x00000100

#define BUTTON_MAIN (BUTTON_POWER|BUTTON_VOL_UP|BUTTON_VOL_DOWN\
                    |BUTTON_UP|BUTTON_DOWN|BUTTON_LEFT\
                    |BUTTON_RIGHT|BUTTON_SELECT|BUTTON_HOLD)

#define BUTTON_REMOTE 0

/* Software power-off */
#define POWEROFF_BUTTON BUTTON_POWER
#define POWEROFF_COUNT 40
                
#endif /* _BUTTON_TARGET_H_ */
