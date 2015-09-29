/** 
* @file update.c
* @brief Functions to update radio parameters 
* @author Frank Brickle, AB2KT and Bob McGwier, N4HY 

common defs and code for parm update 
   
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

#include <common.h>

////////////////////////////////////////////////////////////////////////////
/// for commands affecting RX, which RX is Listening

#define RL (uni->multirx.lis)

////////////////////////////////////////////////////////////////////////////

/* -------------------------------------------------------------------------- */
/** @brief private db2lin 
* 
* @param dB 
* @return REAL 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE REAL INLINE
dB2lin(REAL dB) { return pow(10.0, dB / 20.0); }

/* -------------------------------------------------------------------------- */
/** @brief private setRXFilter 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setRXFilter(int n, char **p) {
  REAL low_frequency = atof(p[0]),
       high_frequency = atof(p[1]);
  int i, ncoef = rx[RL]->len + 1, fftlen = 2 * rx[RL]->len;
  fftwf_plan ptmp;
  COMPLEX *zcvec;

  if (fabs(low_frequency) >= 0.5 * uni->rate.sample)
    return -1;
  if (fabs(high_frequency) >= 0.5 * uni->rate.sample)
    return -2;
  if ((low_frequency + 10) >= high_frequency)
    return -3;
  delFIR_COMPLEX(rx[RL]->filt.coef);

#if 0
  fprintf(stderr, "setRXFilter %f %f\n", low_frequency, high_frequency);
#endif
  
  rx[RL]->filt.lo = low_frequency;
  rx[RL]->filt.hi = high_frequency;

  rx[RL]->filt.coef = newFIR_Bandpass_COMPLEX(low_frequency,
					      high_frequency,
					      uni->rate.sample,
					      ncoef);

  zcvec = newvec_COMPLEX(fftlen, "filter z vec in setFilter");
  ptmp = fftwf_plan_dft_1d(fftlen,
			   (fftwf_complex *) zcvec,
			   (fftwf_complex *) rx[RL]->filt.ovsv->zfvec,
			   FFTW_FORWARD,
			   uni->wisdom.bits);
#ifdef LHS
  for (i = 0; i < ncoef; i++)
    zcvec[i] = rx[RL]->filt.coef->coef[i];
#else
  for (i = 0; i < ncoef; i++)
    zcvec[fftlen - ncoef + i] = rx[RL]->filt.coef->coef[i];
#endif
  fftwf_execute(ptmp);
  fftwf_destroy_plan(ptmp);
  delvec_COMPLEX(zcvec);
  normalize_vec_COMPLEX(rx[RL]->filt.ovsv->zfvec, rx[RL]->filt.ovsv->fftlen);
  memcpy((char *) rx[RL]->filt.save, (char *) rx[RL]->filt.ovsv->zfvec,
	 rx[RL]->filt.ovsv->fftlen * sizeof(COMPLEX));

  return 0;
}

PRIVATE int
getRXFilter(int n, char **p) {
  sprintf(top->resp.buff, "getRXFilter %f %f\n",
	  rx[RL]->filt.lo, rx[RL]->filt.hi);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setRXFiltCoefs 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setRXFiltCoefs(int n, char **p) {
  int i, j, ncoef = rx[RL]->len + 1, fftlen = 2 * rx[RL]->len;
  fftwf_plan ptmp;
  COMPLEX *zcvec;

  delFIR_COMPLEX(rx[RL]->filt.coef);

  rx[RL]->filt.coef = newFIR_COMPLEX(ncoef, "setRXFiltCoefs");

  for (i = j = 0; i < ncoef && j < n; i++, j += 2)
    FIRtap(rx[RL]->filt.coef, i) = Cmplx(atof(p[j]), atof(p[j + 1]));
  for (; i < ncoef; i++)
    FIRtap(rx[RL]->filt.coef, i) = cxzero;

  zcvec = newvec_COMPLEX(fftlen, "filter z vec in setFilter");
  ptmp = fftwf_plan_dft_1d(fftlen,
			   (fftwf_complex *) zcvec,
			   (fftwf_complex *) rx[RL]->filt.ovsv->zfvec,
			   FFTW_FORWARD,
			   uni->wisdom.bits);

#ifdef LHS
  for (i = 0; i < ncoef; i++)
    zcvec[i] = FIRtap(rx[RL]->filt.coef, i);
#else
  for (i = 0; i < ncoef; i++)
    zcvec[fftlen - ncoef + i] = FIRtap(rx[RL]->filt.coef, i);
#endif

  fftwf_execute(ptmp);
  fftwf_destroy_plan(ptmp);
  delvec_COMPLEX(zcvec);
  normalize_vec_COMPLEX(rx[RL]->filt.ovsv->zfvec, rx[RL]->filt.ovsv->fftlen);
  memcpy((char *) rx[RL]->filt.save,
	 (char *) rx[RL]->filt.ovsv->zfvec,
	 rx[RL]->filt.ovsv->fftlen * sizeof(COMPLEX));

  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setTXFilter 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setTXFilter(int n, char **p) {
  REAL low_frequency = atof(p[0]),
       high_frequency = atof(p[1]);
  int i, ncoef = tx->len + 1, fftlen = 2 * tx->len;
  fftwf_plan ptmp;
  COMPLEX *zcvec;

  if (fabs(low_frequency) >= 0.5 * uni->rate.sample)
    return -1;
  if (fabs(high_frequency) >= 0.5 * uni->rate.sample)
    return -2;
  if ((low_frequency + 10) >= high_frequency)
    return -3;

  tx->filt.lo = low_frequency;
  tx->filt.hi = high_frequency;

  delFIR_COMPLEX(tx->filt.coef);
  tx->filt.coef = newFIR_Bandpass_COMPLEX(low_frequency,
					  high_frequency,
					  uni->rate.sample,
					  ncoef);

  zcvec = newvec_COMPLEX(fftlen, "filter z vec in setFilter");
  ptmp = fftwf_plan_dft_1d(fftlen,
			   (fftwf_complex *) zcvec,
			   (fftwf_complex *) tx->filt.ovsv->zfvec,
			   FFTW_FORWARD,
			   uni->wisdom.bits);

#ifdef LHS
  for (i = 0; i < ncoef; i++)
    zcvec[i] = tx->filt.coef->coef[i];
#else
  for (i = 0; i < ncoef; i++)
    zcvec[fftlen - ncoef + i] = tx->filt.coef->coef[i];
#endif
  fftwf_execute(ptmp);
  fftwf_destroy_plan(ptmp);
  delvec_COMPLEX(zcvec);
  normalize_vec_COMPLEX(tx->filt.ovsv->zfvec, tx->filt.ovsv->fftlen);
  memcpy((char *) tx->filt.save,
	 (char *) tx->filt.ovsv->zfvec,
	 tx->filt.ovsv->fftlen * sizeof(COMPLEX));

  return 0;
}

PRIVATE int
getTXFilter(int n, char **p) {
  sprintf(top->resp.buff, "getTXFilter %f %f\n",
	  tx->filt.lo, tx->filt.hi);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setTXFiltCoefs 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setTXFiltCoefs(int n, char **p) {
  int i, j, ncoef = tx->len + 1, fftlen = 2 * tx->len;
  fftwf_plan ptmp;
  COMPLEX *zcvec;

  delFIR_COMPLEX(tx->filt.coef);

  tx->filt.coef = newFIR_COMPLEX(ncoef, "setRXFiltCoefs");

  for (i = j = 0; i < ncoef && j < n; i++, j += 2)
    FIRtap(tx->filt.coef, i) = Cmplx(atof(p[j]), atof(p[j + 1]));
  for (; i < ncoef; i++)
    FIRtap(tx->filt.coef, i) = cxzero;

  zcvec = newvec_COMPLEX(fftlen, "filter z vec in setFilter");
  ptmp = fftwf_plan_dft_1d(fftlen,
			   (fftwf_complex *) zcvec,
			   (fftwf_complex *) tx->filt.ovsv->zfvec,
			   FFTW_FORWARD,
			   uni->wisdom.bits);

#ifdef LHS
  for (i = 0; i < ncoef; i++)
    zcvec[i] = FIRtap(tx->filt.coef, i);
#else
  for (i = 0; i < ncoef; i++)
    zcvec[fftlen - ncoef + i] = FIRtap(tx->filt.coef, i);
#endif

  fftwf_execute(ptmp);
  fftwf_destroy_plan(ptmp);
  delvec_COMPLEX(zcvec);
  normalize_vec_COMPLEX(rx[RL]->filt.ovsv->zfvec, tx->filt.ovsv->fftlen);
  memcpy((char *) tx->filt.save,
	 (char *) tx->filt.ovsv->zfvec,
	 tx->filt.ovsv->fftlen * sizeof(COMPLEX));

  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setFilter 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setFilter(int n, char **p) {
  if (n == 2)
    return setRXFilter(n, p);
  else {
    int trx = atoi(p[2]);
    if (trx == RX)
      return setRXFilter(n, p);
    else if (trx == TX)
      return setTXFilter(n, p);
    else
      return -1;
  }
}

/* -------------------------------------------------------------------------- */
/** @brief private setMode 
* 
* setMode <mode> [TRX]
*
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setMode(int n, char **p) {
  int mode = atoi(p[0]);
  if (n > 1) {
    int trx = atoi(p[1]);
    switch (trx) {
    case TX:
      tx->mode = mode;
      break;
    case RX:
    default:
      rx[RL]->mode = mode;
      break;
    }
  } else
    tx->mode = rx[RL]->mode = uni->mode.sdr = mode;
  if (rx[RL]->mode == AM)
    rx[RL]->am.gen->mode = AMdet;
  if (rx[RL]->mode == SAM)
    rx[RL]->am.gen->mode = SAMdet;
  return 0;
}

PRIVATE int
getRXMode(int n, char **p) {
  sprintf(top->resp.buff, "getRXMode %d\n", rx[RL]->mode);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

PRIVATE int
getTXMode(int n, char **p) {
  sprintf(top->resp.buff, "getTXMode %d\n", tx->mode);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setOSC 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setOsc(int n, char **p) {
  double newfreq = atof(p[0]);
  if (fabs(newfreq) >= 0.5 * uni->rate.sample)
    return -1;
  newfreq *= 2.0 * M_PI / uni->rate.sample;
  if (n > 1) {
    int trx = atoi(p[1]);
    switch (trx) {
    case TX:
      tx->osc.gen->Frequency = newfreq;
      break;
    case RX:
    default:
      rx[RL]->osc.gen->Frequency = newfreq;
      break;
    }
  } else
    tx->osc.gen->Frequency = rx[RL]->osc.gen->Frequency = newfreq;
  return 0;
}

PRIVATE int
getRXOsc(int n, char **p) {
  sprintf(top->resp.buff, "getRXOsc %f\n", rx[RL]->osc.gen->Frequency);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

PRIVATE int
getTXOsc(int n, char **p) {
  sprintf(top->resp.buff, "getTXOsc %f\n", tx->osc.gen->Frequency);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private replay_update 
*
* @return void 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE void
replay_updates(void) {
  char _str[4096], *str = _str;
  splitfld _splt, *splt = &_splt;
  BOOLEAN quiet = FALSE,
          switcheroo = FALSE;
  int oldRL, tmpRL;
  extern CTE update_cmds[];
  FILE *log;

  log = top->verbose ? stderr : 0;

  // left back at end after rewind
  // and subsequent reads
  rewind(uni->update.fp);

  while (fgets(str, 4096, uni->update.fp)) {

    // echo to logging output?
    if (*str == '-') {
      quiet = TRUE;
      str++;
    } else
      quiet = FALSE;

    // do the old switcheroo?
    // ie, temporary redirect of a command to another RX
    if (*str == '@') {
      char *endp;
      tmpRL = strtol(++str, &endp, 10);
      if (tmpRL < 0 || tmpRL >= uni->multirx.nrx)
	continue;
      while (*endp && isspace(*endp))
	endp++;
      str = endp;
      switcheroo = TRUE;
    }

    split(splt, str);

    if (NF(splt) < 1)
      continue;

    else {
      Thunk thk = Thunk_lookup(update_cmds, F(splt, 0));
      if (!thk)
	continue;

      else {
	int val;

	if (switcheroo)
	  oldRL = RL, RL = tmpRL;

	val = (*thk)(NF(splt) - 1, Fptr(splt, 1));

	if (switcheroo)
	  RL = oldRL;

	if (log && !quiet) {
	  int i;
	  char *s = since(&top->start_tv);
	  fprintf(log, "%s replay[%s]: returned %d from", top->snds.name, s, val);
	  for (i = 0; i < NF(splt); i++)
	    fprintf(log, " %s", F(splt, i));
	  putc('\n', log);
	  fflush(log);
	}
	// discard val
      }
    }
  }
}

/* -------------------------------------------------------------------------- */
/** @brief private setBlkNR 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setBlkNR(int n, char **p) {
  rx[RL]->banr.flag = atoi(p[0]);
  return 0;
}

PRIVATE int
getBlkNR(int n, char **p) {
  sprintf(top->resp.buff, "getBlkNR %d %f\n",
	  rx[RL]->banr.flag,
	  rx[RL]->banr.gen->adaptation_rate);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setBlkNRval 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setBlkNRval(int n, char **p) {
  REAL adaptation_rate = atof(p[0]);
  rx[RL]->banr.gen->adaptation_rate = adaptation_rate;
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setBlkANF 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setBlkANF(int n, char **p) {
  rx[RL]->banf.flag = atoi(p[0]);
  return 0;
}

PRIVATE int
getBlkANF(int n, char **p) {
  sprintf(top->resp.buff, "getBlkANF %d %f\n",
	  rx[RL]->banf.flag,
	  rx[RL]->banf.gen->adaptation_rate);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setBlkANFval 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setBlkANFval(int n, char **p) {
  REAL adaptation_rate = atof(p[0]);
  rx[RL]->banf.gen->adaptation_rate = adaptation_rate;
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setNB 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setNB(int n, char **p) {
  rx[RL]->nb.flag = atoi(p[0]);
  return 0;
}

PRIVATE int
getNB(int n, char **p) {
  sprintf(top->resp.buff, "getNB %d %f\n",
	  rx[RL]->nb.flag,
	  rx[RL]->nb.gen->threshold);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setNBvals 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setNBvals(int n, char **p) {
  REAL threshold = atof(p[0]);
  rx[RL]->nb.gen->threshold = rx[RL]->nb.thresh = threshold;
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setSDROM 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setSDROM(int n, char **p) {
  rx[RL]->nb_sdrom.flag = atoi(p[0]);
  return 0;
}

PRIVATE int
getSDROM(int n, char **p) {
  sprintf(top->resp.buff, "getSDROM %d %f\n",
	  rx[RL]->nb_sdrom.flag,
	  rx[RL]->nb_sdrom.gen->threshold);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setSDROMvals 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setSDROMvals(int n, char **p) {
  REAL threshold = atof(p[0]);
  rx[RL]->nb_sdrom.gen->threshold = rx[RL]->nb_sdrom.thresh = threshold;
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setBIN 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setBIN(int n, char **p) {
  rx[RL]->bin.flag = atoi(p[0]);
  return 0;
}

PRIVATE int
getBIN(int n, char **p) {
  sprintf(top->resp.buff, "getBIN %d\n", rx[RL]->bin.flag);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setfixedAGC 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setfixedAGC(int n, char **p) {
  REAL gain = atof(p[0]);
  if (n > 1) {
    int trx = atoi(p[1]);
    switch (trx) {
    case TX:
      tx->leveler.gen->gain.now = gain;
      break;
    case RX:
    default:
      rx[RL]->dttspagc.gen->gain.now = gain;
      break;
    }
  } else
    tx->leveler.gen->gain.now = rx[RL]->dttspagc.gen->gain.now = gain;
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setRXAGCCompression 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setRXAGCCompression(int n, char **p) {
  REAL rxcompression = atof(p[0]);
  rx[RL]->dttspagc.gen->gain.top = pow(10.0, rxcompression * 0.05);
  return 0;
}

PRIVATE int
getRXAGC(int n, char **p) {
  sprintf(top->resp.buff,
	  "getRXAGC %d %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f\n",
	  rx[RL]->dttspagc.flag,
	  rx[RL]->dttspagc.gen->gain.bottom,
	  rx[RL]->dttspagc.gen->gain.fix,
	  rx[RL]->dttspagc.gen->gain.limit,
	  rx[RL]->dttspagc.gen->gain.top,
	  rx[RL]->dttspagc.gen->fastgain.bottom,
	  rx[RL]->dttspagc.gen->fastgain.fix,
	  rx[RL]->dttspagc.gen->fastgain.limit,
	  rx[RL]->dttspagc.gen->attack,
	  rx[RL]->dttspagc.gen->decay,
	  rx[RL]->dttspagc.gen->fastattack,
	  rx[RL]->dttspagc.gen->fastdecay,
	  rx[RL]->dttspagc.gen->fasthangtime,
	  rx[RL]->dttspagc.gen->hangthresh,
	  rx[RL]->dttspagc.gen->hangtime,
	  rx[RL]->dttspagc.gen->slope);
	  
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setTXLevelerAttack 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setTXLevelerAttack(int n, char **p) {
  REAL tmp = atof(p[0]);
  tx->leveler.gen->attack = 1.0 - exp(-1000.0 / (tmp * uni->rate.sample));
  tx->leveler.gen->one_m_attack = exp(-1000.0 / (tmp * uni->rate.sample));
  tx->leveler.gen->sndx =
    (tx->leveler.gen->indx +
     (int) (0.003 * uni->rate.sample * tmp)) & tx->leveler.gen->mask;
  tx->leveler.gen->fastindx =
    (tx->leveler.gen->sndx +
     FASTLEAD * tx->leveler.gen->mask) & tx->leveler.gen->mask;
  tx->leveler.gen->fasthangtime = 0.1;	//wa6ahl: 100 ms
  return 0;
}

PRIVATE int
getTXLeveler(int n, char **p) {
  sprintf(top->resp.buff,
	  "getTXLeveler %d %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f\n",
	  tx->leveler.flag,
	  tx->leveler.gen->gain.bottom,
	  tx->leveler.gen->gain.fix,
	  tx->leveler.gen->gain.limit,
	  tx->leveler.gen->gain.top,
	  tx->leveler.gen->fastgain.bottom,
	  tx->leveler.gen->fastgain.fix,
	  tx->leveler.gen->fastgain.limit,
	  tx->leveler.gen->attack,
	  tx->leveler.gen->decay,
	  tx->leveler.gen->fastattack,
	  tx->leveler.gen->fastdecay,
	  tx->leveler.gen->fasthangtime,
	  tx->leveler.gen->hangthresh,
	  tx->leveler.gen->hangtime,
	  tx->leveler.gen->slope);
	  
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setTXLevelerSt 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setTXLevelerSt(int n, char **p) {
  BOOLEAN tmp = atoi(p[0]);
  tx->leveler.flag = tmp;
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setTXLevelerDecay 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setTXLevelerDecay(int n, char **p) {
  REAL tmp = atof(p[0]);
  tx->leveler.gen->decay = 1.0 - exp(-1000.0 / (tmp * uni->rate.sample));
  tx->leveler.gen->one_m_decay = exp(-1000.0 / (tmp * uni->rate.sample));
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setTXLevelerTop 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setTXLevelerTop(int n, char **p) {
  REAL top = atof(p[0]);
  tx->leveler.gen->gain.top = top;
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setTXLevelerHang 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setTXLevelerHang(int n, char **p) {
  REAL hang = atof(p[0]);
  tx->leveler.gen->hangtime = 0.001 * hang;
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setRXAGC 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setRXAGC(int n, char **p) {
  int setit = atoi(p[0]);
  rx[RL]->dttspagc.gen->mode = 1;
  rx[RL]->dttspagc.gen->attack = 1.0 - exp(-1000 / (2.0 * uni->rate.sample));
  rx[RL]->dttspagc.gen->one_m_attack = 1.0 - rx[RL]->dttspagc.gen->attack;
  rx[RL]->dttspagc.gen->hangindex = rx[RL]->dttspagc.gen->indx = 0;
  rx[RL]->dttspagc.gen->sndx = (int) (uni->rate.sample * 0.006f);
  rx[RL]->dttspagc.gen->fastindx = FASTLEAD;
  switch (setit) {
  case agcOFF:
    rx[RL]->dttspagc.gen->mode = agcOFF;
    rx[RL]->dttspagc.flag = TRUE;
    break;
  case agcSLOW:
    rx[RL]->dttspagc.gen->mode = agcSLOW;
    rx[RL]->dttspagc.gen->hangtime = 0.5;
    rx[RL]->dttspagc.gen->fasthangtime = 0.1;
    rx[RL]->dttspagc.gen->decay = 1.0 - exp(-1000 / (500.0 * uni->rate.sample));
    rx[RL]->dttspagc.gen->one_m_decay = 1.0 - rx[RL]->dttspagc.gen->decay;
    rx[RL]->dttspagc.flag = TRUE;
    break;
  case agcMED:
    rx[RL]->dttspagc.gen->mode = agcMED;
    rx[RL]->dttspagc.gen->hangtime = 0.25;
    rx[RL]->dttspagc.gen->fasthangtime = 0.1;
    rx[RL]->dttspagc.gen->decay = 1.0 - exp(-1000 / (250.0 * uni->rate.sample));
    rx[RL]->dttspagc.gen->one_m_decay = 1.0 - rx[RL]->dttspagc.gen->decay;
    rx[RL]->dttspagc.flag = TRUE;
    break;
  case agcFAST:
    rx[RL]->dttspagc.gen->mode = agcFAST;
    rx[RL]->dttspagc.gen->hangtime = 0.1;
    rx[RL]->dttspagc.gen->fasthangtime = 0.1;
    rx[RL]->dttspagc.gen->hangtime = 0.1;
    rx[RL]->dttspagc.gen->decay = 1.0 - exp(-1000 / (100.0 * uni->rate.sample));
    rx[RL]->dttspagc.gen->one_m_decay = 1.0 - rx[RL]->dttspagc.gen->decay;
    rx[RL]->dttspagc.flag = TRUE;
    break;
  case agcLONG:
    rx[RL]->dttspagc.gen->mode = agcLONG;
    rx[RL]->dttspagc.flag = TRUE;
    rx[RL]->dttspagc.gen->hangtime = 0.75;
    rx[RL]->dttspagc.gen->fasthangtime = 0.1;
    rx[RL]->dttspagc.gen->decay = 1.0 - exp(-0.5 / uni->rate.sample);
    rx[RL]->dttspagc.gen->one_m_decay = 1.0 - rx[RL]->dttspagc.gen->decay;
    break;
  }
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setRXAGCAttack 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setRXAGCAttack(int n, char **p) {
  REAL tmp = atof(p[0]);
  rx[RL]->dttspagc.gen->mode = 1;
  rx[RL]->dttspagc.gen->hangindex = rx[RL]->dttspagc.gen->indx = 0;
  rx[RL]->dttspagc.gen->sndx = (int) (uni->rate.sample * 0.006);
  rx[RL]->dttspagc.gen->fasthangtime = 0.1;
  rx[RL]->dttspagc.gen->fastindx = FASTLEAD;
  rx[RL]->dttspagc.gen->attack = 1.0 - exp(-1000.0 / (tmp * uni->rate.sample));
  rx[RL]->dttspagc.gen->one_m_attack = exp(-1000.0 / (tmp * uni->rate.sample));
  rx[RL]->dttspagc.gen->sndx = (int) (uni->rate.sample * tmp * 0.003);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setRXAGCDelay 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setRXAGCDecay(int n, char **p) {
  REAL tmp = atof(p[0]);
  rx[RL]->dttspagc.gen->decay = 1.0 - exp(-1000.0 / (tmp * uni->rate.sample));
  rx[RL]->dttspagc.gen->one_m_decay = exp(-1000.0 / (tmp * uni->rate.sample));
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setRXAGCHang 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setRXAGCHang(int n, char **p) {
  REAL hang = atof(p[0]);
  rx[RL]->dttspagc.gen->hangtime = 0.001 * hang;
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setRXAGCSlope 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setRXAGCSlope(int n, char **p) {
  REAL slope = atof(p[0]);
  rx[RL]->dttspagc.gen->slope = dB2lin(0.1 * slope);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setRXAGCHangThreshold 
* 
* @param h 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setRXAGCHangThreshold(int h, char **p) {
  REAL hangthreshold = atof(p[0]);
  rx[RL]->dttspagc.gen->hangthresh = 0.01 * hangthreshold;
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setRXAGCLimit 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setRXAGCLimit(int n, char **p) {
  REAL limit = atof(p[0]);
  rx[RL]->dttspagc.gen->gain.top = limit;
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setRXAGCTop 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setRXAGCTop(int n, char **p) {
  REAL top = atof(p[0]);
  rx[RL]->dttspagc.gen->gain.top = top;
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setRXAGCFix 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setRXAGCFix(int n, char **p) {
  rx[RL]->dttspagc.gen->gain.fix = atof(p[0]);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setfTXAGCFF 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setTXAGCFF(int n, char **p) {
  tx->spr.flag = atoi(p[0]);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setTXAGCFFCompression 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setTXAGCFFCompression(int n, char **p) {
  REAL txcompression = atof(p[0]);
  tx->spr.gen->MaxGain =
    (((0.0000401002 * txcompression) - 0.0032093390) * txcompression + 0.0612862687) * txcompression + 0.9759745718;
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setTXSpeechCompression 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setTXSpeechCompression(int n, char **p) {
  tx->spr.flag = atoi(p[0]);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setTXSpeechCompressionGain 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setTXSpeechCompressionGain(int n, char **p) {
  tx->spr.gen->MaxGain = dB2lin(atof(p[0]));
  return 0;
}

PRIVATE int
getTXSpeechCompression(int n, char **p) {
  sprintf(top->resp.buff, "getTXSpeechCompression %d %f %f\n",
	  tx->spr.flag,
	  tx->spr.gen->K,
	  tx->spr.gen->MaxGain);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

#if 0
PRIVATE int
f2x(REAL f) {
  REAL fix = tx->filt.ovsv->fftlen * f / uni->rate.sample;
  return (int) (fix + 0.5);
}
#endif 

/* -------------------------------------------------------------------------- */
/** @brief private gmean 
* 
* @param x 
* @param y 
* @return REAL 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE REAL
gmean(REAL x, REAL y) { return sqrt(x * y); }

/* -------------------------------------------------------------------------- */
/** @brief private setGrphRXEQ3 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setGrphRXEQ3(int n, char **p) {
  if (n < 4)
    return -1;
  else {
    int i;
    fftwf_plan ptmp;
    ComplexFIR tmpfilt;
    COMPLEX *filtcoef = newvec_COMPLEX(512, "filter for EQ"),
            *tmpcoef  = newvec_COMPLEX(257, "tmp filter for EQ");
    REAL dB = atof(p[0]),
         gain[3],
         preamp = dB2lin(dB) * 0.5;

    rx[RL]->grapheq.parm.size = 3;
    rx[RL]->grapheq.parm.pre = dB;
    for (i = 0; i < 3; i++) {
      rx[RL]->grapheq.parm.gain[i] = atof(p[i + 1]);
      gain[i] = preamp * rx[RL]->grapheq.parm.gain[i];
    }

    tmpfilt = newFIR_Bandpass_COMPLEX(-400, 400, uni->rate.sample, 257);
    for (i = 0; i < 257; i++)
      tmpcoef[i] = Cscl(tmpfilt->coef[i], gain[0]);
    delFIR_Bandpass_COMPLEX(tmpfilt);

    tmpfilt = newFIR_Bandpass_COMPLEX(400, 1500, uni->rate.sample, 257);
    for (i = 0; i < 257; i++)
      tmpcoef[i] = Cadd(tmpcoef[i], Cscl(tmpfilt->coef[i], gain[1]));
    delFIR_Bandpass_COMPLEX(tmpfilt);

    tmpfilt = newFIR_Bandpass_COMPLEX(-1500, -400, uni->rate.sample, 257);
    for (i = 0; i < 257; i++)
      tmpcoef[i] = Cadd(tmpcoef[i], Cscl(tmpfilt->coef[i], gain[1]));
    delFIR_Bandpass_COMPLEX(tmpfilt);

    tmpfilt = newFIR_Bandpass_COMPLEX(1500, 6000, uni->rate.sample, 257);
    for (i = 0; i < 257; i++)
      tmpcoef[i] = Cadd(tmpcoef[i], Cscl(tmpfilt->coef[i], gain[2]));
    delFIR_Bandpass_COMPLEX(tmpfilt);

    tmpfilt = newFIR_Bandpass_COMPLEX(-6000, -1500, uni->rate.sample, 257);
    for (i = 0; i < 257; i++)
      tmpcoef[i] = Cadd(tmpcoef[i], Cscl(tmpfilt->coef[i], gain[2]));
    delFIR_Bandpass_COMPLEX(tmpfilt);

    for (i = 0; i < 257; i++)
      filtcoef[254 + i] = tmpcoef[i];

    ptmp = fftwf_plan_dft_1d(512,
			     (fftwf_complex *) filtcoef,
			     (fftwf_complex *) rx[RL]->grapheq.gen->p->zfvec,
			     FFTW_FORWARD,
			     uni->wisdom.bits);

    fftwf_execute(ptmp);
    fftwf_destroy_plan(ptmp);
    delvec_COMPLEX(filtcoef);
    delvec_COMPLEX(tmpcoef);
  }

  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setGrphRXEQ10 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setGrphRXEQ10(int n, char **p) {
  if (n < 11)
    return -1;
  else {
    int band, i, j;
    fftwf_plan ptmp;
    ComplexFIR tmpfilt;
    COMPLEX *filtcoef = newvec_COMPLEX(512, "filter for EQ"),
            *tmpcoef  = newvec_COMPLEX(257, "tmp filter for EQ");
    REAL dB = atof(p[0]),
         gain[10],
         preamp = dB2lin(dB) * 0.5;

    rx[RL]->grapheq.parm.size = 10;
    rx[RL]->grapheq.parm.pre = dB;

    for (j = 0; j < 10; j++)
      rx[RL]->grapheq.parm.gain[j] = gain[j] = atof(p[j + 1]);

    for (j = 0, band = 15; j < 10; j++, band += 3) {
      REAL f_here  = ISOband_get_nominal(band),
	   f_below = gmean(f_here / 2.0, f_here),
	   f_above = gmean(f_here, f_here * 2.0),
	   g_here  = dB2lin(gain[j]) * preamp;

      tmpfilt = newFIR_Bandpass_COMPLEX(-f_above, -f_below, uni->rate.sample, 257);
      for (i = 0; i < 257; i++)
	tmpcoef[i] = Cadd(tmpcoef[i], Cscl(tmpfilt->coef[i], g_here));
      delFIR_Bandpass_COMPLEX(tmpfilt);

      tmpfilt = newFIR_Bandpass_COMPLEX(f_below, f_above, uni->rate.sample, 257);
      for (i = 0; i < 257; i++)
	tmpcoef[i] = Cadd(tmpcoef[i], Cscl(tmpfilt->coef[i], g_here));
      delFIR_Bandpass_COMPLEX(tmpfilt);
    }

    for (i = 0; i < 257; i++)
      filtcoef[254 + i] = tmpcoef[i];

    ptmp = fftwf_plan_dft_1d(512,
			     (fftwf_complex *) filtcoef,
			     (fftwf_complex *) rx[RL]->grapheq.gen->p->zfvec,
			     FFTW_FORWARD,
			     uni->wisdom.bits);

    fftwf_execute(ptmp);
    fftwf_destroy_plan(ptmp);
    delvec_COMPLEX(filtcoef);
    delvec_COMPLEX(tmpcoef);
  }

  return 0;
}

PRIVATE int
getGrphRXEQ(int n, char **p) {
  if (rx[RL]->grapheq.parm.size == 3)
    sprintf(top->resp.buff,
	    "getGrphRXEQ %d %d %f %f %f %f\n",
	    rx[RL]->grapheq.flag,
	    rx[RL]->grapheq.parm.size,
	    rx[RL]->grapheq.parm.pre,
	    rx[RL]->grapheq.parm.gain[0],
	    rx[RL]->grapheq.parm.gain[1],
	    rx[RL]->grapheq.parm.gain[2]);
  else
    sprintf(top->resp.buff,
	    "getGrphRXEQ %d %d %f %f %f %f %f %f %f %f %f %f %f\n",
	    rx[RL]->grapheq.flag,
	    rx[RL]->grapheq.parm.size,
	    rx[RL]->grapheq.parm.pre,
	    rx[RL]->grapheq.parm.gain[0],
	    rx[RL]->grapheq.parm.gain[1],
	    rx[RL]->grapheq.parm.gain[2],
	    rx[RL]->grapheq.parm.gain[3],
	    rx[RL]->grapheq.parm.gain[4],
	    rx[RL]->grapheq.parm.gain[5],
	    rx[RL]->grapheq.parm.gain[6],
	    rx[RL]->grapheq.parm.gain[7],
	    rx[RL]->grapheq.parm.gain[8],
	    rx[RL]->grapheq.parm.gain[9]);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setGrphTXEQ3 
* 
* @param n 
* @param *p 
* @return int 
*
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setGrphTXEQ3(int n, char **p) {
  if (n < 4)
    return -1;
  else {
    int i;
    fftwf_plan ptmp;
    ComplexFIR tmpfilt;
    COMPLEX *filtcoef = newvec_COMPLEX(512, "filter for EQ"),
            *tmpcoef  = newvec_COMPLEX(257, "tmp filter for EQ");
    REAL dB = atof(p[0]),
         gain[3],
         preamp = dB2lin(dB) * 0.5;

    tx->grapheq.parm.size = 3;
    tx->grapheq.parm.pre = dB;
    for (i = 0; i < 3; i++) {
      tx->grapheq.parm.gain[i] = atof(p[i + 1]);
      gain[i] = preamp * tx->grapheq.parm.gain[i];
    }

    tmpfilt = newFIR_Bandpass_COMPLEX(-400, 400, uni->rate.sample, 257);
    for (i = 0; i < 257; i++)
      tmpcoef[i] = Cscl(tmpfilt->coef[i], gain[0]);
    delFIR_Bandpass_COMPLEX(tmpfilt);

    tmpfilt = newFIR_Bandpass_COMPLEX(400, 1500, uni->rate.sample, 257);
    for (i = 0; i < 257; i++)
      tmpcoef[i] = Cadd(tmpcoef[i], Cscl(tmpfilt->coef[i], gain[1]));
    delFIR_Bandpass_COMPLEX(tmpfilt);

    tmpfilt = newFIR_Bandpass_COMPLEX(-1500, -400, uni->rate.sample, 257);
    for (i = 0; i < 257; i++)
      tmpcoef[i] = Cadd(tmpcoef[i], Cscl(tmpfilt->coef[i], gain[1]));
    delFIR_Bandpass_COMPLEX(tmpfilt);

    tmpfilt = newFIR_Bandpass_COMPLEX(1500, 6000, uni->rate.sample, 257);
    for (i = 0; i < 257; i++)
      tmpcoef[i] = Cadd(tmpcoef[i], Cscl(tmpfilt->coef[i], gain[2]));
    delFIR_Bandpass_COMPLEX(tmpfilt);

    tmpfilt = newFIR_Bandpass_COMPLEX(-6000, -1500, uni->rate.sample, 257);
    for (i = 0; i < 257; i++)
      tmpcoef[i] = Cadd(tmpcoef[i], Cscl(tmpfilt->coef[i], gain[2]));
    delFIR_Bandpass_COMPLEX(tmpfilt);
    for (i = 0; i < 257; i++)
      filtcoef[255 + i] = tmpcoef[i];

    ptmp = fftwf_plan_dft_1d(512,
			     (fftwf_complex *) filtcoef,
			     (fftwf_complex *) tx->grapheq.gen->p->zfvec,
			     FFTW_FORWARD,
			     uni->wisdom.bits);
    fftwf_execute(ptmp);

    fftwf_destroy_plan(ptmp);
    delvec_COMPLEX(filtcoef);
  }

  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setGrphTXEQ10 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setGrphTXEQ10(int n, char **p) {
  if (n < 11)
    return -1;
  else {
    int band, i, j;
    fftwf_plan ptmp;
    ComplexFIR tmpfilt;
    COMPLEX *filtcoef = newvec_COMPLEX(512, "filter for EQ"),
            *tmpcoef  = newvec_COMPLEX(257, "tmp filter for EQ");
    REAL dB = atof(p[0]),
         gain[10],
         preamp = dB2lin(dB) * 0.5;

    tx->grapheq.parm.size = 10;
    tx->grapheq.parm.pre = dB;

    for (j = 0; j < 10; j++)
      tx->grapheq.parm.gain[j] = gain[j] = atof(p[j + 1]);

    for (j = 0, band = 15; j < 10; j++, band += 3) {
      REAL f_here  = ISOband_get_nominal(band),
	   f_below = gmean(f_here / 2.0, f_here),
	   f_above = gmean(f_here, f_here * 2.0),
	   g_here  = dB2lin(gain[j]) * preamp;

      tmpfilt = newFIR_Bandpass_COMPLEX(-f_above, -f_below, uni->rate.sample, 257);
      for (i = 0; i < 257; i++)
	tmpcoef[i] = Cadd(tmpcoef[i], Cscl(tmpfilt->coef[i], g_here));
      delFIR_Bandpass_COMPLEX(tmpfilt);

      tmpfilt = newFIR_Bandpass_COMPLEX(f_below, f_above, uni->rate.sample, 257);
      for (i = 0; i < 257; i++)
	tmpcoef[i] = Cadd(tmpcoef[i], Cscl(tmpfilt->coef[i], g_here));
      delFIR_Bandpass_COMPLEX(tmpfilt);
    }

    for (i = 0; i < 257; i++)
      filtcoef[254 + i] = tmpcoef[i];

    ptmp = fftwf_plan_dft_1d(512,
			     (fftwf_complex *) filtcoef,
			     (fftwf_complex *) tx->grapheq.gen->p->zfvec,
			     FFTW_FORWARD,
			     uni->wisdom.bits);

    fftwf_execute(ptmp);
    fftwf_destroy_plan(ptmp);
    delvec_COMPLEX(filtcoef);
    delvec_COMPLEX(tmpcoef);
  }

  return 0;
}

PRIVATE int
getGrphTXEQ(int n, char **p) {
  if (tx->grapheq.parm.size == 3)
    sprintf(top->resp.buff,
	    "getGrphTXEQ %d %d %f %f %f %f\n",
	    tx->grapheq.flag,
	    tx->grapheq.parm.size,
	    tx->grapheq.parm.pre,
	    tx->grapheq.parm.gain[0],
	    tx->grapheq.parm.gain[1],
	    tx->grapheq.parm.gain[2]);
  else
    sprintf(top->resp.buff,
	    "getGrphTXEQ %d %d %f %f %f %f %f %f %f %f %f %f %f\n",
	    tx->grapheq.flag,
	    tx->grapheq.parm.size,
	    tx->grapheq.parm.pre,
	    tx->grapheq.parm.gain[0],
	    tx->grapheq.parm.gain[1],
	    tx->grapheq.parm.gain[2],
	    tx->grapheq.parm.gain[3],
	    tx->grapheq.parm.gain[4],
	    tx->grapheq.parm.gain[5],
	    tx->grapheq.parm.gain[6],
	    tx->grapheq.parm.gain[7],
	    tx->grapheq.parm.gain[8],
	    tx->grapheq.parm.gain[9]);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setNotch160 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setNotch160(int n, char **p) {
#if 0
  tx->grapheq.gen->notchflag = atoi(p[0]);
#endif
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setTXCarrierLevel 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setTXCarrierLevel(int n, char **p) {
  tx->am.carrier_level = atof(p[0]);
  return 0;
}

PRIVATE int
getTXCarrierLevel(int n, char **p) {
  sprintf(top->resp.buff, "getTXCarrierLevel %f\n", tx->am.carrier_level);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setANF 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setANF(int n, char **p) {
  rx[RL]->anf.flag = atoi(p[0]);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setANFvals 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setANFvals(int n, char **p) {
  int taps = atoi(p[0]), delay = atoi(p[1]);
  REAL gain = atof(p[2]), leak = atof(p[3]);
  rx[RL]->anf.gen->adaptive_filter_size = taps;
  rx[RL]->anf.gen->delay = delay;
  rx[RL]->anf.gen->adaptation_rate = gain;
  rx[RL]->anf.gen->leakage = leak;
  return 0;
}

PRIVATE int
getANF(int n, char **p) {
  sprintf(top->resp.buff, "getANF %d %d %d %f %f\n",
	  rx[RL]->anf.flag,
	  rx[RL]->anf.gen->adaptive_filter_size,
	  rx[RL]->anf.gen->delay,
	  rx[RL]->anf.gen->adaptation_rate,
	  rx[RL]->anf.gen->leakage);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setNR 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setNR(int n, char **p) {
  rx[RL]->anr.flag = atoi(p[0]);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setNRvals 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setNRvals(int n, char **p) {
  int taps = atoi(p[0]), delay = atoi(p[1]);
  REAL gain = atof(p[2]), leak = atof(p[3]);
  rx[RL]->anr.gen->adaptive_filter_size = taps;
  rx[RL]->anr.gen->delay = delay;
  rx[RL]->anr.gen->adaptation_rate = gain;
  rx[RL]->anr.gen->leakage = leak;
  return 0;
}

PRIVATE int
getANR(int n, char **p) {
  sprintf(top->resp.buff, "getANR %d %d %d %f %f\n",
	  rx[RL]->anr.flag,
	  rx[RL]->anr.gen->adaptive_filter_size,
	  rx[RL]->anr.gen->delay,
	  rx[RL]->anr.gen->adaptation_rate,
	  rx[RL]->anr.gen->leakage);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setcorrectIQ 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setcorrectIQ(int n, char **p) {
  REAL phase = atof(p[0]),
       gain = atof(p[1]);
  int i;																// SV1EIA AIR
	for (i = 0; i < DEFSPEC; i++)											// SV1EIA AIR
	{																	// SV1EIA AIR
		rx[RL]->iqfix->phase[i] = (REAL) (0.001 * phase);		// SV1EIA AIR
		rx[RL]->iqfix->gain[i] = (REAL) (1.0 + 0.001 * gain);	// SV1EIA AIR
	}																	// SV1EIA AIR
	rx[RL]->iqfix->spec = 0;									// SV1EIA AIR
  return 0;
}

PRIVATE int
getRXIQ(int n, char **p) {
  sprintf(top->resp.buff, "getRXIQ %f %f\n",
	  rx[RL]->iqfix->phase[0] * 1000,
	  (1 - rx[RL]->iqfix->gain[0]) * 1000);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

PRIVATE int
getTXIQ(int n, char **p) {
  sprintf(top->resp.buff, "getTXIQ %f %f\n",
	  tx->iqfix->phase[0] * 1000,
	  (1 - tx->iqfix->gain[0]) * 1000);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setcorrectIQphase 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setcorrectIQphase(int n, char **p) {
  REAL phase = atof(p[0]);
  int i;																// SV1EIA AIR
  printf("setcorrectIQphase %f\n", (REAL) (0.001 * phase));
	for (i = 0; i < DEFSPEC; i++)											// SV1EIA AIR
	{																	// SV1EIA AIR
		rx[RL]->iqfix->phase[i] = (REAL) (0.001 * phase);		// SV1EIA AIR
	}																	// SV1EIA AIR
	rx[RL]->iqfix->spec = 0;									// SV1EIA AIR
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setcorrectIQgain 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setcorrectIQgain(int n, char **p) {
  REAL gain = atof(p[0]);
  int i;																// SV1EIA AIR
	for (i = 0; i < DEFSPEC; i++)											// SV1EIA AIR
	{																	// SV1EIA AIR
		rx[RL]->iqfix->gain[i] = (REAL) (1.0 + 0.001 * gain);	// SV1EIA AIR
	}																	// SV1EIA AIR
	rx[RL]->iqfix->spec = 0;									// SV1EIA AIR
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setcorrectTXIQ 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setcorrectTXIQ(int n, char **p) {
  REAL phase = atof(p[0]),
       gain = atof(p[1]);
  int i;																// SV1EIA AIR
	for (i = 0; i < DEFSPEC; i++)											// SV1EIA AIR
	{																	// SV1EIA AIR
		tx->iqfix->phase[i] = (REAL) (0.001 * phase);		// SV1EIA AIR
		tx->iqfix->gain[i] = (REAL) (1.0 + 0.001 * gain);	// SV1EIA AIR
	}																	// SV1EIA AIR
	tx->iqfix->spec = 0;									// SV1EIA AIR
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setcorrectTXIQphase 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setcorrectTXIQphase(int n, char **p) {
  //REAL phaseadjustment = atof(p[0]);
  //tx->iqfix->phase = 0.001 * phaseadjustment;
  REAL phase = atof(p[0]);
  int i;																// SV1EIA AIR
  //printf("setcorrectTXIQphase %f\n", (REAL) (0.001 * phase));
	for (i = 0; i < DEFSPEC; i++)											// SV1EIA AIR
	{																	// SV1EIA AIR
		tx->iqfix->phase[i] = (REAL) (0.001 * phase);		// SV1EIA AIR
	}																	// SV1EIA AIR
	tx->iqfix->spec = 0;									// SV1EIA AIR
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setcorrectTXIQgain 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setcorrectTXIQgain(int n, char **p) {
  //REAL gainadjustment = atof(p[0]);
  //tx->iqfix->gain = 1.0 + 0.001 * gainadjustment;
  REAL gain = atof(p[0]);
  int i;																// SV1EIA AIR
  //printf("setcorrectTXIQgain %f\n", (REAL) (1.0 + 0.001 * gain));
	for (i = 0; i < DEFSPEC; i++)											// SV1EIA AIR
	{																	// SV1EIA AIR
		tx->iqfix->gain[i] = (REAL) (1.0 + 0.001 * gain);	// SV1EIA AIR
	}																	// SV1EIA AIR
	tx->iqfix->spec = 0;									// SV1EIA AIR
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setSquelch 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setSquelch(int n, char **p) {
  rx[RL]->squelch.thresh = atof(p[0]);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setSquelchSt 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setSquelchSt(int n, char **p) {
  rx[RL]->squelch.flag = atoi(p[0]);
  return 0;
}

PRIVATE int
getRXSquelch(int n, char **p) {
  sprintf(top->resp.buff, "getRXSquelch %d %f\n",
	  rx[RL]->squelch.flag,
	  rx[RL]->squelch.thresh);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setTXSquelch 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setTXSquelch(int n, char **p) {
  tx->squelch.thresh = atof(p[0]);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setTXSquelchSt 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setTXSquelchSt(int n, char **p) {
  tx->squelch.flag = atoi(p[0]);
  return 0;
}

PRIVATE int
getTXSquelch(int n, char **p) {
  sprintf(top->resp.buff, "getTXSquelch %d %f\n",
	  tx->squelch.flag,
	  tx->squelch.thresh);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setTXWaveShapeFunc 
* 
* experimental waveshaping/pre-distortion
*
* len f0 f1 ... f{len-1}
* if #args < 2, clear current, set to nil
* len should be odd in the usual case,
*  to avoid low-level DC artifacts
*
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
/***/

