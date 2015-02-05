/* sdrexport.h

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

#ifndef _sdrexport_h
#define _sdrexport_h

#include <fromsys.h>
#include <defs.h>
#include <banal.h>
#include <splitfields.h>
#include <datatypes.h>
#include <bufvec.h>
#include <cxops.h>
#include <ringb.h>
#include <lmadf.h>
#include <fftw3.h>
#include <ovsv.h>
#include <filter.h>
#include <oscillator.h>
#include <dttspagc.h>
#include <am_demod.h>
#include <fm_demod.h>
#include <noiseblanker.h>
#include <correctIQ.h>
#include <speechproc.h>
#include <spottone.h>
#include <update.h>
#include <local.h>
#include <meter.h>
#include <spectrum.h>
#include <resample.h>
//------------------------------------------------------------------------
// max no. simultaneous receivers
#ifndef MAXRX
#define MAXRX (4)
#endif
//------------------------------------------------------------------------
/* modulation types, modes */

//========================================================================
/* RX/TX both */
//------------------------------------------------------------------------
extern
struct _uni {
  struct {
    REAL sample;
  } rate;
  int buflen;

  struct {
    SDRMODE sdr;
    TRXMODE trx;
  } mode;

  METERBlock meter;
  SpecBlock spec;

  struct {
    BOOLEAN flag;
    char *path;
    FILE *fp;
    splitfld splt;
  } update;

  struct {
    char *path;
    int bits;
  } wisdom;

  struct {
    BOOLEAN act[MAXRX];
    int lis, nac, nrx;
  } multirx;

  int cpdlen;
  long tick;

} *uni;

//------------------------------------------------------------------------
/* RX */
//------------------------------------------------------------------------

extern
struct _rx {

  int len;

  struct {
    CXB i, o;
  } buf;

  IQ iqfix;

  struct {
    double freq, phase;
    OSC gen;
  } osc;

  struct {
    REAL lo, hi;
    ComplexFIR coef;
    FiltOvSv ovsv;
    COMPLEX *save;
  } filt;

  struct {
    REAL thresh;
    NB gen;
    BOOLEAN flag;
  } nb;

  struct {
    REAL thresh;
    NB gen;
    BOOLEAN flag;
  } nb_sdrom;

  struct {
    LMSR gen;
    BOOLEAN flag;
  } anr, anf;

  struct {
    BLMS gen;
    BOOLEAN flag;
  } banr, banf;

  struct {
    DTTSPAGC gen;
    BOOLEAN flag;
  } dttspagc;

  struct {
    AMD gen;
  } am;

  struct {
    FMD gen;
  } fm;

  struct {
    BOOLEAN flag;
    SpotToneGen gen;
  } spot;

  struct {
    REAL thresh, power;
    BOOLEAN flag, running, set;
    int num;
  } squelch;

  struct {
    BOOLEAN flag;
    WSCompander gen;
  } cpd;

  struct {
    EQ gen;
    BOOLEAN flag;
    struct {
      int size;
      REAL pre, gain[10];
    } parm;
  } grapheq;

  SDRMODE mode;
  
  struct {
    BOOLEAN flag;
  } bin;

  REAL norm;

  struct {
    REAL i, o;
  } gain;
  COMPLEX azim;

  long tick;
} *rx[MAXRX];

//------------------------------------------------------------------------
/* TX */
//------------------------------------------------------------------------
extern
struct _tx {

  int len;

  struct {
    CXB i, o;
  } buf;

  IQ iqfix;

  struct {
    BOOLEAN flag;
    DCBlocker gen;
  } dcb;

  struct {
    double freq, phase;
    OSC gen;
  } osc;

  struct {
    REAL lo, hi;
    ComplexFIR coef;
    FiltOvSv ovsv;
    COMPLEX *save;
  } filt;

  struct {
    REAL carrier_level;
  } am;

  struct {
    REAL cvtmod2freq;
  } fm;

  struct {
    REAL thresh, power;
    BOOLEAN flag, running, set;
    int num;
  } squelch;

  struct {
    DTTSPAGC gen;
    BOOLEAN flag;
  } leveler;

  struct {
    EQ gen;
    BOOLEAN flag;
    struct {
      int size;
      REAL pre, gain[10];
    } parm;
  } grapheq;

  struct {
    SpeechProc gen;
    BOOLEAN flag;
  } spr;

  /***/

  struct {
    BOOLEAN flag;
    WaveShaper gen;
  } wvs;

  /***/

  struct {
    BOOLEAN flag;
    WSCompander gen;
  } cpd;

  struct {
    REAL comp, cpdr, eqtap, lvlr, mic, pwr;
  } sav;

  struct {
    REAL i, o;
  } gain;

  SDRMODE mode;

  long tick;
  REAL norm;

} *tx;

//------------------------------------------------------------------------

typedef
enum _runmode {
  RUN_MUTE, RUN_PASS, RUN_PLAY, RUN_SWCH, RUN_TEST
} RUNMODE;

typedef
enum _testmode {
  TEST_TONE, TEST_2TONE, TEST_CHIRP, TEST_NOISE
} TESTMODE;

extern
struct _top {
  int pid;
  uid_t uid;

  struct timeval start_tv;

  BOOLEAN running, verbose;
  RUNMODE state;

  // audio io

  struct {
    struct {
      float *l, *r;
    } buf;
    struct {
      unsigned int frames, bytes;
    } size;
  } hold;

  struct {
    char *path;
    int fd;
    FILE *fp;
    char buff[4096];
  } echo;

  struct {
    char buff[4096];
    unsigned short port;
  } parm;

  struct {
    char buff[4096];
    int size;
  } resp;

  struct {
    struct {
      short port;
    } mtr, spec;
  } meas;

  int offs;	// to compensate for L/R ADC lags

  struct {
    char name[256];

    BOOLEAN doin;
    size_t size, rsiz;
    jack_client_t *client;

    struct {
      struct {
	jack_port_t *l, *r;
      } i, o;
    } port;

    struct {
      struct {
	ringb_float_t *l, *r;
      } i, o;
    } ring;

  } snds;

  // update io
  // multiprocessing & synchronization
  struct {
    struct {
      pthread_t id;
    } mtr, pws, trx, upd;
  } thrd;

  struct {
    struct {
      sem_t *sem;
      char name[512];
    } buf, mtr, pws, upd;
  } sync;

  // TRX switching
  struct {
    struct {
      struct {
	SWCHSTATE type;
	int cnt;
	REAL val;
      } curr;
      struct {
	int size;
	REAL incr;
      } fall, rise, stdy;
    } env;
    struct {
      TRXMODE next;
    } trx;
    struct {
      RUNMODE last;
    } run;
  } swch;

  BOOLEAN susp;

  // test signals
  struct {

    struct {
      OSC gen;
      REAL amp, freq;
    } tone;

    struct {
      struct {
	OSC gen;
	REAL amp, freq;
      } a, b;
    } twotone;

#ifdef notdef
    struct {
    } chirp;
#endif

    struct {
      REAL amp;
    } noise;

    TESTMODE mode;
    BOOLEAN thru;
  } test;

} *top;

#endif
