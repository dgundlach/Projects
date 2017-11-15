#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>
#include <stdlib.h>
#include <pwd.h>
#include <shadow.h>
#include <grp.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <pgsql/libpq-fe.h>

#define SYNCUP_MAX		500
#define ONE_DAY			86400
#define PASSWD_FILE		"/etc/passwd"
#define SHADOW_FILE		"/etc/shadow"
#define GROUP_FILE		"/etc/group"
#define PASSWD_LOCK		"/etc/.pwd.lock"
#define SHADOW_LOCK		"/etc/.spwd.lock"
#define GROUP_LOCK		"/etc/.grp.lock"

struct db_def {
  char *table;
  char *filename;
  char *lockname;
  char *fields;
} db_def;

char *stringify(char *, char *);
void initialize(void);

char *nodename;
char *lastchecked;
char *checktime;
char **classes;

char *stringify(char *string, char *escape)
{
  char *newstr;
  char *n, *o;

  if (!string || !escape) {
    return NULL;
  }
  newstr = malloc((2 * strlen(string)) + 3);
  n = newstr;
  o = string;
  *n++ = *escape;
  while (*o) {
    if (strchr(escape,*o)) {
      *n++ = '\\';
    }
    *n++ = *o++;
  }
  *n++ = *escape;
  *n++ = '\0';
  newstr = realloc(newstr,n - newstr);
  return newstr;
}

