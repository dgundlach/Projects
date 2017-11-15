#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#define _GNU_SOURCE
#include <string.h>
#include <ctype.h>

#include "sys-scsi.h"

#define MAJOR_MASK 0xff00
#define PARTITION_MASK 0x000f

dev_t scsi_majors[] = {0x0800, 0x4000, 0x4100, 0x4200, 0x4300, 0x4400, 0x4500, 0x4600,
                       0x8000, 0x8100, 0x8200, 0x8300, 0x8400, 0x8500, 0x8600, 0x8700, 0};

/*
 * Derive the scsi disk or partition name from the device number.
 */

int lookup_scsi_device_name(char *name, int device) {

   int i = 0;
   int remain;
   int num;
   int drive = -1;
   int partition = device & PARTITION_MASK;
   char *n = name;
   dev_t dev;

/*
 * Normalize the device number.  The major numbers for the scsi drives are all
 * over the place, so se need to somehow make them contiguous.  The upper 4
 * bits are calculated by multiplying the position of the matching major
 * number in a table by 16.  The least significant 4 bits are contained in the
 * upper nibble of the minor number, so divide the device number by 16 and
 * mask off the significant 12 bits.  Then just add both parts together.
 */

	while (dev = scsi_majors[i]) {
		if ((device & MAJOR_MASK) == dev) {
			drive = (i << 4) + ((device >> 4) & PARTITION_MASK);
			break;
		}
		i++;
	}
   if (!name || (drive == -1)) return 0;

/*
 * Derive the scsi disk name from the major device number.  We're basically
 * converting the number to base 26 (all letters, no digits).  We have to
 * shift the  major number up and just pull 1 or 2 digits since the name is not
 * a real base 26 number.  The value "aa" would be the decimal equivalent of
 * "0" when it has to correspond to "26".  The shift moves the base up to the
 * base 26 number "za", which corresponds to the decimal number "650" (26 * 25).
 */

   *n++ = 's';
   *n++ = 'd';
   if (drive < 26) {
   	*n++ = drive + 'a'; 				// 'a' .. 'z'
   } else {
      i = 2;
      num = drive + 650;
		while (i) {
		   remain = num % 26;
		   num = num / 26;
		   *(n + --i) = remain + 'a';	// Put the characters on backwards
		}
		n += 2;
   }

/*
 * Add the partition number on if needed.
 */

   if (partition) sprintf(n, "%d", partition);
   else *n = '\0';
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

/*
 * Return the device number if the device is a SCSI disk.  A SCSI partition
 * is not a SCSI disk.
 */

dev_t is_scsi_disk(char *name) {

	dev_t device;

	if (device = lookup_scsi_device_number(name)) {
		if (device & PARTITION_MASK) {
			device = 0;
		}
	}
	return device;
}

dev_t is_scsi_device(char *name, int which) {

	dev_t device;

	if (device = lookup_scsi_device_number(name)) {
		switch (which & 0x03) {
			case IS_SCSI_DISK:
				if (device & PARTITION_MASK) device = 0;
				break;
			case IS_SCSI_PARTITION:
				if (!(device & PARTITION_MASK)) device = 0;
		}
	}
	return device;
}

dev_t is_unpartitioned_scsi_disk(char *path) {

	dev_t device = 0;
	struct stat statbuf;
	char syspath[275];
	char *pathtail = syspath;
	char name[10];
	int i;

	if (!stat(path, &statbuf)) {
		if (lookup_scsi_device_name(name, statbuf.st_rdev)) {
			if (device = is_scsi_device(name, IS_SCSI_DISK)) {
				pathtail += sprintf("/sys/block/%s/%s", name, name);
				for (i=1; i<16; i++) {
					if (!stat(path, &statbuf)) {
						device = 0;
						break;
					}
				}
			}
		}
	}
	return device;
}

dev_t sys_major_minor(char *path) {

	FILE *devfile;
	char buf[256];
   unsigned long mm;
   char *eptr;

	snprintf(buf, sizeof(buf), "%s/dev", path);
	devfile = fopen(buf, "r");
	fread(buf, 1, sizeof(buf), devfile);
	fclose(devfile);
	mm = strtoul(buf, &eptr, 10);
	eptr++;
	mm = mm * 256 + strtoul(eptr, NULL, 10);
	return (dev_t)mm;
}

dev_t is_scsi_disk_sys(char *path) {

	dev_t device;
	dev_t sm;
	int i = 0;

	device = sys_major_minor(path);
	while (sm = scsi_majors[i++]) {
	   if ((device & MAJOR_MASK) == sm) return device;
	}
	return 0;		
}

dev_t is_unpartitioned_scsi_disk_dev(char *path) {

	dev_t device;
	struct stat statbuf;
	char pathcopy[275];
	char *pathtail;
	int i;

	if (device = is_scsi_disk(path)) {
		pathtail = path + sprintf(pathcopy, "%s", path);
		for (i=1; i<16; i++) {
			sprintf(pathtail, "%d", i);
			if (!stat(path, &statbuf) && (statbuf.st_rdev & PARTITION_MASK))
				return 0;
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
		if (S_ISBLK(statbuf.st_mode)
				&& ((statbuf.st_rdev & MAJOR_MASK) == device)) {
			if (statbuf.st_rdev & PARTITION_MASK) return 1;
			return 0;
		}
	}
	return 0;
}
