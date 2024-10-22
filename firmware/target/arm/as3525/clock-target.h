/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: clock-target.h 28925 2010-12-29 16:07:15Z fredwbauer $
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
#ifndef CLOCK_TARGET_H
#define CLOCK_TARGET_H

#include "config.h"
#include "cpu.h"

/* returns clock divider, given maximal target frequency and clock reference */
#define CLK_DIV(ref, target) ((ref + target - 1) / target)


/*  Frequency and Bus Settings
 *  These bus settings work on the assumption that unboosted performance will be
 *  based on fastbus mode(FCLK == PCLK) at a frequency configured with this file.
 *  Boosted performance defaults to synchronous bus but will be changed to
 *  asynchronous bus if FCLK is not an integer multiple of PCLK.
 *  The player starts up in fastbus mode and synchronous or asynchronous mode is
 *  implemented in the set_cpu_frequency() function in system-as3525.c.  There
 *  are limitations on both frequencies and frequency relationships listed in 7.3.14
 *  of the as3525 datasheet that need to be observed.  If you are determined to
 *  use a frequency that is not "legal" you can do that. There are no checks for
 *  legal frequency values, only some validity checks to make sure the divider
 *  value fits into the number of bits allotted to it.
 *
 *  The CLOCK_DIV macro does a pretty good job at selecting divider values but
 *  you can always override it by choosing your own value and commenting out the
 *  macro.   AS3525_FCLK_PREDIV values other than 0 allow you to choose frequencies
 *  from lines below the main PLL frequency lines.  AS3525_FCLK_POSTDIV
 *  will be calculated automagically depending on the value you have selected
 *  for AS3525_FCLK_FREQ.  You may add more PLL frequencies by simply commenting
 *  out the current #defines for AS3525_PLLA_FREQ & AS3525_PLLA_SETTING and
 *  adding a #define for FREQ and divider setting to produce that frequency.I
 *  have included USB & PLLB for future use but commented them out for now.
 */

/* Clock Sources  */
#define AS3525_CLK_MAIN           0
#define AS3525_CLK_PLLA           1
#define AS3525_CLK_PLLB           2
#define AS3525_CLK_FCLK           3         /* Available as PCLK input only  */

/** ************ Change these to reconfigure clocking scheme *******************/
#if CONFIG_CPU == AS3525v2

/* PLLA & PLLB registers differ from AS3525(v1)
 * PLL bits:
 * - bit 0-6 = F-1 (F=multiplier)
 * - bit 7-9 = R-1 (R=divisor)
 * - bit 10  = OD (output divider)? Divides by 2 if set.
 * - bit 11  = unknown (no effect)
 * - bit 12  = unknown (always set to 1)
 * Fpll = Fin * F / (R * OD), where Fin = 12 MHz
 */
#define AS3525_PLLA_FREQ        240000000
#define AS3525_PLLA_SETTING     0x113B

#define AS3525_PLLB_FREQ        192000000   /* allows 44.1kHz with 0.04% error*/
#define AS3525_PLLB_SETTING     0x155F

#define AS3525_FCLK_PREDIV      0
#define AS3525_FCLK_FREQ        AS3525_PLLA_FREQ

/* XXX: CGU_PERI might also be different (i.e. no PCLK_DIV1_SEL), so we don't
 * set bit 6 (PCLK_DIV1_SEL) for the moment
 *
 * Note that setting bits 1:0 have no effect and they always read back as 0
 * Also note that CGU_PERI is based on fclk, not PLLA
 */

#ifdef SANSA_FUZEV2
/* display is unbearably slow at 24MHz
 * 34285715 HZ works ok but 40MHz works even better*/
#define AS3525_DRAM_FREQ        40000000    /* Initial DRAM frequency  */
#else
#define AS3525_DRAM_FREQ        24000000    /* Initial DRAM frequency  */
#endif /* SANSA_FUZEV2 */

#else
/* AS3525v1 */

/* PLL  frequencies and settings*/
#define AS3525_PLLA_FREQ        248000000   /*124,82.7,62,49.6,41.3,35.4 */
       /* FCLK_PREDIV->  *7/8 = 217MHz      108.5 ,72.3, 54.25, 43.4, 36.17 */
                      /* *6/8 = 186MHz      93, 62, 46.5, 37.2 */
                      /* *5/8 = 155MHz      77.5, 51.67, 38.75 */
#define AS3525_PLLA_SETTING     0x261F