int sync_db(PGconn *conn,struct db_def *db) {
{
  char sql[512];
  char *endptr;
  int printed, ntuples, nfields, tuple, field, i, f;
  PGresult *res;
  struct flock fl;

  printed = sprintf(sql,"select %s from %s where class='%s'",
		db->fields,db->table,nodename);
  endptr = sql + printed;
  i = 0;
  while (classes[i]) {
    printed = sprintf(endptr," or class='%s'",classes[i++]);
    endptr += printed;
  }
  sprintf(endptr," order by uid");
  res = PQexec(conn,sql);
  if (!res || (PQresultstatus(res) != PGRES_TUPLES_OK)) {
    PQclear(res);
    return 0;
  }
  if ((ntuples = PQntuples(res))) {
    fl.l_type = F_WRLCK;
    fl.l_pid = getpid();
    if (!(f = open(db->lockname,O_WRONLY))) {
      PQclear(res);  
      return 0;
    }
    fcntl(f,F_SETLK,&fl);
    for (tuple=0; tuple<ntuples; tuple++) {
      for (field=0; field<nfields; field++) {
        if (field) {
          write(f,":",1);
        }
        write(f,PQgetvalue(res,tuple,field),PQgetlength(res,tuple,field));
      }
      write(f,"\n",1);
    }
    close(f);
    rename(db->lockname,db->filename);
  }
  PQclear(res);
  if (ntuples) {
    return 1;
  }
  return 0;
}

int syncdown(PGconn *conn)
{
  struct db_def passwd_db = {
    PASSWD_FILE,
    PASSWD_LOCK,
    "users",
    "username,'x',uid,gid,gecos,home,shell"
  };
  struct db_def shadow_db = {
    SHADOW_FILE,
    SHADOW_LOCK,
    "users",
    "username,passwd,lastchange,minchange,maxchange,warn,inact,expire,flag"
  };
  struct db_def group_db = {
    GROUP_FILE,
    GROUP_LOCK,
    "groups",
    "groupname,grppasswd,gid,members"
  };

  sync_db(conn,passwd_db);
  sync_db(conn,shadow_db);
  sync_db(conn,group_db);
}

void syncup(PGconn *conn)
{
  struct passwd *pwent;
  struct spwd *spent;
  struct group *grent;
  char *sp_pwdp;
  long sp_lstchg;
  int sp_min,sp_max,sp_warn,sp_inact,sp_expire,sp_flag;
  time_t now;
  char buff[2048];
  char *start, *endptr;
  int plen, i;
  PGresult *res;
  int p,s,g;
  struct flock fl;

  if (p = open(PASSWD_LOCK,O_RDONLY)) {
    close(p);
    return;
  }
  if (s = open(SHADOW_LOCK,O_RDONLY)) {
    close(s);
    return;
  }
  if (g = open(GROUP_LOCK,O_RDONLY)) {
    close(g);
    return;
  }
  bzero(fl,sizeof(struct flock));
  fl.l_type = F_WRLCK;
  fl.l_pid = getpid();
  p = open(PASSWD_LOCK,O_WRONLY|O_CREAT);
  fcntl(p,F_SETLK,&fl);
  s = open(SHADOW_LOCK,O_WRONLY|O_CREAT);
  fcntl(s,F_SETLK,&fl);
  g = open(GROUP_LOCK,O_WRONLY|O_CREAT);
  fcntl(g,F_SETLK,&fl);
  now = time(NULL) / ONE_DAY;
  while ((pwent = getpwent())) {
    if (pwent->pw_uid < SYNCUP_MAX) {
      if ((spent = getspnam(pwent->pw_name))) {
        sp_pwdp = spent->sp_pwdp;
        sp_lstchg = spent->sp_lstchg;
        sp_min = spent->sp_min;
        sp_max = spent->sp_max;
        sp_warn = spent->sp_warn;
        sp_inact = spent->sp_inact;
        sp_expire = spent->sp_expire;
        sp_flag = spent->sp_flag;
      } else {
        sp_pwdp = pwent->pw_passwd;
        sp_lstchg = -1;
	sp_min = sp_max = sp_warn = sp_inact = sp_expire = sp_flag = -1;
      }
      sprintf(buff,"insert into base values('%s','%s',%d,%d,'%s','%s','%s'"
		",%d,%d,%d,%d,%d,%d,%d,timestamp('now'),'A','%s'",
		pwent->pw_name,sp_pwdp,pwent->pw_uid,pwent->pw_gid,
		pwent->pw_gecos,pwent->pw_dir,pwent->pw_shell,sp_lstchg,
		sp_min,sp_max,sp_warn,sp_inact,sp_expire,sp_flag,nodename);
      res=PQexec(conn,buff);
      PQclear(res);
    }
  }
  while ((grent = getgrent())) {
    if (grent->gr_gid < SYNCUP_MAX) {
      plen = sprintf(buff,"insert into base_groups values('%s','%s',%d,'",
		grent->gr_name,grent->gr_passwd,grent->gr_gid);
      endptr = buff + plen;
      if (grent->gr_mem) {
        i = 0;
        while (grent->gr_mem[i]) {
          if (!i) {
            plen = sprintf(endptr,"%s",grent->gr_mem[i++]);
          } else {
            plen = sprintf(endptr,",%s",grent->gr_mem[i++]);
          }
          endptr += plen;
        }
      }
      sprintf(endptr,"')");
      res = PQexec(conn,buff);
      PQclear(res);
    }
  }
  fl.l_type = F_UNLCK;
  fcntl(p,F_SETLK,&fl);
  close(p);
  fcntl(s,F_SETLK,&fl);
  close(s);
  fcntl(g,F_SETLK,&fl);
  close(g);
  unlink(PASSWD_LOCK);
  unlink(SHADOW_LOCK);
  unlink(GROUP_LOCK);
}

void getlastchecked(PGconn * conn)
{
  char *buffer;
  PGresult *res;
  char *classbuf, *classptr, *next;
  int i;

  buffer = malloc(128 + strlen(nodename));
  sprintf(buffer,"select timestamp('now')");
  res = PQexec(conn,buffer);
  if (PQntuples(res)) {
    checktime = strdup(PQgetvalue(res,0,1));
  }
  PQclear(res);
  sprintf(buffer,"select * from clients where hostname='%s'",nodename);
  res = PQexec(conn,buffer);
  if (PQntuples(res)) {
    lastchecked = strdup(PQgetvalue(res,0,1));
    classbuf = strdup(PQgetvalue(res,0,2));
    classes = malloc(10 * sizeof(char *));
    i = 0;
    classptr = classbuf;
    while (*classptr) {
      classes[i] = classptr;
      if ((next = strchr(classptr,','))) {
        *next++ = '\0';
      } else {
        next = classptr;
        while (*next) {
          next++;
        }
      }
      classptr = next;
    }
    PQclear(res);
    sprintf(buffer,"update clients set lastchecked='%s' where hostname='%s'",
		checktime,nodename);
    res = PQexec(conn,buffer);
  } else {
    PQclear(res);
    sprintf(buffer,"insert into clients values ('%s','%s','staff')",
		nodename,checktime);
    res = PQexec(conn,buffer);
  }
  PQclear(res);
  free(buffer);
}

void initialize(void)
{
  struct utsname uts;

  if (uname(&uts) != -1) {
    nodename = strdup(uts.nodename);
  } else {
    nodename = NULL;
  }
}


int main (int argc, char **argv)
{
  initialize();
  if (nodename == NULL) {
    exit(1);
  }
  


}
