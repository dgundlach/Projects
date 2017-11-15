#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include "mdb_functions.h"
#include "tstr.h"
#include "currorprev.h"

int main(int argc, char **argv) {

	DIR *d;
	struct dirent *entry;
	struct stat st;
	tstr *path;
	FILE *f = stdout;

	if (mdb_init(MDB_NO_BACKUP) == -1) {
		fprintf(stderr, "%s\n", mdb_errorstr);
		exit(1);
	}
	path = tstrnew(NULL, 256);
	if ((d = opendir(mdb_movies))) {
		while ((entry = readdir(d))) {
			if (!CURRORPREV(entry->d_name)) {
				tstrprintf(&path, "%s/%s", mdb_movies, entry->d_name);
				if (stat(path->str, &st) && (errno == ENOENT)) {
					fprintf(f, "%s\n", entry->d_name);
				}
			}
		}
		closedir(d);
	}
}
