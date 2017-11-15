#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "readtextline.h"
#include "limits.h"

typedef struct filebuf {
	FILE *f;
	FILE *(*openfun)(const char *, const char *);
	int (*closefun)(FILE*);
	int bufsize;
	char *buf;
} filebuf;

//
//  Read a line from a file or a pipe.  If the file is not open, allocate
//  some space for a structure with a buffer, and return the first line.
//  If the buffer is too small, reallocate it until it isn't. On subsequent
//  calls, the allocation does not get done unless the buffer is too small.
//  When end of file is reached, destroy the structure and return an EOF
//  condition.
//

int read_text_line(char **buffer, char *path) {

	struct filebuf *filebuf;
	struct filebuf *b;
	int p;
	int rv = READTEXTLINE_SUCCESS;
	size_t len;
	char *t;

//
//  If there's no buffer, create one and open the file.  Otherwise, calculate
//  where the filebuf structure starts.
//

	if (!*buffer) {
		if (!(filebuf = malloc(PATH_MAX + sizeof(struct filebuf)))) {
			return READTEXTLINE_OOM;
		}
		filebuf->bufsize = PATH_MAX;
		filebuf->buf = (char *)filebuf + sizeof(struct filebuf);
		len = strlen(path);
		char pth[len + 1];
		if (path[len - 1] == '|') {
			filebuf->openfun = &popen;
			filebuf->closefun = &pclose;
		} else {
			len++;
			filebuf->openfun = &fopen;
			filebuf->closefun = &fclose;
		}
		strncpy(pth, path, len);
		if (!(filebuf->f = filebuf->openfun(pth, "r"))) {
			free(filebuf);
			return READTEXTLINE_EOF;
		}
		*buffer = filebuf->buf;
	} else {
		filebuf = (void *)*buffer - sizeof(struct filebuf);
	}

	p = filebuf->bufsize - 2;
	filebuf->buf[p] = '\0';
	while ((filebuf->buf[p] != '@') && (filebuf->buf[p] != '\n')) {

//
//  Set the second to last character of the buffer to some value that we can
//  test for to see if the buffer is large enough to hold the line.  It will
//  either be '\n' or the value that we set it to if the buffer is big enough.
//  That way, we don't have to search through the buffer looking for a '\n'.
//

		filebuf->buf[p] = '@';
		if (fgets(filebuf->buf, filebuf->bufsize, filebuf->f)) {
			if ((filebuf->buf[p] != '@') && (filebuf->buf[p] != '\n')) {

//
//  The buffer is not large enough, reallocate it and read more characters onto
//  the end.
//

				filebuf->bufsize += PATH_MAX;
				p += PATH_MAX;
				if (b = realloc(filebuf, filebuf->bufsize
						+ sizeof(struct filebuf))) {
					filebuf = b;
					filebuf->buf = (char *)b + sizeof(struct filebuf);
					*buffer = filebuf->buf;
				} else {

//
//  We can't allocate any more memory, so the whole thing is blown.  Close the
//  file, and deallocate the memory for our buffer.
//

					filebuf->closefun(filebuf->f);
					free(filebuf);
					*buffer = NULL;
					return READTEXTLINE_OOM;
				}

//
//  We need to add characters to the end of the buffer, so we start where the
//  '\0' was written.  We can read in 1 extra character this time.  If we
//  can't read anything more, send along what was already read.  We'll close
//  the file and deallocate the buffer the next time.  Don't tell the calling
//  program that we've reached EOF just yet.
//

				filebuf->buf[p] = '@';
				if (!fgets(filebuf->buf + p - (PATH_MAX - 1),
						filebuf->bufsize + 1, filebuf->f)) {
					break;
				}
			}
		} else {

//
// We're at EOF.  close the file, free the memory, and return EOF.
//

			filebuf->closefun(filebuf->f);
			free(filebuf);
			*buffer = NULL;
			rv = READTEXTLINE_EOF;
			break;
		}
	}
	return rv;
}

//
//  Read a single line text file or pipe.  This may be a simple procedure, but
//  it saves alot of hassle.
//

char *read_single_line_text_file(char *buffer, size_t size, char *path) {

	FILE *f;
	FILE *(*openfun)(const char *, const char *) = &fopen;
	int (*closefun)(FILE*) = &fclose;
	char *rv = NULL;
	char t[PATH_MAX];
	size_t len;

	len = strlen(path);
	if (path[len -1] == '|') {
		openfun = &popen;
		closefun = &pclose;
	} else {
		len++;
	}
	strncpy(t, path, len);
	if (f = openfun(t, "r")) {
		rv = fgets(buffer, size, f);
		closefun(f);
	}
	return rv;
}
