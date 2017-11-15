/* rstrcpy.c */
int str_ready(char **str, size_t *oldsize, size_t newsize, int pad);
int rstrncpy(char **dest, size_t *dsize, size_t offset, const char *src, size_t ssize);
int rstrcpy(char **dest, size_t *dsize, size_t offset, const char *src);
int rstrncpye(char **dest, size_t *dsize, size_t offset, const char *src, size_t ssize, size_t (*escape)(char *, const char *, size_t));
int rstrcpye(char **dest, size_t *dsize, size_t offset, const char *src, size_t (*escape)(char *, const char *, size_t));
