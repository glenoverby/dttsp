// halfband.h
/*
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

or

The DTTS Microwave Society
3125 Capilano Crescent #201
North Vancouver, BC V7R 4X5
Canada
*/			   

#ifndef _halfband_h
#define _halfband_h

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>  
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/socket.h>  
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <assert.h>

#ifndef M_PI
#define M_PI 3.14159265358928
#endif

#include <datatypes.h>
#include <complex.h>
#include <bufvec.h>			   
			   
//==================================================================

typedef
struct hb_ap {
  REAL c, x[3], y[3];
} hb_ap_t;

typedef
struct hb_ap_casc {
  int numf;
  hb_ap_t **apf;
} hb_ap_casc_t;

typedef
struct hb_filt {
  hb_ap_casc_t *a, *b;
  REAL old;
} hb_filt_t;

REAL hb_ap_proc(hb_ap_t *, REAL);
hb_ap_t *new_hb_ap(REAL coefficient);
void del_hb_ap(hb_ap_t *);

REAL hb_ap_casc_proc(hb_ap_casc_t *, REAL);
hb_ap_casc_t *new_hb_ap_casc(REAL *, int);
void del_hp_ap_casc(hb_ap_casc_t *);

REAL hb_filt_proc(hb_filt_t *, REAL);
hb_filt_t *new_hb_filt(int, int);
void del_hb_filt(hb_filt_t *);

//==================================================================

typedef
struct _half_band {
  struct {
    struct {
      hb_filt_t *re, *im;
    } gen;
  } filt;
  struct {
    CXB i, o;
  } buf;
} HalfBandInfo, *HalfBander;

extern void HalfBandit(HalfBander h);
extern HalfBander newHalfBander(int ord, BOOLEAN steep, CXB ibuf, CXB obuf);
extern void delHalfBander(HalfBander h);

#endif
