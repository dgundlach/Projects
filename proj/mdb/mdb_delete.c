#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <stdlib.h>
#include "mdb_functions.h"
#include "tstr.h"
#include "xalloc.h"

void usage(char *program) {

	fprintf(stderr, "Usage: %s [-s] [-h] <movie> [movie ...]\n"
					"\t-s : Scripted, don't update master list\n"
					"\t-h : Display this text\n", basename(program));
}

int main(int argc, char **argv) {

	int i;
	FILE *f = stdout;
	struct stat st;
	keyPair *findpath; 
	char *t, *dn;
	tstr *path = tstrnew(NULL, 256);
	tstr *topath = tstrnew(NULL, 256);
	mdb_drive *drive;
	int scripted = 0;
	int opt;

	if (mdb_init(MDB_NO_BACKUP) == -1) {
		fprintf(stderr, "%s\n", mdb_errorstr);
		exit(-1);
	}
	while ((opt = getopt(argc, argv, "sh")) != -1) {
		switch (opt) {
		case 's':
			scripted = 1;
			break;
		case 'h':
			usage(argv[0]);
			exit(0);
		default:
			usage(argv[0]);
			exit(-1);
		}
	}
	if (optind >= argc) {
		usage(argv[0]);
		exit(-1);
	}
	for (i=optind; i<argc; i++) {
		if ((findpath = mdb_find(argv[i], 0))) {
			drive = findpath->data;
			tstrprintf(&path, "%s/%s", findpath->key, argv[i]);
			tstrprintf(&topath, "%s/%s", drive->trash, argv[i]);
			if (stat(topath->str, &st)) {
				rename(path->str, topath->str);
				tstrprintf(&path, "%s/%s", mdb_removed, argv[i]);
				symlink(topath->str, path->str);
				tstrprintf(&path, "%s/%s", mdb_movies, argv[i]);
				unlink(path->str);

//	Don't remove backup link just in case the movie will be restored later.

//				tstrprintf(&path, "%s/%s", mdb_backup, argv[i]);
//				unlink(path->str);
			} else {
				fprintf(stderr, "%s already exists.\n", topath->str);
			}
		} else {
			fprintf(stderr, "%s not found.  Attempting to remove links.", argv[i]);
			tstrprintf(&path, "%s/%s", mdb_movies, argv[i]);
			if (!stat(path->str, &st)) {
				unlink(path->str);
				tstrprintf(&topath, "%s/%s", mdb_removed, argv[i]);
				tstrprintf(&path, "x0x0x0x");
				symlink(path->str, topath->str);

//	Don't remove backup link just in case the movie will be restored later.

//				tstrprintf(&path, "%s/%s", mdb_backup, argv[i]);
//				unlink(path->str);
			} else {
				fprintf(stderr, "No links found.  Doing nothing.", path->str);
			}
		}
	}
	if (!scripted) {
		mdb_savemasterlist();
	}
}
