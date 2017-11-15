#define _GNU_SOURCE
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <alloca.h>
#include "tstr.h"
#include "xalloc.h"

size_t __tstrallocpadto = 256;

/*
 * Skip whitespace in a string; if we hit a comment delimiter, consider the rest of the
 * entry a comment.
 */

char *__skipspace(char *whence, char *cdelim) {

	while (1) {
		switch (*whence) {
			case ' ':
			case '\b':
			case '\t':
			case '\n':
			case '\v':
			case '\f':
			case '\r':
				whence++;
				break;
			default:
				if (strchr(cdelim, *whence)) {
					while (*whence) {
						whence++;
					}
				}
				return whence;
		}
	}
}

/*
 * Check whether the next character is whitespace (or equivalent).
 */

int __isspace(char *whence, char *cdelim) {

	switch (*whence) {
		case ' ':
		case '\b':
		case '\t':
		case '\n':
		case '\v':
		case '\f':
		case '\r':
			return 1;
		default:
			if (strchr(cdelim, *whence)) {
				return 1;
			}
			return 0;
	}
}

/*
 * Copy a string (or a part of one) from one location to another.  The
 * length has been precalculated, so we don't need to do any testing
 * of the length.
 */

void __tstrncpy(char *dest, char *src, size_t length) {

	if (dest && src) {
		while (length--) {
			*dest++ = *src++;
		}
	}
}

/*
 * Copy a string (or a part of one) from one location to another.  This
 * routine copies from the back end, so as to handle overlaps where we are
 * opening a hole to put another string in.  The length has been precalculated,
 * so we don't need to do any testing of the length.
 */

void __tstrncpyb(char *dest, char *src, size_t length) {

	if (dest && src) {
		dest += length - 1;
		src += length - 1;
		while (length--) {
			*dest-- = *src--;
		}
	}
}

/*
 * Calculate the length of a tstr string (or a substring of it).
 */

size_t __tstrlencalc(tstr *src, size_t start, size_t length) {

	size_t len = 0;

	if (src && start--) {
		if (src->length > start) {
			len = src->length - start;
			len = (length && (len > length)) ? length : len;
		}
	}
	return len;
}

/*
 * Calculate the length of a c string (or a substring of it).
 */

size_t __strlencalc(char *str, size_t start, size_t length) {

	size_t len = 0;

	if (start--) {
		len = strlen(str);
		if (len > start) {
			len -= start;
			len = (length && (len > length)) ? length : len;
		} else {
			len = 0;
		}
	}
	return len;
}

/*
 * Set the internal allocation size.  We don't want to export a symbol to outside programs, so use
 * a private variable that's set using a function.
 */

void tstrsetpadto(size_t padto) {

	__tstrallocpadto = padto;
}

/*
 * Create a new tstr structure, or reallocate one if we don't have enough room.
 */

tstr *tstrnew(tstr *src, size_t alloc) {

	tstr *new = src;
	int reallynew = (src == NULL);
	size_t allocsize;
	
	allocsize = ((size_t)((sizeof(struct tstr) + alloc)
		/ __tstrallocpadto) + 1) * __tstrallocpadto;
	if ((!reallynew) && (allocsize <= new->alloc)) {
		return new;
	}
	new = xrealloc(new, allocsize);
	new->alloc = allocsize;
	new->str = (char *)new + sizeof(struct tstr);
	if (reallynew) {
		new->length = 0;
		new->trunc = 0;
		*new->str = '\0';
	}
	return new;
}

/*
 * Destroy a tstr structure.
 */

void tstrdestroy(tstr **src) {

	if (*src) {
		free(*src);
		*src = NULL;
	}
}

/*
 * Duplicate a tstr.
 */

tstr *tstrdup(tstr *src) {

	tstr *new;

	if (src && (new = tstrnew(NULL, src->alloc - sizeof(struct tstr) - 1))) {
		strcpy(new->str, src->str);
		new->length = src->length;
		new->trunc = src->trunc;
	}
	return new;
}

/*
 * Duplicate a c string into a tstr.
 */

tstr *tstrdupc(char *src) {

	tstr *new;
	size_t len;

   if (!src) return NULL;
	len = strlen(src);
	if (new = tstrnew(NULL, len)) {
		strcpy(new->str, src);
		new->length = len;
	}
	return new;
}

/*
 * Copy a tstr (or a substring of one) into another tstr.
 */

