#include <ctype.h>
#include <stdio.h>

int split(char *str, char **parts, int max_parts, int (*ctest)(int)) {

	int (*test)(int);
	char *s = str;
	char c;
	int i = 0;

	if (ctest) test = ctest;
	else test = &isspace;

	c = *s++;
	while (c) {
		while (c && test(c)) {
			c = *s++;
		}
		if (c) {
			parts[i++] = s - 1;
			if (i == (max_parts - 1)) break;
			while (c && !test(c)) {
				c = *s++;
			}
			*(s - 1) = '\0';
		}
		if (i == (max_parts - 1)) break;
		if (c) {
			c = *s++;
		}
	}
	parts[i] = NULL;
	return i;
}

/*
 * Skip whitespace in a string.
 */

char *split_skipspace(char *whence) {

	while (1) {
		switch (*whence) {
			case ' ':
			case '\b':
			case '\t':
			case '\n':
			case '\v':
			case '\f':
			case '\r':
				whence++;
				break;
			default:
				return whence;
		}
	}
}

char *split_skipspace_comments(char *whence) {

	while (1) {
		switch (*whence) {
			case ' ':
			case '\b':
			case '\t':
			case '\n':
			case '\v':
			case '\f':
			case '\r':
				whence++;
				break;
			default:
				if (*whence == '#') {
					while (*whence) {
						whence++;
					}
				}
				return whence;
		}
	}
}

int splitn(char *str, char **parts, char *(*next)(char *)) {

	char *(*test)(char *);
	char *currstr = str;
	char *nextstr = str;
	char c;
	int i = 0;

	if (next) test = next;
	else test = &split_skipspace;

	currstr = test(currstr);
	while (currstr) {
		parts[i++] = currstr++;
		while ((nextstr = test(currstr)) == currstr) {
			currstr++;
		}
		*currstr = '\0';
		currstr = nextstr;
	}
	parts[i] = NULL;
	return i;
}
