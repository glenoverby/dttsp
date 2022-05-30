/** 
* @file sdr-main.c
* @brief Software Defined Radio Main Module 
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

///////////////////////////////////////////////////////////////////////

#include <common.h>

/// elementary defaults

struct _loc loc;

/////////////////////////////////////////////////////////////////////////
/// most of what little we know here about the inner loop,
/// functionally speaking

extern void reset_meters(void);
extern void reset_spectrum(void);
extern void reset_counters(void);
extern void process_samples(float *, float *, int);
extern void setup_workspace(REAL samplerate,
			    int buflen,
			    SDRMODE mode,
			    char *wisdom,
			    int specsize,
			    int numrecv,
			    int cpdsize);
extern void destroy_workspace(void);
extern int reset_for_buflen(int new_buflen);

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/* @brief private spectrum_thread 
 * @return void
 */

PRIVATE void
spectrum_thread(void) {
  unsigned short port = top->meas.spec.port;
  struct sockaddr_in clnt;
  int sock, clnt_len;

  if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    perror("Failed to create UDP socket for spectrum");
    exit(1);
  }

  clnt_len = sizeof(clnt);
  memset((char *) &clnt, 0, clnt_len);
  clnt.sin_family = AF_INET;
  clnt.sin_addr.s_addr = htonl(INADDR_ANY);
  clnt.sin_port = htons(port);

  if (top->verbose)
    fprintf(stderr, "%s: Ready to return spectrum on port %d\n", top->snds.name, port);

  while (top->running) {
    int sp_blen;
    char sp_buff[32768];

    sem_wait(top->sync.pws.sem);
    sem_wait(top->sync.upd.sem);

    // generate & pack up data to be sent

    {
      char *ptr = sp_buff;
      memcpy(ptr, (char *) &uni->spec.label, sizeof(int));
      ptr += sizeof(int);
      memcpy(ptr, (char *) &uni->spec.stamp, sizeof(int));
      ptr += sizeof(int);

      // spectrum or scope?

      if (uni->spec.last == SPEC_LAST_FREQ) {
	int cnt = sizeof(float) * uni->spec.size;
	compute_spectrum(&uni->spec);
	memcpy(ptr, (char *) uni->spec.output, cnt);
	ptr += cnt;
      } else {
	int cnt = sizeof(float) * uni->spec.size;
	memcpy(ptr, (char *) uni->spec.oscope, cnt);
	ptr += cnt;
      }
      sp_blen = ptr - sp_buff;
    }

    if (sendto(sock,
	       sp_buff,
	       sp_blen,
	       0,
	       (struct sockaddr *) &clnt,
	       clnt_len)
	!= sp_blen) {
      perror("Failed to send spectrum");
      exit(1);
    }

    sem_post(top->sync.upd.sem);
  }

  close(sock);
  pthread_exit(0);
}

//////////////////////////////////////////////////////////////////////////

/* @brief private meter_thread 
 * @return void
 */

PRIVATE void
meter_thread(void) {
  unsigned short port = top->meas.mtr.port;
  struct sockaddr_in clnt;
  int sock, clnt_len;

  if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    perror("Failed to create UDP socket for meter");
    exit(1);
  }

  clnt_len = sizeof(clnt);
  memset((char *) &clnt, 0, clnt_len);
  clnt.sin_family = AF_INET;
  clnt.sin_addr.s_addr = htonl(INADDR_ANY);
  clnt.sin_port = htons(port);

  if (top->verbose)
    fprintf(stderr, "%s: Ready to return meter on port %d\n", top->snds.name, port);

  while (top->running){ 
    int mtr_blen;
    char mtr_buff[1024];

    sem_wait(top->sync.mtr.sem);
    sem_wait(top->sync.upd.sem);

    // pack up data to be sent

    {
      char *ptr = mtr_buff;
      memcpy(ptr, &uni->meter.label, sizeof(int));
      ptr += sizeof(int);
      
      // rx or tx?
      
      if (uni->meter.last == METER_LAST_RX) {
	int cnt = sizeof(float) * MAXRX * RXMETERPTS;
	memcpy(ptr, (char *) uni->meter.snap.rx, cnt);
	ptr += cnt;
      } else {
	int cnt = sizeof(float) * TXMETERPTS;
	memcpy(ptr, (char *) uni->meter.snap.tx, cnt);
	ptr += cnt;
      }
      mtr_blen = ptr - mtr_buff;
    }
      
    if (sendto(sock,
	       mtr_buff,
	       mtr_blen,
	       0,
	       (struct sockaddr *) &clnt,
	       clnt_len)
	!= mtr_blen) {
      perror("Failed to send meter");
      exit(1);
    }

    sem_post(top->sync.upd.sem);
  }

  close(sock);
  pthread_exit(0);
}

