#define EVENT_SIZE		(sizeof(struct inotify_event))
#define BUF_LEN			(1024 * (EVENT_SIZE + 16))
#define UDEV_JITTER		5

#define OWNER "dan"
#define FDISK_INSTRUCTIONS ",,L\n";
#define FDISK_COMMAND	"/sbin/sfdisk -q -O /etc/diskarray/backup/%d /dev/%s"
#define MAKEFS_COMMAND	"/sbin/mke2fs -j -m0 -L \"%s\" /dev/%s%d"
#define TUNEFS_COMMAND	"/sbin/tune2fs -c0 /dev/%s%d"
#define BLKID_COMMAND	"/sbin/blkid -c /dev/null -w /etc/blkid/blkid.tab"
#define FSOPTIONS "-fstype=ext3,noatime"

#define MIN_UNIQUE_ID	3
#define MAX_UNIQUE_ID	22

#define SMBCONF_FORMAT "\n[%s%s]\n\tcomment = Drive %s%s\n\tpath = %s%s%s\n\tbrowseable = yes\n\twriteable = yes\n\tvalid users = %s\n"
#define APPLEVOLUMES_FORMAT "%s%s%s\t\t\"drv%s%s\"\n"
#define AUTODRV_FORMAT "%s%s\t%s\t:%s=\"%s\"\n"
