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
#include "currorprev.h"

void usage(char *program) {

	fprintf(stderr, "Usage: %s [-s] [-h] <old name> <new name>\n"
					"\t-s : Scripted, don't update master list\n"
					"\t-h : Display this text\n", basename(program));}

int main(int argc, char **argv) {

	char *oldname;
	char *newname;
	struct stat st;
	keyPair *drive;
	tstr *oldpath, *newpath;
	DIR *d;
	struct dirent *entry;
	size_t len;
	int scripted = 0;
	int opt;

	if (mdb_init(MDB_NO_BACKUP) == -1) {
		fprintf(stderr, "%s\n", mdb_errorstr);
		exit(-1);
	}
	while ((opt = getopt(argc, argv, "sh")) != -1) {
		switch (opt) {
		case 's':
			scripted = 1;
			break;
		case 'h':
			usage(argv[0]);
			exit(0);
		default:
			usage(argv[0]);
			exit(-1);
		}
	}
	if ((argc - optind) != 2) {
		usage(argv[0]);
		exit(-1);
	}
	oldname = argv[optind++];
	newname = argv[optind];
	if ((drive = mdb_find(newname, 0))) {
		fprintf(stderr, "%s: already exists\n", newname);
		exit(-1);
	}
	if (!(drive = mdb_find(oldname, 0))) {
		fprintf(stderr, "%s: not found\n", oldname);
		exit(-1);
	}
	oldpath = tstrnew(NULL, 256);
	newpath = tstrnew(NULL, 256);
	tstrprintf(&oldpath, "%s/%s", drive->key, oldname);
	if (!(d = opendir(oldpath->str))) {
		fprintf(stderr, "%s: not a directory\n", oldpath->str);
		exit(-1);
	}
	len = strlen(oldname);
	while ((entry = readdir(d))) {
		if (!strncmp(entry->d_name, oldname, len)) {
			tstrprintf(&oldpath, "%s/%s/%s", drive->key, oldname, entry->d_name);
			tstrprintf(&newpath, "%s/%s/%s%s", drive->key, oldname, newname, entry->d_name + len);
			if (stat(newpath->str, &st)) {
				rename(oldpath->str, newpath->str);
			} else {
				fprintf(stderr, "%s: already exists.  Not renamimg.\n", newpath->str);
			}
		}
	}
	closedir(d);
	tstrprintf(&oldpath, "%s/%s", drive->key, oldname);
	tstrprintf(&newpath, "%s/%s", drive->key, newname);
	rename(oldpath->str, newpath->str);
	tstrprintf(&oldpath, "%s/%s", mdb_movies, oldname);
	if (!lstat(oldpath->str, &st)) {
		unlink(oldpath->str);
	}
	tstrprintf(&oldpath, "%s/%s", mdb_movies, newname);
	symlink(newpath->str, oldpath->str);
	tstrprintf(&oldpath, "%s/%s", mdb_backup, oldname);
	if (!lstat(oldpath->str, &st)) {
		unlink(oldpath->str);
		tstrprintf(&oldpath, "%s/%s", mdb_backup, newname);
		symlink(newpath->str, oldpath->str);
	}
	if (!scripted) {
		mdb_savemasterlist();
	}
}