//////////////////////////////////////////////////////////////////////////

/* @brief private process_updates_thread 
 * @return void
 */

PRIVATE void
process_updates_thread(void) {
  int sock;

  // set up listening port

  {
    unsigned short port = top->parm.port;
    struct sockaddr_in serv;
    int serv_len;
    
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
      perror("Failed to create UDP socket for update");
      exit(1);
    }
    
    serv_len = sizeof(serv);
    memset((char *) &serv, 0, serv_len);
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = htonl(INADDR_ANY);
    serv.sin_port = htons(port);
    
    if (bind(sock, (struct sockaddr *) &serv, serv_len) < 0) {
      perror("Failed to bind update UDP socket");
      exit(1);
    }
    
    if (top->verbose)
      fprintf(stderr, "%s: Ready to receive commands on port %d\n", top->snds.name, port);
  }

  while (top->running) {
    struct sockaddr_in clnt;
    socklen_t clnt_len;
    int rcvd, rtn;

    pthread_testcancel();
    clnt_len = sizeof(clnt);

    // block until a command shows up

    if ((rcvd = recvfrom(sock,
			 top->parm.buff,
			 sizeof(top->parm.buff) - 1,
			 0,
			 (struct sockaddr *) &clnt,
			 &clnt_len))
	< 0) {
      perror("Failed to receive UDP update command");
      exit(1);
    }

    top->parm.buff[rcvd] = 0; // properly terminated string?

    if (top->verbose)
      fprintf(stderr,
	      "%s: update client connected: %s\n",
	      top->snds.name,
	      inet_ntoa(clnt.sin_addr));

    top->resp.size = 0; // assume no returned data

    // do_update may block internally!

    rtn = do_update(top->parm.buff, top->verbose ? top->echo.fp : 0);

    // ack (?)

    {
      static char
	*ack_msg = "ok",
	*err_msg = "error";
      static int
	ack_len = 2,
	err_len = 5;
      char *msg;
      int msg_len;

      if (rtn != 0)
	msg = err_msg, msg_len = err_len;
      else {
	if (top->resp.size == 0)
	  msg = ack_msg, msg_len = ack_len;
	else {
	  static char rbuf[8192];
	  sprintf(rbuf, "%s %s", ack_msg, top->resp.buff);
	  msg = rbuf;
	  msg_len = ack_len + 1 + top->resp.size;
	}
      }
      
      if (sendto(sock,
		 msg,
		 msg_len,
		 0,
		 (struct sockaddr *) &clnt,
		 sizeof(clnt))
	  != msg_len) {
	perror("Failed to send update command response");
	exit(1);
      }
    }
  }
    
  close(sock);
  pthread_exit(0);
}

//////////////////////////////////////////////////////////////////////////

PRIVATE void
run_mute(void) {
  memset((char *) top->hold.buf.l, 0, top->hold.size.bytes);
  memset((char *) top->hold.buf.r, 0, top->hold.size.bytes);
  uni->tick++;
}

PRIVATE void
run_pass(void) { uni->tick++; }

PRIVATE void
run_play(void) {
#if 0
  {
    int i;
    float sum = 0.0;
    for (i = 0; i < top->hold.size.frames; i++)
      sum += sqr(top->hold.buf.l[i]) + sqr(top->hold.buf.r[i]);
    fprintf(stdout, "%9d %9.6f",
	    uni->tick, sqrt(sum / top->hold.size.frames));
  }
#endif

  process_samples(top->hold.buf.l, top->hold.buf.r, top->hold.size.frames);

#if 0
  {
    int i;
    float sum = 0.0;
    for (i = 0; i < top->hold.size.frames; i++)
      sum += sqr(top->hold.buf.l[i]) + sqr(top->hold.buf.r[i]);
    fprintf(stdout, " %9.6f\n", sqrt(sum / top->hold.size.frames));
  }
  fflush(stdout);
#endif
}


// NB do not set RUN_SWCH directly via setRunState;
// use setSWCH instead