int tstrcpy(tstr **dest, tstr *src, size_t start, size_t length) {

	tstr *new = *dest;
	size_t len;

	if (src && (len = __tstrlencalc(src, start, length))
			&& (new = tstrnew(new, len))) {
		__tstrncpy(new->str, src->str + start - 1, len);
		*(new->str + len) = '\0';
		new->length = len;
		if (len + 1 > new->trunc) new->trunc = 0;
		*dest = new;
		return 1;
	}
	return 0;
}

/*
 * Copy a c string (or a substring of one) into a tstr.
 */

int tstrcpyc(tstr **dest, char *src, size_t start, size_t length) {

	tstr *new = *dest;
	size_t len;

	if (src && (len = __strlencalc(src, start, length))
			&& (new = tstrnew(new, len))) {
		__tstrncpy(new->str, src + start - 1, len);
		*(new->str + len) = '\0';
		new->length = len;
		if (len + 1 > new->trunc) new->trunc = 0;
		*dest = new;
		return 1;
	}
	return 0;
}

/*
 * Copy a c string (or a substring of one) into a tstr.  The length has
 * been precalculated, so no testing is done here.
 */

int tstrcpyn(tstr **dest, char *src, size_t length) {

	tstr *new = *dest;

	if (src && (new = tstrnew(new, length))) {
		__tstrncpy(new->str, src, length);
		*(new->str + length) = '\0';
		new->length = length;
		if (length + 1 > new->trunc) new->trunc = 0;
		*dest = new;
		return 1;
	}
	return 0;
}

/*
 * Concatenate a tstr (or a substring of one) to the end (or truncation point)
 * of a tstr.
 */

int tstrcat(tstr **dest, tstr *src, size_t start, size_t length) {

	tstr *new = *dest;
	size_t len;
	size_t alloc;
	size_t pos;

	if (src && (len = __tstrlencalc(src, start, length))) {
		if (new) {
			pos = (new->trunc) ? new->trunc - 1 : new->length;
			alloc = len + new->length;
		} else {
			pos = 0;
			alloc = len;
		}
		if (new = tstrnew(new, alloc)) {
			__tstrncpy(new->str + pos, src->str + start - 1, len);
			new->length = pos + len;
			*(new->str + new->length) = '\0';
			*dest = new;
			return 1;
		}
	}
	return 0;
}

/*
 * Concatenate a c string (or a substring of one) to the end (or truncation point)
 * of a tstr.
 */

int tstrcatc(tstr **dest, char *src, size_t start, size_t length) {

	tstr *new = *dest;
	size_t len;
	size_t alloc;
	size_t pos;

	if (src && (len = __strlencalc(src, start, length))) {
		if (new) {
			pos = (new->trunc) ? new->trunc - 1 : new->length;
			alloc = len + new->length;
		} else {
			pos = 0;
			alloc = len;
		}
		if (new = tstrnew(new, alloc)) {
			__tstrncpy(new->str + pos, src + start - 1, len);
			new->length = pos + len;
			*(new->str + new->length) = '\0';
			*dest = new;
			return 1;
		}
	}
	return 0;
}

/*
 * Concatenate a c string (or a substring of one) to the end (or truncation point)
 * of a tstr.  The length has been precalculated, so no testing is done here.
 */

int tstrcatn(tstr **dest, char *src, size_t length) {

	tstr *new = *dest;
	size_t alloc;
	size_t pos;

	if (src) {
		if (new) {
			pos = (new->trunc) ? new->trunc - 1 : new->length;
			alloc = length + new->length;
		} else {
			pos = 0;
			alloc = length;
		}
		if (new = tstrnew(new, alloc)) {
			__tstrncpy(new->str + pos, src, length);
			new->length = pos + length;
			*(new->str + new->length) = '\0';
			*dest = new;
			return 1;
		}
	}
	return 0;
}

/*
 * Add 1 character to the end (or truncation point) of a tstr.
 */

int tstrcat1(tstr **dest, char c) {

	tstr *new = *dest;
	size_t len;
	char *d;
	size_t pos;

	if (new) {
		pos = (new->trunc) ? new->trunc - 1 : new->length;
		if (new = tstrnew(new, pos + 1)) {
			d = new->str + pos;
			*d++ = c;
			*d = '\0';
			new->length = pos + 1;
			*dest = new;
			return 1;
		}
	}
	return 0;
}

/*
 * Print a formatted string to a tstr.
 */

