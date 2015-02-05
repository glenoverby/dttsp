/** 
* @file dttspagc.c
* @brief Functions to implement automatic gain control  
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

#include <dttspagc.h>

#ifdef min
#undef min
#endif

/* -------------------------------------------------------------------------- */
/** @brief min 
* 
* @param a 
* @param b 
*/
/* ---------------------------------------------------------------------------- */
static INLINE REAL
min(REAL a, REAL b) { return a < b ? a : b; }

#ifdef max
#undef max
#endif

/* -------------------------------------------------------------------------- */
/** @brief 
* 
* @param a 
* @param b 
*/
/* ---------------------------------------------------------------------------- */
static INLINE REAL
max(REAL a, REAL b) { return a > b ? a : b; }

/* -------------------------------------------------------------------------- */
/** @brief DttSPAgc 
* 
* @param mode 
* @param Vec 
* @param BufSize 
* @param Limit 
* @param attack 
* @param decay 
* @param slope 
* @param hangtime 
* @param samprate 
* @param MaxGain 
* @param MinGain 
* @param CurGain 
* @param tag 
*/
/* ---------------------------------------------------------------------------- */
DTTSPAGC
newDttSPAgc(AGCMODE mode,
	    COMPLEX *Vec,
	    int BufSize,
	    REAL Limit,
	    REAL attack,
	    REAL decay,
	    REAL slope,
	    REAL hangtime,
	    REAL samprate,
	    REAL MaxGain,
	    REAL MinGain,
	    REAL CurGain,
	    char *tag) {
  DTTSPAGC a;

  a = (DTTSPAGC) safealloc(1, sizeof(dttspagc), tag);
  a->mode = mode;

  a->attack = (REAL) (1.0 - exp(-1000.0 / (attack * samprate)));
  a->one_m_attack = (REAL) exp(-1000.0 / (attack * samprate));

  a->decay = (REAL) (1.0 - exp(-1000.0 / (decay * samprate)));
  a->one_m_decay = (REAL) exp(-1000.0 / (decay * samprate));

  a->fastattack = (REAL) (1.0 - exp(-1000.0 / (0.2 * samprate)));
  a->one_m_fastattack = (REAL) exp(-1000.0 / (0.2 * samprate));

  a->fastdecay = (REAL) (1.0 - exp(-1000.0 / (3.0 * samprate)));
  a->one_m_fastdecay = (REAL) exp(-1000.0 / (3.0 * samprate));

  strcpy(a->tag, tag);
  a->mask = 2 * BufSize;

  a->hangindex = a->indx = 0;
  a->hangtime = hangtime * 0.001;
  a->hangthresh = 0.0;
  a->sndx = (int) (samprate * attack * 0.003);
  a->fastindx = FASTLEAD;
  a->gain.fix = 10.0;

  a->slope = slope;
  a->gain.top = MaxGain;
  a->hangthresh = a->gain.bottom = MinGain;
  a->gain.fastnow = a->gain.old = a->gain.now = CurGain;

  a->gain.limit = Limit;

  a->buff = newCXB(BufSize, Vec, "agc in buffer");
  a->circ = newvec_COMPLEX(a->mask, "circular agc buffer");
  a->mask -= 1;

  a->fasthang = 0;
  a->fasthangtime = 48 * 0.001;
  a->samprate = samprate;

  return a;
}

/* -------------------------------------------------------------------------- */
/** @brief DttSPAgc 
* 
* @param a 
* @param tick 
*/
/* ---------------------------------------------------------------------------- */
void
DttSPAgc(DTTSPAGC a, int tick) {
  int i,
      hangtime = (int) (a->samprate * a->hangtime),
      fasthangtime = (int) (a->samprate * a->fasthangtime);
  REAL hangthresh;

  if (a->hangthresh > 0)
    hangthresh =
      a->gain.top * a->hangthresh +
      a->gain.bottom * (REAL) (1.0 - a->hangthresh);
  else
    hangthresh = 0.0;

  if (a->mode == 0) {
#ifdef __SSE3__
    SSEScaleCOMPLEX(a->buff, a->buff, a->gain.fix, CXBhave(a->buff));
#else
    for (i = 0; i < CXBhave(a->buff); i++)
      CXBdata(a->buff, i) = Cscl(CXBdata(a->buff, i), a->gain.fix);
#endif
    return;
  }

  for (i = 0; i < CXBhave(a->buff); i++) {
    REAL tmp;

    a->circ[a->indx] = CXBdata(a->buff, i);	/* Drop sample into circular buffer */
    tmp = 1.1 * Cmag(a->circ[a->indx]);

    if (tmp != 0.0)
      tmp = a->gain.limit / tmp;	// if not zero sample, calculate gain
    else
      tmp = a->gain.now;	// update. If zero, then use old gain

    if (tmp < hangthresh)
      a->hangindex = hangtime;

    if (tmp >= a->gain.now) {
      a->gain.raw = a->one_m_decay * a->gain.now + a->decay * tmp;
      if (a->hangindex++ > hangtime)
	a->gain.now =
	  a->one_m_decay * a->gain.now + a->decay * min(a->gain.top, tmp);
    } else {
      a->hangindex = 0;
      a->gain.raw = a->one_m_attack * a->gain.now + a->attack * tmp;
      a->gain.now =
	a->one_m_attack * a->gain.now + a->attack * max(tmp, a->gain.bottom);
    }

    tmp = 1.2f * Cmag(a->circ[a->fastindx]);
    if (tmp != 0.0)
      tmp = a->gain.limit / tmp;
    else
      tmp = a->gain.fastnow;

    if (tmp > a->gain.fastnow) {
      if (a->fasthang++ > fasthangtime) {
	a->gain.fastnow =
	  min(a->one_m_fastdecay * a->gain.fastnow +
	      a->fastdecay * min(a->gain.top, tmp), a->gain.top);
      }
    } else {
      a->fasthang = 0;
      a->gain.fastnow =
	max(a->one_m_fastattack * a->gain.fastnow +
	    a->fastattack * max(tmp, a->gain.bottom),
	    a->gain.bottom);
    }

    a->gain.fastnow = max(min(a->gain.fastnow, a->gain.top), a->gain.bottom);
    a->gain.now = max(min(a->gain.now, a->gain.top), a->gain.bottom);
    CXBdata(a->buff, i) =
      Cscl(a->circ[a->sndx], min(a->gain.fastnow,
				 min(a->slope * a->gain.now, a->gain.top)));

    a->indx = (a->indx + a->mask) & a->mask;
    a->sndx = (a->sndx + a->mask) & a->mask;
    a->fastindx = (a->fastindx + a->mask) & a->mask;
  }
}

/* -------------------------------------------------------------------------- */
/** @brief delDttSPAgc 
* 
* @param a 
*/
/* ---------------------------------------------------------------------------- */
void
delDttSPAgc(DTTSPAGC a) {
  if (a) {
    delCXB(a->buff);
    delvec_COMPLEX(a->circ);
    safefree((char *) a);
  }
}

#if 0
gain = 1e-3;
spoint = 1.0;
for (;;) {
  /* Voltage controlled amplifier is just a multiplier here */
  yout = yin * iout;

  /* error */
  err = spoint - abs(yout);

  /* Integrate */
  iout1 = iout;
  iout = iout1 + gain * err;
}

if (signal_too_big())
  decrease_gain_quickly();
else if (signal_below_threshold() && gain_not_too_high_already())
  increase_gain_slowly();

#endif
