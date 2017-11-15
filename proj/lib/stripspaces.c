#include <unistd.h>
#include <ctype.h>

char *stripspaces(char *str) {

	char *p = str;
	char *s = NULL;
	char *e = NULL;

	if (!p) return NULL;
	while (*p && (*p != '\r') && (*p != '\n')) {
		if (!isspace(*p)) {
			if (!s) s = p;
			e = p;
		}
		p++;
	}
	if (s) {
		*(e + 1) = '\0';
	}
	return s;
}
