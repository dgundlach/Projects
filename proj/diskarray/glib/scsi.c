#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#define _GNU_SOURCE
#include <string.h>
#include <ctype.h>
#include <glib.h>

#include "FastBitCount.h"
#include "scsi.h"
#include "readtextline.h"
#include "limits.h"
#include "paths.h"

dev_t scsi_majors[] =  {0x0800, 0x4000, 0x4100, 0x4200,
						0x4300, 0x4400, 0x4500, 0x4600,
						0x8000, 0x8100, 0x8200, 0x8300,
						0x8400, 0x8500, 0x8600, 0x8700, 0};

/*
 * Derive the SCSI host from the device number.  This roughly corresponds to
 * the SCSI host on which the drive is attached.  It matches perfectly if the
 * drive has not just been hotplugged into the system.  Once we get a reboot,
 * everything will match, though.
 */

int lookup_scsi_host(int device) {

	int i = 0;
	dev_t dev;
	int host = -1;

/*
 * Normalize the device number.  The major numbers for the scsi drives are all
 * over the place, so se need to somehow make them contiguous.  The upper 4
 * bits are calculated by multiplying the position of the matching major
 * number in a table by 16.  The least significant 4 bits are contained in the
 * upper nibble of the minor number, so divide the device number by 16 and
 * mask off the significant 12 bits.  Then just add both parts together.
 */

	while (dev = scsi_majors[i]) {
		if ((device & DEVICE_MAJOR_MASK) == dev) {
			host = (i << 4) + ((device >> 4) & PARTITION_MASK);
			break;
		}
		i++;
	}
	return host;
}

/*
 * Derive the scsi disk or partition name from the device number.
 */

int lookup_scsi_device_name(char *name, int device) {

	int i;
	int remain;
	int num;
	int drive;
	int partition;
	char *n;
	dev_t dev;

	drive = lookup_scsi_host(device);
	partition = device & PARTITION_MASK;
	if (!name || (drive == -1)) return 0;
	*name++ = 's';					// Only the local copy of name pointer is changed.
	*name++ = 'd';
	n = name + 1;					// This is where we write the partition number.
	if (drive >= 26) n++;			// We need to write 2 characters for the name.

/*
 * Add the partition number on if needed.
 */

	if (partition)  sprintf(n--, "%d", partition);
	else *n-- = '\0';

/*
 * Convert the drive number to a modified base 26 number.  Shift the drive number
 * up 650 places (za), since we want the decimal value 26 to return the base 26
 * value aa instead of ba.  We will be truncating the results by 1 character (most
 * significant digit).  We also have to add the characters on from the tail end of
 * the string.
 */

	num = drive + 650;				// Shift the drive number up (26^2 - 26) places.
	do {
		remain = num % 26;
		num = num / 26;
		*n-- = remain + 'a';		// Put the characters on backwards
	} while (n >= name);
	return 1;
}

/*
 * Look up the device number for a scsi drive or partition on a scsi drive.
 */

dev_t lookup_scsi_device_number(char *name) {

	char *n = name;
	char c;
	int drive;
	int partition = 0;
	dev_t device = 0;

/*
 * Parse the device name while calculating the device number.
 */

	if (strstr(n, DEV_PATH)) n += 5;
	if (*n++ == 's') {
		if (*n++ = 'd') {
			c = *n++;

/*
 * Get the drive name and calculate the index we will use to determine the
 * device number.
 */

			if (islower(c)) {
				drive = (int)c - 'a';
				c = *n++;
				if (islower(c)) {
					drive = (drive + 1) * 26 + (int)c - 'a';
					c = *n++;
				}
/*
 * If there is a partition involved, we'll have to add that on to the
 * device number;
 */

				if (isdigit(c)) {
					partition = c - '0';
					c = *n++;
					if ((c >= '0') && (c <= '5')) {
						partition = partition * 10 + c - '0';
						c = *n;
					}
				}

/*
 * Each value in the lookup table references 16 drives, so we have to
 * divide the device number by 16 to determine the index.  We then add the
 * least significant nibble (multiplied by 16) of our drive number and the
 * partition number, if any, to the fetched device number.
 */

				if ((c == '\0') || isspace(c) || ispunct(c)) {
					device = scsi_majors[drive >> 4]
								+ ((drive & 0x0f) << 4) + partition;
				}
			}
		}
	}
	return device;
}

