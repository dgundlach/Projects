#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wordexp.h>
#include <blkid/blkid.h>

#include "xalloc.h"
#include "fgettext.h"
#include "initokenize.h"
#include "iniparse.h"
#include "positiondata.h"

#define PATH_MAX 4096
#define SYS_BLOCK_HOST_PATH				"/sys/block/sd*/device/scsi_device*"
#define SYS_BLOCK_PARTITION_PATH		"/sys/block/sd*/sd*"
#define SYS_BLOCK_MULTI_FORMAT			"/sys/block/%s/%s"
#define SYS_CLASS_SCSI_UNIQUE_ID_FORMAT	"/sys/class/scsi_host/host%d/unique_id"
#define ETC_DISKARRAY_LABELS_PATH		"/etc/diskarray/labels.conf"

char *position_strings;
char *program;
directory_t *default_dirs = NULL;
directory_t *dir_lists[256];

typedef struct partition_t {
	int device;
	char label[24];
	char uuid[40];
} partition_t;

typedef struct scsi_device_t {
	char name[16];
	char *label;
	int device;
	int host_id;
	int unique_id;
	int partition_count;
	partition_t *partitions[16];
} scsi_device_t;

typedef struct scsi_device_array_t {
	int	first_device;
	int last_device;
	int device_count;
	int partition_count;
	scsi_device_t *devices[256];
	partition_t *partitions;
	scsi_device_t *devs;
} scsi_device_array_t;

void Usage(void) {
	printf("\nUsage: %s [-p] [-u]\n    -p: Only show partitioned devices.\n"
			"    -u: Only show unpartitioned devices.\n\n", program);
	exit(1);
}

int get_scsi_unique_id(int host_id) {

	char path[PATH_MAX];
	FILE *F;
	int unique_id = 0;

	snprintf(path, PATH_MAX, SYS_CLASS_SCSI_UNIQUE_ID_FORMAT, host_id);
	F = fopen(path, "r");
	fscanf(F, "%d", &unique_id);
	fclose(F);
	return unique_id;
}

int get_scsi_host_id(char *name) {

	char path[PATH_MAX];
	wordexp_t blocks;
	int host_id = -1;

	snprintf(path, PATH_MAX, "/sys/block/%s/device/scsi_device*", name);
	wordexp(path, &blocks, 0);
	if (strcmp(path, blocks.we_wordv[0])) {
		sscanf(blocks.we_wordv[0], "/sys/block/%*[a-z]/device/scsi_device:%d:", &host_id);
	}
	wordfree(&blocks);
	return host_id;
}

int get_scsi_device(char *name) {

	char path[PATH_MAX];
	FILE *F;
	int major, minor;

	snprintf(path, PATH_MAX, SYS_BLOCK_MULTI_FORMAT, name, "dev");
	F = fopen(path, "r");
	fscanf(F, "%d:%d", &major, &minor);
	fclose(F);
	return major << 8 + minor;		
}

int scsi_device_array_free(scsi_device_array_t *sda) {

	if (sda->devs) free(sda->devs);
	if (sda->partitions) free(sda->partitions);
	memset(sda, 0, sizeof(struct scsi_device_array_t));
}	

