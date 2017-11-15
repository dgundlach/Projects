#include <stdio.h>
#include <signal.h>
#include <libgen.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>
#include <syslog.h>
#include <sys/inotify.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <pwd.h>

#include "scsi.h"
#include "limits.h"
#include "daemonize.h"
#include "readtextline.h"
#include "array_mon.h"
#include "paths.h"
#include "positiondata.h"

char *position_names[] = {
							"4-1", "3-1", "2-1", "1-1",
							"4-2", "3-2", "2-2", "1-2",
							"4-3", "3-3", "2-3", "1-3",
							"4-4", "3-4", "2-4", "1-4",
							"4-5", "3-5", "2-5", "1-5", NULL
						 };

char *service_names[] = {"autofs", "atalk", "smb", "linkmoviesd", NULL};
char *default_dirs[] = {"Files", "Movies", "Television", NULL};

char *owner = OWNER;
char *fdisk = FDISK_COMMAND;
char *fdisk_inst = FDISK_INSTRUCTIONS;
char *makefs = MAKEFS_COMMAND;
char *tunefs = TUNEFS_COMMAND;
char *fsoptions = FSOPTIONS;
char *applevolumes = APPLEVOLUMES_PATH;
char *smbconf = SMBCONF_PATH;
char *autodrv = AUTODRV_PATH;
char *dev_dir = DEV_PATH;
char *autodrv_dir = "/drv/";
char *program;
int min_unique_id = MIN_UNIQUE_ID;
int max_unique_id = MAX_UNIQUE_ID;
uid_t ouid = 0;
gid_t ogid = 0;

static int cmp_partitions(const void *p1, const void *p2) {
	scsi_partition * const *sp1 = p1;
	scsi_partition * const *sp2 = p2;

	return strncmp((*sp1)->data, (*sp2)->data, NAME_MAX);
}

int main(int argc, char **argv) {

	dev_t device;
	char *device_name;
	int host_id;
	int unique_id;
	pid_t pid;
	char path[PATH_MAX];
	char buf[PATH_MAX];
	FILE *F;
	FILE *APPLEVOLUMES;
	FILE *AUTODRV;
	FILE *SMBCONF;
	int i, j;
	int pos;
	char *colrow;
	char *uuid;
	char event_name[] = "sdl";
	char pf[16] = "\0";
	scsi_partition **partlist;
	scsi_partition *part;
	partition_table *parttable;

/*
 * See if the new device is an unpartitioned scsi disk, and that the disk
 * falls into our sphere of influence.  Fork a new process to handle it.
 */

	j = min_unique_id - 1;
	for (i=0; position_names[i]; i++) {
		position_data[j++] = position_names[i];
	}

	parttable = get_scsi_partition_table((void **)position_data);
	partlist = parttable->partitions;
	qsort(partlist, parttable->count, sizeof(scsi_partition *), &cmp_partitions);

//hexdump(parttable, parttable->alloc);
//exit(0);	

//	snprintf(path, PATH_MAX, "%s~", autodrv);
//	rename(autodrv, path);
//	if (!(AUTODRV = fopen(autodrv, "w"))) exit(-1);

	i = 0;
	while (i < parttable->count) {
		part = partlist[i++];
		if ((part->unique_id >= min_unique_id) && (part->unique_id <= max_unique_id)) {
			if (part->partition_count > 1) {
				snprintf(pf, 16, "-%d", part->device & PARTITION_MASK);
			} else {
				*pf = '\0';
			}
			//printf(APPLEVOLUMES_FORMAT, autodrv_dir, part->data, pf, part->data, pf);
			//printf(SMBCONF_FORMAT, part->data, pf, part->data, pf, autodrv_dir, part->data, pf, owner);
			if (*part->label) {
				printf(AUTODRV_FORMAT, part->data, pf, fsoptions, "LABEL", part->label);
			} else {
				printf(AUTODRV_FORMAT, part->data, pf, fsoptions, "UUID", part->uuid);
			}
		}
	}
//	fflush(AUTODRV);
//	fclose(AUTODRV);
	exit(1);
}
