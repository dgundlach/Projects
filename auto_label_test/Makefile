CC = gcc -Wall -O -g
TARGETS = remountd auto_label
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

remountd: remountd.o daemonize.o remount.o configure.o
	$(CC) -o $@ $^ -lblkid

auto_label: auto_label.o remount.o configure.o
	$(CC) -o $@ $^ -lblkid

%.o: %.c
	$(CC) -o $@ -c $^
