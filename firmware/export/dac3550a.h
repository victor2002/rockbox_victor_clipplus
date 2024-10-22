/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: dac3550a.h 30924 2011-11-06 22:44:25Z torne $
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
#ifndef _DAC3550A_H_
#define _DAC3550A_H_

#include "config.h"

/*
    DAC I2C defs
*/
#define DAC_ADR         0x9a
#define DAC_DEV_WRITE   (DAC_ADR | 0x00)

#define DAC_REG_WRITE   0xc0

/* registers..*/
#define DAC_SR_REG      1
#define DAC_AVOL        2
#define DAC_GCFG        3

/* function prototypes */
extern int  tenthdb2reg(int db);
extern int  dac_volume(unsigned int left, unsigned int right, bool deemph);
extern void dac_enable(bool enable);
extern void dac_line_in(bool enable);
extern void dac_init(void);

#endif /* _DAC3550A_H_ */