PRIVATE void
run_swch(void) {
  int i, n = top->hold.size.frames;
  REAL w;

  process_samples(top->hold.buf.l, top->hold.buf.r, top->hold.size.frames);

  for (i = 0; i < n; i++) {

    if (top->swch.env.curr.type == SWCH_FALL) {
      top->swch.env.curr.val += top->swch.env.fall.incr;
      w = sin(top->swch.env.curr.val * M_PI /  2.0);
      top->hold.buf.l[i] *= w, top->hold.buf.r[i] *= w;

      if (++top->swch.env.curr.cnt >= top->swch.env.fall.size) {
	top->swch.env.curr.type = SWCH_STDY;
	top->swch.env.curr.cnt = 0;
	top->swch.env.curr.val = 0.0;
      }

    } else if (top->swch.env.curr.type == SWCH_STDY) {
      top->hold.buf.l[i]= top->hold.buf.r[i] = 0.0;

      if (++top->swch.env.curr.cnt >= top->swch.env.stdy.size) {
	top->swch.env.curr.type = SWCH_RISE;
	top->swch.env.curr.cnt = 0;
	top->swch.env.curr.val = 0.0;
      }

    } else if (top->swch.env.curr.type == SWCH_RISE) {
      top->swch.env.curr.val += top->swch.env.rise.incr;
      w = sin(top->swch.env.curr.val * M_PI /  2.0);
      top->hold.buf.l[i] *= w, top->hold.buf.r[i] *= w;

      if (++top->swch.env.curr.cnt >= top->swch.env.rise.size) {
	uni->mode.trx = top->swch.trx.next;
	top->state = top->swch.run.last;
	break;
      }
    }
  }
}

PRIVATE void
run_test(void) {
  int i;

  switch (top->test.mode) {

  case TEST_TONE:
    ComplexOSC(top->test.tone.gen); // 1000
    for (i = 0; i < top->hold.size.frames; i++)
      top->hold.buf.l[i] = OSCreal(top->test.tone.gen, i) * top->test.tone.amp,
      top->hold.buf.r[i] = OSCimag(top->test.tone.gen, i) * top->test.tone.amp;
    break;

  case TEST_2TONE:
    ComplexOSC(top->test.twotone.a.gen); // 700
    ComplexOSC(top->test.twotone.b.gen); // 1900
    for (i = 0; i < top->hold.size.frames; i++)
      top->hold.buf.l[i] =
	OSCreal(top->test.twotone.a.gen, i) * top->test.twotone.a.amp +
	OSCreal(top->test.twotone.b.gen, i) * top->test.twotone.b.amp,
      top->hold.buf.r[i] =
	OSCimag(top->test.twotone.a.gen, i) * top->test.twotone.a.amp +
	OSCimag(top->test.twotone.b.gen, i) * top->test.twotone.b.amp;
    break;

#define ransig(x) ((drand48() * 0.5 - 1.0) * (x))
  case TEST_NOISE:
    for (i = 0; i < top->hold.size.frames; i++)
      top->hold.buf.l[i] = ransig(top->test.noise.amp),
      top->hold.buf.r[i] = ransig(top->test.noise.amp);
    break;
#undef ransig

  default:
    memset((char *) top->hold.buf.l, 0, top->hold.size.bytes);
    memset((char *) top->hold.buf.r, 0, top->hold.size.bytes);
    break;
  }

#if 0
  {
    int i;
    float sum = 0.0;
    for (i = 0; i < top->hold.size.frames; i++)
      sum += sqr(top->hold.buf.l[i]) + sqr(top->hold.buf.r[i]);
    fprintf(stdout, "%9d %9.6f",
	    uni->tick, sqrt(sum / top->hold.size.frames));
  }
#endif

  if (!top->test.thru)
    process_samples(top->hold.buf.l, top->hold.buf.r, top->hold.size.frames);

#if 0
  {
    int i;
    float sum = 0.0;
    for (i = 0; i < top->hold.size.frames; i++)
      sum += sqr(top->hold.buf.l[i]) + sqr(top->hold.buf.r[i]);
    fprintf(stdout, " %9.6f\n", sqrt(sum / top->hold.size.frames));
  }
  fflush(stdout);
#endif
}

//========================================================================

