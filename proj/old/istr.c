#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "istr.h"

size_t istrallocpadto = 256;

void istrsetpadto(size_t padto) {

	istrallocpadto = padto;
}

size_t istrlencalc(size_t slen, size_t start, size_t length) {

	size_t len;

	if (!start || (slen < start)) return 0;
	len = slen - start;
	if (length) len = (len > length) ? length : len;
	return len;
}

istr *istrnew(istr *orig, size_t alloc) {

	istr *new = orig;
	int reallynew = (orig == NULL);
	size_t allocsize;
	
	allocsize = ((size_t)((sizeof(struct istr) + alloc) / istrallocpadto) + 1) * istrallocpadto;
	if ((!reallynew) && (allocsize <= new->alloc)) {
		return new;
	}
	if (new = (istr *)(realloc(new, allocsize))) {
		new->alloc = allocsize;
		if (reallynew) {
			new->length = 0;
			new->trunc = 0;
			*istrdata(new, 0) = '\0';
		}
	}
	return new;
}

istr *istrdestroy(istr *orig) {

	free(orig);
	return NULL;
}

istr *istrdup(istr *orig) {

	istr *new;

	if (new = istrnew(NULL, orig->alloc)) {
		memcpy(new, orig, sizeof(struct istr) + orig->length + 1);
	}
	return new;
}

istr *istrdupc(char *orig) {

	istr *new;
	size_t len;

	len = strlen(orig);
	if (new = istrnew(NULL, len + 1)) {
		strcpy(istrdata(new, 0), orig);
		new->length = len;
	}
	return new;
}

istr *istrcpy(istr *dest, istr *src, size_t start, size_t length) {

	istr *new;
	size_t len;

	if (!(len = istrlencalc(src->length, start, length))) return dest;
	if (new = istrnew(dest, len + 1)) {
		strncpy(istrdata(new, 0), istrdata(src, start - 1), len);
		new-> length = len;
	}
	return new;
}

istr *istrcpyc(istr *dest, char *src, size_t start, size_t length) {

	istr *new;
	size_t len;

	if (!(len = istrlencalc(strlen(src), start, length))) return dest;
	if (new = istrnew(dest, len + 1)) {
		strncpy(istrdata(new, 0), src + start - 1, len);
		new->length = len;
	}
	return new;
}

istr *istrcat(istr *dest, istr *src, size_t start, size_t length) {

	istr *new;
	size_t len;

	if (!(len = istrlencalc(src->length, start, length))) return dest;
	if (new = istrnew(dest, len + dest->length + 1)) {
		strncpy(istrdata(new, new->length), istrdata(src, start - 1), len);
		new->length += len;
	}
	return new;
}

istr *istrcatc(istr *dest, char *src, size_t start, size_t length) {

	istr *new;
	size_t len;

	if (!(len = istrlencalc(strlen(src), start, length))) return dest;
	if (new = istrnew(dest, len + dest->length + 1)) {
		strncpy(istrdata(new, new->length), src + start - 1, len);
		new->length += len;
	}
	return new;
}

int istrsettrunc(istr *dest, size_t trunc, int now) {

	if (dest && (dest->length >= trunc)) {
		dest->trunc = trunc;
		if (now) {
			dest->length = trunc;
			*istrdata(dest, dest->trunc) = '0';
		}
		return 0;
	}
	return 1;
}

int istrtrunc(istr *dest) {

	if (dest && (dest->length > dest->trunc)) {
		dest->length = dest->trunc;
		*istrdata(dest, dest->trunc) = '\0';
		return 0;
	}
	return 1;
}

istr *istrcat1(istr *dest, char c) {

	istr *new;
	size_t len;

	if (new = istrnew(dest, dest->length + 2)) {
		*istrdata(new, new->length++) = c;
		*istrdata(new, new->length) = '\0';
	}
	return new;
}
