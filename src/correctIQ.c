/* correctIQ.c

This routine restores quadrature between arms of an analytic signal
possibly distorted by ADC hardware.

This file is part of a program that implements a Software-Defined Radio.

Copyright (C) 2004, 2005, 2006 by Frank Brickle, AB2KT and Bob McGwier, N4HY

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
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
rwmcgwier@comcast.net

or by paper mail at

The DTTS Microwave Society
6 Kathleen Place
Bridgewater, NJ 08807
*/

#include <common.h>

IQ
newCorrectIQ (REAL phase, REAL gain)
{
	IQ iq = (IQ) safealloc (1, sizeof (iqstate), "IQ state");
	int i;															// SV1EIA AIR
	for ( i = 0; i < DEFSPEC; i++)									// SV1EIA AIR
	{																// SV1EIA AIR
		iq->phase[i] = phase;										// SV1EIA AIR
		iq->gain[i] = gain;											// SV1EIA AIR
	}																// SV1EIA AIR
	return iq;
}

void
delCorrectIQ (IQ iq)
{
	safefree ((char *) iq);
}

void
correctIQ (CXB sigbuf, IQ iq)
{
	int i;															// SV1EIA AIR
	if (CXBhave (sigbuf)!= DEFSPEC)									// SV1EIA AIR
	{																// SV1EIA AIR
		if (iq->buffer_counter>DEFSPEC-1) iq->buffer_counter = 0;	// SV1EIA AIR
		iq->buffer_length[iq->buffer_counter] = CXBhave (sigbuf);	// SV1EIA AIR
		iq->buffer_counter++;										// SV1EIA AIR
	}																// SV1EIA AIR
	iq->im_max_actual = 0;											// SV1EIA AIR
	iq->im_max_test = 0;											// SV1EIA AIR
	iq->im_min_actual = 0;											// SV1EIA AIR
	iq->im_min_test = 0;											// SV1EIA AIR
	iq->re_max_actual = 0;											// SV1EIA AIR
	iq->re_max_test = 0;											// SV1EIA AIR
	iq->re_min_actual = 0;											// SV1EIA AIR
	iq->re_min_test = 0;											// SV1EIA AIR

	for (i = 0; i < CXBhave (sigbuf); i++)							// SV1EIA AIR
	{																// SV1EIA AIR
		if (CXBimag (sigbuf, i) >= iq->im_max_actual)				// SV1EIA AIR
		{															// SV1EIA AIR
			iq->im_max_actual = CXBimag (sigbuf, i);				// SV1EIA AIR
			iq->im_max_bin_actual = i;								// SV1EIA AIR
		}															// SV1EIA AIR
		if (CXBreal (sigbuf, i) >= iq->re_max_actual)				// SV1EIA AIR
		{															// SV1EIA AIR
			iq->re_max_actual = CXBreal (sigbuf, i);				// SV1EIA AIR
			iq->re_max_bin_actual = i;								// SV1EIA AIR
		}															// SV1EIA AIR
		if (CXBimag (sigbuf, i) <= iq->im_min_actual)				// SV1EIA AIR
		{															// SV1EIA AIR
			iq->im_min_actual = CXBimag (sigbuf, i);				// SV1EIA AIR
			iq->im_min_bin_actual = i;								// SV1EIA AIR
		}															// SV1EIA AIR
		if (CXBreal (sigbuf, i) <= iq->re_min_actual)				// SV1EIA AIR
		{															// SV1EIA AIR
			iq->re_min_actual = CXBreal (sigbuf, i);				// SV1EIA AIR
			iq->re_min_bin_actual = i;								// SV1EIA AIR
		}															// SV1EIA AIR
		iq->im_actual[i] = CXBimag (sigbuf, i);						// SV1EIA AIR
		iq->re_actual[i] = CXBreal (sigbuf, i);						// SV1EIA AIR
		CXBimag (sigbuf, i) += iq->phase[i] * CXBreal (sigbuf, i);	// SV1EIA AIR
		CXBreal (sigbuf, i) *= iq->gain[i];							// SV1EIA AIR
		iq->im_test[i] = CXBimag (sigbuf, i);						// SV1EIA AIR
		iq->re_test[i] = CXBreal (sigbuf, i);						// SV1EIA AIR
		if (CXBimag (sigbuf, i) >= iq->im_max_test)					// SV1EIA AIR
		{															// SV1EIA AIR
			iq->im_max_test = CXBimag (sigbuf, i);					// SV1EIA AIR
			iq->im_max_bin_test = i;								// SV1EIA AIR
		}															// SV1EIA AIR
		if (CXBreal (sigbuf, i) >= iq->re_max_test)					// SV1EIA AIR
		{															// SV1EIA AIR
			iq->re_max_test = CXBreal (sigbuf, i);					// SV1EIA AIR
			iq->re_max_bin_test = i;								// SV1EIA AIR
		}															// SV1EIA AIR
		if (CXBimag (sigbuf, i) <= iq->im_min_test)					// SV1EIA AIR
		{															// SV1EIA AIR
			iq->im_min_test = CXBimag (sigbuf, i);					// SV1EIA AIR
			iq->im_min_bin_test = i;								// SV1EIA AIR
		}															// SV1EIA AIR
		if (CXBreal (sigbuf, i) <= iq->re_min_test)					// SV1EIA AIR
		{															// SV1EIA AIR
			iq->re_min_test = CXBreal (sigbuf, i);					// SV1EIA AIR
			iq->re_min_bin_test = i;								// SV1EIA AIR
		}															// SV1EIA AIR
	}																// SV1EIA AIR
//	if (iq->spec > 0)
//	{
//	}
}

