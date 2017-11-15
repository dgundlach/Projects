#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void xalloc_fatal(char *message) {

	printf("%s\n", message);
	exit(-1);
}

void (*xalloc_fatal_call)(char *) = &xalloc_fatal;
char *xalloc_fatal_message = "Virtual memory exhausted.";

void *xmalloc(size_t size) {

	register void *value;

	value = malloc(size);
	if (!value) {
		xalloc_fatal_call(xalloc_fatal_message);
	}
	return value;
}

void *xcalloc(size_t nmemb, size_t size) {

	register void *value;

	value = calloc(nmemb, size);
	if (!value) {
		xalloc_fatal_call(xalloc_fatal_message);
	}
	return value;
}

void *xrealloc(void *ptr, size_t size) {

	register void *value;

	value = realloc(ptr, size);
	if (!value) {
		xalloc_fatal_call(xalloc_fatal_message);
	}
	return value;
}

char *xstrdup(char *s) {

	register char *value;

	value = strdup(s);
	if (!value) {
		xalloc_fatal_call(xalloc_fatal_message);
	}
	return value;
}		

char *xstrndup(char *s, size_t n) {

	register char *value;

	value = strndup(s, n);
	if (!value) {
		xalloc_fatal_call(xalloc_fatal_message);
	}
	return value;
}		

char *xgetcwd(int *len) {

	register char *value;
	register char *wd;
	register size_t curlen = 256;

	value = xmalloc(curlen);
	while (!(wd = getcwd(value, curlen))) {
		curlen += 256;
		value = xrealloc(value, curlen);
	}
	curlen = strlen(value) + 1;
	value = xrealloc(value, curlen);
	if (len) {
		*len = curlen;
	}
	return value;
}

char *xgetcwdr(char **buf, int *len) {

	register char *value;
	register char *wd;
	register size_t curlen;

	if (!buf || !len) {
		return NULL;
	}
	curlen = *len;
	value = *buf;
	if (!value) {
		if (!curlen) {
			curlen = 256;
		}
		value = xmalloc(curlen);
	}
	while (!(wd = getcwd(value, curlen))) {
		curlen += 256;
		value = xrealloc(value, curlen);
	}
	*len = curlen;
	*buf = value;
	return value;
}

char *xreadlink(const char *path, size_t *bufsize, ssize_t *len) {

	register char *value;
	register size_t cursize = 256;
	register ssize_t curlen;

	if (!path) {
		return NULL;
	}
	value = xmalloc(cursize);
	while ((curlen = readlink(path, value, cursize)) == cursize) {
		cursize += 256;
		value = xrealloc(value, cursize);
	}
	if (curlen == -1) {
		free(value);
		value = NULL;
		cursize = 0;
	} else {
		value[curlen] = '\0';
	}
	if (bufsize) {
		*bufsize = cursize;
	}
	if (len) {
		*len = curlen;
	}
	return value;
}

ssize_t xreadlinkr(const char *path, char **buf, size_t *bufsize) {

	register char *value;
	register size_t cursize;
	register ssize_t curlen;

	if (!path || !buf || !bufsize) {
		return -1;
	}
	cursize = *bufsize;
	value = *buf;
	if (!value) {
		if (!cursize) {
			cursize = 256;
		}
		value = xmalloc(cursize);
	}
	while ((curlen = readlink(path, value, cursize)) == cursize) {
		cursize += 256;
		value = xrealloc(value, cursize);
	}
	if (curlen != -1) {
		value[curlen] = '\0';
	} else {
		if (!*buf) {
			free(value);
			value = NULL;
			cursize = *bufsize;
		} else {
			if (*bufsize != cursize) {
				value = realloc(value, *bufsize);
				cursize = *bufsize;
			}
		}
	}
	*bufsize = cursize;
	*buf = value;
	return curlen;
}