dev_t is_scsi_device(char *name, int which) {

	dev_t device = 0;
	struct stat statbuf;

	if (strstr(name, DEV_PATH) == name) {
		if (!stat(name, &statbuf)) {
			device = statbuf.st_rdev;
		}
	} else {
		device = lookup_scsi_device_number(name);
	}
	switch (which & 0x03) {
		case IS_SCSI_DISK:
			if (device & PARTITION_MASK) device = 0;
			break;
		case IS_SCSI_PARTITION:
			if (!(device & PARTITION_MASK)) device = 0;
	}
	return device;
}

dev_t is_unpartitioned_scsi_disk(char *path) {

	dev_t device = 0;
	struct stat statbuf;
	char syspath[275];
	char name[10];
	int i;

	if (!stat(path, &statbuf)) {
		if (lookup_scsi_device_name(name, statbuf.st_rdev)) {
			if (device = is_scsi_device(name, IS_SCSI_DISK)) {
				for (i=1; i<16; i++) {
					sprintf(syspath, SYS_BLOCK_PARTITION_PATH, name, name, i);
					if (!stat(syspath, &statbuf)) {
						device = 0;
						break;
					}
				}
			}
		}
	}
	return device;
}
/*
int lookup_scsi_unique_id(char *name) {

	char path[275];
	DIR *sysblock;
	struct dirent *entry;
	long scsi_id;
	int unique_id = 0;
	int fd;

	sprintf(path, SYS_BLOCK_DEVICE_PATH, name);
	sysblock = opendir(path);
	while (entry = readdir(sysblock)) {
		if (strstr(entry->d_name, "scsi_device:") == entry->d_name) {
			scsi_id = strtol(entry->d_name + 12, NULL, 10);
			sprintf(path, SYS_UNIQUE_ID_PATH, (int)scsi_id);
			read_single_line_text_file(path, sizeof(path), path);
			unique_id = strtol(path, NULL, 10);
			break;
		}
	}
	closedir(sysblock);
	return unique_id;
}
*/
gboolean uuid_equal(gconstpointer a, gconstpointer b) {

	if (((uuid_t*)a)->device == ((uuid_t*)b)->device) return TRUE;
	return FALSE;
}

guint uuid_hash(gconstpointer v) {

	return g_int_hash(&((uuid_t*)v)->device)
	;
}

void uuid_entry_destroy(gpointer d) {

	if (d) free(d);
}

int blkid_punct(int c) {

	switch (c) {
		case '<':
		case '>':
		case ' ':
		case '\r':
		case '\n':
			return 1;
		default:
			return 0;
	}
}

#define DECODE_HEX(p) (isxdigit(*p)) ? ((*p >= 'a') ? *p++ - 'a' : *p++ - '0') : 0

GHashTable *get_device_uuids(GHashTable *uuid_table) {

	char *buf = NULL;
	char *parts[64];
	uuid_t *u;
	dev_t device;
	char *uuid;
	char *p;
	int count;
	int i;
	int j;

	if (!uuid_table) uuid_table = g_hash_table_new(&uuid_hash, &uuid_equal);
	while (read_text_line(&buf, ETC_BLOCK_ID_PATH) == READTEXTLINE_SUCCESS) {
		if (count = split(buf, parts, &blkid_punct)) {
			device = 0;
			uuid = NULL;
			for (i=0; i<count; i++) {
				if (strstr(parts[i], "DEVNO=") == parts[i]) {
					p = parts[i] + 9;
					device = DECODE_HEX(p);
					device = device * 16 + DECODE_HEX(p);
					device = device * 16 + DECODE_HEX(p);
					device = device * 16 + DECODE_HEX(p);
					if (uuid) break;
				} else if (strstr(parts[i], "UUID=") == parts[i]) {
					uuid = parts[i] + 6;
					*(uuid + 36) = '\0';
					if (device) break;
				}
			}
			if (uuid && device) {
				if (!g_hash_table_lookup(uuid_table, &device)) {
					u = malloc(sizeof(struct uuid_t));
					u->device = device;
					strncpy(u->uuid, uuid, UUID_SIZE + 1);
					g_hash_table_insert(uuid_table, &u->device, u);
				}
			}
		}
	}
	return uuid_table;
}

