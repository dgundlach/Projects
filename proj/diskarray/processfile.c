#include <stdlib.h>
#include <stdio.h>

#define BUF_ALLOC 4096
#define PROCESSFILE_COMPLETE 100
#define PROCESSFILE_ERROR -100
#define OOM -100

int process_text_file(char *path, void *result, int (*process_line)(char *, void *)) {

	FILE *f;
	void *res;
	char *b;
	char *buf;
	int bufsize = BUF_ALLOC;
	int p = BUF_ALLOC - 2;
	int rv = 0;

	if (!(buf = malloc(bufsize))) return OOM;
	if (f = fopen(path, "r")) {
		rv = 0;

/*
 * Set the second to last character of the buffer to some value that we can test for
 * to see if the buffer is large enough to hold the line.  It will either be '\n' or
 * the value that we set it to if the buffer is big enough.  That way, we don't have
 * to search through the buffer looking for a '\n'.
 */

		buf[p] = '@';
		while (fgets(buf, bufsize, f)) {
			while ((buf[p] != '@') && (buf[p] != '\n')) {

/*
 * The buffer is not large enough, reallocate it and read more characters onto the end.
 */

				bufsize += BUF_ALLOC;
				p += BUF_ALLOC;
				if (b = realloc(buf, bufsize)) {
					buf = b;
				} else {
					free(buf);
					return OOM;
				}
				buf[p] = '@';

/*
 * We need to add characters to the end of the buffer, so we start where the '\0' was
 * written.  We can read in 1 extra character this time.
 */

				if (!fgets(buf + p - (BUF_ALLOC - 1), bufsize + 1, f)) break;
			}

/*
 * If we've read in a full line, process the line.
 */

			if ((buf[p] == '@') || (buf[p] == '\n')) {
				rv = process_line(buf, result);
				if ((rv == PROCESSFILE_COMPLETE) || (rv == PROCESSFILE_ERROR)) break;
			}
			buf[p] = '@';
		}
		fclose(f);
	}
	free(buf);
	return rv;
}
