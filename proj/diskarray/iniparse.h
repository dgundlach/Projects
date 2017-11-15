//enum sections {
//	no_section,
//	device_section,
//	directory_section,
//	label_section,
//	error_section
//} section_e;

typedef struct directory_t {
	char *name;
	int uid;
	int gid;
	int mode;
	struct directory_t *next;
} directory_t;

//void report_ini(int, char *, char *);
//enum sections create_dir_entry(enum sections, ini_t *, directory_t **, int);
int parse_ini(char **, directory_t **, directory_t **);
