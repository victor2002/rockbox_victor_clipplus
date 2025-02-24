/*
 * motion_comp.c
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: motion_comp_c.c 29381 2011-02-22 21:44:15Z jethead71 $
 */
#include <inttypes.h>
#include "mpeg2.h"
#include "attributes.h"
#include "mpeg2_internal.h"
#include "motion_comp.h"

/* definitions of the actual mc functions */

MC_FUNC (put, o)
MC_FUNC (avg, o)
MC_FUNC (put, x)
MC_FUNC (avg, x)
MC_FUNC (put, y)
MC_FUNC (avg, y)
MC_FUNC (put, xy)
MC_FUNC (avg, xy)
