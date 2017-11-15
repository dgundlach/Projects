#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <stdlib.h>
#include "xalloc.h"
#include "getdu.h"
#include "getavail.h"
#include "mdb_functions.h"
#include "tstr.h"
#include "pathent.h"

char *paths[] = {"/backup", "/cygdrive", "/ext", NULL};

int main(int argc, char **argv) {

	int i;
	char *dn;
	FILE *f = stdout;
	struct stat st;
	char *backup_drive = NULL;
	char *backup_dir = NULL;
	char *backup_trash = NULL;
	DIR *d;
	struct dirent *entry;
	tstr *path = tstrnew(NULL, 256);
	tstr *topath = tstrnew(NULL, 256);
	char *p;
	int fd;
	size_t blocks;
	size_t disk_free;
	keyPair *drivepath;
	
	if (mdb_init(MDB_BACKUP) == -1) {
		fprintf(stderr, "%s\n", mdb_errorstr);
		exit(1);
	}
	for (i=0; paths[i]; i++) {
		if ((d = opendir(paths[i]))) {
			while ((entry = readdir(d))) {
				if (PATHENT(entry->d_name)) continue;
				tstrprintf(&path, "%s/%s", paths[i], entry->d_name);
				tstrsettrunc(path, path->length, 0);
				tstrcatn(&path, "/_Movies/mdb.XXXXXX", 19);
				if ((fd = mkstemp(path->str)) != -1) {
					close(fd);
					unlink(path->str);
					tstrcatn(&path, "/_Trash/mdb.XXXXXX", 18);
					if ((fd = mkstemp(path->str)) != -1) {
						close(fd);
						unlink(path->str);
						tstrtrunc(path);
						backup_drive = strdup(path->str);
						tstrcatn(&path, "/_Movies", 8);
						backup_dir = strdup(path->str);
						tstrcatn(&path, "/_Trash", 7);
						backup_trash = strdup(path->str);
						break;
					}
				}
			}
			closedir(d);
			if (backup_drive) break;
		}
	}
	if (!backup_drive) {
		fprintf(stderr, "\nNo backup drive found.  If there is, create a "
				"_Movies and a _Trash folder\nand try again.\n\n");
		exit(-1);
	}
	tstrunsettrunc(path);
	chdir(backup_dir);
	d = opendir(".");
	while ((entry = readdir(d))) {
		if (!PATHENT(entry->d_name)) {
			stat(entry->d_name, &st);
			if (S_ISDIR(st.st_mode) && !mdb_find(entry->d_name, 0)) {
				tstrprintf(&path, "%s/%s", mdb_movies, entry->d_name);
				if (stat(path->str, &st) || !S_ISDIR(st.st_mode)) {
					if ((drivepath = mdb_findspace(mdb_dir_list, getdu(entry->d_name) / 2))) {
						printf("New title: %s/%s\n", drivepath->key, entry->d_name);
						mdb_xfermovie(entry->d_name, drivepath->key);
						tstrprintf(&topath, "%s/%s", drivepath->key, entry->d_name);
						unlink(path->str);
						symlink(topath->str, path->str);
						tstrprintf(&path, "%s/%s", mdb_backup, entry->d_name);
						unlink(path->str);
						symlink(topath->str, path->str);
						tstrprintf(&path, "%s/%s", backup_trash, entry->d_name);
						rename(entry->d_name, path->str);
					} else {
						fprintf(stderr, "Not enough space for: %s\n", entry->d_name);
					}
					
				}
			}
		}
	}
	closedir(d);
	for (i=0; mdb_dir_list->list[i]; i++) {
		d = opendir(mdb_dir_list->list[i]->key);
		while ((entry = readdir(d))) {
			if (!PATHENT(entry->d_name)) {
				tstrprintf(&topath, "%s/%s", mdb_backup, entry->d_name);
				if (lstat(topath->str, &st)) {
					tstrprintf(&path, "%s/%s", mdb_dir_list->list[i]->key, entry->d_name);
					blocks = getdu(path->str);
					disk_free = getavail(".");
					if (disk_free > blocks) {
						printf("Backing up: %s\n", path->str);
						mdb_xfermovie(path->str, ".");
						unlink(topath->str);
						symlink(path->str, topath->str);
					} else {
						fprintf(stderr, "Not enough space for: %s\n", path->str);
					}
				}
			}
		}
		closedir(d);
	}
	mdb_savemasterlist();
}
