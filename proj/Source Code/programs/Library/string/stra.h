typedef struct {
    char *s;
    size_t a;
    size_t l;
} StrA;

#define StrA_length(sa)        sa->l
/* stra.c */
int StrA_ready(StrA *str, size_t size);
StrA *StrA_new(size_t size);
void StrA_destroy(StrA *str);
int StrA_catc(StrA *str, char c);
int StrA_copyc(StrA *str, char c);
int StrA_dupc(StrA *str, size_t count);
int StrA_0(StrA *str);
int StrA_copyb(StrA *str, unsigned char *buf, size_t bsize);
int StrA_catb(StrA *str, unsigned char *buf, size_t bsize);
int StrA_copy(StrA *dest, StrA *src);
int StrA_cat(StrA *dest, StrA *src);
int StrA_copye(StrA *dest, StrA *src, size_t (*escape)(char *, const char *, size_t));
int StrA_cate(StrA *dest, StrA *src, size_t (*escape)(char *, const char *, size_t));
int StrA_copys(StrA *dest, char *src);
int StrA_cats(StrA *dest, char *src);
int StrA_copyse(StrA *dest, char *src, size_t (*escape)(char *, const char *, size_t));
int StrA_catse(StrA *dest, char *src, size_t (*escape)(char *, const char *, size_t));
int StrA_ncopys(StrA *dest, char *src, size_t ssiz);
int StrA_ncats(StrA *dest, char *src, size_t ssiz);
int StrA_ncopyse(StrA *dest, char *src, size_t ssiz, size_t (*escape)(char *, const char *, size_t));
int StrA_ncatse(StrA *dest, char *src, size_t ssiz, size_t (*escape)(char *, const char *, size_t));
int StrA_vprintf(StrA *str, const char *format, va_list args);
int StrA_printf(StrA *str, const char *format, ...);
int StrA_vcatprintf(StrA *str, const char *format, va_list args);
int StrA_catprintf(StrA *str, const char *format, ...);
int StrA_truncate(StrA *str, size_t size);
int StrA_cmp(StrA *str1, StrA *str2, size_t len);
int StrA_casecmp(StrA *str1, StrA *str2, size_t len);
char *StrA_index(StrA *str, char c);
char *StrA_rindex(StrA *str, char c);
