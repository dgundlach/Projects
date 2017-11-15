OPTS=-O3
OBJS=hexdump.o work.o split.o psplit.o split_ready.o
TARGETS=work

default: it

clean:
	rm -f ${OBJS} ${TARGETS}

checkpw.o: \
checkpw.c
	cc ${OPTS} -o checkpw.o -c checkpw.c

chomp.o: \
chomp.c
	cc ${OPTS} -o chomp.o -c chomp.c

getpeerip.o: \
getpeerip.c
	cc ${OPTS} -o getpeerip.o -c getpeerip.c

hexdump.o: \
hexdump.c
	cc ${OPTS} -o hexdump.o -c hexdump.c

it: \
work

psplit.o: \
psplit.c split.h
	cc ${OPTS} -o psplit.o -c psplit.c

split.o: \
split.c split.h
	cc ${OPTS} -o split.o -c split.c

split_ready.o: \
split_ready.c split.h
	cc ${OPTS} -o split_ready.o -c split_ready.c

work.o: \
work.c hexdump.h
	cc ${OPTS} -o work.o -c work.c

work: \
${OBJS}
	cc ${OPTS} -o work ${OBJS}
