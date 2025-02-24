/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: appevents.h 29923 2011-05-25 08:35:31Z jethead71 $
 *
 * Copyright (C) 2008 by Jonathan Gordon
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

#ifndef _APPEVENTS_H
#define _APPEVENTS_H

#include <stdbool.h>
#include "events.h"

/** Only app/ level events should be defined here.
 *  firmware/ level events and CLASS's are defined in firmware/export/events.h
 */

/** Playback events **/
enum {
    /* Playback is starting from a stopped state */
    PLAYBACK_EVENT_START_PLAYBACK = (EVENT_CLASS_PLAYBACK|1),
    /* Audio has begun buffering for decoding track (or is already completed) */
    PLAYBACK_EVENT_TRACK_BUFFER,
    /* Handles for current user track are ready (other than audio or codec) */
    PLAYBACK_EVENT_CUR_TRACK_READY,
    /* Current user track finished */
    PLAYBACK_EVENT_TRACK_FINISH,
    /* A new current user track has begun */
    PLAYBACK_EVENT_TRACK_CHANGE,
    /* A manual skip is about to be processed */
    PLAYBACK_EVENT_TRACK_SKIP,
    /* Next track medadata was just loaded */
    PLAYBACK_EVENT_NEXTTRACKID3_AVAILABLE,
};

/** Buffering events **/
enum {
    BUFFER_EVENT_BUFFER_LOW = (EVENT_CLASS_BUFFERING|1),
    BUFFER_EVENT_REBUFFER,
    BUFFER_EVENT_CLOSED,
    BUFFER_EVENT_MOVED,
    BUFFER_EVENT_FINISHED,
};

/** Generic GUI class events **/
enum {
    GUI_EVENT_STATUSBAR_TOGGLE = (EVENT_CLASS_GUI|1),
    GUI_EVENT_ACTIONUPDATE,
    GUI_EVENT_THEME_CHANGED,
};

/** Recording events **/
enum {
    RECORDING_EVENT_START = (EVENT_CLASS_RECORDING|1),
    RECORDING_EVENT_STOP,
};
#endif


