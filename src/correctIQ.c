/** 
* @file correctIQ.c
* @brief Functions to correct IQ values 
* @author Frank Brickle, AB2KT and Bob McGwier, N4HY

This routine restores quadrature between arms of an analytic signal
possibly distorted by ADC hardware.

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

#include <correctIQ.h>

/* -------------------------------------------------------------------------- */
/** @brief new correctIQ object
* 
* @param phase 
* @param gain 
*/
/* ---------------------------------------------------------------------------- */
IQ
newCorrectIQ(REAL phase, REAL gain) {
  IQ iq = (IQ) safealloc(1, sizeof(iqstate), "IQ state");
  iq->phase = phase;
  iq->gain = gain;
  iq->flag = TRUE;
  iq->mu = 0.0025;
  memset((void *) iq->w, 0, 16 * sizeof(COMPLEX));
  return iq;
}

/* -------------------------------------------------------------------------- */
/** @brief destroy a correctIQ object 
* 
* @param iq 
*/
/* ---------------------------------------------------------------------------- */
void
delCorrectIQ(IQ iq) { safefree((char *) iq); }

/* -------------------------------------------------------------------------- */
/** @brief Run IQ correction
* 
* @param sigbuf 
* @param iq 
*/
/* ---------------------------------------------------------------------------- */
void
correctIQ(CXB sigbuf, IQ iq) {
  int i;
  for (i = 0; i < CXBhave(sigbuf); i++) {
    COMPLEX y;
    CXBimag(sigbuf, i) += iq->phase * CXBreal(sigbuf, i);
    CXBreal(sigbuf, i) *= iq->gain;
    y = Cadd(CXBdata(sigbuf, i),
	     Cmul(iq->w[0], Conjg(CXBdata(sigbuf, i))));
    iq->w[0] = Csub(Cscl(iq->w[0], 1.0 - iq->mu * 0.000001),
		    Cscl(Cmul(y, y), iq->mu));
    CXBdata(sigbuf, i) = y;
  }
}
