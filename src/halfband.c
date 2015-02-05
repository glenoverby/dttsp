/** 
* @file halfband.c
* @brief Functions to calculate halfband
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
			   
//------------------------------------------------------------------
#include <halfband.h>
//------------------------------------------------------------------

/* -------------------------------------------------------------------------- */
/** @brief Run a Half Band function 
* 
* @param h 
* @return void
*/
/* ---------------------------------------------------------------------------- */
void
HalfBandit(HalfBander h) {
  int i;
  for (i = 0; i < CXBhave(h->buf.i); i++)
    CXBdata(h->buf.o, i) = Cmplx(hb_filt_proc(h->filt.gen.re,
					      CXBreal(h->buf.i, i)),
				 hb_filt_proc(h->filt.gen.im,
					      CXBimag(h->buf.i, i)));
}

/* -------------------------------------------------------------------------- */
/** @brief Create a new Halfbander 
* 
* @param ord 
* @param steep 
* @param ibuf 
* @param obuf 
* @return void
*/
/* ---------------------------------------------------------------------------- */
HalfBander
newHalfBander(int ord, BOOLEAN steep, CXB ibuf, CXB obuf) {
  HalfBander h = (HalfBander) safealloc(1, sizeof(HalfBandInfo), "HalfBander wrapper");
  h->filt.gen.re = new_hb_filt(ord, steep);
  h->filt.gen.im = new_hb_filt(ord, steep);
  h->buf.i = newCXB(CXBsize(ibuf), CXBbase(ibuf), "HalfBander input");
  h->buf.o = newCXB(CXBsize(obuf), CXBbase(obuf), "HalfBander output");
  return h;
}

/* -------------------------------------------------------------------------- */
/** @brief Destroy Halfbander 
* 
* @param h 
* @return void
*/
/* ---------------------------------------------------------------------------- */
void
delHalfBander(HalfBander h) {
  if (h) {
    del_hb_filt(h->filt.gen.re);
    del_hb_filt(h->filt.gen.im);
    delCXB(h->buf.i);
    delCXB(h->buf.o);
    safefree((char *) h);
  }
}

//------------------------------------------------------------------

static void _bail(int);
static char *_safealloc(int, int, int);
static void _safefree(char *);

//------------------------------------------------------------------
//------------------------------------------------------------------

/* -------------------------------------------------------------------------- */
/** @brief Half bander filter process 
* 
* @param this 
* @param inp 
* @return REAL
*/
/* ---------------------------------------------------------------------------- */
REAL
hb_filt_proc(hb_filt_t *this, REAL inp) {
  REAL out = (hb_ap_casc_proc(this->a, inp) + this->old) * 0.5;
  this->old = hb_ap_casc_proc(this->b, inp);
  return out;
}

//..................................................................

/* -------------------------------------------------------------------------- */
/** @brief _hb_ap_proc 
* 
* @param this 
* @param inp 
* @return REAL
*/
/* ---------------------------------------------------------------------------- */
static inline REAL
_hb_ap_proc(hb_ap_t *this, REAL inp) {
  this->x[2] = this->x[1], this->x[1] = this->x[0], this->x[0] = inp;
  this->y[2] = this->y[1], this->y[1] = this->y[0],
    this->y[0] = this->x[2] + (this->c * (inp - this->y[2]));
  return this->y[0];
}

//..................................................................

/* -------------------------------------------------------------------------- */
/** @brief hb_ap_casc_proc 
* 
* @param this 
* @param inp 
* @return REAL
*/
/* ---------------------------------------------------------------------------- */
REAL
hb_ap_casc_proc(hb_ap_casc_t *this, REAL inp) {
  int i;
  REAL out = inp;
  for (i = 0; i < this->numf; i++) out = _hb_ap_proc(this->apf[i], out);
  return out;
}

/* -------------------------------------------------------------------------- */
/** @brief hb_ap_proc 
* 
* @param this 
* @param inp 
* @return REAL
*/
/* ---------------------------------------------------------------------------- */
REAL
hb_ap_proc(hb_ap_t *this, REAL inp) {
  this->x[2] = this->x[1], this->x[1] = this->x[0], this->x[0] = inp;
  this->y[2] = this->y[1], this->y[1] = this->y[0],
    this->y[0] = this->x[2] + (this->c * (inp - this->y[2]));
  return this->y[0];
}

//------------------------------------------------------------------
//------------------------------------------------------------------