PRIVATE int
setTXWaveShapeFunc(int n, char **p) {
  if (n < 2) {
    if (tx->wvs.gen->tbl) {
      delvec_REAL(tx->wvs.gen->tbl);
      tx->wvs.gen->tbl = 0;
    }
    return 0;
  } else {
    int npts = atoi(p[0]);
    REAL *temp = newvec_REAL(npts, "TX WaveShape table update temp");
    {
      int i;
      for (i = 0; i < npts; i++)
	temp[i] = atof(p[i + 1]);
    }
    setWaveShaper(tx->wvs.gen, npts, temp);
    delvec_REAL(temp);
    return 0;
  }
}

/* -------------------------------------------------------------------------- */
/** @brief private setTXWaveShapeSt 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setTXWaveShapeSt(int n, char **p) {
  tx->wvs.flag = atoi(p[0]);
  return 0;
}

PRIVATE int
getTXWaveShape(int n, char **p) {
  sprintf(top->resp.buff, "getTXWaveShape %d\n", tx->wvs.flag);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

/***/

/* -------------------------------------------------------------------------- */
/** @brief private setTRX 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setTRX(int n, char **p) {
  uni->mode.trx = atoi(p[0]);
  return 0;
}

PRIVATE int
getTRX(int n, char **p) {
  sprintf(top->resp.buff, "getTRX %d\n", uni->mode.trx);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setNRunState 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setRunState(int n, char **p) {
  RUNMODE rs = (RUNMODE) atoi(p[0]);
  top->state = rs;
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setSpotToneVals 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setSpotToneVals(int n, char **p) {
  REAL gain = atof(p[0]),
       freq = atof(p[1]),
       rise = atof(p[2]),
       fall = atof(p[3]);
  setSpotToneGenVals(rx[RL]->spot.gen, gain, freq, rise, fall);
  return 0;
}

PRIVATE int
getSpotTone(int n, char **p) {
  sprintf(top->resp.buff, "getSpotTone %d %f %f %f %f\n",
	  rx[RL]->spot.flag,
	  rx[RL]->spot.gen->gain,
	  rx[RL]->spot.gen->osc.freq,
	  rx[RL]->spot.gen->rise.dur,
	  rx[RL]->spot.gen->fall.dur);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setSpotTone 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setSpotTone(int n, char **p) {
  if (atoi(p[0])) {
    SpotToneOn(rx[RL]->spot.gen);
    rx[RL]->spot.flag = TRUE;
  } else
    SpotToneOff(rx[RL]->spot.gen);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setFinished 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setFinished(int n, char **p) {
  top->running = FALSE;
  pthread_exit(0);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setSWCH 
* 
* next-trx-mode fall-msec stdy-msec rise-msec
*
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */

