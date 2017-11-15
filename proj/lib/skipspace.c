#include <string.h>

/*
 * Skip whitespace in a string; if we hit a comment delimiter, consider the rest of the
 * entry a comment.
 */

char *skipspace(char *whence, char *cdelim) {

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
				if (strchr(cdelim, *whence)) {
					while (*whence) {
						whence++;
					}
				}
				return whence;
		}
	}
}
