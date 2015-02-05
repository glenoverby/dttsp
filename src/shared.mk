CFLAGS = -fPIC -O3 -g -I.
SHAREDLIBS = -L ../lib -lDttSP -ljack -lpthread -lfftw3f -lm

sharedlibname = libDttSP.so

OBJ =	am_demod.o\
	banal.o\
	bufvec.o\
	correctIQ.o\
	cwtones.o\
	cxops.o\
	dcblock.o\
	dttspagc.o\
	fastrig.o\
	filter.o\
	graphiceq.o\
	isoband.o\
	fm_demod.o\
	lmadf.o\
	meter.o\
	noiseblanker.o\
	oscillator.o\
	ovsv.o\
	resample.o\
	ringb.o\
	sdr.o\
	sdrexport.o\
	spectrum.o\
	speechproc.o\
	splitfields.o\
	spottone.o\
	thunk.o\
	window.o\
	wscompand.o\
	update.o

all:	obj sharedlib sdr-core-shared

sdr-core-shared:	sdr-main.o
	$(CC) -o sdr-core sdr-main.o $(SHAREDLIBS)


$(OBJ): sdrexport.h

obj:	$(OBJ)

clean:
	/bin/rm -f *.o ../bin/sdr-core $(staticlibname)

sharedlib:	$(OBJ)
	$(CC) -shared -o $(sharedlibname) $(OBJ)
	mv libDttSP.so ../lib

install:	sdr-core-shared
	mv sdr-core ../bin
