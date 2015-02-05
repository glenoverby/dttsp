CFLAGS = -O3 -I.
#CFLAGS = -g -O -I.
LIBS = ./libDttSP.a -ljack -lpthread -lfftw3f -lm

iambic-keyer:	iambic-keyer.o
	$(CC) -O3 -o iambic-keyer iambic-keyer.o $(LIBS)

keyboard-keyer:	keyboard-keyer.o
	$(CC) -o keyboard-keyer keyboard-keyer.o $(LIBS)

install:	iambic-keyer keyboard-keyer
	mv iambic-keyer keyboard-keyer ../bin

clean:
	/bin/rm -f *.o iambic-keyer keyboard-keyer\
			 ../bin/iambic-keyer ../bin/keyboard-keyer 

