BINDIR=/usr/local/bin
OPTS=-Wall -O3
TARGETS=stra.a
OBJS=prealloc.o rsprintf.o rstrcpy.o stra.o

default: it

clean:
	rm -f ${TARGETS} *.o *.h

it: \
${TARGETS}

prealloc.h: \
prealloc.c
	cproto prealloc.c > prealloc.h

prealloc.o: \
prealloc.c
	cc ${OPTS} -o prealloc.o -c prealloc.c

rsprintf.h: \
rsprintf.c prealloc.h
	cproto rsprintf.c > rsprintf.h

rsprintf.o: \
rsprintf.c prealloc.h
	cc ${OPTS} -o rsprintf.o -c rsprintf.c

rstrcpy.h: \
rstrcpy.c prealloc.h
	cproto rstrcpy.c > rstrcpy.h

rstrcpy.o: \
rstrcpy.c prealloc.h
	cc ${OPTS} -o rstrcpy.o -c rstrcpy.c

stra.h: \
stra.c rstrcpy.h rsprintf.h stra.hh
	cat stra.hh > stra.h
	cproto stra.c >> stra.h

stra.o: \
stra.c stra.h rstrcpy.h rsprintf.h
	cc ${OPTS} -o stra.o -c stra.c

stra.a: \
${OBJS}
	ar r stra.a ${OBJS}
	ranlib stra.a
