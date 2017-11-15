int Daemonize(void) {

    int pid;
    int sid;
    
    pid = fork();
    if (pid == -1) {
        exit(1);
    }
    if (pid) {
        exit(0);
    }
    signal(SIGCLD, SIG_IGN);
    sid = setsid();
    pid = fork();
    if (pid == -1) {
        exit(1);
    }
    if (!pid) {
        chdir("/");
        umask(0);
        close(0);
        close(1);
        close(2);
    }
    return 0;
}
