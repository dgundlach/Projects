#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wordexp.h>
#include "fgettext.h"
#include "positiondata.h"

#define PATH_MAX 4096
#define SYS_BLOCK_HOST_PATH				"/sys/block/sd*/device/scsi_device*"
#define SYS_BLOCK_MULTI_FORMAT			"/sys/block/%s/%s"
#define SYS_CLASS_SCSI_UNIQUE_ID_FORMAT	"/sys/class/scsi_host/host%d/unique_id"
#define ETC_DISKARRAY_LABELS_PATH		"/etc/diskarray/labels.conf"

char *position_strings;
char *program;

typedef struct partition_t {
	int partition_id;
	char *label;
	char *uuid;
} partition_t;

typedef struct scsi_device_t {
	char *name;
	char *label;
	int device;
	int host_id;
	int unique_id;
	int partition_count;
	partition_t partition[16];
} scsi_device_t;

typedef struct scsi_device_array_t {
	int	first_device;
	int last_device;
	int device_count;
	scsi_device_t *device[256];
} scsi_device_array_t;

void Usage(void) {
	printf("\nUsage: %s [-p] [-u]\n    -p: Only show partitioned devices.\n"
			"    -u: Only show unpartitioned devices.\n\n", program);
	exit(1);
}

void get_position_data(void) {

	int len;
	char *buf = NULL;
	int alloc = 0;
	int unique_id;
	char *l;
	struct stat st;

	if (stat(ETC_DISKARRAY_LABELS_PATH, &st)) return;
	l = position_strings = malloc(st.st_size);

	while ((len = fgettext(&buf, &alloc, ETC_DISKARRAY_LABELS_PATH)) > 0) {
		if ((sscanf(buf, "%d : %13s", &unique_id, l) == 2)
				&& (unique_id >=1) && (unique_id <= 256)) {
			position_data[unique_id - 1] = l;
			l += 1 + strlen(l);
		}
	}
	free(buf);
	return;
}

int main (int argc, char **argv) {

	int show_partitioned = 1;
	int show_unpartitioned = 1;
	wordexp_t blocks;
	wordexp_t parts;
	int wordexp_flags = 0;
	char **b;
	int i;
	char path[256];
	char buf[256];
	scsi_device_t *s_device;
	FILE *U;
	int c;
	char *dnbuf;
	char *d;
	int namelen;
	int major,minor,device;
	
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
	get_position_data();
	wordexp(SYS_BLOCK_HOST_PATH, &blocks, 0);
	b = blocks.we_wordv;
	d = dnbuf = malloc((5 * blocks.we_wordc) + 256);
	s_device = malloc(blocks.we_wordc * sizeof(struct scsi_device_t));
	for (i=0; i<blocks.we_wordc; i++) {
		sscanf(b[i],"/sys/block/%[a-z]/device/scsi_device:%d:", d, &c);
		s_device[i].name = d;
		d += 5;
		s_device[i].host_id = c;
		snprintf(path, 256, SYS_CLASS_SCSI_UNIQUE_ID_FORMAT, s_device[i].host_id);
		U = fopen(path, "r");
		fscanf(U, "%d", &c);
		fclose(U);
		s_device[i].unique_id = c;
		s_device[i].label = position_data[c - 1];
		snprintf(path, 256, SYS_BLOCK_MULTI_FORMAT, s_device[i].name, "dev");
		U = fopen(path, "r");
		fscanf(U, "%d:%d", major, minor);
		fclose(U);
		s_device[i].device = major << 8 + minor;		
		snprintf(path, PATH_MAX, SYS_BLOCK_MULTI_FORMAT "*", s_device[i].name, s_device[i].name);
		wordexp(path, &parts, wordexp_flags);
		wordexp_flags = WRDE_REUSE;

// scanf string for parts: "/sys/block/%*[a-z]/%[a-z]%d"

		if (strcmp(parts.we_wordv[0], path)) {
			s_device[i].partition_count = parts.we_wordc;
		} else {
			s_device[i].partition_count = 0;
		}
	}
	for (i=0; i<blocks.we_wordc; i++) {
		if (s_device[i].partition_count && show_partitioned) {
			printf("/dev/%s %s %d\n", s_device[i].name, s_device[i].label, s_device[i].partition_count);
		} else if (!s_device[i].partition_count && show_unpartitioned) {
			printf("/dev/%s %s %d\n", s_device[i].name, s_device[i].label, s_device[i].partition_count);
		}
	}
	wordfree(&blocks);
	wordfree(&parts);
	free(position_strings);
	free (dnbuf);
	free (s_device);
	return 0;
}
