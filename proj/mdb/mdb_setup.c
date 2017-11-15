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
#include "xalloc.h"
#include "strfilter.h"
#include "currorprev.h"

#define ARRAY_DEFAULT "/cygdrive"

void usage(char *program) {

}

static int cmpstringp(const void *p1, const void *p2) {

	return strcmp(* (char * const *) p1, * (char * const *) p2);
}

int main(int argc, char **argv) {

	struct stat st;
	struct utsname uts;
	char *array = NULL;
	char *sysdrive = NULL;
	int use_sysdrive = 0;
	char *p, *t;
	DIR *d;
	struct dirent *entry;
	char **alldrives = NULL;
	int a_count = 0;
	int a_max = 0;
	int i;
	char buffer[1024];
	char *answer;
	int map[256];
	mdb_drive *drive;
	char *meta = NULL;
	char *master_list = NULL;
	char *home;
	
	for (i=0; i<256; i++) map[i] = i;
	for (i=8; i<14; i++) map[i] = ' ';

	if (!uname(&uts)) {
		if (!strncmp(uts.sysname, "CYGWIN_NT", 9)) {
			array = ARRAY_DEFAULT;
			t = getenv("SYSTEMDRIVE");
			*t = *t - 'A' + 'a';
			sysdrive = xstrdup(t);
			*(sysdrive + 1) = '\0';
		} else if (stat(ARRAY_DEFAULT, &st) && S_ISDIR(st.st_mode)) {
			array = ARRAY_DEFAULT;
		}
	} else {
		fprintf(stderr, "Cannot determine OS.\n");
		exit(1);
	}
	mdb_init(MDB_BACKUP);
	if (stat(mdb_data, &st)) {
		if (mkdir(mdb_data, 0755) == -1) {
			fprintf(stderr, "Home directory is not writeable!\n");
			exit(1);
		}
		mkdir(mdb_backup, 0755);
		mkdir(mdb_removed, 0755);
	}
	if (stat(mdb_movies, &st)) {
		if (mkdir(mdb_movies, 0755) == -1) {
			fprintf(stderr, "Home directory is not writeable!\n");
			exit(1);
		}
	}
	if (mdb_dir_list->list) {
		t = array;
		array = strdup(mdb_dir_list->list[0]->key);
		if ((p = strchr(array + 1, '/'))) {
			*p = '\0';
		} else {
			free(array);
			array = t;
		}
	}
	printf("\nEnter the base directory for your array of drives.  If a "
		   "default value\nis shown, you can usually press <Enter> to a"
		   "ccept the default.\n\nBase Directory");
	if (array) {
		printf(" [%s]", array);
	}
	printf(": ");
	if (fgets(buffer, sizeof(buffer), stdin)) {
		answer = strfilter(buffer, 1, map);
		if (strlen(answer)) {
			array = answer;
		} else {
			free(answer);
		}
	}
	if (array) {
		if (d = opendir(array)) {
			while ((entry = readdir(d))) {
				if (!CURRORPREV(entry->d_name)) {
					if (a_count > (a_max - 2)) {
						a_max += 256;
						alldrives = xrealloc(alldrives, a_max * sizeof(void *));
					}
					alldrives[a_count++] = xstrdup(entry->d_name);
				}
			}
			alldrives[a_count] = NULL;
			closedir(d);
		}
	}
	printf("\nEnter the drives that you want to use for movies in the or"
		   "der that you\nwant to fill them.  The drives will only be us"
		   "ed if you manually add a\nMovies folder.  If you include the"
		   " system drive in the list, it will be\nmoved to the end.  Yo"
		   "u may include drive letters not shown if you plan on\nadding"
		   " them later.  You can always run this script at a later date"
		   " to\ninclude new drives.\n\n");
	if (a_count) {
		qsort(alldrives, a_count, sizeof(char *), &cmpstringp);
		printf("Available drives are: [");
		for (i=0; alldrives[i]; i++) {
			if (i) {
				printf(" ");
			}
			printf("%s", alldrives[i]);
		}
		printf("]\n\n");
	}
	printf("Drives");
	if (mdb_dir_list->list) {
		printf(" [");
		for (i=0; mdb_dir_list->list[i]; i++) {
			if (i) {
				printf(" ");
			}
			printf("%s", ((mdb_drive *)(mdb_dir_list->list[i]->data))->alias);
		}
		printf("]");
	}
	printf(": ");
	if (fgets(buffer, sizeof(buffer), stdin)) {
		if (!strcmp(array, ARRAY_DEFAULT)) {
			for (i='A'; i<='Z'; i++) {
				map[i] = map[i] - 'A' + 'a';
			}
		}
		answer = strfilter(buffer, 1, map);
		if (strlen(answer)) {

		} else {
			free(answer);
		}
	}
	for (i='A'; i<='Z'; i++) {
		map[i] = i;
	}
	meta = xreadlink(mdb_meta, NULL, NULL);
	printf("\nEnter the location of your movie data cache.  If you don't "
		   "have one or don't\nwant to use one, enter nothing here.  If a"
		   "default value is shown, it will be\nselected by default if yo"
		   "u just press enter.\n\nData Cache");
	if (meta) {
		printf("[%s]", meta);
	}
	printf(": ");
	if (fgets(buffer, sizeof(buffer), stdin)) {
		answer = strfilter(buffer, 1, map);
		if (strlen(answer)) {

		} else {
			free(answer);
		}
	}
	if (mdb_masterlist) {
		master_list = mdb_masterlist;
	} else {
		home = getenv("HOME");
		master_list = xmalloc(strlen(home) + 32);
		sprintf(master_list, "%s/Documents/Movie_db.txt", home);
	}
	printf("\nA master list can be maintained by these scripts.  Enter th"
		   "e the full name and path where you would like the list to be "
		   "stored.\n\nMaster List [%s]: ");
	if (fgets(buffer, sizeof(buffer), stdin)) {
		answer = strfilter(buffer, 1, map);
		if (strlen(answer)) {

		} else {
			free(answer);
		}
	}
	
	
	
}




































