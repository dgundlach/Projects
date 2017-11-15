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
#include "currorprev.h"

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
	tstr *linkto = tstrnew(NULL, 256);
	char *p;
	int fd;
	size_t blocks;
	size_t disk_free;
	keyPair *drivepath;
	
	if (mdb_init(MDB_BACKUP) == -1) {
		fprintf(stderr, "%s\n", mdb_errorstr);
		exit(1);
	}
	if (!mdb_backup_list->list) {
		fprintf(stderr, "\nNo backup drive found.  If there is, create a "
				"_Movies and a _Trash folder\nand try again.\n\n");
		exit(-1);
	}
	for (i=0; mdb_backup_list->list[i]; i++) {
		if ((d = opendir(mdb_backup_list->list[i]->key))) {
			while ((entry = readdir(d))) {
				if (!CURRORPREV(entry->d_name) && !mdb_find(entry->d_name, 0)) {
					tstrprintf(&path, "%s/%s", mdb_backup_list->list[i]->key, entry->d_name);
					if ((drivepath = mdb_findspace(mdb_dir_list, getdu(path->str) / 2))) {
						tstrprintf(&topath, "%s/%s", drivepath->key, entry->d_name);
						printf("New title: %s\n", topath->str);
						mdb_xfermovie(path->str, drivepath->key);
						tstrprintf(&linkto, "%s/%s", mdb_movies, entry->d_name);
						unlink(linkto->str);
						symlink(topath->str, linkto->str);
						tstrprintf(&linkto, "%s/%s", mdb_backup, entry->d_name);
						unlink(linkto->str);
						symlink(topath->str, linkto->str);
						tstrprintf(&topath, "%s/%s", ((mdb_drive *)(mdb_backup_list->list[i]->data))->trash, entry->d_name);
						rename(path->str, topath->str);
					} else {
						fprintf(stderr, "Not enough space for: %s\n", entry->d_name);
					}
				}
			}
			closedir(d);
		}
	}
	for (i=0; mdb_dir_list->list[i]; i++) {
		if ((d = opendir(mdb_dir_list->list[i]->key))) {
			while ((entry = readdir(d))) {
				tstrprintf(&linkto, "%s/%s", mdb_backup, entry->d_name);
				if (!CURRORPREV(entry->d_name) && lstat(linkto->str, &st)) {
					tstrprintf(&path, "%s/%s", mdb_dir_list->list[i]->key, entry->d_name);
					if ((drivepath = mdb_findspace(mdb_backup_list, getdu(path->str) / 2))) {
						printf("Backing up: %s\n", path->str);
						tstrprintf(&topath, "%s/%s", drivepath->key, entry->d_name);
						mdb_xfermovie(path->str, drivepath->key);
						unlink(linkto->str);
						symlink(topath->str, linkto->str);
					} else {
						fprintf(stderr, "Not enough space for: %s\n", path->str);
					}
				}
			}
			closedir(d);
		}
	}
	mdb_savemasterlist();
}
