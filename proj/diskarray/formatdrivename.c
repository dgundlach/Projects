#include <stdio.h>
#include <string.h>

/*
 * I wrote this just to prove that I could.  It doesn't really do much good
 * to go past 256 SCSI drives since the kernel doesn't support that many.
 */

int formatdrivename(char *d_name, int drive) {

	int powerof26 = 26;
	int startposition = 0;
	int sumofpowers = 0;
	int iterator = 1;
	int remain;
	int num;
	char *p = d_name;

/*
 * This code figures out where to start iterating so that the result we want is
 * returned.  The start postition is: 26^n - (26^(n-1) .... 26^2 + 26) where 26^n
 * is greater than or equal to our value.  It also figures out where to start 
 * placing our "digits", since the string returned would be backwards if built in
 * order.  
 */

	while (drive >= sumofpowers + powerof26) {
		sumofpowers += powerof26;
		powerof26 = powerof26 * 26;
		startposition = powerof26 - sumofpowers;
		iterator++;
	}
	*p++ = 's';
	*p++ = 'd';
	*(p + iterator--) = '\0';				// Put the end of line on first.
	num = drive + startposition;
	do {
		remain = num % 26;
		num = num / 26;
		*(p + iterator--) = remain + 'a'; 	// Put the characters on backwards.
	} while (iterator > -1);
	return 1;
}

int main(int argc, char **argv) {

   int drive = 0;

	int j;
	char data[256];
	char *p;
	int k = 1;
	
	while (1) {
		formatdrivename(data, drive);
		printf ("%d %s\n", drive++, data);
	}
}
