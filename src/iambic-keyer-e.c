/* iambic-keyer.c */
/*
This file is part of a program that implements a Software-Defined Radio.

The code in this file is derived from routines originally written by
Pierre-Philippe Coupard for his CWirc X-chat program. That program
is issued under the GPL and is
Copyright (C) Pierre-Philippe Coupard - 18/06/2003

This derived version is
Copyright (C) 2004-2008 by Frank Brickle, AB2KT and Bob McGwier, N4HY

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

#include <linux/rtc.h>
#include <fromsys.h>
#include <banal.h>
#include <splitfields.h>
#include <datatypes.h>
#include <bufvec.h>
#include <cxops.h>
#include <oscillator.h>
#include <cwtones.h>

//========================================================================

#define L_KEY_DOWN	  (01 << 00)
#define R_KEY_DOWN	  (01 << 01)

#define NO_TIME_LEFTS_SCHED	(-2)
#define NO_ELEMENT		(-1)
#define DIT			 (0)
#define DAH			 (1)
#define MODE_A			 (0)
#define MODE_B			 (1)
#define NO_PADDLE_SQUEEZE	 (0)
#define PADDLES_SQUEEZED	 (1)
#define PADDLES_RELEASED	 (2)
#define NO_DELAY		 (0)
#define CHAR_SPACING_DELAY	 (1)
#define WORD_SPACING_DELAY	 (2)
#define DEBOUNCE_BUF_MAX_SIZE	(30)

//========================================================================

typedef
struct _keyer_state {
  struct {
    BOOLEAN iambic,	// iambic or straight
      	    mdlmdB,	// set true if mode B
            revpdl;	// paddles reversed
    struct {
      BOOLEAN dit, dah;
    } memory;		// set both true for mode B
    struct {
      BOOLEAN khar, word;
    } autospace;
  } flag;
  int debounce,	// # seconds to read paddles
      mode,	// 0 = mode A, 1 = mode B
      weight;	// 15 -> 85%
  double wpm;	// for iambic keyer
} KeyerStateInfo, *KeyerState;

extern KeyerState newKeyerState(void);
extern void delKeyerState(KeyerState ks);

//------------------------------------------------------------------------

typedef
struct _keyer_logic {
  struct {
    BOOLEAN init;
    struct {
      BOOLEAN dit, dah;
    } prev;
  } flag;
  struct {
    BOOLEAN altrn, // insert alternate element
            psqam; // paddles squeezed after mid-element
    int curr, // -1 = nothing, 0 = dit, 1 = dah
        iamb, //  0 = none, 1 = squeezed, 2 = released
        last; // -1 = nothing, 0 = dit, 1 = dah
  } element;
  struct {
    double beep, dlay, elem, midl;
  } time_left;
  int dlay_type; // 0 = none, 1 = interchar, 2 = interword
} KeyerLogicInfo, *KeyerLogic;

extern KeyerLogic newKeyerLogic(void);
extern void delKeyerLogic(KeyerLogic kl);

extern BOOLEAN klogic(KeyerLogic kl,
		      BOOLEAN dit,
		      BOOLEAN dah,
		      double wpm,
		      int iambicmode,
		      BOOLEAN need_midelemodeB,
		      BOOLEAN want_dit_mem,
		      BOOLEAN want_dah_mem,
		      BOOLEAN autocharspacing,
		      BOOLEAN autowordspacing,
		      int weight,
		      double ticklen);

//========================================================================
//========================================================================
//========================================================================

#define SAMP_RATE (48000)

// # times key is sampled per sec
//#define RTC_RATE (64)
#define RTC_RATE (1024)

// # samples generated during 1 clock tick at RTC_RATE
#define TONE_SIZE (SAMP_RATE / RTC_RATE)

// ring buffer size
#define RING_SIZE (01 << 022)

KeyerState ks;
KeyerLogic kl;

pthread_t poller, play, key, update, outside;
sem_t poll_action, clock_fired, keyer_started, update_ok, state_change;
int poll_status;

int fdser, fdrtc;

jack_client_t *client;
jack_port_t *lport, *rport;
jack_ringbuffer_t *lring, *rring;
jack_nframes_t size;

CWToneGen gen;
BOOLEAN playing = FALSE, iambic = FALSE;
double wpm = 18.0, freq = 700.0, ramp = 5.0, gain = -3.0, hang = 200.0;

//========================================================================
//========================================================================
//========================================================================

/* Get most recent input port status,
   do debouncing,
   then return the key state */

