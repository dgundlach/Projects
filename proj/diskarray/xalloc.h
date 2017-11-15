void xalloc_fatal(char *);
extern void (*xalloc_fatal_call)(char *);
extern char *xalloc_fatal_message;
void *xmalloc(size_t);
void *xcalloc(size_t, size_t);
void *xrealloc(void *, size_t);
