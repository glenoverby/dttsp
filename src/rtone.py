#!/usr/bin/env python
# rtone rate dur Hz dB file

import sys
import wave
import math as M
import numpy as N

if len(sys.argv) <> 6:
    print >>sys.stderr, "usage:"
    print >>sys.stderr, "rtone.py rate dur freq dB file"
    sys.exit(1)

rate = float(sys.argv[1])
dur  = float(sys.argv[2])
Hz   = float(sys.argv[3])
dB   = float(sys.argv[4])
name = sys.argv[5]

fp = wave.open(name, 'wb')

nchannels = 1
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
scl   = 32768*(10.0**(float(dB)/20.0))
phs   = 0.0
inc   = twopi*Hz/rate

while nframes > 0:
    num = min(nbuf, nframes)
    buf = N.zeros(num)
    for i in range(num):
        buf[i] = M.cos(phs)*scl
        phs += inc
    fp.writeframes(buf.clip(-32768,32767.999).astype(N.int16).tostring())
    while phs >= +twopi: phs -= twopi
    while phs <= -twopi: phs += twopi
    nframes -= num

fp.close()
