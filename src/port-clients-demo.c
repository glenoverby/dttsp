/* port-clients-demo.c */
/*
This file is part of a program that implements a Software-Defined Radio.

Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 by Frank Brickle, AB2KT and Bob McGwier, N4HY

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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#include <port-clients.h>

// default no. pts in a single spectral frame
#define SPECPTS 4096
// default max no. simultaneous receivers possible in one sdr-core
#define NRX 4
// no. different meter values on RX
#define RXMPTS 5
// no. different meter values on TX
#define TXMPTS 10

int
main(int argc, char **argv) {
  dttsp_port_client_t *cp, *sp, *mp;

  // create the command, spectrum, and meter ports

  // this does a blind send to a port that's already bound,
  // so it's outbound == !inbound -> inbound = 0
  cp = new_dttsp_port_client(DTTSP_PORT_CLIENT_COMMAND, 0);

  // these need to suck up blind sends from elsewhere,
  // so they need to be bound, hence inbound = 1
  sp = new_dttsp_port_client(DTTSP_PORT_CLIENT_SPECTRUM, 1);
  mp = new_dttsp_port_client(DTTSP_PORT_CLIENT_METER, 1);

#if 0
  // issue an assortments of commands
  // NB error checking left out for clarity

  send_command(cp, "setMode 0 0");
  send_command(cp, "setFilter -4000 -200");
  send_command(cp, "setOsc 6000.0");
#endif

  // now a bunch of spectrum and meter requests

  {
    float spec[SPECPTS], meas[NRX * RXMPTS];
    int tick, label;
    int n = 15;
    unsigned int usecs = 1e6 / 15;
    
    while (n-- > 0) {
      send_command(cp, "reqSpectrum 4277009102");
      fetch_spectrum(sp, &tick, &label, spec, SPECPTS);
      
      send_command(cp, "reqMeter 3735928559");
      fetch_meter(mp, &label, meas, NRX * RXMPTS);
#if 0
      // if we were fetching TX meter data, this would be
      fetch_meter(mp, &label, meas, TXMPTS);
#endif
      
      // wait a bit
      usleep(usecs);
    }
  }
    
  // clean up

  del_dttsp_port_client(cp);
  del_dttsp_port_client(sp);
  del_dttsp_port_client(mp);

  exit(0);
}