int main2(int argc, char **argv) {

	int i;
	FILE *f = stdout;
	struct stat st;
	DIR *d;
	struct dirent *entry;
	tstr *path;
	tstr *linkpath;
	char *meta = NULL;
	char *media = NULL;
	int opt;
	struct utsname uts;

	if ((uname(&uts))) exit(1);
	while ((opt = getopt(argc, argv, "d:m:h?")) != -1) {
		switch (opt) {
			case 'd':
				meta = optarg;
				break;
			case 'm':
				media = optarg;
				break;
			case 'h':
			case '?':
				usage(argv[0]);
				exit(0);
			default:
				usage(argv[0]);
				exit(1);
		}
	}
	if (!media) {
		if (!strncmp(uts.sysname, "CYGWIN_NT", 9)
				|| (stat("/cygdrive", &st) && S_ISDIR(st.st_mode))) {
			media = "/cygdrive";
		} else {
			fprintf(stderr, "Cannot determine media path.  Specify it with the -m option.\n");
			exit(1);
		}
	}
	path = tstrnew(NULL, 256);
	linkpath = tstrnew(NULL, 256);
	if (stat(mdb_movies, &st)) {
		if (mkdir(mdb_movies, 0755) == -1) {
			fprintf(stderr, "Could not create directory: %s\n", mdb_movies);
			exit(1);
		}
	}
	if (stat(mdb_data, &st)) {
		mkdir(mdb_data, 0755);
		mkdir(mdb_backup, 0755);
		mkdir(mdb_removed, 0755);
	}
	if ((d = opendir(mdb_backup))) {
		while ((entry = readdir(d))) {
			if (!CURRORPREV(entry->d_name)) {
				tstrprintf(&path, "%s/%s", mdb_backup, entry->d_name);
				unlink(path->str);
			}
		}
		closedir(d);
	}
	if ((d = opendir(mdb_movies))) {
		while ((entry = readdir(d))) {
			if (!CURRORPREV(entry->d_name)) {
				tstrprintf(&path, "%s/%s", mdb_movies, entry->d_name);
				unlink(path->str);
			}
		}
		closedir(d);
	}
	mdb_init(MDB_BACKUP);
	for (i=0; mdb_dir_list->list[i]; i++) {
		if ((d = opendir(mdb_dir_list->list[i]->key))) {
			while ((entry = readdir(d))) {
				if (!CURRORPREV(entry->d_name)) {
					tstrprintf(&path, "%s/%s", mdb_dir_list->list[i]->key,
							entry->d_name);
					if (!stat(path->str, &st) && S_ISDIR(st.st_mode)) {
						tstrprintf(&linkpath, "%s/%s", mdb_movies, entry->d_name);
						symlink(path->str, linkpath->str);
						tstrprintf(&linkpath, "%s/%s", mdb_backup, entry->d_name);
						symlink(path->str, linkpath->str);
					}
				}
			}
			closedir(d);
		}
	}
}
