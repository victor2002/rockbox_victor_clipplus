/*
 * attributes.h
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
 * $Id: attributes.h 29381 2011-02-22 21:44:15Z jethead71 $
 * libmpeg2 sync history:
 * 2008-07-01 - CVS revision 1.5
 */

/* use gcc attribs to align critical data structures */
#ifdef ATTRIBUTE_ALIGNED_MAX
#define ATTR_ALIGN(align) __attribute__ ((__aligned__ ((ATTRIBUTE_ALIGNED_MAX < align) ? ATTRIBUTE_ALIGNED_MAX : align)))
#else
#define ATTR_ALIGN(align)
#endif

#if defined(LIKELY) && defined (UNLIKELY)
#define likely(x) LIKELY(x)
#define unlikely(x) UNLIKELY(x)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

