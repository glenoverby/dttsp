#! /usr/bin/env python
"""
A pair of classes to exercise the spectrum and meter data capture
from an sdr-core. Does fetch on spectrum and meter ports of a running
process.

sdog = spec_doggie([host], [command-port], [spectrum-port])
then
label, stamp, spectrum = sdog.get()

mdog = meter_doggie([host], [command-port], [spectrum-port])
then
label, readings = meter_doggie([host], [command-port], [meter-port])

Standalone usage:

port_doggie [host cmd-port spec-data-port meter-data-port]

Reads lines from stdin
s	means read spectrum
m	means read meter

Default host		localhost
	cmd-port	19001
        spec-data-port	19002
        meter-data-port	19003
"""

import sys
import socket
import select
import numpy as N

CMD_PORT = 19001
SPEC_PORT = 19002
METER_PORT = 19003

SPEC_BUFSIZE = 32768
METER_BUFSIZE = 1024
DUMMY_BUFSIZE = 512

#------------------------------------------------------------------
# base doggie
#------------------------------------------------------------------

class doggie(object):
    """
    A requester for a packet of data from server on (host, port)
    Command issued and ack'ed on cmd_port
    Data returned on data_port
    Base case, simply do command and ignore data
    """

    def __init__(self, host, cmd_port, data_port):
        self.cmd_host = host
        self.cmd_port = cmd_port
        try:
            self.cmd_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        except Exception, e:
            print >>sys.stderr, "Can't open UDP command socket on %r, %r" % (host, cmd_port)
            print >>sys.stderr, type(e)
            sys.exit(1)
        self.cmd_sock.bind(('', 0))
        self.cmd_addr = self.cmd_host, self.cmd_port

        self.data_host = host
        self.data_port = data_port
        try:
            self.data_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        except Exception, e:
            print >>sys.stderr, "Can't open UDP data socket on %r, %r" % (host, data_port)
            print >>sys.stderr, type(e)
            sys.exit(1)
        self.data_sock.bind(('', self.data_port))

    def done(self):
        self.cmd_sock.close()
        self.data_sock.close()

    def _req(self, command):
        """
        Issue command, wait a little for response, check ack
        """
        # make request
        self.reqmsg = command
        self.cmd_sock.sendto(self.reqmsg, self.cmd_addr)
        # wait a bit for ack
        irdy, ordy, erdy = select.select([self.cmd_sock], [], [], 1.0)
        if not self.cmd_sock in irdy:
            print >>sys.stderr, "Late or missing ACK from command"
            return False
        self.ack, fromaddr = self.cmd_sock.recvfrom(DUMMY_BUFSIZE)
        if self.ack <> 'ok':
            print >>sys.stderr, "Command failed"
            return False
        return True

    def _inp(self, timeout):
        """
        Wait a bit for data, then read back
        """
        irdy, ordy, erdy = select.select([self.data_sock], [], [], timeout)
        if not self.data_sock in irdy:
            print >>sys.stderr, "Data late or missing"
            return False
        self.payload, self.fromaddr = self.data_sock.recvfrom(SPEC_BUFSIZE)
        return True

    def get(self, command):
        if not self._req(command):
            print >>sys.stderr, command, 'failed'
        return None


#------------------------------------------------------------------
# derived spectrum doggie
#------------------------------------------------------------------

class spec_doggie(doggie):
    """
    Spectrum fetcher
    """

    def __init__(self,
                 host      = 'localhost',
                 cmd_port  = CMD_PORT,
                 data_port = SPEC_PORT):
        super(spec_doggie, self).__init__(host, cmd_port, data_port)

    def get(self, token = 4277009102):
        self.token = token

        if not self._req('reqSpectrum' + ' '+ str(token) + '\n'):
            print >>sys.stderr, "Command failed"
            return None, None, None

        if not self._inp(1.0):
            print >>sys.stderr, "Spectrum data late or missing"
            return None, None, None
            
        # break out
        self.labelstr = self.payload[0:4]
        self.stampstr = self.payload[4:8]
        self.specpstr = self.payload[8: ]
        label = N.fromstring(self.labelstr, N.uint32)[0]
        stamp = N.fromstring(self.stampstr, N.uint32)[0]
        specp = N.fromstring(self.specpstr, N.float)
        return label, stamp, specp


#------------------------------------------------------------------
# derived meter doggie
#------------------------------------------------------------------

class meter_doggie(doggie):
    """
    Meter fetcher
    """

    def __init__(self,
                 host      = 'localhost',
                 cmd_port  = CMD_PORT,
                 data_port = METER_PORT):
        super(meter_doggie, self).__init__(host, cmd_port, data_port)

    def get(self, token = 3735928559):
        self.token = token

        if not self._req('reqMeter' + ' ' + str(token) + '\n'):
            print >>sys.stderr, "Command failed"
            return None, None

        if not self._inp(1.0):
            print >>sys.stderr, "Spectrum data late or missing"
            return None, None
            
        # break out
        self.labelstr = self.payload[0:4]
        self.mvalsstr = self.payload[4: ]
        label = N.fromstring(self.labelstr, N.uint32)[0]
        mvals = N.fromstring(self.mvalsstr, N.float)
        return label, mvals


#------------------------------------------------------------------
#------------------------------------------------------------------

if __name__ == '__main__':
    if len(sys.argv) == 1:
        host = 'localhost'
        cport = CMD_PORT
        sport = SPEC_PORT
        mport = METER_PORT
    elif len(sys.argv) == 5:
        host = sys.argv[1]
        cport = int(sys.argv[2])
        sport = int(sys.argv[3])
        mport = int(sys.argv[4])
    else:
        print >>sys.stderr, __doc__
        sys.exit(2)

    sdog = spec_doggie(host, cport, sport)
    print >>sys.stderr, "spec doggie is ready"
    mdog = meter_doggie(host, cport, mport)
    print >>sys.stderr, "meter doggie is ready"

    while True:
        try:
            line = sys.stdin.readline()
        except KeyboardInterrupt:
            break;
        if not line:
            break
        if line[0] == 's':
            print 'Fetch spectrum'
            pkt = sdog.get()
            print pkt
#            label, stamp, specp = sdog.get()
#            print 'Got', label, stamp, specp
        elif line[0] == 'm':
            print 'Fetch meter'
            pkt = mdog.get()
            print pkt
#            label, mvals = mdog.get()
#            print 'Got', label, mvals
        else:
            print 'Can\'t do that'

    sdog.done()
    mdog.done()