BOOLEAN
read_straight_key(KeyerState ks) {
  int i, j, status;
  static BOOLEAN keystate = 0;
  static int debounce_buf_i = 0,
             debounce_buf[DEBOUNCE_BUF_MAX_SIZE];

  /* Get the key state */
  sem_wait(&poll_action);
  status = poll_status;
  poll_status = 0;

  debounce_buf[debounce_buf_i] = status & (L_KEY_DOWN | R_KEY_DOWN);
  debounce_buf_i++;

  /* If the debounce buffer is full,
     determine the state of the key */
  if (debounce_buf_i >= ks->debounce) {
    debounce_buf_i = 0;

    j = 0;
    for (i = 0; i < ks->debounce; i++)
      if (debounce_buf[i])
	j++;
    keystate = (j > ks->debounce / 2) ? 1 : 0;
  }

  return keystate;
}

//------------------------------------------------------------------------

/* Get most recent input port status,
   do debouncing,
   emulate a straight key,
   then return the emulated key state */

BOOLEAN
read_iambic_key(KeyerState ks, KeyerLogic kl, double ticklen) {
  int i, j, status;
  static BOOLEAN dah_debounce_buf[DEBOUNCE_BUF_MAX_SIZE],
                 dit_debounce_buf[DEBOUNCE_BUF_MAX_SIZE];
  static int dah = 0, debounce_buf_i = 0, dit = 0;

  /* Get the key states */
  sem_wait(&poll_action);
  status = poll_status;
  poll_status = 0;

  if (ks->flag.revpdl) {
    dah_debounce_buf[debounce_buf_i] = status & L_KEY_DOWN;
    dit_debounce_buf[debounce_buf_i] = status & R_KEY_DOWN;
  } else {
    dit_debounce_buf[debounce_buf_i] = status & R_KEY_DOWN;
    dah_debounce_buf[debounce_buf_i] = status & L_KEY_DOWN;
  }
  debounce_buf_i++;

  /* If the debounce buffer is full, determine the state of the keys */
  if (debounce_buf_i >= ks->debounce) {

    j = 0;
    for (i = 0; i < ks->debounce; i++)
      if (dah_debounce_buf[i]) j++;
    dah = (j > ks->debounce / 2) ? 1 : 0;

    j = 0;
    for (i = 0; i < ks->debounce; i++)
      if (dit_debounce_buf[i]) j++;
    dit = (j > ks->debounce / 2) ? 1 : 0;

    debounce_buf_i = 0;
  }

  return klogic(kl,
		dit,
		dah,
		ks->wpm,
		ks->mode,
		ks->flag.mdlmdB,
		ks->flag.memory.dit,
		ks->flag.memory.dah,
		ks->flag.autospace.khar,
		ks->flag.autospace.word,
		ks->weight,
		ticklen);
}

//========================================================================

