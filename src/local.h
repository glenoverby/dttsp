/* local.h

Some manifest constants for the particular implementation
   
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

#ifndef _local_h
#define _local_h

#include <common.h>

#define RCBASE ".DttSPrc"
#define PARMPORT 19001
#define SPECPORT 19002
#define METERPORT 19003
#define REPLAYPATH ".replay"
#define WISDOMPATH ".wisdom"

extern struct _loc {
  char name[MAXPATHLEN];
  struct {
    char rcfile[MAXPATHLEN],
         echo[MAXPATHLEN],
         meter[MAXPATHLEN],
         replay[MAXPATHLEN],
         spec[MAXPATHLEN],
         wisdom[MAXPATHLEN];
  } path;
  struct {
    REAL rate, chan;
    int size, bufl, nrx, spec, comp;
    SDRMODE mode;
  } def;
  struct {
    int ring;
  } mult;
  struct {
    int offs;
  } skew;
  struct {
    unsigned short spec, meter, parm;
  } port;
} loc;

#endif