/* PLLB frequencies and settings (audio and USB) */
#define AS3525_PLLB_FREQ        384000000   /* allows 44.1kHz with 0.04% error*/
#define AS3525_PLLB_SETTING     0x2630

//#define AS3525_PLLA_FREQ        384000000 /*192,128,96,76.8,64,54.9,48,42.7,38.4*/
       /* FCLK_PREDIV->  *7/8 = 336MHz      168, 112, 84, 67.2, 56, 48, 42, 37.3*/
                      /* *6/8 = 288MHz      144, 96, 72, 57.6, 48, 41.1, */
                      /* *5/8 = 240MHz      120, 80, 60, 48, 40  */
//#define AS3525_PLLA_SETTING     0x2630

#define AS3525_FCLK_PREDIV    0 /* div = (8-n)/8 Enter manually & postdiv will be calculated*/
                                /* 0 gives you the PLLA 1st line choices, 1 the 2nd line etc. */

#define AS3525_FCLK_FREQ      248000000            /* Boosted FCLK frequency  */
#define AS3525_DRAM_FREQ       62000000            /* Initial DRAM frequency  */
/* AS3525_PCLK_FREQ != AS3525_DRAM_FREQ/1 will boot to white lcd screen */

#endif  /* CONFIG_CPU == AS3525v2 */

#define AS3525_PCLK_FREQ      (AS3525_DRAM_FREQ/1)   /* PCLK divided from DRAM freq */

#define AS3525_DBOP_FREQ      (AS3525_PCLK_FREQ/1)   /* DBOP divided from PCLK freq */

/** ****************************************************************************/

/* Tell the software what frequencies we're running */
#define CPUFREQ_MAX              AS3525_FCLK_FREQ
#define CPUFREQ_DEFAULT          AS3525_PCLK_FREQ
#define CPUFREQ_NORMAL           AS3525_PCLK_FREQ

/* FCLK */
#define AS3525_FCLK_SEL          AS3525_CLK_PLLA
#define AS3525_FCLK_POSTDIV      (CLK_DIV((AS3525_PLLA_FREQ*(8-AS3525_FCLK_PREDIV)/8), AS3525_FCLK_FREQ) - 1) /*div=1/(n+1)*/

#if CONFIG_CPU == AS3525v2
#define AS3525_FCLK_POSTDIV_UNBOOSTED      (CLK_DIV((AS3525_PLLA_FREQ*(8-AS3525_FCLK_PREDIV)/8), CPUFREQ_NORMAL) - 1) /*div=1/(n+1) */
/* Since pclk is based on fclk, we need to change CGU_PERI as well */
#define AS3525_PCLK_DIV0_UNBOOSTED (CLK_DIV(CPUFREQ_NORMAL, AS3525_DRAM_FREQ) - 1) /*div=1/(n+1)*/
#endif /* CONFIG_CPU == AS3525v2 */

/* MCLK */
#if CONFIG_CPU == AS3525v2
/* on AMSv2 we can enable PLLB for MCLK to increase PCM sample rate accuracy
   with no significant impact on battery life */
#define AS3525_MCLK_SEL          AS3525_CLK_PLLB
#else
#define AS3525_MCLK_SEL          AS3525_CLK_PLLA
#endif /* CONFIG_CPU == AS3525v2 */

#if (AS3525_MCLK_SEL==AS3525_CLK_PLLA)
#define AS3525_MCLK_FREQ         AS3525_PLLA_FREQ
#elif (AS3525_MCLK_SEL==AS3525_CLK_PLLB)
#define AS3525_MCLK_FREQ         AS3525_PLLB_FREQ
#else
#error Choose either PLLA or PLLB for MCLK!
#endif

/* PCLK */

#if CONFIG_CPU == AS3525

#define AS3525_PCLK_SEL          AS3525_CLK_PLLA
         /*unable to use AS3525_PCLK_DIV1 != 0 successfuly so far*/
#define AS3525_PCLK_DIV1         (CLK_DIV(AS3525_DRAM_FREQ, AS3525_PCLK_FREQ) - 1)/* div = 1/(n+1)*/ 
#define AS3525_PCLK_DIV0         (CLK_DIV(AS3525_PLLA_FREQ, AS3525_DRAM_FREQ) - 1) /*div=1/(n+1)*/
#else

#define AS3525_PCLK_SEL          AS3525_CLK_FCLK
#define AS3525_PCLK_DIV0         (CLK_DIV(AS3525_FCLK_FREQ, AS3525_DRAM_FREQ) - 1) /*div=1/(n+1)*/

