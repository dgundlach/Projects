
all: libnss_pgsql.so.2

log.so : log.c
	gcc -c -O3 -march=i586 -Wall -Wwrite-strings -I. -fPIC log.c -o log.so

passwd.so : passwd.c setup.h
	gcc -c -O3 -march=i586 -Wall -Wwrite-strings -I. -fPIC passwd.c -o passwd.so

shadow.so : shadow.c setup.h
	gcc -c -O3 -march=i586 -Wall -Wwrite-strings -I. -fPIC shadow.c -o shadow.so

group.so : group.c setup.h
	gcc -c -O3 -march=i586 -Wall -Wwrite-strings -I. -fPIC group.c -o group.so

libnss_pgsql.so.2 : log.so passwd.so shadow.so group.so
	gcc -shared -O3 -march=i586 -Wl,-soname,libnss_pgsql.so.2 -lpq log.so \
	passwd.so shadow.so group.so -o libnss_pgsql.so.2 
   
   
install : libnss_pgsql.so.2 
	cp -f libnss_pgsql.so.2 /lib/libnss_pgsql.so.2;\
	install -C -m 644 nss-pgsql.conf /etc/nss-pgsql.conf
	install -C -m 600 nss-pgsql-root.conf /etc/nss-pgsql-root.conf

clean: 
	rm -f libnss_pgsql.so.2 *.so *.o
