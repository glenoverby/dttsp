// stkstuff.h
// DSP algorithms inspired by Stk. From-scratch in C.
// GPL v3

#ifndef _stkstuff_h
#define _stkstuff_h

#include <fromsys.h>
#include <defs.h>
#include <datatypes.h>
#include <bufvec.h>
#include <cxops.h>

typedef struct _stk_filter {
  REAL gain;
  struct { RLB g, b, a, x, y; } fil;
} STKFiltInfo, *STKFilt;

extern STKFilt newSTKFilt(REAL *b, int nb, REAL *a, int na);
extern void delSTKFilt(STKFilt f);
extern void STKFilt_clear(STKFIlt f);
extern void STKFilt_set_coeffs(STKFilt f,
			       REAL *b, int nb, REAL *a, int na,
			       BOOLEAN clearstate);
extern void STKFilt_set_numerator(STKFilt f,
				  REAL *b, int nb,
				  BOOLEAN clearstate);
extern void STKFilt_set_denominator(STKFilt f,
				    REAL *a, int na,
				    BOOLEAN clearstate);
extern void STKFilt_set_gain(STKFilt f, REAL gain);
extern REAL STKFilt_get_gain(STKFilt f);
extern REAL STKFilt_last_out(STKFilt f);
extern REAL STKFilt_tick(STKFilt f, REAL input);

typedef struct _stk_delay {
  REAL gain;
  struct { RLB g, b, a, x, y; } fil;
  struct { int i, o, t; } del;
} STKDelayInfo, *STKDelay;

extern STKDelay newSTKDelay(int delay, int max_delay);
extern void delSTKDelay(STKDelay d);
extern void STKDelay_clear(STKDelay d);
extern void STKDelay_set_delay(STKDelay d, int delay);
extern int  STKDelay_get_delay(STKDelay d);
extern void STKDelay_set_max_delay(STKDelay d, int max_delay);
extern int  STKDelay_get_max_delay(STKDelay d);
extern REAL STKDelay_energy(STKDelay d);
extern REAL STKDelay_contents_at(STKDelay d, int tap);
extern REAL STKDelay_last_out(STKDelay d);
extern REAL STKDelay_next_out(STKDelay d);
extern REAL STKDelay_compute_sample(STKDelay d, REAL input);
extern REAL STKDelay_tick(STKDelay d, REAL input);

typedef struct _stk_delay_a {
  REAL gain;
  struct { RLB g, b, a, x, y; } fil;
  struct {
    struct { int i, o; } pt;
    REAL t;
    struct {
      REAL alpha, coeff, ap_input, next_output;
      BOOLEAN do_next_out;
    } A;
  } del;
} STKDelayAInfo, *STKDelayA;

extern STKDelayA newSTKDelayA(REAL delay, int max_delay);
extern void delSTKDelayA(STKDelayA d);
extern void STKDelayA_clear(STKDelayA d);
extern void STKDelayA_set_delay(STKDelayA d, REAL delay);
extern REAL STKDelayA_get_delay(STKDelayA d);
extern void STKDelayA_set_max_delay(STKDelayA d, int max_delay);
extern REAL STKDelayA_get_max_delay(STKDelayA d);
extern REAL STKDelayA_energy(STKDelayA d);
extern REAL STKDelayA_contents_at(STKDelayA d, int tap_delay);
extern REAL STKDelayA_last_out(STKDelayA d);
extern REAL STKDelayA_next_out(STKDelayA d);
extern REAL STKDelayA_compute_sample(STKDelayA d, REAL input);
extern REAL STKDelayA_tick(STKDelayA d, REAL input);

typedef struct _stk_delay_l {
  REAL gain;
  struct { RLB b, a, x, y; } fil;
  struct {
    struct { int i, o; } pt;
    REAL t;
  } del;
  struct {
    REAL alpha, om_alpha, next_output;
    BOOLEAN do_next_out;
  } delL;
} STKDelayLInfo, *STKDelayL;

extern STKDelayL newSTKDelayL(int delay, int max_delay);
extern void delSTKDelayL(STKDelayL d);
extern void STKDelayL_clear(STKDelayL d);
extern void STKDelayL_set_delay(STKDelayL d, int delay);
extern REAL STKDelayL_get_delay(STKDelayL d);
extern void STKDelayL_set_max_delay(STKDelayL d, int max_delay);
extern REAL STKDelayL_get_max_delay(STKDelayL d);
extern REAL STKDelayL_energy(STKDelayL d);
extern REAL STKDelayL_contents_at(STKDelayL d, int tap_delay);
extern REAL STKDelayL_last_out(STKDelayL d);
extern REAL STKDelayL_next_out(STKDelayL d);
extern REAL STKDelayL_compute_sample(STKDelayL d, REAL input);
extern REAL STKDelayL_tick(STKDelayL d, REAL input);

#endif
