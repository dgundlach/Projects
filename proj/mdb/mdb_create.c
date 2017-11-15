#define _GNU_SOURCE
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
#include "stripspaces.h"

int makemovie(char *volume, char *moviename, char *thumb) {

	static char *exts[] = {".nfo", ".tbn", "-fanart.jpg", NULL};
	keyPair *drivepath;
	size_t isosize;
	size_t totalsize;
	char *m, *d;
	int i;
	char buffer[4096];
	FILE *srcf, *destf;
	size_t size;
	struct stat st;
	tstr *command;
	tstr *source;
	tstr *dest;
	char *volname;
	
	if (!volume || !moviename) return -1;
	char tvn[strlen(volume) + 1];
	char tmn[strlen(moviename) + 1];
	strcpy(tvn, volume);
	volname = basename(tvn);
	strcpy(tmn, moviename);
	moviename = basename(tmn);

	if ((drivepath = mdb_find(moviename, 0))) {
		fprintf(stderr, "Movie exists: %s/%s\n", drivepath->key, moviename);
		return -1;
	}
	if ((mdb_calcsize(volume, moviename, &isosize, &totalsize) == -1)) {
		fprintf(stderr, "Could not calculate iso size of: %s\n", volume);
		return -1;
	}
	if (!(drivepath = mdb_findspace(mdb_dir_list, totalsize))) {
		fprintf(stderr, "Not enough space for: %s\n", moviename);
		return -1;
	}
	dest = tstrnew(NULL, 256);
	tstrprintf(&dest, "%s/%s", drivepath->key, moviename);
	if (mkdir(dest->str, 0755) == -1) {
		fprintf(stderr, "Could not create directory: %s\n", dest->str);
		tstrdestroy(&dest);
		return -1;
	}
	fprintf(stdout, "Writing: %s\n", dest->str);
	tstrcatprintf(&dest, "/%s", moviename);
	tstrsettrunc(dest, dest->length, 0);
	source = tstrnew(NULL, 256);
	tstrprintf(&source, "%s/%s", mdb_meta, moviename);
	tstrsettrunc(source, source->length, 0);
	for (i=0; exts[i]; i++) {
		tstrcatprintf(&source, "%s", exts[i]); 
		if ((srcf = fopen(source->str, "r"))) {
			tstrcatprintf(&dest, "%s", exts[i]);
			if (destf = fopen(dest->str, "w")) {
				while ((size = fread(buffer, 1, 4096, srcf))) {
					fwrite(buffer, 1, size, destf);
				}
				fclose(destf);
			}
			fclose(srcf);
		}
	}
	if (thumb && (srcf = fopen(thumb, "r"))) {
		tstrcatprintf(&dest, ".tbn");
		if ((destf = fopen(dest->str, "w"))) {
			while ((size = fread(buffer, 1, 4096, srcf))) {
				fwrite(buffer, 1, size, destf);
			}
			fclose(destf);
		}
		fclose(srcf);
	}
	stat(volume, &st);
	if (S_ISREG(st.st_mode)) {
		tstrprintf(&dest, "pv -s %dk -p -e -r > \"%s/%s/%s\"",
				isosize, drivepath->key, moviename, volname);
		if ((srcf = fopen(volume, "r"))) {
			if ((destf = popen(dest->str, "w"))) {
				while ((size = fread(buffer, 1, 4096, srcf))) {
					fwrite(buffer, 1, size, destf);
				}
				fclose(destf);
			}
			fclose(srcf);
		}
	} else {
		tstrprintf(&dest, "mkisofs -udf -dvd-video -V \"%s\" \"%s\" 2>/dev/null"
				" | pv -s %dk -p -e -r > \"%s/%s/%s.iso\"",
				volname, volume, isosize, drivepath->key, moviename, moviename);
		system(dest->str);
	}
	tstrprintf(&source, "%s/%s", drivepath->key, moviename);
	tstrprintf(&dest, "%s/%s", mdb_movies, moviename);
	symlink(source->str, dest->str);
	tstrdestroy(&dest);
	tstrdestroy(&source);
	return 0;
}

void usage(char *program) {

	fprintf(stderr, "Usage: %s [-h] <-b file | Volume Title [Thumb]>"
					"\t    -b : Batch, volumes and titles in file\n"
					"\t    -h : Display this text\n"
					"\tVolume : Directory containing DVD contents, or iso file\n"
					"\tTitle  : Title to create\n"
					"\tThumb  : Optional thumbnail file\n", basename(program));
}

int main(int argc, char **argv) {

	char *batchfile = NULL;
	int opt;
	char *volume;
	char *moviename;
	char *thumb = NULL;
	FILE *f;
	char *buffer = NULL;
	size_t bufsize = 0;

	if (mdb_init(MDB_NO_BACKUP) == -1) {
		fprintf(stderr, "%s\n", mdb_errorstr);
		exit(-1);
	}
	while ((opt = getopt(argc, argv, "b:h")) != -1) {
		switch (opt) {
			case 'b':
				batchfile = optarg;
				break;
			case 'h':
				usage(argv[0]);
				exit(0);
			default:
				usage(argv[0]);
				exit(-1);
		}
	}
	if (batchfile) {
		if (!(f = fopen(batchfile, "r"))) {
			fprintf(stderr, "%s: not found\n", batchfile);
			exit(-1);
		}
		while (getline(&buffer, &bufsize, f) != -1) {
			volume = buffer;
			while (*volume == '\t') {
				volume++;
			}
			if (moviename = strchr(volume, '\t')) {
				while (*moviename == '\t') {
					*moviename++ = '\0';
				}
				if (thumb = strchr(moviename, '\t')) {
					while (*thumb == '\t') {
						*thumb++ = '\0';
					}
					thumb = stripspaces(thumb);
				}
				volume = stripspaces(volume);
				moviename = stripspaces(moviename);
				if (*volume && *moviename) {
					makemovie(volume, moviename, thumb);
				}
			}
		}
		fclose(f);
	} else {
		if ((argc - optind) < 2) {
			usage(argv[0]);
			exit(-1);
		}
		volume = argv[optind++];
		moviename = argv[optind++];
		if (optind < argc) {
			thumb = argv[optind];
		}
		makemovie(volume, moviename, thumb);
	}
	mdb_savemasterlist();
}
