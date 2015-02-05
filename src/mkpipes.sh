#!/bin/sh

CMDPIPE=${SDR_PARMPATH:-/dev/shm/SDRcommands}
if [ ! -p $CMDPIPE ]; then
    mkfifo $CMDPIPE
fi

SPECPIPE=${SDR_SPECPATH:-/dev/shm/SDRspectrum}
if [ ! -p $SPECPIPE ]; then
    mkfifo $SPECPIPE
fi

METERPIPE=${SDR_METERPATH:-/dev/shm/SDRmeter}
if [ ! -p $METERPIPE ]; then
    mkfifo $METERPIPE
fi