correctIQspec (CXB sigbuf, IQ iq)
{
	int i;															// SV1EIA AIR
	if (CXBhave (sigbuf)!= DEFSPEC)									// SV1EIA AIR
	{																// SV1EIA AIR
		if (iq->buffer_counter>DEFSPEC-1) iq->buffer_counter = 0;	// SV1EIA AIR
		iq->buffer_length[iq->buffer_counter] = CXBhave (sigbuf);	// SV1EIA AIR
		iq->buffer_counter++;										// SV1EIA AIR
	}																// SV1EIA AIR

	iq->im_max_actual = 0;											// SV1EIA AIR
	iq->im_max_test = 0;											// SV1EIA AIR
	iq->im_min_actual = 0;											// SV1EIA AIR
	iq->im_min_test = 0;											// SV1EIA AIR
	iq->re_max_actual = 0;											// SV1EIA AIR
	iq->re_max_test = 0;											// SV1EIA AIR
	iq->re_min_actual = 0;											// SV1EIA AIR
	iq->re_min_test = 0;											// SV1EIA AIR

	for (i = 0; i < CXBhave (sigbuf); i++)							// SV1EIA AIR
	{																// SV1EIA AIR
		if (CXBimag (sigbuf, i) >= iq->im_max_actual)				// SV1EIA AIR
		{															// SV1EIA AIR
			iq->im_max_actual = CXBimag (sigbuf, i);				// SV1EIA AIR
			iq->im_max_bin_actual = i;								// SV1EIA AIR
		}															// SV1EIA AIR
		if (CXBreal (sigbuf, i) >= iq->re_max_actual)				// SV1EIA AIR
		{															// SV1EIA AIR
			iq->re_max_actual = CXBreal (sigbuf, i);				// SV1EIA AIR
			iq->re_max_bin_actual = i;								// SV1EIA AIR
		}															// SV1EIA AIR
		if (CXBimag (sigbuf, i) <= iq->im_min_actual)				// SV1EIA AIR
		{															// SV1EIA AIR
			iq->im_min_actual = CXBimag (sigbuf, i);				// SV1EIA AIR
			iq->im_min_bin_actual = i;								// SV1EIA AIR
		}															// SV1EIA AIR
		if (CXBreal (sigbuf, i) <= iq->re_min_actual)				// SV1EIA AIR
		{															// SV1EIA AIR
			iq->re_min_actual = CXBreal (sigbuf, i);				// SV1EIA AIR
			iq->re_min_bin_actual = i;								// SV1EIA AIR
		}															// SV1EIA AIR
		iq->im_actual[i] = CXBimag (sigbuf, i);						// SV1EIA AIR
		iq->re_actual[i] = CXBreal (sigbuf, i);						// SV1EIA AIR
		CXBimag (sigbuf, i) += iq->phase[i] * CXBreal (sigbuf, i);	// SV1EIA AIR
		CXBreal (sigbuf, i) *= iq->gain[i];							// SV1EIA AIR
		iq->im_test[i] = CXBimag (sigbuf, i);						// SV1EIA AIR
		iq->re_test[i] = CXBreal (sigbuf, i);						// SV1EIA AIR
		if (CXBimag (sigbuf, i) >= iq->im_max_test)					// SV1EIA AIR
		{															// SV1EIA AIR
			iq->im_max_test = CXBimag (sigbuf, i);					// SV1EIA AIR
			iq->im_max_bin_test = i;								// SV1EIA AIR
		}															// SV1EIA AIR
		if (CXBreal (sigbuf, i) >= iq->re_max_test)					// SV1EIA AIR
		{															// SV1EIA AIR
			iq->re_max_test = CXBreal (sigbuf, i);					// SV1EIA AIR
			iq->re_max_bin_test = i;								// SV1EIA AIR
		}															// SV1EIA AIR
		if (CXBimag (sigbuf, i) <= iq->im_min_test)					// SV1EIA AIR
		{															// SV1EIA AIR
			iq->im_min_test = CXBimag (sigbuf, i);					// SV1EIA AIR
			iq->im_min_bin_test = i;								// SV1EIA AIR
		}															// SV1EIA AIR
		if (CXBreal (sigbuf, i) <= iq->re_min_test)					// SV1EIA AIR
		{															// SV1EIA AIR
			iq->re_min_test = CXBreal (sigbuf, i);					// SV1EIA AIR
			iq->re_min_bin_test = i;								// SV1EIA AIR
		}															// SV1EIA AIR
	}																// SV1EIA AIR
	if (iq->spec > 0)
	{
	}
}