BOOLEAN
klogic(KeyerLogic kl,
       BOOLEAN dit,
       BOOLEAN dah,
       double wpm,
       int iambicmode,
       BOOLEAN need_midelemodeB,
       BOOLEAN want_dit_mem,
       BOOLEAN want_dah_mem,
       BOOLEAN autocharspacing,
       BOOLEAN autowordspacing,
       int weight,
       double ticklen) {

  double ditlen = 1200 / wpm;
  int set_which_ele_time_left = NO_TIME_LEFTS_SCHED;

  /* Do we need to initialize the keyer? */
  if (!kl->flag.init) {
    kl->flag.prev.dit = dit;
    kl->flag.prev.dah = dah;
    kl->element.last = kl->element.curr = NO_ELEMENT;
    kl->element.iamb = NO_PADDLE_SQUEEZE;
    kl->element.psqam = 0;
    kl->element.altrn = 0;
    kl->time_left.midl = kl->time_left.beep = kl->time_left.elem = 0;
    kl->time_left.dlay = 0;
    kl->dlay_type = NO_DELAY;
    kl->flag.init = 1;
  }

  /* Decrement the time_lefts */
  kl->time_left.dlay -= kl->time_left.dlay > 0 ? ticklen : 0;
  if (kl->time_left.dlay <= 0) {
    /* If nothing is scheduled to play,
       and we just did a character space delay,
       and we're doing auto word spacing,
       then pause for a word space,
       otherwise resume the normal element time_left countdowns */
    if (kl->time_left.elem <= 0 &&
	kl->dlay_type == CHAR_SPACING_DELAY &&
	autowordspacing) {
      kl->time_left.dlay = ditlen * 4;
      kl->dlay_type = WORD_SPACING_DELAY;
    } else {
      kl->dlay_type = NO_DELAY;
      kl->time_left.midl -= kl->time_left.midl > 0 ? ticklen : 0;
      kl->time_left.beep -= kl->time_left.beep > 0 ? ticklen : 0;
      kl->time_left.elem -= kl->time_left.elem > 0 ? ticklen : 0;
    }
  }

  /* Are both paddles squeezed? */
  if (dit && dah) {
    kl->element.iamb = PADDLES_SQUEEZED;
    /* Are the paddles squeezed past the middle of the element? */
    if (kl->time_left.midl <= 0)
      kl->element.psqam = 1;
  } else if (!dit && !dah && kl->element.iamb == PADDLES_SQUEEZED)
    /* Are both paddles released and we had gotten a squeeze in this element? */
    kl->element.iamb = PADDLES_RELEASED;

  /* Is the current element finished? */
  if (kl->time_left.elem <= 0 && kl->element.curr != NO_ELEMENT) {
    kl->element.last = kl->element.curr;

    /* Should we insert an alternate element? */
    if (((dit && dah) ||
	 (kl->element.altrn &&
	  kl->element.iamb != PADDLES_RELEASED) ||
	 (kl->element.iamb == PADDLES_RELEASED &&
	  iambicmode == MODE_B &&
	  (!need_midelemodeB || kl->element.psqam)))) {
      if (kl->element.last == DAH)
	set_which_ele_time_left = kl->element.curr = DIT;
      else
	set_which_ele_time_left = kl->element.curr = DAH;

    } else {
      /* No more element */
      kl->element.curr = NO_ELEMENT;
      /* Do we do automatic character spacing? */
      if (autocharspacing && !dit && !dah) {
	kl->time_left.dlay = ditlen * 2;
	kl->dlay_type = CHAR_SPACING_DELAY;
      }
    }

    kl->element.altrn = 0;
    kl->element.iamb = NO_PADDLE_SQUEEZE;
    kl->element.psqam = 0;
  }

  /* Is an element not currently being played? */
  if (kl->element.curr == NO_ELEMENT) {
    if (dah)		/* Dah paddle down? */
      set_which_ele_time_left = kl->element.curr = DAH;
    else if (dit)	/* Dit paddle down? */
      set_which_ele_time_left = kl->element.curr = DIT;
  }

  /* Take the dah memory request into account */
  if (kl->element.curr == DIT &&
      !kl->flag.prev.dah &&
      dah &&
      want_dah_mem)
    kl->element.altrn = 1;

  /* Take the dit memory request into account */
  if (kl->element.curr == DAH &&
      !kl->flag.prev.dit &&
      dit &&
      want_dit_mem)
    kl->element.altrn = 1;

  /* If we had a dit or dah scheduled for after a delay,
     and both paddles are up before the end of the delay,
     and we have not requested dit or dah memory,
     forget it
     NB can't happen in full mode B */

  if (kl->time_left.dlay > 0 && !dit && !dah &&
      ((kl->element.curr == DIT && !want_dit_mem) ||
       (kl->element.curr == DAH && !want_dah_mem)))
    set_which_ele_time_left = kl->element.curr = NO_ELEMENT;

  /* Set element time_lefts, if needed */
  switch (set_which_ele_time_left) {
  case NO_ELEMENT:		/* Cancel any element */
    kl->time_left.beep = 0;
    kl->time_left.midl = 0;
    kl->time_left.elem = 0;
    break;

  case DIT:			/* Schedule a dit */
    kl->time_left.beep = (ditlen * (double) weight) / 50;
    kl->time_left.midl = kl->time_left.beep / 2;
    kl->time_left.elem = ditlen * 2;
    break;

  case DAH:			/* Schedule a dah */
    kl->time_left.beep = (ditlen * (double) weight) / 50 + ditlen * 2;
    kl->time_left.midl = kl->time_left.beep / 2;
    kl->time_left.elem = ditlen * 4;
    break;
  }

  kl->flag.prev.dit = dit;
  kl->flag.prev.dah = dah;

  return kl->time_left.beep > 0 && kl->time_left.dlay <= 0;
}

