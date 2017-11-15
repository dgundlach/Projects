#include <stdlib.h>
#include <string.h>

#define ALLOC_SIZE		10

typedef struct SplitStr {
    int Count;
    int CAlloc;
    int SAlloc;
    char **String;
    size_t *Length;
    char *Data;
} SplitStr;

SplitStr *SplitReady(SplitStr *ssi, size_t dlen, size_t nstrings) {

    SplitStr *ss;
    char **s;
    char *d;
    size_t *l;
    int len;

    len = dlen + 1;
    ss = ssi;
    if (!ss) {
        if (!(ss = malloc(sizeof(struct SplitStr)))) {
            return NULL;
        }
        bzero(ss, sizeof(struct SplitStr));
    }
    if (ss->SAlloc < nstrings) {
        if (!(s = realloc(ss->String, nstrings * sizeof(char *)))) {
            if (ss->String) {
                free(ss->String);
                free(ss->Length);
            }
            if (ss->Data) {
                free(ss->Data);
            }
            free(ss);
            return NULL;
        }
        if (!(l = realloc(ss->Length, nstrings * sizeof(size_t)))) {
            if (ss->Length) {
                free(ss->Length);
            }
            if (ss->Data) {
                free(ss->Data);
            }
            free(ss->String);
            free(ss);
            return NULL;
        }
        ss->String = s;
        ss->Length = l;
        ss->SAlloc = nstrings;
    }
    if (ss->CAlloc < len) {
        if (!(ss->Data = malloc(len))) {
            if (ss->Data) {
                free(ss->Data);
            }
            free(ss->String);
            free(ss->Length);
            free(ss);
            return NULL;
        }
        ss->Data = d;
        ss->CAlloc = len;
    }
    return ss;
}

SplitStr *PSplit(SplitStr *ssi, char *str, int(*delim)(char)) {

    SplitStr *ss;
    size_t len;
    size_t slen;
    size_t offset;
    char *s1, *s2;
    char c;

    offset = 0;
    len = strlen(str);
    if (!(ss = SplitReady(ssi, len, ALLOC_SIZE))) {
        return NULL;
    }
    ss->String[offset] = ss->Data;
    s1 = str;
    s2 = ss->Data;
    slen = 0;
    while ((c = *s1++)) {
        if (delim(c)) {
            c = '\0';
            if (offset + 1 == ss->SAlloc && 
                    !(ss = SplitReady(ss, len, ss->SAlloc + ALLOC_SIZE))) {
                return NULL;
            }
            ss->Length[offset++] = slen;
            slen = 0;                
	    ss->String[offset] = s2 + 1;
	}
        if (c) {
            slen++;
        }
	*s2++ = c;
    }
    *s2 = '\0';
    ss->Length[offset++] = slen;
    if (offset == ss->SAlloc && 
            !(ss = SplitReady(ss, len, ss->SAlloc + ALLOC_SIZE))) {
        return NULL;
    }
    ss->String[offset] = NULL;
    ss->Length[offset] = 0;
    ss->Count = offset;
    return ss;
}

SplitStr *Split(SplitStr *ssi, char *str, int(*delim)(char)) {

    struct SplitStr *ss;
    size_t len;
    size_t slen;
    size_t offset;
    char *s1, *s2;
    char c;

    offset = 0;
    len = strlen(str);
    if (!(ss = SplitReady(ssi, len, ALLOC_SIZE))) {
        return NULL;
    }
    s1 = str;
    s2 = ss->Data;
    ss->String[offset] = NULL;
    slen = 0;
    while ((c = *s1++)) {
        if (delim(c)) {
            c = '\0';
            if (slen) {
                ss->Length[offset++] = slen;
                offset++;
                slen = 0;
            }
	}
        if (c) {
            slen++;
            if (slen == 1) {
                if (offset == ss->SAlloc && 
                        !(ss = SplitReady(ss, len, ss->SAlloc + ALLOC_SIZE))) {
                    return NULL;
                }                
                ss->String[offset] = s2;
            }
	}
	*s2++ = c;
    }
    *s2 = '\0';
    if (slen) {
        ss->Length[offset++] = slen;
        if (offset == ss->SAlloc && 
                !(ss = SplitReady(ss, len, ss->SAlloc + ALLOC_SIZE))) {
            return NULL;
        }                
	ss->String[offset] = NULL;
        ss->Length[offset] = 0;
    }
    ss->Count = offset;
    return ss;
}

int IsColon(int ch) {

    return (ch == ':') ? 1 : 0;
}

int IsWhiteSpace(int ch) {

    return (ch == ' ' || ch == '\t') ? 1 : 0;
}
