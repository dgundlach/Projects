#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#include "users_sql.h"

#define ADDVUSER_PROG		"addvuser"
#define ADDVALIAS_PROG		"addvalias"
#define ADDVMAILUSER_PROG	"addvmailuser"

#define USER_GID		100
#define MAILUSER_GID		200
#define ALIAS_GID		300

#define DEFAULT_HOME		"/users"

#define SETUP_TIME		5
#define FIRST_GRACE		5
#define GRACE_PERIOD		15

long strtoday(char *);

static char *T_Default = USERS_TABLE;

static char *F_Username    = USERS_USERNAME;
static char *F_Passwd      = USERS_PASSWD;
static char *F_UID         = USERS_UID;
static char *F_GID         = USERS_GID;
static char *F_Comment     = USERS_COMMENT;
static char *F_Home        = USERS_HOMEDIR;
static char *F_LastChange  = USERS_LASTCHANGE;
static char *F_MinChange   = USERS_MINCHANGE;
static char *F_MaxChange   = USERS_MAXCHANGE;
static char *F_Warn        = USERS_WARN;
static char *F_Inact       = USERS_INACT;
static char *F_Expire      = USERS_EXPIRE;
static char *F_LockDate    = USERS_LOCKDATE;
static char *F_LockedBy    = USERS_LOCKEDBY;
static char *F_LockComment = USERS_LOCKCOMMENT;

static char *Prog;
static char *Def_Home = DEFAULT_HOME;


static char *MasterUser = NULL;
static char *UnEncrypted = NULL;
static char *HomeDir = NULL;
static char *Comment = NULL;
static char *Domain = "users";
static char *Expire = NULL;
static char *Username = NULL;
static char *UnEncrypted = NULL;
static char *Crypted = NULL;
static char[12] GID = "\0";
static char[12] UID = "\0";
static uid_t UserUID;
static gid_t UserGID;

static char *DBFields[64];
static char *DBData[64];

char *BaseName(char *prog) {

  char *b;

  b = strrchr(prog,'/');
  return b ? b + 1 : prog;
}

void usage(gid_t gid) {

  printf("usage: %s [-u <existing user>] [-c <comment>] [-o <domain>]\n",Prog);
  switch (gid) {
  case USER_GID:
    printf("\t[-p <password>] [-d <alt dir>] [-e <expire>]\n");
    break;
  case MAILUSER_GID:
    printf("\t[-p <password>]\n");
    break;
  }
  exit(1);
} 

int process_flags(int argc, char **argv) {

  int arg;

  while ((arg = getopt(argc,argv,"u:p:d:c:o:e")) != EOF) {
    switch (arg) {
    case 'u' :
      MasterUser = optarg;
      break;
    case 'p' :
      if (UserGID == ALIAS_GID) usage(gid);
      UnEncrypted = optarg;
      break;
    case 'd' :
      if (UserGID != USER_GID) usage(gid);
      HomeDir = optarg;
      break;
    case 'c' :
      Comment = optarg;
      break;
    case 'o' :
      Domain = optarg;
      break;
    case 'e' :
      if (UserGID != USER_GID) usage(gid);
      Expire = optarg;
      break;
    default :
      usage(UserGID);
    }
  }
  if (optind != argc - 1) {
    usage(UserGID);
  }
  Username = argv[optind];

}

int massage_data(void) {

  int i = 0;

  DBFields[i] = F_Username;
  DBData[i++] = Username;
  if (UnEncrypted) {
    Crypted = crypt(UnEncrypted,salt());
  } else {
    Crypted = "!!";
  }
  DBFields[i] = F_Passwd;
  DBData[i++] = Crypted;
  if (MasterUser) {
    char *f[3];
    char **rdata = NULL;

    f[0] = F_UID;
    f[1] = F_Home;
    f[2] = NULL;
    if ((rdata = retrieve_one(conn,Domain,f,"%s = '%s'",F_Username,MasterUser))) {
      DBFields[i] = F_UID;
      DBData[i++] = rdata[0];
      if (UserGID == ALIAS_GID) {
        HomeDir = rdata[1];
      }
    } else {
      ExitNicely("User %s does not exist in domain %s.\n",MasterUser,Domain);
    }
  }
  sprintf(GID,"%d",UserGID);
  DBFields[i] = F_GID;
  DBData[i++] = GID;
  if (!Comment) Comment = Username;
  DBFields[i] = F_Comment;
  DBData[i++] = Comment;
  if (!HomeDir) {
    HomeDir = malloc(1024);
    sprintf(HomeDir,"%s/%s",Def_Home,Username);
  }
  DBFields[i] = F_Home;
  DBData[i++] = HomeDir;
  if (Expire) {
    DBFields[i] = F_Expire;
    DBData[i++] = Expire;
  } 

}

int main(int argc, char **argv) {

  int gid;

  Prog = BaseName(argv[0]);
  if (!strcmp(Prog,ADDVUSER_PROG)) {
    process_flags(USER_GID,argc,argv);
  } else if (!strcmp(Prog,ADDVALIAS_PROG)) {
    process_flags(ALIAS_GID,argc,argv);
  } else {
    process_flags(MAILUSER_GID,argc,argv);
  }

  exit(0);
}
