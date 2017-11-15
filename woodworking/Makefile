CC = gcc -Wall -O -g
TARGETS = drawers panel
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

drawers: drawers.o gcd.o measurement.o
	$(CC) -o $@ $^ -lm

panel: panel.o part.o gcd.o measurement.o
	$(CC) -o $@ $^ -lm

%.o: %.c
	$(CC) -o $@ -c $^
