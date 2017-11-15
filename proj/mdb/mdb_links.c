#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <stdlib.h>
#include "mdb_functions.h"

int main(int argc, char **argv) {

	int i;
	char *path;
	char *dn;
	FILE *f = stdout;
	struct stat st;

	if (mdb_init(MDB_NO_BACKUP) == -1) {
		fprintf(stderr, "%s\n", mdb_errorstr);
		exit(1);
	}
	for (i=0; mdb_dir_list->list[i]; i++) {
		mdb_createlinks(mdb_dir_list->list[i]->key, mdb_movies);
	}
}
