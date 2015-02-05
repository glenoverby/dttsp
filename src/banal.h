/* banal.h
   stuff we're too embarrassed to declare otherwise
   
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

#ifndef _banal_h

#define _banal_h

#include <fromsys.h>
#include <defs.h>
#include <datatypes.h>

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef abs
#define abs(a) ((a) >= 0 ? (a) : -(a))
#endif

#define MONDO 1e16
#define BITSY 1e-16
#define KINDA 2.56e2

#define TRUE 1
#define FALSE 0

extern void nilfunc(void);
extern int popcnt(int);
extern int npoof2(int);
extern int nblock2(int);
extern int in_blocks(int count, int block_size);

/*
extern INLINE REAL sqr(REAL);
extern INLINE REAL Log10(REAL);
extern INLINE REAL Log10P(REAL);
extern INLINE REAL Log10Q(REAL);
extern INLINE REAL dBP(REAL);
extern INLINE REAL DamPlus(REAL, REAL); */

static INLINE REAL
sqr(REAL x) { return x * x; }

static INLINE REAL
Log10(REAL x) { return log10(x + BITSY); }

static INLINE REAL
Log10P(REAL x) { return +10.0 * log10(x + BITSY); }

static INLINE REAL
Log10Q(REAL x) { return -10.0 * log10(x + BITSY); }

static INLINE REAL
dBP(REAL x) { return 20.0 * log10(x + BITSY); }

static INLINE REAL
DamPlus(REAL x0, REAL x1) { return 0.9995 * x0 + 0.0005 * x1; }

extern FILE *efopen(char *path, char *mode);
extern FILE *efreopen(char *path, char *mode, FILE * strm);
extern size_t filesize(char *path);
extern size_t fdsize(int fd);

extern struct timeval now_tv(void);
extern struct timeval diff_tv(struct timeval *, struct timeval *);
extern struct timeval sum_tv(struct timeval *, struct timeval *);
extern char *fmt_tv(struct timeval *);
extern char *since(struct timeval *);
extern struct timeval now_tv(void);

extern int hinterp_vec(REAL *, int, REAL *, int);

extern void status_message(char *msg);

extern FILE *find_rcfile(char *base);

extern unsigned long hash(unsigned char *str);

extern int gcd(int u, int v);
extern int lcm(int u, int v);

#endif
