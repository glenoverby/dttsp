/** 
* @file wscompand.c
* @brief Functions to control the WS compander 
* @author Frank Brickle, AB2KT and Bob McGwier, N4HY 

 waveshaping compander, mostly for speech

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

#include <wscompand.h>

/* -------------------------------------------------------------------------- */
/** @brief Lookup values in the WS compander 
* 
* @param wsc 
* @param x 
* @return REAL 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE INLINE REAL
WSCLookup(WSCompander wsc, REAL x) {
  if (x > 0.0) {
    REAL d, xn, y, *tbl;
    int end, i;
    
    xn = x * wsc->npts, i = xn, d = xn - i;
    tbl = wsc->tbl, end = wsc->nend;
    
    if (i < end)
      y = tbl[i] + d * (tbl[i + 1] - tbl[i]);
    else
      y = tbl[end];
    return y / x;
  } else
    return 0.0;
}

/* -------------------------------------------------------------------------- */
/** @brief Run the WS compander 
* 
* @param wsc 
* @return void 
*/
/* ---------------------------------------------------------------------------- */
void
WSCompand(WSCompander wsc) {
  int i, n = CXBhave(wsc->buff);

  for (i = 0; i < n; i++) {
    COMPLEX val = CXBdata(wsc->buff, i);
    REAL mag = Cmag(val), scl = WSCLookup(wsc, mag);
    CXBdata(wsc->buff, i) = Cscl(val, scl);
  }
}

/* -------------------------------------------------------------------------- */
/** @brief Reset the WS compander 
* 
* @param wsc 
* @param fac 
* @return void 
*/
/* ---------------------------------------------------------------------------- */
void
WSCReset(WSCompander wsc, REAL fac) {
  int i;
  REAL *tbl = wsc->tbl;

  if (fac == 0.0)		/// just linear
    for (i = 0; i < wsc->npts; i++)
      tbl[i] = i / (REAL) wsc->nend;

  else {			/// exponential
    REAL del = fac / wsc->nend, scl = 1.0 - exp(fac);
    for (i = 0; i < wsc->npts; i++)
      tbl[i] = (1.0 - exp(i * del)) / scl;
  }
  wsc->fac = fac;
}

/// fac < 0: compression
/// fac > 0: expansion

/* -------------------------------------------------------------------------- */
/** @brief create a new WS Compander 
* 
* @param npts 
* @param fac 
* @param buff 
* @return WSCompander 
*/
/* ---------------------------------------------------------------------------- */
WSCompander
newWSCompander(int npts, REAL fac, CXB buff) {
  WSCompander wsc;

  wsc = (WSCompander) safealloc(1,
				sizeof(WSCompanderInfo),
				"WSCompander struct");
  wsc->npts = npts;
  wsc->nend = npts - 1;
  wsc->tbl = newvec_REAL(npts, "WSCompander table");
  wsc->buff = newCXB(CXBsize(buff), CXBbase(buff), "WSCompander buff");
  WSCReset(wsc, fac);
  return wsc;
}

/* -------------------------------------------------------------------------- */
/** @brief destroy a WS compander 
* 
* @param wsc 
* @return void 
*/
/* ---------------------------------------------------------------------------- */
void
delWSCompander(WSCompander wsc) {
  if (wsc) {
    delvec_REAL(wsc->tbl);
    delCXB(wsc->buff);
    safefree((char *) wsc);
  }
}