PRIVATE int
audio_callback(jack_nframes_t nframes, void *arg) {
  float *lp, *rp;
  int nbytes = nframes * sizeof(float);

  if (top->snds.doin) {
    int i;
    const float zero = 0.0;
    
    ringb_float_reset(top->snds.ring.i.l);
    ringb_float_reset(top->snds.ring.i.r);
    
    for (i = top->offs; i < 0; i++)
      ringb_float_write(top->snds.ring.i.l, &zero, 1);

    for (i = 0; i < top->offs; i++)
      ringb_float_write(top->snds.ring.i.r, &zero, 1);
    
    ringb_float_restart(top->snds.ring.o.r, nframes);
    ringb_float_restart(top->snds.ring.o.l, nframes);
    
    top->snds.doin = FALSE;
  }

  if (top->susp) {
    lp = (float *) jack_port_get_buffer(top->snds.port.o.l, nframes);
    rp = (float *) jack_port_get_buffer(top->snds.port.o.r, nframes);
    memset((char *) lp, 0, nbytes);
    memset((char *) rp, 0, nbytes);
    return 0;
  }
  
  // output: copy from ring to port
  
  lp = (float *) jack_port_get_buffer(top->snds.port.o.l, nframes);
  rp = (float *) jack_port_get_buffer(top->snds.port.o.r, nframes);
  
  if ((ringb_float_read_space(top->snds.ring.o.l) >= nframes) &&
      (ringb_float_read_space(top->snds.ring.o.r) >= nframes)) {
    ringb_float_read(top->snds.ring.o.l, lp, nframes);
    ringb_float_read(top->snds.ring.o.r, rp, nframes);
  } else {
    memset((char *) lp, 0, nbytes);
    memset((char *) rp, 0, nbytes);
  }
  
  // input: copy from port to ring
  if ((ringb_float_write_space(top->snds.ring.i.l) >= nframes) &&
      (ringb_float_write_space(top->snds.ring.i.l) >= nframes)) {
    lp = (float *) jack_port_get_buffer(top->snds.port.i.l, nframes);
    rp = (float *) jack_port_get_buffer(top->snds.port.i.r, nframes);
    ringb_float_write(top->snds.ring.i.l, lp, nframes);
    ringb_float_write(top->snds.ring.i.r, rp, nframes);
  } else
    top->snds.doin = TRUE;
  
  // fire dsp
  sem_post(top->sync.buf.sem);
  return 0;
}

/* @brief private gethold
 * @return void
 */

PRIVATE BOOLEAN
gethold(void) {
  if ((ringb_float_read_space(top->snds.ring.i.l) >= top->hold.size.frames) &&
      (ringb_float_read_space(top->snds.ring.i.r) >= top->hold.size.frames)) {
    ringb_float_read(top->snds.ring.i.l, top->hold.buf.l, top->hold.size.frames);
    ringb_float_read(top->snds.ring.i.r, top->hold.buf.r, top->hold.size.frames);
    return TRUE;
  } else
    return FALSE;
}

/* @brief private puthold 
 * @return void
 */

PRIVATE void
puthold(void) {
  if ((ringb_float_write_space(top->snds.ring.o.l) >= top->hold.size.frames) &&
      (ringb_float_write_space(top->snds.ring.o.r) >= top->hold.size.frames)) {
    ringb_float_write(top->snds.ring.o.l,
		      top->hold.buf.l,
		      top->hold.size.frames);
    ringb_float_write(top->snds.ring.o.r,
		      top->hold.buf.r,
		      top->hold.size.frames);
  }
}

/* @brief private process_samples_thread
 * @return void
 */

PRIVATE void
process_samples_thread(void) {
  while (top->running) {
    sem_wait(top->sync.buf.sem);
    while (gethold()) {
      sem_wait(top->sync.upd.sem);
      // run synchronous updates here
      switch (top->state) {
      case RUN_MUTE: run_mute(); break;
      case RUN_PASS: run_pass(); break;
      case RUN_PLAY: run_play(); break;
      case RUN_SWCH: run_swch(); break;
      case RUN_TEST: run_test(); break;
      }
      sem_post(top->sync.upd.sem);
      puthold();
    }
  }
}

//========================================================================

PRIVATE void
execute(void) {
  // let updates run
  sem_post(top->sync.upd.sem);
  
  // rcfile
  {
    FILE *frc = find_rcfile(loc.path.rcfile);
    if (frc) {
      while (fgets(top->parm.buff, sizeof(top->parm.buff), frc))
	do_update(top->parm.buff, top->echo.fp ? stderr : 0);
      fclose(frc);
    }
  }

  // start audio processing
  if (jack_activate(top->snds.client))
    fprintf(stderr, "%s: cannot activate jack client", top->snds.name), exit(1);

  // final shutdown always starts in update thread,
  // so join it first
  pthread_join(top->thrd.upd.id, 0);

  // issue cancellation notices
  pthread_cancel(top->thrd.trx.id);
  if (uni->meter.flag)
    pthread_cancel(top->thrd.mtr.id);
  if (uni->spec.flag)
    pthread_cancel(top->thrd.pws.id);

  // wait for remaining threads to finish
  pthread_join(top->thrd.trx.id, 0);
  if (uni->meter.flag)
    pthread_join(top->thrd.mtr.id, 0);
  if (uni->spec.flag)
    pthread_join(top->thrd.pws.id, 0);
  
  // stop audio processing
  jack_client_close(top->snds.client);
}

/* @brief private run test 
*/

