#! /usr/bin/env python
"""
A bogus data generator for exercising port_doggie.
Some slippers and a newspaper for the doggie to fetch.

Standalone usage:

port_slippers [address cmd-port spec-data-port meter-data-port]

Default address		localhost
        cmd-port	19001
        spec-data-port	19002
        meter-data-port	19003
"""

import sys
from socket import *
import select
import numpy as N
import math as M

HOST = 'localhost'

CMD_PORT = 19001
SPEC_PORT = 19002
METER_PORT = 19003

CMD_BUFSIZE = 1024
SPEC_BUFSIZE = 32768
METER_BUFSIZE = 1024
DUMMY_BUFSIZE = 512

def main():
    
    host = HOST
    cmd_port = CMD_PORT
    spec_port = SPEC_PORT
    meter_port = METER_PORT

    if len(sys.argv) == 5:
        host = sys.argv[1]
        cmd_port = int(sys.argv[2])
        spec_port = int(sys.argv[3])
        meter_port = int(sys.argv[4])
    elif len(sys.argv) <> 1:
        print __doc__
        sys.exit(1)

    csock = socket(AF_INET, SOCK_DGRAM)
    csock.bind(('', cmd_port))
    print >>sys.stderr, "slippers command port (%r, %r) ready" % (host, cmd_port)

    ssock = socket(AF_INET, SOCK_DGRAM)
    ssock.bind(('', 0))
    spec_addr = host, spec_port
    print >>sys.stderr, "slippers spectrum port (%r, %r) ready" % (host, spec_port)

    msock = socket(AF_INET, SOCK_DGRAM)
    msock.bind(('', 0))
    meter_addr = host, meter_port
    print >>sys.stderr, "slippers meter port (%r, %r) ready" % (host, meter_port)

    tick = 0

    while True:

        # Wait for request to take snapshot of spectrum or meters
        try:
            data, addr = csock.recvfrom(CMD_BUFSIZE)
        except KeyboardInterrupt:
            break
        # ack
        csock.sendto('ok', addr)

        # Parse request (a little)
        field = data.strip().split(' ')

        # empty?
        if len(field) < 1:
            continue

        # has a label?
        if len(field) > 1:
            label = int(field[1])
        else:
            label = 3405705229L

        # spectrum?
        if field[0] == 'reqSpectrum':
            # 2 words + 4096 floats
            shead = N.array([label, tick]).astype(N.uint32).tostring()
            spayl = N.array(4096*[-M.pi]).astype(N.float).tostring()
            sdata = shead + spayl
            ssock.sendto(sdata, spec_addr)
            print >>sys.stderr, "Returned spectrum"
            tick += 1

        # meter?
        elif field[0] == 'reqMeter':
            # 1 word + 20 floats (RX case)
            mhead = N.array([label]).astype(N.uint32).tostring()
            mpayl = N.array(20*[M.e]).astype(N.float).tostring()
            mdata = mhead + mpayl
            msock.sendto(mdata, meter_addr)
            print >>sys.stderr, "Returned meter"

        else:
            print >>sys.stderr, "Garbled command"

    # 

    csock.close()
    ssock.close()
    msock.close()

main()
