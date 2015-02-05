/** 
* @file dcblock.c
* @brief Function to implement dc blocks 
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

#include <dcblock.h>

/* -------------------------------------------------------------------------- */
/** @brief DCBlock 
* 
* @param dcb 
*/
/* ---------------------------------------------------------------------------- */
void
DCBlock(DCBlocker dcb) {
  int i;

  for (i = 0; i < CXBhave(dcb->buf); i++) {
    REAL x = CXBreal(dcb->buf, i),
         y = x - dcb->xm1 + 0.995 * dcb->ym1;
    dcb->xm1 = x;
    dcb->ym1 = y;
    CXBdata(dcb->buf, i) = Cmplx(y, 0.0);
  }
}

/* -------------------------------------------------------------------------- */
/** @brief resetDCBlocker
*
* @param dcb 
*/
/* ---------------------------------------------------------------------------- */
void
resetDCBlocker(DCBlocker dcb) {
  dcb->xm1 = dcb->ym1 = 0.0;
}

/* -------------------------------------------------------------------------- */
/** @brief newDCBlocker 
* 
* @param buf 
*/
/* ---------------------------------------------------------------------------- */
DCBlocker
newDCBlocker(CXB buf) {
  DCBlocker dcb =
    (DCBlocker) safealloc(1, sizeof(DCBlockerInfo), "DCBlocker");
  dcb->buf = newCXB(CXBsize(buf), CXBbase(buf), "DCBlocker");
  resetDCBlocker(dcb);
  return dcb;
}

/* -------------------------------------------------------------------------- */
/** @brief delDCBlocker 
* 
* @param dcb 
*/
/* ---------------------------------------------------------------------------- */
void
delDCBlocker(DCBlocker dcb) {
  if (dcb) {
    delCXB(dcb->buf);
    safefree((char *) dcb);
  }
}
