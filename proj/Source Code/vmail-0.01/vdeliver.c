#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <pgsql/libpq-fe.h>
#include "config.h"

#define PERM			100
#define TEMP			111

char buffer[512];
char *connect_str = NULL;
char *default_domain = NULL;

int configure (void)
{
  int i, f, flen;
  char *ch;

  if (!(f = open(CONF_FILE,O_RDONLY))) {
    return -1;
  }
  if (!(flen = read(f,buffer,sizeof(buffer)))) {
    close(f);
    return -1;
  }
  close(f);
  connect_str = buffer;
  i = 0;
  for (;;) {
    if (buffer[i] == '\n') {
      buffer[i] = '\0';
      if (!default_domain) {
        default_domain = buffer + i;
      }
    }
    i++;
  }
  if (!default_domain) {
    return -1;
  }
  return 0;
}

int main (int argc, char **argv)
{
  char *delhost;
  char *recipient;
  char *ch;
  char *sql;
  char *action;
  char *command;
  char buffer[1024];
  PGconn *conn;
  PGresult *res;
  FILE *p;

  delhost = getenv("HOST");
  if (!delhost) {
    fprintf(stderr,"vdeliver: HOST not set.\n");
    exit(PERM);
  }
  if (chdir(delhost) == -1) {
    fprintf(stderr,"vdeliver: Could not chdir to: %s\n",delhost);
    exit(TEMP);
  }
  ch = delhost;
  while (*ch) {
    if (*ch == '.') {
      *ch = '_';
    }
    ++ch;
  }
  recipient = getenv("EXT");
  if (!recipient) {
    fprintf(stderr,"vdeliver: EXT not set.\n");
    exit(PERM);
  }
  if (configure() == -1) {
    fprintf(stderr,"vdeliver: Could not read configuration.\n");
    exit(TEMP);
  }
  conn = PQconnectdb(connect_str);
  if (!conn || (PQstatus(conn) == CONNECTION_BAD)) {
    PQfinish(conn);
    fprintf(stderr,"vdeliver: Could not connect to database.\n");
    exit(TEMP);
  }
  sql = malloc(50 + strlen(recipient) + strlen(delhost));
  sprintf(sql,"select maildest from %s where username = '%s'",
			delhost,recipient);
  res = PQexec(conn,sql);
  free(sql);
  if (!res || (PQresultStatus(res) != PGRES_TUPLES_OK)) {
    PQclear(res);
    PQfinish(conn);
    fprintf(stderr,"vdeliver: Database dropped connection.\n");
    exit(TEMP);
  }
  if (!PQntuples(res)) {
    PQclear(res);
    PQfinish(conn);
    fprintf(stderr,"vdeliver: Recipient not in database.\n");
    exit(PERM);
  }
  action = strdup(PQgetvalue(res,0,0));
  PQclear(res);
  PQfinish(conn);
  if (*action == '|') {
    command = action + 1;
  } else if (*action == '&') {
    command = malloc(5 + sizeof(FORWARD) + strlen(action));
    sprintf(command,"%s %s",FORWARD,action + 1);
  } else {
    if (chdir(action) == -1) {
      umask(0077);
      if (mkdir(action,0700) != -1) {
        chdir(action);
        mkdir("cur",0700);
        mkdir("new",0700);
        mkdir("tmp",0700);
        chdir("..");
      } else {
        fprintf(stderr,"vdeliver: Could not create maildir %s",action);
        exit(TEMP);
      }
    } else {
      chdir("..");
    }
    command = malloc(5 + sizeof(MAILDIRDELIVER) + strlen(action));
    sprintf(command,"%s %s",MAILDIRDELIVER,action);
  }
  if (!(p = popen(command,"w"))) {
    fprintf(stderr,"vdeliver: Could not exec: %s\n",command);
    exit(TEMP);
  }
  while (fgets(buffer,sizeof(buffer),stdin)) {
    fprintf(p,"%s",buffer);
  }
  pclose(p);
  exit(0);
}
