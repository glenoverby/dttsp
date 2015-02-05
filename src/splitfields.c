/** 
* @file splitfields.c
* @brief Functions to split character fields 
* @author Frank Brickle, AB2KT and Bob McGwier, N4HY 

This file is part of a program that implements a Software-Defined Radio.

Copyright (C) 2004, 2005, 2006, 2007, 2008 by Frank Brickle, AB2KT and Bob McGwier, N4HY
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

#include <splitfields.h>

static char *_white = " \t\n";

/* -------------------------------------------------------------------------- */
/** @brief Create a new character block structure 
 * @return SPLIT
*/
/* ---------------------------------------------------------------------------- */
SPLIT
newSPLIT(void) {
  return (SPLIT) safealloc(1, sizeof(splitfld), "splitfield");
}

/* -------------------------------------------------------------------------- */
/** @brief Destroy a character block structure
* 
* @param s 
* @return void
*/
/* ---------------------------------------------------------------------------- */
void
delSPLIT(SPLIT s) {
  safefree((char *) s);
}

/* -------------------------------------------------------------------------- */
/** @brief Function to return the spliting character 
* 
* @param s 
* @param i 
* @return *char
*/
/* ---------------------------------------------------------------------------- */
char *
F(SPLIT s, int i) {
  return s->f[i];
}

/* -------------------------------------------------------------------------- */
/** @brief Function to return a pointer to the spliting character 
* 
* @param s 
* @param i 
* @return *char
*/
/* ---------------------------------------------------------------------------- */
char **
Fptr(SPLIT s, int i) {
  return &(s->f[i]);
}

/* -------------------------------------------------------------------------- */
/** @brief Function to return the number of characters 
* 
* @param s 
* @return int
*/
/* ---------------------------------------------------------------------------- */
int
NF(SPLIT s) {
  return s->n;
}

/* -------------------------------------------------------------------------- */
/** @brief Function to break character string on tokens to a maximum value 
* 
* @param s  -- The split structure 
* @param str -- vector of character stings  
* @param delim -- the spliting token 
* @param *fld -- character sting vector for each element in fmx 
* @param fmx -- maximum number of element in fld vector 
* @return The number times the splits will be less than fmx 
*/
/* ---------------------------------------------------------------------------- */
int
splitonto(SPLIT s, char *str, char *delim, char **fld, int fmx) {
  int i = 0;
  char *p = strtok(str, delim);
  while (p) {
    fld[i] = p;
    if (++i >= fmx)
      break;
    p = strtok(0, delim);
  }
  return i;
}

/* -------------------------------------------------------------------------- */
/** @brief Function to break character string on tokens 
* 
* @param s 
* @param str 
* @param delim 
* @return int
*/
/* ---------------------------------------------------------------------------- */
int
spliton(SPLIT s, char *str, char *delim) {
  return (s->n = splitonto(s, str, delim, s->f, MAXFLD));
}

/* -------------------------------------------------------------------------- */
/** @brief Function to break a caracter string into default tokens 
* 
* @param s 
* @param str 
* @return void
*/
/* ---------------------------------------------------------------------------- */
void
split(SPLIT s, char *str) {
  spliton(s, str, _white);
}
