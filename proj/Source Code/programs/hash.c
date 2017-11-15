#include <mysql3/mysql/mysql.h>
#include <stdio.h>
#include <stdlib.h>

static const unsigned int primes[] = {
        53,        97,        193,       389,
        769,       1543,      3079,      6151,
        12289,     24593,     49157,     98317,
        196613,    393241,    786433,    1572869,
        3145739,   6291469,   12582917,  25165843,
        50331653,  100663319, 201326611, 402653189,
        805306457, 1610612741
};

const unsigned int prime_table_length = sizeof(primes)/sizeof(primes[0]);

unsigned int hashsize(int tablesize) {

    int i;

    tablesize = ((tablesize * 5) + 2) / 3;
    for (i=0; i<prime_table_length; ++i)
        if (primes[i] > tablesize) return primes[i];
    return 0;
}

unsigned long hash(unsigned char *str) {
        
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

typedef struct hash_entry_s {
    char *key;
    void *value;
    struct hash_entry_s *next;
    struct hash_entry_s *bottom;
} hash_entry;

typedef struct {
    size_t size;
    size_t count;
    hash_entry **table;
    hash_entry *top;
} hash_table;

void clear_hash_table(hash_table *hashtable, hash_entry **free_list) {

    hash_entry *entry;

    if (hashtable->count && hashtable->top) {
        hashtable->count = 0;
        if (*free_list) {
            entry = *free_list;
            while (entry->bottom) {
                entry = entry->bottom;
            }
            entry->bottom = hashtable->top;
        } else {
            *free_list = hashtable->top;
        }
        hashtable->top = NULL;
    }
}

int hash_insert(hash_table *hashtable, hash_entry **free_list, char *key, void *value) {

    unsigned int index;
    hash_entry *e = *free_list;

    if (e) {
        *free_list = e->bottom;
    }
    if (!e && !(e = malloc(sizeof(hash_entry)))) {
        return 0;
    }
    e->key = key;
    e->value = value;
    e->bottom = hashtable->top;
    index = hash(e->key) % hashtable->size;
    e->next = hashtable->table[index];
    hashtable->table[index] = e;
    hashtable->count++;
    hashtable->top = e;
    return 1;
}

hash_table *build_hashtable(void *result, size_t *hsize) {

    hash_table *hashtable;
    MYSQL_FIELD *fields;
    unsigned int i;
    unsigned int count;

    hashtable = malloc(sizeof(hash_table));
    count = mysql_num_fields((MYSQL_RES *)result);
    hashtable->size = hashsize(hashtable->count);
    hashtable->table = calloc(hashtable->size, sizeof(void *));

    fields = mysql_fetch_fields((MYSQL_RES *)result);
    for (i=0; i<count; ++i) {
        hash_insert(hashtable, NULL, fields[i].name, NULL);
    }
    return hashtable;
}

int main (int argc, char **argv) {

    int i;
    int size;

    size = hashsize(argc);
    printf("%8i\n", size);
    for (i=2; i<argc; ++i)
        printf("%8lu %s\n", hash(argv[i]) % size, argv[i]);
    return 0;
}


