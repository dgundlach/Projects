#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/statfs.h>
#include <dirent.h>
#include <errno.h>
#include "xalloc.h"
#include "stripspaces.h"
#include "keypair.h"
#include "tstr.h"


char *mdb_data = NULL;
char *mdb_meta = NULL;
char *mdb_backup = NULL;
char *mdb_removed = NULL;
char *mdb_missing = NULL;
char *mdb_movies = NULL;
char *mdb_movielist = NULL;
char *mdb_conf = NULL;
keyPairList *mdb_dir_list = NULL;
keyPairList *mdb_movie_list = NULL;
int mdb_have_pv = 0;

int mdb_init(void) {

	char buffer[4096];
	char *home;
	size_t len;
	FILE *f;
	DIR *d;
	int drivecount = 0;
	int maxdrives = 0;
	char *p, *e, *s, *n, *a, c;
	char *newdir = NULL;
	struct stat st;

	home = getenv("HOME");
	len = strlen(home);
	mdb_data = xmalloc((len + 16) * 16);
	mdb_meta = mdb_data + 1
			+ sprintf(mdb_data, "%s/.mdb", home);
	mdb_backup = mdb_meta + 1
			+ sprintf(mdb_meta, "%s/meta", mdb_data);
	mdb_removed = mdb_backup + 1
			+ sprintf(mdb_backup, "%s/backup", mdb_data);
	mdb_missing = mdb_removed + 1
			+ sprintf(mdb_removed, "%s/removed", mdb_data);
	mdb_movielist = mdb_missing + 1
			+ sprintf(mdb_missing, "%s/missing", mdb_data);
	mdb_conf = mdb_movielist + 1
			+ sprintf(mdb_movielist, "%s/Movie_db", mdb_data);
	mdb_movies = mdb_conf + 1
			+ sprintf(mdb_conf, "%s/paths", mdb_data);
	sprintf(mdb_movies, "%s/Movies", home);
	mdb_dir_list = loadKeyPairList(mdb_conf, 16, 0, &check_dir);
	if ((f = popen("pv -h", "r"))) {
		pclose(f);
		mdb_have_pv = 1;
	}
	return 1;
}

char *mdb_findspace(keyPairList *dirlist, size_t size) {

	struct statfs stfs;
	long blocks;
	int i;

	if (!dirlist) dirlist = mdb_dir_list;
	for (i=0; dirlist->list[i]; i++) {
		if (!statfs(dirlist->list[i]->key, &stfs)) {
			if (stfs.f_bsize >= 1024) {
				blocks = size / (stfs.f_bsize / 1024);
			} else {
				blocks = size * (1024 / stfs.f_bsize);
			}
			if (stfs.f_bavail > blocks) {
				return dirlist->list[i]->key;
			}
		}
	}
	return NULL;
}

#define CALC_SIZE "mkisofs -udf -dvd-video -print-size \"%s\" 2>/dev/null"

int mdb_calcsize(char *volume, char *movie, long *size, long *totalsize) {

	struct stat st;
	char buffer[32];
	char *e;
	FILE *f;
	tstr *path;

	if (stat(volume, &st)) return -1;
	*size = 0;
	if (S_ISREG(st.st_mode)) {
		*size = st.st_size / 1024;
	} else {
		sprintf(buffer, CALC_SIZE, volume);
		if (popen(buffer, "r")) {
			if (fgets(buffer, 4096, f)) {
				*size = strtoul(buffer, &e, 10) * 2;
			}
			pclose(f);
		}
	}
	if (!size) return -1;
	path = tstrnew(NULL, 256);
	tstrprintf(&path, "%s/%s", mdb_meta, movie);
	tstrsettrunc(path, path->length, 0);
	*totalsize = 0;
	tstrcatn(&path, "-fanart.jpg", 11);
	if (!stat(path->str, &st)) {
		*totalsize += st.st_blocks / 2;
	}
	tstrcatn(&path, ".nfo", 4);
	if (!stat(path->str, &st)) {
		*totalsize += st.st_blocks / 2;
	}
	tstrcatn(&path, ".tbn", 4);
	if (!stat(path->str, &st)) {
		*totalsize += st.st_blocks / 2;
	}
	if (!*totalsize) {
		*totalsize = 4096;
	}
	*totalsize += *size;
	tstrdestroy(&path);
	return 0;
}

char *mdb_find(char *movie) {

	int i;
	tstr *path = tstrnew(NULL, 256);
	char *rv = NULL;
	struct stat st;

	for (i=0; mdb_dir_list->list[i]; i++) {
		tstrprintf(&path, "%s/%s", mdb_dir_list->list[i]->key, movie);
		if (!stat(path->str, &st)) {
			rv = mdb_dir_list->list[i]->key;
			break;
		}
	}
	tstrdestroy(&path);
	return rv;
}

