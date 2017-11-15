#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include "xalloc.h"

char *strfilter(char *str, int new, int *map) {

	char *r = str;
	char *newstr;
	char *w;
	unsigned char c, lastc = ' ';

	if (!str) return NULL;
	if (!*str) return NULL;
	if (new) {
		w = newstr = xmalloc(strlen(str) + 1);
	} else {
		w = newstr = str;
	}
	if (map) {
		while ((c = *r++)) {
			c = map[c];
			if (!isspace(c) || !isspace(lastc)) {
				*w++ = c;
			}
			lastc = c;
		}
	} else {
		while ((c = *r++)) {
			if (!isspace(c) || !isspace(lastc)) {
				*w++ = c;
			}
			lastc = c;
		}
	}
	if ((lastc == ' ') && (w != newstr)) {
		w--;
	}
	*w = '\0';
	return newstr;
}

char *strfilterdd(char *str, int new, int *map) {

	char *r = str;
	char *newstr;
	char *w;
	unsigned char c, lastc = ' ';

	if (!str) return NULL;
	if (!*str) return NULL;
	if (new) {
		w = newstr = xmalloc(strlen(str) + 1);
	} else {
		w = newstr = str;
	}
	if (map) {
		while ((c = *r++)) {
			if ((c == '-') && (*r == '-')) {
				c = map[':'];
				r++;
				if (c == ':') {
					lastc = *w++ = c;
					c = ' ';
				}
			} else {
				c = map[c];
			}
			if (!isspace(c) || !isspace(lastc)) {
				*w++ = c;
			}
			lastc = c;
		}
	} else {
		while ((c = *r++)) {
			if ((c == '-') && (*r == '-')) {
				*w++ = ':';
				c = *w++ = ' ';
				r++;
			} else if (!isspace(c) || !isspace(lastc)) {
				*w++ = c;
			}
			lastc = c;
		}
	}
	if ((lastc == ' ') && (w != newstr)) {
		w--;
	}
	*w = '\0';
	return newstr;
}