void																// SV1EIA AIR
correctIQbin (IQ iq, int bin_position, REAL phase, REAL gain)		// SV1EIA AIR
{																	// SV1EIA AIR
	if (bin_position > -1 && bin_position < DEFSPEC)				// SV1EIA AIR
	{																// SV1EIA AIR
		iq->phase[bin_position] = phase;							// SV1EIA AIR
		iq->gain[bin_position] = gain;								// SV1EIA AIR
	}																// SV1EIA AIR
}																	// SV1EIA AIR

IQ
newCorrectIQspec (REAL phase, REAL gain)
{
	IQ iq = (IQ) safealloc (1, sizeof (iqstate), "IQ state");
	int i;															// SV1EIA AIR
	for ( i = 0; i < DEFSPEC; i++)									// SV1EIA AIR
	{																// SV1EIA AIR
		iq->phase[i] = phase;										// SV1EIA AIR
		iq->gain[i] = gain;											// SV1EIA AIR
	}																// SV1EIA AIR
	return iq;
}

newCorrectIQspec2 ()
{
	IQ iq = (IQ) safealloc (1, sizeof (iqstate), "IQ state");
	int i;															// SV1EIA AIR
	for ( i = 0; i < DEFSPEC; i++)									// SV1EIA AIR
	{																// SV1EIA AIR
		iq->phase[i] = 0.0;										// SV1EIA AIR
		iq->gain[i] = 1.0;											// SV1EIA AIR
	}																// SV1EIA AIR
	return iq;
}
