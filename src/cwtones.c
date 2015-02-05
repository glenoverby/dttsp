/** 
* @file cwtones.c
* @brief Functions to implement cw tones 
* @author Frank Brickle, AB2KT and Bob McGwier, N4HY

This file is part of a program that implements a Software-Defined Radio.

Copyright (C) 2005, 2006, 2007 by Frank Brickle, AB2KT and Bob McGwier, N4HY
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

#include <cwtones.h>

/// ------------------------------------------------------------------------
///  An ASR envelope on a complex phasor,
///  with asynchronous trigger for R stage.
///  A/R use sine shaping.
/// ------------------------------------------------------------------------

/* -------------------------------------------------------------------------- */
/** @brief Run CWTone 
* 
* @param cwt 
*/
/* ---------------------------------------------------------------------------- */
BOOLEAN
CWTone(CWToneGen cwt) {
  int i, n = cwt->size;

  ComplexOSC(cwt->osc.gen);

  for (i = 0; i < n; i++) {

    // in an envelope stage?

    if (cwt->stage == CWTone_RISE) {

      // still going?
      if (cwt->rise.have++ < cwt->rise.want) {
	cwt->curr += cwt->rise.incr;
	cwt->mul = cwt->scl * sin(cwt->curr * M_PI / 2.0);
      } else {
	// no, assert steady-state, force level
	cwt->curr = 1.0;
	cwt->mul = cwt->scl;
	cwt->stage = CWTone_STDY;
	// won't come back into envelopes
	// until FALL asserted from outside
      }

    } else if (cwt->stage == CWTone_FALL) {

      // still going?
      if (cwt->fall.have++ < cwt->fall.want) {
	cwt->curr -= cwt->fall.incr;
	cwt->mul = cwt->scl * sin(cwt->curr * M_PI / 2.0);
      } else {
	// no, assert trailing, force level
	cwt->curr = 0.0;
	cwt->mul = 0.0;
	cwt->stage = CWTone_HOLD;
	// won't come back into envelopes hereafter
      }
    }
    ///  apply envelope
    // (same base as osc.gen internal buf)
    CXBdata(cwt->buf, i) = Cscl(CXBdata(cwt->buf, i), cwt->mul);
  }

  ///  indicate whether it's turned itself off
  ///  sometime during this pass

  return cwt->stage != CWTone_HOLD;
}

/// ------------------------------------------------------------------------
///  turn tone on with current settings

/* -------------------------------------------------------------------------- */
/** @brief Turn CW Tone On 
* 
* @param cwt 
*/
/* ---------------------------------------------------------------------------- */
void
CWToneOn(CWToneGen cwt) {

  ///  gain is in dB

  cwt->scl = pow(10.0, cwt->gain / 20.0);
  cwt->curr = cwt->mul = 0.0;

  ///  A/R times are in msec

  cwt->rise.want = (int) (0.5 + cwt->sr * (cwt->rise.dur / 1e3));
  cwt->rise.have = 0;
  if (cwt->rise.want <= 1)
    cwt->rise.incr = 1.0;
  else
    cwt->rise.incr = 1.0f / (cwt->rise.want - 1);

  cwt->fall.want = (int) (0.5 + cwt->sr * (cwt->fall.dur / 1e3));
  cwt->fall.have = 0;
  if (cwt->fall.want <= 1)
    cwt->fall.incr = 1.0;
  else
    cwt->fall.incr = 1.0f / (cwt->fall.want - 1);

  ///  freq is in Hz

  OSCfreq(cwt->osc.gen) = 2.0 * M_PI * cwt->osc.freq / cwt->sr;
  OSCphase(cwt->osc.gen) = 0.0;

  cwt->stage = CWTone_RISE;
}

/// ------------------------------------------------------------------------
/// initiate turn-off

/* -------------------------------------------------------------------------- */
/** @brief Turn CW Tone Off 
* 
* @param cwt 
*/
/* ---------------------------------------------------------------------------- */
void
CWToneOff(CWToneGen cwt) {
  cwt->stage = CWTone_FALL;
}

//------------------------------------------------------------------------

/* -------------------------------------------------------------------------- */
/** @brief set CW Tone General Values 
* 
* @param cwt 
* @param gain 
* @param freq 
* @param rise 
* @param fall 
*/
/* ---------------------------------------------------------------------------- */
void
setCWToneGenVals(CWToneGen cwt, REAL gain, REAL freq, REAL rise, REAL fall) {
  cwt->gain = gain;
  cwt->osc.freq = freq;
  cwt->rise.dur = rise;
  cwt->fall.dur = fall;
}

/* -------------------------------------------------------------------------- */
/** @brief Create new CW Tone General 
* @param gain 
* @param freq 
* @param rise 
* @param fall 
* @param size 
* @param samplerate 
*/
/* ---------------------------------------------------------------------------- */
CWToneGen
newCWToneGen(REAL gain,	// dB
	     REAL freq, // Hz
             REAL rise,	// ms
	     REAL fall,	// ms
	     int size, 
             REAL samplerate) {

  CWToneGen cwt = (CWToneGen) safealloc(1, sizeof(CWToneGenDesc),
					"CWToneGenDesc");

  setCWToneGenVals(cwt, gain, freq, rise, fall);
  cwt->size = size;
  cwt->sr = samplerate;

  cwt->osc.gen = newOSC(cwt->size,
			ComplexTone,
			(double) cwt->osc.freq, 0.0, cwt->sr, "CWTone osc");

  /// overload oscillator buf
  cwt->buf = newCXB(cwt->size, OSCCbase(cwt->osc.gen), "CWToneGen buf");

  return cwt;
}

/* -------------------------------------------------------------------------- */
/** @brief Create delete CW Tone General 
a 
* @param cwt 
*/
/* ---------------------------------------------------------------------------- */
void
delCWToneGen(CWToneGen cwt) {
  if (cwt) {
    delCXB(cwt->buf);
    delOSC(cwt->osc.gen);
    safefree((char *) cwt);
  }
}
