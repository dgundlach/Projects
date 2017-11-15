#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

//
//  Prune a file object if there are no other copies of it.  If a symbolic
//  link is passed, it will not be dereferenced.  Empty directories will
//  be removed if keep is not set.  This routine calls itself when non-
//  empty directories are encountered.
//

int prune(char *path, int keep) {

	DIR *workingdir;
	struct dirent *entry;
	int entrycount = 0;
	struct stat st;
	char newpath[strlen(path) + 258];
	char *p;

	if (lstat(path, &st)) {
		return -1;
	}
	if (S_ISDIR(st.st_mode)) {
		if (!(workingdir = opendir(path))) {
			return -1;
		}
		p = newpath + sprintf(newpath, "%s/", path);
		entrycount = keep ? 1 : 0;
		while (entry = readdir(workingdir)) {
			if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
				strcpy(p, entry->d_name);
				if (prune(newpath, keep)) {
					entrycount++;
				}
			}
		}
		closedir(workingdir);
		if (!entrycount) {
			if (rmdir(path)) {
				entrycount++;
			}
		}
	} else if (st.st_nlink == 1) {
		entrycount++;
	} else if (unlink(path)) {
		entrycount++;
	}
	return entrycount;
}

//
//  Prune a directory tree.  If a symbolic link is passed, the link(s) will 
//  also be removed if the entire tree is removed.
//

int prune_d(char *path) {

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
		if (!(rv = prune_d(wp))) {
			return unlink(path);
		}
		return rv;
	}
	return prune(path, 0);
}

//
//  Find the ultimate target of nested symbolic links.
//

char *deref(char *path) {

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
