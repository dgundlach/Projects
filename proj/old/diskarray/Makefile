all: array_mon

array_mon: array_mon.o daemonize.o scsi.o
	gcc scsi.o daemonize.o array_mon.o -o array_mon

array_mon.o: array_mon.c
	gcc -o array_mon.o -c array_mon.c

daemonize.o: daemonize.c
	gcc -o daemonize.o -c daemonize.c

scsi.o: scsi.c
	gcc -o scsi.o -c scsi.c

clean:
	rm -f *.o array_mon