PRIVATE int
setSWCH(int n, char **p) {
  if (n < 4)
    return -1;
  else {
    int  nextrx = atoi(p[0]);
    REAL fallms = atof(p[1]),
         stdyms = atof(p[2]),
         risems = atof(p[3]);

    top->swch.trx.next = nextrx;

    top->swch.env.fall.size = uni->rate.sample * fallms / 1e3 + 0.5;
    if (top->swch.env.fall.size > 1)
      top->swch.env.fall.incr = -1.0 / (top->swch.env.fall.size - 1);
    else
      top->swch.env.fall.incr = -1.0;

    top->swch.env.stdy.size = uni->rate.sample * stdyms / 1e3 + 0.5;
    top->swch.env.stdy.incr = 0.0;

    top->swch.env.rise.size = uni->rate.sample * risems / 1e3 + 0.5;
    if (top->swch.env.rise.size > 1)
      top->swch.env.rise.incr = 1.0 / (top->swch.env.rise.size - 1);
    else
      top->swch.env.rise.incr = 1.0;

    top->swch.env.curr.type = SWCH_FALL;
    top->swch.env.curr.cnt = 0;
    top->swch.env.curr.val = 1.0;

    if (top->state != RUN_SWCH)
      top->swch.run.last = top->state;
    top->state = RUN_SWCH;
    return 0;
  }
}

