typedef struct keyPair {
	char *key;
	void *data;
} keyPair;

typedef struct keyPairList {
	int alloc;
	int max;
	int count;
	keyPair **list;
	int (*compare)(const void *, const void *);
	void (*freepair)(void *);
} keyPairList;

#ifndef _KEYPAIR_SOURCE
extern int key_pair_list_default_alloc;
#endif

int check_dir(char *);
int compareKeyPair(const void *, const void *);
void freeKeyPair(void *);
void freeKeyPairList(keyPairList *);
keyPairList *newKeyPairList(size_t);
keyPairList *addKeyPair(keyPairList *, char *, void *);
keyPairList *addExistingKeyPair(keyPairList *, keyPair *);
keyPairList *loadKeyPairList(char *, size_t, int, int (*)(char *));
