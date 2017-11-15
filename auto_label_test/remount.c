#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <mntent.h>
#include <sys/mount.h>
#include <libgen.h>
#include <string.h>
#include <blkid/blkid.h>

#include "configure.h"

#define MTAB		"/etc/mtab"
#define TEMP_MTAB	"/etc/mtab~"
#define PROC_MOUNTS	"/proc/mounts"

int remountToggle (char *mountPoint, char *dir) {

	char *mountName;
	FILE *oldMtab, *newMtab;
	struct mntent *ent;
	int rc = 0;
	int mntOpts = MS_MGC_VAL|MS_REMOUNT;
	char *rf;
	char *options = NULL;
	char *mtabOptions = NULL;

	if ((mountName = malloc(strlen(dir) + NAME_MAX + 2)) == NULL) return -2;
	sprintf(mountName, "%s/%s", dir, mountPoint);

	/*
	 * Open the old and new mtab files.
	 */

	oldMtab = setmntent(MTAB, "r");
	newMtab = setmntent(TEMP_MTAB, "w");
	while ((ent = getmntent(oldMtab))) {

		/*
		 * If the drive is mounted, toggle the write status.
		 */

		if (!strcmp(ent->mnt_dir, mountName)) {
			if (hasmntopt(ent, MNTOPT_RW)) {
				mntOpts |= MS_RDONLY;
				rf = "ro";
			} else {
				rf = "rw";
			}

			/*
			 * Remount the drive.
			 */

			rc = mount(ent->mnt_fsname, ent->mnt_dir, NULL, mntOpts, NULL);

			/*
			 * Update the mtab entry.  Save the old options pointer so that it
			 * can be restored after writing the entry.
			 */

			strcpy(mtabOptions, ent->mnt_opts);
			options = fixOptions(ent->mnt_opts);
			if (*options) {
				sprintf(mtabOptions, "%s,%s", rf, options);
			} else {
				sprintf(mtabOptions, "%s", rf);
			}
			free(options);
			options = ent->mnt_opts;
			ent->mnt_opts = mtabOptions;
			addmntent(newMtab, ent);
			ent->mnt_opts = options;
		} else {

			/*
			 * No match, so just copy the entry to the new file.
			 */

			addmntent(newMtab, ent);
		}
	}
	endmntent(oldMtab);
	endmntent(newMtab);

	/*
	 * Change over to the new mtab if we remounted anything.  Otherwise, remove the new file.
	 */

	if (mtabOptions) {
		rename(TEMP_MTAB, MTAB);
		free(mtabOptions);
	} else {
		unlink(TEMP_MTAB);
	}
	free(mountName);
	return rc;
}

int mounted(char *device) {

	FILE *mounts;
	struct mntent *ent;
	int rc = 0;

	mounts = setmntent(PROC_MOUNTS, "r");
	while ((ent = getmntent(mounts))) {
		if (!strcmp(ent->mnt_fsname, device)) {
			rc = 1;
			break;
		}
	}
	endmntent(mounts);
	return rc;
}
