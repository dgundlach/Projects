#define IS_SCSI_DISK				1
#define IS_SCSI_PARTITION		2

#define UUID_SIZE					36
#define MAX_SCSI_HOSTS				256

typedef struct uuid_t{
	dev_t device;
	char uuid[UUID_SIZE + 1];
} uuid_t;

typedef struct scsi_host {
	dev_t device;
	int host_id;
	int unique_id;
	char block_name[MAX_SCSI_HOSTS];
	int partition_count;
	int partitions;
	char uuid[UUID_SIZE + 1][16];
} scsi_host;

int lookup_scsi_host(int);
int lookup_scsi_device_name(char *, int);
dev_t lookup_scsi_device_number(char *);
dev_t is_scsi_device(char *, int);
dev_t is_unpartitioned_scsi_disk(char *);
// int lookup_scsi_unique_id(char *);
GHashTable *get_device_uuids(GHashTable *);
dev_t get_dev_uuid(char **);
int get_scsi_host_id(char *);
int get_scsi_unique_id(int);
int get_scsi_block_name(char *, int);
int get_scsi_partitions(char *);
scsi_host **get_scsi_host_table(scsi_host **);
