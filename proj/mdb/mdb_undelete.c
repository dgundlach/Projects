#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/utsname.h>
#include "mdb_functions.h"
#include "tstr.h"
#include "xalloc.h"
#include "currorprev.h"

void usage(char *program) {

	fprintf(stderr, "Usage: %s [-s] [-h] <movie|all> [movie ...]\n"
					"\t-s : Scripted, don't update master list\n"
					"\t-h : Display this text\n", basename(program));
}

int main(int argc, char **argv) {

	int i, j;
	DIR *d;
	struct dirent *entry;
	keyPair *drive;
	tstr *path = tstrnew(NULL, 256);
	tstr *topath = tstrnew(NULL, 256);
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
	if (!strcasecmp(argv[optind], "all")) {
		for (i=0; mdb_dir_list->list[i]; i++) {
			if (d = opendir(((mdb_drive *)(mdb_dir_list->list[i]->data))->trash)) {
				while ((entry = readdir(d))) {
					if (!CURRORPREV(entry->d_name)) {
						tstrprintf(&path, "%s/%s", ((mdb_drive *)(mdb_dir_list->list[i]->data))->trash, entry->d_name);
						tstrprintf(&topath, "%s/%s", mdb_dir_list->list[i]->key, entry->d_name);
						rename(path->str, topath->str);
						tstrprintf(&path, "%s/%s", mdb_movies, entry->d_name);
						symlink(topath->str, path->str);
						tstrprintf(&path, "%s/%s", mdb_removed, entry->d_name);
						unlink(path->str);
					}
				}
				closedir(d);
			}
		}
	} else {
		for (i=optind; argv[i]; i++) {
			if ((drive = mdb_find(argv[i], 1))) {
				tstrprintf(&path, "%s/%s", ((mdb_drive *)(drive->data))->trash, argv[i]);
				tstrprintf(&topath, "%s/%s", drive->key, argv[i]);
				rename(path->str, topath->str);
				tstrprintf(&path, "%s/%s", mdb_movies, argv[i]);
				symlink(topath->str, path->str);
				tstrprintf(&path, "%s/%s", mdb_removed, argv[i]);
				unlink(path->str);
			} else {
				fprintf(stderr, "%s: not found in _Trash\n");
			}
		}
	}
	if (!scripted) {
		mdb_savemasterlist();
	}
}