void mdb_loaddir(keyPairList *dirlist, char *path, char *alias) {

	DIR *d;
	struct dirent *entry;
	keyPair *new_pair;
	struct stat st;
	tstr *fullpath = NULL;

	if (!(d = opendir(path))) return;
	fullpath = tstrnew(NULL, 256);
	if (!alias) alias = path;
	while ((entry = readdir(d))) {
		tstrprintf(&fullpath, "%s/%s", path, entry->d_name);
		if ((*(entry->d_name) != '.') && !stat(fullpath->str, &st)
						&& S_ISDIR(st.st_mode)) {
			if (dirlist->count > (dirlist->max - 2)) {
				dirlist->max += dirlist->alloc;
				dirlist->list = xrealloc(dirlist->list,
						dirlist->max * sizeof(keyPair *));
			}
			new_pair = xmalloc(sizeof(struct keyPair)
					+ strlen(entry->d_name) + 1);
			new_pair->data = alias;
			new_pair->key = (void *)new_pair + sizeof(struct keyPair);
			strcpy(new_pair->key, entry->d_name);
			dirlist->list[dirlist->count++] = new_pair;
		}
	}
	if (dirlist->list) {
		dirlist->list[dirlist->count] = NULL;
	}
	closedir(d);
	tstrdestroy(&fullpath);
}

int mdb_createlinks(char *dir, char *linkdir) {

	DIR *d;
	struct dirent *entry;
	struct stat st;
	tstr *dirpath;
	tstr *linkpath;

	if (!(d = opendir(dir))) return 0;
	dirpath = tstrnew(NULL, 256);
	linkpath = tstrnew(NULL, 256);
	while ((entry = readdir(d))) {
		if (*(entry->d_name) != '.') {
			tstrprintf(&dirpath, "%s/%s", dir, entry->d_name);
			tstrprintf(&linkpath, "%s/%s", linkdir, entry->d_name);
			if (stat(linkpath->str, &st) && (errno == ENOENT)) {
				if (!lstat(linkpath->str, &st)) {
					unlink(linkpath->str);
				}
				symlink(dirpath->str, linkpath->str);
			}
		}
	}
	closedir(d);
	tstrdestroy(&dirpath);
	tstrdestroy(&linkpath);
	return 1;
}

int mdb_compare(const void *first, const void *second) {

	char *one, *two;

	one = (*(keyPair **)first)->key;
	two = (*(keyPair **)second)->key;
	if (!strncasecmp(one, "the ", 4)) one += 4;
	if (!strncasecmp(two, "the ", 4)) two += 4;
	return strcasecmp(one, two);
}

keyPairList *mdb_loadlist(char *list) {

	struct stat st;
	keyPairList *new;
	int i;

	if (list) {
		new = loadKeyPairList(list, 1024, 1, NULL);
	} else {
		new = newKeyPairList(1024);
		for (i=0; mdb_dir_list->list[i]; i++) {
			mdb_loaddir(new, mdb_dir_list->list[i]->key, mdb_dir_list->list[i]->data);
		}
	}
	if (new) {
		qsort(new->list, new->count, sizeof(keyPair *), &mdb_compare);
	}
	return new;
}

int mdb_xfermovie(char *frompath, char *topath) {

	tstr *fp = NULL;
	tstr *tp = NULL;
	tstr *comm = NULL;
	char buffer[4096];
	char *movie;
	DIR *d;
	FILE *inf, *outf;
	size_t size;
	struct dirent *entry;
	struct stat st;

	if (!frompath || !topath) return 0;
	char tfp[strlen(frompath) + 1];
	strcpy(tfp, frompath);
	movie = basename(tfp);
	if (!(d = opendir(frompath))) return 0;
	tp = tstrnew(NULL, 256);	
	tstrprintf(&tp, "%s/%s", topath, movie);
	if (mkdir(tp->str, 0755)) {
		closedir(d);
		tstrdestroy(&tp);
		return 0;
	}
	fp = tstrnew(NULL, 256);
	comm = tstrnew(NULL, 256);
	while ((entry = readdir(d))) {
		tstrprintf(&fp, "%s/%s", frompath, entry->d_name);
		if ((*(entry->d_name) != '.') && !stat(fp->str, &st)
				&& S_ISREG(st.st_mode)) {
			tstrprintf(&tp, "%s/%s/%s", topath, movie, entry->d_name);
			if ((inf = fopen(fp->str, "r"))) {
				if (mdb_have_pv && (st.st_blocks > 2000000)) {
					tstrprintf(&comm, "pv -s %dk -p -e -r > \"%s\"",
								st.st_size / 1024, tp->str);
					if ((outf = popen(comm->str, "w"))) {
						while ((size = fread(buffer, 1, 4096, inf))) {
							fwrite(buffer, 1, size, outf);
						}
						pclose(outf);
					}
				} else {
					if ((outf = fopen(tp->str, "w"))) {
						while ((size = fread(buffer, 1, 4096, inf))) {
							fwrite(buffer, 1, size, outf);
						}
						fclose(outf);
					}
				}
			}
		}
	}
	closedir(d);
	tstrdestroy(&fp);
	tstrdestroy(&tp);
	tstrdestroy(&comm);
	return 1;
}
