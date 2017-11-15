#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <gdbm.h>
#include <stdlib.h>
#include <stdio.h>

int builddb(char *dbname, char sep) {

    char *dbn;
    char *dbt;
    GDBM_FILE dbf;
    FILE *f;
    int l;
    datum key;
    datum content;
    char buf[4096];
    char *s;
    int code;

    l = strlen(dbname);
    dbn = malloc(l + 4);
    dbt = malloc(l + 5);
    snprintf(dbn, l + 4, "%s.db", dbname);
    snprintf(dbt, l + 5, "%s.tmp", dbname);
    content.dptr = buf;
    key.dptr = buf;
    f = fopen(dbname, "r");
    dbf = gdbm_open(dbt, 4096, GDBM_NEWDB, 0666, NULL);
    fgets(buf, sizeof(buf), f);
    while (!feof(f)) {
	content.dsize = strlen(content.dptr);
        if (content.dptr[content.dsize - 1] == '\n') {
	    s = buf;
            key.dsize = 0;
	    while (*s) {
		if (*s == sep) {
		    break;
		}
		key.dsize++;
		s++;
	    }
	    code = gdbm_store(dbf, key, content, GDBM_INSERT);
	} else {
	    printf("Line too long.  Discarding...\n%s\n", buf);
	}
	fgets(buf, sizeof(buf), f);
    }
    gdbm_close(dbf);
    fclose(f);
    rename(dbt, dbn);
    free(dbn);
    free(dbt);
    return 0;
}



int main(int argc, char **argv) {

    umask(066);
    builddb("passwd", ':');
    umask(022);
    exit(0);
}
