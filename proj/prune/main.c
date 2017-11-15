#define _BSD_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <getopt.h>
#include "prune.h"

void Usage(char *program) {

	fprintf(stderr, "\nUsage %s [-d] [-k] <path> [<path> [...]]\n", 
					basename(program));
	exit(1);
}

int main(int argc, char **argv) {

	int keep = 0;
	int c;
	char *dir;

	while ((c = getopt(argc, argv, "k")) != -1) {
		switch(c) {
			case 'k':
				keep = 1;
				break;
			default:
				Usage(argv[0]);
		}
		if (optind = argc) {
			Usage(argv[0]);
		}
	}
	for (c=optind; c<argc; c++) {
		if (dir = deref(argv[c])) {
			prune(dir, keep);
			free(dir);
		}
	}
}
