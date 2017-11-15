OPTS=-O3
WORKOBJS=hexdump.o work.o split.o psplit.o split_ready.o
UPDATEDNSOBJS=updatedns.o split_ready.o psplit.o getpeerip.o gohome.o salt.o
TARGETS=work updatedns

default: it

clean:
	rm -f ${WORKOBJS} ${UPDATEDNSOBJS} ${TARGETS}

checkpw.o: \
checkpw.c
	cc ${OPTS} -o checkpw.o -c checkpw.c

chomp.o: \
chomp.c
	cc ${OPTS} -o chomp.o -c chomp.c

getpeerip.o: \
getpeerip.c
	cc ${OPTS} -o getpeerip.o -c getpeerip.c

gohome.o: \
gohome.c
	cc ${OPTS} -o gohome.o -c gohome.c

hexdump.o: \
hexdump.c
	cc ${OPTS} -o hexdump.o -c hexdump.c

it: \
work updatedns

psplit.o: \
psplit.c split.h
	cc ${OPTS} -o psplit.o -c psplit.c

salt.o: \
salt.c salt.h
	cc ${OPTS} -o salt.o -c salt.c

split.o: \
split.c split.h
	cc ${OPTS} -o split.o -c split.c

split_ready.o: \
split_ready.c split.h
	cc ${OPTS} -o split_ready.o -c split_ready.c

updatedns.o: \
updatedns.c split.h getpeerip.h gohome.h salt.h
	cc ${OPTS} -o updatedns.o -c updatedns.c

updatedns: \
${UPDATEDNSOBJS}
	cc ${OPTS} -o updatedns ${UPDATEDNSOBJS} -lcrypt

work.o: \
work.c hexdump.h
	cc ${OPTS} -o work.o -c work.c

work: \
${WORKOBJS}
	cc ${OPTS} -o work ${WORKOBJS}