int get_scsi_device_array(scsi_device_array_t *sda, char **position_data) {

	wordexp_t blocks;
	wordexp_t parts;
	char **b, **p;
	int wordexp_flags = 0;
	void *data;
	scsi_device_t *sd;
	partition_t *pa;
	char *strings;
	int host_id;
	int unique_id;
	int x;

	static blkid_cache cache = NULL;
	blkid_dev_iterate iter;
	blkid_dev dev;
	char *uuid;
	char *devname;
	char *label;
	struct stat st;
	

	int i;
	char path[256];
	char buf[256];
	scsi_device_t *s_device;
	FILE *U;
	int partition_id;
	char *dnbuf;
	char *d;
	int namelen;
	int major,minor,device;

//
// Get lists of the scsi drives and the partitions on them.
//
	
	wordexp(SYS_BLOCK_HOST_PATH, &blocks, 0);
	b = blocks.we_wordv;
	wordexp(SYS_BLOCK_PARTITION_PATH, &parts, 0);
	p = parts.we_wordv;

//
// Allocate space for the data that we just fetched, along with some space
// for data that we're going to fetch.
//

	int drive_count = blocks.we_wordc;
	int part_count = parts.we_wordc;
	memset(sda, 0, sizeof(struct scsi_device_array_t));
	sda->devs = sd = xcalloc(drive_count + 1, sizeof(struct scsi_device_t));
	sda->partitions = pa = xcalloc(part_count + 1, sizeof(struct partition_t));
	sda->device_count = drive_count;
	sda->partition_count = part_count;
	sda->partitions = pa;
	sda->first_device = 256;

//
// Go through the data and store it.  It will be indexed by the unique_id.
//

	for (i=0; i<blocks.we_wordc; i++) {
		sscanf(b[i],"/sys/block/%4[a-z]/device/scsi_device:%d:", buf, &host_id);
		unique_id = get_scsi_unique_id(host_id);
		x = unique_id - 1;
		if (x < sda->first_device) sda->first_device = x;
		if (x > sda->last_device) sda->last_device = x;
		sda->devices[x] = sd;
		strncpy(sd->name, buf, 16);
		sd->device = get_scsi_device(sd->name);
		sd->host_id = host_id;
		sd->unique_id = unique_id;
		sd->label = position_data[x];
		sd++;
	}
	wordfree(&blocks);
	wordfree(&parts);

//
// Iterate through the blkid data and pull the partition infromation (UUID, LABEL)
// from it.
//

	if (!cache) {
		blkid_get_cache(&cache, NULL);
		blkid_probe_all_new(cache);
	}

	iter = blkid_dev_iterate_begin(cache);
	blkid_dev_set_search(iter, NULL, NULL);
	while (blkid_dev_next(iter, &dev) == 0) {
		dev = blkid_verify(cache, dev);
		if (!dev) continue;
		devname = (char *)blkid_dev_devname(dev);
		if ((sscanf(devname, "/dev/%4[a-z]%d", buf, &partition_id) == 2)
				&& ((host_id = get_scsi_host_id(buf)) != -1)) {
			unique_id = get_scsi_unique_id(host_id);
			uuid = blkid_get_tag_value(cache, "UUID", devname);
			label = blkid_get_tag_value(cache, "LABEL", devname);
			sd = sda->devices[unique_id - 1];
			sd->partition_count++;
			sd->partitions[partition_id] = pa;
			pa->device = sd->device + partition_id;
			strncpy(pa->uuid, uuid, 37);
			strncpy(pa->label, label, 17);
			free(uuid);
			free(label);
			pa++;
		}
		free(devname);
	}
	blkid_dev_iterate_end(iter);
	return 0;
}

int main (int argc, char **argv) {

	int show_partitioned = 1;
	int show_unpartitioned = 1;
	scsi_device_array_t sda;
	scsi_device_t **sd;
	partition_t *pa;
	int i;
	char path[256];
	char buf[256];
	int c;
	
	argv[0] = program = basename(argv[0]);
	while (1) {
		if ((c = getopt(argc, argv, "pu?h")) == -1) break;

		switch (c) {
			case 'p':
				if (!show_partitioned) {
					Usage();
				}
				show_unpartitioned = 0;
				break;
			case 'u':
				if (!show_unpartitioned) {
					Usage();
				}
				show_partitioned = 0;
				break;
			case 'h':
			case '?':
			default:
				Usage();
		}
	}
	if (optind < argc) {
		Usage();
	}
	parse_ini(position_data, &default_dirs, dir_lists);

	get_scsi_device_array(&sda, position_data);
	sd = sda.devices;
	for (i=0; i<256; i++) {
		if (sd[i]) {
			if (sd[i]->partition_count && show_partitioned) {
				printf("/dev/%s %s %d\n", sd[i]->name, sd[i]->label, sd[i]->partition_count);
			} else if (!sd[i]->partition_count && show_unpartitioned) {
				printf("/dev/%s %s %d\n", sd[i]->name, sd[i]->label, sd[i]->partition_count);
			}
		}
	}
	scsi_device_array_free(&sda);
	return 0;

}
