#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

int MySystem(const char *cmd) {

   int stat;
   pid_t pid;
   struct sigaction sa, savintr, savequit;
   sigset_t saveblock;

   if (cmd == NULL)
      return(1);
   sa.sa_handler = SIG_IGN;
   sigemptyset(&sa.sa_mask);
   sa.sa_flags = 0;
   sigemptyset(&savintr.sa_mask);
   sigemptyset(&savequit.sa_mask);
   sigaction(SIGINT, &sa, &savintr);
   sigaction(SIGQUIT, &sa, &savequit);
   sigaddset(&sa.sa_mask, SIGCHLD);
   sigprocmask(SIG_BLOCK, &sa.sa_mask, &saveblock);
   if ((pid = fork()) == 0) {
      sigaction(SIGINT, &savintr, (struct sigaction *)0);
      sigaction(SIGQUIT, &savequit, (struct sigaction *)0);
      sigprocmask(SIG_SETMASK, &saveblock, (sigset_t *)0);
      execl("/bin/sh", "sh", "-c", cmd, (char *)0);
      _exit(127);
   }
   if (pid == -1) {
      stat = -1; /* errno comes from fork() */
   } else {
      while (waitpid(pid, &stat, 0) == -1) {
         if (errno != EINTR) {
            stat = -1;
            break;
         }
      }
   }
   sigaction(SIGINT, &savintr, (struct sigaction *)0);
   sigaction(SIGQUIT, &savequit, (struct sigaction *)0);
   sigprocmask(SIG_SETMASK, &saveblock, (sigset_t *)0);
   return(stat);
}
