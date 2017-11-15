#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

char *salt(void) {

	static char salty[2];
	char base64[] = 
		"./0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKIMNOPQRSTUVWXYZ";

	salty[0] = base64[(int) (64.0 * rand() / (RAND_MAX + 1.0))];
	salty[1] = base64[(int) (64.0 * rand() / (RAND_MAX + 1.0))];
	return salty;
}


int main (int argc, char **argv) {

	if (argc != 2) exit(1);
	srand(time(NULL));
	printf ("%s\n", crypt(argv[1], salt()));   
}
