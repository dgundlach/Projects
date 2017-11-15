
typedef struct tstr {
	size_t alloc;
	size_t length;
	size_t trunc;
	char *str;
} tstr;

#define tstrcmp(s1, s2) strcmp(s1->str, s2->str)
#define tstrcmpc(s1, s2) strcmp(s1->str, s2)
#define tstrncmp(s1, s2, n) strncmp(s1->str, s2->str, n)
#define tstrncmpc(s1, s2, n) strncmp(s1->str, s2, n)
#define tstrcasecmp(s1, s2) strcasecmp(s1->str, s2->str)
#define tstrcasecmpc(s1, s2) strcasecmp(s1->str, s2)
#define tstrncasecmp(s1, s2, n) strncasecmp(s1->str, s2->str, n)
#define tstrncasecmpc(s1, s2, n) strncasecmp(s1->str, s2, n)

void tstrsetpadto(size_t);
tstr *tstrnew(tstr *, size_t);
void tstrdestroy(tstr **);
tstr *tstrdup(tstr *);
tstr *tstrdupc(char *);
int tstrcpy(tstr **, tstr *, size_t, size_t);
int tstrcpyc(tstr **, char *, size_t, size_t);
int tstrcpyn(tstr **, char *, size_t);
int tstrcat(tstr **, tstr *, size_t, size_t);
int tstrcatc(tstr **, char *, size_t, size_t);
int tstrcatn(tstr **, char *, size_t);
int tstrcat1(tstr **, char);
int tstrprintf(tstr **, const char *, ...);
int tstrcatprintf(tstr **, const char *, ...);
int tstrins(tstr **dest, tstr *src, size_t pos, size_t start, size_t length);
int tstrinsc(tstr **dest, char *src, size_t pos, size_t start, size_t length);
int tstrdel(tstr *, size_t, size_t);
int tstredit(tstr **, char *, char *, int);
int tstredit2(tstr **, char *, char *, int);
int tstredit3(tstr **, char *, char *, int, int []);
int tstredit4(tstr **, char *, char *, int, ...);
int tstrsettrunc(tstr *, size_t, int);
int tstrtrunc(tstr *);
int tstrunsettrunc(tstr *);
int tstrfill(tstr **, char *, size_t);
int tstrrev(tstr *, size_t, size_t);
int tstrextracttok(tstr **, char **, char *);
