enum ini_datatype {
	ini_no_type,
	ini_section_type,
	ini_parameter_type,
	ini_error_type
} ini_type;

typedef struct ini_t {
	enum ini_datatype linetype;
	char *lvalue;
	char *rvalue;
	int lcount;
	int rcount;
	char **lparts;
	char **rparts;
	int *llengths;
	int *rlengths;
	char *data;
	int datasize;
	int maxcount;
} ini_t;

char *ini_gettok(char *, char *, char *, int *);
void ini_split_free(ini_t *);
enum ini_datatype ini_split(ini_t *, char *, int, char *);
