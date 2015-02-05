/* cxops.h
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

#ifndef _cxops_h

#define _cxops_h

#include <complex.h>
#include <datatypes.h>
#include <fastrig.h>
#ifdef __SSE3__
#include <pmmintrin.h>
#endif

extern COMPLEX cxzero;
extern COMPLEX cxone;
extern COMPLEX cxJ;
extern COMPLEX cxminusone;
extern COMPLEX cxminusJ;



// scalar

PRIVATE INLINE
COMPLEX
Cmplx(REAL x, IMAG y) {
  COMPLEX z;
  z.re = x, z.im = y;
  return z;
}

PRIVATE INLINE
COMPLEX
Cscl(COMPLEX x, REAL a) {
  COMPLEX z;
  c_re(z) = c_re(x) * a;
  c_im(z) = c_im(x) * a;
  return z;
}

PRIVATE INLINE
COMPLEX
Cadd(COMPLEX x, COMPLEX y) {
  COMPLEX z;
  c_re(z) = c_re(x) + c_re(y);
  c_im(z) = c_im(x) + c_im(y);
  return z;
}

PRIVATE INLINE
COMPLEX
Csub(COMPLEX x, COMPLEX y) {
  COMPLEX z;
  c_re(z) = c_re(x) - c_re(y);
  c_im(z) = c_im(x) - c_im(y);
  return z;
}

PRIVATE INLINE
COMPLEX
Cmul(COMPLEX x, COMPLEX y) {
  COMPLEX z;
  c_re(z) = c_re(x) * c_re(y) - c_im(x) * c_im(y);
  c_im(z) = c_im(x) * c_re(y) + c_re(x) * c_im(y);
  return z;
}


//PRIVATE INLINE
//REAL
//Cappmag(COMPLEX z) {
//  REAL tmpr = (REAL) fabs(z.re),
//       tmpi = (REAL) fabs(z.im);
//  if (tmpr < tmpi) return 0.4f * tmpr + 0.7f * tmpi;
//  else             return 0.4f * tmpi + 0.7f * tmpr;
//}

PRIVATE INLINE
REAL
Cmag(COMPLEX z) {
  return (REAL) hypot(z.re, z.im);
}

PRIVATE INLINE
REAL
Cabs(COMPLEX z) {
  return (REAL) hypot(z.re, z.im);
}

PRIVATE INLINE
REAL
Csqrmag(COMPLEX z) {
  return (REAL) (sqr(z.re) + sqr(z.im));
}

PRIVATE INLINE
COMPLEX
Conjg(COMPLEX z) {
  return Cmplx(z.re, -z.im);
}

PRIVATE INLINE
COMPLEX
Cexp(COMPLEX z) {
  REAL r = (REAL) exp(z.re);
  return Cmplx((REAL) (r * cos(z.im)), (IMAG) (r * sin(z.im)));
}

PRIVATE INLINE
COMPLEX
Cp2r(COMPLEX z) {
  return Cmplx((REAL) (z.re * cos(z.im)), (IMAG) (z.re * sin(z.im)));
}

PRIVATE INLINE
COMPLEX
Cr2p(COMPLEX z) {
  return Cmplx((REAL) hypot(z.re, z.im),
	       (REAL) ATAN2(z.im, z.re));
}



PRIVATE INLINE
COMPLEX
Cdiv(COMPLEX x, COMPLEX y) {
  REAL d = sqr(c_re(y)) + sqr(c_im(y));
  COMPLEX z;
  c_re(z) = (c_re(x) * c_re(y) + c_im(x) * c_im(y)) / d;
  c_im(z) = (c_re(y) * c_im(x) - c_im(y) * c_re(x)) / d;
  return z;
}



#define alphaC 0.948059448969
#define betaC 0.392699081699

PRIVATE INLINE
REAL
Cappmag(COMPLEX z) {
  REAL tmpr = fabs(z.re), tmpi = fabs(z.im);
  if (tmpr > tmpi) return alphaC * tmpr + betaC * tmpi;
  else             return alphaC * tmpi + betaC * tmpr;
}


// end of scalar section

#ifdef __SSE3__

// vector complex arithmetic accelerated by SSE3

PRIVATE INLINE
void
sse3ComplexMult(COMPLEX *c, COMPLEX *a, COMPLEX *b)
{
  __m128 x, y, yl, yh, z, tmp1, tmp2;

  x = _mm_load_ps((float *)a);  // Load as (r,i)
  y = _mm_load_ps((float *)b);  // Load as (r,i)

  yl = _mm_moveldup_ps(y);  // Load yl with cr, cr, dr, dr
  yh = _mm_movehdup_ps(y);  // Load yh with ci, ci, di ,di

  tmp1 = _mm_mul_ps(x,yl);  // tmp1 = ar*cr, ai*cr, br*dr, bi*dr

  x = _mm_shuffle_ps(x, x, 0xB1); // Re-arrange x tp be ai,ar,bi,br

  tmp2 = _mm_mul_ps(x,yh);  // tmp2 = ai*ci, ar*ci, bi*di, br*di

  z = _mm_addsub_ps(tmp1,tmp2); // ar*cr-ai*ci, ai*cr+ar*ci, br*dr-bi*di, bi*dr+br*di

  _mm_store_ps((float *)c,z); // Store the results in the target Complex container
}

PRIVATE INLINE
void
CmulSSE3(COMPLEX *c, COMPLEX *a, COMPLEX *b, int len)
{
  COMPLEX *aPtr;
  COMPLEX *bPtr;
  COMPLEX *cPtr;
  int i;

  aPtr = a;
  bPtr = b;
  cPtr = c;

  for(i=0;i<len;i+=2) {
    // Do 2 Complex Multiplies per loop
    sse3ComplexMult(cPtr, aPtr, bPtr);
    cPtr += 2;
    aPtr += 2;
    bPtr += 2;
  }

  if (len%2) //take care of the odd dangler
      *cPtr = Cmul(*aPtr,*bPtr);
}



// The following routines are adapted from code done by  Phil Covington, N8VB
// p.covington@gmail.com
// N8VB

PRIVATE INLINE
void 
DoSSEScaleCOMPLEX(COMPLEX * c, COMPLEX * a, float b) {
    __m128 x, y, z;
           
    x = _mm_load_ps((float *)a);
    y = _mm_load_ps1(&b);
           
    z = _mm_mul_ps(x, y); 
    
    _mm_store_ps((float *)c, z);    
}

PRIVATE INLINE
void 
SSEScaleCOMPLEX(COMPLEX * c, COMPLEX * a, float b, int size) {
    COMPLEX * aa;
    float sb;
    COMPLEX * cc;
    int i;

    aa = a;
    sb = b;
    cc = c;

    for (i = 0; i < size; i+=2) {
        DoSSEScaleCOMPLEX(cc, aa, sb);
        cc += 2;
        aa += 2;        
    }
    if (size%2)
        *cc = Cscl(*aa, sb); 
}


PRIVATE INLINE void DoSSEAddCOMPLEX(COMPLEX * c, COMPLEX * a, COMPLEX * b) {
    __m128 x, y, z;
           
    x = _mm_load_ps((float *)a);
    y = _mm_load_ps((float *)b);
           
    z = _mm_add_ps(x, y); 
    
    _mm_store_ps((float *)c, z);    
}

PRIVATE INLINE 
void 
SSEAddCOMPLEX(COMPLEX * c, COMPLEX * a, COMPLEX * b, int size) {
    COMPLEX * aa;
    COMPLEX * bb;
    COMPLEX * cc;
    int i;

    aa = a;
    bb = b;
    cc = c;
    
    for (i = 0; i < size; i+=2) {
        DoSSEAddCOMPLEX(cc, aa, bb);
        cc += 2;
        aa += 2;
        bb += 2;
    }
    if (size%2)
        *cc = Cadd(*aa, *bb);
}


PRIVATE INLINE
void 
DoSSEMultCOMPLEX(COMPLEX * c, COMPLEX * a, COMPLEX * b) {
    
    __m128 x, y, yl, yh, t1, t2, z;
           
    x = _mm_load_ps((float *)a);
    y = _mm_load_ps((float *)b);
    
    yl = _mm_moveldup_ps(y);
    yh = _mm_movehdup_ps(y);
    
    t1 = _mm_mul_ps(x, yl);
    
    x = _mm_shuffle_ps(x, x, _MM_SHUFFLE(2,3,0,1));
    
    t2 = _mm_mul_ps(x, yh);
    
    z = _mm_addsub_ps(t1, t2);
    
    _mm_store_ps((float *)c, z);    
}

PRIVATE INLINE
void
SSEMultCOMPLEX(COMPLEX * c, COMPLEX * a, COMPLEX * b, int size) {

    COMPLEX * aa;
    COMPLEX * bb;
    COMPLEX * cc;
    int i;
    
    aa = a;
    bb = b;
    cc = c;
    
    for (i = 0; i < size; i+=2) {
        DoSSEMultCOMPLEX(cc, aa, bb);
        cc += 2;
        aa += 2;
        bb += 2;
    }
    if (size%2)
        *cc = Cmul(*aa, *bb);
}

PRIVATE INLINE
void
DoSSEMagCOMPLEX(COMPLEX * c, COMPLEX * a) {
    __m128 x, y, t1, z;
           
    x = _mm_load_ps((float *)a);
    y = _mm_load_ps((float *)a);
            
    t1 = _mm_mul_ps(x, y);
    
    t1 = _mm_hadd_ps(t1, t1);
    
    t1 = _mm_shuffle_ps(t1, t1, _MM_SHUFFLE(3,1,2,0));
    
    z = _mm_sqrt_ps(t1);
    
    _mm_store_ps((float *)c, z);    
}

PRIVATE INLINE
void
SSEMagCOMPLEX(COMPLEX * c, COMPLEX * a, int size) {
    COMPLEX * aa;
    COMPLEX * cc;
    int i;
    
    aa = a;
    cc = c;
    
    for (i = 0; i < size; i+=2) {
        DoSSEMagCOMPLEX(cc, aa);
        cc += 2;
        aa += 2;        
    }
    if (size%2) {
        cc->re = Cmag(*aa);
        cc->im = cc->re;
    }
}


// End p.c. contribution.

#endif  // end of SSE3 section
  


#if 0
extern INLINE COMPLEX Cscl(COMPLEX, REAL);
extern INLINE COMPLEX Cadd(COMPLEX, COMPLEX);
extern INLINE COMPLEX Csub(COMPLEX, COMPLEX);
extern INLINE COMPLEX Cmul(COMPLEX, COMPLEX);
extern INLINE COMPLEX Cdiv(COMPLEX, COMPLEX);
extern INLINE REAL Cmag(COMPLEX);
extern INLINE REAL Cappmag(COMPLEX);
extern INLINE REAL Cabs(COMPLEX);
extern INLINE REAL Csqrmag(COMPLEX);
extern INLINE COMPLEX Cmplx(REAL, IMAG);
extern INLINE COMPLEX Conjg(COMPLEX);
extern INLINE COMPLEX Cexp(COMPLEX);

extern INLINE COMPLEX Cp2r(COMPLEX);
extern INLINE COMPLEX Cr2p(COMPLEX);
#endif

#endif
