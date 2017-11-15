DEFINES=
LIBS=
CFLAGS= -O2
INSTALLDIR=/usr/bin
CC=gcc

ETCDIR=/etc/vauth

CMN_OBJS=Prompt.o Flock.o Split.o
EXECUTABLES=newaccount
DEFINES=-DETCDIR=$(ETCDIR)

all:	newaccount

clean:
	rm -f *.o $(EXECUTABLES)

newaccount: newaccount.o $(CMN_OBJS)
	$(CC) $(CFLAGS) -o newaccount newaccount.o $(CMN_OBJS) $(LIBS)

newaccount.o: newaccount.c
	$(CC) $(CFLAGS) $(DEFINES) -c newaccount.c

Prompt.o: Prompt.c
	$(CC) $(CFLAGS) -c Prompt.c

Flock.o: Flock.c
	$(CC) $(CFLAGS) -c Flock.c

Split.o: Split.c
	$(CC) $(CFLAGS) -c Split.c
