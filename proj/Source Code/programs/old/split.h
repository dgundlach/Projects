struct splitstr_t {
    int c;
    int a;
    char **s;
};

struct splitstr_t *split_ready (struct splitstr_t *, int);
struct splitstr_t *psplit (struct splitstr_t *, char *, char *);
struct splitstr_t *split (struct splitstr_t *, char *, char *);