int tstrprintf(tstr **dest, const char *format, ...) {

	tstr *new = *dest;
	size_t len;
	size_t alloc;
	va_list parms;

	if (!new) {
		new = tstrnew(new, __tstrallocpadto - sizeof(struct tstr));
	}
	if (new && format) {
		alloc = new->alloc - sizeof(struct tstr);
		va_start(parms, format);
		len = vsnprintf(new->str, alloc, format, parms);
		va_end(parms);
		if (len < alloc) {
			new->length = len;
			if (len + 1 < new->trunc) new->trunc = 0;
			*dest = new;
			return 1;
		} else {
			if (new = tstrnew(new, len)) {
				alloc = new->alloc - sizeof(struct tstr);
				va_start(parms, format);
				len = vsnprintf(new->str, alloc, format, parms);
				va_end(parms);
				new->length = len;
				if (len + 1 < new->trunc) new->trunc = 0;
				*dest = new;
				return 1;
			}
		}
	}
	return 0;
}

/*
 * Print a formatted string to the end (or truncation point) of a tstr.
 */

int tstrcatprintf(tstr **dest, const char *format, ...) {

	tstr *new = *dest;
	size_t len;
	size_t alloc;
	size_t pos;
	va_list parms;

	if (new && format) {
		pos = (new->trunc) ? new->trunc - 1 : new->length;
		alloc = new->alloc - sizeof(struct tstr);
		va_start(parms, format);
		len = pos + vsnprintf(new->str + pos, alloc, format, parms);
		va_end(parms);
		if (len < alloc) {
			new->length = len;
			*dest = new;
			return 1;
		} else {
			if (new = tstrnew(new, len)) {
				alloc = new->alloc - sizeof(struct tstr);
				va_start(parms, format);
				len = pos + vsnprintf(new->str + pos, alloc, format, parms);
				va_end(parms);
				new->length = len;
				*dest = new;
				return 1;
			}
		}
	}
	return 0;
}

/*
 * Insert a tstr into another one.
 */

int tstrins(tstr **dest, tstr *src, size_t pos, size_t start, size_t length) {

	tstr *new = *dest;
	size_t len;
	size_t alloc;

	if (new && src && pos && (pos-- <= new->length)
			&& (len = __tstrlencalc(src, start, length))) {
		alloc = len + new->length;
		if (new = tstrnew(new, alloc)) {
			__tstrncpyb(new->str + pos + len, new->str + pos,
				new->length + 1 - pos);
			__tstrncpy(new->str + pos, src->str + start - 1, len);
			new->length += len;
			*dest = new;
			return 1;
   	}
   }
   return 0;
}

/*
 * Insert a c string into a tstr.
 */

int tstrinsc(tstr **dest, char *src, size_t pos, size_t start, size_t length) {

	tstr *new = *dest;
	size_t len;
	size_t alloc;

	if (new && src && pos && (pos-- <= new->length)
			&& (len = __strlencalc(src, start, length))) {
		alloc = len + new->length;
		if (new = tstrnew(new, alloc)) {
			__tstrncpyb(new->str + pos + len, new->str + pos,
				new->length + 1 - pos);
			__tstrncpy(new->str + pos, src + start - 1, len);
			new->length += len;
			*dest = new;
			return 1;
		}
	}
   return 0;
}

/*
 * Delete part of a tstr.
 */

int tstrdel(tstr *dest, size_t pos, size_t length) {

	if (dest && pos && (pos <= dest->length)) {
		pos--;
		if ((pos + length) >= dest->length) {
			*(dest->str + pos) = '\0';
			dest->length = pos;
		} else {
			__tstrncpy(dest->str + pos, dest->str + pos + length,
				dest->length - pos - length + 1);
			dest->length -= length;
		}
		if (dest->length + 1 < dest->trunc) dest->trunc = 0;
		return 1;
	}
	return 0;
}

/*
 * Change the first instance of a substring within a tstr to another string.
 */

int tstredit(tstr **dest, char *from, char *to, int ignorecase) {

   tstr *new = *dest;
	size_t fl;
	size_t tl;
	size_t al;
	size_t pos;
	char *f;

	fl = strlen(from);
	tl = strlen(to);
	al = abs(tl - fl);
	if (ignorecase) {
		f = strcasestr(new->str, from);
	} else {
		f = strstr(new->str, from);
	}
	if (f) {
		pos = f - new->str;
		if (tl == fl) {
			__tstrncpy(f, to, fl);
			return 1;
		} else if (tl < fl) {
			__tstrncpy(f, to, tl);
			__tstrncpy(f + tl, f + fl, new->length - pos - fl + 1);
			new->length -= al;
			if (new->length + 1 < new->trunc) new->trunc = 0;
			return 1;
		} else {
			if (new = tstrnew(new, new->length + al)) {
				__tstrncpyb(new->str + pos + tl, new->str + pos + fl,
					new->length - pos - fl + 1);
				__tstrncpy(new->str + pos, to, tl);
				new->length += al;
				*dest = new;
				return 1;
			}
		}
	}
	return 0;
}

