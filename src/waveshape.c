// waveshape.c
// bipolar waveshaping
/*
This file is part of a program that implements a Software-Defined Radio.

Copyright (C) 2004, 2005, 2006, 2007, 2008 by Frank Brickle, AB2KT and Bob McGwier, N4HY

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

#include <waveshape.h>

// Sample is assumed to be (0+jx),
//  ie, a real signal with signal in Q.
// Waveshaping function is initialized at nil,
//  so default result is identity.  
// Original table should have odd length.
// Table *as stored* is extended by 1 point
//  with duplicate of last point to simplify
//  interpolation logic.
//  So wvs->tbl is actually wvs->npts+1 long,
//   but wvs->npts represents original length.

void
WaveShape(WaveShaper wvs) {
  REAL *tbl = wvs->tbl;
  if (tbl) {
    int i,
        half = wvs->npts / 2,
        n = CXBhave(wvs->buff);
    
    for (i = 0; i < n; i++) {
      int j;
      REAL d, x, xn, y;
      
      x = CXBreal(wvs->buff, i);
      if (x < -1.0) x = -1.0;
      if (x > +1.0) x = +1.0;
      
      xn = half * (x + 1.0),
      j = xn,
      d = xn - j,
      y = tbl[j] + d * (tbl[j + 1] - tbl[j]);
      
      CXBreal(wvs->buff, i) = y;
    }
  }
}

void
setWaveShaper(WaveShaper wvs, int npts, REAL *tbl) {
  if (npts < 1) {
    if (wvs->tbl)
      delvec_REAL(wvs->tbl);
    wvs->npts = 0;
  } else {
    if (wvs->tbl)
      delvec_REAL(wvs->tbl);
    wvs->tbl = newvec_REAL(npts + 1, "setWaveShaper table");
    memcpy((char *) wvs->tbl, (char *) tbl, npts * sizeof(REAL));
    wvs->tbl[npts] = wvs->tbl[npts - 1];
    wvs->npts = npts;
  }
}

WaveShaper
newWaveShaper(CXB buff) {
  WaveShaper wvs;

  wvs = (WaveShaper) safealloc(1,
			       sizeof(WaveShaper),
			       "WaveShaper struct");
  wvs->npts = 0;
  wvs->tbl = 0;
  wvs->buff = newCXB(CXBsize(buff), CXBbase(buff), "WaveShaper buff");
  return wvs;
}

void
delWaveShaper(WaveShaper wvs) {
  if (wvs) {
    if (wvs->tbl)
      delvec_REAL(wvs->tbl);
    delCXB(wvs->buff);
    safefree((char *) wvs);
  }
}
