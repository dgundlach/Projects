#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#define _GNU_SOURCE
#include <string.h>
#include <ctype.h>
#include <blkid/blkid.h>

#include "FastBitCount.h"
#include "scsi.h"
#include "readtextline.h"
#include "limits.h"
#include "paths.h"
#include "split2.h"
#include "hexdump.h"

int scsi_majors[] =	{0x0800, 0x4000, 0x4100, 0x4200,
					 0x4300, 0x4400, 0x4500, 0x4600,
					 0x8000, 0x8100, 0x8200, 0x8300,
					 0x8400, 0x8500, 0x8600, 0x8700, 0};

//
// Derive the SCSI host from the device number.  This roughly corresponds to
// the SCSI host on which the drive is attached.  It matches perfectly if the
// drive has not just been hotplugged into the system.  Once we get a reboot,
// everything will match, though.
//

int lookup_scsi_host(int device) {

	int i = 0;
	int dev;
	int host = -1;

//
// Normalize the device number.  The major numbers for the scsi drives are all
// over the place, so se need to somehow make them contiguous.  The upper 4
// bits are calculated by multiplying the position of the matching major
// number in a table by 16.  The least significant 4 bits are contained in the
// upper nibble of the minor number, so divide the device number by 16 and
// mask off the significant 12 bits.  Then just add both parts together.
//

	while (dev = scsi_majors[i]) {
		if ((device & DEVICE_MAJOR_MASK) == dev) {
			host = (i << 4) + ((device >> 4) & PARTITION_MASK);
			break;
		}
		i++;
	}
	return host;
}

//
// Derive the scsi disk or partition name from the device number.
//

int lookup_scsi_device_name(char *name, int device) {

	int i;
	int remain;
	int num;
	int drive;
	int partition;
	char *n;
	int dev;

	*name = '\0';
	drive = lookup_scsi_host(device);
	partition = device & PARTITION_MASK;
	if (!name || (drive == -1)) return 0;
	*name++ = 's';					// Only the local copy of name pointer is changed.
	*name++ = 'd';
	n = name + 1;					// This is where we write the partition number.
	if (drive >= 26) n++;			// We need to write 2 characters for the name.

//
// Add the partition number on if needed.
//

	if (partition)  sprintf(n--, "%d", partition);
	else *n-- = '\0';

//
// Convert the drive number to a modified base 26 number.  Shift the drive number
// up 650 places (za), since we want the decimal value 26 to return the base 26
// value aa instead of ba.  We will be truncating the results by 1 character (most
// significant digit).  We also have to add the characters on from the tail end of
// the string.
//

	num = drive + 650;				// Shift the drive number up (26^2 - 26) places.
	do {
		remain = num % 26;
		num = num / 26;
		*n-- = remain + 'a';		// Put the characters on backwards
	} while (n >= name);
	return 1;
}

//
// Look up the device number for a scsi drive or partition on a scsi drive.
//

int lookup_scsi_device_number(char *name) {

	char *n = name;
	char c;
	int drive;
	int partition = 0;
	int device = 0;

//
// Parse the device name while calculating the device number.
//

	if (strstr(n, DEV_PATH)) n += 5;
	if (*n++ == 's') {
		if (*n++ = 'd') {
			c = *n++;

//
// Get the drive name and calculate the index we will use to determine the
// device number.
//

			if (islower(c)) {
				drive = (int)c - 'a';
				c = *n++;
				if (islower(c)) {
					drive = (drive + 1) * 26 + (int)c - 'a';
					c = *n++;
				}
//
// If there is a partition involved, we'll have to add that on to the
// device number;
//

				if (isdigit(c)) {
					partition = c - '0';
					c = *n++;
					if ((c >= '0') && (c <= '5')) {
						partition = partition * 10 + c - '0';
						c = *n;
					}
				}

//
// Each value in the lookup table references 16 drives, so we have to
// divide the device number by 16 to determine the index.  We then add the
// least significant nibble (multiplied by 16) of our drive number and the
// partition number, if any, to the fetched device number.
//

				device = scsi_majors[drive >> 4]
						+ ((drive & 0x0f) << 4) + partition;
			}
		}
	}
	return device;
}

