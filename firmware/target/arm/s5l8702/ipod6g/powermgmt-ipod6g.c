/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: powermgmt-nano2g.c 28159 2010-09-24 22:42:06Z Buschel $
 *
 * Copyright © 2008 Rafaël Carré
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
#include "powermgmt.h"
#include "pmu-target.h"
#include "power.h"
#include "audiohw.h"

const unsigned short battery_level_dangerous[BATTERY_TYPES_COUNT] =
{
    3600
};

const unsigned short battery_level_shutoff[BATTERY_TYPES_COUNT] =
{
    3350
};

/* voltages (millivolt) of 0%, 10%, ... 100% when charging disabled */
const unsigned short percent_to_volt_discharge[BATTERY_TYPES_COUNT][11] =
{
    { 3600, 3730, 3780, 3810, 3840, 3880, 3930, 3990, 4080, 4170, 4270 }
};

#if CONFIG_CHARGING
/* voltages (millivolt) of 0%, 10%, ... 100% when charging enabled */
const unsigned short percent_to_volt_charge[11] =
{
    3800, 3900, 3990, 4070, 4140, 4200, 4250, 4290, 4320, 4340, 4350
};
#endif /* CONFIG_CHARGING */

/* ADC should read 0x3ff=6.00V */
#define BATTERY_SCALE_FACTOR 6000
/* full-scale ADC readout (2^10) in millivolt */


/* Returns battery voltage from ADC [millivolts] */
unsigned int battery_adc_voltage(void)
{
    int compensation = (10 * (pmu_read_battery_current() - 7)) / 12;
    if (charging_state()) return pmu_read_battery_voltage() - compensation;
    return pmu_read_battery_voltage() + compensation;
}


#ifdef HAVE_ACCESSORY_SUPPLY
void accessory_supply_set(bool enable)
{
    if (enable)
    {
        /* Accessory voltage supply on */
//TODO:        pmu_ldo_power_on(6);
    }
    else
    {
        /* Accessory voltage supply off */
//TODO:        pmu_ldo_power_off(6);
    }
}
#endif

#ifdef HAVE_LINEOUT_POWEROFF
void lineout_set(bool enable)
{
    /* Call audio hardware driver implementation */
    audiohw_enable_lineout(enable);
}
#endif
