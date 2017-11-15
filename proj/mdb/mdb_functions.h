#include "keypair.h"

typedef struct mdb_drive {
	char *mount;
	char *trash;
	char *alias;
} mdb_drive;

extern char *mdb_array;
extern char *mdb_data;
extern char *mdb_meta;
extern char *mdb_backup;
extern char *mdb_removed;
extern char *mdb_missing;
extern char *mdb_movielist;
extern char *mdb_masterlist;
extern char *mdb_conf;
extern char *mdb_movies;
extern char *mdb_remote;
extern keyPairList *mdb_dir_list;
extern keyPairList *mdb_backup_list;
extern keyPairList *mdb_movie_list;
extern int mdb_have_pv;
extern char *mdb_errorstr;

int mdb_symlink(const char *, int);
keyPair *mdb_new_drive(char *, char *);
keyPairList *mdb_load_drives(keyPairList *, char *, char *);
int mdb_load_conf(int);
int mdb_init(int);
keyPair *mdb_findspace(keyPairList *, size_t);
int mdb_calcsize(char *, char *, long *, long *);
keyPair *mdb_find(char *, int);
void mdb_loaddir(keyPairList *, char *, char *);
int mdb_createlinks(char *, char *);
int mdb_compare(const void *, const void *);
keyPairList *mdb_loadlist(char *);
int mdb_savelist(char *, keyPairList **, int);
int mdb_savemasterlist(void);
int mdb_xfermovie(char *, char *);

#define MDB_NO_BACKUP	0
#define MDB_BACKUP		1
