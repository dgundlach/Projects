#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <stdlib.h>
#include <dirent.h>
#include "mdb_functions.h"
#include "tstr.h"

int main(int argc, char **argv) {

	int i;
	struct stat st;
	DIR *d;
	struct dirent *entry;
	tstr *path;
	tstr *linkpath;

	if (mdb_init(MDB_NO_BACKUP) == -1) {
		fprintf(stderr, "%s\n",mdb_errorstr);
		exit(1);
	}
	path = tstrnew(NULL, 256);
	linkpath = tstrnew(NULL, 256);
	for (i=0; mdb_dir_list->list[i]; i++) {
		if ((d = opendir(mdb_dir_list->list[i]->key))) {
			while ((entry = readdir(d))) {
				if (*(entry->d_name) != '.') {
					tstrprintf(&path, "%s/%s", mdb_dir_list->list[i]->key,
							entry->d_name);
					if (!stat(path->str, &st) && S_ISDIR(st.st_mode)) {
						tstrprintf(&linkpath, "%s/%s", mdb_movies, entry->d_name);
						symlink(path->str, linkpath->str);
						tstrprintf(&linkpath, "%s/%s", mdb_backup, entry->d_name);
						symlink(path->str, linkpath->str);
					}
				}
			}
			closedir(d);
		}
	}
}
