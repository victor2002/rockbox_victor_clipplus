/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003-2004 M. Bakker, Ahead Software AG, http://www.nero.com
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Ahead Software through Mpeg4AAClicense@nero.com.
**
** $Id: huffman.h 27225 2010-07-01 21:18:42Z Buschel $
**/

#ifndef __HUFFMAN_H__
#define __HUFFMAN_H__

#ifdef __cplusplus
extern "C" {
#endif

int8_t huffman_scale_factor(bitfile *ld);
uint8_t huffman_spectral_data(uint8_t cb, bitfile *ld, int16_t *sp) ICODE_ATTR_FAAD_LARGE_IRAM;
#ifdef ERROR_RESILIENCE
int8_t huffman_spectral_data_2(uint8_t cb, bits_t *ld, int16_t *sp);
#endif

#ifdef __cplusplus
}
#endif
#endif
