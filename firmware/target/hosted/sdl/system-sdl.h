/***************************************************************************
 *             __________               __   ___.                  
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \  
 *                     \/            \/     \/    \/            \/ 
 * $Id: system-sdl.h 31339 2011-12-17 07:27:24Z jethead71 $
 *
 * Copyright (C) 2007 by Michael Sevakis
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
#ifndef _SYSTEM_SDL_H_
#define _SYSTEM_SDL_H_

#include <stdbool.h>
#include "config.h"
#include "gcc_extensions.h"

#define HIGHEST_IRQ_LEVEL 1

int set_irq_level(int level);

#define disable_irq() \
    ((void)set_irq_level(HIGHEST_IRQ_LEVEL))

#define enable_irq()  \
    ((void)set_irq_level(0))

#define disable_irq_save() \
    set_irq_level(HIGHEST_IRQ_LEVEL)

#define restore_irq(level) \
    ((void)set_irq_level(level))

void sim_enter_irq_handler(void);
void sim_exit_irq_handler(void);
void sim_kernel_shutdown(void);
void sys_poweroff(void);
void sys_handle_argv(int argc, char *argv[]);
void gui_message_loop(void);
void sim_do_exit(void) NORETURN_ATTR;
#ifndef HAVE_SDL_THREADS
void wait_for_interrupt(void);
#endif

extern bool background;  /* True if the background image is enabled */
extern bool showremote;
extern int display_zoom;
extern long start_tick;

static inline void commit_dcache(void) {}
static inline void commit_discard_dcache(void) {}
static inline void commit_discard_idcache(void) {}

#endif /* _SYSTEM_SDL_H_ */
