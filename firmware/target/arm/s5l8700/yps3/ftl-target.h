/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: ftl-target.h 23107 2009-10-11 13:15:23Z bertrik $
 *
 * Copyright (C) 2009 by Michael Sparmann
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

#ifndef __FTL_TARGET_H__
#define __FTL_TARGET_H__

#include "config.h"
#include "inttypes.h"

#ifdef BOOTLOADER
/* Bootloaders don't need write access */
#define FTL_READONLY
#endif

uint32_t ftl_init(void);
uint32_t ftl_read(uint32_t sector, uint32_t count, void* buffer);
uint32_t ftl_write(uint32_t sector, uint32_t count, const void* buffer);
uint32_t ftl_sync(void);


#endif
