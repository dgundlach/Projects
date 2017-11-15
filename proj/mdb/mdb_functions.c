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
#include <sys/utsname.h>
#include "xalloc.h"
#include "stripspaces.h"
#include "keypair.h"
#include "tstr.h"
#include "currorprev.h"

#define ARRAY_DEFAULT "/cygdrive"

#define MDB_NO_BACKUP	0
#define MDB_BACKUP		1

typedef struct mdb_drive {
	char *mount;
	char *trash;
	char *alias;
} mdb_drive;

char *mdb_array = ARRAY_DEFAULT;
char *mdb_data = NULL;
char *mdb_meta = NULL;
char *mdb_backup = NULL;
char *mdb_removed = NULL;
char *mdb_missing = NULL;
char *mdb_movies = NULL;
char *mdb_movielist = NULL;
char *mdb_masterlist = NULL;
char *mdb_conf = NULL;
char *mdb_remote = NULL;
keyPairList *mdb_dir_list = NULL;
keyPairList *mdb_movie_list = NULL;
keyPairList *mdb_backup_list = NULL;
int mdb_have_pv = 0;
char *mdb_errorstr = NULL;
char mdb_systemdrive = '\0';

#define MDB_REMOTE_SYMLINK "ssh %s \"mdb_link%s %s\""

int mdb_symlink(const char *oldpath, int backup) {

	char *backupsw;
	tstr *newpath;
	tstr *comm;

	if (backup) {
		backupsw = " -b";
	} else {
		backupsw = "";
	}
	if (!mdb_remote) {
		tstr *newpath = tstrnew(256);
		return symlink(oldpath, newpath);
	}
	
}

keyPair *mdb_new_drive(char *mount, char *target) {

	struct stat st;
	keyPair *new;
	mdb_drive *new_drive;
	size_t len;

	if (!mount) return NULL;
	len = strlen(mount) + strlen(mdb_array) + 16;
	char path[len];
	sprintf(path, "%s/%s/%s", mdb_array, mount, target);
	if (stat(path, &st) || !S_ISDIR(st.st_mode)) return NULL;
	new = xmalloc(sizeof(keyPair));
	new->key = xstrdup(path);
	new->data = new_drive = xmalloc(sizeof(mdb_drive) + (3 * len));
	new_drive->mount = (void *)new_drive + sizeof(mdb_drive);
	new_drive->trash = new_drive->mount + 1 + sprintf(new_drive->mount, "%s/%s", mdb_array, mount);
	new_drive->alias = new_drive->trash + 1 + sprintf(new_drive->trash, "%s/%s/_Trash", mdb_array, mount);
	strcpy(new_drive->alias, mount);
	return new;
}

keyPairList *mdb_load_drives(keyPairList *drive_list, char *target, char *use) {

	keyPairList *new = drive_list;
	keyPair *new_pair;
	DIR *d;
	struct dirent *entry;
	char *u, *drive;
	struct utsname uts;
	char sysdrive[2] = "\0\0";
	int use_sysdrive = 0;

	if (!new) {
		new = newKeyPairList(16);
	}
	uname(&uts);
	if (!strncmp(uts.sysname, "CYGWIN_NT", 9)) {
		u = getenv("SYSTEMDRIVE");
		sysdrive[0] = *u - 'A' + 'a';
	}
	if (!use || !strlen(use)) {
		if ((d = opendir(mdb_array))) {
			while ((entry = readdir(d))) {
				if (!CURRORPREV(entry->d_name)) {
					if (!strcmp(entry->d_name, sysdrive)) {
						use_sysdrive = 1;
					} else {
						if ((new_pair = mdb_new_drive(entry->d_name, target))) {
							new = addExistingKeyPair(new, new_pair);
						}
					}
				}
			}
			closedir(d);
		}
	} else {
		char temp[strlen(use) + 1];
		strcpy(temp, use);
		u = temp;
		while (*u) {
			while (*u && isspace(*u)) {
				u++;
			}
			if (*u) {
				drive = u;
				while (*u && !isspace(*u)) {
					u++;
				}
				if (*u) {
					 *u++ = '\0';
				}
				if (!strcmp(drive, sysdrive)) {
					use_sysdrive = 1;
				} else {
					if (new_pair = mdb_new_drive(drive, target)) {
						new = addExistingKeyPair(new, new_pair);
					}
				}
			}
		} 
	}
	if (use_sysdrive && (new_pair = mdb_new_drive(sysdrive, target))) {
		new = addExistingKeyPair(new, new_pair);
	}
	return new;
}

int mdb_load_conf(int backup) {

	FILE *f;
	char *line = NULL;
	size_t line_len = 0;
	ssize_t len;
	char *var, *value;
	char *home;

	if ((f = fopen(mdb_conf, "r"))) {
		while ((len = getline(&line, &line_len, f)) != -1) {
			if ((value = strchr(line, '='))) {
				*value++ = '\0';
				var = stripspaces(line);
				value = stripspaces(value);
				if (!strcasecmp(var, "array")) {
					if (strlen(value)) {
						mdb_array = xstrdup(value);
					} else {
						mdb_array = xstrdup(ARRAY_DEFAULT);
					}
				} else if (!strcasecmp(var, "drives")) {						
					mdb_dir_list = mdb_load_drives(mdb_dir_list, "Movies", value);
				} else if (!strcasecmp(var, "backup")) {
					if (backup) {
						mdb_backup_list = mdb_load_drives(mdb_backup_list, "_Movies", value);
					}
				} else if (!strcasecmp(var, "master list")) {
					if (strlen(value)) {
						if (*value != '/') {
							home = getenv("HOME");
							mdb_masterlist = xmalloc(strlen(home) + strlen(value) + 2);
							sprintf(mdb_movielist, "%s/%s", home, value);
						} else {
							mdb_masterlist = xstrdup(value);
						}
					} else {
						mdb_masterlist = mdb_movielist;
					}
				} else {
					fprintf(stderr, "%s: unknown key: %s\n", mdb_conf, var);
				}
			}
		}
		fclose(f);
		free(line);
	} else {
		mdb_dir_list = mdb_load_drives(NULL, "Movies", NULL);
	}
	if (backup && !mdb_backup_list) {
		mdb_backup_list = mdb_load_drives(NULL, "_Movies", NULL);
	}
	return 0;
}

