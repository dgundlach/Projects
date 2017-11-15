#include <stdlib.h>
#include <string.h>
#include "Stack.h"

int StackPush(struct Stack **top, void *data, int dsize) {

    struct Stack *s;

    if (top && (s = malloc(sizeof(struct Stack)))) {
        if (!(s->data = malloc(dsize))) {
            free(s);
            return 0;
        }
        s->prev = *top;
        memcpy(s->data, data, dsize);
        s->dsize = dsize;
        *top = s;
        return 1;
    }
    return 0;
}

int StackPop(struct Stack **top, void **data, int *dsize) {

    struct Stack *t;
    
    if (top && *top) {
        t = *top;
        *data = t->data;
        *dsize = t->dsize;
        *top = t->prev;
        free(t);
        return 1;
    }
    return 0;
}

int StackShrink(struct Stack **top) {

    struct Stack *t;
    
    if (top && *top) {
        t = *top;
        *top = t->prev;
        free(t->data);
        free(t);
        return 1;
    }
    return 0;
}

int StackPeek(struct Stack **top, void **data, int *dsize) {

    if (top && *top) {
        *data = (*top)->data;
        *dsize = (*top)->dsize;
        return 1;
    }
    return 0;
}
