#define IS_SCSI_DISK			1
#define IS_SCSI_PARTITION	2

int lookup_scsi_device_name(char *, int);
dev_t lookup_scsi_number(char *);
dev_t sys_major_minor(char *);
dev_t is_scsi_disk(char *);
dev_t is_scsi_device(char *, int);
dev_t is_unpartitioned_scsi_disk(char *);
dev_t is_scsi_disk_sys(char *);
dev_t is_unpartitioned_scsi_disk_dev(char *);
dev_t is_scsi_partition(char *);


