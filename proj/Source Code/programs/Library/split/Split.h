typedef struct SplitStr {
    int Count;
    int CAlloc;
    int SAlloc;
    char **String;
    size_t *Length;
    char *Data;
} SplitStr;

SplitStr *SplitReady(SplitStr *, size_t, size_t);
SplitStr *PSplit(SplitStr *, char *, int (*)(char));
SplitStr *Split(SplitStr *, char *, int (*)(char));
int IsColon(int);
int IsWhiteSpace(int);