int is_scsi_device(char *name, int which) {

	int device = 0;
	struct stat statbuf;

	if (strstr(name, DEV_PATH) == name) {
		if (!stat(name, &statbuf) && (lookup_scsi_host(statbuf.st_rdev) != -1)) {
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

int is_unpartitioned_scsi_disk(char *path) {

	int device = 0;
	struct stat statbuf;
	char syspath[PATH_MAX];
	char name[10];
	int i;

	if (!stat(path, &statbuf)) {
		if (lookup_scsi_device_name(name, statbuf.st_rdev)) {
			if (device = is_scsi_device(name, IS_SCSI_DISK)) {
				for (i=1; i<=MAX_SCSI_PARTITIONS; i++) {
					snprintf(syspath, PATH_MAX, SYS_BLOCK_PARTITION_PATH, name, name, i);
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

int get_scsi_unique_id(int host_id) {

	char path[PATH_MAX];
	char buf[64];

	snprintf(path, PATH_MAX, SYS_UNIQUE_ID_PATH, host_id);
	read_single_line_text_file(buf, sizeof(buf), path);
	return (int)strtol(buf, NULL, 10);
}

int get_scsi_host_id(char *name) {

	char path[PATH_MAX];
	char nam[NAME_MAX];
	char *nc = nam;
	char *n = name;
	char c;
	DIR *sysblock;
	struct dirent *entry;
	int host_id = -1;

	c = *n++;
	while (c && islower(c)) {
		*nc++ = c;
		c = *n++;
	}
	*nc = '\0';
	snprintf(path, PATH_MAX, SYS_BLOCK_DEVICE_PATH, nam);
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

int lookup_scsi_unique_id(char *name) {

	char path[PATH_MAX];
	DIR *sysblock;
	struct dirent *entry;
	long scsi_id;
	int unique_id = 0;
	int fd;

	snprintf(path, PATH_MAX, SYS_BLOCK_DEVICE_PATH, name);
	sysblock = opendir(path);
	while (entry = readdir(sysblock)) {
		if (strstr(entry->d_name, "scsi_device:") == entry->d_name) {
			scsi_id = strtol(entry->d_name + 12, NULL, 10);
			snprintf(path, PATH_MAX, SYS_UNIQUE_ID_PATH, (int)scsi_id);
			read_single_line_text_file(path, sizeof(path), path);
			unique_id = strtol(path, NULL, 10);
			break;
		}
	}
	closedir(sysblock);
	return unique_id;
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

int get_scsi_partitions(char *block) {

	struct stat statbuf;
	char syspath[PATH_MAX];
	int mask = 1;
	int partitions = 0;
	int i;
	
	for (i=1; i<=MAX_SCSI_PARTITIONS; i++) {
		snprintf(syspath, PATH_MAX, SYS_BLOCK_PARTITION_PATH, block, block, i);
		if (!stat(syspath, &statbuf)) {
			partitions += mask;
		}
		mask <<= 1;
	}
	return partitions;
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

#define DECODE_HEX(p) ((isxdigit(*p)) ? ((*p <= '9') ? *p++ - '0' : tolower(*p++) - 'a' + 10) : 0)

int get_dev_uuid(char **buf) {

	char *buffer = *buf;
	char *parts[64];
	int device = 0;
	char *uuid;
	char *devname;
	char *p;
	int count;
	int i;
	char tmpfile[30];

	while (1) {
		if (read_text_line(&buffer, ETC_BLOCK_ID_PATH) == READTEXTLINE_SUCCESS) {
			if (count = split(buffer, parts, 64, &blkid_punct)) {
				device = 0;
				uuid = NULL;
				devname = NULL;
				for (i=0; i<count; i++) {
					if (strstr(parts[i], "DEVNO=") == parts[i]) {
						p = parts[i] + 9;
						device = DECODE_HEX(p);
						device = device * 16 + DECODE_HEX(p);
						device = device * 16 + DECODE_HEX(p);
						device = device * 16 + DECODE_HEX(p);
					} else if ((strstr(parts[i], "UUID=")) == parts[i]) {
						uuid = parts[i] + 6;
						*(uuid + UUID_SIZE) = '\0';
					} else if ((strstr(parts[i], "/dev/")) == parts[i]) {
						devname = parts[i] + 5;
						p = devname;
						while (islower(*p)) {
							p++;
						}
						*p = '\0';
					}
				}
				if ((uuid != NULL) && (device != 0)) {
					snprintf(buffer, UUID_SIZE + 1, "%s", uuid);
					snprintf(buffer + UUID_SIZE + 1, 10, "%s", devname);
					break;
				} else {
					device = 0;
				}
			}
		} else {
			break;
		}
	}
	*buf = buffer;
	return device;
}
                                                                              
partition_table *get_scsi_partition_table(void **data) {

	int partition_count = 0;
	partition_table *ptbl;
	scsi_partition **tbl;
	scsi_partition *sp;
	int host_id;
	char *name;
	char partcounts[MAX_SCSI_HOSTS];
	int pn = 0;
	static blkid_cache cache = NULL;
	blkid_dev_iterate iter;
	blkid_dev dev;
	char *uuid;
	char *devname;
	struct stat st;

	if (!cache) {
		blkid_get_cache(&cache, NULL);
		blkid_probe_all_new(cache);
	}
	memset(partcounts, '\0', MAX_SCSI_HOSTS);
//
// First, let's do a little reconnisance.  See how many partitions we have, and how
// many partitions each scsi disk has.
//
	iter = blkid_dev_iterate_begin(cache);
	blkid_dev_set_search(iter, NULL, NULL);
	while (blkid_dev_next(iter, &dev) == 0) {
		dev = blkid_verify(cache, dev);
		if (!dev) continue;
		devname = (char *)blkid_dev_devname(dev);
		if (!stat(devname, &st)
				&& (lookup_scsi_host(st.st_rdev) != -1)
			    && (uuid = blkid_get_tag_value(cache, "UUID", devname))) {
			host_id = get_scsi_host_id(devname + 5);
			partcounts[host_id]++;
			partition_count++;
			free(devname);
			free(uuid);
		}
	}
	blkid_dev_iterate_end(iter);	
//
// Allocate some space for our array and data.
//
	int partition_array_alloc = (partition_count + 1) * sizeof(struct scsi_partition *);
	int alloc_size = partition_array_alloc + sizeof(struct partition_table)
				+ (partition_count * sizeof(struct scsi_partition));
	
	if (!(ptbl = malloc(alloc_size))) return NULL;
	memset(ptbl, '\0', alloc_size);
	tbl = (void *)ptbl + sizeof(struct partition_table);
	sp = (void *)tbl + partition_array_alloc;

	ptbl->count = partition_count;
	ptbl->alloc = alloc_size;
	ptbl->partitions = tbl;
//
// Loop through our uuid list once again, this time saving data to an array that
// asociates it with the unique id.
//
	blkid_get_cache(&cache, NULL);
	iter = blkid_dev_iterate_begin(cache);
	blkid_dev_set_search(iter, NULL, NULL);
	while (blkid_dev_next(iter, &dev) == 0) {
		dev = blkid_verify(cache, dev);
		if (!dev) continue;
		devname = (char *)blkid_dev_devname(dev);
		if (!stat(devname, &st)
				&& (lookup_scsi_host(st.st_rdev) != -1)
			    && (uuid = blkid_get_tag_value(cache, "UUID", devname))) {
			host_id = get_scsi_host_id(devname + 5);
			sp->device = st.st_rdev;
			sp->unique_id = get_scsi_unique_id(host_id);
			sp->partition_count = partcounts[host_id];
			if (data) sp->data = data[sp->unique_id - 1];
			strncpy(sp->uuid, uuid, UUID_SIZE + 1);
			free(devname);
			free(uuid);
			tbl[pn++] = sp;
			sp++;
		}
	}
	blkid_dev_iterate_end(iter);	
	return ptbl;
}
                                                                              
scsi_partition **get_scsi_partition_list(void **data) {

	char *uuid = NULL;
	int device;
	int partition_count = 0;
	void *alloc;
	scsi_partition **tbl;
	scsi_partition *sp;
	int host_id;
	char *name;
	char partcounts[MAX_SCSI_HOSTS];
	int pn = 0;
	int pc;
	
//
// First, let's do a little reconnisance.  See how many partitions we have.
//

	memset(partcounts, '\0', MAX_SCSI_HOSTS);
	while (device = get_dev_uuid(&uuid)) {
		if ((pc = lookup_scsi_host(device)) != -1) {
			name = uuid + UUID_SIZE + 1;
			host_id = get_scsi_host_id(name);
			partcounts[host_id]++;
			partition_count++;
		}
	}

//
// Allocate some space for our array and data.
//

	int partition_array_alloc = (partition_count + 1) * sizeof(struct scsi_partition *);
	int alloc_size = partition_array_alloc + (partition_count * sizeof(struct scsi_partition));
	
	if (!(tbl = malloc(alloc_size))) return NULL;
	memset(tbl, '\0', alloc_size);
	tbl[pn++] = (void *)partition_count;
	sp = (void *) tbl + partition_array_alloc;

//
// Loop through our uuid list once again, this time saving data to an array that
// asociates it with the unique id.
//

	uuid = NULL;
	while (device = get_dev_uuid(&uuid)) {
		if (lookup_scsi_host(device) != -1) {
			name = uuid + UUID_SIZE + 1;
			sp->device = device;
			host_id = get_scsi_host_id(name);
			sp->unique_id = get_scsi_unique_id(host_id);
			sp->partition_count = partcounts[host_id];
			strncpy(sp->uuid, uuid, UUID_SIZE + 1);
			if (data) sp->data = data[sp->unique_id - 1];
			tbl[pn++] = sp;
			sp++;
		}
	}
	return tbl;
}

int get_disk_chs(char *disk, disk_chs_t *chs) {

	struct stat st;
	char path[PATH_MAX];
	char buf[NAME_MAX];
	char *p;
	char *n;
	char *parts[16];
	int count;
	int rv = 0;
	int cylinders;
	int heads;
	int sectors;

	chs->device = 0;
	snprintf(path, PATH_MAX, GET_DISK_GEOMETRY_PATH, disk);
	if (read_single_line_text_file(buf, NAME_MAX, path)) {
		if (p = strchr(buf, ' ')) {
			cylinders = strtol(++p, &n, 10);
			if (n > p) {
				p = n + 12;
				heads = strtol(p, &n, 10);
				if (n > p) {
					p = n + 9;
					sectors = strtol(p, &n, 10);
					if (n > p) {
						snprintf(path, PATH_MAX, "%s%s", DEV_PATH, disk);
						if (!stat(path, &st)) {
							chs->device = st.st_rdev;
							strncpy(chs->block, disk, NAME_MAX);
							chs->cylinders = cylinders;
							chs->heads = heads;
							chs->sectors = sectors;
							rv = 1;
						}
					}
				}
			}
		}
	}
	return rv;
}

scsi_host **get_scsi_host_table(scsi_host **scsi_host_table) {

	char path[PATH_MAX];
	char name[NAME_MAX];
	char *uuid = NULL;
	char *uuidpool = NULL;
	char *u;
	DIR *hostdir;
	struct dirent *hostent;
	scsi_host **tbl = scsi_host_table;
	scsi_host *h;
	int device;
	int host_id;
	int partition_count = 1;
	int i, j;
	int mask;

	if (!tbl) {
		if (!(tbl = calloc(MAX_SCSI_HOSTS, sizeof(scsi_host *)))) return NULL;
	} else {

//
// Free the UUID pool so that it can be rebuilt.
//

		for (i=0; i<MAX_SCSI_HOSTS; i++) {
			if (tbl[i] && tbl[i]->uuid[0]) {
				free(tbl[i]->uuid[0]);
				break;
			}
		}
	}
	snprintf(path, PATH_MAX, SYS_CLASS_HOST_PATH);
	if (hostdir = opendir(path)) {
		while (hostent = readdir(hostdir)) {
			if (strstr(hostent->d_name, "host") == hostent->d_name) {
				host_id = (int)strtol(hostent->d_name + 4, NULL, 10);
				if (get_scsi_block_name(name, host_id)
						&& (device = lookup_scsi_device_number(name))
						&&(tbl[host_id] == NULL)) {
					h = calloc(1, sizeof(struct scsi_host));
					tbl[host_id] = h;
					h->device = device;
					h->host_id = host_id;
					strncpy(h->block_name, name, NAME_MAX);
					h->unique_id = get_scsi_unique_id(host_id);
					h->partitions = get_scsi_partitions(name);
					h->partition_count = BitCount(h->partitions);
					partition_count += h->partition_count;
					for (i=0; i<=PARTITION_MASK; i++) h->uuid[i] = NULL;
				}
			}
		}
		closedir(hostdir);
		uuidpool = calloc(partition_count, UUID_SIZE + 1);
		u = uuidpool;
		while (device = get_dev_uuid(&uuid)) {
			if (lookup_scsi_device_name(name, device & ~PARTITION_MASK)) {
				host_id = get_scsi_host_id(name);
				h = tbl[host_id];
				strncpy(u, uuid, UUID_SIZE + 1);
				h->uuid[0] = uuidpool;
				h->uuid[device & PARTITION_MASK] = u;
				u += UUID_SIZE + 1;
			}
		}
/* This uses the blkid libraries instead.
		for (i=0; i<MAX_SCSI_HOSTS; i++) {
			if (h = tbl[i]) {
				mask = 1;
				for (j=1; j<=MAX_SCSI_PARTITIONS; j++) {
					if (h->partitions & mask) {
						if (get_device_uuid(u, h->device + j)) {
							h->uuid[j] = u;
							u += UUID_SIZE + 1;
						}
					}
					mask <<= 1;
				}
				h->uuid[0] = uuidpool;
			}
		}
*/
	}
	return tbl;
}
