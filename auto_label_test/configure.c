#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <mntent.h>
#include <blkid/blkid.h>
#include <pwd.h>
#include <grp.h>
#include "configure.h"

char *fixOptions(char *options) {

	char *newOptions;
	char *p;
	char *tmp;
	char *token;
	int firstOpt = 1;

	tmp = strdup(options);
	newOptions = malloc(strlen(options) + 2);
	p = newOptions;
	*p = '\0';
	token = strtok(tmp, ",");

	/*
	 * Remove "ro" and "rw" from the options.
	 */

	while (token != NULL) {
		if (strcmp(token, "ro") && strcmp(token, "rw")) {
				if (firstOpt) {
					firstOpt = 0;
					p += sprintf(p, "%s", token);
				} else {
					p += sprintf(p, ",%s", token);
				}
		}
		token = strtok(NULL, ",");
	}
	free(tmp);
	return newOptions;
}

mount_opts *addMountDef(mount_opts *mountOpts, char *dir, char *peerDir, char *fsType, char *options) {

	mount_opts *newOpts;

	/*
	 * Create the list if it doesn't exist or prepend it if the entry key is list than
	 * the current key.
	 */

	if ((mountOpts == NULL) || (strcmp(dir, mountOpts->dir) < 0)) {
		newOpts = malloc(sizeof(struct mount_opts));
		newOpts->dir = strdup(dir);
		newOpts->peerDir = strdup(peerDir);
		newOpts->fsType = strdup(fsType);
		newOpts->wd = -1;
		if (!strcmp(peerDir, "*")) {
			newOpts->options = strdup(options);
		} else {
			newOpts->options = fixOptions(options);
		}
		return newOpts;

	/*
	 * The new entry key is greater than the current one, so add it after this one.
	 */

	} else if (strcmp(dir, mountOpts->dir) > 0) {
		mountOpts->next = addMountDef(mountOpts->next, dir, peerDir, fsType, options);

	/*
	 * The keys are equal, so just amend the contents.
	 */

	} else {
		free(mountOpts->peerDir);
		free(mountOpts->fsType);
		free(mountOpts->options);
		mountOpts->peerDir = strdup(peerDir);
		mountOpts->fsType = strdup(fsType);
		if (!strcmp(peerDir, "*")) {
			mountOpts->options = strdup(options);
		} else {
			mountOpts->options = fixOptions(options);
		}
	}
	return mountOpts;
}

int setOwnerOpts(owner_opts *ownerOpts, char *dir, char *owner, char *group, char *mode) {

	struct passwd *pwd;
	struct group *grp;

	if (ownerOpts == NULL) {
		return 1;
	}
	if ((pwd = getpwnam(owner))) {
		ownerOpts->uid = pwd->pw_uid;
		ownerOpts->gid = pwd->pw_gid;
		if (group) {
			if ((grp = getgrnam(group))) {
				ownerOpts->gid = grp->gr_gid;
			} else {
				return 3;
			}
		}
	} else {
		return 2;
	}
	ownerOpts->mode = 0755;
	if (mode & (*mode != '\0')) {
		ownerOpts->mode = (mode_t) strtol(mode, NULL, 8);
	}
	ownerOpts->dir = strdup(dir);
	return 0;
}

owner_opts *addOwnerOpts(owner_opts *ownerOpts, char *dir, char *owner, char *group, char *mode) {

	owner_opts *newOpts;

	/*
	 * Create the list if it doesn't exist or prepend it if the entry key is list than
	 * the current key.
	 */

	if ((ownerOpts == NULL) || (strcmp(dir, ownerOpts->dir) < 0)) {
		newOpts = malloc(sizeof(struct owner_opts));
		if (!setOwnerOpts(newOpts, dir, owner, group, mode)) {
			newOpts->next = ownerOpts;
			return newOpts;
		} else {
			free(newOpts);
			return NULL;
		}

    /*
     * The new entry key is greater than the current one, so add it after this one.
     */

    } else if (strcmp(dir, ownerOpts->dir) > 0) {
        ownerOpts->next = addOwnerOpts(ownerOpts->next, dir, owner, group, mode);

    /*
     * The keys are equal, so just amend the contents.
     */

    } else {
		free(ownerOpts->dir);
		ownerOpts->dir = NULL;
		setOwnerOpts(ownerOpts, dir, owner, group, mode);
    }
    return ownerOpts;

}

fs_opts *addFSDefault(fs_opts *fsOpts, char *fsType, char *mountType, char *options) {

	fs_opts *newOpts;

	/*
	 * Create the list if it doesn't exist or prepend it if the entry key is list than
	 * the current key.
	 */

	if ((fsOpts == NULL) || (strcmp(fsType, fsOpts->fsType) < 0)) {
		newOpts = malloc(sizeof(struct fs_opts));
		newOpts->fsType = strdup(fsType);
		newOpts->mountType = strdup(mountType);
		newOpts->options = strdup(options);
		newOpts->next = fsOpts;
		return newOpts;

	/*
	 * The new entry key is greater than the current one, so add it after this one.
	 */

	} else if (strcmp(fsType, fsOpts->fsType) > 0) {
		fsOpts->next = addFSDefault(fsOpts->next, fsType, mountType, options);

	/*
	 * The keys are equal, so just amend the contents.
	 */

	} else {
		free(fsOpts->mountType);
		free(fsOpts->options);
		fsOpts->mountType = strdup(mountType);
		fsOpts->options = strdup(options);
	}
	return fsOpts;
}

int configure(fs_opts **fsOpts, owner_opts **ownerOpts, mount_opts **mountOpts) {

	FILE *conf;
	struct mntent *ent;
	char *owner;
	char *group;

	/*
	 * Set up some default filesystem type entries.  These can be overridden.
	 */

	*fsOpts = addFSDefault(*fsOpts, "ext2", "ext4", "defaults");
	*fsOpts = addFSDefault(*fsOpts, "ext3", "ext4", "defaults");
	*fsOpts = addFSDefault(*fsOpts, "ext4", "ext4", "defaults");
	*fsopts = addFSDefault(*fsOpts, "hfsplus", "ufsd", "defaults");
	*fsOpts = addFSDefault(*fsOpts, "ntfs", "ufsd", "defaults");
	*fsOpts = addFSDefault(*fsOpts, "vfat", "vfat", "defaults");

	/*
	 * Open the configuration file.  If it doesn't exist, skip the rest of the configuration.
	 */

	if (!(conf = setmntent(CONF_FILE, "r"))) return 1;
	while ((ent = getmntent(conf))) {
		if (!strcmp(ent->mnt_fsname, FS_OPTS)) {
			*fsOpts = addFSDefault(*fsOpts, ent->mnt_dir, ent->mnt_type, ent->mnt_opts);
		} else if (!strcmp(ent->mnt_fsname, OWNER_OPTS)) {
			owner = strdup(ent->mnt_type);
			if ((group = strchr(owner, ':'))) {
				*group++ = '\0';
			}
			free(owner);
			*ownerOpts = addOwnerOpts(*ownerOpts, ent->mnt_dir, owner, group, ent->mnt_opts);
		} else {
			*mountOpts = addMountDef(*mountOpts, ent->mnt_fsname, ent->mnt_dir, ent->mnt_type, ent->mnt_opts);
		}
	}
	endmntent(conf);
	return 0;
}
