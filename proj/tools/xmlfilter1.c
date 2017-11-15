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

int process_it(FILE *f, int pre, int ts) {

	char *inbuf = NULL;
	char *outbuf = NULL;
	char *ip;
	char *ie;
	char *op;
	char *oe;
	char *printend;
	ssize_t len;
	size_t buflen = 0;
	size_t outbuflen = 4096;
	char c, lastc;
	int tc;
	int count, i;
	char *src;
	#define _scopy(dest, source)\
		src = source;			\
		while (*src) {			\
			*dest++ = *src++;	\
		}

	outbuf = malloc(outbuflen);
	oe = outbuf + outbuflen;
	if (pre) {
		printf("<pre>\n");
	}
	while ((len = getline(&inbuf, &buflen, f)) != -1) {
		ip = inbuf;
		ie = inbuf + len;
		op = outbuf;
		printend = op;
		lastc = ' ';
		tc = 0;
		while (ip < ie) {
			c = *ip++;
			if ((c == '\0') || (c == '\v')) c = ' ';
			if ((oe - op) < 20) {
				oe = op - (long)outbuf;
				printend = printend - (long)outbuf;
				outbuflen += 4096;
				outbuf = realloc(outbuf, outbuflen);
				op = outbuf + (long)oe;
				oe = outbuf + (long)outbuflen;
				printend = outbuf + (long)printend;
			}
			switch(c) {
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
				case ' ':
				case '\t':
					count = 0;
					if (!pre) {
						if (c == ' ') {
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
				case '\n':
				case '\r':
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
		if (printend > outbuf) {
			*printend++ = '\n';
			*printend++ = '\0';
			printf("%s", outbuf);
		} else {
			printf("\n");
		}
	}
	if (pre) {
		printf("</pre>\n");
	}
}

void usage(char **argv) {

	fprintf(stderr, "\nUsage: %s [-p] [-t <tabsize>] [-h|?] [file]\n"
					"\t-p          : Output data between <pre> and </pre>\n"
					"\t-t <tabsize>: Set the tab stops to <tabsize>\n"
					"\t-h,-?       : This text\n\n"
					"If a file is not specified, standard input is used.\n\n",
					basename(argv[0]));
	exit(-1);
}

int main(int argc, char **argv) {

	FILE *f = stdin;
	int pre = 0;
	int tabfill = 8;
	int c;
	char *arg;

	while (1) {
		if ((c = getopt(argc, argv, "pt:h?")) == -1) break;

		switch(c) {
			case 't':
				tabfill = strtol(optarg, &arg, 10);
				if (*arg || (tabfill <= 0) || (tabfill > 16)) {
					fprintf(stderr, "%s: invalid argument for tabsize.\n", optarg);
					exit(-1);
				}
				break;
			case 'p':
				pre = 1;
				break;
			case '?':
			case 'h':
			default:
				usage(argv);
		}
	}
	if (optind != argc) {
		arg = argv[optind++];
		if (strcmp(arg, "-") && !(f = fopen(arg, "r"))) {
			fprintf(stderr, "%s: file not found.\n", arg);
			exit(-1);
		}
	}
	if (optind != argc) {
		usage(argv);
	}
	process_it(f, pre, tabfill);
	fclose(f);
}
