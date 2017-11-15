#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

#define _scopy(dest, source)\
	src = source;			\
	while (*src) {			\
		*dest++ = *src++;	\
	}

enum os_type {
	autodetect_os,
	unix_os,
	mac_os,
	dos_os
} os_type_e;

//
// Detect what kind of text file we have.
//
// LF:    Multics, Unix and Unix-like  systems (GNU/Linux, AIX, Xenix, Mac OS X,
//        FreeBSD, etc.), BeOS, Amiga, RISC OS, and others.
// CR:    Commodore 8-bit machines, Apple II family, Mac OS vers. 1-9 and OS-9
// CR+LF: DEC RT-11 and most other early non-Unix, non-IBM OSes, CP/M, MP/M,
//        DOS, OS/2, Microsoft Windows, Symbian OS
//
enum os_type detect_os_type(char *buf, size_t len) {

	char *bp = buf + len;
	char c;

	do {
		c = *--bp;
	while ((bp > buf) && (c != '\r') && (c != '\n'));
	if (c == '\r') {
		return mac_os;
	} else if (c == '\n') {
		bp--;
		if ((bp >= buf) && (*bp == '\r')) {
			return dos_os;
		} else {
			return unix_os;
		}
	} else {
		return autodetect_os;
	}
}

size_t expand_tabs(char **outputbuf, size_t *outbufsize, char *inbuf,
			size_t len, int ts, enum os_type os) {

	char *ip = inbuf;
	char *ie = inbuf + len;
	char *op = NULL;
	char *oe = NULL;
	char *printend = NULL;
	char *outbuf = NULL;
	size_t outbuflen;
	char c;
	int tc = 0;
	int count, i;
	char *src;
	enum os_type from_os;

	if (!outputbuf || !outbufsize) return -1;
	from_os = detect_os_type(inbuf, len);
	if (os == autodetect_os) {
		os = from_os;
	}
	outbuf = *outputbuf;
	outbuflen = *outbufsize;
	printend = outbuf;
	op = outbuf;
	oe = outbuf + outbuflen;
	while (ip < ie) {
		c = *ip++;
		if ((oe - op) < 20) {
//
// Save our place in case the buffer address is changed.
//
			oe = op - (long)outbuf;
			printend = printend - (long)outbuf;
			outbuflen += 3 * len;
			outbuf = realloc(outbuf, outbuflen);
			op = outbuf + (long)oe;
			oe = outbuf + (long)outbuflen;
			printend = outbuf + (long)printend;
		}
		switch(c) {
//
// Strip BS, VT, and NUL
//
			case '\b':
			case '\v':
			case '\0':
				continue;
//
// Spaces and tabs don't count until a non-space is encountered.
//
			case ' ':
				*op++ = c;
				break;
//
// Handle tabs.
//
			case '\t':
				count = ts - tc;
				for (i=0; i<count; i++) {
					*op++ = ' ';
				}
				tc = -1;
				break;
//
// Strip CRs if we're not processing a Mac text file.
//
			case '\r':
				if (from_os != mac_os) {
					continue;
				}
				// Fall through
//
// Strip all spaces from the end of the current output line and set the
// appropriate line ending.  Set the tab counter so that the next line
// starts with it at zero.
//
			case '\n':
				op = printend;
				switch (os) {
					case dos_os:
						*op++ = '\r';
						// Fall through
					case unix_os:
						*op++ = '\n';
						break;
					case mac_os:
						*op++ = '\r';
						break;
				}
				printend = op;
				tc = -1;
				break;
			default:
				*op++ = c;
				printend = op;
		}
		if (++tc == ts) {
			tc = 0;
		}
	}
	*printend = '\0';
	*outputbuf = outbuf;
	*outbufsize = outbuflen;
	return (size_t)printend - (size_t)outbuf;
}

int process_file(int fd, int ts, enum os_type os) {

	char inbuf[4096];
	char *outbuf = NULL;
	size_t outbufsize = 0;
	size_t outbuflen = 0;
	size_t len;
	char *ip = inbuf;
	char *ie;
	char c;
	int tc = 0;

	while ((len = read(fd, inbuf, sizeof(inbuf))) > 0) {
		outbuflen = expand_tabs(&outbuf, &outbufsize, inbuf, len, &tc, ts, os);
		write(1, outbuf, outbuflen);
	}
}

int getfile(char **buf, int *bsize, char *path) {

	int len = 0;
	int ilen;
	struct stat st;
	int fd;

	if (!buf || !bsize) return -1;
	if (!path || ((*path == '-') && !*(path + 1))) {
		do {
			if (*bsize <= len + 1) {
				*bsize += 4096;
				*buf = realloc(*buf, *bsize);
			}
			if ((ilen = read(0, *buf + len, *bsize - len)) > 0) {
				len += ilen;
			}
		} while (ilen > 0);
	} else {
		if (!stat(path, &st) & ((fd = open(path, O_RDONLY)) > 0)) {
			*buf = realloc(*buf, st.st_size + 1);
			len = read(fd, *buf, st.st_size + 1);
			close(fd);
		} else {
			len = -1;
		}
	}
	return len;
}

void usage(char **argv) {

	fprintf(stderr, "\nUsage: %s [-d] [-m] [-t <tabsize>] [-u] [-h|?] [file]\n"
					"\t-d           : Use DOS/Windows line endings.\n"
					"\t-m           : Use Mac OS line endings (version < 10).\n"
					"\t-t <tabsize> : Set the tab size to <tabsize> (default: 8)\n"
					"\t-u           : Use UNIX/Linux/OSX line endings (default).\n"
					"\t-h,-?        : This text\n\n"
					"If a file is not specified, standard input is used.\n\n",
					basename(argv[0]));
	exit(-1);
}

int main(int argc, char **argv) {

	char *path = "-";
	int fd = 0;
	int tabfill = 8;
	enum os_type os = autodetect_os;
	int c;
	char *arg;
	char *buf = NULL;
	char *outbuf = NULL;
	size_t bufsize;
	size_t outbufsize;

	while (1) {
		if ((c = getopt(argc, argv, "dmt:uh?")) == -1) break;
		switch(c) {
			case 'd':
				os = dos_os;
				break;
			case 'm':
				os = mac_os;
				break;
			case 't':
				tabfill = strtol(optarg, &arg, 10);
				if (*arg || (tabfill <= 0) || (tabfill > 16)) {
					fprintf(stderr, "%s: invalid argument for tabsize.\n", optarg);
					exit(-1);
				}
				break;
			case 'u':
				os = unix_os;
				break;
			case '?':
			case 'h':
			default:
				usage(argv);
		}
	}
//	if (optind != argc) {
//		arg = argv[optind++];
//		if (strcmp(arg, "-") && ((fd = open(arg, O_RDONLY)) < 0)) {
//			fprintf(stderr, "%s: file not found.\n", arg);
//			exit(-1);
//		}
//	}
	if (optind != argc) {
		path = argv[optind++];
	}
	if (optind != argc) {
		usage(argv);
	}
	len = getfile(&buf, &bufsize, path);
	len = expand_tabs(&outbuf, &outbufsize, buf, len, tabfill, os);
	write(1, outbuf, len);
//	process_file(fd, tabfill, os);
//	close(fd);
}
