/* rsprintf.c */
int vrsprintf(char **str, size_t *size, size_t offset, const char *format, va_list args);
int rsprintf(char **str, size_t *size, size_t offset, const char *format, ...);
