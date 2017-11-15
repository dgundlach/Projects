#define IS_SCSI_DISK				1
#define IS_SCSI_PARTITION			2

#define UUID_SIZE					36
#define MAX_SCSI_HOSTS				256
#define	MAX_SCSI_PARTITIONS			15

#define DEVICE_MAJOR_MASK		0xff00
#define DEVICE_MINOR_MASK		0x00ff
#define PARTITION_MASK			0x000f

typedef struct disk_chs_t {
	int device;
	char block[NAME_MAX];
	int cylinders;
	int heads;
	int sectors;
} disk_chs_t;

typedef struct scsi_partition {
	int device;
	int unique_id;
	int partition_count;
	void *data;
	char uuid[UUID_SIZE + 1];
	char label[17];
	char pad[10];
} scsi_partition;

typedef struct partition_table {
	int count;
	int alloc;
	scsi_partition **partitions;
} partition_table;

typedef struct scsi_uuid_t {
	int device;
	char *devname;
	char *uuid;
	char *label;
	int host_id;
	struct scsi_uuid_t *next;
} scsi_uuid_t;

int lookup_scsi_host(int);
int lookup_scsi_device_name(char *, int);
int lookup_scsi_device_number(char *);
int is_scsi_device(char *, int);
int is_unpartitioned_scsi_disk(char *);
int lookup_scsi_unique_id(char *);
int get_dev_uuid(char **);
int get_scsi_host_id(char *);
int get_scsi_unique_id(int);
int get_scsi_block_name(char *, int);
int get_scsi_partitions(char *);
int get_device_uuid(char **);

partition_table *get_scsi_partition_table(void **data);
int get_disk_chs(char *, disk_chs_t *);
