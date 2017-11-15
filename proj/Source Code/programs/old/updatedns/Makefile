OPTS=-O3
WORKOBJS=hexdump.o work.o split.o psplit.o split_ready.o
UPDATEDNSOBJS=updatedns-new.o split_ready.o psplit.o getpeerip.o gohome.o \
	salt.o md5.o md5_digest.o splitargs.o command.o
CLIENTOBJS=getconfig.o updatedns-client.o split.o split_ready.o
TARGETS=work updatedns-new updatedns-client

default: it

clean:
	rm -f ${WORKOBJS} ${UPDATEDNSOBJS} ${TARGETS}

checkpw.o: \
checkpw.c
	cc ${OPTS} -o checkpw.o -c checkpw.c

chomp.o: \
chomp.c
	cc ${OPTS} -o chomp.o -c chomp.c

command.o: \
command.c command.h splitargs.h
	cc ${OPTS} -o command.o -c command.c

getconfig.o: \
getconfig.c getconfig.h split.h
	cc ${OPTS} -o getconfig.o -c getconfig.c

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
${TARGETS}

md5.o: \
md5.c md5.h conf.h
	cc ${OPTS} -o md5.o -c md5.c

md5_digest.o: \
md5_digest.c md5.h
	cc ${OPTS} -o md5_digest.o -c md5_digest.c

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

splitargs.o: \
splitargs.c
	cc ${OPTS} -o splitargs.o -c splitargs.c

updatedns-new.o: \
updatedns.c split.h getpeerip.h gohome.h salt.h
	cc ${OPTS} -o updatedns-new.o -c updatedns-new.c

updatedns-new: \
${UPDATEDNSOBJS}
	cc ${OPTS} -o updatedns-new ${UPDATEDNSOBJS} -lcrypt

updatedns-client.o: \
updatedns-client.c split.h
	cc ${OPTS} -o updatedns-client.o -c updatedns-client.c

updatedns-client: \
${CLIENTOBJS}
	cc ${OPTS} -o updatedns-client ${CLIENTOBJS} -lcrypt

work.o: \
work.c hexdump.h
	cc ${OPTS} -o work.o -c work.c

work: \
${WORKOBJS}
	cc ${OPTS} -o work ${WORKOBJS}