#endif /* CONFIG_CPU */

   /* PCLK as Source */
  #define AS3525_DBOP_DIV        (CLK_DIV(AS3525_PCLK_FREQ, AS3525_DBOP_FREQ) - 1) /*div=1/(n+1)*/
  #define AS3525_I2C_PRESCALER   CLK_DIV(AS3525_PCLK_FREQ, AS3525_I2C_FREQ)
  #define AS3525_I2C_FREQ        400000
  #define AS3525_SD_IDENT_DIV    ((CLK_DIV(AS3525_PCLK_FREQ, AS3525_SD_IDENT_FREQ) / 2) - 1)
  #define AS3525_SD_IDENT_FREQ   400000      /* must be between 100 & 400 kHz */
  #define AS3525_SSP_PRESCALER   ((CLK_DIV(AS3525_PCLK_FREQ, AS3525_SSP_FREQ) + 1) & ~1)    /* must be an even number */
  #define AS3525_SSP_FREQ        12000000

#define AS3525_IDE_SEL           AS3525_CLK_PLLA           /* Input Source   */
#define AS3525_IDE_DIV           (CLK_DIV(AS3525_PLLA_FREQ, AS3525_IDE_FREQ) - 1)/*div=1/(n+1)*/

#if CONFIG_CPU == AS3525v2
#define AS3525_MS_FREQ           120000000
#define AS3525_MS_DIV            (CLK_DIV(AS3525_PLLA_FREQ, AS3525_MS_FREQ) -1)
#define AS3525_SDSLOT_FREQ       24000000
#define AS3525_SDSLOT_DIV        (CLK_DIV(AS3525_PLLA_FREQ, AS3525_SDSLOT_FREQ) -1)
#define AS3525_IDE_FREQ          80000000
#else
#define AS3525_IDE_FREQ          50000000    /* The OF uses 66MHz maximal freq */
#endif /* CONFIG_CPU == AS3525v2 */


//#define AS3525_USB_SEL           AS3525_CLK_PLLA            /* Input Source   */
//#define AS3525_USB_DIV           /* div = 1/(n=0?1:2n)*/


/* Validity Checks */

/* AS3525_PCLK_FREQ  */
#if (CLK_DIV(AS3525_PLLA_FREQ, AS3525_PCLK_FREQ) - 1) >= (1<<4) /* 4 bits */
#error PCLK frequency is too low : clock divider will not fit !
#endif

/*  AS3525_DBOP_FREQ */
#if (CLK_DIV(AS3525_PCLK_FREQ, AS3525_DBOP_FREQ) - 1) >= (1<<3) /* 3 bits */
#error DBOP frequency is too low : clock divider will not fit !
#endif

/* AS3525_IDE_FREQ */
#if (CLK_DIV(AS3525_PLLA_FREQ, AS3525_IDE_FREQ) - 1) >= (1<<4) /* 4 bits */
#error IDE frequency is too low : clock divider will not fit !
#endif

/* AS3525_I2C_FREQ */
#if (CLK_DIV(AS3525_PCLK_FREQ, AS3525_I2C_FREQ)) >= (1<<10) /* 2+8 bits */
#error I2C frequency is too low : clock divider will not fit !
#endif

/* AS3525_SSP_FREQ */
#if (((CLK_DIV(AS3525_PCLK_FREQ, AS3525_SSP_FREQ)) + 1 ) & ~1) >= (1<<8) /* 8 bits */
#error SSP frequency is too low : clock divider will not fit !
#endif

/* AS3525_SD_IDENT_FREQ */
#if ((CLK_DIV(AS3525_PCLK_FREQ, AS3525_SD_IDENT_FREQ) / 2) - 1) >= (1<<8) /* 8 bits */
#error SD IDENTIFICATION frequency is too low : clock divider will not fit !
#endif

/* I2SIN / I2SOUT frequencies */
/* low samplerate */
#if ((AS3525_MCLK_FREQ/(128*8000))) > 512   /* 8kHz = lowest frequency */
#error AS3525_MCLK_FREQ is too high for 8kHz samplerate !
#endif
/* high samplerate */
#if ((AS3525_MCLK_FREQ/(128*96000))) < 1   /* 96kHz = highest frequency */
#error AS3525_MCLK_FREQ is too low for 96kHz samplerate !
#endif

#endif /* CLOCK_TARGET_H */
