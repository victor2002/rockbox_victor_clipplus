/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: dsp-util.h 31142 2011-12-04 18:19:39Z jethead71 $
 *
 * Copyright (C) 2011 by Michael Sevakis
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
#ifndef DSP_HELPER_H
#define DSP_HELPER_H

/** Clip sample to signed 16 bit range **/

#ifdef CPU_ARM
#if ARM_ARCH >= 6
static FORCE_INLINE int32_t clip_sample_16(int32_t sample)
{
    int32_t out;
	asm ("ssat %0, #16, %1"
        : "=r" (out) : "r"(sample));
    return out;
}
#define CLIP_SAMPLE_16_DEFINED
#endif /* ARM_ARCH */
#endif /* CPU_ARM */

#ifndef CLIP_SAMPLE_16_DEFINED
/* Generic implementation */
static FORCE_INLINE int32_t clip_sample_16(int32_t sample)
{
    if ((int16_t)sample != sample)
        sample = 0x7fff ^ (sample >> 31);
    return sample;
}
#endif /* CLIP_SAMPLE_16_DEFINED */

#undef CLIP_SAMPLE_16_DEFINED

#endif /* DSP_HELPER_H */
