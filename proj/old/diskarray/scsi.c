#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAJOR_MASK 0xff00
#define MINOR_MASK 0x000f

dev_t scsi_majors[] = {0x0800, 0x4000, 0x4100, 0x4200, 0x4300, 0x4400, 0x4500, 0x4600,
                       0x8000, 0x8100, 0x8200, 0x8300, 0x8400, 0x8500, 0x8600, 0x8700, 0};

dev_t is_scsi_disk(char *path) {

	struct stat statbuf;
	int i = 0;
	dev_t device;

	if (stat(path, &statbuf)) return 0;
	while (device = scsi_majors[i++]) {
		if (S_ISBLK(statbuf.st_mode) && ((statbuf.st_rdev & MAJOR_MASK) == device)) {
			if (statbuf.st_rdev & MINOR_MASK) return 0;
			return statbuf.st_rdev;
		}
	}
	return 0;
}

dev_t is_unpartitioned_scsi_disk(char *path) {

	dev_t device;
	struct stat statbuf;
	char pathcopy[275];
	char *pathtail;
	int i;

	if (device = is_scsi_disk(path)) {
		pathtail = path + sprintf(pathcopy, "%s", path);
		for (i=1; i<16; i++) {
			sprintf(pathtail, "%d", i);
			if (!stat(path, &statbuf) && (statbuf.st_rdev & MINOR_MASK)) return 0;
		}
		return device;
	}
	return 0;
}

dev_t is_scsi_partition(char *path) {

	struct stat statbuf;
	int i = 0;
	dev_t device;

	if (stat(path, &statbuf)) return 0;
	while (device = scsi_majors[i++]) {
		if (S_ISBLK(statbuf.st_mode) && ((statbuf.st_rdev & MAJOR_MASK) == device)) {
			if (statbuf.st_rdev & MINOR_MASK) return 1;
			return 0;
		}
	}
	return 0;
}
