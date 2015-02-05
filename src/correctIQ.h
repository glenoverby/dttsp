#ifndef _correctIQ_h
#define _correctIQ_h

#include <bufvec.h>

typedef struct _iqstate
{
	REAL	phase[DEFSPEC], gain[DEFSPEC],					// SV1EIA AIR
			phase_test[DEFSPEC], gain_test[DEFSPEC],		// SV1EIA AIR
			im_actual[DEFSPEC], re_actual[DEFSPEC],			// SV1EIA AIR
			im_max_actual, re_max_actual,					// SV1EIA AIR
			im_min_actual, re_min_actual,					// SV1EIA AIR
			im_max_test, re_max_test,						// SV1EIA AIR
			im_min_test, re_min_test,						// SV1EIA AIR
			im_test[DEFSPEC], re_test[DEFSPEC];				// SV1EIA AIR
	int		spec, buffer_length[DEFSPEC],					// SV1EIA AIR
			buffer_counter,									// SV1EIA AIR
			re_max_bin_actual, re_min_bin_actual,			// SV1EIA AIR
			im_max_bin_actual, im_min_bin_actual,			// SV1EIA AIR
			re_max_bin_test, re_min_bin_test,				// SV1EIA AIR
			im_max_bin_test, im_min_bin_test;				// SV1EIA AIR
} *IQ, iqstate;

extern IQ newCorrectIQ (REAL phase, REAL gain);
extern void delCorrectIQ (IQ iq);
extern void correctIQ (CXB sigbuf, IQ iq);
extern void correctIQbin (IQ iq, int bin_position, REAL phase, REAL gain);	// SV1EIA AIR

#endif
