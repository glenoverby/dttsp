/** 
* @file isoband.c
* @brief Function to implement isobands
* @author Frank Brickle, AB2KT and Bob McGwier, N4HY 


This file is part of a program that implements a Software-Defined Radio.

Copyright (C) 2006, 2007 by Frank Brickle, AB2KT and Bob McGwier, N4HY
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

#include <isoband.h>

typedef
struct _ISOband_t {
  REAL nominal, exact, low, high;
} ISOband_t;

static
ISOband_t _ISOband_info[] = {
  { /* 1 */      1.25,	  1.26,     1.12,    1.41 },
  { /* 2 */      1.6,	  1.58,     1.41,    1.78 },
  { /* 3 */      2.0,	  2.00,     1.78,    2.24 },
  { /* 4 */      2.5,	  2.51,     2.24,    2.82 },
  { /* 5 */      3.15,	  3.16,     2.82,    3.55 },
  { /* 6 */      4.0,	  3.98,     3.55,    4.47 },
  { /* 7 */      5.0,	  5.01,     4.47,    5.62 },
  { /* 8 */      6.3,	  6.31,     5.62,    7.08 },
  { /* 9 */      8.0,	  7.94,     7.08,    8.91 },
  { /* 10 */    10.0,	 10.0,      8.91,   11.2  },
  { /* 11 */    12.5,	 12.59,    11.2,    14.1  },
  { /* 12 */    16.0,	 15.85,    14.1,    17.8  },
  { /* 13 */    20.0,	 19.95,    17.8,    22.4  },
  { /* 14 */    25.0,	 25.12,    22.4,    28.2  },
  { /* 15 */    31.5,	 31.62,    28.2,    35.5  },
  { /* 16 */    40.0,	 39.81,    35.5,    44.7  },
  { /* 17 */    50.0, 	 50.12,    44.7,    56.2  },
  { /* 18 */    63.0, 	 63.10,    56.2,    70.8  },
  { /* 19 */    80.0,    79.43,    70.8,    89.1  },
  { /* 20 */   100.0,   100.00,    89.1,   112.0  },
  { /* 21 */   125.0,   125.89,   112.0,   141.0  },
  { /* 22 */   160.0,   158.49,   141.0,   178.0  },
  { /* 23 */   200.0,   199.53,   178.0,   224.0  },
  { /* 24 */   250.0,   251.19,   224.0,   282.0  },
  { /* 25 */   315.0,   316.23,   282.0,   355.0  },
  { /* 26 */   400.0,   398.11,   355.0,   447.0  },
  { /* 27 */   500.0,   501.19,   447.0,   562.0  },
  { /* 28 */   630.0,   630.96,   562.0,   708.0  },
  { /* 29 */   800.0,   794.33,   708.0,   891.0  },
  { /* 30 */  1000.0,  1000.0,    891.0,  1120.0  },
  { /* 31 */  1250.0,  1258.9,   1120.0,  1410.0  },
  { /* 32 */  1600.0,  1584.9,   1410.0,  1780.0  },
  { /* 33 */  2000.0,  1995.3,   1780.0,  2240.0  },
  { /* 34 */  2500.0,  2511.9,   2240.0,  2820.0  },
  { /* 35 */  3150.0,  3162.3,   2820.0,  3550.0  },
  { /* 36 */  4000.0,  3981.1,   3550.0,  4470.0  },
  { /* 37 */  5000.0,  5011.9,   4470.0,  5620.0  },
  { /* 38 */  6300.0,  6309.6,   5620.0,  7080.0  },
  { /* 39 */  8000.0,  7943.3,   7080.0,  8910.0  },
  { /* 40 */ 10000.0, 10000.0,   8910.0, 11200.0  },
  { /* 41 */ 12500.0, 12589.3,  11200.0, 14100.0  },
  { /* 42 */ 16000.0, 15848.9,  14100.0, 17800.0  },
  { /* 43 */ 20000.0, 19952.6,  17800.0, 22400.0  }
};

/* -------------------------------------------------------------------------- */
/** @brief Get information on ISObands 
* 
* @param band 
* @return ISOband_t
*/
/* ---------------------------------------------------------------------------- */
PRIVATE INLINE ISOband_t *
ISOband_get_info(int band) {
  if (band < 1 || band > 43) {
    fprintf(stderr, "ISO: band index out of range (%d)\n", band);
    exit(1);
  } else
    return &_ISOband_info[band - 1];
}

/* -------------------------------------------------------------------------- */
/** @brief Get nominal ISObands 
* 
* @param band 
* @return REAL
*/
/* ---------------------------------------------------------------------------- */
REAL
ISOband_get_nominal(int band) {
  return ISOband_get_info(band)->nominal;
}

/* -------------------------------------------------------------------------- */
/** @brief Get exact ISOband
* 
* @param band 
* @return REAL
*/
/* ---------------------------------------------------------------------------- */
REAL
ISOband_get_exact(int band) {
  return ISOband_get_info(band)->exact;
}

/* -------------------------------------------------------------------------- */
/** @brief Get low ISOband
* 
* @param band 
* @return REAL
*/
/* ---------------------------------------------------------------------------- */
REAL
ISOband_get_low(int band) {
  return ISOband_get_info(band)->low;
}

/* -------------------------------------------------------------------------- */
/** @brief Get high ISOband 
* 
* @param band 
* @return REAL
*/
/* ---------------------------------------------------------------------------- */
REAL
ISOband_get_high(int band) {
  return ISOband_get_info(band)->high;
}
