INCLUDES=-I lib
LIBS=
COMPILE=gcc ${INCLUDES}
LINK=gcc ${LIBS}
PRUNE_OBJS = lib/xalloc.o prune.o main.o

all: prune

prune: ${PRUNE_OBJS}
	${LINK} ${PRUNE_OBJS} -o prune

%.o: %.c
	$(COMPILE) -o $@ -c $<

clean:
	rm -f *.o prune
