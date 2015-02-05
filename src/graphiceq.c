/** 
* @file graphiceq.c
* @author Frank Brickle, AB2KT and Bob McGwier, N4HY
* @brief  PCM frequency domain equalizer

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

#include <graphiceq.h>

/* -------------------------------------------------------------------------- */
/** @brief graphiceq 
* 
* @param a 
*/
/* ---------------------------------------------------------------------------- */
void
graphiceq(EQ a) {
  int sigsize = CXBhave(a->data), sigidx = 0;

  do {
    memcpy(CXBbase(a->in),
	   &CXBdata(a->data, sigidx),
	   256 * sizeof (COMPLEX));
    filter_OvSv(a->p);
    memcpy(&CXBdata (a->data, sigidx),
	   CXBbase (a->out),
	   256 * sizeof (COMPLEX));
    sigidx += 256;
  } while (sigidx < sigsize);
}

/* -------------------------------------------------------------------------- */
/** @brief new_EQ 
* 
* @param d 
* @param samplerate 
* @param pbits 
*/
/* ---------------------------------------------------------------------------- */
EQ
new_EQ (CXB d, REAL samplerate, int pbits) {
  ComplexFIR BP;
  EQ a = (EQ) safealloc(1, sizeof (eq), "new eq state");

  BP = newFIR_Bandpass_COMPLEX(-6000.0, 6000.0, samplerate, 257);
  a->p = newFiltOvSv(BP->coef, 257, pbits);
  a->in = newCXB(256, FiltOvSv_fetchpoint(a->p), "EQ input CXB");
  a->out = newCXB(256, FiltOvSv_storepoint(a->p), "EQ output CXB");
  a->data = d;
  delFIR_Bandpass_COMPLEX(BP);
  return a;
}

/* -------------------------------------------------------------------------- */
/** @brief delEQ 
* 
* @param a 
*/
/* ---------------------------------------------------------------------------- */
void
delEQ (EQ a) {
  if (a) {
    delCXB(a->in);
    delCXB(a->out);
    delFiltOvSv (a->p);
    safefree ((char *) a);
  }
}