/* -------------------------------------------------------------------------- */
/** @brief private setRingBufferOffset 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setRingBufferOffset(int n, char **p) {
  top->offs = atoi(p[0]);
  top->snds.doin = TRUE;
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setRingBufferReset 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setRingBufferReset(int n, char **p) {
  top->snds.doin = TRUE;
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setSNDResetSize 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setSNDSResetSize(int n, char **p) {
  top->snds.rsiz = atoi(p[0]);
  top->snds.doin = TRUE;
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setRXListen 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setRXListen(int n, char **p) {
  int lis = atoi(p[0]);
  if (lis < 0 || lis >= uni->multirx.nrx)
    return -1;
  else {
    uni->multirx.lis = lis;
    return 0;
  }
}

PRIVATE int
getRXListen(int n, char **p) {
  sprintf(top->resp.buff, "getRXListen %d\n", uni->multirx.lis);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setRXOn 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setRXOn(int n, char **p) {
  if (n < 1) {
    if (uni->multirx.act[RL])
      return -1;
    else {
      uni->multirx.act[RL] = TRUE;
      uni->multirx.nac++;
      rx[RL]->tick = 0;
      return 0;
    }
  } else {
    int k = atoi(p[0]);
    if (k < 0 || k >= uni->multirx.nrx)
      return -1;
    else {
      if (uni->multirx.act[k])
	return -1;
      else {
	uni->multirx.act[k] = TRUE;
	uni->multirx.nac++;
	rx[k]->tick = 0;
	return 0;
      }
    }
  }
}

/* -------------------------------------------------------------------------- */
/** @brief private setRXOff 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setRXOff(int n, char **p) {
  if (n < 1) {
    if (!uni->multirx.act[RL])
      return -1;
    else {
      uni->multirx.act[RL] = FALSE;
      --uni->multirx.nac;
      return 0;
    }
  } else {
    int k = atoi(p[0]);
    if (k < 0 || k >= uni->multirx.nrx)
      return -1;
    else {
      if (!uni->multirx.act[k])
	return -1;
      else {
	uni->multirx.act[k] = FALSE;
	--uni->multirx.nac;
	return 0;
      }
    }
  }
}

PRIVATE int
getRXCount(int n, char **p) {
  sprintf(top->resp.buff, "getRXCount %d\n", uni->multirx.nac);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setRXPan 
* 
* [pos]  0.0 <= pos <= 1.0
*
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */

