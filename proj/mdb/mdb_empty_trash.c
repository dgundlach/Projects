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
#include "rm.h"

void usage(char *program) {

}

int main(int argc, char **argv) {

	int i;
	DIR *d;
	struct dirent *entry;
	tstr *path = tstrnew(NULL, 256);

	if (mdb_init(MDB_NO_BACKUP) == -1) {
		fprintf(stderr, "%s\n", mdb_errorstr);
		exit(1);
	}
	for (i=0; mdb_dir_list->list[i]; i++) {
		if ((d = opendir(((mdb_drive *)(mdb_dir_list->list[i]->data))->trash))) {
			if (!CURRORPREV(entry->d_name)) {
				tstrprintf(&path, "%s/%s", ((mdb_drive *)(mdb_dir_list->list[i]->data))->trash, entry->d_name);
				rm(path->str, RM_RECURSE, RM_NO_FOLLOW);
				tstrprintf(&path, "%s/%s", mdb_backup, entry->d_name);
				unlink(path->str);
			}
		}
	}
}
