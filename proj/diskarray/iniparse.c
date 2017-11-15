#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <alloca.h>
#include <ctype.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

#include "xalloc.h"
#include "initokenize.h"
#include "iniparse.h"

#define INI_FILE_NAME "/etc/diskarray/diskarray.conf"
#define IDENT_CHARS "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_-."
#define QUOTE_CHARS "\"'`"

enum sections {
	no_section,
	global_section,
	device_section,
	directory_section,
	label_section,
	error_section
} section_e;

void report_ini(int line, char *data, char *mess) {

	printf("%d: %s - %s\n", line, data, mess);
}

enum sections create_dir_entry(enum sections section, ini_t *ini_data, directory_t **dir_list, int line) {

	int len;
	int uid;
	int gid = -1;
	char *groupname;
	struct passwd *pwent;
	struct group *grent;
	int mode;
	int i;
	char *q;
	char **data = ini_data->lparts;
	int count = ini_data->lcount + ini_data->rcount;
	directory_t *direct_entry;

	len = ini_data->llengths[1];
	if ((i = strcspn(data[1], ".:")) < len) {
		groupname = data[1] + i;
		*groupname++ = '\0';
		if (grent = getgrnam(groupname)) {
			gid = grent->gr_gid;
		} else {
			report_ini(line, groupname, "Group name not found.");
			return error_section;
		}
	}
	if (pwent = getpwnam(data[1])) {
		uid = pwent->pw_uid;
		if (gid == -1) {
			gid = pwent->pw_gid;
		}
	} else {
		report_ini(line, data[1], "User name not found.");
		return error_section;
	}
	if ((count == 3) && ini_data->llengths[2]) {
		mode = strtol(data[2], &q, 8);
		if (*q || (mode < 0) || (mode > 0777)) {
			report_ini(line, data[2], "Syntax error in mode or mode out of range.");
			return error_section;
		}
	} else {
		mode = 0755;
	}
	len = ini_data->llengths[0] + 1;
	direct_entry = xmalloc(sizeof(struct directory_t) + len);
	direct_entry->name = (void *)direct_entry + sizeof(struct directory_t);
	strncpy(direct_entry->name, data[0], len);
	direct_entry->uid = uid;
	direct_entry->gid = gid;
	direct_entry->mode = mode;
	direct_entry->next = *dir_list;
	*dir_list = direct_entry;
	return section;
}

int parse_ini(char **position_data, directory_t **default_dirs, directory_t **dir_lists) {

	char **data;
	int count;
	char *buf;
	int buflen;
	size_t bufsize;
	enum sections section = no_section;
	enum ini_datatype linetype;
	int i;
	FILE *f;
	int unique_id;
	char *q;
	int len;
	int label_index;
	int line = 0;
	ini_t ini_data;
	char *s;

	memset(dir_lists, '\0', 256 * sizeof(struct directory_t *));
	memset(&ini_data, '\0', sizeof(ini_t));
	f = fopen(INI_FILE_NAME, "r");
	while ((buflen = getline(&buf, &bufsize, f)) != -1) {
		linetype = ini_split(&ini_data, buf, buflen, IDENT_CHARS);
		data = ini_data.lparts;
		line++;
		if (linetype == ini_error_type) {
			report_ini(line, buf, "Line will not parse.");
			break;
		}
		switch (linetype) {
			case ini_section_type:
				if (!strcasecmp(data[0], "global")) {
					section = global_section;
				} else if (!strcasecmp(data[0], "devices")) {
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
						report_ini(line, data[0], "Unrecognized section name.");
					}
				}
				break;
			case ini_parameter_type:
				switch (section) {
					case no_section:
						section = error_section;
						report_ini(line, buf, "We are not in a section.");
						break;
					case global_section:
						break;
					case device_section:
						len = ini_data.llengths[1];
						unique_id = strtol(data[0], &q, 10);
						if (!*q && (unique_id >= 1) && (unique_id <= 256)
								&& (len > 0) && (len < 14)) {
							s = xmalloc(len + 1);
							strcpy(s, data[1]);
							position_data[unique_id - 1] = s;
						} else {
							section = error_section;
							report_ini(line, data[1], "Unique id or label name out of range.");
						}
						break;
					case directory_section:
						section = create_dir_entry(section, &ini_data, &default_dirs[0], line);
						break;
					case label_section:
						section = create_dir_entry(section, &ini_data, &dir_lists[label_index], line);
						break;
				}
				break;
		}
		if (section == error_section) break;
	}
	fclose(f);
	ini_split_free(&ini_data);
	if (section = error_section) return 0;
	return 1;
}
