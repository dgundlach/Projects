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

void usage(char *program) {

}

int main(int argc, char **argv) {

	keyPairList *deleted_list;
	int i;
	char *buffer = NULL;
	size_t buf_len = 0;
	struct stat st;
	tstr *path;

	if (mdb_init(MDB_NO_BACKUP) == -1) {
		fprintf(stderr, "%s\n", mdb_errorstr);
		exit(1);
	}
	deleted_list = newKeyPairList(256);
	mdb_loaddir(deleted_list, mdb_removed, "");
	if (!deleted_list->list) exit(0);
	qsort(deleted_list->list, deleted_list->count, sizeof(keyPair *), &mdb_compare);
	path = tstrnew(NULL, 256);
	for (i=0; deleted_list->list[i]; i++) {
		tstrprintf(&path, "%s/%s", mdb_removed, deleted_list->list[i]->key);
		if (xreadlinkr(path->str, &buffer, &buf_len) != -1) {
			if (!stat(buffer, &st)) {
				printf("*\t%s\r\n", deleted_list->list[i]->key);
			} else {
				printf("\t%s\r\n", deleted_list->list[i]->key);
			}
		} else {
			printf("\t%s\r\n", deleted_list->list[i]->key);
		}			
	}
	printf("\n* in _Trash folder.\n");
}
