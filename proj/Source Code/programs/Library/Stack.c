#include <stdlib.h>
#include <string.h>

struct Stack {
    struct Stack *prev;
    void *data;
    int dsize;
};

static struct Stack *__iS__ = NULL;

int StackPush(void *data, int dsize) {

    struct Stack *s;

    if (data && dsize && (s = malloc(sizeof(struct Stack)))) {
        if (!(s->data = malloc(dsize))) {
            free(s);
            return 0;
        }
        s->prev = __iS__;
        memcpy(s->data, data, dsize);
        s->dsize = dsize;
        __iS__ = s;
        return 1;
    }
    return 0;
}

void *StackPop(int *dsize) {

    struct Stack *t;
    void *data;

    if (__iS__) {
        t = __iS__;
        data = t->data;
        if (dsize) {
            *dsize = t->dsize;
        }
        __iS__ = t->prev;
        free(t);
        return data;
    }
    return NULL;
}

int StackShrink(void) {

    struct Stack *t;
    
    if (__iS__) {
        t = __iS__;
        __iS__ = t->prev;
        free(t->data);
        free(t);
        return 1;
    }
    return 0;
}

void *StackPeek(int *dsize) {

    void *data;

    if (__iS__) {
        data = __iS__->data;
        if (dsize) {
            *dsize = __iS__->dsize;
        }
        *dsize = __iS__->dsize;
        return data;
    }
    return NULL;
}
