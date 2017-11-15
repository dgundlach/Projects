#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <alloca.h>
#include <ctype.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include "positiondata.h"

typedef struct directory_t {
	char *name;
	int uid;
	int gid;
	int mode;
	struct directory_t *next;
} directory_t;

directory_t *default_dirs = NULL;
directory_t *dir_lists[256];

enum ini_datatype {
	ini_no_type,
	ini_section_type,
	ini_parameter_type,
	ini_error_type
} ini_type;

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

#define IDENT_CHARS "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_-."

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

typedef struct ini_t {
	enum ini_datatype linetype;
	char *lvalue;
	char *rvalue;
	int lcount;
	int rcount;
	char *lparts[256];
	char **rparts;
	int llengths[256];
	int *rlengths;
	char *data;
	int datasize;
} ini_t;

#define QUOTE_CHARS "\"'`"

void parse_ini_line2_free(ini_t *ini_data) {

	if (ini_data) {
		free(ini_data->data);
		memset(ini_data, 0, sizeof(struct ini_t));
	}
}

enum ini_datatype parse_ini_line2(ini_t *ini_data, char *line) {

	int datasize;
	char *d, *s;
	char *lvaluestart = NULL;
	char *lvalueend = NULL;
	char *rvaluestart = NULL;
	char *rvalueend = NULL;
	int storeprev;
	int len;
	
