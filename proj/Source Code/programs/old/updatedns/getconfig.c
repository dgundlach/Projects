#include <pwd.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include "split.h"
#define _COMPILING_GETCONFIG_
#include "getconfig.h"

int getconfig(struct param_t *parms, char *conf) {

    FILE *f = NULL;
    char buf[256];
    struct splitstr_t *ss = NULL;
    char *fn;
    int i;
    int found;
    struct passwd *pwd;

    if (!(f = fopen(conf, "r"))) {
	return GETCONFIG_ERR_NO_CONFIG;
    }
    for (i = 0; parms[i].n; i++) {
	if (parms[i].i) {
	    *(parms[i].d) = parms[i].i;
	}
    }
    if (!(ss = split_ready(ss, 128))) {
	return GETCONFIG_ERR_OUT_OF_MEMORY;
    }
    fgets(buf, sizeof(buf), f);
    while (!feof(f)) {
	if (*buf != '#' && *buf != ';') {
	    ss = split(ss, buf, " \t");
	    if (ss->c) {
		if (ss->c != 2) {
		    fclose(f);
		    free(ss);
		    return GETCONFIG_ERR_BAD_FORMAT;
		}
		found = 0;
		for (i = 0; parms[i].n; i++) {
		    if (!strcasecmp(ss->s[0], parms[i].n)) {
			*(parms[i].d) = strdup(ss->s[1]);
			found = 1;
			break;
		    }
		}
		if (!found) {
		    fclose(f);
		    free(ss);
		    return GETCONFIG_ERR_BAD_KEYWORD;
		}
	    }
	}
	fgets(buf, sizeof(buf), f);
    }
    fclose(f);
    free(ss);
    return GETCONFIG_OK;
}
