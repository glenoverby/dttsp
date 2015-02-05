/** 
* @file stkstuff.c
* @brief DSP algorithms inspired by Stk. From-scratch in C.
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

#include <stkstuff.h>

//------------------------------------------------------------------

/* -------------------------------------------------------------------------- */
/** @brief  Create a new STK Filter structure 
* 
* @param b 
* @param nb 
* @param a 
* @param na 
* @return STKFilt
*/
/* ---------------------------------------------------------------------------- */
STKFilt
newSTKFilt(REAL *b, int nb, REAL *a, int na) {
  STKFilt f = (STKFilt) safealloc(1, sizeof(STKFiltInfo), "STKFiltInfo");
  f->fil.g = 1.0;
  f->fil.b = newRLB(nb, 0, "STKFiltInfo fil.b");
  memcpy((char *) RLBbase(f->fil.b), (char *) b, nb * sizeof(REAL));
  f->fil.a = newRLB(na, 0, "STKFiltInfo fil.a");
  memcpy((char *) RLBbase(f->fil.a), (char *) a, na * sizeof(REAL));
  f->fil.x = newRLB(nb, 0, "STKFiltInfo fil.x");
  f->fil.y = newRLB(na, 0, "STKFiltInfo fil.y");
  return f;
}

/* -------------------------------------------------------------------------- */
/** @brief Destroy a STK Filter structure 
* 
* @param f -- the STK structure  
* @return void
*/
/* ---------------------------------------------------------------------------- */
void
delSTKFilt(STKFilt f) {
  if (f) {
    delRLB(f->fil.b);
    delRLB(f->fil.a);
    delRLB(f->fil.x);
    delRLB(f->fil.y);
    safefree((char *) f);
  }
}

/* -------------------------------------------------------------------------- */
/** @brief Clear the STK filter structure 
* 
* @param f 
* @return void
*/
/* ---------------------------------------------------------------------------- */
void
STKFilt_clear(STKFilt f) {
  memset((char *) RLBbase(f->fil.x), 0, RLBsize(f->fil.x) * sizeof(REAL));
  memset((char *) RLBbase(f->fil.y), 0, RLBsize(f->fil.y) * sizeof(REAL));
}

/* -------------------------------------------------------------------------- */
/** @brief Set the filter coefficients in a STK filter structure 
* 
* @param f -- stk filter structure 
* @param b 
* @param nb 
* @param a 
* @param na 
* @param clearstate 
* @return void
*/
/* ---------------------------------------------------------------------------- */
void
STKFilt_set_coeffs(STKFilt f,
		   REAL *b, int nb, REAL *a, int na,
		   BOOLEAN clearstate) {
  if (nb != RLBsize(f->fil.b)) {
    delRLB(f->fil.b);
    f->fil.b = newRLB(nb, 0, "STKFiltInfo fil.b");
    memcpy((char *) RLBbase(f->fil.b), (char *) b, nb * sizeof(REAL));
  }
  if (clearstate)
    memset((char *) RLBbase(f->fil.x), 0, RLBsize(f->fil.x) * sizeof(REAL));

  if (na != RLBsize(f->fil.a)) {
    delRLB(f->fil.a);
    f->fil.a = newRLB(na, 0, "STKFiltInfo fil.a");
    memcpy((char *) RLBbase(f->fil.a), (char *) a, na * sizeof(REAL));
  }
  if (clearstate)
    memset((char *) RLBbase(f->fil.y), 0, RLBsize(f->fil.y) * sizeof(REAL));
}

/* -------------------------------------------------------------------------- */
/** @brief Set the numerator in a STK filter structure
* 
* @param f 
* @param b 
* @param nb 
* @param clearstate 
* @return void
*/
/* ---------------------------------------------------------------------------- */
void
STKFilt_set_numerator(STKFilt f,
		      REAL *b, int nb,
		      BOOLEAN clearstate) {
  if (nb != RLBsize(f->fil.b)) {
    delRLB(f->fil.b);
    f->fil.b = newRLB(nb, 0, "STKFiltInfo fil.b");
    memcpy((char *) RLBbase(f->fil.b), (char *) b, nb * sizeof(REAL));
  }
  if (clearstate)
    memset((char *) RLBbase(f->fil.x), 0, RLBsize(f->fil.x) * sizeof(REAL));
}

/* -------------------------------------------------------------------------- */
/** @brief Set the demoniator in a STK filter structure
* 
* @param f 
* @param a 
* @param na 
* @param clearstate 
* @return void
*/
/* ---------------------------------------------------------------------------- */
void
STKFilt_set_denominator(STKFilt f,
			REAL *a, int na,
			BOOLEAN clearstate) {
  if (na != RLBsize(f->fil.a)) {
    delRLB(f->fil.a);
    f->fil.a = newRLB(na, 0, "STKFiltInfo fil.a");
    memcpy((char *) RLBbase(f->fil.a), (char *) a, na * sizeof(REAL));
  }
  if (clearstate)
    memset((char *) RLBbase(f->fil.y), 0, RLBsize(f->fil.y) * sizeof(REAL));
}

