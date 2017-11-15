#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <stdlib.h>
#include "mdb_functions.h"

void usage(char *program) {

	fprintf(stderr, "Usage: %s [-i] [-h] [-?] [file]\n"
					"\t  -i : add an index\n"
					"\t  -h : display this text\n"
					"\t  -? : display this text\n"
					"\tfile : file to write list to\n", basename(program));
}

int main(int argc, char **argv) {

	int i;
	char *path;
	char *dn;
	FILE *f = stdout;
	struct stat st;
	int opt;
	int index = 0;

	if (mdb_init(MDB_NO_BACKUP) == -1) {
		fprintf(stderr, "%s\n", mdb_errorstr);
		exit(1);
	}
	while ((opt = getopt(argc, argv, "ih?")) != -1) {
		switch(opt) {
		case 'i':
			index = 1;
			break;
		case 'h':
		case '?':
			usage(argv[0]);
			exit(0);
		default:
			usage(argv[0]);
			exit(1);
		}
	}
	if (optind < argc) {
		path = strdup(argv[optind]);
		dn = dirname(argv[optind]);
		if (!stat(dn, &st) && S_ISDIR(st.st_mode)) {
			if (!stat(path, &st) & S_ISDIR(st.st_mode)) {
				fprintf(stderr, "%s: is a directory\n", path);
				exit(1);
			}
			if (!(f = fopen(path, "w"))) {
				fprintf(stderr, "cannot open %s for writing\n", path);
				exit(1);
			}
		} else {
			fprintf(stderr, "%s: not a directory\n", dn);
			exit(1);
		}
	}
	if ((mdb_movie_list = mdb_loadlist(NULL))) {
		for (i=0; mdb_movie_list->list[i]; i++) {
			if (index) {
				fprintf(f, "%s\t", mdb_movie_list->list[i]->data);
			}
			fprintf(f, "%s\n",mdb_movie_list->list[i]->key);
		}
	}
	fclose(f);
}
