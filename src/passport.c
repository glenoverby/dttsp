/* passport.c */
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

int port = DTTSP_PORT_CLIENT_COMMAND;
char line[8192];
dttsp_port_client_t *cp;

int
main(int argc, char **argv) {
  int i;
  int verbose = 0;

  // optional port number for command socket

  for (i = 1; i < argc; i++)
    if (argv[i][0] == '-')
      switch (argv[i][1]) {
      case 'p':
	port = atoi(argv[++i]);
	break;
      case 'v':
	verbose++;
	break;
      default:
	fprintf(stderr, "usage: passport [-p port] [-v] [filename]\n");
	exit(1);
      }
    else
      break;

  // optionally open file or fifo as stdin

  if (i < argc) {
    if (!freopen(argv[i], "r", stdin))
      perror(argv[i]), exit(1);
    i++;
  }
  
  // open port to sdr-core

  if (!(cp = new_dttsp_port_client(port, 0))) {
    fprintf(stderr, "Can't create command port on %d\n", port);
    exit(1);
  }

  // main loop

  while (fgets(line, sizeof(line), stdin)) {
    int err;
    if (verbose)
	printf(">%s", line);
    err = send_command(cp, line);
    if (err < 0)
      fprintf(stderr, "command failed with %d:\n%s\n%s\n", err, line, cp->buff);
    if (verbose)
	printf("<%s", cp->buff);
  }

  del_dttsp_port_client(cp);
  exit(0);
}
