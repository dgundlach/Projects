INCLUDES=`pkg-config glib-2.0 --cflags`
LIBS=`pkg-config glib-2.0 --libs`
COMPILE=gcc ${INCLUDES}
LINK=gcc ${LIBS}

all: array_mon splittest x

array_mon: array_mon.o split.o daemonize.o FastBitCount.o scsi.o readtextline.o
	${LINK} readtextline.o split.o FastBitCount.o scsi.o daemonize.o array_mon.o -o array_mon

splittest: splittest.o split.o readtextline.o
	${LINK} splittest.o split.o readtextline.o -o splittest

x: x.o split.o FastBitCount.o scsi.o readtextline.o
	${LINK} -o x x.o split.o FastBitCount.o scsi.o readtextline.o

%.o: %.c
	$(COMPILE) -o $@ -c $<

clean:
	rm -f *.o array_mon
