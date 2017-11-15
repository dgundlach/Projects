#include <stdio.h>
#include <stdlib.h>

void xalloc_fatal(char *message) {

	printf("%s\n", message);
	exit(-1);
}

void (*xalloc_fatal_call)(char *) = &xalloc_fatal;
char *xalloc_fatal_message = "Virtual memory exhausted.";

void *xmalloc(size_t size, ...) {

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

