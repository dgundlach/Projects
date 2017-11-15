struct dnsent_t {
    char *h;
    char *p;
    char *k;
    char *t;
};

struct dnsent_t *getdnsnam(char *);

static FILE *_dnspasswd = NULL;



struct dnsent_t *getdnsnam(char *host) {




    if (!_dnspasswd) {
	if (!(_dnspasswd = fopen(PASSWORD_FILE, "r") {
	    return NULL;
	}
    }
    fgets(buf, sizeof(buf), _dnspasswd);
    while (!feof(_dnspasswd)) {
	ss = psplit(ss, buf, ":");
	if (!strcasecmp(ss->s[0], host)) {
	    stored = ss->s[1];
	    keyfile = ss->s[2];
	    ttl = ss->s[3];
	    break;
	}
	fgets(buf, sizeof(buf), pf);
    }
    fclose(pf);
    if (!stored) {
        exit(-1);
    }

