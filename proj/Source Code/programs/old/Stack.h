struct Stack {
    struct Stack *prev;
    void *data;
    int dsize;
};

int StackPush(struct Stack **, void *, int);
int StackPop(struct Stack **, void **, int *);
int StackShrink(struct Stack **);
int StackPeek(struct Stack **, void **, int *);
