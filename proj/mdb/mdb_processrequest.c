#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <stdlib.h>
#include "mdb_functions.h"
#include "tstr.h"

int main(int argc, char **argv) {

	struct stat st;
	tstr *path;
	keyPairList *request;
	int i;

	if (mdb_init(MDB_NO_BACKUP) == -1) {
		fprintf(stderr, "%s\n", mdb_errorstr);
		exit(1);
	}
	request = mdb_loadlist("-");
	path = tstrnew(NULL, 256);
	for (i=0; request->list[i]; i++) {
		tstrprintf(&path, "%s/%s", mdb_backup, request->list[i]->key);
		if (!stat(path->str, &st)) {
			unlink(path->str);
		}
	}
}