/* -------------------------------------------------------------------------- */
/** @brief new_hb_ap 
* 
* @param coef 
* @return hb_ap_t *
*/
/* ---------------------------------------------------------------------------- */
hb_ap_t *
new_hb_ap(REAL coef) {
  hb_ap_t *this = (hb_ap_t *) _safealloc(1, sizeof(hb_ap_t), __LINE__);
  this->c = coef;
  return this;
}

/* -------------------------------------------------------------------------- */
/** @brief del_hb_ap 
* 
* @param this 
* @param  
* @param this 
* @return void
*/
/* ---------------------------------------------------------------------------- */
void
del_hb_ap(hb_ap_t *this) { _safefree((char *) this); }

//------------------------------------------------------------------

/* -------------------------------------------------------------------------- */
/** @brief bh_ap_casc_t 
* 
* @param coef 
* @param n 
* @return hb_ap_casc_t *
*/
/* ---------------------------------------------------------------------------- */
hb_ap_casc_t *
new_hb_ap_casc(REAL *coef, int n) {
  hb_ap_casc_t *this = (hb_ap_casc_t *) _safealloc(1,
						   sizeof(hb_ap_casc_t),
						   __LINE__);
  this->numf = n;
  this->apf = (hb_ap_t **) _safealloc(this->numf, sizeof(hb_ap_t *), __LINE__);
  {
    int i;
    for (i = 0; i < this->numf; i++) this->apf[i] = new_hb_ap(coef[i]);
  }
}

/* -------------------------------------------------------------------------- */
/** @brief del_hb_ap_casc 
* 
* @param this 
* @return void
*/
/* ---------------------------------------------------------------------------- */
void
del_hb_ap_casc(hb_ap_casc_t *this) {
  if (this) {
    int i;
    for (i = 0; i < this->numf; i++) del_hb_ap(this->apf[i]);
    _safefree(( char*) this->apf);
    _safefree((char *) this);
  }
}

//------------------------------------------------------------------

