/** 
* @file spectrum.c
* @brief Functions to take a snapshot of the spectrum 
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

#include <spectrum.h>

/* -------------------------------------------------------------------------- */
/** @brief Function to take a stapshot of the spectrum 
* 
* snapshot of current signal
*
* @param sb 
* @param label 
* @param stamp 
* @return void
*/
/* ---------------------------------------------------------------------------- */
void
snap_spectrum(SpecBlock *sb, int label, int stamp) {
  int i, j;

  // where most recent signal started
  j = sb->fill;

  // copy starting from there in circular fashion,
  // applying window as we go
  if (!sb->polyphase) {
    for (i = 0; i < sb->size; i++) {
      CXBdata(sb->timebuf, i) = Cscl(CXBdata(sb->accum, j), sb->window[i]);
      j = (++j & sb->mask);
    }
  } else {
    int k;
    for (i = 0; i < sb->size; i++) {
      CXBreal(sb->timebuf, i) = CXBreal(sb->accum, j) * sb->window[i];
      CXBimag(sb->timebuf, i) = CXBimag(sb->accum, j) * sb->window[i];
      for (k = 1; k < 8; k++) {
	int accumidx = (j + k * sb->size) & sb->mask,
	    winidx = i + k * sb->size;
	CXBreal(sb->timebuf, i) +=
	  CXBreal(sb->accum, accumidx) * sb->window[winidx];
	CXBimag(sb->timebuf, i) +=
	  CXBimag(sb->accum, accumidx) * sb->window[winidx];
      }
      j = ++j & sb->mask;
    }

  }
  sb->label = label;
  sb->stamp = stamp;
  sb->last = SPEC_LAST_FREQ;
}

/* -------------------------------------------------------------------------- */
/** @brief cwFunction to take a snapshot of the oscilloscope 
* 
* @param sb 
* @param label 
* @param stamp 
* @return void
*/
/* ---------------------------------------------------------------------------- */
void
snap_scope(SpecBlock *sb, int label, int stamp) {
  int i, j;

  // where most recent signal started
  j = sb->fill;

  // copy starting from there in circular fashion
  for (i = 0; i < sb->size; i++) {
    sb->oscope[i] = CXBreal(sb->accum, j);
    j = (++j & sb->mask);
  }

  sb->label = label;
  sb->stamp = stamp;
  sb->last = SPEC_LAST_TIME;
}

/* -------------------------------------------------------------------------- */
/** @brief Compute the spectrum block 
* 
* snapshot -> frequency domain
*
* @param sb 
* @return void
*/
/* ---------------------------------------------------------------------------- */
void
compute_spectrum(SpecBlock *sb) {
  int i, j, half = sb->size / 2;

  // assume timebuf has windowed current snapshot

  fftwf_execute(sb->plan);

  if (sb->scale == SPEC_MAG) {
    for (i = 0, j = half; i < half; i++, j++) {
      sb->output[i] = Cmag(CXBdata(sb->freqbuf, j));
      sb->output[j] = Cmag(CXBdata(sb->freqbuf, i));
    }
  } else {			// SPEC_PWR
    for (i = 0, j = half; i < half; i++, j++) {
      sb->output[i] = Log10P(Csqrmag(CXBdata(sb->freqbuf, j)));
      sb->output[j] = Log10P(Csqrmag(CXBdata(sb->freqbuf, i)));
    }
  }
}

/* -------------------------------------------------------------------------- */
/** @brief Initialize the spectrum block 
* 
* @param sb 
* @return void
*/
/* ---------------------------------------------------------------------------- */
void
init_spectrum(SpecBlock *sb) {
  sb->fill = 0;
  sb->accum = newCXB(sb->size * 16, 0, "spectrum accum");
  sb->timebuf = newCXB(sb->size, 0, "spectrum timebuf");
  sb->freqbuf = newCXB(sb->size, 0, "spectrum freqbuf");
  sb->oscope = newvec_REAL(sb->size, "scope vec");
  sb->window = newvec_REAL(sb->size * 16, "spectrum window");
  makewindow(BLACKMANHARRIS_WINDOW, sb->size, sb->window);
  sb->mask = sb->size - 1;
  sb->polyphase = FALSE;
  sb->output = (float *) safealloc(sb->size, sizeof(float), "spectrum output");
  sb->plan = fftwf_plan_dft_1d(sb->size,
			       (fftwf_complex *) CXBbase(sb->timebuf),
			       (fftwf_complex *) CXBbase(sb->freqbuf),
			       FFTW_FORWARD, sb->planbits);
}

/* -------------------------------------------------------------------------- */
/** @brief Reinitialize the spectrum block 
* 
* @param sb 
* @return void
*/
/* ---------------------------------------------------------------------------- */
void
reinit_spectrum(SpecBlock *sb) {
  size_t polysize = 1;
  sb->fill = 0;
  if (sb->polyphase)
    polysize = 8;
  memset((char *) CXBbase(sb->accum), 0, polysize * sb->size * sizeof(REAL));
  memset((char *) sb->output, 0, sb->size * sizeof(float));
}

/* -------------------------------------------------------------------------- */
/** @brief Finish and cleanup the spectrum block 
* 
* @param sb 
* @return void
*/
/* ---------------------------------------------------------------------------- */
void
finish_spectrum(SpecBlock *sb) {
  if (sb) {
    delCXB(sb->accum);
    delCXB(sb->timebuf);
    delCXB(sb->freqbuf);
    delvec_REAL(sb->oscope);
    delvec_REAL(sb->window);
    safefree((char *) sb->output);
    fftwf_destroy_plan(sb->plan);
  }
}
