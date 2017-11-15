#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include "currorprev.h"

size_t getdu(const char *path) {

	DIR *workingdir;
	struct dirent *entry;
	size_t blocks = 0;
	struct stat st;
	char newpath[strlen(path) + 258];
	char *p;

	if (stat(path, &st)) return 0;
	blocks = st.st_blocks;
	if (S_ISDIR(st.st_mode)) {
		if (!(workingdir = opendir(path))) return blocks;
		p = newpath + sprintf(newpath, "%s/", path);
		while ((entry = readdir(workingdir))) {
			if (!CURRORPREV(entry->d_name)) {
				strcpy(p, entry->d_name);
				blocks += getdu(newpath);
			}
		}
		closedir(workingdir);
	}
	return blocks;
}

int main(int argc, char **argv) {

	if (argv[1]) {
		printf("%d %s\n", getdu(argv[1]), argv[1]);
	}
}