PRIVATE void
setup_local_audio(void) {
  top->offs = loc.skew.offs;
  top->hold.size.frames = uni->buflen;
  top->hold.size.bytes = top->hold.size.frames * sizeof(float);
  top->hold.buf.l = (float *) safealloc(top->hold.size.frames,
					sizeof(float),
					"main hold buffer left");
  top->hold.buf.r = (float *) safealloc(top->hold.size.frames,
					sizeof(float),
					"main hold buffer right");

  // test generators

  top->test.mode = TEST_TONE;
  top->test.thru = FALSE;

  // single tone
  top->test.tone.gen = newOSC(top->hold.size.frames,
			      ComplexTone,
			      1000.0,
			      0.0,
			      uni->rate.sample,
			      "test mode single tone");
  // expressed in dB in update
  top->test.tone.amp = 0.5;

  // two tone
  // top->test.mode = TEST_2TONE;
  top->test.twotone.a.gen = newOSC(top->hold.size.frames,
				   ComplexTone,
				   700.0,
				   0.0,
				   uni->rate.sample,
				   "test mode two tone A");
  top->test.twotone.b.gen = newOSC(top->hold.size.frames,
				   ComplexTone,
				   1900.0,
				   0.0,
				   uni->rate.sample,
				   "test mode two tone B");
  // expressed in dB in update
  top->test.twotone.a.amp = top->test.twotone.b.amp = 0.5;

  // noise
  // top->test.mode = TEST_NOISE;
  srand48(time(0));
  top->test.noise.amp = 0.5;
}

PRIVATE void 
setup_updates(void) {
  top->parm.port = loc.port.parm;

  // do this here 'cuz the update thread is controlling the action
  if (uni->meter.flag)
    top->meas.mtr.port = loc.port.meter;
  if (uni->spec.flag)
    top->meas.spec.port = loc.port.spec;

  if ((uni->update.path = loc.path.replay)) {
    uni->update.flag = TRUE;
    uni->update.fp = efopen(uni->update.path, "w+");
  }

  if (top->verbose) {
    if (loc.path.echo[0]) {
      top->echo.path = loc.path.echo;
      if ((top->echo.fd = open(top->echo.path, O_RDWR)) == -1)
	perror(top->echo.path), exit(1);
      if (!(top->echo.fp = fdopen(top->echo.fd, "r+"))) {
	fprintf(stderr, "can't fdopen echo file %s\n", loc.path.echo);
	exit(1);
      }
    } else {
      top->echo.path = 0;
      top->echo.fp = stderr;
      top->echo.fd = fileno(top->echo.fp);
    }
  }
}

/* @brief private audio callback 
 * @param nframes 
 * @param arg 
 * @return void
 */

PRIVATE void
jack_xrun(void *arg) {}

PRIVATE void
jack_shutdown(void *arg) {}

PRIVATE void
setup_system_audio(void) {
  if (loc.name[0])
    strcpy(top->snds.name, loc.name);
  else
    sprintf(top->snds.name, "sdr-%d", top->pid);
  
  if (!(top->snds.client = jack_client_open(top->snds.name, JackUseExactName, NULL))) {	// JackNullOption
    fprintf(stderr, "%s: ", top->snds.name);
    perror("can't make client -- jack not running?");
    exit(1);
  }

  if ((jack_nframes_t) loc.def.rate != jack_get_sample_rate(top->snds.client)) {
    fprintf(stderr, "In %s:\n", top->snds.name);
    fprintf(stderr, "There's a sample rate mismatch.\n");
    fprintf(stderr, "dttsp rate is %d.\n", (jack_nframes_t) loc.def.rate);
    fprintf(stderr, "jackd rate is %d.\n", jack_get_sample_rate(top->snds.client));
    fprintf(stderr, "Time to exit so you can straighten this out.\n");
    exit(1);
  }

  jack_set_process_callback(top->snds.client, (void *) audio_callback, 0);
  jack_on_shutdown(top->snds.client, (void *) jack_shutdown, 0);
  jack_set_xrun_callback(top->snds.client, (void *) jack_xrun, 0);
  top->snds.size = jack_get_buffer_size(top->snds.client);

  top->snds.port.i.l = jack_port_register(top->snds.client,
                                          "il",
					  JACK_DEFAULT_AUDIO_TYPE,
					  JackPortIsInput,
					  0);
  top->snds.port.i.r = jack_port_register(top->snds.client,
					  "ir",
					  JACK_DEFAULT_AUDIO_TYPE,
					  JackPortIsInput,
					  0);
  top->snds.port.o.l = jack_port_register(top->snds.client,
					  "ol",
					  JACK_DEFAULT_AUDIO_TYPE,
					  JackPortIsOutput,
					  0);
  top->snds.port.o.r = jack_port_register(top->snds.client,
					  "or",
					  JACK_DEFAULT_AUDIO_TYPE,
					  JackPortIsOutput,
					  0);

  {
    int nr = nblock2(max(top->hold.size.frames, top->snds.size) * loc.mult.ring);
    top->snds.ring.i.l = ringb_float_create(nr);
    top->snds.ring.i.r = ringb_float_create(nr);
    top->snds.ring.o.l = ringb_float_create(nr);
    top->snds.ring.o.r = ringb_float_create(nr);
  }

  top->snds.doin = TRUE;
}

