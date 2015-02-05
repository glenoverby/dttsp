#!/usr/bin/env python
# ztone2 rate dur Hz_A dB_A Hz_B dB_B file

import sys
import wave
import math as M
import numpy as N

if len(sys.argv) <> 8:
    print >>sys.stderr, "usage:"
    print >>sys.stderr, "ztone2.py rate dur A dB_A B dB_B file"
    sys.exit(1)

rate = float(sys.argv[1])
dur  = float(sys.argv[2])
Hz_A = float(sys.argv[3])
dB_A = float(sys.argv[4])
Hz_B = float(sys.argv[5])
dB_B = float(sys.argv[6])
name = sys.argv[7]

fp = wave.open(name, 'wb')

nchannels = 2
sampwidth = 2
framerate = int(rate)
nframes   = int(rate*dur+0.5)
comptype  = "NONE"
compname  = "NONE"

fp.setparams((nchannels,
              sampwidth,
              framerate,
              nframes,
              comptype,
              compname))

twopi = 2.0*M.pi
nbuf  = 1024

scl_A = 32768*(10.0**(float(dB_A)/20.0))
phs_A = 0.0
inc_A = twopi*Hz_A/rate

scl_B = 32768*(10.0**(float(dB_B)/20.0))
phs_B = 0.0
inc_B = twopi*Hz_B/rate

while nframes > 0:
    num = min(nbuf, nframes)
    buf = N.zeros((num,2))

    for i in range(num):

        buf[i][0]  = M.cos(phs_A)*scl_A
        buf[i][1]  = M.sin(phs_A)*scl_A
        buf[i][0] += M.cos(phs_B)*scl_B
        buf[i][1] += M.sin(phs_B)*scl_B

        phs_A += inc_A
        phs_B += inc_B

    fp.writeframes(buf.clip(-32768,32767.999).astype(N.int16).tostring())

    while phs_A >= +twopi: phs_A -= twopi
    while phs_A <= -twopi: phs_A += twopi
    while phs_B >= +twopi: phs_B -= twopi
    while phs_B <= -twopi: phs_B += twopi

    nframes -= num

fp.close()
