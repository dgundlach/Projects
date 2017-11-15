#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/statfs.h>
#include <dirent.h>
#include "xalloc.h"
#include "stripspaces.h"
#define _KEYPAIR_SOURCE
#include "keypair.h"

int key_pair_list_default_alloc = 256;

int check_dir(char *path) {

	DIR *d;

	if ((d = opendir(path))) {
		closedir(d);
		return 1;
	}
	return 0;
}

int compareKeyPair(const void *first, const void *second) {

	return strcmp((*(keyPair **)first)->key, (*(keyPair **)second)->key);
}

void freeKeyPair(void *pair) {

	free(((keyPair *)pair)->key);
	free(((keyPair *)pair)->data);
	free(pair);
}

void freeKeyPairList(keyPairList *pairlist) {

	int i;

	if (!pairlist) return;
	if (pairlist->list) {
		for (i=0; pairlist->list[i]; i++) {
			if (pairlist->freepair) {
				pairlist->freepair(pairlist->list[i]);
			} else {
				freeKeyPair(pairlist->list[i]);
			}
		}
		free(pairlist->list);
	}
	free(pairlist);
}

keyPairList *newKeyPairList(size_t alloc_size) {

	keyPairList *new;

	new = xmalloc(sizeof(struct keyPairList));
	new->alloc = alloc_size;
	new->max = 0;
	new->count = 0;
	new->list = NULL;
	new->compare = &compareKeyPair;
	new->freepair = &freeKeyPair;
	return new;
}

keyPairList *addKeyPair(keyPairList *key_list, char *key, void *data) {

	keyPair *new_pair;

	if (!key_list) {
		key_list = newKeyPairList(key_pair_list_default_alloc);
	}
	if (!key) return key_list;
	if (key_list->count > (key_list->max - 2)) {
		key_list->max += key_list->alloc;
		key_list->list = xrealloc(key_list->list, key_list->max * sizeof(void *));
	}
	new_pair = xmalloc(sizeof(struct keyPair));
	new_pair->key = key;
	new_pair->data = data;
	key_list->list[key_list->count++] = new_pair;
	key_list->list[key_list->count] = NULL;
	return key_list;
}

keyPairList *addExistingKeyPair(keyPairList *key_list, keyPair *new_pair) {

	if (!key_list) {
		key_list = newKeyPairList(key_pair_list_default_alloc);
	}
	if (!new_pair) return key_list;
	if (key_list->count > (key_list->max - 2)) {
		key_list->max += key_list->alloc;
		key_list->list = xrealloc(key_list->list, key_list->max * sizeof(void *));
	}
	key_list->list[key_list->count++] = new_pair;
	key_list->list[key_list->count] = NULL;
	return key_list;
}
	
keyPairList *loadKeyPairList(char *file, size_t alloc_size,
			int key, int (*check)(char *)) {

	keyPairList *new = NULL;
	keyPair *new_pair;
	FILE *f = stdin;
	char buffer[4096];
	char *a, *b, *t;

	if (strcmp(file, "-")) {
		if (!(f = fopen(file, "r"))) return NULL;
	}
	while (fgets(buffer, sizeof(buffer), f)) {
		a = buffer;
		while (*a == '\t') {
			a++;
		}
		if ((b = strchr(a, '\t'))) {
			*b++ = '\0';
			b = stripspaces(b);
		}
		a = stripspaces(a);
		if (b && key) {
			t = a;
			a = b;
			b = t;
		}
		if (a) {
			if (check && !check(a)) continue;
			new = addKeyPair(new, xstrdup(a), b ? xstrdup(b) : b);
		}
	}
	fclose(f);
	return new;
}
