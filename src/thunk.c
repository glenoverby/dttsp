/** 
* @file thunk.c
* @brief Function to cross map a key table to a hash table 
* @author Frank Brickle, AB2KT and Bob McGwier, N4HY 

 This file is part of a program that implements a Software-Defined Radio.

Copyright (C) 2004, 2005, 2006, 2007, 2008 by Frank Brickle, AB2KT and Bob McGwier, N4HY.
Doxygen comments added by Dave Larsen, KV0S

This program is free software; you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

The authors can be reached by email at

ab2kt@arrl.net
or
rwmcgwier@gmail.com

or by paper mail at

The DTTS Microwave Society
6 Kathleen Place
Bridgewater, NJ 08807
*/

#include <common.h>
#include <thunk.h>

PRIVATE BOOLEAN
/* -------------------------------------------------------------------------- */
/** @brief streq 
* 
* @param p 
* @param q 
*/
/* ---------------------------------------------------------------------------- */
streq(char *p, char *q) {
  return !strcmp(p, q);
}

/** somewhere along the line
   we'll kick this up a notch
   with gperf */

/* -------------------------------------------------------------------------- */
/** @brief Lookup function for thunk 
* 
* @param ctb 
* @param key 
* @return Thunk
*/
/* ---------------------------------------------------------------------------- */
Thunk
Thunk_lookup(CTB ctb, char *key) {
  if (key && *key) {
    for (;;) {
      if (!ctb || !ctb->key || !ctb->thk)
	break;
      if (streq(key, ctb->key))
	return ctb->thk;
      ctb++;
    }
  }
  return (Thunk) 0;
}

#ifdef notdef
/* -------------------------------------------------------------------------- */
/** @brief Hash table lookup used if notedef is not defined 
* 
* @param str 
* @return unsigned long 
*/
/* ---------------------------------------------------------------------------- */
unsigned long
hash(unsigned char *str) {
  unsigned long hash = 5381;
  int c;
  while (c = *str++)
    hash = ((hash << 5) + hash) + c;	// (hash * 33 + c) better
  return hash;
}
#endif
