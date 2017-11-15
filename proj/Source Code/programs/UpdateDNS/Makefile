OPTS=-Wall -O3
TARGETS=UpdateDNS UpdateDNS_BuildDB
SERVEROBJS=UpdateDNS.o GetPeerIP.o GoHome.o md5.o MD5Digest.o SplitArgs.o Command.o
SERVERLIBS=-lgdbm

default: it

BitCount.o: \
BitCount.c
	cc ${OPTS} -o BitCount.o -c BitCount.c

UpdateDNS_BuildDB: \
UpdateDNS_BuildDB.c
	cc ${OPTS} -o UpdateDNS_BuildDB UpdateDNS_BuildDB.c ${SERVERLIBS}

clean:
	rm -f ${TARGETS} *.o

Command.o: \
Command.c Command.h SplitArgs.h
	cc ${OPTS} -o Command.o -c Command.c

GetPeerIP.o: \
GetPeerIP.c
	cc ${OPTS} -o GetPeerIP.o -c GetPeerIP.c

GetSubnet.o: \
GetSubnet.c GetSubnet.h
	cc ${OPTS} -o GetSubnet.o -c GetSubnet.c

GoHome.o: \
GoHome.c GoHome.h
	cc ${OPTS} -o GoHome.o -c GoHome.c

HexDump.o: \
HexDump.c
	cc ${OPTS} -o HexDump.o -c HexDump.c

valid_netmask.o: \
valid_netmask.c
	cc ${OPTS} -o valid_netmask.o -c valid_netmask.c

it: \
${TARGETS}

md5.o: \
md5.c md5.h conf.h
	cc ${OPTS} -o md5.o -c md5.c
      
MD5Digest.o: \
MD5Digest.c md5.h conf.h
	cc ${OPTS} -o MD5Digest.o -c MD5Digest.c

SplitArgs.o: \
SplitArgs.c
	cc ${OPTS} -o SplitArgs.o -c SplitArgs.c

UpdateDNS.o: \
UpdateDNS.c GetPeerIP.h GoHome.h MD5Digest.h Command.h SplitArgs.h
	cc ${OPTS} -o UpdateDNS.o -c UpdateDNS.c

UpdateDNS: \
${SERVEROBJS}
	cc -o UpdateDNS ${SERVEROBJS} ${SERVERLIBS}
