#define SYS_BLOCK_PATH				"/sys/block/%s"
#define SYS_BLOCK_PARTITION_PATH	"/sys/block/%s/%s%d"
#define SYS_BLOCK_DEVICE_PATH		"/sys/block/%s/device"
#define SYS_CLASS_HOST_PATH			"/sys/class/scsi_host"
#define SYS_UNIQUE_ID_PATH			"/sys/class/scsi_host/host%d/unique_id"
#define SYS_BLOCK_NAME_PATH			"/sys/class/scsi_host/host%d/device/target%d:0:0/%d:0:0:0/"

#define GET_DISK_GEOMETRY_PATH		"/sbin/sfdisk -g /dev/%s|"

#define DEV_PATH					"/dev/"
#define ETC_BLOCK_ID_PATH			"/etc/blkid/blkid.tab"

#define APPLEVOLUMES_PATH			"/etc/netatalk/AppleVolumes.default"
#define SMBCONF_PATH				"/etc/samba/smb.conf.drv"
#define AUTODRV_PATH				"/etc/auto.drv";
#define TEMPDIR_PATH				"/etc/diskarray/mounts/mountXXXXXX"
