CFLAGS = -g -I.
LIBS = -lm

demo:	port-clients.o port-clients-demo.o
	$(CC) -o port-clients-demo port-clients-demo.o port-clients.o $(LIBS)

passport: passport.o port-clients.o
	$(CC) -o passport passport.o port-clients.o $(LIBS)

clean:
	/bin/rm -f port-clients-demo.o port-clients.o