/*
 * Change all instances of a substring within a tstr to another string.
 */

int tstredit2(tstr **dest, char *from, char *to, int ignorecase) {

	tstr *new = *dest;
	size_t fl;
	size_t tl;
	size_t al;
	char **instances;
	int ni = 0;
	int instance = 0;
   int rc = 0;
	char *(*findstr)(const char *, const char *);
	char *sp;
	char *dp;
	char *ep;
	char *tp;

	fl = strlen(from);
	tl = strlen(to);
	al = abs(tl - fl);
	if (ignorecase) findstr = strcasestr;
	else findstr = strstr;
	if (new && fl) {
		instances = alloca((new->length / fl + 1) * sizeof(char *));
		sp = new->str;
		while (sp = findstr(sp, from)) {
			instances[ni++] = sp;
			sp += fl;
		}
		al *= ni;
		if (ni) {
			if (tl == fl) {
				while (dp = instances[instance++]) {
					__tstrncpy(dp, to, tl);
				}
				rc = 1;
			} else if (tl < fl) {
				instances[ni] = new->str + new->length;
				dp = instances[instance++];
				sp = dp;
				while (ep = instances[instance++]) {
					tp = to;
					sp += fl;
					while (*tp) {
						*dp++ = *tp++;
					}
					while (sp < ep) {
						*dp++ = *sp++;
					}
				}
				*dp = '\0';
				new->length -= al;
				if (new->length + 1 < new->trunc) new->trunc = 0;
				rc = 1;
			} else {
				if (new = tstrnew(new, new->length + al)) {
					instance = ni;
					dp = new->str + new->length + al;
					sp = new->str + new->length;
					do {
						ep = instances[--instance] + fl - 1;
						tp = to + tl - 1;
						while (sp > ep) {
							*dp-- = *sp--;
						}
						while (tp >= to) {
							*dp-- = *tp--;
						}
						sp -= fl;
					} while (instance);
					new->length += al;
					*dest = new;
					rc = 1;
				}
			}
		}
	}
	return rc;
}

/*
 * Selectively change instances of a substring within a tstr to another string.
 * Pass the instances to change in an array.  If the array is NULL, change all
 * instances.
 */

int tstredit3(tstr **dest, char *from, char *to, int ignorecase, int *which) {

	tstr *new = *dest;
	size_t fl;
	size_t tl;
	size_t al;
	char **instances;
	int ni = 0;
	int instance = 0;
	int w = 0;
	int i = 1;
	int cw;
	int rc = 0;
	char *(*findstr)(const char *, const char *);
	char *sp;
	char *dp;
	char *ep;
	char *tp;

	fl = strlen(from);
	tl = strlen(to);
	al = abs(tl - fl);
	if (ignorecase) findstr = strcasestr;
	else findstr = strstr;
	if (new && fl) {
		instances = alloca((new->length / fl + 1) * sizeof(char *));
		sp = new->str;
		if (which) {
			cw = which[w++];
			while (cw && (sp = findstr(sp, from))) {
				if (i++ == cw) {
					instances[ni++] = sp;
					cw = which[w++];
				}
				sp += fl;
			}
		} else {
			while (sp = findstr(sp, from)) {
				instances[ni++] = sp;
				sp += fl;
			}
		}
		al *= ni;
		if (ni) {
			if (tl == fl) {
				while (dp = instances[instance++]) {
					__tstrncpy(dp, to, tl);
				}
				rc = 1;
			} else if (tl < fl) {
				instances[ni] = new->str + new->length;
				dp = instances[instance++];
				sp = dp;
				while (ep = instances[instance++]) {
					tp = to;
					sp += fl;
					while (*tp) {
						*dp++ = *tp++;
					}
					while (sp < ep) {
						*dp++ = *sp++;
					}
				}
				*dp = '\0';
				new->length -= al;
				if (new->length + 1 < new->trunc) new->trunc = 0;
				rc = 1;
			} else {
				if (new = tstrnew(new, new->length + al)) {
					instance = ni;
					dp = new->str + new->length + al;
					sp = new->str + new->length;
					do {
						ep = instances[--instance] + fl - 1;
						tp = to + tl - 1;
						while (sp > ep) {
							*dp-- = *sp--;
						}
						while (tp >= to) {
							*dp-- = *tp--;
						}
						sp -= fl;
					} while (instance);
					new->length += al;
					*dest = new;
					rc = 1;
				}
			}
		}
	}
	return rc;
}