PRIVATE sem_t *
make_sem(char *id, char *path) {
  sem_t *s;

  sprintf(path, "%s.%s", top->snds.name, id);
  sem_unlink(path);
  if ((s = sem_open(path, O_CREAT | O_EXCL, 0600, 0))
      == (sem_t *) -1) {
    fprintf(stderr, "Failed to open semaphore %s\n", path);
    exit(1);
  }
  //sem_unlink(path);
  return s;
}

/* @brief private process samples 
 * @return void
 */

PRIVATE void
setup_threading(void) {
  top->sync.upd.sem = make_sem("update", top->sync.upd.name);
  pthread_create(&top->thrd.upd.id, 0, (void *) process_updates_thread, 0);

  top->sync.buf.sem = make_sem("buffer", top->sync.buf.name);
  pthread_create(&top->thrd.trx.id, 0, (void *) process_samples_thread, 0);

  top->susp = FALSE;

  if (uni->meter.flag) {
    top->sync.mtr.sem = make_sem("meter", top->sync.mtr.name);
    pthread_create(&top->thrd.mtr.id, 0, (void *) meter_thread, 0);
  }

  if (uni->spec.flag) {
    top->sync.pws.sem = make_sem("spectrum", top->sync.pws.name);
    pthread_create(&top->thrd.pws.id, 0, (void *) spectrum_thread, 0);
  }
}


/////////////////////////////////////
// hard defaults, then environment
/////////////////////////////////////

/* @brief private execute 
 * @return void
 */

PRIVATE void
setup_defaults(void) {
  // no env vars for these
  loc.name[0] = 0; // no default for client name, period
  loc.path.echo[0] = 0;  // file defaults to stderr

  strcpy(loc.path.rcfile, RCBASE);
  strcpy(loc.path.replay, REPLAYPATH);
  strcpy(loc.path.wisdom, WISDOMPATH);

  loc.def.comp   = DEFCOMP;
  loc.def.mode   = DEFMODE;
  loc.def.nrx    = MAXRX;
  loc.def.rate   = DEFRATE;
  loc.def.size   = DEFSIZE;
  loc.def.spec   = DEFSPEC;
  loc.mult.ring  = RINGMULT;
  loc.skew.offs  = DEFOFFS;
  loc.port.spec  = SPECPORT;
  loc.port.meter = METERPORT;
  loc.port.parm  = PARMPORT;

  {
    char *ep;
    if ((ep = getenv("SDR_DEFMODE")))    loc.def.mode = atoi(ep);
    if ((ep = getenv("SDR_DEFRATE")))    loc.def.rate = atof(ep);
    if ((ep = getenv("SDR_DEFSIZE")))    loc.def.size = atoi(ep);
    if ((ep = getenv("SDR_RINGMULT")))   loc.mult.ring = atoi(ep);
    if ((ep = getenv("SDR_SKEWOFFS")))   loc.skew.offs = atoi(ep);
    if ((ep = getenv("SDR_METERPORT")))  loc.port.meter = atoi(ep);
    if ((ep = getenv("SDR_NAME")))       strcpy(loc.name, ep);
    if ((ep = getenv("SDR_PARMPORT")))   loc.port.parm = atoi(ep);
    if ((ep = getenv("SDR_RCBASE")))     strcpy(loc.path.rcfile, ep);
    if ((ep = getenv("SDR_REPLAYPATH"))) strcpy(loc.path.replay, ep);
    if ((ep = getenv("SDR_SPECPORT")))   loc.port.spec = atoi(ep);
    if ((ep = getenv("SDR_WISDOMPATH"))) strcpy(loc.path.wisdom, ep);
  }
}

//========================================================================

int
reset_for_buflen(int new_buflen) {

  // make sure new size is power of 2
  if (popcnt(new_buflen) != 1)
    return -1;

  safefree((char *) top->hold.buf.r);
  safefree((char *) top->hold.buf.l);

  destroy_workspace();

  //  fprintf(stderr, "safemem %d\n", safememcurrcount());
  safememreset();

  setup_workspace(loc.def.rate,
		  loc.def.size = new_buflen,
		  loc.def.mode,
		  loc.path.wisdom, 
		  loc.def.spec, 
		  loc.def.nrx, 
		  loc.def.size);

  setup_local_audio();

  reset_meters();
  reset_spectrum();
  reset_counters();

  return 0;
}


/* @brief private destroy_globals
 * @return void
 */

