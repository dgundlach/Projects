#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <libgen.h>

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

size_t process_buf(char **outputbuf, size_t *outbufsize, char *inbuf,
		size_t len, int pre, int ts, enum os_type os) {

	char *ip = inbuf;
	char *ie = inbuf + len;
	char *outbuf = NULL;
	char *op = NULL;
	char *oe = NULL;
	char *printend = NULL;
	size_t outbuflen;
	char c;
	char lastc = ' ';
	int tc = 0;
	int count, i;
	char *src;

	if (!outputbuf || !outbufsize) return -1;
	outbuf = *outputbuf;
	outbuflen = *outbufsize;
	printend = outbuf;
	op = outbuf;
	oe = outbuf + outbuflen;
	while (ip < ie) {
		c = *ip++;
		if ((c == '\0') || (c == '\v')) c = ' ';
		if ((oe - op) < 20) {
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
// '&', '<', '>', and '"' are simple to handle.
//
			case '&':
				_scopy(op, "&amp;");
				printend = op;
				break;
			case '<':
				_scopy(op, "&lt");
				printend = op;
				break;
			case '>':
				_scopy(op, "&gt;");
				printend = op;
				break;
			case '"':
				_scopy(op, "&quot;");
				printend = op;
				break;
//
// Handle spaces and tabs.
//			
			case ' ':
			case '\t':
				count = 0;
				if (!pre) {
					if (c == ' ') {
//
// Only convert spaces if that last character was also a space or a tab.
//
						if ((lastc == ' ') || (lastc == '\t')) {
							count = 1;
						}
					} else {
						count = ts - tc;
					}
				}
				if (count) {
					for (i=0; i<count; i++) {
						_scopy(op, "&nbsp;");
						tc++;
					}
					tc--;
				} else {
//
// Convert tabs to a series of spaces if the tab stops are not the default.
//
					if ((c == '\t') && (ts != 8)) {
						count = ts - tc;
						for (i=0; i<count; i++) {
							*op++ = ' ';
							tc++;
						}
						tc--;
					} else {
						*op++ = c;
					}
				}
				break;
//
// Strip all space and tab characters from the end of the current output line
// and set the appropriate line ending.  Set the tab counter so that the next
// line starts with it at zero.
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
				}
				printend = op;
				tc = ts - 1;
				c = ' ';
				break;
//
// Strip \r chars and decrement the tab counter so that the tab column will
// be correct when we increment it later.
//
			case '\r':
				tc--;
				c = ' ';
				break;
			default:
				*op++ = c;
				printend = op;
		}
		lastc = c;
		if (++tc == ts) {
			tc = 0;
		}
	}
	*printend = '\0';
	*outputbuf = outbuf;
	*outbufsize = outbuflen;
	return (size_t)printend - (size_t)outbuf;
}

int process_file(int fd, int pre, int ts, enum os_type os) {

	char inbuf[4096];
	char *outbuf = NULL;
	size_t outbufsize = 0;
	size_t outbuflen = 0;
	size_t len;
	char *ip = inbuf;
	char *ie;
	char c;

	if (pre) {
		printf("<pre>\n");
	}
	while ((len = read(fd, inbuf, 4096)) > 0) {
//
// Detect what kind of text file we have.
//
// LF:    Multics, Unix and Unix-like  systems (GNU/Linux, AIX, Xenix, Mac OS X,
//        FreeBSD, etc.), BeOS, Amiga, RISC OS, and others.
// CR:    Commodore 8-bit machines, Apple II family, Mac OS up to version 9  and OS-9
// CR+LF: DEC RT-11 and most other early non-Unix, non-IBM OSes, CP/M, MP/M, DOS,
//        OS/2, Microsoft Windows, Symbian OS
//
		if (os == autodetect_os) {
			ie = inbuf + len;
			do {
				c = *ip++;
			} while ((ip < ie) && (c != '\n') && (c != '\r'));
			if (c == '\r') {
				if (*ip == '\n') {
					os = dos_os;
				} else {
					os = mac_os;
				}
			} else {
				os = unix_os;
			}
		}		
		outbuflen = process_buf(&outbuf, &outbufsize, inbuf, len, pre, ts, os);
		write(1, outbuf, outbuflen);
	}
	if (pre) {
		printf("</pre>\n");
	}
}

void usage(char **argv) {

	fprintf(stderr, "\nUsage: %s [-d] [-m] [-p] [-t <tabsize>] [-u] [-h|?] [file]\n"
					"\t-d           : Use DOS/Windows line endings.\n"
					"\t-m           : Use Mac OS line endings (version < 10).\n"
					"\t-p           : Output data between <pre> and </pre>\n"
					"\t-t <tabsize> : Set the tab size to <tabsize> (default: 8)\n"
					"\t-u           : Use UNIX/Linux/OSX line endings (default).\n"
					"\t-h,-?        : This text\n\n"
					"If a file is not specified, standard input is used.\n\n",
					basename(argv[0]));
	exit(-1);
}

int main(int argc, char **argv) {

	int fd = 0;
	int pre = 0;
	int tabfill = 8;
	enum os_type os = unix_os;
	int c;
	char *arg;
	
	while (1) {
		if ((c = getopt(argc, argv, "dmpt:uh?")) == -1) break;
		switch(c) {
			case 'd':
				os = dos_os;
				break;
			case 'm':
				os = mac_os;
				break;
			case 'p':
				pre = 1;
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
	if (optind != argc) {
		arg = argv[optind++];
		if (strcmp(arg, "-") && ((fd = open(arg, O_RDONLY)) < 0)) {
			fprintf(stderr, "%s: file not found.\n", arg);
			exit(-1);
		}
	}
	if (optind != argc) {
		usage(argv);
	}
	process_file(fd, pre, tabfill, os);
	close(fd);
}
