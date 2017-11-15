struct commands {
    char *text;
    int (*fun)();
    void (*flush)();
};

int command(char *, struct commands *);
