#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <alloca.h>
#include <ctype.h>
#include <sys/types.h>

#include "xalloc.h"
#include "skipspace.h"
#include "initokenize.h"

char *ini_gettok(char *dest, char *source, char *ident_inc, int *len) {

	char *d = dest;
	char *s = source;
	char e;
	char c;
	int l = 0;

	if (!s) return s;
	s = skipspace(s, ";#");
	c = *s;
	if (c) {
	
//
// Identifier.
//

		if (strchr(ident_inc, c)) {
			do {
				*d++ = c;
				l++;
				c = *++s;
			} while (c && strchr(ident_inc, c));

//
// Quoted string.
//

		} else if ((c == '\'') || (c == '"') || (c == '`')) {
			e = c;
			do {
				*d++ = c;
				l++;
				c = *++s;
				if (c == '\\') {
					*d++ = c;
					l++;
					c = *++s;
					if (c) {
						*d++ = c;
						l++;
						c = *++s;
					}
				}
			} while (c && (c != e));
			if (c) {
				*d++ = c;
				l++;
				s++;
			}

//
// Punctuation.
//

		} else {
			*d++ = c;
			l++;
			s++;
		}
	}
	*len = l;
	*d = '\0';
	return s;
}

void ini_split_free(ini_t *ini_data) {

	if (ini_data) {
		free(ini_data->data);
		memset(ini_data, 0, sizeof(struct ini_t));
	}
}

enum ini_datatype ini_split(ini_t *ini_data, char *line, int linelen, char *ident_inc) {

	int datasize;
	char *d, *s;
	char *lvaluestart = NULL;
	char *lvalueend = NULL;
	char *rvaluestart = NULL;
	char *rvalueend = NULL;
	int storeprev;
	int len;
	int count = 0;
	int maxcount;
	
	if (!line) return ini_error_type;
	if (!*line) return ini_no_type;
	datasize = 2 * (linelen + 1) + 256;
	ini_data->lvalue = NULL;
	ini_data->rvalue = NULL;
	ini_data->lcount = 0;
	ini_data->rcount = 0;
	ini_data->rparts = NULL;
	if (!ini_data->data || (datasize > ini_data->datasize)) {
		ini_data->data = xrealloc(ini_data->data, datasize);
		ini_data->datasize = datasize;
	}
	if (ini_data->maxcount < 16) {
		ini_data->lparts = xrealloc(ini_data->lparts, 16 * sizeof(char *));
		ini_data->llengths = xrealloc(ini_data->llengths, 16 * sizeof(int));
		ini_data->maxcount = 16;
	}
	maxcount = ini_data->maxcount;
	d = ini_data->data;
	ini_data->linetype = ini_no_type;
	s = skipspace(line, "#;");
	if (*s) {
		lvaluestart = s;
		if (*s == '[') {
			ini_data->linetype = ini_section_type;
			s = ini_gettok(d, s + 1, ident_inc, &len);
			if (strchr(ident_inc, *d) && (*s == ']')) {
				ini_data->llengths[ini_data->lcount] = len;
				ini_data->lparts[ini_data->lcount++] = d;
				d += len + 1;
				lvalueend = ++s;
				s = ini_gettok(d, s, ident_inc, &len);
				if (len) {
					ini_data->linetype = ini_error_type;
				}
			} else {
				ini_data->linetype = ini_error_type;
			}
		} else if (strchr(ident_inc, *s)) {
			ini_data->linetype = ini_parameter_type;
			while (*s && (*s != ':') && (*s != '=')) {
				s = ini_gettok(d, s, ident_inc, &len);
				if (!strchr(ident_inc, *d)) {
					ini_data->linetype = ini_error_type;
					break;
				}
				if (count >= maxcount) {
					ini_data->maxcount = maxcount += 16;
					ini_data->lparts = xrealloc(ini_data->lparts, maxcount * sizeof(char *));
					ini_data->llengths = xrealloc(ini_data->llengths, maxcount * sizeof(int));
				}
				ini_data->llengths[count] = len;
				ini_data->lparts[count++] = d;
				d += len + 1;
				lvalueend = s;
				s = skipspace(s, ";#");
			}
			ini_data->lcount = count;
			if ((ini_data->linetype != ini_error_type)
					&& ((*s == ':') || (*s == '='))) {

//
// Store each token of the right hand side separately.  Each part
// can be separated by spaces, commas, or both.  If the list starts
// with a comma; or ends with one; or there are 2 successive commas,
// an empty string is stored in the parameter list.
//

				ini_data->rparts = &ini_data->lparts[count];
				ini_data->rlengths = &ini_data->llengths[count];
				s = skipspace(s + 1, ";#");
				rvaluestart = s;
				rvalueend = s;
				storeprev = 2;		// Handle a possibe leading comma.
				while (*s) {
					s = ini_gettok(d, s, ident_inc, &len);
					if (len) {
						rvalueend = s;
						if (*d == ',') {
							if (storeprev) {
								*d = '\0';
								len = 1;
							} else {
								len = 0;
							}
							storeprev = 1;
						} else {
							len++;
						}
						if (len) {
							if (count >= maxcount) {
								ini_data->maxcount = maxcount += 16;
								ini_data->lparts = xrealloc(ini_data->lparts, maxcount * sizeof(char *));
								ini_data->rparts = &ini_data->lparts[ini_data->lcount];
								ini_data->llengths = xrealloc(ini_data->llengths, maxcount * sizeof(int));
								ini_data->rlengths = &ini_data->llengths[ini_data->lcount];
							}
							ini_data->llengths[count] = len - 1;
							ini_data->lparts[count++] = d;
							d += len;
							storeprev = 0;
						}
					}
				}
				if (storeprev == 1) {	// Handle a trailing comma.
					if (ini_data->lcount >= maxcount) {
						ini_data->maxcount = maxcount += 16;
						ini_data->lparts = xrealloc(ini_data->lparts, maxcount * sizeof(char *));
						ini_data->rparts = &ini_data->lparts[ini_data->lcount];
						ini_data->llengths = xrealloc(ini_data->llengths, maxcount * sizeof(int));
						ini_data->rlengths = &ini_data->llengths[ini_data->lcount];
					}
					ini_data->llengths[count] = 0;
					ini_data->lparts[count++] = d;
					*d++ = '\0';
				}
				ini_data->rcount = count - ini_data->lcount;
			} else {
				ini_data->linetype = ini_error_type;
			}
		} else {
			ini_data->linetype = ini_error_type;
		}
	}
	if (ini_data->linetype	== ini_error_type) {

//
// Clear everything if there was an error.
//

		ini_data->lvalue = NULL;
		ini_data->rvalue = NULL;
		ini_data->lcount = 0;
		ini_data->rcount = 0;
		ini_data->rparts = NULL;
		int i;
		for (i=0; i<256; i++) {
			ini_data->lparts[i] = NULL;
		}
	} else {

//
// Store the left and right hand sides each as a whole.
//

		s = lvaluestart;
		ini_data->lvalue = d;
		while (s < lvalueend) {
			*d++ = *s++;
		}
		*d++ = '\0';
		s = rvaluestart;
		ini_data->rvalue = d;
		while (s < rvalueend) {
			*d++ = *s++;
		}
		*d = '\0';
	}
	return ini_data->linetype;
}