KeyerState
newKeyerState(void) {
  return (KeyerState) safealloc(1, sizeof(KeyerStateInfo), "newKeyerState");
}

void
delKeyerState(KeyerState ks) {
  safefree((char *) ks);
}

KeyerLogic
newKeyerLogic(void) {
  return (KeyerLogic) safealloc(1, sizeof(KeyerLogicInfo), "newKeyerLogic");
}

void
delKeyerLogic(KeyerLogic kl) {
  safefree((char *) kl);
}

//========================================================================

void
jack_ringbuffer_clear(jack_ringbuffer_t *ring, int nbytes) {
  int i;
  char zero = 0;
  for (i = 0; i < nbytes; i++)
    jack_ringbuffer_write(ring, &zero, 1);
}

void
jack_ringbuffer_restart(jack_ringbuffer_t *ring, int nbytes) {
  jack_ringbuffer_reset(ring);
  jack_ringbuffer_clear(ring, nbytes);
}

//------------------------------------------------------------

// generated tone -> output ringbuffer
void
send_tone(void) {
  if (jack_ringbuffer_write_space(lring) < TONE_SIZE * sizeof(float)) {
    write(2, "overrun tone\n", 13);
    jack_ringbuffer_restart(lring, TONE_SIZE * sizeof(float));
    jack_ringbuffer_restart(rring, TONE_SIZE * sizeof(float));
  } else {
    int i;
    for (i = 0; i < gen->size; i++) {
      float l = CXBreal(gen->buf, i),
	    	r = CXBimag(gen->buf, i);
      jack_ringbuffer_write(lring, (char *) &l, sizeof(float));
      jack_ringbuffer_write(rring, (char *) &r, sizeof(float));
    }
  }
}

// silence -> output ringbuffer
void
send_silence(void) {
  if (jack_ringbuffer_write_space(lring) < TONE_SIZE * sizeof(float)) {
    write(2, "overrun zero\n", 13);
    jack_ringbuffer_restart(lring, TONE_SIZE * sizeof(float));
    jack_ringbuffer_restart(rring, TONE_SIZE * sizeof(float));
  } else {
    int i;
    for (i = 0; i < gen->size; i++) {
      float zero = 0.0;
      jack_ringbuffer_write(lring, (char *) &zero, sizeof(float));
      jack_ringbuffer_write(rring, (char *) &zero, sizeof(float));
    }
  }
}

