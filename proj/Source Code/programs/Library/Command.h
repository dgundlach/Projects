struct commands {
    char *text;
    int (*fun)();
    void (*flush)();
};

int Command(char *, struct commands *);


