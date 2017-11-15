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
	char uuid[40];  // Pad it out to a wordsize.  This should work for 32 and 64 bit systems.
} scsi_partition;

typedef struct partition_table {
	int count;
	int alloc;
	scsi_partition **partitions;
} partition_table;

typedef struct scsi_host {
	int device;
	int host_id;
	int unique_id;
	char block_name[NAME_MAX];
	int partition_count;
	int partitions;
	char *uuid[16];
} scsi_host;

typedef struct host_table {
	int host_count;
	int partition_count;
	int host_min;
	int host_max;
	char *uuidpool;
	scsi_host **scsi_host_array;
} host_table;

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

scsi_host **get_scsi_host_table(scsi_host **);
int get_disk_chs(char *, disk_chs_t *);
