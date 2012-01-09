/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#include "config.h"
#include "system.h"
#include "general.h"
#include "kernel.h"
#include "pcm.h"
#include "pcm-internal.h"
#include "pcm_mixer.h"
#include "dsp.h"

/* Channels use standard-style PCM callback interface but a latency of one
   frame by double-buffering is introduced in order to facilitate mixing and
   keep the hardware fed. There must be sufficient time to perform operations
   before the last samples are sent to the codec and so things are done in
   parallel (as much as possible) with sending-out data. */

/* Define this to nonzero to add a marker pulse at each frame start */
#define FRAME_BOUNDARY_MARKERS 0

/* Descriptor for each channel */
struct mixer_channel
{
    unsigned char *start;            /* Buffer pointer */
    size_t size;                     /* Bytes remaining */
    size_t last_size;                /* Size of consumed data in prev. cycle */
    pcm_play_callback_type get_more; /* Registered callback */
    enum channel_status status;      /* Playback status */
    uint32_t amplitude;              /* Amp. factor: 0x0000 = mute, 0x10000 = unity */
};

/* Forget about boost here for the moment */
#define MIX_FRAME_SIZE      (MIX_FRAME_SAMPLES*4)

/* Because of the double-buffering, playback is always from here, otherwise a
   mechanism for the channel callbacks not to free buffers too early would be
   needed (if we _really_ want it and it's worth it, we _can_ do that ;-) ) */
static uint32_t downmix_buf[2][MIX_FRAME_SAMPLES] DOWNMIX_BUF_IBSS MEM_ALIGN_ATTR;
static int downmix_index = 0;   /* Which downmix_buf? */
static size_t next_size = 0;    /* Size of buffer to play next time */

/* Descriptors for all available channels */
static struct mixer_channel channels[PCM_MIXER_NUM_CHANNELS] IBSS_ATTR;

/* History for channel peaks */
static struct pcm_peaks channel_peaks[PCM_MIXER_NUM_CHANNELS];

/* Packed pointer array of all playing (active) channels in "channels" array */
static struct mixer_channel * active_channels[PCM_MIXER_NUM_CHANNELS+1] IBSS_ATTR;

/* Number of silence frames to play after all data has played */
#define MAX_IDLE_FRAMES     (NATIVE_FREQUENCY*3 / MIX_FRAME_SAMPLES)
static unsigned int idle_counter = 0;

/* Cheapo buffer align macro to align to the 16-16 PCM size */
#define ALIGN_CHANNEL(start, size) \
    ({ start = (void *)(((uintptr_t)start + 3) & ~3); \
       size &= ~3; })

#if (CONFIG_PLATFORM & PLATFORM_NATIVE)

/* Include any implemented CPU-optimized mixdown routines */
#if defined(CPU_ARM)
#if ARM_ARCH >= 6
#include "pcm-mixer-armv6.c"
#elif ARM_ARCH >= 5
#include "pcm-mixer-armv5.c"
#else
#include "pcm-mixer-armv4.c"
#endif /* ARM_ARCH */
#elif defined (CPU_COLDFIRE)
#include "pcm-mixer-coldfire.c"
#endif /* CPU_* */

#endif /* CONFIG_PLATFORM */

/** Generic mixing routines **/

#ifndef MIXER_OPTIMIZED_MIX_SAMPLES
#include "dsp-util.h" /* for clip_sample_16 */

/* Mix channels' samples and apply gain factors */
static FORCE_INLINE void mix_samples(uint32_t *out,
                                     int16_t *src0,
                                     int32_t src0_amp,
                                     int16_t *src1,
                                     int32_t src1_amp,
                                     size_t size)
{
    if (src0_amp == MIX_AMP_UNITY && src1_amp == MIX_AMP_UNITY)
    {
        /* Both are unity amplitude */
        do
        {
            int32_t l = *src0++ + *src1++;
            int32_t h = *src0++ + *src1++;
            *out++ = (uint16_t)clip_sample_16(l) | (clip_sample_16(h) << 16);
        }
        while ((size -= 4) > 0);
    }
    else if (src0_amp != MIX_AMP_UNITY && src1_amp != MIX_AMP_UNITY)
    {
        /* Neither are unity amplitude */
        do
        {
            int32_t l = (*src0++ * src0_amp >> 16) + (*src1++ * src1_amp >> 16);
            int32_t h = (*src0++ * src0_amp >> 16) + (*src1++ * src1_amp >> 16);
            *out++ = (uint16_t)clip_sample_16(l) | (clip_sample_16(h) << 16);
        }
        while ((size -= 4) > 0);
    }
    else
    {
        /* One is unity amplitude */
        if (src0_amp != MIX_AMP_UNITY)
        {
            /* Keep unity in src0, amp0 */
            int16_t *src_tmp = src0;
            src0 = src1;
            src1 = src_tmp;
            src1_amp = src0_amp;
            src0_amp = MIX_AMP_UNITY;
        }

        do
        {
            int32_t l = *src0++ + (*src1++ * src1_amp >> 16);
            int32_t h = *src0++ + (*src1++ * src1_amp >> 16);
            *out++ = (uint16_t)clip_sample_16(l) | (clip_sample_16(h) << 16);
        }
        while ((size -= 4) > 0);
    }
}
#endif /* MIXER_OPTIMIZED_MIX_SAMPLES */

