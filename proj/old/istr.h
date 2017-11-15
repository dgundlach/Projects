
typedef struct istr {
	size_t length;
	size_t alloc;
	size_t trunc;
} istr;

#define istrdata(src, offset) (char *)(src + sizeof(struct istr) + offset)
#define istrlen(src) src->length;

#define istrcmp(s1, s2) strcmp(istrdata(s1, 0), istrdata(s2, 0))
#define istrcmpc(s1, s2) strcmp(istrdata(s1, 0), s2)
#define istrncmp(s1, s2, n) strcmp(istrdata(s1, 0), istrdata(s2, 0), n)
#define istrncmpc(s1, s2, n) strcmp(istrdata(s1, 0), s2, n)
#define istrcasecmp(s1, s2) strcasecmp(istrdata(s1, 0), istrdata(s2, 0))
#define istrcasecmpc(s1, s2) strcasecmp(istrdata(s1, 0), s2)
#define istrncasecmp(s1, s2, n) strncasecmp(istrdata(s1, 0), istrdata(s2, 0), n)
#define istrncasecmpc(s1, s2, n) strncasecmp(istrdata(s1, 0), s2, n)

void istrsetpadto(size_t padto);
size_t istrlencalc(size_t slen, size_t start, size_t length);
istr *istrnew(istr *orig, size_t alloc);
istr *istrdestroy(istr *orig);
istr *istrdup(istr *orig);
istr *istrdupc(char *orig);
istr *istrcpy(istr *dest, istr *src, size_t start, size_t length);
istr *istrcpyc(istr *dest, char *src, size_t start, size_t length);
istr *istrcat(istr *dest, istr *src, size_t start, size_t length);
istr *istrcatc(istr *dest, char *src, size_t start, size_t length);
int istrsettrunc(istr *dest, size_t trunc, int now);
int istrtrunc(istr *dest);
istr *istrcat1(istr *dest, char c);

