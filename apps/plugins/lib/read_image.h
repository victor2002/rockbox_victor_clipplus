/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id: read_image.h 22232 2009-08-09 22:20:14Z bertrik $
*
* Copyright (C) 2009 by Andrew Mahone
*
* This is a wrapper for the core jpeg_load.c
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

#ifndef _LIB_READ_IMAGE_H_
#define _LIB_READ_IMAGE_H_
#include <plugin.h>

int read_image_file(const char* filename, struct bitmap *bm, int maxsize,
                    int format, const struct custom_format *cformat);
#endif

