/** 
* @file filterbank.c
* @brief Functions to implement a filter bank 
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

#include <filterbank.h>

/* -------------------------------------------------------------------------- */
/** @brief Create new filter bank 
* 
* @param SampleRate 
* @param ChangeRatio 
* @param Analysis 
* @param StopbandAttenuation 
* @param TransitionBandwidth 
* @param databuf 
* @param lp 
* @param wisdombits 
*/
/* ---------------------------------------------------------------------------- */
FilterBank
newFilterBank(REAL SampleRate,
	      int ChangeRatio,
	      BOOLEAN Analysis,
	      REAL StopbandAttenuation,
	      REAL TransitionBandwidth,
	      COMPLEX *databuf,
	      RealFIR lp,
	      int wisdombits)
{
  FilterBank fbtmp;

  fbtmp = (FilterBank) safealloc(1, sizeof(FilterBankDesc), "FilterBank");
  int FilterSize;
  REAL cutoff;
  fbtmp->MyFilt = FALSE;
  gsl_vector_float_view old, new;
  if (floor(SampleRate / ChangeRatio) != (SampleRate / ChangeRatio)) {
    perror("Partitions must divide Sample Rate");
    exit(1);
  } else {
    if (!lp) {	// Filter was NOT passed in, we need to calculate
      FilterSize =
	pow(10.0, StopbandAttenuation / 20.0) *
	TransitionBandwidth / (22.0 * SampleRate);
      fbtmp->MyFilt = TRUE;
      cutoff = (SampleRate - 0.5 * TransitionBandwidth) / ChangeRatio;

      // needed to partition the filter.  You round up to have the
      // same number of of columns and the filter can be zero stuffed
      // if you want a specific filter.
      FilterSize = fbtmp->Partitions * fbtmp->Columns;
      fbtmp->fbfil = newFIR_Lowpass_REAL(cutoff, SampleRate, FilterSize);	// Make a real low pass filter for Analysis 
    } else {
      FilterSize = lp->size;
      fbtmp->fbfil = lp;
    }
    // ChangeRatio gives the number of bands
    fbtmp->Partitions = ChangeRatio;
    // The filtersize determines the number of columns
    fbtmp->Columns = ceil(FilterSize / fbtmp->Partitions);
  }
  fbtmp->out = (gsl_complex_float *) databuf;
  fbtmp->in  = (gsl_complex_float *) safealloc(1, sizeof(COMPLEX *),
					       "FB analysis fft iput buffer");
  if (Analysis) {
    int index;
    fbtmp->databuf = (gsl_complex_float *) databuf;
    fbtmp->gvcf.data = (float *) databuf;
    fbtmp->gvcf.size = fbtmp->Partitions;
    fbtmp->gvcf.stride = 1;
    fbtmp->gvcf.block = (gsl_block_complex_float *) databuf;
    fbtmp->gvcf.owner = 0;
    fbtmp->filter_partition =
      (gsl_matrix_float *) gsl_matrix_float_calloc(fbtmp->Partitions,
						   fbtmp->Columns);
    fbtmp->data_partition =
      (gsl_matrix_complex_float *) gsl_matrix_complex_float_calloc(fbtmp->Partitions,
								   fbtmp->Columns);
    fbtmp->old =
      gsl_matrix_complex_float_submatrix(fbtmp->data_partition, 0, 0,
					 fbtmp->Partitions,
					 fbtmp->Columns - 1);
    fbtmp->new =
      gsl_matrix_complex_float_submatrix(fbtmp->data_partition, 0, 1,
					 fbtmp->Partitions, fbtmp->Columns);

    //  FFT performs channel construction and alias elimination
    fbtmp->fb_fft = fftwf_plan_dft_1d(fbtmp->Partitions,
				      (fftwf_complex *) fbtmp->in,
				      (fftwf_complex *) fbtmp->out,
				      FFTW_MEASURE | FFTW_FORWARD,
				      wisdombits);


    // Partition the filter here
    for (index = 0; index < FilterSize; index++) {
      int i_index, j_index;
      i_index = index % fbtmp->Partitions;
      j_index = (index - i_index) / fbtmp->Partitions;
      gsl_matrix_float_set(fbtmp->filter_partition,
			   i_index,
			   j_index,
			   fbtmp->fbfil->coef[index]);
    }
  } else {
    // Synthesis
  }
  return fbtmp;
}

// Filterbank in this form is called ONCE with a vector of complex
// samples "Partitions" long and the same vector returns the filter
// bank of downsampled and shifted bins filtered by the partitioned
// low pass filter.

/* -------------------------------------------------------------------------- */
/** @brief Run Analysis Filter Bank 
* 
* @param p 
*/
/* ---------------------------------------------------------------------------- */
void
runAnalysisFilterBank(FilterBank p)
{
  size_t i, j;
  for (i = 0; i < p->Partitions; i++)
    gsl_matrix_complex_float_set(p->data_partition, i, 0, p->databuf[i]);
  for (i = 0; i < p->Partitions; i++) {
    float tmp;
    gsl_complex_float tmpz;
    gsl_complex_float *datptr = &p->in[i];
    tmp = gsl_matrix_float_get(p->filter_partition, i, 0);
    tmpz = gsl_matrix_complex_float_get(p->data_partition, i, 0);
    GSL_SET_COMPLEX(datptr, GSL_REAL(tmpz) * tmp, GSL_IMAG(tmpz) * tmp);	// initialize sum for filter partition
    for (j = 0; j < p->Columns; j++) {
      tmp = gsl_matrix_float_get(p->filter_partition, i, j);
      tmpz = gsl_matrix_complex_float_get(p->data_partition, i, j);
      datptr->dat[0] += tmp * tmpz.dat[0];
      datptr->dat[1] += tmp * tmpz.dat[1];
    }
  }
  fftwf_execute(p->fb_fft);
  for (i = 0; i < p->Partitions; i++)
    for (j = p->Columns - 2; j >= 0; j--) {}
  // Copy the submatrices in the correct order for the partitions.
  // REPLACE WITH CIRCULAR BUFFERS!!!!
}

/* -------------------------------------------------------------------------- */
/** @brief Delete Filter Bank 
* 
* @param p 
*/
/* ---------------------------------------------------------------------------- */
void
delFilterBank(FilterBank p)
{
  if (p->MyFilt)
    delFIR_REAL(p->fbfil);
  fftwf_destroy_plan(p->fb_fft);
  gsl_matrix_float_free(p->filter_partition);
  gsl_matrix_complex_float_free(p->data_partition);
  safefree((char *) p->in);
  safefree((char *) p);
}
