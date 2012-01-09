/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: timefuncs.h 31319 2011-12-16 00:09:28Z dreamlayers $
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
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

#ifndef _TIMEFUNCS_H_
#define _TIMEFUNCS_H_

#include "config.h"
#include <stdbool.h>
#include "time.h"

struct tm *get_time(void);
int set_time(const struct tm *tm);
#if CONFIG_RTC
bool valid_time(const struct tm *tm);
void set_day_of_week(struct tm *tm);
#endif

#endif /* _TIMEFUNCS_H_ */

