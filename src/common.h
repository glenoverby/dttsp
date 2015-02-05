/* common.h
a simple way to get all the necessary includes
   
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

#ifndef _common_h
#define _common_h

#include <fromsys.h>
#include <defs.h>
#include <banal.h>
#include <splitfields.h>
#include <datatypes.h>
#include <bufvec.h>
#include <cxops.h>
#include <ringb.h>
#include <fftw3.h>
#include <window.h>
#include <ovsv.h>
#include <filter.h>
#include <oscillator.h>
#include <hilbert.h>
#include <lmadf.h>
#include <dttspagc.h>
#include <am_demod.h>
#include <fm_demod.h>
#include <resample.h>
#include <noiseblanker.h>
#include <correctIQ.h>
#include <dcblock.h>
#include <graphiceq.h>
#include <speechproc.h>
#include <wscompand.h>
#include <spottone.h>
#include <cwtones.h>
#include <update.h>
#include <meter.h>
#include <spectrum.h>
#include <isoband.h>
#include <hilbert.h>
#include <halfband.h>
#include <waveshape.h>

#include <sdrexport.h>
#include <local.h>
#endif