PRIVATE void
destroy_globals(void) {
  int i;
  safefree((char *) tx);
  for (i = 0; i < MAXRX; i++)
    safefree((char *) rx[i]);
  safefree((char *) uni);
  safefree((char *) top);
}

/* @brief private closeup
 * @return void
 */

PRIVATE void 
closeup(void) {
  ringb_float_free(top->snds.ring.o.r);
  ringb_float_free(top->snds.ring.o.l);
  ringb_float_free(top->snds.ring.i.r);
  ringb_float_free(top->snds.ring.i.l);

  safefree((char *) top->hold.buf.r);
  safefree((char *) top->hold.buf.l);

  delOSC(top->test.tone.gen);
  delOSC(top->test.twotone.a.gen);
  delOSC(top->test.twotone.b.gen);

  if (top->verbose && top->echo.fp != stderr)
    fclose(top->echo.fp);

  sem_close(top->sync.buf.sem);
  sem_unlink(top->sync.buf.name);
  sem_close(top->sync.upd.sem);
  sem_unlink(top->sync.upd.name);

  if (uni->meter.flag) {
    sem_close(top->sync.mtr.sem);
    sem_unlink(top->sync.mtr.name);
  }

  if (uni->spec.flag) {
    sem_close(top->sync.pws.sem);
    sem_unlink(top->sync.pws.name);
  }

  if (uni->update.flag)
    fclose(uni->update.fp);

  destroy_workspace();

  destroy_globals();

  //  fprintf(stderr, "safemem %d\n", safememcurrcount());

  exit(0);
}

//========================================================================
// commandline processing

PRIVATE struct option long_options[] = {
  {"verbose",       no_argument,       0,  0},
  {"spectrum",      no_argument,       0,  1},
  {"metering",      no_argument,       0,  2},
  {"load",          required_argument, 0,  3},
  {"mode",          required_argument, 0,  4},
  {"buffsize",      required_argument, 0,  5},
  {"ringmult",      required_argument, 0,  6},
  {"meter-port",    required_argument, 0,  8},
  {"client-name",   required_argument, 0,  9},
  {"command-port",  required_argument, 0, 10},
  {"init-path",     required_argument, 0, 11},
  {"replay-path",   required_argument, 0, 12},
  {"spectrum-port", required_argument, 0, 13},
  {"wisdom-path",   required_argument, 0, 14},
  {"echo-path",     required_argument, 0, 15},
  {"skewoffs",      required_argument, 0, 16},
  {"help",          no_argument,       0, 99},
  {0,               0,                 0,  0}
};
PRIVATE char *short_options = "vmsl:h?";
PRIVATE int option_index;

PRIVATE void usage(void);


/* @brief private setup threading 
 * @return void
 */

PRIVATE void
setup_from_commandline(int argc, char **argv) {
  int c;
  while ((c = getopt_long(argc, argv,
			  short_options,
			  long_options,
			  &option_index))
	 != EOF) {
    switch (c) {
    case 0:
    case 'v':
      top->verbose = TRUE;
      break;

    case 1:
    case 's':
      uni->spec.flag = TRUE;
      break;

    case 2:
    case 'm':
      uni->meter.flag = TRUE;
      break;

    case 3:
    case 'l':
      strcpy(loc.path.rcfile, optarg);
      break;

    case 4:
      loc.def.mode = atoi(optarg);
      break;

    case 5:
      loc.def.size = atoi(optarg);
      break;

    case 6:
      loc.mult.ring = atoi(optarg);
      break;

    case 8:
      loc.port.meter = atoi(optarg);
      break;

    case 9:
      strcpy(loc.name, optarg);
      break;

    case 10:
      loc.port.parm = atoi(optarg);
      break;

    case 11:
      strcpy(loc.path.rcfile, optarg);
      break;

    case 12:
      strcpy(loc.path.replay, optarg);
      break;

    case 13:
      loc.port.spec = atoi(optarg);
      break;

    case 14:
      strcpy(loc.path.wisdom, optarg);
      break;

    case 15:
      strcpy(loc.path.echo, optarg);
      break;

    case 16:
      loc.skew.offs = atoi(optarg);
      break;

    case 99:
    case 'h':
    default:
      usage();
    }
  }
}  

//========================================================================

PRIVATE void
create_globals(void) {
  int i;
  top = (struct _top *) safealloc(1, sizeof(struct _top), "top global");
  uni = (struct _uni *) safealloc(1, sizeof(struct _uni), "uni global");
  for (i = 0; i < MAXRX; i++)
    rx[i] = (struct _rx *) safealloc(1, sizeof(struct _rx), "rx global");
  tx = (struct _tx *) safealloc(1, sizeof(struct _tx), "tx global");
}

/* @brief private destroy globals 
 * @return void
 */