PRIVATE int
setRXPan(int n, char **p) {
  REAL pos, theta;
  if (n < 1) {
    pos = 0.5;
    theta = (1.0 - pos) * M_PI / 2.0;
    rx[RL]->azim = Cmplx(cos(theta), sin(theta));
    return 0;
  } else {
    if ((pos = atof(p[0])) < 0.0 || pos > 1.0)
      return -1;
    theta = (1.0 - pos) * M_PI / 2.0;
    rx[RL]->azim = Cmplx(cos(theta), sin(theta));
    return 0;
  }
}

PRIVATE int
getRXPan(int n, char **p) {
  sprintf(top->resp.buff, "getRXPan %f %f\n",
	  rx[RL]->azim.re, rx[RL]->azim.im);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setGain 
* 
* TRX IO [gain] in->0, out->1, gain in dB, gain default->reset
*
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */

PRIVATE int
setGain(int n, char **p) {
  if (n < 2)
    return -1;
  
  else {
    int trx = atoi(p[0]),
        io = atoi(p[1]);
    REAL gain;
    
    if (n < 3)
      gain = 1.0;
    else
      gain = dB2lin(atof(p[2]));
    
    switch (trx) {
      
    case RX:
      switch (io) {
      case 0: rx[RL]->gain.i = gain; break;
      case 1: rx[RL]->gain.o = gain; break;
      default: return -1;
      }
      break;
      
    case TX:
      switch (io) {
      case 0: tx->gain.i = gain; break;
      case 1: tx->gain.o = gain; break;
      default: return -1;
      }
      break;

    default:
      return -1;
    }
  }
  
  return 0;
}

PRIVATE int
getRXGain(int n, char **p) {
  sprintf(top->resp.buff, "getRXGain %f %f\n",
	  rx[RL]->gain.i,
	  rx[RL]->gain.o);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

PRIVATE int
getTXGain(int n, char **p) {
  sprintf(top->resp.buff, "getTXGain %f %f\n",
	  tx->gain.i,
	  tx->gain.o);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setCompandSt 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setCompandSt(int n, char **p) {
  if (n < 1) {
    tx->cpd.flag = FALSE;
    return 0;
  } else {
    BOOLEAN flag = atoi(p[0]);
    if (n > 1) {
      switch (atoi(p[1])) {
      case RX:
	rx[RL]->cpd.flag = flag;
	break;
      case TX:
      default:
	tx->cpd.flag = flag;
	break;
      }
    } else
      tx->cpd.flag = flag;
    return 0;
  }
}

PRIVATE int
getRXCompand(int n, char **p) {
  sprintf(top->resp.buff, "getRXCompand %d %f\n",
	  rx[RL]->cpd.flag,
	  rx[RL]->cpd.gen->fac);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

PRIVATE int
getTXCompand(int n, char **p) {
  sprintf(top->resp.buff, "getTXCompand %d %f\n",
	  tx->cpd.flag,
	  tx->cpd.gen->fac);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setCompand 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setCompand(int n, char **p) {
  if (n < 1)
    return -1;
  else {
    REAL fac = atof(p[0]);
    if (n > 1) {
      switch (atoi(p[1])) {
      case RX:
	WSCReset(rx[RL]->cpd.gen, fac);
	break;
      case TX:
      default:
	WSCReset(tx->cpd.gen, fac);
	break;
      }
    } else
      WSCReset(tx->cpd.gen, fac);
    return 0;
  }
}

/* -------------------------------------------------------------------------- */
/** @brief private setGrphTXEQcmd 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setGrphTXEQcmd(int n, char **p) {
  if (n < 1) {
    tx->grapheq.flag = FALSE;
    return 0;
  } else {
    BOOLEAN flag = atoi(p[0]);
    tx->grapheq.flag = flag;
  }
  return 0;
}


/* -------------------------------------------------------------------------- */
/** @brief private setGrphRXEQcmd 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setGrphRXEQcmd(int n, char **p) {
  if (n < 1) {
    rx[RL]->grapheq.flag = FALSE;
    return 0;
  } else {
    BOOLEAN flag = atoi(p[0]);
    rx[RL]->grapheq.flag = flag;
  }
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setTXCompandSt 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setTXCompandSt(int n, char **p) {
  if (n < 1) {
    tx->cpd.flag = FALSE;
    return 0;
  } else {
    BOOLEAN flag = atoi(p[0]);
    tx->cpd.flag = flag;
  }
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setTXCompand 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setTXCompand(int n, char **p) {
  if (n < 1)
    return -1;
  else {
    REAL fac = atof(p[0]);
    WSCReset(tx->cpd.gen, fac);
  }
  return 0;
}

//------------------------------------------------------------------------

/* -------------------------------------------------------------------------- */
/** @brief private setSpectrumPolyphase 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setSpectrumPolyphase(int n, char **p) {
  BOOLEAN setit = atoi(p[0]);
  if (uni->spec.polyphase != setit) {
    if (setit) {
      uni->spec.polyphase = TRUE;
      uni->spec.mask = (8 * uni->spec.size) - 1;
      {
	RealFIR WOLAfir;
	REAL MaxTap = 0;
	int i;
	WOLAfir = newFIR_Lowpass_REAL(1.0,
				      (REAL) uni->spec.size,
				      8 * uni->spec.size - 1);
	memset(uni->spec.window, 0, 8 * sizeof(REAL) * uni->spec.size);
	memcpy(uni->spec.window,
	       FIRcoef(WOLAfir),
	       sizeof(REAL) * (8 * uni->spec.size - 1));
	for (i = 0; i < 8 * uni->spec.size; i++)
	  MaxTap = max(MaxTap, fabs(uni->spec.window[i]));
	MaxTap = 1.0f / MaxTap;
	for (i = 0; i < 8 * uni->spec.size; i++)
	  uni->spec.window[i] *= MaxTap;
	delFIR_REAL(WOLAfir);
      }
    } else {
      uni->spec.polyphase = FALSE;
      uni->spec.mask = uni->spec.size - 1;
      memset(uni->spec.window, 0, sizeof(REAL) * uni->spec.size);
      makewindow(uni->spec.wintype, uni->spec.size - 1, uni->spec.window);
    }
    reinit_spectrum(&uni->spec);
  }
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private seSpectrumWindow 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setSpectrumWindow(int n, char **p) {
  Windowtype window = atoi(p[0]);
  if (!uni->spec.polyphase)
    makewindow(window, uni->spec.size, uni->spec.window);
  uni->spec.wintype = window;
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setSpectrumType 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setSpectrumType(int n, char **p) {
  uni->spec.type = SPEC_POST_FILT;
  uni->spec.scale = SPEC_PWR;
  uni->spec.rxk = RL;
  switch (n) {
  case 3:
    uni->spec.rxk = atoi(p[2]);
  case 2:
    uni->spec.scale = atoi(p[1]);
  case 1:
    uni->spec.type = atoi(p[0]);
    break;
  case 0:
    break;
  default:
    return -1;
  }
  return 0;
}

PRIVATE int
getSpectrumInfo(int n, char **p) {
  sprintf(top->resp.buff, "getSpectrumInfo %d %d %d %d %d\n",
	  uni->spec.polyphase,
	  uni->spec.wintype,
	  uni->spec.type,
	  uni->spec.scale,
	  uni->spec.rxk);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setDCBlockSt 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setDCBlockSt(int n, char **p) {
  if (n < 1) {
    tx->dcb.flag = FALSE;
    return 0;
  } else {
    tx->dcb.flag = atoi(p[0]);
    return 0;
  }
}

PRIVATE int
getDCBlock(int n, char **p) {
  sprintf(top->resp.buff, "getDCBlock %d\n", tx->dcb.flag);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setDCBlock 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setDCBlock(int n, char **p) {
  resetDCBlocker(tx->dcb.gen);
  return 0;
}

//========================================================================

/* -------------------------------------------------------------------------- */
/** @brief private setNewBuflen 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setNewBuflen(int n, char **p) {
  extern int reset_for_buflen(int);
  int rtn = -1;
  if (n == 1) {
    top->susp = TRUE;
    if (reset_for_buflen(atoi(p[0])) != -1) {
      if (uni->update.flag)
	replay_updates();
      rtn = 0;
    }
    top->susp = FALSE;
  }
  return rtn;
}

PRIVATE int
getBuflen(int n, char **p) {
  sprintf(top->resp.buff, "getBuflen %d\n", uni->buflen);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private setTEST 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setTEST(int n, char **p) {
  if (n == 1) {
    int mode = atoi(p[0]);
    if ((mode >= TEST_TONE) && (mode <= TEST_NOISE)) {
      top->test.mode = mode;
      return 0;
    }
  }
  return -1;
}

/* -------------------------------------------------------------------------- */
/** @brief private setTestTone 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
// setTestTone freq-in-hz amp-in-dB

PRIVATE int
setTestTone(int n, char **p) {
  if (n == 2) {
    REAL freq = atof(p[0]),
         amp  = dB2lin(atof(p[1])),
         phs  = OSCphase(top->test.tone.gen);
    delOSC(top->test.tone.gen);
    top->test.tone.gen = newOSC(top->hold.size.frames,
				ComplexTone,
				freq,
				phs,
				uni->rate.sample,
				"test mode single tone");
    top->test.tone.amp = amp;
    return 0;
  }
  return -1;
}

/* -------------------------------------------------------------------------- */
/** @brief private setTestTwoTone 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
// setTestTwoTone freq-in-hz-A amp-in-dB-A freq-in-hz-B amp-in-dB-B

PRIVATE int
setTestTwoTone(int n, char **p) {
  if (n == 4) {
    REAL freq = atof(p[0]),
         amp  = dB2lin(atof(p[1])),
         phs  = OSCphase(top->test.twotone.a.gen);
    delOSC(top->test.twotone.a.gen);
    top->test.twotone.a.gen = newOSC(top->hold.size.frames,
				     ComplexTone,
				     freq,
				     phs,
				     uni->rate.sample,
				     "test mode two tone A");
    top->test.twotone.a.amp = amp;

    freq = atof(p[2]),
    amp  = dB2lin(atof(p[3])),
    phs  = OSCphase(top->test.twotone.b.gen);
    delOSC(top->test.twotone.b.gen);
    top->test.twotone.b.gen = newOSC(top->hold.size.frames,
				     ComplexTone,
				     freq,
				     phs,
				     uni->rate.sample,
				     "test mode two tone B");
    top->test.twotone.b.amp = amp;

    return 0;
  }

  return -1;
}

/* -------------------------------------------------------------------------- */
/** @brief private setTestNoise 
* 
* setTestNoise amp-in-dB
*
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */

PRIVATE int
setTestNoise(int n, char **p) {
  if (n == 1) {
    top->test.noise.amp = dB2lin(atof(p[0]));
    return 0;
  }
  return -1;
}

/* -------------------------------------------------------------------------- */
/** @brief private setTestThru 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
PRIVATE int
setTestThru(int n, char **p) {
  if (n == 1) {
    top->test.thru = atoi(p[0]);
    return 0;
  }
  return -1;
}

PRIVATE int
getTEST(int n, char **p) {
  sprintf(top->resp.buff, "getTEST %f %f %f %f %f %f %f %d %d\n",
	  top->test.tone.amp, top->test.tone.freq,
	  top->test.twotone.a.amp, top->test.twotone.a.freq,
	  top->test.twotone.b.amp, top->test.twotone.b.freq,
	  top->test.noise.amp,
	  top->test.mode,
	  top->test.thru);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

//========================================================================

PRIVATE int
setTXMeterMode(int n, char **p) {
  uni->meter.tx.mode = atoi(p[0]);
  return 0;
}

PRIVATE int
getTXMeterMode(int n, char **p) {
  sprintf(top->resp.buff, "getTXMeterMode %d\n", uni->meter.tx.mode);
  top->resp.size = strlen(top->resp.buff);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private reqMeter 
* 
* save current state while guarded by upd sem
*
* reqMeter [label [TRX]]
*
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */

PRIVATE int
reqMeter(int n, char **p) {
  if (n > 1) {
    int trx = atoi(p[1]);
    switch (trx) {
    case TX:
      snap_meter_tx(&uni->meter, atoi(p[0]));
      break;
    case RX:
    default:
      snap_meter_rx(&uni->meter, atoi(p[0]));
    }
  } else
    snap_meter_rx(&uni->meter, n > 0 ? atoi(p[0]) : 0);
    
  sem_post(top->sync.mtr.sem);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private reqRXMeter 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
// reqRXMeter [label]

PRIVATE int
reqRXMeter(int n, char **p) {
  snap_meter_rx(&uni->meter, n > 0 ? atoi(p[0]) : 0);
  sem_post(top->sync.mtr.sem);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private reqTXMeter 
* 
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
// reqTXMeter [label]

PRIVATE int
reqTXMeter(int n, char **p) {
  snap_meter_tx(&uni->meter, n > 0 ? atoi(p[0]) : 0);
  sem_post(top->sync.mtr.sem);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private ReqSpectrum 
* 
* simile modo
*
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */

PRIVATE int
reqSpectrum(int n, char **p) {
  snap_spectrum(&uni->spec, n > 0 ? atoi(p[0]) : 0, uni->tick);
  sem_post(top->sync.pws.sem);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private reqScope 
* 
* quasi modo
*
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */

PRIVATE int
reqScope(int n, char **p) {
  snap_scope(&uni->spec, n > 0 ? atoi(p[0]) : 0, uni->tick);
  sem_post(top->sync.pws.sem);
  return 0;
}

/* -------------------------------------------------------------------------- */
/** @brief private reqDump
* 
* quasi modo
*
* @param n 
* @param *p 
* @return int 
*/
/* ---------------------------------------------------------------------------- */

PRIVATE int
reqDump(int n, char **p) {
#if 0
  snap_dump();
  sem_post(top->sync.dmp.sem);
#endif
  return 0;
}

//========================================================================

CTE update_cmds[] = {
  {"reqDump", reqDump},
  {"reqMeter", reqMeter},
  {"reqRXMeter", reqRXMeter},
  {"reqScope", reqScope},
  {"reqSpectrum", reqSpectrum},
  {"reqTXMeter", reqTXMeter},

  {"setANF", setANF},
  {"setANFvals", setANFvals},
  {"setBIN", setBIN},
  {"setBlkANF", setBlkANF},
  {"setBlkANFval", setBlkANFval},
  {"setBlkNR", setBlkNR},
  {"setBlkNRval", setBlkNRval},
  {"setCompand", setCompand},
  {"setCompandSt", setCompandSt},
  {"setDCBlock", setDCBlock},
  {"setDCBlockSt", setDCBlockSt},
  {"setFilter", setFilter},
  {"setFinished", setFinished},
  {"setGain", setGain},
  {"setGrphRXEQ10", setGrphRXEQ10},
  {"setGrphRXEQ3", setGrphRXEQ3},
  {"setGrphRXEQcmd", setGrphRXEQcmd},
  {"setGrphTXEQ10", setGrphTXEQ10},
  {"setGrphTXEQ3", setGrphTXEQ3},
  {"setGrphTXEQcmd", setGrphTXEQcmd},
  {"setMode", setMode},
  {"setNB", setNB},
  {"setNBvals", setNBvals},
  {"setNR", setNR},
  {"setNRvals", setNRvals},
  {"setNewBuflen", setNewBuflen},
  {"setNotch160", setNotch160},
  {"setOsc", setOsc},
  {"setRXAGC", setRXAGC},
  {"setRXAGCAttack", setRXAGCAttack},
  {"setRXAGCCompression", setRXAGCCompression},
  {"setRXAGCDecay", setRXAGCDecay},
  {"setRXAGCFix", setRXAGCFix},
  {"setRXAGCHang", setRXAGCHang},
  {"setRXAGCHangThreshold", setRXAGCHangThreshold},
  {"setRXAGCLimit", setRXAGCLimit},
  {"setRXAGCSlope", setRXAGCSlope},
  {"setRXAGCTop", setRXAGCTop},
  {"setRXFiltCoefs", setRXFiltCoefs},
  {"setRXListen", setRXListen},
  {"setRXOff", setRXOff},
  {"setRXOn", setRXOn},
  {"setRXPan", setRXPan},
  {"setRingBufferOffset", setRingBufferOffset},
  {"setRingBufferReset", setRingBufferReset},
  {"setRunState", setRunState},
  {"setSDROM", setSDROM},
  {"setSDROMvals", setSDROMvals},
  {"setSNDSResetSize", setSNDSResetSize},
  {"setSWCH", setSWCH},
  {"setSpectrumPolyphase", setSpectrumPolyphase},
  {"setSpectrumType", setSpectrumType},
  {"setSpectrumWindow", setSpectrumWindow},
  {"setSpotTone", setSpotTone},
  {"setSpotToneVals", setSpotToneVals},
  {"setSquelch", setSquelch},
  {"setSquelchSt", setSquelchSt},
  {"setTEST", setTEST},
  {"setTRX", setTRX},
  {"setTXAGCFF", setTXAGCFF},
  {"setTXAGCFFCompression", setTXAGCFFCompression},
  {"setTXCarrierLevel", setTXCarrierLevel},
  {"setTXCompand", setTXCompand},
  {"setTXCompandSt", setTXCompandSt},
  {"setTXFiltCoefs", setTXFiltCoefs},
  {"setTXLevelerAttack", setTXLevelerAttack},
  {"setTXLevelerDecay", setTXLevelerDecay},
  {"setTXLevelerHang", setTXLevelerHang},
  {"setTXLevelerSt", setTXLevelerSt},
  {"setTXLevelerTop", setTXLevelerTop},
  {"setTXMeterMode", setTXMeterMode},
  {"setTXSpeechCompression", setTXSpeechCompression},
  {"setTXSpeechCompressionGain", setTXSpeechCompressionGain},
  {"setTXSquelch", setTXSquelch},
  {"setTXSquelchSt", setTXSquelchSt},
  {"setTXWaveShapeFunc", setTXWaveShapeFunc},
  {"setTXWaveShapeSt", setTXWaveShapeSt},
  {"setTestNoise", setTestNoise},
  {"setTestThru", setTestThru},
  {"setTestTone", setTestTone},
  {"setTestTwoTone", setTestTwoTone},
  {"setcorrectIQ", setcorrectIQ},
  {"setcorrectIQgain", setcorrectIQgain},
  {"setcorrectIQphase", setcorrectIQphase},
  {"setcorrectTXIQ", setcorrectTXIQ},
  {"setcorrectTXIQgain", setcorrectTXIQgain},
  {"setcorrectTXIQphase", setcorrectTXIQphase},
  {"setfixedAGC", setfixedAGC},

  {"getANF", getANF},
  {"getANR", getANR},
  {"getBIN", getBIN},
  {"getBlkANF", getBlkNR},
  {"getBlkNR", getBlkNR},
  {"getBuflen", getBuflen},
  {"getDCBLock", getDCBlock},
  {"getGrphRXEQ", getGrphRXEQ},
  {"getGrphTXEQ", getGrphTXEQ},
  {"getNB", getNB},
  {"getRXAGC", getRXAGC},
  {"getRXCompand", getRXCompand},
  {"getRXCount", getRXCount},
  {"getRXFilter", getRXFilter},
  {"getRXGain", getRXGain},
  {"getRXIQ", getRXIQ},
  {"getRXListen", getRXListen},
  {"getRXMode", getRXMode},
  {"getRXOsc", getRXOsc},
  {"getRXPan", getRXPan},
  {"getRXSquelch", getRXSquelch},
  {"getSDROM", getSDROM},
  {"getSpectrumInfo", getSpectrumInfo},
  {"getSpotTone", getSpotTone},
  {"getTEST", getTEST},
  {"getTRX", getTRX},
  {"getTXCarrierLevel", getTXCarrierLevel},
  {"getTXCompand", getTXCompand},
  {"getTXFilter", getTXFilter},
  {"getTXGain", getTXGain},
  {"getTXIQ", getTXIQ},
  {"getTXLeveler", getTXLeveler},
  {"getTXMeterMode", getTXMeterMode},
  {"getTXMode", getTXMode},
  {"getTXOsc", getTXOsc},
  {"getTXSpeechCompression", getTXSpeechCompression},
  {"getTXSquelch", getTXSquelch},
  {"getTXWaveShape", getTXWaveShape},

  {0, 0}
};

//........................................................................

/* -------------------------------------------------------------------------- */
/** @brief do_update 
* 
* @param str 
* @param log 
* @return int 
*/
/* ---------------------------------------------------------------------------- */
int
do_update(char *str, FILE *log) {
  BOOLEAN quiet = FALSE,
          switcheroo = FALSE;
  int oldRL, tmpRL;
  SPLIT splt = &uni->update.splt;

  // append to replay file?
  if (*str == '!') {
    str++;			// strip !
    if (uni->update.flag)
      fputs(str, uni->update.fp);
  }
  // echo to logging output?
  if (*str == '-') {
    quiet = TRUE;
    str++;
  }

  // do the old switcheroo?
  // ie, temporary redirect of a command to another RX
  if (*str == '@') {
    char *endp;
    tmpRL = strtol(++str, &endp, 10);
    if (tmpRL < 0 || tmpRL >= uni->multirx.nrx)
      return -1;
    while (*endp && isspace(*endp))
      endp++;
    str = endp;
    switcheroo = TRUE;
  }

  split(splt, str);

  if (NF(splt) < 1)
    return -1;

  else {
    Thunk thk = Thunk_lookup(update_cmds, F(splt, 0));
    if (!thk)
      return -1;
    else {
      int val;

      sem_wait(top->sync.upd.sem);

      if (switcheroo)
	oldRL = RL, RL = tmpRL;

      val = (*thk)(NF(splt) - 1, Fptr(splt, 1));

      if (switcheroo)
	RL = oldRL;

      sem_post(top->sync.upd.sem);

      if (log && !quiet) {
	int i;
	char *s = since(&top->start_tv);
	fprintf(log, "%s update[%s]: returned %d from", top->snds.name, s, val);
	for (i = 0; i < NF(splt); i++)
	  fprintf(log, " %s", F(splt, i));
	putc('\n', log);
	fflush(log);
      }

      return val;
    }
  }
}


//------------------------------------------------------------------------