/* -------------------------------------------------------------------------- */
/** @brief set the STK Filter gain 
* 
* @param f 
* @param gain 
* @return void
*/
/* ---------------------------------------------------------------------------- */
void
STKFilt_set_gain(STKFilt f, REAL gain) {
  f->fil.g = gain;
}

/* -------------------------------------------------------------------------- */
/** @brief get the STK Filter gain 
* 
* @param f 
* @param gain 
* @return REAL
*/
/* ---------------------------------------------------------------------------- */
REAL
STKFilt_get_gain(STKFilt f, REAL gain) {
  return f->fil.g;
}

/* -------------------------------------------------------------------------- */
/** @brief get the last value out of the STK Filter 
* 
* @param f 
* @return REAL
*/
/* ---------------------------------------------------------------------------- */
REAL
STKFilt_last_out(STKFilt f) {
  return f->fil.y[0];
}

/* -------------------------------------------------------------------------- */
/** @brief set tick in STK Filter structure 
* 
* @param f 
* @param input 
* @return REAL
*/
/* ---------------------------------------------------------------------------- */
REAL
STKFilt_tick(STKFilt f, REAL input) {
  REAL
    g  = f->fil.g,
    *b = RLBbase(f->fil.b),
    *a = RLBbase(f->fil.a),
    *x = RLBbase(f->fil.x),
    *y = RLBbase(f->fil.y);
  int
    nb = RLBsize(f->fil.b),
    na = RLBsize(f->fil.a),
    i;

  y[0] = 0.0, x[0] = g * input;
  
  for (i = nb - 1; i > 0; --i)
    y[0] += b[i] * x[i], x[i] = x[i - 1];
  
  y[0] += b[0] * x[0];

  for (i = n1 - 1; i > 0; --i)
    y[0] += -a[i] * y[i], y[i] = y[i - 1];

  return y[0];
}

//==================================================================
/* -------------------------------------------------------------------------- */
/** @brief Create a new STK structure delay 
* 
* This "works" but it's a scruffy basic design.
* Contortions imposed by stretching the Filter
*   to cover the interpolating Delay Line cases...
* Another software engineering victory for C++.
* Fix this eventually. Yeah, right.
*
* @param delay 
* @param max_delay 
* @return STKDelay
*/
/* ---------------------------------------------------------------------------- */
STKDelay
newSTKDelay(int delay, int max_delay) {
  STKDelay d = (STKDelay) safealloc(1, sizeof(STKDelayInfo), "STKDelayInfo");
  d->fil.x = newRLB(max_delay + 1, 0, "STKDelayInfo fil.x");
  d->fil.y = newRLB(1, 0, "STKDelayInfo fil.y");
  d->del.t = delay;
  return d;
}

/* -------------------------------------------------------------------------- */
/** @brief Destroy the STK structure delay 
* 
* @param d 
* @return void
*/
/* ---------------------------------------------------------------------------- */
void
delSTKDelay(STKDelay d) {
  if (d) {
    delRLB(d->fil.x);
    delRLB(d->fil.y);
    // d->fil.? otherwise untouched. Slick, huh?
    safefree((char *) d);
  }
}

/* -------------------------------------------------------------------------- */
/** @brief Clear the STK structure delay 
* 
* @param d 
* @return void
*/
/* ---------------------------------------------------------------------------- */
void
STKDelay_clear(STKDelay d) {
  memset((char *) RLBbase(d->fil.x), 0, RLBsize(d->fil.x) * sizeof(REAL));
  RLBdata(d->fil.y, 0) = 0.0;
}

/* -------------------------------------------------------------------------- */
/** @brief Set the STK structure delay 
* 
* @param d 
* @param delay 
* @return void
*/
/* ---------------------------------------------------------------------------- */
void
STKDelay_set_delay(STKDelay d, int delay) {
  if (delay >= RLBsize(d->fil.x)) {
    if ((d->del.o = d->del.i + 1) == RLBsize(d->fil.x))
      d->del.o = 0;
    d->del.t = RLBsize(d->fil.x);
  } else if (delay < 0) {
    d->del.o = d->del.i;
    d->del.t = 0;
  } else {
    if (d->del.i >= delay)
      d->del.o = d->del.i - delay;
    else
      d->del.o = RLBsize(d->fil.x) + d->del.i - delay;
    d->del.t = delay;
  }
}

/* -------------------------------------------------------------------------- */
/** @brief Get the STK structure delay delay  
* 
* @param d 
* @return int
*/
/* ---------------------------------------------------------------------------- */
int
STKDelay_get_delay(STKDelay d) {
  return d->del.t;
}

