#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <mntent.h>
#include <sys/mount.h>
#include <libgen.h>
#include <string.h>

#include "configure.h"

#define MTAB		"/etc/mtab"
#define TEMP_MTAB	"/etc/mtab~"
#define PROC_MOUNTS	"/proc/mounts"

int remountToggle (char *mountPoint, char *roPath) {

	char *mountName;
	FILE *oldMtab, *newMtab;
	struct mntent *ent;
	int rc = 0;
	int changed = 0;
	int mntOpts = MS_MGC_VAL|MS_REMOUNT;
	char optsStr[16];

	if ((mountName = malloc(strlen(roPath) + NAME_MAX + 2)) == NULL) return -2;
	snprintf(mountName, NAME_MAX_PLUS, "%s/%s", roPath, mountPoint);
	oldMtab = setmntent(MTAB, "r");
	newMtab = setmntent(TEMP_MTAB, "w");
	while (ent = getmntent(oldMtab)) {
		if (!strcmp(ent->mnt_dir, mountName)) {
			optsStr[0] = 'r';
			if hasmntopt(ent, MNTOPT_RW)) {
				mntOpts |= MNT_RDONLY;
				optsStr[1] = 'o';
			} else {
				optsStr[1] = 'w';
			}
			if (hasmntopt(ent,"noatime")) {
				mntOpts |= MS_NOATIME;
				sprintf(optsStr[2], ",noatime");
			}
			rc = mount(ent->mnt_fsname, ent->mnt_dir, NULL, mntOpts, NULL);
			ent->mnt_opts = optsStr;
			addmntent(newMtab, ent);
			changed = 1;
		} else {
			addmntent(newMtab, ent);
		}
	}
	endmntent(oldMtab);
	endmntent(newMtab);
	if (changed) {
		rename(TEMP_MTAB, MTAB);
	} else {
		unlink(TEMP_MTAB);
	}
	free mountName;
	return rc;
}

int mounted(char *device) {

	FILE *mounts;
	struct mntent *ent;
	int rc = 0;

	mounts = setmntent(PROC_MOUNTS, "r");
	while (ent = getmntent(mounts)) {
		if (!strcmp(ent->mnt_fsname, device)) {
			rc = 1;
			break;
		}
	}
	endmntent(mounts);
	return rc;
}
