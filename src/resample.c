/** 
* @file resample.c
* @brief Resampling functions 
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

#include <resample.h>

/* -------------------------------------------------------------------------- */
/** @brief New Polyphase FIR filter
* 
* @param source 
* @param insize 
* @param dest 
* @param iold 
* @param intrp 
* @param phase 
* @param decim 
* @return ResSt
*/
/* ---------------------------------------------------------------------------- */
ResSt
newPolyPhaseFIR(CXB source,
		int insize,
		CXB dest,
		int iold,
		int intrp,
		int phase,
		int decim) {
  ResSt r = (ResSt) safealloc(1, sizeof(resampler), "PF Resampler");

  r->iold  = iold;
  r->intrp = intrp;
  r->phase = phase;
  r->decim = decim;
  r->nflt  = 31 * decim;
  r->nold  = nblock2(r->nflt);
  r->mask  = r->nold - 1;
  r->inp   = CXBbase(source);
  r->nnew  = insize;
  r->out   = CXBbase(dest);
  r->old   = (COMPLEX *) safealloc(r->nold, sizeof(COMPLEX), "resampler past");
  r->filt  = newFIR_Lowpass_REAL(0.45f / (REAL) decim, (REAL) intrp, r->nflt);

  return r;
}

/* -------------------------------------------------------------------------- */
/** @brief Destroy a Polypahse FIR filter 
* 
* @param r 
* @return void
*/
/* ---------------------------------------------------------------------------- */
void
delPolyPhaseFIR(ResSt r) {
  if (r)
    delFIR_Lowpass_REAL(r->filt),
    safefree((char *) r->old),
    safefree((char *) r);
}

/* -------------------------------------------------------------------------- */
/** @brief PolyPhase FIR filter 
* 
* @param r 
* @return void
*/
/* ---------------------------------------------------------------------------- */
void
PolyPhaseFIR(ResSt r) {
  int i, n = 0;

  for (i = 0; i < r->nnew; i++) {

    r->old[r->iold] = r->inp[i];

    while (r->phase < r->intrp) {
      COMPLEX sum = cxzero;
      int j = r->iold, k;

      for (k = r->phase; k < r->nflt; k += r->intrp) {
	sum = Cadd(sum, Cscl(r->old[j], FIRtap(r->filt, k)));
	j = (j + r->mask) & r->mask;
      }

      r->out[n] = Cscl(sum, (REAL) r->intrp);
      r->phase += r->decim;
      n++;
    }

    r->iold = (r->iold + 1) & r->mask;
    r->phase -= r->intrp;
  }

  r->nout = n;
}

/** 
* *inp              pointer to inp COMPLEX data array
* *out              pointer to out COMPLEX data array
* filt              pointer to filt coefficients array
* *old              pointer to buffer used as filt memory. Initialized
*                   all data to 0 before 1st call.  length is calculated
*                   from nflt
* nold              length of old
* nnew              length of inp array :note that "out" may differ in length
* nflt              number of filt taps in array "filtcoeff": < nold
* iold              index to where next inp sample is to be stored in
*                   "old",initalized 0 to before first call
* intrp             interpolation factor: out rate = inp rate * interp / decim.
* phase             filt phase number (index), initialized to 0
* decim             decimation factor:
*                   out rate = (inp rate * interp/decim)
* nout              number of out samples placed in array "out"
*
* DESCRIPTION: This function is used to change the sampling rate of the data.
*              The inp is first upsampled to a multiple of the desired
*              sampling rate and then down sampled to the desired sampling rate.
*
*              Ex. If we desire a 7200 Hz sampling rate for a signal that has
*                  been sampled at 8000 Hz the signal can first be upsampled
*                  by a factor of 9 which brings it up to 72000 Hz and then
*                  down sampled by a factor of 10 which results in a sampling
*                  rate of 7200 Hz.
*
* NOTES:
*        Also, the "*old" MUST be 2^N REALs long. This
*        routine uses circular addressing on this buffer assuming that
*        it is 2^N REALs in length.
*/