#ifndef MIXER_OPTIMIZED_WRITE_SAMPLES
/* Write channel's samples and apply gain factor */
static FORCE_INLINE void write_samples(uint32_t *out,
                                       int16_t *src,
                                       int32_t amp,
                                       size_t size)
{
    if (LIKELY(amp == MIX_AMP_UNITY))
    {
        /* Channel is unity amplitude */
        memcpy(out, src, size);
    }
    else
    {
        /* Channel needs amplitude cut */
        do
        {
            int32_t l = *src++ * amp >> 16;
            int32_t h = *src++ * amp & 0xffff0000;
            *out++ = (uint16_t)l | h;
        }
        while ((size -= 4) > 0);
    }     
}
#endif /* MIXER_OPTIMIZED_WRITE_SAMPLES */


/** Private generic routines **/

/* Mark channel active to mix its data */
static void mixer_activate_channel(struct mixer_channel *chan)
{
    void **elem = find_array_ptr((void **)active_channels, chan);

    if (!*elem)
    {
        idle_counter = 0;
        *elem = chan;
    }
}

/* Stop channel from mixing */
static void mixer_deactivate_channel(struct mixer_channel *chan)
{
    remove_array_ptr((void **)active_channels, chan);
}

/* Deactivate channel and change it to stopped state */
static void channel_stopped(struct mixer_channel *chan)
{
    mixer_deactivate_channel(chan);
    chan->size = 0;
    chan->start = NULL;
    chan->status = CHANNEL_STOPPED;
}

/* Main PCM callback - sends the current prepared frame to play */
static void mixer_pcm_callback(unsigned char **start, size_t *size)
{
    *start = (unsigned char *)downmix_buf[downmix_index];
    *size = next_size;
}

/* Buffering callback - calls sub-callbacks and mixes the data for next
   buffer to be sent from mixer_pcm_callback() */
static void MIXER_CALLBACK_ICODE mixer_buffer_callback(void)
{
    downmix_index ^= 1; /* Next buffer */

    void *mixptr = downmix_buf[downmix_index];
    size_t mixsize = MIX_FRAME_SIZE;
    struct mixer_channel **chan_p;

    next_size = 0;

    /* "Loop" back here if one round wasn't enough to fill a frame */
fill_frame:
    chan_p = active_channels;

    while (*chan_p)
    {
        /* Find the active channel with the least data remaining and call any
           callbacks for channels that ran out - stopping whichever report
           "no more" */
        struct mixer_channel *chan = *chan_p;
        chan->start += chan->last_size;
        chan->size -= chan->last_size;

        if (chan->size == 0)
        {
            if (chan->get_more)
            {
                chan->get_more(&chan->start, &chan->size);
                ALIGN_AUDIOBUF(chan->start, chan->size);
            }

            if (!(chan->start && chan->size))
            {
                /* Channel is stopping */
                channel_stopped(chan);
                continue;
            }
        }

        /* Channel will play for at least part of this frame */

        /* Channel with least amount of data remaining determines the downmix
           size */
        if (chan->size < mixsize)
            mixsize = chan->size;

        chan_p++;
    }

    /* Add all still-active channels to the downmix */
    chan_p = active_channels;

    if (LIKELY(*chan_p))
    {
        struct mixer_channel *chan = *chan_p++;

        if (LIKELY(!*chan_p))
        {
            write_samples(mixptr, (void *)chan->start,
                          chan->amplitude, mixsize);
        }
        else
        {
            void *src0, *src1;
            unsigned int amp0, amp1;

            /* Mix first two channels with each other as the downmix */
            src0 = chan->start;
            amp0 = chan->amplitude;
            chan->last_size = mixsize;

            chan = *chan_p++;
            src1 = chan->start;
            amp1 = chan->amplitude;

            while (1)
            {
                mix_samples(mixptr, src0, amp0, src1, amp1, mixsize);

                if (!*chan_p)
                    break;

                /* More channels to mix - mix each with existing downmix */
                chan->last_size = mixsize;
                chan = *chan_p++;
                src0 = mixptr;
                amp0 = MIX_AMP_UNITY;
                src1 = chan->start;
                amp1 = chan->amplitude;
            }
        }

        chan->last_size = mixsize;
        next_size += mixsize;

        if (next_size < MIX_FRAME_SIZE)
        {
            /* There is still space remaining in this frame */
            mixptr += mixsize;
            mixsize = MIX_FRAME_SIZE - next_size;
            goto fill_frame;
        }
    }
    else if (idle_counter++ < MAX_IDLE_FRAMES)
    {
        /* Pad incomplete frames with silence */
        if (idle_counter <= 3)
            memset(mixptr, 0, MIX_FRAME_SIZE - next_size);

        next_size = MIX_FRAME_SIZE;
    }
    /* else silence period ran out - go to sleep */

#if FRAME_BOUNDARY_MARKERS != 0
    if (next_size)
        *downmix_buf[downmix_index] = downmix_index ? 0x7fff7fff : 0x80008000;
#endif
}