int mdb_init(int backup) {

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
	mdb_movielist = mdb_removed + 1
			+ sprintf(mdb_removed, "%s/removed", mdb_data);
	mdb_conf = mdb_movielist + 1
			+ sprintf(mdb_movielist, "%s/Movie_db", mdb_data);
	mdb_movies = mdb_conf + 1
			+ sprintf(mdb_conf, "%s/mdb.conf", mdb_data);
	sprintf(mdb_movies, "%s/Movies", home);
	if ((stat(mdb_data, &st) || !S_ISDIR(st.st_mode))
			|| (stat(mdb_backup, &st) || !S_ISDIR(st.st_mode))
			|| (stat(mdb_removed, &st) || !S_ISDIR(st.st_mode))
			|| (stat(mdb_movies, &st) || !S_ISDIR(st.st_mode))) {
		mdb_errorstr = "Database not configured.  Run mdb_setup.";
		return -1;
	}
	mdb_load_conf(backup);
	if (!mdb_dir_list->list) {
		mdb_errorstr = "No drives found";
		return -1;
	}
	if ((f = popen("pv -h", "r"))) {
		pclose(f);
		mdb_have_pv = 1;
	}
	return 0;
}

keyPair *mdb_findspace(keyPairList *dirlist, size_t size) {

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
				return dirlist->list[i];
			}
		}
	}
	return NULL;
}

#define CALC_SIZE "mkisofs -udf -dvd-video -print-size \"%s\" 2>/dev/null"

int mdb_calcsize(char *volume, char *movie, long *size, long *totalsize) {

	struct stat st;
	char buffer[512];
	char *e;
	FILE *f;
	tstr *path;

	if (stat(volume, &st)) return -1;
	*size = 0;
	if (S_ISREG(st.st_mode)) {
		*size = st.st_size / 1024;
	} else {
		sprintf(buffer, CALC_SIZE, volume);
		if ((f = popen(buffer, "r"))) {
			if (fgets(buffer, 512, f)) {
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

keyPair *mdb_find(char *movie, int trash) {

	int i;
	tstr *path = tstrnew(NULL, 256);
	keyPair *drive = NULL;
	struct stat st;

	for (i=0; mdb_dir_list->list[i]; i++) {
		if (trash) {
			tstrprintf(&path, "%s/%s", ((mdb_drive *)(mdb_dir_list->list[i]->data))->trash, movie);
		} else {
			tstrprintf(&path, "%s/%s", mdb_dir_list->list[i]->key, movie);
		}
		if (!stat(path->str, &st)) {
			drive = mdb_dir_list->list[i];
			break;
		}
	}
	tstrdestroy(&path);
	return drive;
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
		if (!CURRORPREV(entry->d_name)) {
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

	if (!(d = opendir(dir))) return -1;
	dirpath = tstrnew(NULL, 256);
	linkpath = tstrnew(NULL, 256);
	while ((entry = readdir(d))) {
		if (!CURRORPREV(entry->d_name)) {
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
	return 0;
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
		new->freepair = &free;
		for (i=0; mdb_dir_list->list[i]; i++) {
			mdb_loaddir(new, mdb_dir_list->list[i]->key, ((mdb_drive *)(mdb_dir_list->list[i]->data))->alias);
		}
	}
	if (new) {
		qsort(new->list, new->count, sizeof(keyPair *), &mdb_compare);
	}
	return new;
}

int mdb_savelist(char *path, keyPairList **movies, int index) {

	int i;
	FILE *f;
	int fd;
	char tmpname[16];

	if (!movies) return -1;
	if (!*movies) {
		*movies = mdb_loadlist(NULL);
	}
	if (!(*movies)->list) return -1;
	strcpy(tmpname, "/tmp/mdb.XXXXXX");
	if (!(fd = mkstemp(tmpname))) return -1;
	if (!(f = fdopen(fd, "w"))) return -1;
	if (index) {
		for (i=0; (*movies)->list[i]; i++) {
			fprintf(f, "%s\t%s\r\n", ((mdb_drive *)((*movies)->list[i]->data)), (*movies)->list[i]->key);
		}
	} else {
		for (i=0; (*movies)->list[i]; i++) {
			fprintf(f, "%s\r\n", (*movies)->list[i]->key);
		}
	}
	fclose(f);
	return rename(tmpname, path);
}

int mdb_savemasterlist(void) {

	if (mdb_masterlist) {
		return mdb_savelist(mdb_masterlist, &mdb_movie_list, 1);
	}
	return -1;
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
	int rc;

	if (!frompath || !topath) return -1;
	char tfp[strlen(frompath) + 1];
	strcpy(tfp, frompath);
	movie = basename(tfp);
	if (!(d = opendir(frompath))) return -1;
	tp = tstrnew(NULL, 256);	
	tstrprintf(&tp, "%s/%s", topath, movie);
	rc = mkdir(tp->str, 0755);
	fp = tstrnew(NULL, 256);
	comm = tstrnew(NULL, 256);
	while ((entry = readdir(d))) {
		tstrprintf(&fp, "%s/%s", frompath, entry->d_name);
		if (!CURRORPREV(entry->d_name) && !stat(fp->str, &st)
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
	return 0;
}
