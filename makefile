CC = gcc

EXECUTABLES = tests

all: $(EXECUTABLES)

INCL = globals.h layer0.h layer1.h
CFILES = layer0.c layer1.c testLayer1.c

USER = layer0.o layer1.o testLayer1.o

tests: $(USER) $(INCL)
	$(CC) -o tests $(USER)

.c.o: $(INCL) $(CFILES)
	$(CC) -c $*.c

clean:
	/bin/rm -f *.o core $(EXECUTABLES) a.out