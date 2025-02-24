/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: mas35xx.c 28425 2010-10-31 21:09:34Z wodz $
 *
 * Driver for MAS35xx audio codec
 *
 *
 * Copyright (c) 2007 by Christian Gmeiner
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
#include "system.h" /* MAX MIN macros */
#include "audiohw.h"

const struct sound_settings_info audiohw_settings[] = {
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    [SOUND_VOLUME]        = {"dB", 0,  1,-100,  12, -25},
    [SOUND_BASS]          = {"dB", 0,  1, -12,  12,   6},
    [SOUND_TREBLE]        = {"dB", 0,  1, -12,  12,   6},
#elif CONFIG_CODEC == MAS3507D
    [SOUND_VOLUME]        = {"dB", 0,  1, -78,  18, -18},
    [SOUND_BASS]          = {"dB", 0,  1, -15,  15,   7},
    [SOUND_TREBLE]        = {"dB", 0,  1, -15,  15,   7},
#endif
    [SOUND_BALANCE]       = {"%",  0,  1,-100, 100,   0},
    [SOUND_CHANNELS]      = {"",   0,  1,   0,   5,   0},
    [SOUND_STEREO_WIDTH]  = {"%",  0,  5,   0, 250, 100},
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    [SOUND_LOUDNESS]      = {"dB", 0,  1,   0,  17,   0},
    [SOUND_AVC]           = {"",   0,  1,  -1,   4,   0},
    [SOUND_MDB_STRENGTH]  = {"dB", 0,  1,   0, 127,  48},
    [SOUND_MDB_HARMONICS] = {"%",  0,  1,   0, 100,  50},
    [SOUND_MDB_CENTER]    = {"Hz", 0, 10,  20, 300,  60},
    [SOUND_MDB_SHAPE]     = {"Hz", 0, 10,  50, 300,  90},
    [SOUND_MDB_ENABLE]    = {"",   0,  1,   0,   1,   0},
    [SOUND_SUPERBASS]     = {"",   0,  1,   0,   1,   0},
#endif
#if CONFIG_CODEC == MAS3587F && defined(HAVE_RECORDING)
    [SOUND_LEFT_GAIN]     = {"dB", 1,  1,   0,  15,   8},
    [SOUND_RIGHT_GAIN]    = {"dB", 1,  1,   0,  15,   8},
    [SOUND_MIC_GAIN]      = {"dB", 1,  1,   0,  15,   2},
#endif
};

int channel_configuration = SOUND_CHAN_STEREO;
int stereo_width = 100;

#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
unsigned long mdb_shape_shadow = 0;
unsigned long loudness_shadow = 0;
unsigned long shadow_io_control_main;
#endif


static void set_channel_config(void)
{
    /* default values: stereo */
    unsigned long val_ll = 0x80000;
    unsigned long val_lr = 0;
    unsigned long val_rl = 0;
    unsigned long val_rr = 0x80000;
    int bank;

    switch(channel_configuration)
    {
        /* case SOUND_CHAN_STEREO unnecessary */

        case SOUND_CHAN_MONO:
            val_ll = 0xc0000;
            val_lr = 0xc0000;
            val_rl = 0xc0000;
            val_rr = 0xc0000;
            break;

        case SOUND_CHAN_CUSTOM:
            {
                /* fixed point variables (matching MAS internal format)
                   integer part: upper 13 bits (inlcuding sign)
                   fractional part: lower 19 bits */
                long fp_width, fp_straight, fp_cross;

                fp_width = (stereo_width << 19) / 100;
                if (stereo_width <= 100)
                {
                    fp_straight = - ((1<<19) + fp_width) / 2;
                    fp_cross = fp_straight + fp_width;
                }
                else
                {
                    /* straight = - (1 + width) / (2 * width) */
                    fp_straight = - ((((1<<19) + fp_width) / (fp_width >> 9)) << 9);
                    fp_cross = (1<<19) + fp_straight;
                }
                val_ll = val_rr = fp_straight & 0xfffff;
                val_lr = val_rl = fp_cross & 0xfffff;
            }
            break;

        case SOUND_CHAN_MONO_LEFT:
            val_ll = 0x80000;
            val_lr = 0x80000;
            val_rl = 0;
            val_rr = 0;
            break;

        case SOUND_CHAN_MONO_RIGHT:
            val_ll = 0;
            val_lr = 0;
            val_rl = 0x80000;
            val_rr = 0x80000;
            break;

        case SOUND_CHAN_KARAOKE:
            val_ll = 0xc0000;
            val_lr = 0x40000;
            val_rl = 0x40000;
            val_rr = 0xc0000;
            break;
    }

#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    bank = MAS_BANK_D0;
#elif CONFIG_CODEC == MAS3507D
    bank = MAS_BANK_D1;
#endif

    mas_writemem(bank, MAS_D0_OUT_LL, &val_ll, 1); /* LL */
    mas_writemem(bank, MAS_D0_OUT_LR, &val_lr, 1); /* LR */
    mas_writemem(bank, MAS_D0_OUT_RL, &val_rl, 1); /* RL */
    mas_writemem(bank, MAS_D0_OUT_RR, &val_rr, 1); /* RR */
}

