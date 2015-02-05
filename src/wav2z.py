#!/usr/bin/env python

import sys
import wave
import math as M
import numpy as N
import hilbert as H

if len(sys.argv) <> 3:
    print >>sys.stderr, "usage:"
    print >>sys.stderr, "wav2z infile outfile"
    sys.exit(1)

try:
    iwav = wave.open(sys.argv[1], 'rb')
except:
    print >>sys.stderr, "Can't open ", sys.argv[1], " for reading."
    sys.exit(1)

(nchannels,
 sampwidth,
 framerate,
 nframes,
 comptype,
 compname) = iwav.getparams()

if nchannels <> 1:
    print >>sys.stderr, sys.argv[1], " is not a mono file."
    sys.exit(1)
if sampwidth <> 2:
    print >>sys.stderr, sys.argv[1], " is not 16t."
    sys.exit(1)

try:
    owav = wave.open(sys.argv[2], 'wb')
except:
    print >>sys.stderr, "Can't open ", sys.argv[2], " for writing."
    sys.exit(1)

nchannels = 2
owav.setparams((nchannels,
                sampwidth,
                framerate,
                nframes,
                comptype,
                compname))

filt = H.HilbertTransformer()
nbuf = 1024

while nframes > 0:
    iraw = iwav.readframes(nbuf)
    ngot = len(iraw)
    ibuf = N.fromstring(iraw, dtype=N.int16).astype(float)
    ibuf /= 32768.0

    ocpx = filt.run(ibuf)

    obuf = N.array([ocpx.real, ocpx.imag])
    obuf = obuf.clip(-1, 0.999999)
    obuf *= 32768.0
    obuf = obuf.transpose()
    obuf = obuf.astype(N.int16)
    oraw = obuf.tostring()

    owav.writeframes(oraw)
    nframes -= ngot

iwav.close()
owav.close()