dev_t get_dev_uuid(char **buf) {

	char *parts[64];
	dev_t device = 0;
	char *uuid;
	char *p;
	int count;
	int i;

	while (1) {
		if (read_text_line(buf, ETC_BLOCK_ID_PATH) == READTEXTLINE_SUCCESS) {
			if (count = split(*buf, parts, &blkid_punct)) {
				device = 0;
				uuid = NULL;
				for (i=0; i<count; i++) {
					if (strstr(parts[i], "DEVNO=") == parts[i]) {
						p = parts[i] + 9;
						device = DECODE_HEX(p);
						device = device * 16 + DECODE_HEX(p);
						device = device * 16 + DECODE_HEX(p);
						device = device * 16 + DECODE_HEX(p);
						if (uuid) break;
					} else if (strstr(parts[i], "UUID=") == parts[i]) {
						uuid = parts[i] + 6;
						*(uuid + UUID_SIZE) = '\0';
						if (device) break;
					}
				}
				if (uuid && device) {
					strcpy(*buf, uuid);
				} else {
					device = 0;
					*buf = '\0';
				}
			}
		}
	}
	return device;
}

int get_scsi_unique_id(int host_id) {

	char path[PATH_MAX];
	char buf[64];

	snprintf(path, PATH_MAX, SYS_UNIQUE_ID_PATH, host_id);
	read_single_line_text_file(buf, sizeof(buf), path);
	return (int)strtol(buf, NULL, 10);
}

int get_scsi_host_id(char *name) {

	char path[PATH_MAX];
	DIR *sysblock;
	struct dirent *entry;
	int host_id = -1;

	sprintf(path, SYS_BLOCK_DEVICE_PATH, name);
	if (sysblock = opendir(path)) {
		while (entry = readdir(sysblock)) {
			if (strstr(entry->d_name, "scsi_device:") == entry->d_name) {
				host_id = strtol(entry->d_name + 12, NULL, 10);
				break;
			}
		}
		closedir(sysblock);
	}
	return host_id;
}

int get_scsi_block_name(char *name, int host_id) {

	char path[PATH_MAX];
	DIR *targetdir;
	struct dirent *targetent;
	int rv = 0;

	snprintf(path, PATH_MAX, SYS_BLOCK_NAME_PATH, host_id, host_id, host_id);
	if (targetdir = opendir(path)) {
		while (targetent = readdir(targetdir)) {
			if (strstr(targetent->d_name, "block:") == targetent->d_name) {
				strncpy(name, targetent->d_name + 6, NAME_MAX);
				rv = 1;
				break;
			}
		}
		closedir(targetdir);
	}
	return rv;
}

gboolean scsi_host_equal(gconstpointer a, gconstpointer b) {

	if (((scsi_host*)a)->device == ((scsi_host*)b)->device) return TRUE;
	return FALSE;
}

guint scsi_host_hash(gconstpointer v) {

	return g_int_hash(&((scsi_host*)v)->device);
}

int get_scsi_partitions(char *block) {

	struct stat statbuf;
	char syspath[PATH_MAX];
	int mask = 1;
	int partitions = 0;
	int i;
	
	for (i=1; i<16; i++) {
		sprintf(syspath, SYS_BLOCK_PARTITION_PATH, block, block, i);
		if (!stat(syspath, &statbuf)) {
			partitions += mask;
		}
		mask <<= 1;
	}
	return partitions;
}

