/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: pictureflow.c 27873 2010-08-24 14:30:46Z kugel $
 *
 * Copyright (C) 2009 Andrew Mahone
 *
 * Overlay loader stub plugin for PictureFlow on Archos
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
#include "plugin.h"

#include "lib/overlay.h"



/* this is the plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
    return run_overlay(parameter, PLUGIN_DEMOS_DIR "/pictureflow.ovl", "PictureFlow");
}
