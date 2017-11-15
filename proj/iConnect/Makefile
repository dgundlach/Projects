INCLUDES=-I ../lib
LIBS=
COMPILE=gcc ${INCLUDES}
LINK=gcc ${LIBS}
WATCHDEVICE_OBJS = ../lib/xalloc.o ../lib/mkdirp.o ../lib/openp.o \
					../lib/daemonize.o ../lib/sighandler.o watchdevice.o
WATCHBUTTOND_OBJS = ../lib/xalloc.o ../lib/mkdirp.o ../lib/openp.o \
					../lib/daemonize.o ../lib/sighandler.o watchbuttond.o

all: watchdevice watchbuttond

watchdevice: ${WATCHDEVICE_OBJS}
	${LINK} ${WATCHDEVICE_OBJS} -o watchdevice

watchbuttond: ${WATCHBUTTOND_OBJS}
	${LINK} ${WATCHBUTTOND_OBJS} -o watchbuttond

%.o: %.c
	$(COMPILE) -o $@ -c $<

clean:
	rm -f *.o ../lib/*.o watchdevice watchbuttond