	if (!line) return ini_error_type;
	if (!*line) return ini_no_type;
	datasize = 2 * (strlen(line) + 1) + 256;
	if (datasize > ini_data->datasize) {
		if (!(d = realloc(ini_data->data, datasize))) return ini_error_type;
		ini_data->data = d;
		ini_data->datasize = datasize;
	}
	ini_data->linetype = ini_no_type;
	s = skipspace(line, "#;");
	if (*s) {
		lvaluestart = s;
		if (*s == '[') {
			ini_data->linetype = ini_section_type;
			s = ini_gettok(d, s + 1, IDENT_CHARS, &len);
			if (strchr(IDENT_CHARS, *d) && (*s == ']')) {
				ini_data->llengths[ini_data->lcount] = len;
				ini_data->lparts[ini_data->lcount++] = d;
				d += len + 1;
				lvalueend = ++s;
				s = ini_gettok(d, s, IDENT_CHARS, &len);
				if (len) {
					ini_data->linetype = ini_error_type;
				}
			} else {
				ini_data->linetype = ini_error_type;
			}
		} else if (strchr(IDENT_CHARS, *s)) {
			ini_data->linetype = ini_parameter_type;
			while (*s && (*s != ':') && (*s != '=')) {
				s = ini_gettok(d, s, IDENT_CHARS, &len);
				if (!strchr(IDENT_CHARS, *d)) {
					ini_data->linetype = ini_error_type;
					break;
				}
				if (ini_data->lcount < 256) {
					ini_data->llengths[ini_data->lcount] = len;
					ini_data->lparts[ini_data->lcount++] = d;
				}
				d += len + 1;
				lvalueend = s;
				s = skipspace(s, ";#");
			}
			if ((ini_data->linetype != ini_error_type)
					&& ((*s == ':') || (*s == '#'))) {

//
// Store each token of the right hand side separately.  Each part
// can be separated by spaces, commas, or both.  If the list starts
// with a comma; or ends with one; or there are 2 successive commas,
// an empty string is stored in the parameter list.
//

				ini_data->rparts = &ini_data->lparts[ini_data->lcount];
				ini_data->rlengths = &ini_data->llengths[ini_data->lcount];
				s = skipspace(s, ";#");
				rvaluestart = s;
				rvalueend = s;
				storeprev = 2;		// Handle a possibe leading comma.
				while (*s) {
					s = ini_gettok(d, s, IDENT_CHARS, &len);
					if (len) {
						rvalueend = s;
						if (*d = ',') {
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
							if (ini_data->rcount < (256 - ini_data->lcount)) {
								ini_data->rlengths[ini_data->rcount] = len - 1;
								ini_data->rparts[ini_data->rcount++] = d;
							}
							d += len;
							storeprev = 0;
						}
					}
				}
				if (storeprev == 1) {	// Handle a trailing comma.
					if (ini_data->rcount < (256 - ini_data->lcount)) {
						ini_data->rlengths[ini_data->rcount] = 0;
						ini_data->rparts[ini_data->rcount++] = d;
					}
					*d++ = '\0';
				}
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
	} else if (ini_data->linetype != ini_no_type) {

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

enum ini_datatype parse_ini_line(char **data, int *dcount, char *line) {

	char *lcopy;
	char *d[7];
	char *p;
	enum ini_datatype linetype = ini_no_type;
	int i, len;
	int count;
	char sep[5];

//
// Strip any comments from the input.
//

	lcopy = alloca(strlen(line) + 1);
	strcpy(lcopy, line);
	if (i = strcspn(lcopy, "#;")) {
		*(lcopy + i) = '\0';
	}

//
// Allocate some temporary space for our data.
//

	len = strlen(lcopy) + 1;
	for (i=0; i<6; i++) {
		d[i] = alloca(len);
		*d[i] = '\0';
		data[i] = NULL;
	}

//
// Parse the line.
//

	if ((count = sscanf(lcopy, " [ %[0-9A-Za-z_-] ] %s", d[0], d[1])) == 1) {
		linetype = ini_section_type;
	} else if (count == 2) {
		linetype = ini_error_type;
	} else if ((count = sscanf(lcopy,
			" %[0-9A-Za-z_-.] %1[:=] %[^,\r\n] , %[^,\r\n] , %[^,\r\n] , %[^,\r\n] , %[^,\r\n] ",
			d[0], sep, d[1], d[2], d[3], d[4], d[5], d[6])) >= 2) {
		if (count > 2) count--;
		linetype = ini_parameter_type;
	} else if (count == 1) {
		linetype = ini_error_type;
	}

//
// Copy the strings to the line, and set up pointers to them.
//

	if ((linetype != ini_no_type) && (linetype != ini_error_type)) {
		lcopy = malloc(len + len + count);
		for (i=0; i<count; i++) {
			data[i] = lcopy;
			lcopy += 1 + sprintf(lcopy, "%s", d[i]);
		}
		if (linetype == ini_parameter_type) {
			data[i] = lcopy;
			p = line + 1 + strcspn(line, ":=");
			while (*p && isspace(*p)) {
				p++;
			}
			strcpy(lcopy, p);
			p = lcopy + strlen(lcopy) - 1;
			while ((p > lcopy) && isspace(*p)) {
				*p-- = '\0';
			}
		}
	} else {
		count = 0;
	}
	*dcount = count;
	return linetype;
}

enum sections {
	no_section,
	device_section,
	directory_section,
	label_section,
	error_section
} section_e;

void report(int line, char *data, char *mess) {

	printf("%d: %s - %s\n", line, data, mess);
}

enum sections create_dir_entry(enum sections section, char **data, int count, directory_t **dir_list, int line) {

	int len;
	int uid;
	int gid = -1;
	char *groupname;
	struct passwd *pwent;
	struct group *grent;
	int mode;
	int i;
	char *q;
	directory_t *direct_entry;

	len = strlen(data[1]);
	if ((i = strcspn(data[1], ".:")) < len) {
		groupname = data[1] + i;
		*groupname++ = '\0';
		if (grent = getgrnam(groupname)) {
			gid = grent->gr_gid;
		} else {
			report(line, groupname, "Group name not found.");
			return error_section;
		}
	}
	if (pwent = getpwnam(data[1])) {
		uid = pwent->pw_uid;
		if (gid == -1) {
			gid = pwent->pw_gid;
		}
	} else {
		report(line, data[1], "User name not found.");
		return error_section;
	}
	if (count == 3) {
		mode = strtol(data[2], &q, 8);
		if (*q || (mode < 0) || (mode > 0777)) {
			report(line, data[2], "Syntax error in mode or mode out of range.");
			return error_section;
		}
	} else {
		mode = 0755;
	}
	direct_entry = malloc(sizeof(struct directory_t));
	direct_entry->name = data[0];
	direct_entry->uid = uid;
	direct_entry->gid = gid;
	direct_entry->mode = mode;
	direct_entry->next = *dir_list;
	*dir_list = direct_entry;
	return section;
}

int parse_ini(void) {

	char *data[10];
	int count;
	char buf[256];
	enum sections section = no_section;
	enum ini_datatype linetype;
	int i;
	FILE *f;
	int unique_id;
	char *q;
	int len;
	int label_index;
	int line = 0;

	memset(dir_lists, '\0', 256 * sizeof(struct directory_t *));
	f = fopen("labels.conf", "r");
	while (fgets(buf, 256, f)) {
		linetype = parse_ini_line(data, &count, buf);
		line++;
		if (linetype == ini_error_type) {
			free(data[0]);
			report(line, buf, "Line will not parse.");
			break;
		}
		switch (linetype) {
			case ini_section_type:
				if (!strcasecmp(data[0], "devices")) {
					section = device_section;
				} else if (!strcasecmp(data[0], "directories")) {
					section = directory_section;
				} else {
					for (label_index=0; label_index<256; label_index++) {
						if (!strcasecmp(data[0], position_data[label_index])) break;
					}
					if (label_index < 256) {
						section = label_section;
					} else {
						section = error_section;
						report(line, data[0], "Unrecognized section name.");
					}
				}
				free(data[0]);
				break;
			case ini_parameter_type:
				switch (section) {
					case no_section:
						section = error_section;
						report(line, buf, "We are not in a section.");
						free(data[0]);
						break;
					case device_section:
						len = strlen(data[1]);
						unique_id = strtol(data[0], &q, 10);
						if (!*q && (unique_id >= 1) && (unique_id <= 256)
								&& (len > 0) && (len < 14)) {
							position_data[unique_id - 1] = data[1];
						} else {
							section = error_section;
							report(line, data[1], "Unique id out of range.");
							free(data[0]);
						}
						break;
					case directory_section:
						section = create_dir_entry(section, data, count, &default_dirs, line);
						if (section == error_section) {
							free(data[0]);
						}
						break;
					case label_section:
						section = create_dir_entry(section, data, count, &dir_lists[label_index], line);
						if (section == error_section) {
							free(data[0]);
						}
						break;
				}
				break;
		}
		if (section == error_section) break;
	}
	fclose(f);
	if (section = error_section) return 0;
	return 1;
}

int main(int argc, char **argv) {

	char *data[10];
	int count;
	char sbuf[256];
	char dbuf[256];
	char *s;
	char *d;
	int len;
	enum ini_datatype linetype;
	int i;
	FILE *f;
	directory_t *dirs;

	f = fopen("labels.conf", "r");
	while (fgets(sbuf, 256, f)) {
		s = sbuf;
		d = dbuf;
		printf("%s", sbuf);
		while (*s) {
			s = ini_gettok(d, s, IDENT_CHARS, &len);
			if (*d) {
				printf("Token: %s, Length: %d\n", d, len);
			}
		}
	}
	fclose(f);
	
	
	
	
	
	
	
/*
	i = parse_ini();
	// for (i=0; i<256; i++) {
		// printf("%d %s\n", i, position_data[i]);
	// }
	for (i=0; i<256; i++) {
		if (dirs = dir_lists[i]) {
			printf ("\n[%s]\n", position_data[i]);
			while (dirs) {
				printf ("%04o %4d %4d %s\n", dirs->mode, dirs->uid, dirs->gid, dirs->name);
				dirs = dirs->next;
			}
		}
	}
*/
}