/*
 * Selectively change instances of a substring within a tstr to another string.
 * Pass the instances to change as optional parameters.  If the first is 0, change
 * all instances.  The instances should be ascending in value, and end with 0.
 */

int tstredit4(tstr **dest, char *from, char *to, int ignorecase, ...) {

	tstr *new = *dest;
	size_t fl;
	size_t tl;
	size_t al;
	char **instances;
	int ni = 0;
	int instance = 0;
	int i = 1;
	int cw;
	int rc = 0;
	char *(*findstr)(const char *, const char *);
	char *sp;
	char *dp;
	char *ep;
	char *tp;
	va_list which;

	fl = strlen(from);
	tl = strlen(to);
	al = abs(tl - fl);
	if (ignorecase) findstr = strcasestr;
	else findstr = strstr;
	if (new && fl) {
		instances = alloca((new->length / fl + 1) * sizeof(char *));
		sp = new->str;
		va_start(which, ignorecase);
		cw = va_arg(which, int);
		if (cw) {
			while (cw && (sp = findstr(sp, from))) {
				if (i++ == cw) {
					instances[ni++] = sp;
					cw = va_arg(which, int);
				}
				sp += fl;
			}
		} else {
			while (sp = findstr(sp, from)) {
				instances[ni++] = sp;
				sp += fl;
			}
		}
		va_end(which);
		al *= ni;
		if (ni) {
			if (tl == fl) {
				while (dp = instances[instance++]) {
					__tstrncpy(dp, to, tl);
				}
				rc = 1;
			} else if (tl < fl) {
				instances[ni] = new->str + new->length;
				dp = instances[instance++];
				sp = dp;
				while (ep = instances[instance++]) {
					tp = to;
					sp += fl;
					while (*tp) {
						*dp++ = *tp++;
					}
					while (sp < ep) {
						*dp++ = *sp++;
					}
				}
				*dp = '\0';
				new->length -= al;
				if (new->length + 1 < new->trunc) new->trunc = 0;
				rc = 1;
			} else {
				if (new = tstrnew(new, new->length + al)) {
					instance = ni;
					dp = new->str + new->length + al;
					sp = new->str + new->length;
					do {
						ep = instances[--instance] + fl - 1;
						tp = to + tl - 1;
						while (sp > ep) {
							*dp-- = *sp--;
						}
						while (tp >= to) {
							*dp-- = *tp--;
						}
						sp -= fl;
					} while (instance);
					new->length += al;
					*dest = new;
					rc = 1;
				}
			}
		}
	}
	return rc;
}

int tstrsettrunc(tstr *dest, size_t trunc, int now) {

	if (dest && (dest->length >= trunc)) {
		dest->trunc = trunc + 1;
		if (now) {
			dest->length = trunc;
			*(dest->str + trunc) = '\0';
		}
		return 1;
	}
	return 0;
}

int tstrtrunc(tstr *dest) {

	size_t trunc;

	trunc = dest->trunc - 1;
	if (dest && (dest->length > trunc)) {
		dest->length = trunc;
		*(dest->str + trunc) = '\0';
		return 1;
	}
	return 0;
}

int tstrunsettrunc(tstr *dest) {

	if (dest) {
		dest->trunc = 0;
		return 1;
	}
	return 0;
}

int tstrfill(tstr **dest, char *pattern, size_t length) {

	tstr *new = *dest;
	char *p;
	char *n;
	size_t len;
	size_t pos;

	if (new = tstrnew(new, length)) {
		pos = (new->trunc) ? new->trunc - 1 : new->length;
		if (pos < length) {
			len = length - pos;
			n = new->str + pos;
			while (len) {
				p = pattern;
				while (*p && len) {
					*n++ = *p++;
					len--;
				}
			}
			*n = '\0';
			new->length = length;
		}
		*dest = new;
		return 1;
	}
	return 0;
}

int tstrrev(tstr *dest, size_t start, size_t length) {

	char *b;
	char *e;
	char t;

	if (dest && start && (start < dest->length)) {
		b = dest->str + start - 1;
		e = b + __tstrlencalc(dest, start, length) - 1;
		if (e > b) {
			while (e > b) {
				t = *e;
				*e-- = *b;
				*b++ = t;
			}
			return 1;
		}
	}
	return 0;
}

int tstrextracttok(tstr **dest, char **whence, char *cdelim) {

	char *w = *whence;
	size_t length = 0;

	w = __skipspace(w, cdelim);
	while (!__isspace(w + length, cdelim)) {
		length++;
	}
	*whence = w + length;
	if (length) {
		return tstrcpyn(dest, w, length);
	}
	return 0;
}
