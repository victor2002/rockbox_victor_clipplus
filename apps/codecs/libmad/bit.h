/*
 * libmad - MPEG audio decoder library
 * Copyright (C) 2000-2004 Underbit Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: bit.h 27160 2010-06-27 18:59:54Z Buschel $
 */

# ifndef LIBMAD_BIT_H
# define LIBMAD_BIT_H

struct mad_bitptr {
  uint32_t *ptr;
  uint32_t readbit;
};

void mad_bit_init(struct mad_bitptr *, unsigned char const *);

# define mad_bit_finish(bitptr)         /* nothing */

unsigned int mad_bit_length(struct mad_bitptr const *,
                            struct mad_bitptr const *);

/* rockbox: not used
unsigned char mad_bit_bitsleft(struct mad_bitptr const *bitptr);
*/
unsigned char const *mad_bit_nextbyte(struct mad_bitptr const *);

void mad_bit_skip(struct mad_bitptr *, unsigned int);
uint32_t mad_bit_read(struct mad_bitptr *, unsigned int);
/* rockbox: not used
void mad_bit_write(struct mad_bitptr *, unsigned int, uint32_t);
*/

unsigned short mad_bit_crc(struct mad_bitptr, unsigned int, unsigned short);

# endif