//------------------------------------------------------------------------

// sound/silence generation
// tone turned on/off asynchronously

void
sound_thread(void) {
  for (;;) {
    sem_wait(&clock_fired);

    if (playing) {
      // CWTone keeps playing for awhile after it's turned off,
      // in order to allow for a decay envelope;
      // returns FALSE when it's actually done.
      playing = CWTone(gen);
      send_tone();
    } else {
      send_silence();
      // only let updates run when we've just generated silence
      sem_post(&update_ok);
    }
  }

  pthread_exit(0);
}

//------------------------------------------------------------------------

void
poll_thread(void) {
  struct timespec req, rem;
  req.tv_sec = 0;
  req.tv_nsec = 1000000;
  for (;;) {
    int status;
    nanosleep(&req, &rem);
    if (ioctl(fdser, TIOCMGET, &status) != -1) {
      if (status & TIOCM_DSR)
	poll_status |= L_KEY_DOWN;
      if (status & TIOCM_CTS)
	poll_status |= R_KEY_DOWN;
      sem_post(&poll_action);
    }
  }
  pthread_exit(0);
}

//------------------------------------------------------------------------

// basic heartbeat
// returns actual dur in msec since last tick;
// uses Linux rtc interrupts.
// other strategies will work too, so long as they
// provide a measurable delay in msec.

double
timed_delay(void) {
  double del;
  unsigned long data;
  if (read(fdrtc, &data, sizeof(unsigned long)) == -1) {
    perror("read");
    exit(1);
  }
  del = (data >> 010) * 1000 / (double) RTC_RATE;
  return del;
}

// key down? (real or via keyer logic)

BOOLEAN
read_key(double del) {
  if (iambic)
    return read_iambic_key(ks, kl, del);
  else
    return read_straight_key(ks);
}

//------------------------------------------------------------------------

// main keyer loop

void
key_thread(void) {

  sem_wait(&keyer_started);

  for (;;) {
    double del = timed_delay();
    BOOLEAN keydown = read_key(del);

    if (!playing && keydown) {
      CWToneOn(gen);
      playing = TRUE;
      sem_post(&state_change);
    } else if (playing && !keydown)
      CWToneOff(gen);
      sem_post(&state_change);
    }	

    sem_post(&clock_fired);
  }

  pthread_exit(0);
}

void
state_watcher_thread(void) {

  for (;;) {
    sem_wait(&state_change);
    if (playing)
      notify_keydown();
    else {
      do_hangtime();
      notify_keyup();
    }
  }

  pthread_exit(0);
}

//------------------------------------------------------------------------

// update keyer parameters via text input from stdin
// <wpm xxx>  -> set keyer speed to xxx
// <gain xxx> -> set gain to xxx (dB)
// <freq xxx> -> set freq to xxx
// <ramp xxx> -> set attack/decay times to xxx ms
// <quit>     -> terminate keyer

#define MAX_ESC (512)
#define ESC_L '<'
#define ESC_R '>'

void
update_thread(void) {
  for (;;) {
    int c;

    // get or wait for next input char
    if ((c = getchar()) == EOF) goto finish;

    // if we see the beginning of a command,
    if (c == ESC_L) {
      int i = 0;
      char buf[MAX_ESC];

      // gather up the remainder
      while ((c = getchar()) != EOF) {
	if (c == ESC_R) break;
	buf[i] = c;
	if (++i >= (MAX_ESC - 1)) break;
      }
      if (c == EOF) goto finish;
      buf[i] = 0;

      // wait until changes are safe
      sem_wait(&update_ok);

      if (!strncmp(buf, "wpm", 3))
	ks->wpm = wpm = atof(buf + 3);
      else if (!strncmp(buf, "ramp", 4)) {
	ramp = atof(buf + 4);
	setCWToneGenVals(gen, gain, freq, ramp, ramp);
      } else if (!strncmp(buf, "freq", 4)) {
	freq = atof(buf + 4);
	setCWToneGenVals(gen, gain, freq, ramp, ramp);
      } else if (!strncmp(buf, "gain", 4)) {
	gain = atof(buf + 4);
	setCWToneGenVals(gen, gain, freq, ramp, ramp);
      } else if (!strncmp(buf, "quit", 4))
	goto finish;

    } // otherwise go around again
  }

  // we saw an EOF or quit; kill other threads and exit neatly

 finish:
  pthread_cancel(poller);
  pthread_cancel(play);
  pthread_cancel(key);
  pthread_exit(0);
}