scsi_host **get_scsi_host_table(scsi_host **scsi_host_table) {

	char path[PATH_MAX];
	char name[NAME_MAX];
	char *uuid = NULL;
	char *p = path;
	DIR *hostdir;
	struct dirent *hostent;
	scsi_host *tbl = NULL;
	scsi_host *h;
	uuid_t *u;
	int mask;
	int i, j;
	int device;
	int host_id;
	int partitions;

	if (!scsi_host_table) scsi_host_table = calloc(MAX_SCSI_HOSTS, sizeof(scsi_host *));
	p += snprintf(path, PATH_MAX, SYS_CLASS_HOST_PATH);
	if (hostdir = opendir(path)) {
		while (hostent = readdir(hostdir)) {
			if (strstr(hostent->d_name, "host") == hostent->d_name) {
				host_id = (int)strtol(hostent->d_name + 4, NULL, 10);
				get_scsi_block_name(name, host_id);
				if ((device = lookup_scsi_device_number(name))) {
					partitions = get_scsi_partitions(name);
					if (scsi_host_table[host_id] == NULL) {
						h = malloc(sizeof(struct scsi_host));
						h->device = device;
						h->host_id = host_id;
						strncpy(h->block_name, name, NAME_MAX);
						h->unique_id = get_scsi_unique_id(host_id);
						h->partitions = partitions;
						h->partition_count = BitCount(partitions);
						scsi_host_table[host_id] = h;
					}
				}
			}
		}
		closedir(hostdir);
		while (device = get_dev_uuid(&uuid)) {
			if (lookup_scsi_device_name(name, device ^ PARTITION_MASK)) {
				host_id = get_scsi_host_id(name);
				h = scsi_host_table[host_id];
				strncpy(h->uuid[device & PARTITION_MASK], uuid, UUID_SIZE + 1);
			}
		}
	}
	


	return scsi_host_table;
}
/*
scsi_host **get_scsi_host_table_unused(scsi_host **scsi_host_table, GHashTable **uuid_table) {

	char path[PATH_MAX];
	char name[NAME_MAX];
	char *p = path;
	DIR *hostdir;
	struct dirent *hostent;
	scsi_host *tbl = NULL;
	scsi_host *h;
	uuid_t *u;
	GHashTable *uuid_tbl = *uuid_table;
	int mask;
	int i, j;
	int device;
	int host_id;
	int partitions;

	if (!uuid_table) return NULL;
	if (!scsi_host_table) scsi_host_table = calloc(MAX_SCSI_HOSTS , sizeof(scsi_host *));
	uuid_tbl = get_device_uuids(*uuid_table);
	if (!*uuid_table) {
		*uuid_table = uuid_tbl;
	}
	p += snprintf(path, PATH_MAX, SYS_CLASS_HOST_PATH);
	if (hostdir = opendir(path)) {
		while (hostent = readdir(hostdir)) {
			if (strstr(hostent->d_name, "host") == hostent->d_name) {
				host_id = (int)strtol(hostent->d_name + 4, NULL, 10);
				get_scsi_block_name(name, host_id);
				if ((device = lookup_scsi_device_number(name))) {
					partitions = get_scsi_partitions(name);
					if (scsi_host_table[host_id] == NULL) {
						h = malloc(sizeof(struct scsi_host));
						h->device = device;
						h->host_id = host_id;
						strncpy(h->block_name, name, NAME_MAX);
						h->unique_id = get_scsi_unique_id(host_id);
						h->partitions = partitions;
						h->partition_count = BitCount(partitions);
						j = 0;
						mask = 1;
						for (i=1; i<16; i++) {
							if (mask & h->partitions) {
								device = h->device + i;
								if (u = g_hash_table_lookup(uuid_tbl, &device)) {
									h->uuid[j++] = u->uuid;
								}
							}
							mask <<= 1;
						}
						scsi_host_table[host_id] = h;
					} else {
						h = scsi_host_table[host_id];
						if (h->partitions != partitions) {
							h->partitions = partitions;
							h->partition_count = BitCount(partitions);
							j = 0;
							mask = 1;
							for (i=1; i<16; i++) {
								if (mask & h->partitions)
								device = h->device + i;
								if (u = g_hash_table_lookup(uuid_tbl, &device)) {
									h->uuid[j++] = u->uuid;
								}
								mask <<= 1;
							}
							for (; j<16; j++) {
								if (h->uuid[j]) {
									h->uuid[j] = NULL;
								} else {
									break;
								}
							}
						}
					}
				}
			}
		}
		closedir(hostdir);
	}
	return scsi_host_table;
}
*/