PRIVATE void
setup(int argc, char **argv) {
  create_globals();

  top->pid = getpid();
  top->uid = getuid();
  top->start_tv = now_tv();
  top->running = TRUE;
  top->verbose = FALSE;
  top->state = RUN_PLAY;
  top->offs = DEFOFFS;
  top->snds.doin = FALSE;
  top->snds.rsiz = DEFSIZE;

  // temporarily call us by our pid, until client name established
  sprintf(top->snds.name, "pid %d", top->pid);

  //  fprintf(stderr, "safemem %d\n", safememcurrcount());
  safememreset();

  setup_defaults();

  setup_from_commandline(argc, argv);

  setup_workspace(loc.def.rate,
		  loc.def.size,
		  loc.def.mode,
		  loc.path.wisdom,
		  loc.def.spec,
		  loc.def.nrx,
		  loc.def.comp);

  setup_updates();

  setup_local_audio();
  setup_system_audio();

  setup_threading();

  reset_meters();
  reset_spectrum();
  reset_counters();
}

//========================================================================

int 
main(int argc, char **argv) {
  setup(argc, argv), execute(), closeup();
  return 0;
} 

/* @brief private create globals 
 * @return void
 */

PRIVATE void
usage(void) {
  fprintf(stderr, "--verbose\n");
  fprintf(stderr, " -v\n");
  fprintf(stderr, "	Turn on verbose mode (echo commands, etc.)\n");
  fprintf(stderr, "--spectrum\n");
  fprintf(stderr, " -s\n");
  fprintf(stderr, "	Turn on spectrum computation\n");
  fprintf(stderr, "--metering\n");
  fprintf(stderr, " -m\n");
  fprintf(stderr, "	Turn on meter computation\n");
  fprintf(stderr, "--load=<load-file>\n");
  fprintf(stderr, " -l <load-file>\n");
  fprintf(stderr, "	Read update commands from <load-file> at startup\n");
  fprintf(stderr, "--mode=<mode>\n");
  fprintf(stderr, "	Start radio in mode <mode>\n");
  fprintf(stderr, "--buffsize=<power-of-2>\n");
  fprintf(stderr, "	Use <power-of-2> as DSP buffersize\n");
  fprintf(stderr, "--ringmult=<num>\n");
  fprintf(stderr, "	Use <num> * <buffsize> for ring buffer length\n");
  fprintf(stderr, "--spectrum-pport=<portnum>\n");
  fprintf(stderr, "	Use port <portnum> as conduit for spectrum data\n");
  fprintf(stderr, "	Default is %d\n", SPECPORT);
  fprintf(stderr, "--meter-port=<portnum>\n");
  fprintf(stderr, "	Use port <portnum> as conduit for meter data\n");
  fprintf(stderr, "	Default is %d\n", METERPORT);
  fprintf(stderr, "--command-port=<portnum>\n");
  fprintf(stderr, "	Use port <portnum> as a conduit for update commands\n");
  fprintf(stderr, "	Default is %d\n", PARMPORT);
  fprintf(stderr, "--init-path=<init-file>\n");
  fprintf(stderr, "	Read update commands from <init-file> at startup. Like -l.\n");
  fprintf(stderr, "--replay-path=<path>\n");
  fprintf(stderr, "	Write/reread saved update commands to/from <path>\n");
  fprintf(stderr, "--wisdom-path=<path>\n");
  fprintf(stderr, "	fftw3 wisdom is in <path>\n");
  fprintf(stderr, "--echo-path=<path>\n");
  fprintf(stderr, "	Write update command processor output to <path>\n");
  fprintf(stderr, "--skewoffs=<+/-num>\n");
  fprintf(stderr, "	Correct for audio channel skew by <num>\n");
  fprintf(stderr, "--help\n");
  fprintf(stderr, " -h\n");
  fprintf(stderr, "	Write this message and exit.\n");
  fprintf(stderr, "Environment variables:\n");
  fprintf(stderr, "\tSDR_DEFMODE\n");
  fprintf(stderr, "\tSDR_DEFRATE\n");
  fprintf(stderr, "\tSDR_DEFSIZE\n");
  fprintf(stderr, "\tSDR_RINGMULT\n");
  fprintf(stderr, "\tSDR_SKEWOFFS\n");
  fprintf(stderr, "\tSDR_METERPORT\n");
  fprintf(stderr, "\tSDR_NAME\n");
  fprintf(stderr, "\tSDR_PARMPORT\n");
  fprintf(stderr, "\tSDR_RCBASE\n");
  fprintf(stderr, "\tSDR_REPLAYPATH\n");
  fprintf(stderr, "\tSDR_SPECPPORT\n");
  fprintf(stderr, "\tSDR_WISDOMPATH\n");

  exit(1);
}  