//------------------------------------------------------------------------

PRIVATE void
jack_xrun(void *arg) {
  char *str = "xrun";
  write(2, str, strlen(str));
}

PRIVATE void
jack_shutdown(void *arg) {}

PRIVATE void
jack_callback(jack_nframes_t nframes, void *arg) {
  float *lp, *rp;
  int nbytes = nframes * sizeof(float);
  if (nframes == size) {
    // output: copy from ring to port
    lp = (float *) jack_port_get_buffer(lport, nframes);
    rp = (float *) jack_port_get_buffer(rport, nframes);
    if (jack_ringbuffer_read_space(lring) >= nbytes) {
      jack_ringbuffer_read(lring, (char *) lp, nbytes);
      jack_ringbuffer_read(rring, (char *) rp, nbytes);
    } else { // rb pathology
      memset((char *) lp, 0, nbytes);
      memset((char *) rp, 0, nbytes);
      jack_ringbuffer_reset(lring);
      jack_ringbuffer_reset(rring);
      jack_ringbuffer_clear(lring, TONE_SIZE * sizeof(float));
      jack_ringbuffer_clear(rring, TONE_SIZE * sizeof(float));
      //write(2, "underrun\n", 9); 
    }
  }
}

int
main(int argc, char **argv) {
  int i;
  char *serialdev = "/dev/ttyS0",
       *clockdev = "/dev/rtc";
  int serstatus;

  for (i = 1; i < argc; i++)
    if (argv[i][0] == '-')
      switch (argv[i][1]) {
      case 'f':
	freq = atof(argv[++i]);
	break;
      case 'i':
	iambic = TRUE;
	break;
      case 'g':
	gain = atof(argv[++i]);
	break;
      case 'r':
	ramp = atof(argv[++i]);
	break;
      case 'w':
	wpm = atof(argv[++i]);
	break;
      default:
	fprintf(stderr,
		"iambic-keyer [-i] [-w wpm] [-g gain_dB] [-r ramp_ms]\n");
	exit(1);
      }
    else break;

  if (i < argc) {
    if (!freopen(argv[i], "r", stdin))
      perror(argv[i]), exit(1);
    i++;
  }

  //------------------------------------------------------------

  gen = newCWToneGen(gain, freq, ramp, ramp, TONE_SIZE, 48000.0);

  //------------------------------------------------------------

  kl = newKeyerLogic();
  ks = newKeyerState();
  ks->flag.iambic = TRUE;
  ks->flag.revpdl = TRUE;
  // set On by default; straight key never sees them,
  // mode A users are on their own
  ks->flag.mdlmdB = ks->flag.memory.dit = ks->flag.memory.dah = TRUE;
  ks->flag.autospace.khar = ks->flag.autospace.word = FALSE;
  ks->debounce = 1;
  ks->mode = MODE_B;
  ks->weight = 50;
  ks->wpm = wpm;

  //------------------------------------------------------------

  if (!(client = jack_client_new("ikyr")))
    fprintf(stderr, "can't make client -- jack not running?\n"), exit(1);
  jack_set_process_callback(client, (void *) jack_callback, 0);
  jack_on_shutdown(client, (void *) jack_shutdown, 0);
  jack_set_xrun_callback(client, (void *) jack_xrun, 0);
  size = jack_get_buffer_size(client);
  lport = jack_port_register(client,
			     "ol",
			     JACK_DEFAULT_AUDIO_TYPE,
			     JackPortIsOutput,
			     0);
  rport = jack_port_register(client,
			     "or",
			     JACK_DEFAULT_AUDIO_TYPE,
			     JackPortIsOutput,
			     0);
  lring = jack_ringbuffer_create(RING_SIZE);
  rring = jack_ringbuffer_create(RING_SIZE);
  jack_ringbuffer_clear(lring, TONE_SIZE * sizeof(float));
  jack_ringbuffer_clear(rring, TONE_SIZE * sizeof(float));
  
  //------------------------------------------------------------

  // key
  if ((fdser = open(serialdev, O_WRONLY)) == -1) {
    fprintf(stderr, "cannot open serial device %s", serialdev);
    exit(1);
  }
  if (ioctl(fdser, TIOCMGET, &serstatus) == -1) {
    close(fdser);
    fprintf(stderr, "cannot get serial device status");
    exit(1);
  }
  serstatus |= TIOCM_DTR;
  if (ioctl(fdser, TIOCMSET, &serstatus) == -1) {
    close(fdser);
    fprintf(stderr, "cannot set serial device status");
    exit(1);
  }

  // rtc
  if ((fdrtc = open(clockdev, O_RDONLY)) == -1) {
    perror(clockdev);
    exit(1);
  }
  if (ioctl(fdrtc, RTC_IRQP_SET, RTC_RATE) == -1) {
    perror("ioctl irqp");
    exit(1);
  }
  if (ioctl(fdrtc, RTC_PIE_ON, 0) == -1) {
    perror("ioctl pie on");
    exit(1);
  }

  //------------------------------------------------------------

  sem_init(&poll_action, 0, 0);
  sem_init(&clock_fired, 0, 0);
  sem_init(&keyer_started, 0, 0);
  sem_init(&update_ok, 0, 0);
  sem_init(&state_change, 0, 0);
  pthread_create(&poller, 0, (void *) poll_thread, 0);
  pthread_create(&play, 0, (void *) sound_thread, 0);
  pthread_create(&key, 0, (void *) key_thread, 0);
  pthread_create(&update, 0, (void *) update_thread, 0);
  pthread_create(&outside, 0, (void *) state_change_thread, 0);

  //------------------------------------------------------------

  jack_activate(client);
  {
    const char **ports;
    if (!(ports = jack_get_ports(client, 0, 0, JackPortIsPhysical | JackPortIsInput))) {
      fprintf(stderr, "can't find any physical playback ports\n");
      exit(1);
    }
    if (jack_connect(client, jack_port_name(lport), ports[0])) {
      fprintf(stderr, "can't connect left output\n");
      exit(1);
    }
    if (jack_connect(client, jack_port_name(rport), ports[1])) {
      fprintf(stderr, "can't connect left output\n");
      exit(1);
    }
    free(ports);
  }

  sem_post(&keyer_started);

  pthread_join(poller, 0);
  pthread_join(play, 0);
  pthread_join(key, 0);
  pthread_join(update, 0);
  pthread_join(outside, 0);
  jack_client_close(client);

  //------------------------------------------------------------

  if (ioctl(fdrtc, RTC_PIE_OFF, 0) == -1) {
    perror("ioctl pie off");
    exit(1);
  }
  close(fdrtc);
  close(fdser);

  jack_ringbuffer_free(lring);
  jack_ringbuffer_free(rring);

  sem_destroy(&poll_action);
  sem_destroy(&clock_fired);
  sem_destroy(&keyer_started);
  sem_destroy(&state-change);

  delCWToneGen(gen);
  delKeyerState(ks);
  delKeyerLogic(kl);

  //------------------------------------------------------------

  exit(0);
}
