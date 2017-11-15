FILE *popen(const char *command, const char *type) {
    int fds[2];
    const char *argv[4] = {"/bin/sh", "-c", command};
    pipe(fds);
    if (fork() == 0) {
        close(fds[0]);
        dup2(type[0] == 'r' ? 0 : 1, fds[1]);
        close(fds[1]);
        execvp(argv[0], argv);
        exit(-1);
    }
    close(fds[1]);
    return fdopen(fds[0], type);
}
