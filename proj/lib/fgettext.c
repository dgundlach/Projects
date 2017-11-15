#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "xalloc.h"
#include "fgettext.h"

typedef struct filebuf {
	FILE *f;
	int (*closefun)(FILE*);
	size_t alloc;
	char *buf;
	char *path;
	struct filebuf *next;
} filebuf;

int fgettext(char **buffer, int *alloc, char *path) {

	static filebuf *head;
	FILE *(*openfun)(const char *, const char *) = &fopen;
	filebuf *curr = NULL;
	filebuf *prev = NULL;
	size_t len;

	if (!buffer || !alloc || !path) return FGETTEXT_INVALID_INPUT;

//
// See if we already have the file open.
//

	curr = head;
	while (curr) {
		if (!strcmp(path, curr->path)) break;
		prev = curr;
		curr = curr->next;
	}

//
// If the file or pipe is not already open, open it, and put the information
// at the head of the list of opened files.
//

	if (!curr) {
		len = strlen(path);
		char newpath[len + 1];
		curr = xmalloc(sizeof(struct filebuf) + len + 1);
		curr->buf = NULL;
		curr->alloc = 0;
		curr->path = (char *)(curr + sizeof(struct filebuf));
		strcpy(curr->path, path);
		if (path[len - 1] == '|') {
			curr->closefun = &pclose;
			openfun = &popen;
		} else {
			len++;
			curr->closefun = &fclose;
		}
		strncpy(newpath, path, len);
		if (!(curr->f = openfun(newpath, "r"))) {
			free(curr);
			return FGETTEXT_NOEXIST;
		}
		curr->next = head;
		head = curr;
	} else {
		curr->alloc = *alloc;
	}

//
// Read a line from the file or pipe into a buffer.  If the buffer does not
// exist, one will be created.  Pass the information back to the calling
// program.
//

	len = 0;	
	if (!feof(curr->f)) {
		if ((len = getline(&curr->buf, &curr->alloc, curr->f)) > 0) {
			*buffer = curr->buf;
			*alloc = curr->alloc;
		}
	} else {
		len = FGETTEXT_EOF;
	}

//
// If we've run into end of file, close the file or pipe, and destroy the
// entry in the opened file list.
//

	if (len < 1) {
		curr->closefun(curr->f);
		if (prev) {
			prev->next = curr->next;
		} else {
			head = curr->next;
		}
		free(curr);
	}
	return len;
}

//
// This function is only called to short-circuit the reading of the entire
// file or pipe when the entire contents are not needed.
//

int fgettext_close(char *buffer) {

	static filebuf *head;
	filebuf *curr = NULL;
	filebuf *prev = NULL;
	char *buf;
	int rv = FGETTEXT_FAILURE;

	if (!buffer) return FGETTEXT_NOEXIST;

//
// Search the list of open files, looking for the one associated with our
// buffer.
//

	curr = head;
	while (curr) {
		if (curr->buf == buf) break;
		prev = curr;
		curr = curr->next;
	}

//
// Close the file and destroy the entry associated with it.
//

	if (curr) {
		curr->closefun(curr->f);
		if (prev) {
			prev->next = curr->next;
		} else {
			head = curr->next;
		}
		free(curr);
		rv = FGETTEXT_SUCCESS;
	}
	return rv;
}

//
// Read a single line text file or pipe.  This may be a simple procedure, but it saves alot of hassle.
//

int fgettext_single(char **buffer, int *alloc, char *path) {

	FILE *f;
	FILE *(*openfun)(const char *, const char *) = &fopen;
	int (*closefun)(FILE*) = &fclose;
	char *buf;
	size_t a;
	size_t len = strlen(path);
	char newpath[len + 1];

	if (!buffer || !alloc || !path) return FGETTEXT_INVALID_INPUT;
	buf = *buffer;
	len = strlen(path);
	if (path[len - 1] == '|') {
		openfun = &popen;
		closefun = &pclose;
	} else {
		len++;
	}
	strncpy(newpath, path, len);
	len = FGETTEXT_NOEXIST;
	if ((f = openfun(newpath, "r"))) {
		if ((len = getline(&buf, &a, f)) > 0) {
			*buffer = buf;
			*alloc = a;
		}
		closefun(f);
	}
	return len;
}
