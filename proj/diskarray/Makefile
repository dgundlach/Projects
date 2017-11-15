INCLUDES=
LIBS=-lblkid
COMPILE=gcc ${INCLUDES}
LINK=gcc ${LIBS}
ARRAY_MON_OBJS = array_mon.o split2.o daemonize.o FastBitCount.o scsi.o readtextline.o hexdump.o xalloc.o
SPLITTEST_OBJS = splittest.o split2.o readtextline.o xalloc.o
X_OBJS = x.o split2.o FastBitCount.o scsi.o hexdump.o readtextline.o xalloc.o
Z_OBJS = z.o scsi.o FastBitCount.o split2.o hexdump.o readtextline.o xalloc.o
SYSBLOCK_OBJS = sysblock.o iniparse.o initokenize.o skipspace.o xalloc.o

all: array_mon splittest x z sysblock

array_mon: ${ARRAY_MON_OBJS}
	${LINK} ${ARRAY_MON_OBJS} -o array_mon

splittest: ${SPLITTEST_OBJS}
	${LINK} ${SPLITTEST_OBJS} -o splittest

sysblock: iniparse.h initokenize.h skipspace.h ${SYSBLOCK_OBJS}
	${LINK} ${SYSBLOCK_OBJS} -o sysblock

x: ${X_OBJS}
	${LINK} ${X_OBJS} -o x

z: ${Z_OBJS}
	${LINK} ${Z_OBJS} -o z

%.o: %.c
	$(COMPILE) -o $@ -c $<

clean:
	rm -f *.o array_mon splittest x z
