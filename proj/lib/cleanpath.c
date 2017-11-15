#include <string.h>
#include <libgen.h>
#include <stdio.h>
#include "xalloc.h"

char *cleanpath(char *path) {

	size_t len = strlen(path) + 1;
	char pathcopy[len];
	char *bn;
	char *dn;
	char *newpath;

	strcpy(pathcopy, path);
	bn = basename(pathcopy);
	dn = dirname(pathcopy);
	newpath = xmalloc(len);
	if (strcmp(dn, ".")) {
		sprintf(newpath, "%s/%s", dn, bn);
	} else {
		strcpy(newpath, bn);
	}
	return newpath;
}
