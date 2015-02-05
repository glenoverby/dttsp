/** 
* @file spottone.c
* @brief Functions to gerenate spot tones 
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

#include <spottone.h>

/* -------------------------------------------------------------------------- */
/** @brief Function to generate a spot tone 
* 
*  An ASR envelope on a complex phasor,
*  with asynchronous trigger for R stage.
*  A/R use sine shaping.
* @param st 
*/
/* ---------------------------------------------------------------------------- */
BOOLEAN
SpotTone(SpotToneGen st) {
  int i, n = st->size;

  ComplexOSC(st->osc.gen);

  for (i = 0; i < n; i++) {

    // in an envelope stage?

    if (st->stage == SpotTone_RISE) {

      // still going?
      if (st->rise.have++ < st->rise.want) {
	st->curr += st->rise.incr;
	st->mul = (REAL) (st->scl * sin(st->curr * M_PI / 2.0));
      } else {
	// no, assert steady-state, force level
	st->curr = 1.0;
	st->mul = st->scl;
	st->stage = SpotTone_STDY;
	// won't come back into envelopes
	// until FALL asserted from outside
      }

    } else if (st->stage == SpotTone_FALL) {

      // still going?
      if (st->fall.have++ < st->fall.want) {
	st->curr -= st->fall.incr;
	st->mul = (REAL) (st->scl * sin(st->curr * M_PI / 2.0));
      } else {
	// no, assert trailing, force level
	st->curr = 0.0;
	st->mul = 0.0;
	st->stage = SpotTone_HOLD;
	// won't come back into envelopes hereafter
      }
    }
    // apply envelope
    // (same base as osc.gen internal buf)
    CXBdata(st->buf, i) = Cscl(CXBdata(st->buf, i), st->mul);
  }

  // indicate whether it's turned itself off
  // sometime during this pass

  return st->stage != SpotTone_HOLD;
}


/* -------------------------------------------------------------------------- */
/** @brief Turn on spot tone 
* 
* turn spotting on with current settings
*
* @param st 
*/
/* ---------------------------------------------------------------------------- */
void
SpotToneOn(SpotToneGen st) {

  // gain is in dB

  st->scl = (REAL) pow(10.0, st->gain / 20.0);
  st->curr = st->mul = 0.0;

  // A/R times are in msec

  st->rise.want = (int) (0.5 + st->sr * (st->rise.dur / 1e3));
  st->rise.have = 0;
  if (st->rise.want <= 1)
    st->rise.incr = 1.0;
  else
    st->rise.incr = (REAL) (1.0 / (st->rise.want - 1));

  st->fall.want = (int) (0.5 + st->sr * (st->fall.dur / 1e3));
  st->fall.have = 0;
  if (st->fall.want <= 1)
    st->fall.incr = 1.0;
  else
    st->fall.incr = (REAL) (1.0 / (st->fall.want - 1));

  // freq is in Hz

  OSCfreq(st->osc.gen) = 2.0 * M_PI * st->osc.freq / st->sr;
  OSCphase(st->osc.gen) = 0.0;

  st->stage = SpotTone_RISE;
}

//------------------------------------------------------------------------

/* -------------------------------------------------------------------------- */
/** @brief Turn off spot tone
* 
* initiate turn-off
*
* @param st 
*/
/* ---------------------------------------------------------------------------- */
void
SpotToneOff(SpotToneGen st) {
  st->stage = SpotTone_FALL;
}

//------------------------------------------------------------------------

/* -------------------------------------------------------------------------- */
/** @brief Set the spot tone general values 
* 
* @param st 
* @param gain 
* @param freq 
* @param rise 
* @param fall 
*/
/* ---------------------------------------------------------------------------- */
void
setSpotToneGenVals(SpotToneGen st, REAL gain, REAL freq, REAL rise, REAL fall) {
  st->gain = gain;
  st->osc.freq = freq;
  st->rise.dur = rise;
  st->fall.dur = fall;
}

/* -------------------------------------------------------------------------- */
/** @brief Create a new spot tone with values 
* 
* @param gain dB 
* @param freq ms 
* @param rise ms 
* @param fall ms 
* @param size 
* @param samplerate 
*/
/* ---------------------------------------------------------------------------- */
SpotToneGen
newSpotToneGen(REAL gain,	// dB
	       REAL freq, REAL rise,	// ms
	       REAL fall,	// ms
	       int size, REAL samplerate) {

  SpotToneGen st = (SpotToneGen) safealloc(1,
					   sizeof(SpotToneGenDesc),
					   "SpotToneGenDesc");

  setSpotToneGenVals(st, gain, freq, rise, fall);
  st->size = size;
  st->sr = samplerate;

  st->osc.gen = newOSC(st->size,
		       ComplexTone,
		       st->osc.freq, 0.0, st->sr, "SpotTone osc");

  // overload oscillator buf
  st->buf = newCXB(st->size, OSCCbase(st->osc.gen), "SpotToneGen buf");

  return st;
}

/* -------------------------------------------------------------------------- */
/** @brief Delete spot tone generation structures 
* 
* @param st 
*/
/* ---------------------------------------------------------------------------- */
void
delSpotToneGen(SpotToneGen st) {
  if (st) {
    delCXB(st->buf);
    delOSC(st->osc.gen);
    safefree((char *) st);
  }
}