void audiohw_set_channel(int val)
{
    channel_configuration = val;
    set_channel_config();
}

void audiohw_set_stereo_width(int val)
{
    stereo_width = val;
    if (channel_configuration == SOUND_CHAN_CUSTOM) {
        set_channel_config();
    }
}

void audiohw_set_bass(int val)
{
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    unsigned tmp = ((unsigned)(val * 8) & 0xff) << 8;
    mas_codec_writereg(MAS_REG_KBASS, tmp);
#elif CONFIG_CODEC == MAS3507D
    mas_writereg(MAS_REG_KBASS, bass_table[val+15]);
#endif
}

#if CONFIG_CODEC == MAS3507D
void audiohw_set_prescaler(int val)
{
    mas_writereg(MAS_REG_KPRESCALE, prescale_table[val/10]);
}
#endif

void audiohw_set_treble(int val)
{
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    unsigned tmp = ((unsigned)(val * 8) & 0xff) << 8;
    mas_codec_writereg(MAS_REG_KTREBLE, tmp);
#elif CONFIG_CODEC == MAS3507D
    mas_writereg(MAS_REG_KTREBLE, treble_table[val+15]);
#endif
}

#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
void audiohw_set_volume(int val) 
{
    unsigned tmp = ((unsigned)(val + 115) & 0xff) << 8;
    mas_codec_writereg(MAS_REG_VOLUME_CONTROL, tmp);
}

void audiohw_set_loudness(int value)
{
    loudness_shadow = (loudness_shadow & 0x04) |
                       (MAX(MIN(value * 4, 0x44), 0) << 8);
    mas_codec_writereg(MAS_REG_KLOUDNESS, loudness_shadow);
}

void audiohw_set_avc(int value)
{
    int tmp;

    static const uint16_t avc_vals[] =
    {
        (0x1 << 8) | (0x8 << 12), /* 20ms */
        (0x2 << 8) | (0x8 << 12), /* 2s */
        (0x4 << 8) | (0x8 << 12), /* 4s */
        (0x8 << 8) | (0x8 << 12), /* 8s */
    };
    switch (value) {
        case 1:
        case 2:
        case 3:
        case 4:
            tmp = avc_vals[value -1];
            break;
        case -1: /* turn off and then turn on again to decay quickly */
            tmp = mas_codec_readreg(MAS_REG_KAVC);
            mas_codec_writereg(MAS_REG_KAVC, 0);
            break;
        default: /* off */
            tmp = 0;
            break;
    }
    mas_codec_writereg(MAS_REG_KAVC, tmp);
}

void audiohw_set_mdb_strength(int value)
{
    mas_codec_writereg(MAS_REG_KMDB_STR, (value & 0x7f) << 8);
}

void audiohw_set_mdb_harmonics(int value)
{
    int tmp = value * 127 / 100;
    mas_codec_writereg(MAS_REG_KMDB_HAR, (tmp & 0x7f) << 8);
}

void audiohw_set_mdb_center(int value)
{
    mas_codec_writereg(MAS_REG_KMDB_FC, (value/10) << 8);
}

void audiohw_set_mdb_shape(int value)
{
    mdb_shape_shadow = (mdb_shape_shadow & 0x02) | ((value/10) << 8);
    mas_codec_writereg(MAS_REG_KMDB_SWITCH, mdb_shape_shadow);
}

void audiohw_set_mdb_enable(int value)
{
    mdb_shape_shadow = (mdb_shape_shadow & ~0x02) | (value?2:0);
    mas_codec_writereg(MAS_REG_KMDB_SWITCH, mdb_shape_shadow);
}

void audiohw_set_superbass(int value)
{
    loudness_shadow = (loudness_shadow & ~0x04) | (value?4:0);
    mas_codec_writereg(MAS_REG_KLOUDNESS, loudness_shadow);
}

void audiohw_set_balance(int val) 
{
    unsigned tmp = ((unsigned)(val * 127 / 100) & 0xff) << 8;
    mas_codec_writereg(MAS_REG_BALANCE, tmp);
}

void audiohw_set_pitch(unsigned long val)
{
    mas_writemem(MAS_BANK_D0, MAS_D0_OFREQ_CONTROL, &val, 1);

    /* We must tell the MAS that the frequency has changed.
     * This will unfortunately cause a short silence. */

    mas_writemem(MAS_BANK_D0, MAS_D0_IO_CONTROL_MAIN, &shadow_io_control_main, 1);
}

#endif /* CONFIG_CODEC == MAS3587F || CONFIG_CODEC == MAS3539F */

