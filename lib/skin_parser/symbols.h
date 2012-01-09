/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: symbols.h 28485 2010-11-05 09:51:19Z bertrik $
 *
 * Copyright (C) 2010 Robert Bieber
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

#ifndef SYMBOLS_H
#define SYMBOLS_H

#ifdef __cplusplus
extern "C"
{
#endif


/* Symbol definitions for WPS parsing */

#define TAGSYM '%'
#define COMMENTSYM '#'
#define CONDITIONSYM '?'
#define MULTILINESYM ';'
#define ARGLISTOPENSYM '('
#define ARGLISTCLOSESYM ')'
#define ARGLISTSEPARATESYM ','
#define ENUMLISTSEPARATESYM '|'
#define ENUMLISTOPENSYM '<'
#define ENUMLISTCLOSESYM '>'
#define DEFAULTSYM '-'

#ifdef __cplusplus
}
#endif

#endif /* SYMBOLS_H */
