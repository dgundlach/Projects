INCLUDES=-I lib
LIBS=
COMPILE=gcc ${INCLUDES}
LINK=gcc ${LIBS}
WATCHDEVICED_OBJS = lib/mkdirp.o lib/xalloc.o lib/openp.o lib/daemonize.o \
	lib/sighandler.o  watchdeviced.o

all: watchdeviced

watchdeviced: ${WATCHDEVICED_OBJS}
	${LINK} ${WATCHDEVICED_OBJS} -o watchdeviced

%.o: %.c
	$(COMPILE) -o $@ -c $<

clean:
	rm -f *.o watchdeviced
