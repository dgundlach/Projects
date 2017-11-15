#define READTEXTLINE_EOF		-1

#define READTEXTLINE_SUCCESS	 1
#define READTEXTLINE_OOM	  -100

int read_text_line(char **, char *);
char *read_single_line_text_file(char *, size_t, char *);
