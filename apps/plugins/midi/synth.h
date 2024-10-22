/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: synth.h 17847 2008-06-28 18:10:04Z bagder $
 *
 * Copyright (C) 2005 Stepan Moskovchenko
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
int initSynth(struct MIDIfile * mf, char * filename, char * drumConfig);
void setPoint(struct SynthObject * so, int pt);
void synthSamples(int32_t *buf_ptr, unsigned int num_samples);

void resetControllers(void);

static inline struct Event * getEvent(struct Track * tr, int evNum)
{
    return tr->dataBlock + (evNum*sizeof(struct Event));
}

