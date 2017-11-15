CC = gcc -O -g
TARGETS = cygdrived
DESTDIR =
prefix = /usr/local
sbindir = $(PREFIX)/sbin
sysconfdir = /etc

all : $(TARGETS)

install: $(TARGETS)
	install -d -m 0755 $(DESTDIR)$(sysconfdir)
	install -m 0755 auto_label $(DESTDIR)$(sysconfdir)
	install -d -m 0755 $(DESTDIR)$(sbindir)
	install -m 0755 $(TARGETS) $(DESTDIR)$(sbindir)

clean:
	rm -f *.o $(TARGETS)

cygdrived: cygdrived.o daemonize.o
	$(CC) -o $@ $^

cygdrived.o: cygdrived.c
	$(CC) -o $@ -c $^

daemonize.o: daemonize.c
	$(CC) -o $@ -c $^