/* -------------------------------------------------------------------------- */
/** @brief Set the STK structure delay maximum delay 
* 
* @param d 
* @param max_delay 
* @return void
*/
/* ---------------------------------------------------------------------------- */
void
STKDelay_set_max_delay(STKDelay d, int max_delay) {
  if ((d->del.t < max_delay) || (max_delay >= RLBsize(d->fil.x))) {
    RLB r = newRLB(max_delay + 1, 0, "STKDelay alter max");
    memcpy((char *) RLBbase(r),
	   (char *) RLBbase(d->fil.x),
	   min(RLBsize(r), RLBsize(d->fil.x)) * sizeof(REAL));
    delRLB(d->fil.x);
    d->fil.x = r;
  }
}

/* -------------------------------------------------------------------------- */
/** @brief Get the STK structure delay maximum delay 
* 
* @param d 
* @return int
*/
/* ---------------------------------------------------------------------------- */
int
STKDelay_get_max_delay(STKDelay d) {
  return RLBsize(d->fil.x) - 1;
}

/* -------------------------------------------------------------------------- */
/** @brief Calucalte the STK structure delay energy  
* 
* @param d 
* @return REAL
*/
/* ---------------------------------------------------------------------------- */
REAL
STKDelay_energy(STKDelay d) {
  REAL e = 0.0;
  int i;
  if (d->del.i >= d->del.o) {
    for (i = d->del.o; i < d->del.i; i++)
      e += RLBdata(d->fil.x, i) * RLBdata(d->fil.x, i);
  } else {
    for (i = d->del.o; i < inputs_.size(); i++)
      e += RLBdata(d->fil.x, i) * RLBdata(d->fil.x, i);
    for (i = 0; i < d->del.i; i++)
      e += RLBdata(d->fil.x, i) * RLBdata(d->fil.x, i);
  }
  return e;
}

/* -------------------------------------------------------------------------- */
/** @brief Get the STK structure delay for contents a at tap
* 
* @param d 
* @param tap 
* @return REAL
*/
/* ---------------------------------------------------------------------------- */
REAL
STKDelay_contents_at(STKDelay d, int tap) {
  if (0 < tap && tap <= d->del.t) {
    if ((tap = d->del.i - tap) < 0)
      tap += RLBsize(d->fil.x);
    return RLBdata(d->fil.x, tap);
  } else
    return 0.0;
}

/* -------------------------------------------------------------------------- */
/** @brief Return the last value out of STK structure delay
* 
* @param d 
* @return REAL
*/
/* ---------------------------------------------------------------------------- */
REAL
STKDelay_last_out(STKDelay d){
  return RLBdata(d->fil.y, 0);
}

/* -------------------------------------------------------------------------- */
/** @brief Return the nexxt value out of a STK structure delay 
* 
* @param d 
* @return REAL
*/
/* ---------------------------------------------------------------------------- */
REAL
STKDelay_next_out(STKDelay d) {
  return RLBdata(d->fil.x, d->del.o);
}

/* -------------------------------------------------------------------------- */
/** @brief Compute a sample from a STK structure delay 
* 
* @param d 
* @param input 
* @return REALyy
*/
/* ---------------------------------------------------------------------------- */
REAL
STKDelay_compute_sample(STKDelay d, REAL input) {
  RLBdata(d->fil.x, d->del.i) = input;
  if (++d->del.i >= RLBsize(d->fil.x)) d->del.i = 0;

  RLBdata(d->fil.y, 0) = RLBdata(d->fil.x, d->del.o);
  if (++d->del.o >= RLBsize(d->fil.x)) d->del.o = 0;

  return RLBdata(d->fil.y, 0);
}

/* -------------------------------------------------------------------------- */
/** @brief Return the sample from the STK structure delay based on d and input 
* 
* @param d 
* @param input 
* @return REAL
*/
/* ---------------------------------------------------------------------------- */
REAL
STKDelay_tick(STKDelay d, REAL input) {
  return STKDelay_compute_sample(d, input);
}

//------------------------------------------------------------------

/* -------------------------------------------------------------------------- */
/** @brief Create a new STK structure delay 
* 
* @param delay 
* @param max_delay 
* @return STKDelay
*/
/* ---------------------------------------------------------------------------- */
STKDelay
newSTKDelayA(REAL delay, int max_delay) {
  STKDelayA d = (STKDelayA) safealloc(1, sizeof(STKDelayAInfo), "STKDelayAInfo");
  d->fil.x = newRLB(max_delay + 1, 0, "STKDelayAInfo fil.x");
  d->fil.y = newRLB(1, 0, "STKDelayAInfo fil.y");
  d->del.t = delay;
  d->del.A.do_next_out = TRUE;
  return d;
}

