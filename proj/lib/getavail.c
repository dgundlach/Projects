#include <sys/statfs.h>

unsigned long getavail(char *path) {

	struct statfs stfs;
	long blocks = 0;

	if (!statfs(path, &stfs)) {
		blocks = stfs.f_bavail * (stfs.f_bsize >> 9);
	}
	return blocks;
}
