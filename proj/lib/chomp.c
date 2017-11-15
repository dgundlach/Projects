#include <unistd.h>

size_t chomp(char *str, size_t len) {

	char *s;

	if (str) {
		if (len <= 0) {
			s = str;
			while (*s) {
				s++;
			}
		} else {
			s = str + len - 1;
		}
		while (s >= str) {
			switch (*s) {
				case ' ':
				case '\t':
				case '\v':
				case '\f':
				case '\r':
				case '\n':
				case '\0':
					s--;
					break;
				default:
					*++s = '\0';
					return s - str;
			}
		}
		*str = '\0';
	}
	return 0;
}
