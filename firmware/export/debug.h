/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: debug.h 29451 2011-02-27 23:42:37Z thomasjfox $
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
#ifndef DEBUG_H
#define DEBUG_H

#include "config.h"
#include "gcc_extensions.h"

extern void debug_init(void);
extern void debugf(const char* fmt,...) ATTRIBUTE_PRINTF(1, 2);
extern void ldebugf(const char* file, int line, const char *fmt, ...)
                    ATTRIBUTE_PRINTF(3, 4);

#ifndef CODEC                    
#ifdef __GNUC__

/*  */
#if defined(SIMULATOR) && !defined(__PCTOOL__) \
    || ((CONFIG_PLATFORM & (PLATFORM_ANDROID|PLATFORM_MAEMO|PLATFORM_PANDORA)) && defined(DEBUG))
#define DEBUGF  debugf
#define LDEBUGF(...) ldebugf(__FILE__, __LINE__, __VA_ARGS__)
#else
#if defined(DEBUG)

#ifdef HAVE_GDB_API
void breakpoint(void);
#endif

#define DEBUGF  debugf
#define LDEBUGF debugf
#else
#define DEBUGF(...) do { } while(0)
#define LDEBUGF(...) do { } while(0)
#endif
#endif


#else

#define DEBUGF debugf
#define LDEBUGF debugf

#endif /* GCC */

#endif /* CODEC */
#endif
