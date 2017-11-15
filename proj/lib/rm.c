#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <stdlib.h>
#include <dirent.h>
#include "xalloc.h"
#include "currorprev.h"
#include "rm.h"

int rm(char *path, int recurse, int follow) {

	DIR *d;
	struct dirent *entry;
	struct stat st;
	char newpath[strlen(path) + 256];
	char *p;
	char *buffer = NULL;

	if (lstat(path, &st)) {
		return -1;
	}
	if (S_ISDIR(st.st_mode)) {
		if (recurse) {
			if (!(d = opendir(path))) {
				return -1;
			}
			p = newpath + sprintf(newpath, "%s/", path);
			while (entry = readdir(d)) {
				if (!CURRORPREV(entry->d_name)) {
					strcpy(p, entry->d_name);
					if (rm(newpath, recurse, follow) == -1) {
						closedir(d);
						return -1;
					}
				}
			}
			closedir(d);
		}
		return rmdir(path);
	} else if (recurse && follow && S_ISLNK(st.st_mode)) {
		if (buffer = xreadlink(path, NULL, NULL)) {
			if (rm(buffer, recurse, follow) == -1) {
				free(buffer);
				return -1;
			}
			free(buffer);
		}
	}
	return unlink(path);
}
