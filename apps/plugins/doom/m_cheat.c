// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: m_cheat.c 30361 2011-08-26 16:12:51Z pondlife $
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// $Log$
// Revision 1.1  2006/03/28 15:44:01  dave
// Patch #2969 - Doom!  Currently only working on the H300.
//
//
// DESCRIPTION:
// Cheat sequence checking.
//
//-----------------------------------------------------------------------------

#include "m_cheat.h"

//
// CHEAT SEQUENCE PACKAGE
//

static int  firsttime = 1;
static unsigned char cheat_xlate_table[256];


//
// Called in st_stuff module, which handles the input.
// Returns a 1 if the cheat was successful, 0 if failed.
//
int
cht_CheckCheat
( cheatseq_t* cht,
  char  key )
{
   int i;
   int rc = 0;

   if (firsttime)
   {
      firsttime = 0;
      for (i=0;i<256;i++)
         cheat_xlate_table[i] = SCRAMBLE(i);
   }

   if (!cht->p)
      cht->p = cht->sequence; // initialize if first time

   if (*cht->p == 0)
      *(cht->p++) = key;
   else if
   (cheat_xlate_table[(unsigned char)key] == *cht->p)
      cht->p++;
   else
      cht->p = cht->sequence;

   if (*cht->p == 1)
      cht->p++;
   else if (*cht->p == 0xff) // end of sequence character
   {
      cht->p = cht->sequence;
      rc = 1;
   }

   return rc;
}

void
cht_GetParam
( cheatseq_t* cht,
  char*  buffer )
{

   unsigned char *p, c;

   p = cht->sequence;
   while (*(p++) != 1)
      ;

   do
   {
      c = *p;
      *(buffer++) = c;
      *(p++) = 0;
   }
   while (c && *p!=0xff );

   if (*p==0xff)
      *buffer = 0;

}


