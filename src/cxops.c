/** 
* @file cxops.c
* @brief Functions Complex operations
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

#include <cxops.h>

// useful constants

COMPLEX cxzero = { 0.0, 0.0 };
COMPLEX cxone = { 1.0, 0.0 };
COMPLEX cxJ = { 0.0, 1.0 };
COMPLEX cxminusone = { -1.0, 0.0 };
COMPLEX cxminusJ = { 0.0, -1.0 };

#if 0
// scalar

/* -------------------------------------------------------------------------- */
/** @brief Cscl 
* 
* @param x 
* @param a 
*/
/* ---------------------------------------------------------------------------- */
COMPLEX
Cscl(COMPLEX x, REAL a) {
  COMPLEX z;
  c_re(z) = c_re(x) * a;
  c_im(z) = c_im(x) * a;
  return z;
}

/* -------------------------------------------------------------------------- */
/** @brief Cadd 
* 
* @param x 
* @param y 
*/
/* ---------------------------------------------------------------------------- */
COMPLEX
Cadd(COMPLEX x, COMPLEX y) {
  COMPLEX z;
  c_re(z) = c_re(x) + c_re(y);
  c_im(z) = c_im(x) + c_im(y);
  return z;
}

/* -------------------------------------------------------------------------- */
/** @brief Csub 
* 
* @param x 
* @param y 
*/
/* ---------------------------------------------------------------------------- */
COMPLEX
Csub(COMPLEX x, COMPLEX y) {
  COMPLEX z;
  c_re(z) = c_re(x) - c_re(y);
  c_im(z) = c_im(x) - c_im(y);
  return z;
}


/* -------------------------------------------------------------------------- */
/** @brief 
* 
* @param x 
* @param y 
*/
/* ---------------------------------------------------------------------------- */
COMPLEX
Cmul(COMPLEX x, COMPLEX y) {
  COMPLEX z;
  c_re(z) = c_re(x) * c_re(y) - c_im(x) * c_im(y);
  c_im(z) = c_im(x) * c_re(y) + c_re(x) * c_im(y);
  return z;
}

/* -------------------------------------------------------------------------- */
/** @brief Cdiv 
* 
* @param x 
* @param y 
*/
/* ---------------------------------------------------------------------------- */
COMPLEX
Cdiv(COMPLEX x, COMPLEX y) {
  REAL d = sqr(c_re(y)) + sqr(c_im(y));
  COMPLEX z;
  c_re(z) = (c_re(x) * c_re(y) + c_im(x) * c_im(y)) / d;
  c_im(z) = (c_re(y) * c_im(x) - c_im(y) * c_re(x)) / d;
  return z;
}

#define alpha 0.948059448969
#define beta 0.392699081699

/* -------------------------------------------------------------------------- */
/** @brief Cappmag 
* 
* @param z 
*/
/* ---------------------------------------------------------------------------- */
REAL
Cappmag(COMPLEX z) {
  REAL tmpr = fabs(z.re), tmpi = fabs(z.im);
  if (tmpr > tmpi) return alpha * tmpr + beta * tmpi;
  else             return alpha * tmpi + beta * tmpr;
}

#undef alpha
#undef beta

//REAL
//Cappmag(COMPLEX z) {
//  REAL tmpr = (REAL) fabs(z.re),
//       tmpi = (REAL) fabs(z.im);
//  if (tmpr < tmpi) return 0.4f * tmpr + 0.7f * tmpi;
//  else             return 0.4f * tmpi + 0.7f * tmpr;
//}

/* -------------------------------------------------------------------------- */
/** @brief Cmag 
* 
* @param z 
*/
/* ---------------------------------------------------------------------------- */
REAL
Cmag(COMPLEX z) {
  return (REAL) hypot(z.re, z.im);
}

/* -------------------------------------------------------------------------- */
/** @brief Cabs 
* 
* @param z 
*/
/* ---------------------------------------------------------------------------- */
REAL
Cabs(COMPLEX z) {
  return (REAL) hypot(z.re, z.im);
}

/* -------------------------------------------------------------------------- */
/** @brief Csqrmag 
* 
* @param z 
*/
/* ---------------------------------------------------------------------------- */
REAL
Csqrmag(COMPLEX z) {
  return (REAL) (sqr(z.re) + sqr(z.im));
}

/* -------------------------------------------------------------------------- */
/** @brief Cmplx 
* 
* @param x 
* @param y 
*/
/* ---------------------------------------------------------------------------- */
COMPLEX
Cmplx(REAL x, IMAG y) {
  COMPLEX z;
  z.re = x, z.im = y;
  return z;
}

/* -------------------------------------------------------------------------- */
/** @brief Conjg 
* 
* @param z 
*/
/* ---------------------------------------------------------------------------- */
COMPLEX
Conjg(COMPLEX z) {
  return Cmplx(z.re, -z.im);
}

/* -------------------------------------------------------------------------- */
/** @brief Cexp 
* 
* @param z 
*/
/* ---------------------------------------------------------------------------- */
COMPLEX
Cexp(COMPLEX z) {
  REAL r = (REAL) exp(z.re);
  return Cmplx((REAL) (r * cos(z.im)), (IMAG) (r * sin(z.im)));
}

/* -------------------------------------------------------------------------- */
/** @brief Cp2r 
* 
* @param z 
*/
/* ---------------------------------------------------------------------------- */
COMPLEX
Cp2r(COMPLEX z) {
  return Cmplx((REAL) (z.re * cos(z.im)), (IMAG) (z.re * sin(z.im)));
}

/* -------------------------------------------------------------------------- */
/** @brief Cr2p 
* 
* @param z 
*/
/* ---------------------------------------------------------------------------- */
COMPLEX
Cr2p(COMPLEX z) {
  return Cmplx((REAL) hypot(z.re, z.im),
	       (REAL) ATAN2(z.im, z.re));
}
#endif
