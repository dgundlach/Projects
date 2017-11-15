#define EVENT_SIZE		(sizeof(struct inotify_event))
#define BUF_LEN			(1024 * (EVENT_SIZE + 16))
#define UDEV_JITTER		5

#define OWNER "dan"
#define FDISK_INSTRUCTIONS "n\np\n1\n\n\nw\n";
#define FDISK_COMMAND  "/sbin/fdisk /dev/%s 2>&1 >/dev/null"
#define MAKEFS_COMMAND "/sbin/mke2fs -j -m0 /dev/%s%i 2>&1 >/dev/null"
#define TUNEFS_COMMAND "/sbin/tune2fs -c0 /dev/%s%i 2>&1 >/dev/null"
#define FSOPTIONS "-fstype=ext3,noatime"

#define MIN_UNIQUE_ID	3
#define MAX_UNIQUE_ID	22

#define SMBCONF_FORMAT "\n[%s]\n\tcomment = Drive %s\n\tpath = %s%s\n\tbrowseable = yes\n\twriteable = yes\n\tvalid users = %s\n"
#define APPLEVOLUMES_FORMAT "%s%s\t\t\"drv%s\"\n"
#define AUTODRV_FORMAT "%s%s\t%s\t:UUID=\"%s\"\n"
