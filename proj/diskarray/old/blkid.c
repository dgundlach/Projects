#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <blkid/blkid.h>

int main(int argc, char **argv) {

	int i = 0;
	const char *val;
	char path[] = "/dev/sdk1";
	blkid_cache cache = NULL;
	struct stat st;
	char *devname;
	blkid_tag_iterate iter;
	blkid_dev dev;
	const char *type;

	if (cache == NULL)
		blkid_get_cache(&cache, NULL);

	blkid_probe_all_new(cache);

	if (stat(path, &st) != 0)
		return 0;
	printf("%x\n", st.st_rdev);
	devname = blkid_devno_to_devname(st.st_rdev);
	printf("devname = %s", devname);
	if (!devname)
		return 0;
	dev = blkid_get_dev(cache, devname, BLKID_DEV_NORMAL);
		free(devname);
	if (!dev)
		return 0;
	iter = blkid_tag_iterate_begin(dev);
	if (!iter)
		return 0;
	while (blkid_tag_next(iter, &type, &val) == 0)
		if (strcmp(type, "UUID") == 0)
			break;
	blkid_tag_iterate_end(iter);
	printf("uuid = %s\n", val);
/*	
	if (!type)
		return 0;
	} else {
		val = uuid;
	}
*/
}