/* -------------------------------------------------------------------------- */
/** @brief new_hb_filt 
* 
* @param ord 
* @param steep 
* @return hb_filt_t
*/
/* ---------------------------------------------------------------------------- */
hb_filt_t *
new_hb_filt(int ord, int steep) {
  hb_filt_t *this= (hb_filt_t *) _safealloc(1,sizeof(hb_filt_t),__LINE__);

  if (steep) {

    switch (ord) {

      // rejection = 104dB, transition band = 0.01
    case 12: {
      REAL a[6] = {
	0.036681502163648017,
	0.274631759379454100,
	0.561098969787919480,
	0.769741833862266000,
	0.892260818003878900,
	0.962094548378084000
      };
      REAL b[6] = {
	0.136547624631957710,
	0.423138617436566670,
	0.677540049974161600,
	0.839889624849638000,
	0.931541959963183900,
	0.987816370732897100
      };

      this->a = new_hb_ap_casc(a, 6);
      this->b = new_hb_ap_casc(b, 6);
      break;
    }

      // rejection = 86dB, transition band = 0.01
    case 10: {
      REAL a[5] = {
	0.051457617441190984,
	0.359786560705670170,
	0.672547593103469300,
	0.859088492824993900,
	0.954020986786078700
      };
      REAL b[5] = {
	0.18621906251989334,
	0.52995137284796400,
	0.78102575274895140,
	0.91418156876053080,
	0.98547502301490700
      };
	  
      this->a = new_hb_ap_casc(a, 5);
      this->b = new_hb_ap_casc(b, 5);
      break;
    }

      // rejection = 69dB, transition band = 0.01
    case 8: {
      REAL a[4] = {
	0.07711507983241622,
	0.48207062506104720,
	0.79682047133157970,
	0.94125142777404710
      };
      REAL b[4] = {
	0.2659685265210946,
	0.6651041532634957,
	0.8841015085506159,
	0.9820054141886075
      };
	  
      this->a = new_hb_ap_casc(a, 4);
      this->b = new_hb_ap_casc(b, 4);
      break;
    }

      // rejection = 51dB, transition band = 0.01
    case 6: {
      REAL a[3] = {
	0.1271414136264853,
	0.6528245886369117,
	0.9176942834328115
      };
      REAL b[3] = {
	0.40056789819445626,
	0.82041638919233430,
	0.97631145158367730
      };
      
      this->a = new_hb_ap_casc(a, 3);
      this->b = new_hb_ap_casc(b, 3);
      break;
    }

      // rejection = 53dB,transition band = 0.05
    case 4: {
      REAL a[2] = {
	0.12073211751675449,
	0.66320202241939950
      };
      REAL b[2] = {
	0.3903621872345006,
	0.8907868326534970
      };

      this->a = new_hb_ap_casc(a, 2);
      this->b = new_hb_ap_casc(b, 2);
      break;
    }

      // rejection = 36dB, transition band = 0.1
    case 2: {
      REAL a[1] = { 0.23647102099689224 };
      REAL b[1] = { 0.71454214971260010 };

      this->a = new_hb_ap_casc(a, 1);
      this->b = new_hb_ap_casc(b, 1);
      break;
    }

    default:
      _bail(__LINE__);
    }

  } else {

    switch (ord) {

      // rejection = 150dB, transition band = 0.05
    case 12: {
      REAL a[6] = {
	0.01677466677723562,
	0.13902148819717805,
	0.33250111173947310,
	0.53766105314488000,
	0.72141840242158050,
	0.88218584020781550
      };
      REAL b[6] = {
	0.06501319274445962,
	0.23094129990840923,
	0.43649423484203550,
	0.06329609551399348,
	0.80378086794111226,
	0.95996874048006940
      };

      this->a = new_hb_ap_casc(a, 6);
      this->b = new_hb_ap_casc(b, 6);
      break;
    }

      // rejection = 133dB, transition band = 0.05
    case 10: {
      REAL a[5] = {
	0.02366831419883467,
	0.18989476227180174,
	0.43157318062118555,
	0.66320202241939950,
	0.86001554249958200
      };
      REAL b[5] = {
	0.09056555904993387,
	0.30785757237490430,
	0.55167824025079340,
	0.76521468637798080,
	0.95247728378667541
      };

      this->a = new_hb_ap_casc(a, 5);
      this->b = new_hb_ap_casc(b, 5);
      break;
    }

      // rejection = 106dB, transition band = 0.05
    case 8: {
      REAL a[4] = {
	0.03583278843106211,
	0.27204014339645760,
	0.57205719723570030,
	0.82712476199732400
      };
      REAL b[4] = {
	0.1340901419430669,
	0.4243248712718685,
	0.7062921421386394,
	0.9415030941737551
      };
	  
      this->a = new_hb_ap_casc(a, 4);
      this->b = new_hb_ap_casc(b, 4);
      break;
    }

      // rejection = 80dB, transition band = 0.05
    case 6: {
      REAL a[3] = {
	0.06029739095712437,
	0.41259072036105630,
	0.77271565374292340
      };
      REAL b[3] = {
	0.21597144456092948,
	0.6043586264658363,
	0.9238861386532906
      };

      this->a = new_hb_ap_casc(a, 3);
      this->b = new_hb_ap_casc(b, 3);
      break;
    }

      // rejection = 70dB, transition band = 0.1
    case 4: {
      REAL a[2] = {
	0.07986642623635751,
	0.54535365107113220
      };
      REAL b[2] = {
	0.28382934487410993,
	0.83441189148073790
      };

      this->a = new_hb_ap_casc(a, 2);
      this->b = new_hb_ap_casc(b, 2);
      break;
    }

      // rejection = 36dB, transition band = 0.1
    case 2: {
      REAL a[1] = { 0.23647102099689224 };
      REAL b[1] = { 0.71454214971260010 };

      this->a = new_hb_ap_casc(a, 1);
      this->b = new_hb_ap_casc(b, 1);
      break;
    }

    default:
      _bail(__LINE__);
    }
  }

  this->old = 0.0;

  return this;
}

/* -------------------------------------------------------------------------- */
/** @brief del_hb_filt 
* 
* @param this 
* @return void
*/
/* ---------------------------------------------------------------------------- */
void
del_hb_filt(hb_filt_t *this) {
  del_hb_ap_casc(this->a);
  del_hb_ap_casc(this->b);
  _safefree((char *) this);
}

//------------------------------------------------------------------

/* -------------------------------------------------------------------------- */
/** @brief _bail
* 
* @param where 
* @return void
*/
/* ---------------------------------------------------------------------------- */
static void
_bail(int where) {
  fprintf(stderr, "halfband _bailed at line %d\n", where);
  exit(1);
}

/* -------------------------------------------------------------------------- */
/** @brief _safealloc
* 
* @param num 
* @param size 
* @param where 
* @return *char
*/
/* ---------------------------------------------------------------------------- */
static char *
_safealloc(int num, int size, int where) {
  char *p = calloc(num, size);
  if (!p) _bail(where);
  return p;
}

/* -------------------------------------------------------------------------- */
/** @brief _safefree 
* 
* @param p 
* @param if(p 
* @param free(p 
* @return void
*/
/* ---------------------------------------------------------------------------- */
static void
_safefree(char *p) { if (p) free(p); }
