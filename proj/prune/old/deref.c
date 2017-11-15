#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "xalloc.h"

int deref_and_prune(const char *path) {

	struct stat st;
	int rv;

	if (lstat(path, &st)) {
		return -1;
	}
	if (S_ISLNK(st.st_mode)) {
		char wp[st.st_size + 1];
		if (readlink(path, wp, st.st_size) == -1) {
			return -1;
		}
		wp[st.st_size] = '\0';
		if (!(rv = deref_and_prune(wp))) {
			return unlink(path);
		}
		return rv;
	}
	return prune(path, 0);
}

char *deref(const char *path) {

	struct stat st;
	
	if (lstat(path, &st)) {
		return NULL;
	}
	if (S_ISLNK(st.st_mode)) {
		char wp[st.st_size + 1];
		if (readlink(path, wp, st.st_size) == -1) {
			return NULL;
		}
		wp[st.st_size] = '\0';
		return deref(wp);
	}
	return xstrdup((char *)path);
}