/* Start PCM driver if it's not currently playing */
static void mixer_start_pcm(void)
{
    if (pcm_is_playing())
        return;

#if defined(HAVE_RECORDING)
    if (pcm_is_recording())
        return;
#endif

    /* Requires a shared global sample rate for all channels */
    pcm_set_frequency(NATIVE_FREQUENCY);

    /* Prepare initial frames and set up the double buffer */
    mixer_buffer_callback();

    /* Save the previous call's output */
    void *start = downmix_buf[downmix_index];

    mixer_buffer_callback();

    pcm_play_set_dma_started_callback(mixer_buffer_callback);
    pcm_play_data(mixer_pcm_callback, start, MIX_FRAME_SIZE);
}

/* Initialize the channel and start it if it has data */
static void mixer_channel_play_start(struct mixer_channel *chan,
                                     pcm_play_callback_type get_more,
                                     unsigned char *start, size_t size)
{
    pcm_play_unlock(); /* Allow playback while doing any callback */

    ALIGN_AUDIOBUF(start, size);

    if (!(start && size))
    {
        /* Initial buffer not passed - call the callback now */
        size = 0;
        if (get_more)
        {
            get_more(&start, &size);
            ALIGN_AUDIOBUF(start, size);
        }
    }

    if (start && size)
    {
        /* We have data - start the channel */
        chan->status = CHANNEL_PLAYING;
        chan->start = start;
        chan->size = size;
        chan->last_size = 0;
        chan->get_more = get_more;

        pcm_play_lock();
        mixer_activate_channel(chan);
        mixer_start_pcm();
    }
    else
    {
        /* Never had anything - stop it now */
        pcm_play_lock();
        channel_stopped(chan);
    }
}


/** Public interfaces **/

/* Start playback on a channel */
void mixer_channel_play_data(enum pcm_mixer_channel channel,
                             pcm_play_callback_type get_more,
                             unsigned char *start, size_t size)
{
    struct mixer_channel *chan = &channels[channel];

    pcm_play_lock();
    mixer_deactivate_channel(chan);
    mixer_channel_play_start(chan, get_more, start, size);
    pcm_play_unlock();
}

/* Pause or resume a channel (when started) */
void mixer_channel_play_pause(enum pcm_mixer_channel channel, bool play)
{
    struct mixer_channel *chan = &channels[channel];

    pcm_play_lock();

    if (play == (chan->status == CHANNEL_PAUSED) &&
        chan->status != CHANNEL_STOPPED)
    {
        if (play)
        {
            chan->status = CHANNEL_PLAYING;
            mixer_activate_channel(chan);
            mixer_start_pcm();
        }
        else
        {
            mixer_deactivate_channel(chan);
            chan->status = CHANNEL_PAUSED;
        }
    }

    pcm_play_unlock();
}

/* Stop playback on a channel */
void mixer_channel_stop(enum pcm_mixer_channel channel)
{
    struct mixer_channel *chan = &channels[channel];

    pcm_play_lock();
    channel_stopped(chan);
    pcm_play_unlock();
}

/* Set channel's amplitude factor */
void mixer_channel_set_amplitude(enum pcm_mixer_channel channel,
                                 unsigned int amplitude)
{
    channels[channel].amplitude = MIN(amplitude, MIX_AMP_UNITY);
}

/* Return channel's playback status */
enum channel_status mixer_channel_status(enum pcm_mixer_channel channel)
{
    return channels[channel].status;
}

/* Returns amount data remaining in channel before next callback */
size_t mixer_channel_get_bytes_waiting(enum pcm_mixer_channel channel)
{
    return channels[channel].size;
}

/* Return pointer to channel's playing audio data and the size remaining */
void * mixer_channel_get_buffer(enum pcm_mixer_channel channel, int *count)
{
    struct mixer_channel *chan = &channels[channel];
    void * buf = *(unsigned char * volatile *)&chan->start;
    size_t size = *(size_t volatile *)&chan->size;
    void * buf2 = *(unsigned char * volatile *)&chan->start;

    /* Still same buffer? */
    if (buf == buf2)
    {
        *count = size >> 2;
        return buf;
    }
    /* else can't be sure buf and size are related */

    *count = 0;
    return NULL;
}

/* Calculate peak values for channel */
void mixer_channel_calculate_peaks(enum pcm_mixer_channel channel,
                                   int *left, int *right)
{
    struct mixer_channel *chan = &channels[channel];
    struct pcm_peaks *peaks = &channel_peaks[channel];
    int count;
    const void *addr = mixer_channel_get_buffer(channel, &count);

    pcm_do_peak_calculation(peaks, chan->status == CHANNEL_PLAYING,
                            addr, count);

    if (left)
        *left = peaks->val[0];

    if (right)
        *right = peaks->val[1];
}

/* Stop ALL channels and PCM and reset state */
void mixer_reset(void)
{
    pcm_play_stop();

    while (*active_channels)
        channel_stopped(*active_channels);

    idle_counter = 0;
}
