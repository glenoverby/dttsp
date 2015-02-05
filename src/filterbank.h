// filterbank.h
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

#ifndef _Filter_Bank_H
#define _Filter_Bank_H
#include <ovsv.h>
#include <gsl/gsl_blas.h>
#include <filter.h>
#include <fftw3.h>

typedef struct _FilterBank {
  RealFIR fbfil; 
  fftwf_plan fb_fft;
  BOOLEAN Analysis,Complex,MyFilt;
  int Partitions,Columns;
  gsl_complex_float *in,*out,*databuf;
  gsl_vector_complex_float gvcf;
  gsl_matrix_float *filter_partition;
  gsl_matrix_complex_float *data_partition;
  gsl_matrix_complex_float_view old,new;
  
} FilterBankDesc, *FilterBank;

typedef struct _FilterBankOvSv {
  FilterBank fb;
  FiltOvSv fbFilterPartion;
} FilterBankOVSVDesc, *FilterBankOVSV;

extern FilterBank newFilterBank(REAL SampleRate, int ChangeRatio, BOOLEAN Analysis,
				REAL StopbandAttenuation, REAL TransitionBandwidth,
				COMPLEX *databuf, RealFIR lp, int wisdombits);

extern FilterBankOVSV newFilterBankOVSV(REAL SampleRate, int ChangeRatio,
					BOOLEAN Analysis, REAL StopbandAttenuation,
					REAL TransitionBandwidth, COMPLEX *databuf,
					RealFIR lp, int wisdombits);

extern void delFilterBank(FilterBank fb);
extern void delFilterBankOVSV(FilterBankOVSV fb);
extern void runAnalysisFilterBank(FilterBank p);
extern void runAnalysisFilterBankOVSV(FilterBank p);
extern void runSynthesisFilterBank(FilterBank p);
extern void runSynthesisFilterBankOVSV(FilterBank p);
#endif
