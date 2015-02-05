/** 
* @file meter.c
* @brief Functions to implement a meter
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

#include <meter.h>


/* -------------------------------------------------------------------------- */
/** @brief Snapshot of Receive meter 
* 
* snapshots of current measurements
*
* @param mb 
* @param label 
* @return void
*/
/* ---------------------------------------------------------------------------- */
void
snap_meter_rx(METERBlock *mb, int label) {
  memcpy ((char *) mb->snap.rx,
	  (char *) mb->rx.val,
	  MAXRX * RXMETERPTS * sizeof (REAL));
  mb->label = label;
  mb->last = METER_LAST_RX;
}

/* -------------------------------------------------------------------------- */
/** @brief Snapshot of the Transmit meter 
* 
* @param mb 
* @param label 
* @return void
*/
/* ---------------------------------------------------------------------------- */
void
snap_meter_tx(METERBlock *mb, int label) {
  memcpy ((char *) &mb->snap.tx,
	  (char *) &mb->tx.val,
	  TXMETERPTS * sizeof (REAL));
  mb->label = label;
  mb->last = METER_LAST_TX;
}
