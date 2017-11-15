BINDIR=/usr/local/bin
#BITCOUNT=BitCount
BITCOUNT=FastBitCount
OPTS=-Wall -O3
TARGETS=SubnetCalc NetmaskCalc
OBJS=GetSubnet.o SubnetCalc.o $(BITCOUNT).o IntLog2.o

default: it

BitCount.o: \
BitCount.c
	cc ${OPTS} -o BitCount.o -c BitCount.c

clean:
	rm -f ${TARGETS} *.o

FastBitCount.o: \
FastBitCount.c
	cc ${OPTS} -o FastBitCount.o -c FastBitCount.c

GetSubnet.o: \
GetSubnet.c GetSubnet.h
	cc ${OPTS} -o GetSubnet.o -c GetSubnet.c

IntLog2.o: \
IntLog2.c
	cc ${OPTS} -o IntLog2.o -c IntLog2.c

it: \
${TARGETS}

install: \
${TARGETS}
	for NAME in ${TARGETS} ; do \
		install -m 755 $$NAME $(BINDIR); \
	done

NetmaskCalc: \
NetmaskCalc.o IntLog2.o
	cc ${OPTS} -o NetmaskCalc NetmaskCalc.o IntLog2.o
	strip NetmaskCalc

NetmaskCalc.o: \
NetmaskCalc.c
	cc ${OPTS} -o NetmaskCalc.o -c NetmaskCalc.c

SubnetCalc: \
${OBJS}
	cc ${OPTS} -o SubnetCalc ${OBJS}
	strip SubnetCalc

SubnetCalc.o: \
SubnetCalc.c
	cc ${OPTS} -o SubnetCalc.o -c SubnetCalc.c
