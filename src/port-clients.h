/* port-clients.h */
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

#ifndef PORT_CLIENTS_H
#define PORT_CLIENTS_H

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>  
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/socket.h>  

#include <netinet/in.h>
#include <arpa/inet.h>

#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define DTTSP_PORT_CLIENT_COMMAND 19001
#define DTTSP_PORT_CLIENT_SPECTRUM 19002
#define DTTSP_PORT_CLIENT_METER 19003
#define DTTSP_PORT_CLIENT_BUFSIZE 65536

typedef struct _dttsp_port_client {
  unsigned short port;
  struct sockaddr_in clnt;
  int clen, flags, sock;
  char buff[DTTSP_PORT_CLIENT_BUFSIZE];
  int size, used;
} dttsp_port_client_t;

extern int send_command(dttsp_port_client_t *cp, char *cmdstr);
extern int fetch_spectrum(dttsp_port_client_t *cp,
			  int *tick, int *label, float *data, int npts);
extern int fetch_meter(dttsp_port_client_t *cp,
		       int *label, float *data, int npts);
extern dttsp_port_client_t *new_dttsp_port_client(int port, int inbound);
extern void del_dttsp_port_client(dttsp_port_client_t *cp);

#endif
