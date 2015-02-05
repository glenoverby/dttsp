/* port-clients.c */
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

#include <port-clients.h>

// hygeinic local copy of command

static int
copy_and_clean(dttsp_port_client_t *cp, char *cmdstr) {
  int i, nl = 0;

  for (i = 0; i < DTTSP_PORT_CLIENT_BUFSIZE - 1; i++) {
    cp->buff[i] = cmdstr[i];
    if (cp->buff[i] == '\n')
      nl = 1;
    if (cp->buff[i] == 0)
      break;
  }
  if (i >= DTTSP_PORT_CLIENT_BUFSIZE - 1)
    return -1;
  if (!nl) {
    cp->buff[i++] = '\n';
    cp->buff[i] = 0;
  }
  return cp->used = i;
}

// send the command
// success return: 0
// error returns:
// -1: port and/or command pointers are invalid
// -2: command itself is borked
// -3: failed to send command
// -4: failed to set up ack from command, probable timeout
// -5: failed to get timely response from command
// -6: command itself failed

int
send_command(dttsp_port_client_t *cp, char *cmdstr) {

  // are we pointing at the moon?
  if (!cp || !cmdstr)
    return -1;

  // make local, properly terminated copy of command
  if (copy_and_clean(cp, cmdstr) <= 0)
    return -2;

  // blast it

  cp->clen = sizeof(cp->clnt);
  memset((char *) &cp->clnt, 0, cp->clen);
  cp->clnt.sin_family = AF_INET;
  cp->clnt.sin_addr.s_addr = htonl(INADDR_ANY);
  cp->clnt.sin_port = htons((unsigned short) cp->port);

  if (sendto(cp->sock, cp->buff, cp->used, cp->flags,
	     (struct sockaddr *) &cp->clnt, cp->clen) != cp->used)
    return -3;

  // wait a little for ack
  {
    fd_set fds;
    struct timeval tv;

    FD_ZERO(&fds);
    FD_SET(cp->sock, &fds);
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    if (!select(cp->sock + 1, &fds, 0, 0, &tv))
      return -4;
    if (recvfrom(cp->sock, cp->buff, cp->size, cp->flags,
		 (struct sockaddr *) &cp->clnt, &cp->clen) <= 0)
      return -5;

    if (cp->buff[0] != 'o' || cp->buff[1] != 'k')
      return -6;
  }

  return 0;
}

int
fetch_spectrum(dttsp_port_client_t *cp,
	       int *tick, int *label, float *data, int npts) {
  fd_set fds;
  struct timeval tv;

  // wait a bit for data to appear
  FD_ZERO(&fds);
  FD_SET(cp->sock, &fds);
  tv.tv_sec = 1;
  tv.tv_usec = 0;
  if (!select(cp->sock + 1, &fds, 0, 0, &tv))
    return -1;
  if (recvfrom(cp->sock, cp->buff, cp->size, cp->flags,
	       (struct sockaddr *) &cp->clnt, &cp->clen) <= 0)
    return -2;

  // copy payload back to client space
  memcpy((char *) tick, cp->buff, sizeof(int));
  memcpy((char *) label, cp->buff + sizeof(int), sizeof(int));
  memcpy((char *) data, cp->buff + 2 * sizeof(int), npts * sizeof(float));
  return 0;
}

int
fetch_meter(dttsp_port_client_t *cp, int *label, float *data, int npts) {
  fd_set fds;
  struct timeval tv;

  // wait a bit for data to appear
  FD_ZERO(&fds);
  FD_SET(cp->sock, &fds);
  tv.tv_sec = 1;
  tv.tv_usec = 0;
  if (!select(cp->sock + 1, &fds, 0, 0, &tv))
    return -1;
  if (recvfrom(cp->sock, cp->buff, cp->size, cp->flags,
	       (struct sockaddr *) &cp->clnt, &cp->clen) <= 0)
    return -2;

  // copy payload back to client space
  memcpy((char *) label, cp->buff, sizeof(int));
  memcpy((char *) data, cp->buff + sizeof(int), npts * sizeof(float));
  return 0;
}

dttsp_port_client_t *
new_dttsp_port_client(int port, int inbound) {
  dttsp_port_client_t *cp;

  if (!(cp = (dttsp_port_client_t *) malloc(sizeof(dttsp_port_client_t)))) {
    perror("Couldn't allocate dttsp_port_client structure");
    exit(1);
  }

  cp->port = port;

  // create socket 
  if ((cp->sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    perror("Couldn't create dttsp_port_client socket");
    exit(1);
  }
  
  cp->flags = 0;

  // are we the listener here?
  if (inbound) {
    cp->clen = sizeof(cp->clnt);
    memset((char *) &cp->clnt, 0, cp->clen);
    cp->clnt.sin_family = AF_INET;
    cp->clnt.sin_addr.s_addr = htonl(INADDR_ANY);
    cp->clnt.sin_port = htons((unsigned short) cp->port);
   
    if (bind(cp->sock, (struct sockaddr *) &cp->clnt, cp->clen) < 0) {
      perror("Failed to bind socket");
      exit(1);
    }
  } // else no, so sockaddr gets filled in at point of use, not bound by us

  // one size fits all
  cp->size = DTTSP_PORT_CLIENT_BUFSIZE;
  memset(cp->buff, 0, cp->size);

  return cp;
}

void
del_dttsp_port_client(dttsp_port_client_t *cp) {
  if (cp) {
    close(cp->sock);
    free(cp);
  }
}
