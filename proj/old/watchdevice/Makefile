INCLUDES=
LIBS=
COMPILE=gcc ${INCLUDES}
LINK=gcc ${LIBS}
WATCHDEVICED_OBJS = daemonize.o  sighandler.o  watchdeviced.o

all: watchdeviced

watchdeviced: ${WATCHDEVICED_OBJS}
	${LINK} ${WATCHDEVICED_OBJS} -o watchdeviced

%.o: %.c
	$(COMPILE) -o $@ -c $<

clean:
	rm -f *.o watchdeviced
