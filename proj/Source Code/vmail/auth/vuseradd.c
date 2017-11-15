#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <ctype.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>
#include <pgsql/libpq-fe.h>

#include "users_sql.h"

static gid_t def_group = 100;
static const char *def_home = "/home";
static const char *def_shell = "/bin/noshell";
static long def_inactive = -1;
static const char *def_expire = "-1";

static const char *user_name = "";
static const char *user_pass = "!";
static uid_t user_id;
static gid_t user_gid;
static const char *user_comment = "";
static const char *user_home = "";
static const char *user_shell = "";
static const char *user_domain = "";
static const char *mail_dest = "";
static char user_lastchange[10] = "";
static long min_change = -1;
static long max_change = -1;
static long user_warn = -1;
static long user_inact = -1;
static long user_expire = -1;
static long user_flag = 0;

static char *user_link = "";

static char *Prog;

static char *fields[USERS_NFIELDS + 1];
static char *data[USERS_NFIELDS + 1];

char *Basename(char *);

static int
	cflg = 0, /* comment (GECOS) field for new account */
	dflg = 0, /* home directory for new account */
	eflg = 0, /* days since 1970-01-01 when account is locked */
	fflg = 0, /* days until account with expired password is locked */
	gflg = 0, /* primary group ID for new account */
	lflg = 0, /* link account to another account */
	mflg = 0, /* set the mail destination if not user's account */
	nflg = 0, /* the minumum numbers of days before user can chg pw */
	oflg = 0, /* set user domain */
	pflg = 0, /* set user's password */
	sflg = 0, /* shell program for new account */
	uflg = 0, /* specify user ID for new account */
	wflg = 0, /* number of days to warn after password ages */
	xflg = 0; /* max days user has to chg pw */

void usage(void) {

  fprintf(stderr,"usage %s\t[-p password] [-u uid] [-l linkuser] [-g group]\n"
		"\t\t[-c comment] [-d home] [-s shell] [-o domain] [-m mailto]\n"
		"\t\t[-n minchange] [-x maxchange] [-w warn] [-f inactive]\n"
		"\t\t[-e expire]\n",Prog);
  exit(1);
}

static long get_number(const char *cp) {

  long val;
  char *ep;

  val = strtol(cp,&ep,10);
  if (*cp != '\0' && *ep == '\0') {  /* valid number */
    return val;
  }
  fprintf(stderr,"%s: invalid numeric argument '%s'\n",Prog,cp);
  exit(1);
}

static void process_flags(int argc, char **argv) {

  int arg;
  int i = 0;

  while ((arg = getopt(argc,argv,"c:d:e:f:g:l:m:n:o:p:s:u:w:x")) != EOF) {
    switch (arg) {
    case 'c' :
      fields[i] = USERS_GECOS;
      data[i++] = stringify(optarg);
      cflg++;
      break;
    case 'd' :
      fields[i] = USERS_HOMEDIR;
      data[i++] = stringify(optarg);
      dflg++;
      break;
    case 'e' :
      if (*optarg) {
        user_expire = strtoday(optarg);
        if (user_expire == -1) {
          fprintf(stderr,"%s: invalid date '%s'\n",Prog,optarg);
          exit(1);
        } else {
          char ex[10];
          sprintf(ex,"%d",user_expire);
          fields[i] = USERS_EXPIRE;
          data[i++] = ex;
        }
      }
      eflg++;
      break;
    case 'f' :
      def_inactive = get_number(optarg);
      fields[i] = USERS_INACT;
      data[i++] = optarg;
      fflg++;
      break;
    case 'g' :
      user_gid = get_number(optarg);
      fields[i] = USERS_GID;
      data[i++] = optarg;
      gflg++;
      break;
    case 'l' :
      if (uflg) {
        fprintf(stderr,"%s: you may not specify -l and -u together\n",Prog);
        exit(1);
      }
      user_link = stringify(optarg);
      lflg++;
      break;
    case 'm' :
      fields[i] = USERS_MAILDEST;
      data[i++] = stringify(optarg);
      mflg++;
      break;
    case 'n' :
      min_change = get_number(optarg);
      fields[i] = USERS_MINCHANGE;
      data[i++] = optarg;
      nflg++;
      break;
    case 'o' :
      fields[i] = USERS_DOMAIN;
      data[i++] = stringify(optarg);
      oflg++;
      break;
    case 'p':
      fields[i] = USERS_PASSWD;
      data[i++] = stringify(optarg);
      pflg++;
      break;
    case 's' :
      fields[i] = USERS_SHELL;
      data[i++] = stringify(optarg);
      sflg++;
      break;
    case 'u' :
      user_id = get_number(optarg);
      fields[i] = USERS_UID;
      data[i++] = optarg;
      uflg++;
      break;
    case 'w' :
      user_warn = get_number(optarg);
      fields[i] = USERS_WARN;
      data[i++] = optarg;
      wflg++;
      break;
    case 'x' :
      max_change = get_number(optarg);
      fields[i] = USERS_MAXCHANGE;
      data[i++] = optarg;
      xflg++;
      break;
    default :
      usage();
    }
  }
  if (optind != argc - 1) {
    usage();
  }
  user_name = argv[optind];
  if (!dflg) {
    char *uh;
    uh = malloc(strlen(def_home) + strlen(user_name) + 4);
    sprintf(uh,"'%s/%s'",def_home,user_name);
    user_home = uh;
  }
}




int main(int argc, char **argv) {

  Prog = Basename(argv[0]);
  sprintf (user_lastchange,"%d",time(NULL) / ONE_DAY);
  process_flags(argc,argv);

  return 0;
}
