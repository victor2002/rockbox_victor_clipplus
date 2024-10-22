/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: ata-mr500.c 31182 2011-12-08 21:23:53Z dreamlayers $
 *
 * Copyright (C) 2007 by Karl Kurbjun
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

#include "config.h"
#include "cpu.h"
#include "kernel.h"
#include "thread.h"
#include "system.h"
#include "power.h"
#include "panic.h"
#include "pcf50606.h"
#include "ata-driver.h"
#include "backlight-target.h"

/* ARESET on C7C68300 and RESET on ATA interface (Active Low) */
#define ATA_RESET_ENABLE  (IO_GIO_BITCLR0 = 1 << 10)
#define ATA_RESET_DISABLE (IO_GIO_BITSET0 = 1 << 10)

void ata_reset(void)
{
    ATA_RESET_ENABLE;
    sleep(1); /* > 25us */
    ATA_RESET_DISABLE;
    sleep(1); /* > 2ms */
}

/* This function is called before enabling the USB bus */
void ata_enable(bool on)
{
    (void) on;
    return;
}

bool ata_is_coldstart(void)
{
    /* If the reset pin direction is unconfigured it is a coldstart */
    return (IO_GIO_DIR0&(1<<10));
}

void ata_device_init(void)
{
    /* ATA reset */
    ATA_RESET_DISABLE; /* Set the pin to disable an active low reset */
    
    /* ATA INT pin 11 (currently unused) */
}

