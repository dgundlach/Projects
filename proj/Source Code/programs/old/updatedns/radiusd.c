/*
 *
 *	RADIUS
 *	Remote Authentication Dial In User Service
 *
 * ASCEND: @(#)radiusd.c	1.4 (95/07/25 00:55:34)
 *
 *
 *	Livingston Enterprises, Inc.
 *	6920 Koll Center Parkway
 *	Pleasanton, CA   94566
 *
 *	Copyright 1992 Livingston Enterprises, Inc.
 *
 *	Permission to use, copy, modify, and distribute this software for any
 *	purpose and without fee is hereby granted, provided that this
 *	copyright and permission notice appear on all copies and supporting
 *	documentation, the name of Livingston Enterprises, Inc. not be used
 *	in advertising or publicity pertaining to distribution of the
 *	program without specific prior permission, and notice be given
 *	in supporting documentation that copying and distribution is by
 *	permission of Livingston Enterprises, Inc.
 *
 *	Livingston Enterprises, Inc. makes no representations about
 *	the suitability of this software for any purpose.  It is
 *	provided "as is" without express or implied warranty.
 *
 *	Modified by Ascend Communications, Inc. to support authentication
 *	via the Enigma Logic SafeWord library, version 4.0 (sync and async
 *	modes but not semisync). No retries supported (CHALLENGE, PASS, or
 *	FAIL are the only possible replies). Also, only dynamic passwords
 *	are supported (no fixed passwords, etc).
 *
 *      Modified by Ascend Communications, Inc. to support authentication
 *      via the Security Dynamics ACE library. Next_passcode is supported,
 *	new_pin support is not complete.
 */

/* $Id: radiusd.c,v 1.23 1998/06/18 20:27:35 lacker Exp $ */

/* don't look here for the version, run radiusd -v or look in version.c */
static char sccsid[] =
"@(#)radiusd.c	1.17 Copyright 1992 Livingston Enterprises Inc";

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/socket.h>
#include	<sys/file.h>
#include	<netinet/in.h>

#include	<stdio.h>
#include	<netdb.h>
#include	<fcntl.h>
#include	<pwd.h>
#include	<time.h>
#include	<ctype.h>
#include	<unistd.h>
#include	<signal.h>
#include	<errno.h>
#include	<sys/wait.h>

#include	<sys/time.h>	/* gettimeofday() */
#include	<pwd.h>
#include	<memory.h>
#include	<string.h>

#if !defined(NFILE)
#	include	<sys/resource.h>
#endif

#if !defined(NOSHADOW)
#include	<shadow.h>
#endif /* !NOSHADOW */

#include	"cache.h"
#include	"radius.h"
#include	"protos.h"
#include	"des.h"

#include	"radipa.h"

#if __STDC__ == 1
#	include	<stdarg.h>
#else
#	include	<varargs.h>
#endif
void vdebugf P__((CONST char *fmt, va_list ap));

#define	PWD_DEBUG	/* undef to avoid clear-text password debug info */
#ifdef PWD_DEBUG
#define DEBUG_PWD(x) DEBUG x
#else
#define DEBUG_PWD(x)
#endif


#define UNIX_PWD	"UNIX"	/* Unix reserved password */
#define SQL_PWD		"SQL"	/* SQL reserved password */

    /* enumeration of results of authentication */
typedef enum authResults {
	SAFEWORD_FAILED	= -100,
	SAFEWORD_PASSED,
	SAFEWORD_CHALLENGING,
	ACE_FAILED,
	ACE_PASSED,
	ACE_CHALLENGING,
	ACE_NEXT_PASSCODE,
	ACE_NEW_PIN,
	DPI_FAILED,
	DPI_PASSED
} authResults;

#if !defined(sys5) && !defined(_BSD_SOURCE)
typedef unsigned long	ulong;
#endif

FILE	*errf;

	/* associate strings with the error messages */
	/* keep these in the same order as the typedefs in radius.h */
DBASE_ERRMSG db_errmsgs[NUM_DBASE_ERRORS] = {
	{DEFAULT_PARSE_ERR,		"Default Parse Error"},
	{DICT_VALFIND_ERR,		"Dict-Value Find Error"},
	{BINARY_FILTER_ERR,		"Binary Filter Error"},
	{MISSING_EQUALS,		"Missing Equals Sign"},
	{DICT_ATTRFIND_ERR,		"Dict-Attribute Find Error"},
	{REPLY_FIRST_IS_NULL,		"INTERNAL: No Valid Reply Attributes"},
	{MISSING_NEWLINE,		"Missing Newline"},
	{NO_USER_OR_DEFAULT_NAME,	"Neither User Nor Default Name"},
	{ZERO_LENGTH_NAME,		"Zero Length Name"},
	{NO_USER_FILE,			"Missing Radius Users File"},
	{NO_WHITESPACE,			"Missing Whitespace After User Name"},
	{USERFILE_RENAME_FAILED,	"Failed Renaming Users File"},
	{USERFILE_READ_ERR,		"Failed Reading Users File"},
	{USERFILE_WRITE_ERR,		"Failed Writing Users File"},
	{END_OF_USERS_LIST,		"Reached End of Users File"},
	{MEMORY_ERR,			"Out of Memory"},
	{DICTFILE_READ_ERR,		"Failed Reading Dictionary File"},
	{DICTFILE_ATTRLINE_ERR,		"Error on Dictionary Attribute Line"},
	{DICTFILE_ATTRNAME_ERR,		"Dictionary Attribute Name Error"},
	{DICTFILE_ATTRVALUE_ERR,	"Dictionary Attribute Name Error"},
	{DICTFILE_ATTRTYPE_ERR,		"Dictionary Attribute Type Error"},
	{DICTFILE_VALLINE_ERR,		"Error on Dictionary Value Line"},
	{DICTFILE_VALATTR_ERR,		"Dictionary Value Attribute Error"},
	{DICTFILE_VALNAME_ERR,		"Dictionary Value Name Error"},
	{DICTFILE_VALVALUE_ERR,		"Dictionary Value Value Error"},
	{UNIX_GETPWNAME_ERR,		"Unix Get Pwd Name Error"},
	{UNIX_GETSHDWNAME_ERR,		"Unix Get Shadow Pwd Name Error"},
	{UNIX_BAD_PASSWORD,		"Unix Bad Password"},
	{CLIENTFILE_READ_ERR,		"Failed Reading Clients File"},
	{WRONG_NAS_ADDR,		"Wrong NAS Address"},
	{LOGFILE_APPEND_ERR,		"Failed Appending to Log File"},
	{NULL_VALUEPAIR,		"INTERNAL: Null Value-Pair Parameter"},
	{PWD_EXPIRED,			"Password Expired"},
	{PIPE_CREATE_ERR,		"Pipe Creation Failed"},
	{INVALID_DATE_FORMAT,		"Invalid Date Format"}
};

/*------------------------------------------------------------*/

	/* function prototypes */
extern int main P__((int argc, char **argv));
static void usage P__((void));
#if defined(sys5) || defined(BSDI) || defined(FreeBSD) || defined(__hpux) || defined(_BSD_SOURCE)
extern char *crypt P__((CONST char *, CONST char *));
#else
extern char *crypt P__((u_char *, u_char *));
#endif
extern char *md5_crypt(const char *, const char *);
static void insertValuePair P__((VALUE_PAIR** list, VALUE_PAIR* pair));
static VALUE_PAIR *copyValuePair P__((VALUE_PAIR* source));
static void insert_response_md5_digest P__((AUTH_HDR *auth, CONST u_char *secret, int length));
static int get_NFILE P__((void));
static int rad_spawn_child P__((AUTH_REQ *authreq, int *pipe_fds));
static int handle_radipa_response P__((int sock, int activefd));
static void forward_radipad_response P__((int sock, AUTH_REQ *authreq, UINT4 ip_address));
static void free_authreq P__((AUTH_REQ *authreq));
static void dequeue_authreq P__((AUTH_REQ *authreq));
static int forward_duplicate_request P__((AUTH_REQ *authreq, int activefd));
static void allocate_ip_address_from_global_pool P__((AUTH_REQ *authreq, int activefd));
static void release_ip_address_to_global_pool P__((AUTH_REQ *authreq, int activefd));
static int maybe_init_radipa P__((void));
static void sig_suicide P__((int sig));
static void sig_hangup P__((int sig));
static void sig_cleanup P__((int sig));
static void sig_fatal P__((int sig));
static char *request_id P__((AUTH_REQ *authreq));

static void handle_radius_request P__((int fd));
static void rad_child_loop P__((AUTH_REQ *authreq, int fd));
static void maybe_retransmit P__((AUTH_REQ *authreq, int fd));
static int parse_authreq P__((AUTH_REQ *authreq, u_char *ptr, int length));
PasswordType getPwdType P__((char *pwd));
VALUE_PAIR *cut_attribute P__((VALUE_PAIR **list, int attributeId));
static int phone_cmp( CONST char *checkPhone, CONST char *authPhone );
void authCleanup P__((AuthInfo *authInfo, VALUE_PAIR *checkList, VALUE_PAIR *replyList));
int authChapToken P__((AuthInfo *authInfo));
int authChapPwd P__((AuthInfo *authInfo));
int authAraDesPwd P__((AuthInfo *authInfo));
int authPapPwd P__((AuthInfo *authInfo));
char *decryptAuthPwd P__((char CONST *where, AuthInfo *authInfo, int subst3rd, char *buffer));

#if defined( ASCEND_LOGOUT )
void send_logout_response P__((AUTH_REQ *authreq, int activefd));
#endif

#define CLEANUP_DELAY		30
#define SUICIDE_DELAY		15
#define REQUESTS_BACKLOG_THRESHOLD 100

/*------------------------------------------------------------*/



#if defined(SAFEWORD)
/*-------------- Safeword Interface Begin --------------*/

	/* SafeWord interface files */
#include "custpb.h"	/* interface information */
#include "custfail.h"	/* failure codes */

	/* forward function declarations */
extern void pbmain P__((struct pblk *));	/* invoke SafeWord */
extern void initpb P__((struct pblk *));	/* init SafeWord param blk */
extern void dbgPblk P__((struct pblk *pb));	/* show SafeWord param blk */
extern int safeword_chall P__((AuthInfo *authInfo, struct pblk *pb));
extern int safeword_eval P__((AuthInfo *authInfo, struct pblk *pb));
		/* request authentication via SafeWord */
extern int safeword_pass P__((AuthInfo *authInfo, struct pblk *pb));

	/* useful definitions */
#define EL_PWD	"SAFEWORD"	/* Enigma Logic's reserved password */

#define BEG_PB_1ST(pb)    (&(pb)->uport)
#define END_PB_1ST(pb)    (&(pb)->msg1)

#define BEG_PB_2ND(pb)    (&(pb)->errcode)
#define END_PB_2ND(pb)    (&(pb)->status + sizeof((pb)->status))

#define LEN_PB_1ST(pb)    ((ulong)END_PB_1ST((pb)) - (ulong)BEG_PB_1ST((pb)))
#define LEN_PB_2ND(pb)    ((ulong)END_PB_2ND((pb)) - (ulong)BEG_PB_2ND((pb)))

#define LEN_PB_TOTAL(pb)  (LEN_PB_1ST(pb) + LEN_PB_2ND(pb))

/*-------------- Safeword Interface End --------------*/

int authSafewordPwd P__((AuthInfo *authInfo));
int do_safeword_pass P__((AuthInfo *authInfo, struct pblk *pb));

#endif

#if defined(ACE)
/*-------------- Ace Interface Begin --------------*/
#include "sdi_athd.h"
#include "sdi_size.h"
#include "sdi_type.h"
#include "sdacmvls.h"
#include "sdconf.h"

/* Allow the user two minutes to respond to a next-passcode request. */
#define ACE_SUICIDE_DELAY 120

	/* Security Dynamics header files do not include prototypes
		for these 4 functions */
extern void creadcfg P__((void));
extern int sd_init P__((struct SD_CLIENT *sd));
extern int sd_check P__((char *passcode, char *name, struct SD_CLIENT *sd));
extern int sd_next P__((char *passcode, struct SD_CLIENT *sd));

extern void dbgSdClient P__((struct SD_CLIENT *sd));
extern int ace_eval P__((AuthInfo *authInfo, struct SD_CLIENT *sd));
extern int ace_next P__((AuthInfo *authInfo, struct SD_CLIENT *sd));
	/* request authentication via ACE */
extern int ace_pass P__((AuthInfo *authInfo, struct SD_CLIENT *sd));

union	config_record configure;

	/* useful definitions */
#define SD_PWD	"ACE"	/* Security Dynamics reserved password */
#define ACE_DUMMY_STATE	"ACE_DUMMY_STATE"
#define LEN_ACE_DUMMY_STATE	(sizeof(ACE_DUMMY_STATE)-1)

#define BEG_SD_1ST(sd)    (&(sd)->application_id)
#define END_SD_1ST(sd)    (&(sd)->application_id + sizeof((sd)->application_id))

#define BEG_SD_2ND(sd)    (&(sd)->passcode_time)
#define END_SD_2ND(sd)    (&(sd)->passcode_time + sizeof((sd)->passcode_time))

#define BEG_SD_3RD(sd)    ((sd)->ignition_key)
#define END_SD_3RD(sd)    (&(sd)->alphanumeric + sizeof((sd)->alphanumeric))

#define LEN_SD_1ST(sd)    ((ulong)END_SD_1ST((sd)) - (ulong)BEG_SD_1ST((sd)))
#define LEN_SD_2ND(sd)    ((ulong)END_SD_2ND((sd)) - (ulong)BEG_SD_2ND((sd)))
#define LEN_SD_3RD(sd)    ((ulong)END_SD_3RD((sd)) - (ulong)BEG_SD_3RD((sd)))

#define LEN_SD_TOTAL(sd)  (LEN_SD_1ST(sd) + LEN_SD_2ND(sd) + LEN_SD_3RD(sd) + sizeof(pid_t))

#define ACE_CHILDREN_ALLOC_CHUNK 16
#define ACE_CHILD_ALARM_PERIOD 300 /* 5 minutes */

/*-------------- Ace Interface End --------------*/

static int authAcePwd P__((AuthInfo *authInfo));
static pid_t is_ace_request P__((u_char *data, int length));
static AUTH_REQ *dequeue_authreq_by_pid P__((pid_t pid));

#endif /* ACE */

/*-------------- Digital Pathways Defender Interface Begin --------------*/

#if defined(DPI)

	/* Digital Pathways (DPI) Defender interface files */
#include "dslib.h"      /* interface information */
#include "dsagent.h"    /* definition information */

	/* forward function declarations */

extern void dpi_readcfg P__((void));
extern void cfg_key P__((void *p, char *ascii_key));
extern void cfg_str P__((void *p, char *string));
extern void cfg_int P__((void *p, char *string));
extern int dpi_init P__((void));
extern int dpi_read P__((int sid, char *buf, int len, int server_state, int f));
extern int dpi_challenge P__((int session_id, char *buf, int len));
extern int dpi_notify P__((int sid, int error, long tid, void *buf));
extern int authDPIPwd P__((AuthInfo *authInfo));

	/* useful definitions */
#ifndef ABORT
#define ABORT           2
#endif

#ifndef ERROR
#define ERROR           -1
#endif

#ifndef SUCCESS
#define SUCCESS         0
#endif

#define	DP_PWD	"DPI"	/* Digital Pathway's Defender reserved password */

#define RLSIZE          128
#define DPI_MAXSES	2	/* Maximum simultaneous DSS sessions */
char *dpi_filename = "agent.cf";

/* variable declarations required for dpi_param[] */
int dss_port = 2626;
int dss_timeout = 60;
char *key;
char *agent_id;
char *dss_addr;

struct dpi_param {
	char *string;
	void (*func)(void *, char *);
	void *target;
};

struct dpi_param dpi_param[] = {
	{"agentkey", cfg_key, (void *)&key},
	{"agentid", cfg_str, (void *)&agent_id},
	{"dss_address", cfg_str, (void *)&dss_addr},
	{"dss_port", cfg_int, (void *)&dss_port},
	{"dss_timeout", cfg_int, (void *)&dss_timeout},
	{NULL, NULL, NULL},
};

struct dpi_state {
	int      sessionid;
	int      transactionid;
	int      done;
	int      result;
	char     *dialog;
	AuthInfo *authInfo;
} ;

struct dpi_state dpi_sessions[DPI_MAXSES];

#endif /* DPI */

/*-------------- Digital Pathways Defender Interface End --------------*/

#define MAX_RCVBUF_SIZE		(4096)
#if defined( ORIG )
# define MAX_SNDBUF_SIZE	(4096)
#else
# define MAX_SNDBUF_SIZE	(8192)
#endif

	/* Order of these enum values is important! */
enum acct_flags {
	ACCT_NONE,
	ACCT_BY_NAME,
	ACCT_ANY
};

char		*progname;
int		sockfd = -1;
int		acctfd = -1;
int		ipadfd = -1;
int		rdCachefd = -1;
int		wrCachefd = -1;
enum acct_flags	acct_flag;
int		debug_flag;
int		spawn_flag;
int		acct_pid;
CONST char	*radius_dir;
CONST char	*radius_users;
CONST char	*radacct_dir;
UINT4		expiration_seconds;
UINT4		warning_seconds;
int		allow_passchange;
int		allow_token_caching;
CACHE		*cache;
extern int	errno;
static AUTH_REQ	*first_request;
extern int	warning;
int		retransmit_flag = FALSE;
int		suicide_flag = FALSE;
int		salt;
char		**sql_connect_strings;
char		*sql_filename = "sqlusers.cf";

/**********************************************************************/

int
main(argc, argv)
	int		argc;
	char		**argv;
{
	int			result;
	struct sockaddr_in	salocal;
	struct servent		*svp;
	u_short                 lport;
	char			argval;
	int			t;
	int			pid;
	int			cons;
	fd_set			readfds;
	int			status;
	int			sockOptFlag;

	progname = *argv++;
	argc--;

	debug_flag = 0;
	spawn_flag = 1;
	radacct_dir = RADACCT_DIR;
	radius_dir = RADIUS_DIR;
	radius_users = RADIUS_USERS;
	errf = stdout;	/* or stderr */
	warning = 0;
	allow_passchange = 0;
	allow_token_caching = 0;
	acct_flag = ACCT_ANY;

	signal(SIGHUP, SIG_DFL);
	signal(SIGINT, sig_fatal);
	signal(SIGQUIT, sig_fatal);
	signal(SIGILL, sig_fatal);
	signal(SIGTRAP, sig_fatal);
	signal(SIGIOT, sig_fatal);
	signal(SIGFPE, sig_fatal);
	signal(SIGTERM, sig_fatal);
	signal(SIGCHLD, sig_cleanup);

	while(argc) {

		if( **argv != '-' ) {
			usage();
		}

		argval = *(*argv + 1);
		argc--;
		argv++;

		switch(argval) {

		case 'A':
			if(argc == 0) {
				usage();
			}
			if( !strcmp(*argv, "none") ) {
				acct_flag = ACCT_NONE;
			}
			else if( !strcmp(*argv, "services") ) {
				acct_flag = ACCT_BY_NAME;
			}
			else if( !strcmp(*argv, "incr") ) {
				acct_flag = ACCT_ANY;
			}
			else {
				usage();
			}
			argc--;
			argv++;
			break;

		case 'a':
			if(argc == 0) {
				usage();
			}
			radacct_dir = *argv;
			argc--;
			argv++;
			break;

		case 'c':
			allow_token_caching = 1;
			break;

		case 'd':
			if(argc == 0) {
				usage();
			}
			radius_dir = *argv;
			argc--;
			argv++;
			break;

		case 'p':
			allow_passchange = 1;
			break;

		case 's':	/* Single process mode */
			spawn_flag = 0;
			break;

		case 'u':
			if(argc == 0) {
				usage();
			}
			radius_users = *argv;
			argc--;
			argv++;
			break;

		case 'v':
			radversion();
			break;

		case 'w':
			warning = 1;
			break;

		case 'x':
			debug_flag = 1;
			debugf ("Debugging enabled\n");
			break;

		default:
			usage();
			break;
		}
	}

	/* Initialize the dictionary */
	if( (result = dict_init()) != 0) {
		exit(result);
	}

	/* Initialize Configuration Values */
	if( (result = config_init()) != 0) {
		exit(result);
	}

	/* Initialize DES, once and for all. */
	if ( (result = des_init()) != 0) {
		exit(result);
	}

	/* Init token caching */
	if( allow_token_caching ) {
		int	pd[2];

		cache = MALLOC (CACHE, 1);
		if( !cache ) {
			log_err("Cache Creation Failed - memory exhausted\n");
			exit(MEMORY_ERR);
		}
		cache_init(cache, BUCKETS);
		if( spawn_flag ) {
			status = pipe(pd);
			if( status != 0 ) {
				(void)perror("Pipe Creation Failed:");
				log_err("Pipe Creation Failed: %s\n", xstrerror (errno));
				exit(PIPE_CREATE_ERR);
			}
			else {
				rdCachefd = pd[0];
				wrCachefd = pd[1];
			}
		}
	}

	svp = getservbyname ("radius", "udp");
	if (svp == (struct servent *) 0) {
		fprintf (errf, "%s: No such service: radius/udp\n",
			progname);
		exit(-1);
	}
	lport = (u_short) svp->s_port;

	sockfd = socket (AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		(void) perror ("auth socket");
		exit(-1);
	}

	sockOptFlag = 1;	/* enable */
	if ( setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR,
		     (char*) &sockOptFlag, sizeof(sockOptFlag)) < 0 ) {
		(void)perror("setsockopt auth SO_REUSEADDR");
		exit(-1);
	}

	memset (&salocal, '\0', sizeof (salocal));
	salocal.sin_family = AF_INET;
	salocal.sin_addr.s_addr = INADDR_ANY;
	salocal.sin_port = lport;

	result = bind (sockfd, (struct sockaddr *) &salocal, sizeof (salocal));
	if (result < 0) {
		(void) perror ("auth bind");
		exit(-1);
	}

	/*
	 * Open Accounting Socket.
	 */
	if( acct_flag >= ACCT_BY_NAME ) {
		svp = getservbyname ("radacct", "udp");
		if( svp != (struct servent *) 0 ) {
			lport = (u_short) svp->s_port;
		}
		else {
			fprintf (errf, "%s: Service %s/%s not defined ",
				progname, "radacct", "udp");

			if( acct_flag >= ACCT_ANY ) {
				lport = htons(ntohs(lport) +1);
				fprintf(errf, "-Using port %d\n", ntohs(lport));
			}
			else {
				fprintf(errf, "-Aborting\n");
				exit(-1);
			}
		}

		acctfd = socket (AF_INET, SOCK_DGRAM, 0);
		if (acctfd < 0) {
			(void) perror ("acct socket");
			exit(-1);
		}

		sockOptFlag = 1;	/* enable */
		if ( setsockopt( acctfd, SOL_SOCKET, SO_REUSEADDR,
		     (char*) &sockOptFlag, sizeof(sockOptFlag)) < 0 ) {
			(void)perror("setsockopt acct SO_REUSEADDR");
			exit(-1);
		}

		memset (&salocal, '\0', sizeof (salocal));
		salocal.sin_family = AF_INET;
		salocal.sin_addr.s_addr = INADDR_ANY;
		salocal.sin_port = lport;

		result = bind (acctfd, (struct sockaddr *) &salocal,
			       sizeof (salocal));
		if (result < 0) {
			(void) perror ("acct bind");
			exit(-1);
		}
	}

	/*
	 *	Disconnect from session
	 */
	if(debug_flag == 0) {
		pid = fork();
		if(pid < 0) {
			fprintf(errf, "%s: Couldn't fork: %s\n",
						progname, xstrerror (errno));
			exit(-1);
		}
		if(pid > 0) {
			exit(0);
		}
	}

	/*
	 *	Disconnect from tty
	 */
	for (t = 32; t >= 3; t--) {
		if(t != sockfd
		   && t != acctfd
		   && t != rdCachefd
		   && t != wrCachefd
		   && t != ipadfd ) {
			close(t);
		}
	}

#if !defined(M_UNIX)
	/*
	 * Open system console as stderr
	 */
	cons = open("/dev/console", O_WRONLY | O_NOCTTY);
	if(cons != 2) {
		dup2(cons, 2);
		close(cons);
	}
#endif
	/*
	 * If we are able to spawn processes, we will start a child
	 * to listen for Accounting requests.  If not, we will
	 * listen for them ourself.
	 */
	if( spawn_flag && (acctfd >= 0) ) {
		acct_pid = fork();
		if(acct_pid < 0) {
			fprintf(errf, "%s: Couldn't fork: %s\n",
						progname, xstrerror (errno));
			exit(-1);
		}
		if(acct_pid > 0) {
			close(acctfd);
			acctfd = -1;
		}
		else {
			if( sockfd > 0 ) {
				close(sockfd);
				sockfd = -1;
			}
			if( ipadfd > 0 ) {
				close(ipadfd);
				ipadfd = -1;
			}
			if( allow_token_caching ) {
				free(cache);
				close(rdCachefd);
				rdCachefd = -1;
				close(wrCachefd);
				wrCachefd = -1;
			}
		}
	}


	/*
	 *	Receive user requests
	 */
	for(;;) {
		FD_ZERO(&readfds);
		if(sockfd >= 0) {
			FD_SET(sockfd, &readfds);
		}
		if(acctfd >= 0) {
			FD_SET(acctfd, &readfds);
		}
		if( rdCachefd >= 0 ) {
			FD_SET(rdCachefd, &readfds);
		}
		if (ipadfd >= 0) {
			FD_SET(ipadfd, &readfds);
		}

		status = select(32, &readfds, NULL, NULL, NULL);
		if(status == -1) {
			if (errno == EINTR)
				continue;
			(void)perror("main:select");
			fprintf(errf, "%s: select status=%d\n",
				progname, status);
			sig_fatal(101);
		}
		if (ipadfd >= 0 && FD_ISSET(ipadfd, &readfds)) {
			if (!handle_radipa_response (ipadfd, sockfd)) {
				close (ipadfd);
				ipadfd = -1;
			}
		}
		if( rdCachefd >= 0 && FD_ISSET(rdCachefd, &readfds) ) {
			CacheMsg	msg;
			int		n = 0;

			while( n != sizeof(msg) ) {
				n += read(rdCachefd, ((char *)&msg)+n,
					sizeof(msg)-n);
			}
			switch( msg.type ) {
			case CACHE_INSERT:
				status = cache_insert(cache,
						msg.key, strlen(msg.key)+1,
						msg.val, strlen(msg.val)+1,
						msg.time, msg.idle);
				if (!status)
					log_err("Insert cache <%s> FAILED\n",
						msg.key);
				break;
			case CACHE_DELETE:
				status = cache_delete(cache,
						msg.key, strlen(msg.key)+1);
				if (!status)
					log_err("Delete cache <%s> FAILED\n",
						msg.key);
				break;
			case CACHE_IDLE_UPDATE:
				status = cache_idle_update(cache,
						msg.key, strlen(msg.key)+1,
						msg.idle);
				if (!status)
					log_err("Idle Update cache <%s> FAILED\n",
						msg.key);
				break;
			default:
				status = FALSE;
				log_err("Unknown cache msg type %d\n",
					msg.type);
				break;
			}
		}
		if(sockfd >= 0 && FD_ISSET(sockfd, &readfds)) {
			handle_radius_request (sockfd);
		}
		if(acctfd >=0 && FD_ISSET(acctfd, &readfds)) {
			handle_radius_request (acctfd);
		}
	}
	/*NOTREACHED*/
	return 0;
}

/* Read and process an incoming radius request of any kind.  Filter
   duplicate requests and spawn a child to do the dirty work if we're
   running multi-threaded.  */

static void
handle_radius_request (fd)
	int		fd;
{
	char		recv_buffer[MAX_RCVBUF_SIZE];
	AUTH_HDR	*auth = (AUTH_HDR *) recv_buffer;
	AUTH_REQ	*authreq;
	struct sockaddr_in saremote;
	int		salen = sizeof (saremote);
	int		length;
	int		isAcctPkt;

	length = recvfrom (fd, recv_buffer, sizeof (recv_buffer), 0,
			   (struct sockaddr *) &saremote, &salen);
	if (length < 0) {
		if (errno != EINTR) {
			log_err("recvfrom failed: %s\n", xstrerror (errno));
		}
		return;
	} else if (length == 0) {
		log_err("recvfrom returned empty packet\n");
		return;
	}

	authreq = CALLOC (AUTH_REQ, 1);
	if (authreq == NULL_REQ) {
		fprintf(errf, "%s: no memory\n", progname);
		exit(MEMORY_ERR);
	}

	authreq->ipaddr = ntohl (saremote.sin_addr.s_addr);
	authreq->udp_port = ntohs (saremote.sin_port);
	authreq->id = auth->id;
	authreq->code = auth->code;
	authreq->timestamp = (UINT4) time(0);
	authreq->next = NULL_REQ;
	authreq->secret[0] = '\0';
	authreq->request = NULL_PAIR;
	authreq->child_pid = -1;
	authreq->answer = NULL;
	authreq->answer_length = 0;
#if defined(ACE)
	authreq->pipe_fd = -1;
#endif

	if (forward_duplicate_request (authreq, fd)) {
		return;
	}
	DEBUG("handle_radius_request: %s, code=%d, length=%d\n",
	      request_id (authreq), auth->code, ntohs (auth->length));

	if (spawn_flag && auth->code == PW_AUTHENTICATION_REQUEST) {
		int pipe_fds[2];
#if defined(ACE)
		pid_t pid = is_ace_request (auth->data, length - AUTH_HDR_LEN);
		if (pid > 0) {
			/* This is an ACE next passcode request.
			   Because the ACE library is stateful, the
			   next passcode must be handled by the same
			   process that handled the original
			   authentication, so we must forward the
			   packet to that process.  */
			AUTH_REQ *origreq = dequeue_authreq_by_pid (pid);
			authreq->child_pid = pid;
			authreq->pipe_fd = origreq->pipe_fd;
			origreq->pipe_fd = -1;
			free_authreq (origreq);
			DEBUG("handle_radius_request: forward %d bytes to ACE pid %ld\n",
			      length, pid);
			if (write (authreq->pipe_fd, recv_buffer, length) < 0) {
				log_err ("handle_radius_request: can't write to pipe: %s\n",
					 xstrerror (errno));
			}
			return;
		} else if (pid < 0) {
			/* This isn't an ACE request, so we don't need
                           a pipe to the child process.  */
			pipe_fds[0] = pipe_fds[1] = -1;
		} else if (pipe (pipe_fds) < 0) {
			/* We needed a pipe, but failed to create it properly.  */
			log_err("Pipe Creation Failed: %s\n", xstrerror (errno));
			return;
		}
#endif
		if (!rad_spawn_child (authreq, pipe_fds)) {
			return;
		}
	}

	memcpy(authreq->vector, auth->vector, AUTH_VECTOR_LEN);
	isAcctPkt = parse_authreq (authreq, auth->data, length - AUTH_HDR_LEN);
	if (auth->code == PW_ACCOUNTING_REQUEST && isAcctPkt
	    && !authenticateAcctReq(authreq, auth)) {
		dequeue_authreq (authreq);
		free_authreq (authreq);
		return;
	}

	radrespond (authreq, fd, isAcctPkt);

	if (spawn_flag && auth->code == PW_AUTHENTICATION_REQUEST) {
		rad_child_loop (authreq, fd);
	}
}

static void
rad_child_loop (authreq, fd)
	AUTH_REQ	*authreq;
	int		fd;
{
	char		recv_buffer[MAX_RCVBUF_SIZE];
	AUTH_HDR	*auth = (AUTH_HDR *) recv_buffer;

#if defined(ACE)
	if (authreq->pipe_fd >= 0) {
		for (;;) {
			int length;
			alarm (ACE_SUICIDE_DELAY);
			length = read (authreq->pipe_fd, recv_buffer,
				       sizeof (recv_buffer));
			if (length < 0) {
				if (errno == EINTR) {
					maybe_retransmit (authreq, fd);
					continue;
				}
				log_err ("rad_child_loop: can't read from pipe: %s\n",
					 xstrerror (errno));
				exit (1);
			}
			DEBUG("rad_child_loop: read %d bytes from pipe\n", length);
			authreq->id = auth->id;
			authreq->secret[0] = '\0';
			pairfree (authreq->request);
			if (authreq->answer) {
				free (authreq->answer);
			}
			memcpy(authreq->vector, auth->vector, AUTH_VECTOR_LEN);
			parse_authreq (authreq, auth->data, length - AUTH_HDR_LEN);
			if (rad_authenticate (authreq, fd) != -ACE_NEXT_PASSCODE) {
				break;
			}
		}
	}
#endif
	for (;;) {
		alarm (SUICIDE_DELAY);
		pause ();
		maybe_retransmit (authreq, fd);
	}
}

static void
maybe_retransmit (authreq, fd)
	AUTH_REQ	*authreq;
	int		fd;
{
	if (suicide_flag) {
		DEBUG("Child process exiting normally\n");
		exit (0);
	}
	if (retransmit_flag) {
		DEBUG("Retransmit %d bytes to %s\n",
		      authreq->answer_length, request_id (authreq));
		if( sendto (fd, (char *) authreq->answer,
		    authreq->answer_length, 0,
		    remote_sockaddr (authreq),
		    sizeof (struct sockaddr_in)) < 0 ) {
			log_err ("Can't re-send response: %s (fd %d)\n",
				xstrerror (errno), fd);
		}
		retransmit_flag = FALSE;
	}
}

/*************************************************************************
 *
 *	Function: parse_authreq
 *
 *	Purpose: Receive UDP client requests, build an authorization request
 *		 structure, and attach attribute-value pairs contained in
 *		 the request to the new structure.
 *
 *************************************************************************/

static int
parse_authreq(authreq, ptr, length)
	AUTH_REQ	*authreq;
	u_char		*ptr;
	int		length;
{
	int		isAcctPkt = FALSE;
	int		attribute;
	int		attrlen;
	DICT_ATTR	*attr;
	UINT4		lvalue;
	VALUE_PAIR	*first_pair;
	VALUE_PAIR	*prev;
	VALUE_PAIR	*pair;

	/*
	 * Extract attribute-value pairs
	 */
	first_pair = NULL_PAIR;
	prev = NULL_PAIR;

	while(length > 0) {

		attribute = *ptr++;
		attrlen = *ptr++;
		if(attrlen < 2) {
			length = 0;
			continue;
		}
		attrlen -= 2;
		if((attr = dict_attrget(attribute)) == (DICT_ATTR *)NULL) {
		    DEBUG("Received unknown attribute %d\n", attribute);
		}
		else if ( attrlen > AUTH_STRING_LEN ) {
		    DEBUG("attribute %d too long, %d >= %d\n", attribute,
			attrlen, AUTH_STRING_LEN);
		}
		else {
			if( attribute == PW_ACCT_STATUS_TYPE ) {
				isAcctPkt = TRUE;
			}

			pair = make_pair (attr->name, attr->value, attr->type);
			pair->size = attrlen;
			pair->next = NULL_PAIR;

			switch(attr->type) {

			case PW_TYPE_STRING:
			case PW_TYPE_PHONESTRING:
				memcpy(pair->strvalue, ptr, attrlen);
				pair->strvalue[attrlen] = '\0';
				break;

			case PW_TYPE_INTEGER:
			case PW_TYPE_IPADDR:
				memcpy(&lvalue, ptr, sizeof(UINT4));
				pair->lvalue = ntohl(lvalue);
				break;
#if defined( BINARY_FILTERS )
			case PW_TYPE_FILTER_BINARY:
				memcpy(pair->strvalue, ptr, attrlen);
				pair->strvalue[attrlen] = '\0';
				break;
#endif
			default:
				DEBUG("    %s (Unknown Type %d)\n",
					attr->name,attr->type);
				free(pair);
				pair = NULL_PAIR;
				break;
			}
			if (pair) {
				/* temp. assign to ASCEND_SPECIAL_STRING
				 * so that the printing will be in hex
				 */
				if((attribute == ASCEND_SESSION_SVR_KEY) || (attribute == ASCEND_NUMBER_SESSIONS)) {
					pair->type = ASCEND_SPECIAL_STRING;
				}
				debug_pair("request", stdout, pair);
				if( attribute == ASCEND_SESSION_SVR_KEY ) {
					pair->type = PW_TYPE_STRING;
				}
				if(first_pair == NULL_PAIR) {
					first_pair = pair;
				} else {
					prev->next = pair;
				}
				prev = pair;
			}
		}
		ptr += attrlen;
		length -= attrlen + 2;
	}
	authreq->request = first_pair;

	return isAcctPkt;
}

/* Remove an authentication request from our queue of pending requests.  */

static void
dequeue_authreq (authreq)
	AUTH_REQ *authreq;
{
	AUTH_REQ **req_ptr = &first_request;

	while (*req_ptr) {
		AUTH_REQ *curreq = *req_ptr;
		if (curreq == authreq) {
			DEBUG("Free authreq: %s\n", request_id (curreq));
			*req_ptr = curreq->next;
			break;
		}
		req_ptr = &curreq->next;
	}
}


/* Free the heap storate associated with an authentication request, and close
   the pipe if one is open.  */

static void
free_authreq (authreq)
	AUTH_REQ *authreq;
{
	if (authreq->answer) {
		free (authreq->answer);
	}
#if defined(ACE)
	if (authreq->pipe_fd >= 0) {
		close (authreq->pipe_fd);
	}
#endif
	pairfree (authreq->request);
	free (authreq);
}

/*************************************************************************
 *
 *	Function: authenticateAcctReq
 *
 *	Purpose: Authenticate an accounting request packet
 *
 *************************************************************************/

int
authenticateAcctReq(authreq, auth)
	AUTH_REQ	*authreq;
	AUTH_HDR	*auth;
{
	u_char		*buffer;
	int		totallen;
	u_char		saveDigest[AUTH_VECTOR_LEN];
	u_char		digest[AUTH_VECTOR_LEN];
	int		secretlen;
	u_char		secret[256];
	char		tmpbuf[256];
	UINT4		ipaddr;
	int		status;

	totallen = ntohs(auth->length);
	status = get_client_info( authreq->ipaddr, &ipaddr, secret, tmpbuf );
	secretlen = strlen( (CONST char *)secret );
	if( status != 0 ) {
		log_err("authenticateAcctReq: %s: %s\n",
			request_id (authreq), get_errmsg(status));
		memset(secret, 0, sizeof(secret));
		return FALSE;
	}
	if( totallen + secretlen > MAX_RCVBUF_SIZE ) {
		log_err("authenticateAcctReq: %s: length+secret too long\n",
			request_id (authreq));
		return FALSE;
	}
	else {
		strcpy((char *)authreq->secret, (CONST char *)secret);
		memcpy( saveDigest, auth->vector, AUTH_VECTOR_LEN );
		memset( auth->vector, 0, AUTH_VECTOR_LEN );
		buffer = (u_char *)auth;
		memcpy( &buffer[totallen], secret, secretlen );
		md5_calc( digest, (u_char *)auth, totallen + secretlen );
		if ( memcmp( digest, saveDigest, AUTH_VECTOR_LEN) ) {
			log_err ("authenticateAcctReq: %s: bad authenticator\n",
				 request_id (authreq));
			return FALSE;
		}
	}

	return TRUE;
}

/*************************************************************************
 *
 *	Function: radrespond
 *
 *	Purpose: Respond to supported requests:
 *
 *		 PW_AUTHENTICATION_REQUEST - Authentication request from
 *				a client network access server.
 *
 *		 PW_ACCOUNTING_REQUEST - Accounting request from
 *				a client network access server.
 *
 *		 PW_PASSWORD_REQUEST - User request to change a password.
 *
 *************************************************************************/

void
radrespond(authreq, activefd, isAcctPkt)
	AUTH_REQ	*authreq;
	int		activefd;
	int		isAcctPkt;
{
	int		result;

	switch(authreq->code) {

	case PW_AUTHENTICATION_REQUEST:
		result = rad_authenticate (authreq, activefd);
#if defined(ACE)
		if (result != -ACE_NEXT_PASSCODE && authreq->pipe_fd >= 0) {
			close (authreq->pipe_fd);
			authreq->pipe_fd = -1;
		}
#endif
		break;

	case PW_ACCOUNTING_REQUEST:
		if( isAcctPkt ) {
			rad_accounting(authreq, activefd, PW_ACCOUNTING_RESPONSE);
		}
#if defined( ASCEND_LOGOUT )
		else {
			send_logout_response(authreq, activefd);
		}
#endif /* ASCEND_LOGOUT */
		break;

	case PW_PASSWORD_REQUEST:
		rad_passchange(authreq, activefd);
		break;

	case PW_ASCEND_RADIPA_ALLOCATE:
		allocate_ip_address_from_global_pool (authreq, activefd);
		break;

	case PW_ASCEND_RADIPA_RELEASE:
		release_ip_address_to_global_pool (authreq, activefd);
		break;

	case PW_ASCEND_EVENT_REQUEST:
		rad_accounting(authreq, activefd, PW_ASCEND_EVENT_RESPONSE);
		break;

	default:
		DEBUG("Unknown request, code=%d\n", authreq->code);
		break;
	}
}

/* Scan our list of pending requests (authentication, accounting, and
   address allocation).  Weed out requests that have died of old age.
   If the given AUTHREQ is already on our list, retransmit our reply
   if we have one, and discard the request as a duplicate if we don't
   yet have a reply.  If we spawned a child to handle the request,
   send it a SIGHUP every time the duplicate request arrives here,
   prompting the child to retransmit the reply.  If the request was a
   duplicate which we handled as above, return TRUE to notify our
   caller that no further action is required.  */

static int
forward_duplicate_request (newreq, activefd)
	AUTH_REQ *newreq;
	int activefd;
{
	int request_count = 0;
	AUTH_REQ **req_ptr = &first_request;

	while (*req_ptr) {
		AUTH_REQ *oldreq = *req_ptr;

		if (oldreq->timestamp + CLEANUP_DELAY <= newreq->timestamp) {
			if (oldreq->child_pid < 0) {
				DEBUG("Free aged authreq: %s\n", request_id (oldreq));
				*req_ptr = oldreq->next;
				free_authreq (oldreq);
				continue;
			}
			if (spawn_flag && oldreq->child_pid > 0) {
				DEBUG("Killing child pid %ld\n", oldreq->child_pid);
				kill (oldreq->child_pid, SIGTERM);
			}
		} else if (oldreq->id == newreq->id
		    && oldreq->ipaddr == newreq->ipaddr
		    && oldreq->udp_port == newreq->udp_port) {
			if (oldreq->child_pid > 0) {
				DEBUG("Asking child %d to retransmit: %s\n",
				      oldreq->child_pid, request_id (oldreq));				
				kill (oldreq->child_pid, SIGHUP);
			} else if (oldreq->answer) {
				DEBUG("Retransmit %d bytes to %s\n",
				      oldreq->answer_length, request_id (oldreq));
				sendto(activefd, (char *) oldreq->answer,
				       oldreq->answer_length, 0,
				       remote_sockaddr (oldreq),
				       sizeof(struct sockaddr_in));
			} else {
				/* We have no response to retransmit,
				   so drop the duplicate */
				log_err ("Drop duplicate: %s\n",
					 request_id (oldreq));
			}
			if ( ((oldreq->child_pid > 0) || (oldreq->answer )) &&
			       ( oldreq->timestamp < newreq->timestamp) ) {
				DEBUG("Increase timeout by %d seconds: %s\n",
				      newreq->timestamp - oldreq->timestamp,
				      request_id (oldreq));
				oldreq->timestamp = newreq->timestamp;
			}
			free (newreq);
			return TRUE;
		}
		request_count++;
		req_ptr = &oldreq->next;
	}

	/* This is a new request */
	if (request_count > REQUESTS_BACKLOG_THRESHOLD) {
		log_err ("Warning: backlog of %d exceeds %d requests\n",
			 request_count, REQUESTS_BACKLOG_THRESHOLD);
	}
	DEBUG("New request: %s\n", request_id (newreq));
	*req_ptr = newreq;

	return FALSE;
}

/*************************************************************************
 *
 *	Function: rad_spawn_child
 *
 *	Purpose: Spawns child processes to perform password authentication
 *		 and respond to RADIUS clients.  This functions also
 *		 cleans up complete child requests, and verifies that there
 *		 is only one process responding to each request (duplicate
 *		 requests are filtered out.
 *
 *************************************************************************/

static int
rad_spawn_child (authreq, pipe_fds)
	AUTH_REQ	*authreq;
	int		*pipe_fds;
{
	pid_t		pid;
	int		fd;

	pid = fork ();
	if(pid < 0) {
		log_err("Fork failed: %s: %s\n",
			request_id (authreq), xstrerror (errno));
		return FALSE;
	}
	DEBUG("fork in rad_spawn_child (%s)\n", pid ? "parent" : "child");
	if (pid) {
		authreq->child_pid = pid;
#if defined(ACE)
		if (pipe_fds[0] >= 0) {
			close (pipe_fds[0]);
			authreq->pipe_fd = pipe_fds[1];
			/* ACE children need to hang around much longer,
			   in case a next-passcode request arrives.  */
			authreq->timestamp += ACE_SUICIDE_DELAY;
		}
#endif
		return FALSE;
	}

	/*
	 * From now on, we're running as the child.
	 * Note:  This closes every fd of which the OS can conceive,
	 * 	not only those that are open.  On BSDI this may entail
	 * 	closing over 16,000 never-opened files.  What would be
	 *	the problem with *not* doing this...?
	 */
	for (fd = get_NFILE () - 1; fd >= 3; fd--) {
		if (fd != sockfd
		   && fd != acctfd
#if defined(ACE)
		    && fd != pipe_fds[0]
#endif
		    && fd != wrCachefd) {
			close(fd);
		}
	}
#if defined(ACE)
	authreq->pipe_fd = pipe_fds[0];
#endif
	rdCachefd = -1;
	ipadfd = -1;
	acctfd = -1;
	signal (SIGHUP, sig_hangup);
	signal (SIGTERM, sig_suicide);
	signal (SIGALRM, sig_suicide);

	return TRUE;
}

static int
get_NFILE()
{
#ifdef NFILE
	return NFILE;
#else
	struct rlimit rl;
	if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
		return 256;
	else
		return rl.rlim_max;
#endif
}

static void
sig_cleanup(sig)
	int		sig;
{
	int		status;
	pid_t		pid;
	AUTH_REQ	*curreq;

	(void)sig;	/* ignore param */

	for (;;) {
		pid = waitpid((pid_t)-1,&status,WNOHANG);
		signal(SIGCHLD, sig_cleanup);
		if (pid <= 0)
			return;

#if defined (aix)
		kill(pid, SIGKILL);
#endif

		if(pid == acct_pid) {
			sig_fatal(100);
		}
		curreq = first_request;
		while (curreq != NULL_REQ) {
			if(curreq->child_pid == pid) {
				curreq->child_pid = -1;
				curreq->timestamp = 0;
				break;
			}
			curreq = curreq->next;
		}
	}
}

void
rad_accounting(authreq, activefd, response_code)
	AUTH_REQ	*authreq;
	int		activefd;
	int             response_code;
{
	FILE		*outfp;
	VALUE_PAIR	*pair;

	/*
	 * Write Detail file.
	 */

	if((outfp = popen("/usr/sbin/radacct","w")) == (FILE *) NULL) {
		log_err( "Acct: Couldn't open pipe to detail logger\n");
	} else {
		/* Write each attribute/value to the log file */
		pair = authreq->request;
		while (pair != NULL_PAIR) {
			fputs("\t", outfp);
			fprint_attr_val(outfp, pair);
			fputs("\n", outfp);
			pair = pair->next;
		}
		pclose(outfp);
	}
	/* let NAS know it is OK to delete from buffer */
	if ( response_code == PW_ASCEND_EVENT_RESPONSE) {
	  send_ascend_event_response(authreq,activefd);
	}
	else {
	  send_acct_reply(authreq, NULL_PAIR, (char *)NULL,activefd, response_code);
	}
}

/*************************************************************************
 *
 *	Function: send_acct_reply
 *
 *	Purpose: Reply to the request with an ACKNOWLEDGE.  Also attach
 *		 reply attribute value pairs and any user message provided.
 *
 *************************************************************************/

void
send_acct_reply(authreq, reply, msg, activefd, response_code)
	AUTH_REQ	*authreq;
	VALUE_PAIR	*reply;
	CONST char	*msg;
	int		activefd;
	int             response_code;
{
	AUTH_HDR	*auth;
	u_short		length = AUTH_HDR_LEN;
	u_char		*ptr;
	int		len;
	UINT4		lvalue;

	DEBUG("send_acct_reply: %s\n", request_id (authreq));

	auth = make_send_buffer (authreq, response_code);

	/* Load up the configuration values for the user */
	ptr = auth->data;
	while (reply != NULL_PAIR) {
		debug_pair("account", stdout, reply);
		*ptr++ = reply->attribute;

		switch(reply->type) {

		case PW_TYPE_STRING:
		case PW_TYPE_PHONESTRING:
			len = strlen(reply->strvalue);
			*ptr++ = len + 2;
			strcpy((char *)ptr, reply->strvalue);
			ptr += len;
			length += len + 2;
			break;
			
		case PW_TYPE_INTEGER:
		case PW_TYPE_IPADDR:
			*ptr++ = sizeof(UINT4) + 2;
			lvalue = htonl(reply->lvalue);
			memcpy(ptr, &lvalue, sizeof(UINT4));
			ptr += sizeof(UINT4);
			length += sizeof(UINT4) + 2;
			break;

		default:
			break;
		}

		reply = reply->next;
	}

	length += append_user_message (ptr, msg, AUTH_STRING_LEN);
	send_answer (activefd, authreq, auth, length);
}

#if defined( ASCEND_LOGOUT )
/*************************************************************************
 *
 *      Function: send_logout_response
 *
 *      Purpose: Reply to the request with an ACKNOWLEDGE.  Also attach
 *               any user message provided.
 *
 *************************************************************************/

void
send_logout_response(authreq, activefd)
	AUTH_REQ	*authreq;
	int		activefd;
{
	AUTH_HDR        *auth;
	int             length = AUTH_HDR_LEN;
  	u_char          digest[AUTH_VECTOR_LEN];

	DEBUG("send_logout_response: %s\n", request_id (authreq));

	/* verify the client and calculate the MD5 Password digest*/
	if( calc_digest( digest, authreq ) ) {
		log_err( "send_logout_response: %s: Security\n",
				request_id (authreq) );
		return;
	}
	auth = make_send_buffer (authreq, PW_ACCOUNTING_RESPONSE);
	length += append_user_message (auth->data, "added by Ascend",
				       AUTH_STRING_LEN);
	send_answer (activefd, authreq, auth, length);
}
#endif /* ASCEND_LOGOUT */

/*************************************************************************
 *
 *      Function: send_ascend_event_response
 *
 *      Purpose: Reply to the request with an ACKNOWLEDGE.
 *
 *************************************************************************/

void
send_ascend_event_response(authreq, activefd)
	AUTH_REQ	*authreq;
	int		activefd;
{
	AUTH_HDR        *auth;
	int             length = AUTH_HDR_LEN;
  	u_char          digest[AUTH_VECTOR_LEN];

	DEBUG("send_ascend_event_response: %s\n", request_id (authreq));

	/* verify the client and calculate the MD5 Password digest*/
	if( calc_digest( digest, authreq ) ) {
		log_err( "send_ascend_event_response: %s: Security\n",
				request_id (authreq) );
		return;
	}
	auth = make_send_buffer (authreq, PW_ASCEND_EVENT_RESPONSE);
	send_answer (activefd, authreq, auth, length);
}

/*************************************************************************
 *
 *	Function: getPwdType
 *
 *	Purpose: Return the password type
 *
 *************************************************************************/

PasswordType
getPwdType(pwd)
	char		*pwd;
{
#if defined(SAFEWORD)
	if( strcmp(pwd, EL_PWD) == 0) {
		return PWD_TOKEN;
	}
#endif
#if defined(ACE)
	if( strcmp(pwd, SD_PWD) == 0) {
		return PWD_TOKEN;
	}
#endif
#if defined(DPI)
	if( strcmp(pwd, DP_PWD) == 0) {
		return PWD_TOKEN;
	}
#endif
	if( strcmp(pwd, UNIX_PWD) == 0) {
		return PWD_UNIX;
	}
	if( strcmp(pwd, SQL_PWD) ==0) {
		return PWD_SQL;
	}
	return PWD_RADIUS;
}

#if( 0 )
/*************************************************************************
 *
 *	Function: get_attribute
 *
 *	Purpose: Given an attribute list, an attribute identifier, and
 *		an indirect pointer to an attribute struct, return a
 *		pointer to the original list; the attribute
 *		struct pointer will be set to the matching attribute
 *		struct, if any.
 *
 *************************************************************************/

VALUE_PAIR *
get_attribute(list, attributeId, attribute)
	VALUE_PAIR	*list;
	int		attributeId;
	VALUE_PAIR	**attribute;
{
	VALUE_PAIR *cur = list;

	*attribute = (VALUE_PAIR *)0;

	while( cur ) {
		if( cur->attribute == attributeId ) {
			*attribute = cur;
			break;
		}
		else {
			cur = cur->next;
		}
	}
	return list;
}
#endif

/*************************************************************************
 *
 *	Function: cut_attribute
 *
 *	Purpose: Given an attribute list, an attribute identifier, and
 *		an indirect pointer to an attribute struct, return a list
 *		of all the attributes in the original list EXCEPT the first
 *		occurence of one matching the attribute id; the attribute
 *		struct pointer will be set to the matching attribute
 *		struct, if any.
 *
 *************************************************************************/

VALUE_PAIR *
cut_attribute(list, attributeId)
	VALUE_PAIR	**list;
	int		attributeId;
{
	while (*list) {
		VALUE_PAIR *pair = *list;
		if (pair->attribute == attributeId) {
			*list = pair->next;
			pair->next = NULL_PAIR;
			return pair;
		}
		list = &pair->next;
	}
	return NULL_PAIR;
}

/*************************************************************************
 *
 *	Function: get_attr_lvalue
 *
 *	Purpose: Given an attribute list and an attribute identifier
 *		return the the lvalue of the specified attribute
 *		or zero if the attribute is not in the list.
 *
 *		In other languages we could return the *value*, whether
 *		that was the lvalue or the strvalue, etc. but C mandates
 *		a single function return type.  There are of course ways
 *		around this but they are overkill for this function, and
 *		kind of ugly, as well.
 *
 *************************************************************************/

UINT4
get_attr_lvalue(list, attribute)
	VALUE_PAIR	*list;
	int		attribute;
{
	UINT4		lval;

	while (list != NULL_PAIR) {
		if(list->attribute == attribute) {
			break;
		}
		list = list->next;
	}

	if (list == NULL_PAIR) {
		lval = 0;
	}
	else {
		lval = list->lvalue;
	}

	DEBUG("get_attr_lvalue(%d) == %ld\n", attribute, lval);
	return lval;
}

/*************************************************************************
 *
 *	Function: get_attr_strvalue
 *
 *	Purpose: Given an attribute list and an attribute identifier
 *		return the the strvalue of the specified attribute
 *		or zero if the attribute is not in the list.
 *
 *************************************************************************/

char *
get_attr_strvalue(list, attribute)
	VALUE_PAIR	*list;
	int		attribute;
{
	char	*strval;

	while (list != NULL_PAIR) {
		if(list->attribute == attribute) {
			break;
		}
		list = list->next;
	}

	if (list == NULL_PAIR) {
		strval = (char *)0;
	}
	else {
		strval = list->strvalue;
	}

	DEBUG("get_attr_strvalue(%d) == %s\n", attribute, strval);
	return strval;
}

/*************************************************************************
 *
 *	Function: rad_passchange
 *
 *	Purpose: Change a users password
 *
 *************************************************************************/

void
rad_passchange(authreq, activefd)
	AUTH_REQ	*authreq;
	int		activefd;
{
	VALUE_PAIR	*check_item;
	VALUE_PAIR	*newpasspair = NULL_PAIR;
	VALUE_PAIR	*oldpasspair = NULL_PAIR;
	VALUE_PAIR	*curpass;
	VALUE_PAIR	*user_check = NULL_PAIR;
	VALUE_PAIR	*user_reply = NULL_PAIR;
	u_char		pw_digest[AUTH_VECTOR_LEN];
	char		string[512];
	char		passbuf[AUTH_PASS_LEN];
	char		*user_name;
	int		i;
	int		secretlen;
	int		vecLen;
	UINT4		user_expiration_days;
	UINT4		user_expiration_seconds;

	if( allow_passchange == 0 ) {
		log_err("Passchange: %s: Password Changing NOT Allowed\n",
				request_id (authreq));
		send_reject(authreq, (char *)NULL, activefd);
		pairfree (user_check);
		pairfree (user_reply);
		return;
	}

	/* Get the username */
	user_name = get_user_values (authreq, &user_check, &user_reply, "Passchange");
	if (user_name == (char *) NULL) {
		log_err ("Passchange: %s: No user name\n", request_id (authreq));
		send_reject(authreq, (char *)NULL, activefd);
		pairfree (user_check);
		pairfree (user_reply);
		return;
	}

	/*
	 * Validate the user -
	 *
	 * We have to unwrap this in a special way to decrypt the
	 * old and new passwords.  The MD5 calculation is based
	 * on the old password.  The vector is different.  The old
	 * password is encrypted using the encrypted new password
	 * as its vector.  The new password is encrypted using the
	 * random encryption vector in the request header.
	 */

	/* Extract the attr-value pairs for the old and new passwords */
	check_item = authreq->request;
	while (check_item != NULL_PAIR) {
		if(check_item->attribute == PW_PASSWORD) {
			newpasspair = check_item;
		}
		else if(check_item->attribute == PW_OLD_PASSWORD) {
			oldpasspair = check_item;
		}
		check_item = check_item->next;
	}

	/* Verify that both encrypted passwords were supplied */
	if (newpasspair == NULL_PAIR || oldpasspair == NULL_PAIR)
	{
		/* Missing one of the passwords */
		log_err("Passchange: %s: Missing Password for `%s'\n",
			request_id (authreq), user_name);
		send_reject(authreq, (char *)NULL, activefd);
		pairfree (user_check);
		pairfree (user_reply);
		return;
	}

	/* Get the current password from the database */
	curpass = user_check;
	while (curpass != NULL_PAIR) {
		if(curpass->attribute == PW_PASSWORD) {
			break;
		}
		curpass = curpass->next;
	}
	if ((curpass == NULL_PAIR) || curpass->strvalue == (char *)NULL) {
		/* Missing our local copy of the password */
		log_err("Passchange: %s: Missing Local Password for `%s'\n",
			request_id (authreq), user_name);
		send_reject(authreq, (char *)NULL, activefd);
		pairfree (user_check);
		pairfree (user_reply);
		return;
	}
	if( getPwdType(curpass->strvalue) != PWD_RADIUS ) {
		/* Can't change passwords that aren't in users file */
		log_err("Passchange: %s: system password change not allowed for `%s'\n",
				request_id (authreq), user_name);
		send_reject(authreq, (char *)NULL, activefd);
		pairfree (user_check);
		pairfree (user_reply);
		return;
	}

	/* Decrypt the old password */
	secretlen = strlen(curpass->strvalue);
	vecLen = newpasspair->size;
#if( 0 )
	DEBUG("strlen(newpasspair->strvalue): %d, newpasspair->size: %d\n",
		strlen(newpasspair->strvalue), newpasspair->size);
#endif
	memcpy(string, curpass->strvalue, secretlen);
	memcpy(string + secretlen, newpasspair->strvalue, vecLen);
	md5_calc(pw_digest, (u_char *)string, vecLen + secretlen);

	memcpy(passbuf, oldpasspair->strvalue, oldpasspair->size);
	for(i = 0;i < oldpasspair->size;i++) {
		passbuf[i] ^= pw_digest[i % vecLen];
	}

	/* Did they supply the correct password ??? */
	if(memcmp(passbuf, curpass->strvalue, curpass->size) != 0) {
		log_err("Passchange: %s: Bad password for `%s'\n",
				request_id (authreq), user_name);
		DEBUG_PWD (("Passchange: Exp Pwd(%d, %.*s); User Exp Pwd(%d, %.*s)\n",
			    curpass->size, curpass->size, curpass->strvalue,
			    newpasspair->size, newpasspair->size, passbuf));
		send_reject(authreq, (char *)NULL, activefd);
		pairfree (user_check);
		pairfree (user_reply);
		return;
	}

	/* Decrypt the new password */
	memcpy(string, curpass->strvalue, secretlen);
	memcpy(string + secretlen, authreq->vector, AUTH_VECTOR_LEN);
	md5_calc(pw_digest, (u_char *)string, AUTH_VECTOR_LEN + secretlen);
	memcpy(passbuf, newpasspair->strvalue, newpasspair->size);
	for( i = 0; i < newpasspair->size; i++ ) {
		passbuf[i] ^= pw_digest[i % AUTH_VECTOR_LEN];
	}

	/* Update the users password */
	/* Note: It is impossible for newpasspair->size
	 *	to be larger than sizeof(curpass->strvalue), so
	 *	this strncpy is safe
	 *
	 *	Also, we *always* have room for the terminating NUL
	 */
	strncpy(curpass->strvalue, passbuf, newpasspair->size);
	curpass->strvalue[newpasspair->size] = 0;

	/* Look for an existing expiration duration entry */
	user_expiration_days = get_attr_lvalue(user_reply, ASCEND_PW_LIFETIME);
	user_expiration_seconds = user_expiration_days * (UINT4)SECONDS_PER_DAY;

	/* Add a new expiration date if we are aging passwords */
	if( user_expiration_seconds != (UINT4)0 ) {
		set_expiration(user_check, user_expiration_seconds);
	}
	else if( expiration_seconds != (UINT4)0 ) {
		set_expiration(user_check, expiration_seconds);
	}

	/* Update the database */
	if(user_update(user_name, user_check, user_reply) != 0) {
		log_err("Passchange: %s: unable to update password for `%s'\n",
			request_id (authreq), user_name);
		send_reject(authreq, (char *)NULL, activefd);
		pairfree (user_check);
		pairfree (user_reply);
		return;
	}
	send_pwack(authreq, activefd);
	pairfree (user_check);
	pairfree (user_reply);
}

/*************************************************************************
 *
 *	Function: authCleanup
 *
 *	Purpose: Standard cleanup code when auth is finished
 *
 *************************************************************************/

void
authCleanup( authInfo, checkList, replyList )
	AuthInfo	*authInfo;
	VALUE_PAIR	*checkList;
	VALUE_PAIR	*replyList;
{
	pairfree(authInfo->cutList);
	authInfo->cutList = (VALUE_PAIR *)0;
	pairfree( checkList );
	pairfree( replyList );
	authInfo->authreq = NULL_REQ;
	/* Don't free authInfo->authreq.   forward_duplicate_request()
	   will do so when the danger of retransmissions has passed. */
}

#if defined(SAFEWORD)
/*************************************************************************
 *
 *	Function: authSafewordPwd
 *
 *	Purpose: Setup for and invoke a SafeWord authentication request
 *
 *************************************************************************/
int
authSafewordPwd(authInfo)
	AuthInfo	*authInfo;
{
	struct pblk *pb;

	DEBUG("authSafewordPwd\n");
    	authInfo->user_msg = (char *)NULL;
	pb = MALLOC (struct pblk, 1);
	if( !pb ) {
		log_err("authSafewordPwd: malloc err for user %s\n",
			authInfo->authName->strvalue);
		authInfo->user_msg = (char *)NULL;
		return -1;
	}

	return do_safeword_pass(authInfo, pb);
}

/*************************************************************************
 *
 *	Function: do_safeword_pass
 *
 *	Purpose: Process and reply to a SafeWord authentication request
 *
 *************************************************************************/

int
do_safeword_pass(authInfo, pb)
	AuthInfo	*authInfo;
	struct pblk	*pb;
{
	char stateInfo[AUTH_STRING_LEN];
	char *si;
	int result;
	CONST char *chalMsg = "Challenge: %s\r\nEnter PASSCODE: ";
	char *ncMsg;


	result = safeword_pass(authInfo, pb);
	switch( result ) {
	default:
		log_err("safeword_pass: %s: BIZARRE result %d, user %s\n",
			request_id (authInfo->authreq),
			result, authInfo->authName->strvalue);
		authInfo->user_msg = (char *)NULL;
		result = SAFEWORD_FAILED;
		break;

	case SAFEWORD_FAILED:
		log_err("safeword_pass: %s: FAILED for user %s\n",
			request_id (authInfo->authreq),
			authInfo->authName->strvalue);
		authInfo->user_msg = (char *)NULL;
		result = SAFEWORD_FAILED;
		break;

	case SAFEWORD_CHALLENGING:
		DEBUG("safeword_pass CHALLENGING\n");
		if( authInfo->chkImmediate ) {
				/* short-circuit challenge */
			si = authInfo->authState->strvalue;
    			memcpy(si, BEG_PB_1ST(pb), LEN_PB_1ST(pb));
			si += LEN_PB_1ST(pb);
    			memcpy(si, BEG_PB_2ND(pb), LEN_PB_2ND(pb));
			authInfo->authState->size = LEN_PB_TOTAL(pb);
				/* once only, please! */
			authInfo->chkImmediate = (VALUE_PAIR *)0;
				/* NB: pb is zeroed and freed by this
					tail-recursive call) */
			return do_safeword_pass(authInfo, pb);
		}
		else {
			/* allocate a buffer big enough for the challenge str */

			ncMsg = malloc(strlen(chalMsg) + strlen(pb->chal) + 1);

			if( !ncMsg ) {
				fprintf(errf, "%s: no memory\n", progname);
				exit(MEMORY_ERR);
			}

			sprintf(ncMsg, chalMsg, pb->chal);
			si = stateInfo;
    			memcpy(si, BEG_PB_1ST(pb), LEN_PB_1ST(pb));
			si += LEN_PB_1ST(pb);
    			memcpy(si, BEG_PB_2ND(pb), LEN_PB_2ND(pb));
			send_challenge(	authInfo->authreq,
					ncMsg, strlen(ncMsg),
					stateInfo, LEN_PB_TOTAL(pb),
					authInfo->fd);
			free(ncMsg);
			result = -SAFEWORD_CHALLENGING;
		}
		break;

	case SAFEWORD_PASSED:
		DEBUG("safeword_pass PASSED\n");
		result = 0;
		break;
	}
	memset(pb, 0, sizeof(*pb));
	free(pb);
	return result;
}
#endif /* SAFEWORD */

#if defined(ACE)
/*************************************************************************
 *
 *	Function: authAcePwd
 *
 *	Purpose: Process and reply to an ACE authentication request
 *
 *************************************************************************/

static int
authAcePwd(authInfo)
	AuthInfo	*authInfo;
{
	u_char stateInfo[AUTH_STRING_LEN];
	u_char *si;
	struct SD_CLIENT *sd = MALLOC (struct SD_CLIENT, 1);
	CONST char *chalMsg = "Enter PASSCODE: ";
	CONST char *nextMsg = "Enter Next PASSCODE (without PIN): ";
	CONST char *ncMsg;
	int result;

    	authInfo->user_msg=(char *)NULL;
	result = ace_pass(authInfo, sd);
	switch( result ) {
	default:
		log_err("ace_pass: %s: BIZARRE result %d for user `%s'\n",
			request_id (authInfo->authreq),
			result, authInfo->authName->strvalue);
		authInfo->user_msg = (char *)NULL;
		result = ACE_FAILED;
		break;

	case ACE_FAILED:
		log_err("ace_pass: %s: FAILED for user `%s'\n",
			request_id (authInfo->authreq),
			authInfo->authName->strvalue);
		authInfo->user_msg = (char *)NULL;
		result = ACE_FAILED;
		break;

	case ACE_CHALLENGING:
		ncMsg = chalMsg;
		DEBUG("ace_pass CHALLENGING\n");
		send_challenge( authInfo->authreq, ncMsg, strlen(ncMsg),
				ACE_DUMMY_STATE, LEN_ACE_DUMMY_STATE, authInfo->fd);
		result = -ACE_CHALLENGING;
		break;

	case ACE_PASSED:
		DEBUG("ace_pass PASSED\n");
		result = 0;
		break;

	case ACE_NEXT_PASSCODE:
		DEBUG("ace_pass NEXT_CODE\n");
		ncMsg = nextMsg;
		si = stateInfo;

		*si = (u_char)(LEN_SD_TOTAL(sd) + 1);
		si++;
    		memcpy(si, BEG_SD_1ST(sd), LEN_SD_1ST(sd));
		si += LEN_SD_1ST(sd);
    		memcpy(si, BEG_SD_2ND(sd), LEN_SD_2ND(sd));
		si += LEN_SD_2ND(sd);
    		memcpy(si, BEG_SD_3RD(sd), LEN_SD_3RD(sd));
		si += LEN_SD_3RD(sd);
		if (spawn_flag) {
			pid_t pid = getpid();
			memcpy(si, &pid, sizeof(pid_t));
		}
		send_nextcode(	authInfo->authreq, ncMsg, strlen(ncMsg),
				stateInfo, LEN_SD_TOTAL(sd) + 1, authInfo->fd);
		result = -ACE_NEXT_PASSCODE;
		break;

	case ACE_NEW_PIN:
		DEBUG("ace_pass NEXT_PIN\n");
		send_newpin(
			authInfo->authreq,
			stateInfo, strlen(stateInfo),
			authInfo->fd);
		result = -ACE_NEW_PIN;
		break;
	}

	memset(sd, 0, sizeof(*sd));
	free(sd);
	return result;
}
#endif /* ACE */

#if defined(DPI)

/*************************************************************************
 *
 *      Function: dpi_readcfg
 *
 *      Purpose: Process the configuration file
 *
 *************************************************************************/

void
dpi_readcfg()
{
	char *cp;
	struct dpi_param *p;
	char line[RLSIZE];
	FILE *dpi_cfgfile;
	char buf[256];

	DEBUG("dpi_readcfg:\n");

	sprintf(buf, "%s/%s", radius_dir, dpi_filename);

	/*
	 * open the configuration file
	 */
	if((dpi_cfgfile = fopen(buf, "r")) == (FILE *)NULL) {
		log_err("Couldn't open %s to read DPI config: %s\n",
				buf, xstrerror (errno));
		return;
	}

	while (fgets(line, RLSIZE, dpi_cfgfile)) {
		/*
		 * first, drop any comment fields
		 */
		cp = strchr(line, '#');
		if (cp)
			*cp = '\0';

		/*
		 * Now look for a token
		 */
		cp = strtok(line, " \t=");
		if (cp) {
			for (p = dpi_param; p->string; p++) {
				if (!strcmp(cp, p->string))
					break;
			}
			if (p->string)
				(*p->func)(p->target, strtok(NULL, "\n"));
		}
	}
}

void
cfg_key(void *p, char *ascii_key)
{
	char **key = p;
	char *tp;
	int i;

	tp = strtok(ascii_key, " \t=,");
	*key = malloc(KEY_LEN);
	memset(*key, 0, KEY_LEN);

	for (i = 0; i < KEY_LEN && tp; i++) {
		(*key)[i] = (char)strtol(tp, NULL, 0);
		tp = strtok(NULL, " \t,");
	}
}

void
cfg_str(void *p, char *string)
{
	char **target = p;
	char *tp;

	if ((tp = strtok(string, " \t=\n"))) {
		*target = malloc(strlen(tp) + 1);
		strcpy(*target, tp);
	}
}

void
cfg_int(void *p, char *string)
{
	int *target = p;
	char *tp;

	if ((tp = strtok(string, " \t=\n")))
		*target = atoi(tp);
}

/*************************************************************************
 *
 *      Function: dpi_init
 *
 *      Purpose: initialize and open the connection to the DSS
 *
 *************************************************************************/

int
dpi_init()
{
	da_config_t cfg;
	struct sockaddr_in sins;
	int fd;
	int len, ses;
	char *user;

	DEBUG("dpi_init:\n");

	dpi_readcfg();

	if (!dss_addr || !agent_id || !key) {
		printf("Configuration Error\n");
		exit(1);
	}
	memset(&cfg, 0, sizeof(da_config_t));
	memset(&sins, 0, sizeof(struct sockaddr_in));
	cfg.flags = REPORT_END_OF_CALL|PRIMARY_SERVER;

	sins.sin_family = AF_INET;
	sins.sin_addr.s_addr = inet_addr( dss_addr ) ;

	if (sins.sin_addr.s_addr == INADDR_NONE) {
	    log_err( "Invalid DPI server address %s\n", dss_addr ) ;
	}

	sins.sin_port = htons(dss_port);

	cfg.max_sessions = DPI_MAXSES;
	cfg.timeout = 10;
	strcpy(cfg.agent_id, agent_id);

	da_permute_key(key, cfg.pkeys);

	fd = da_connect((struct sockaddr *)&sins, &cfg);

	if (fd < 0) {
		log_err("DPI connect fail: server=%s, da_errno=%d: %s\n",
			dss_addr, da_errno, xstrerror(errno));
	}
	return(fd);
}

/*************************************************************************
 *
 *      Function: dpi_read
 *
 *      Purpose: The callback function used to notify us of a DSS response
 *
 *************************************************************************/

/* sid = session_id */
/* len = input_len  */
/* buf = dialog_buffer */
/* f   = flags = responseFromServer (see dsagent.h) */
int
dpi_read(int sid, char *buf, int len, int server_state, int f)
{
	int	id;

	DEBUG("dpi_read:\n");

	for(id=0; id < DPI_MAXSES; id++) {
		if (dpi_sessions[id].sessionid == sid) {
			dpi_sessions[id].dialog = buf;
		}
	}
	return(0);
}

/*************************************************************************
 *
 *      Function: dpi_challenge
 *
 *      Purpose: The callback function used to prompt the user
 *
 *************************************************************************/

int
dpi_challenge(int session_id, char *buf, int len)
{
	int      session;
	int      id;
	AuthInfo *authInfo = (AuthInfo *)NULL;
	char     stateInfo[AUTH_STRING_LEN];
	static char access_approved[] = "Access Approved";

	DEBUG("dpi_challenge:\n");

	for(id=0; id < DPI_MAXSES; id++) {
		if (dpi_sessions[id].sessionid == session_id) {
			session = id;
			authInfo = dpi_sessions[id].authInfo;
			break;
		}
	}

	if (authInfo == (AuthInfo *) NULL) {
		log_err("DPI: Cannot find session data\n");
		return(0);
	}

	if (strncmp(buf, access_approved, sizeof(access_approved) - 1) == 0) {
		DEBUG("dpi_challenge: user ok\n");
		return(len);
	}

#if defined(DPI_ONCE)
	if (authInfo->authState->size != 0) {
		DEBUG("dpi_challenge: non-first challenge\n");
		dpi_sessions[session].done = ABORT;
		dpi_sessions[session].result = DPI_FAILED;
		return(0);
	}
#endif /* DPI_ONCE */

	sprintf(stateInfo,"%d", session_id);

	send_challenge(authInfo->authreq,
			buf, len,
			stateInfo, strlen(stateInfo),
			authInfo->fd);
	return(len);
}

/*************************************************************************
 *
 *      Function: dpi_notify
 *
 *      Purpose: The callback function used when authentication is complete
 *
 *************************************************************************/

/* sid = session_id;     */
/* tid = transaction_id  */
/* buf = dailog_buffer including user information from DSS */

int
dpi_notify(int sid, int error, long tid, void *buf)
{
	int      id;

	DEBUG("dpi_notify:\n");
	DEBUG("DPI: session id       = %d\n", sid);
	DEBUG("DPI: error            = %d\n", error);
	DEBUG("DPI: transaction id   = %d\n", tid);
	DEBUG("DPI: user description = %s\n", (char *)buf);

	for(id=0; id < DPI_MAXSES; id++) {
		if (dpi_sessions[id].sessionid == sid) {
			dpi_sessions[id].result = error;
			dpi_sessions[id].done = TRUE;
			dpi_sessions[id].transactionid = tid;
			break;
		}
	}

	/* When an error is returned */
	if ((tid < 0) && (error > 0)) {
		dpi_sessions[id].result = -dpi_sessions[id].result;
	}

	return(0);
}

/*
 * The dialog_io structure has four members, (*read), (*write), (*notify), and
 * (*error).  If used for da_open_session(), (*error) is optional. (*read)
 * should post the buffer for user input and should not block.  No user
 * input should be accepted unless a buffer is posted.  Once the input
 * is terminated, the buffer should be returned to da_user_input() and
 * all input ignored until another buffer is posted.
 */

/* callback { read(), write(), notify(), error() } */
dialog_io_t callback = {dpi_read, dpi_challenge, dpi_notify, NULL};

/* The function da_close_session() has been changed.  It will no
 * longer send an end of call indication under any circumstances.  The
 * function da_end_call() must be called explicitly to indicate the
 * end of session to the DSS.  This was done to allow the session to be
 * closed before end of call indication.  It is possible that the agent
 * will be managing a large number of open sessions, so this give the
 * option of freeing resources and managing the sessions at a higher
 * level.
 */

/*************************************************************************
 *
 *      Function: authDPIPwd
 *
 *      Purpose: process a RADIUS authentication request
 *
 *************************************************************************/

int
authDPIPwd(authInfo)
	AuthInfo        *authInfo;
{
	static int	cstate=0;
	static int	fd;
	int		isdone=(ERROR);
	int		result;
	int		session;
	int		index;
	int		return_value;
	int		i;
	int		current;
	VALUE_PAIR      *authPwd = authInfo->authPwd;
	VALUE_PAIR      *authName = authInfo->authName;
	VALUE_PAIR      *authState = authInfo->authState;
	char	dialog_buffer[USER_PROMPT_LEN+1];
	char	user_name[ID_LEN+1];

	/* clear the Reply-Mesage field */
	DEBUG("authDPIPwd:\n");
	authInfo->user_msg = (char *)NULL;

	if (cstate == 0) {
		fd = dpi_init();
		if (fd < 0) {
			return(DPI_FAILED);
		}
		cstate = 1;
	}

	if (authState == NULL_PAIR) {
		log_err("authDPIPwd: authState is NULL, no token support\n");
		return(DPI_FAILED);
	}

	if (authName->size > ID_LEN) {
		authName->size = ID_LEN;
	}

	if (authState->size == 0) {
		strncpy (user_name, authName->strvalue, authName->size);
		user_name[authName->size] = '\0';
		DEBUG("authDPIPwd: user_name=%s\n", user_name);
		session = da_open_session(user_name, &callback);
		if (session == ERROR) {
			log_err("DPI session open failed: server=%s, da_errno=%d: %s\n",
				dss_addr, da_errno, xstrerror(errno));
			return(DPI_FAILED);
		}
		for(index=0; index < DPI_MAXSES; index++) {
			if (dpi_sessions[index].sessionid == 0) {
				dpi_sessions[index].sessionid = session;
				dpi_sessions[index].authInfo = authInfo;
				break;
			}
		}
		if (dpi_sessions[index].sessionid != session) {
			log_err("authDPIPwd: index out of range\n");
			result = DPI_FAILED;
		}
		result = da_wait_server((fd_set *)0);
		if ((result = da_process_packet()) == (ERROR)) {
			log_err("DPI session failed: server=%s, da_errno=%d: %s\n",
				dss_addr, da_errno, xstrerror(errno));

			result = DPI_FAILED;
		}
		if ((result==0) && (dpi_sessions[index].done != TRUE)) {
			result = 1;
		}
	}

	if (authState->size != 0) {
		strncpy (dialog_buffer, authState->strvalue, authState->size);
		dialog_buffer[authState->size] = '\0';
		session = atoi(dialog_buffer);
		for(index=0; index < DPI_MAXSES; index++) {
			if (dpi_sessions[index].sessionid == session) {
				isdone = dpi_sessions[index].done;
				current = index;
			}
		}
		if (isdone == FALSE) {
			decryptAuthPwd ("authDPIPwd", authInfo, !authInfo->chkImmediate,
					dpi_sessions[current].dialog);
			da_user_input(dpi_sessions[current].dialog, authPwd->size);
			result = da_wait_server((fd_set *) 0);
			if ((result = da_process_packet()) == (ERROR)) {
				log_err("DPI session failed: server=%s, da_errno=%d: %s\n",
					dss_addr, da_errno, xstrerror(errno));

				result = DPI_FAILED;
			}
		} else if (isdone == (ERROR)) {
			result = DPI_FAILED;
		}
	}
	DEBUG("authDPIPwd: result value=%d\n", result);
	for(index=0; index < DPI_MAXSES; index++) {
#if defined(DPI_ONCE)
		if (dpi_sessions[index].done == (ABORT)) {
			DEBUG("authDPIPwd: aborting DSS session\n");
			while (dpi_sessions[index].done != TRUE) {
				strncpy(dpi_sessions[current].dialog, "01234567", 9);
				da_user_input(dpi_sessions[current].dialog, 9);
				result = da_wait_server((fd_set *) 0);
				if ((result = da_process_packet()) == (ERROR)) {
					DEBUG("DPI session aborted\n");
					result = DPI_FAILED;
					dpi_sessions[index].done = TRUE;
				}
			}
		}
#endif /* DPI_ONCE */
		if (dpi_sessions[index].done == TRUE) {
			strncpy (user_name, dpi_sessions[index].authInfo->authName->strvalue, dpi_sessions[index].authInfo->authName->size);
			user_name[dpi_sessions[index].authInfo->authName->size] = '\0';
			result = da_close_session(dpi_sessions[index].sessionid);
			result = da_end_call(user_name, dpi_sessions[index].transactionid);
			result = dpi_sessions[index].result;
			dpi_sessions[index].sessionid=0;
			dpi_sessions[index].transactionid=0;
			dpi_sessions[index].result=0;
			dpi_sessions[index].done=FALSE;
			dpi_sessions[index].authInfo=0;
		}
	}
	DEBUG("authDPIPwd: returning %d\n", result);
	return(result);
}

#endif /* DPI */

/*************************************************************************
 *
 *	Function: authChapToken
 *
 *	Purpose: Process and reply to a CHAP token authentication request
 *
 *************************************************************************/

int
authChapToken(authInfo)
	AuthInfo	*authInfo;
{
	char		string[AUTH_PASS_LEN + 64];
	char		*ptr;
	u_char		size;
	int		result;

	DEBUG("authChapToken\n");

	size = authInfo->authPwd->size;
	if( size != AUTH_VECTOR_LEN + 1 ) {
		log_err("CHAP Token: %s: Bad Pwd Size (%d), for user `%s'\n",
				request_id (authInfo->authreq),
				size, authInfo->authName->strvalue);
		return -1;
	}
	ptr = string;

	if( authInfo->authState && authInfo->chkSecret ) {
		DEBUG_PWD (("authChapToken: chkSecret = `%s'\n",
			    authInfo->chkSecret->strvalue));
		size = authInfo->chkSecret->size - 1;	/* skip final NUL */
		*ptr++ = authInfo->authPwd->strvalue[0];	/* pkt_id */
		memcpy(ptr, authInfo->chkSecret->strvalue, size);
		ptr += size;
		memcpy(ptr, authInfo->vector, AUTH_VECTOR_LEN);
		md5_calc(authInfo->pw_digest, (u_char *)string,
			1 + AUTH_VECTOR_LEN + size);
		/* shift down to remove the pkt_id */
#if defined(OSUN)
		/* sigh... just use the unsafe memcpy ... */
		memcpy(authInfo->authPwd->strvalue,
			&(authInfo->authPwd->strvalue[1]), CHAP_VALUE_LENGTH);
#else
		memmove(authInfo->authPwd->strvalue,
			&(authInfo->authPwd->strvalue[1]), CHAP_VALUE_LENGTH);
#endif
		authInfo->authPwd->size -= 1;
	}
	else {
		/* authState indicates requestor understands token
		   authentication, and chkSecret is needed to decode
		   the password.  If this request is missing either
		   attribute then it must be rejected.
		*/
		log_err ("CHAP Token failed: %s:%s%s\n",
		       request_id (authInfo->authreq),
		       (authInfo->authState ? "" : " missing state info"),
		       (authInfo->chkSecret ? "" : " missing shared secret"));
		return -1;
	}

	result = -1;
	if( authInfo->tokExp && (allow_token_caching != 0) ) {
		CONST char *val;	/* the temporary shared password */
		val = cache_search(cache, authInfo->authName->strvalue,
					authInfo->authName->size);
		if( val && val != ELEM_DELETED ) {
			int	ix;
			int	len;
			int	status;
			char	*user;
			char	pwd[AUTH_PASS_LEN + 1];

			DEBUG_PWD (("Cache val for %s: <%s>\n",
				    authInfo->authName->strvalue, val));
			decryptAuthPwd ("authChapToken", authInfo, 0, pwd);

			/* see if we got the <password><.><username> format */
			user = strchr(pwd, '.');
			if( user ) {
				*user = '\0';
				user++;
			}

			len = MAX((int)strlen(val)+1, (int)strlen(pwd)+1);
			len = MIN(len, CHAP_VALUE_LENGTH);
			DEBUG_PWD (("pwd(%d): %s\n", len, pwd));
			status = memcmp(pwd, val, len);
			if( status == 0 ) {
				authInfo->cacheAuth = TRUE;
				return status;	/* authenticated */
			}
		}
		else if( val == ELEM_DELETED ) {
			DEBUG("Cache elem for %s was deleted\n",
				authInfo->authName->strvalue);
			if( spawn_flag == 0 ) {
#if(1)
				;	/* cache_search() already deleted it */
#else
				int status = cache_delete(cache,
					authInfo->authName->strvalue,
					authInfo->authName->size);
				if( status != TRUE ) {
					log_err("Err: deleting <%s>\n",
						authInfo->authName->strvalue);
				}
#endif
			}
			else {	/* child copy deleted,
				   now ask parent to delete */
				CacheMsg	msg;
				int		n = 0;

				msg.type = CACHE_DELETE;
				memcpy(msg.key,
					authInfo->authName->strvalue,
					authInfo->authName->size);
				while( n != sizeof(msg) ) {
					n += write(wrCachefd, ((char *)&msg)+n,
						sizeof(msg)-n);
				}
			}
		}
		else {
			DEBUG("Cache elem for %s was not found\n",
				authInfo->authName->strvalue);
		}

		/* token cache missed */
		result = -1;	/* assume failure */
		if( 0 ) {
			;	/* because no particular token is guaranteed
					to be supported */
		}
#if defined(SAFEWORD)
		/* check for SafeWord Authentication */
       		else if( strcmp(authInfo->chkPwd->strvalue, EL_PWD) == 0) {
			result = authSafewordPwd(authInfo);
		}
#endif
#if defined(ACE)
		/* check for Security Dynamics Authentication */
		else if( strcmp(authInfo->chkPwd->strvalue, SD_PWD) == 0) {
			result = authAcePwd(authInfo);
		}
#endif
#if defined(DPI)
		/* check for Digital Pathways Authentication */
		else if( strcmp(authInfo->chkPwd->strvalue, DP_PWD) == 0) {
			if (authInfo->authState != NULL_PAIR) {
				DEBUG("authChapToken: authState: %s : %d\n",
				      authInfo->authState->strvalue,
				      authInfo->authState->size);
			}
			result = authDPIPwd(authInfo);
		}
#endif
	}
	else {
		log_err ("CHAP Token failed: %s:%s%s\n",
		       request_id (authInfo->authreq),
		       (authInfo->tokExp ? "" : " missing token expiry"),
		       (authInfo->chkSecret ? "" : " token cacheing not enabled"));
		return -1;
	}
	return result;
}

/*************************************************************************
 *
 *	Function: authChapPwd
 *
 *	Purpose: Process and reply to a CHAP authentication request
 *
 *************************************************************************/

int
authChapPwd(authInfo)
	AuthInfo	*authInfo;
{
	char	string[AUTH_PASS_LEN + 64];
	char	*ptr;
	int	result = -1;	/* init to failure */

	DEBUG("authChapPwd\n");
	if( getPwdType(authInfo->chkPwd->strvalue) == PWD_TOKEN ) {
		result = authChapToken(authInfo);
		if( (allow_token_caching != 0) && (result == 0) ) {
			int	status;
			int	ix;
			int	len;
			char	clrPwd[AUTH_PASS_LEN + 1];
			char	*user = (char *)NULL;
			time_t	idle_seconds;

			if( authInfo->chkIdle ) {
				idle_seconds = authInfo->chkIdle->lvalue;
			}
			else {
				idle_seconds = 0;
			}

			decryptAuthPwd ("authChapPwd", authInfo, 0, clrPwd);

			/* see if we got the <password><.><username> format */
			user = strchr(clrPwd, '.');
			if( user ) {
				*user = '\0';	/* skip username */
			}
			len = MIN((int)strlen(clrPwd)+1, CHAP_VALUE_LENGTH);

			if( authInfo->cacheAuth == TRUE ) {
				/* no cache insert on cache hits */
				if( !idle_seconds ) {
					/* no cache update if no idle time */
					return result;
				}
				else if( spawn_flag == 0 ) {
					status = cache_idle_update(cache,
						authInfo->authName->strvalue,
						authInfo->authName->size,
						idle_seconds);
					if( status != TRUE ) {
						log_err("authChapPwd: update failed\n");
					}
				}
				else {
					int		n = 0;
					CacheMsg	msg;

					msg.type = CACHE_IDLE_UPDATE;
					memcpy(msg.key,
						authInfo->authName->strvalue,
						authInfo->authName->size);
#if(0)
					memcpy(msg.val, clrPwd, len);
#endif
					msg.time = 0;
					msg.idle = idle_seconds;
					while( n != sizeof(msg) ) {
						n += write(wrCachefd,
							((char *)&msg)+n,
							sizeof(msg)-n);
					}
				}
				return result;
			}

			if( spawn_flag == 0 ) {
				status = cache_insert(cache,
					authInfo->authName->strvalue,
					authInfo->authName->size,
					clrPwd, len,
					authInfo->tokExpSecs, idle_seconds);
				if( status != TRUE ) {
					log_err("authChapPwd: insert failed\n");
				}
			}
			else {
				int		n = 0;
				CacheMsg	msg;

				msg.type = CACHE_INSERT;
				memcpy(msg.key,
					authInfo->authName->strvalue,
					authInfo->authName->size);
				memcpy(msg.val, clrPwd, len);
				msg.time = authInfo->tokExpSecs;
				msg.idle = idle_seconds;
				while( n != sizeof(msg) ) {
					n += write(wrCachefd, ((char *)&msg)+n,
						sizeof(msg)-n);
				}
			}
		}
	}
	else if( (getPwdType(authInfo->chkPwd->strvalue) == PWD_UNIX)
			|| (getPwdType(authInfo->chkPwd->strvalue) == PWD_SQL) ) {
		/* UNIX or SQL passwords not supported with CHAP */
		log_err("CHAP Unix Attempt: %s: user `%s'\n",
				request_id (authInfo->authreq),
				authInfo->authName->strvalue);
		result = -1;
	}
	else {	/* normal 'static' password */
		int size;

		/* Use normal MD5 to verify */
		ptr = string;
		size = authInfo->chkPwd->size - 1;	/* skip final NUL */
		*ptr++ = authInfo->authPwd->strvalue[0];	/* pkt_id */
		memcpy(ptr, authInfo->chkPwd->strvalue, size);
		ptr += size;
		memcpy(ptr, authInfo->vector, AUTH_VECTOR_LEN);
		md5_calc((u_char *)authInfo->pw_digest, (u_char *)string,
			1 + AUTH_VECTOR_LEN + size);

		/* Compare them */
		if( memcmp(authInfo->pw_digest,
				authInfo->authPwd->strvalue + 1,
				CHAP_VALUE_LENGTH) != 0)
		{
			result = -1;
		}
		else {
			result = 0;
		}
	}
	return result;
}

/*************************************************************************
 *
 *      Function: authAraDesPwd
 *
 *      Purpose: validate and reply to an ARADES auth. request
 *
 *************************************************************************/

int
authAraDesPwd( authInfo)
	AuthInfo       *authInfo;
{
	int i, pwdlen, result = -1;
	u_char password[ARA_PASS_LEN];
	u_char scratch[ARA_PASS_LEN];

	DEBUG("authAraDesPwd\n");
	pwdlen= strlen(authInfo->chkPwd->strvalue);
	memset(password, 0, ARA_PASS_LEN);

	/* ARA-DES only uses first 8 bytes MAX */
	if (pwdlen > ARA_PASS_LEN) {
		pwdlen = ARA_PASS_LEN;
	}
	for (i=0; i < pwdlen; i++) {
		password[i] = (u_char) ( authInfo->chkPwd->strvalue[i] << 1);
	}
	/* Now use this as the  key for the des encryptor. */
	dessetkey( (char *) password);

	/* Encrpyt the random eight bytes that we had sent. */
	memcpy(scratch, authInfo->vector, 8);
	endes((char *) scratch);

	/* If they match to what we got, the remote is validated. */
	if (memcmp(scratch, authInfo->authPwd->strvalue, 8) == 0) {
		result = 0;
	}
	return ( result);
}

#ifdef PWD_DEBUG

char *
print_passwd (raw, size)
	unsigned char CONST	*raw;
	int		size;
{
	static char cooked[AUTH_PASS_LEN * 7 + 1];
	char *cook = cooked;
	unsigned char CONST *raw_end = &raw[size];

	while (raw < raw_end) {
		sprintf (cook, "%02x", *raw);
		cook += 2;
		if (isgraph (*raw)) {
			*cook++ = '(';
			*cook++ = *raw;
			*cook++ = ')';
		}
		*cook++ = ' ';
		raw++;
	}
	*cook = '\0';
	return cooked;
}

#endif

char *
decryptAuthPwd (where, authInfo, subst3rd, buffer)
	char CONST	*where;
	AuthInfo	*authInfo;
	int		subst3rd;
	char		*buffer;
{
	int		i;
	int             size;

	if (authInfo->auth3rd && subst3rd) {
		size = authInfo->auth3rd->size;
		memcpy (buffer, authInfo->auth3rd->strvalue, size);
		buffer[size] = '\0';
		DEBUG_PWD (("%s: substitute 3rd prompt for authPwd: `%s'\n",
			    where, buffer));
	} else {
		u_char *pw_digest = authInfo->pw_digest;
		size = authInfo->authPwd->size;
		memcpy (buffer, authInfo->authPwd->strvalue, size);
		for (i = 0; i < size; i++) {
			buffer[i] ^= pw_digest[i % AUTH_VECTOR_LEN];
		}
		buffer[size] = '\0';
		DEBUG_PWD (("%s: authPwd encrypted: %s\n", where,
			    print_passwd (authInfo->authPwd->strvalue, size)));
		DEBUG_PWD (("%s: authPwd decrypted: %s\n", where,
			    print_passwd (buffer, size)));
	}
	return buffer;
}

/*************************************************************************
 *
 *	Function: authPapPwd
 *
 *	Purpose: Process and reply to a PAP authentication request
 *
 *************************************************************************/

int
authPapPwd(authInfo)
	AuthInfo	*authInfo;
{
	int	i;
	char	string[AUTH_PASS_LEN + 1];
	int	result = -1;	/* init to failure */
	int	len;
	char	pwd[AUTH_PASS_LEN + 1];

	DEBUG("authPapPwd\n");
	authInfo->user_msg = (char *)NULL;

		/* check for Unix Authentication */
	if( getPwdType(authInfo->chkPwd->strvalue) == PWD_UNIX ) {
		if(unix_pass(authInfo->authName->strvalue,
			     decryptAuthPwd ("authPapPwd", authInfo, 0, pwd)) == 0) {
			result = 0;
		}
	}
		/* check for SQL Authentication */
	else if( getPwdType(authInfo->chkPwd->strvalue) == PWD_SQL ) {
		if(sql_pass(authInfo->authName->strvalue,
			     decryptAuthPwd ("authPapPwd", authInfo, 0, pwd)) == 0) {
			result = 0;
		}
	}
#if defined(SAFEWORD)
	/* check for SafeWord Authentication */
       	else if( strcmp(authInfo->chkPwd->strvalue, EL_PWD) == 0) {
		result = authSafewordPwd(authInfo);
	}
#endif
#if defined(ACE)
	/* check for Security Dynamics Authentication */
	else if( strcmp(authInfo->chkPwd->strvalue, SD_PWD) == 0) {
		if (authInfo->chkImmediate && authInfo->authState->size == 0) {
			/* short-circuit dummy challenge */
			memcpy(authInfo->authState->strvalue,
				ACE_DUMMY_STATE, LEN_ACE_DUMMY_STATE);
			authInfo->authState->size = LEN_ACE_DUMMY_STATE;
		}
		result = authAcePwd(authInfo);
	}
#endif
#if defined(DPI)
	/* check for SafeWord Authentication */
       	else if( strcmp(authInfo->chkPwd->strvalue, DP_PWD) == 0) {
		if (authInfo->authState != NULL_PAIR) {
			DEBUG("authPapPwd: authState: %s : %d\n",
			      authInfo->authState->strvalue, authInfo->authState->size);
		}
		result = authDPIPwd(authInfo);
	}
#endif
	/* DEFAULT Password is neither UNIX nor SAFEWORD nor ACE nor DPI */
	else if(strcmp(authInfo->chkPwd->strvalue,
		       decryptAuthPwd ("authPapPwd", authInfo, 0, pwd)) == 0) {
		result = 0;
	}

	return result;
}

/*************************************************************************
 *
 *	Function: rad_authenticate
 *
 *	Purpose: Process and reply to an authentication request
 *
 *************************************************************************/

int
rad_authenticate(authreq, activefd)
	AUTH_REQ	*authreq;
	int		activefd;
{
	VALUE_PAIR	*check_item;
	VALUE_PAIR	*auth_item;
	VALUE_PAIR	*checkList;
	VALUE_PAIR	*replyList;
	VALUE_PAIR	*authName;
	VALUE_PAIR	*authPwd;
	VALUE_PAIR	*authState;
	VALUE_PAIR	*chkPwd;
	VALUE_PAIR	*chkImmediate;
	VALUE_PAIR	*chkIdle;
	VALUE_PAIR	*tokExp;
	VALUE_PAIR	*pwdExp;
	VALUE_PAIR	*cutCur;
	VALUE_PAIR	*auth3rd;
	int		result;
	u_char		pw_digest[AUTH_VECTOR_LEN];
	char		umsg[128];
	CONST char	*user_msg;
	int		retval;
	PasswordType	passwordType;
	int		passwordExpired;
	int		tokenExpired;
	AuthInfo	authInfo;

	DEBUG("rad_authenticate\n");

		/* init the info to pass around */
	memset(&authInfo, 0, sizeof(authInfo));	/* paranoia */
	authInfo.fd		= activefd;
	authInfo.authreq	= authreq;
	authInfo.ipaddr		= authreq->ipaddr;
	authInfo.vector		= authreq->vector;
	authInfo.pw_digest	= pw_digest;
	authInfo.cacheAuth	= FALSE;
	authInfo.cutList	= NULL_PAIR;
	authInfo.user_msg	= (char *)NULL;
	authInfo.tokExp		= NULL_PAIR;
	authInfo.chkSecret	= NULL_PAIR;
	authInfo.chkImmediate	= NULL_PAIR;
	authInfo.chkIdle	= NULL_PAIR;
	authInfo.authState	= NULL_PAIR;
	authInfo.tokExpSecs	= (time_t)0;

	checkList = 		NULL_PAIR;
	replyList = 		NULL_PAIR;

	/*
	 * Name Attribute is special:
	 *	- Must be present in the auth request
	 *	- Must match a name in the users file
	 * Password Attribute is special:
	 *	- Must be present in the users file record
	 *	- Must be present in the auth request as one of:
	 *		- PW_PASSWORD
	 *		- PW_CHAP_PASSWORD
	 *		- PW_ASCEND_ARADES
	 *	- If PW_PASSWORD then either:
	 *		- static password
	 *		- UNIX password
	 *		- token password w/o token caching
	 *	- Else if PW_CHAP_PASSWORD then either:
	 *		- static password
	 *		- token password with token caching
	 *	- Else if PW_ASCEND_ARADES
	 *		- static password
	 *
	 * All other attributes:
	 *	- Check the auth request value against the check request value
	 *
	 * Some check attributes determine what kind of password checking we
	 * do rather than being checked against auth request attributes,
	 *
	 * Some reply attributes specify per-user characteristics rather
	 * than being returned to the authentication requestor.
	 *
	 */

	/* Get the username from the request */
	authName = cut_attribute(&authreq->request, PW_USER_NAME);
	if (authName == NULL_PAIR || strlen (authName->strvalue) == 0) {
		log_err("Authenticate: %s: No User Name\n",
			request_id (authreq));
		authCleanup( &authInfo, checkList, replyList );
		return -1;
	}
	else {
		authInfo.authName = authName;
		authInfo.cutList = authName;
		cutCur = authName;
	}

	/* Verify the client and Calculate the MD5 Password Digest */
	if(calc_digest(pw_digest, authreq) != 0) {
		/* We dont respond when this fails */
		log_err("Authenticate: %s: Security Breach: %s\n",
			request_id(authreq), authName->strvalue);
		authCleanup( &authInfo, checkList, replyList );
		return -1;
	}

	/* Get the user from the database */
	if( (retval = user_find(authName->strvalue, &checkList, &replyList))
	  != 0) {
		log_err("Authenticate: %s: %s: %s\n",
			request_id (authreq), get_errmsg(retval),
			authName->strvalue);
		send_reject(authreq, (char *)NULL, activefd);
		authCleanup( &authInfo, checkList, replyList );
		return -1;
	}

	/* init the flags - we will check them later */
	passwordType = PWD_UNKNOWN;
	passwordExpired = FALSE;
	tokenExpired = FALSE;

	/* start by examining the user record check list:
		it MUST have a password entry and MAY have other entries.
	*/
	chkPwd = cut_attribute(&checkList, PW_PASSWORD);
	if( !chkPwd ) {	/* MUST have password in record! */
		log_err("Authenticate: %s: No pwd in record!\n",
			request_id (authreq));
		send_reject(authreq, (char *)NULL, activefd);
		authCleanup( &authInfo, checkList, replyList );
		return -1;
	}
	else {
		authInfo.chkPwd = chkPwd;
		cutCur->next = chkPwd;
		cutCur = cutCur->next;
	}

	pwdExp = cut_attribute(&checkList, ASCEND_PW_EXPIRATION);
	if( pwdExp ) {
		VALUE_PAIR *lifeTime;
		VALUE_PAIR *warnTime;
		UINT4 usr_exp_days = (UINT4)0;
		UINT4 usr_warn_days = (UINT4)0;

		VALUE_PAIR *expiryCopy = copyValuePair(pwdExp);
		insertValuePair(&replyList,expiryCopy);

		DEBUG("rad_auth: check_item: ASCEND_PW_EXPIRATION\n");

		cutCur->next = pwdExp;
		cutCur = cutCur->next;

		/* Has this user's password expired */
		lifeTime = cut_attribute(&replyList, ASCEND_PW_LIFETIME);
		if( lifeTime ) {
			cutCur->next = lifeTime;
			cutCur = cutCur->next;
			usr_exp_days = lifeTime->lvalue;
		}

		warnTime = cut_attribute(&replyList, ASCEND_PW_WARNTIME);
		if( warnTime ) {
			cutCur->next = warnTime;
			cutCur = cutCur->next;
			usr_warn_days = warnTime->lvalue;
		}
		retval = pw_expired(pwdExp->lvalue,
					usr_exp_days, usr_warn_days);
		if(retval < 0) {
			passwordExpired = TRUE;
			DEBUG("rad_auth: password has EXPIRED\n");
		}
		else if( retval > 0 ) {
			DEBUG("rad_auth: password WARNING: %d\n", retval);
			sprintf(umsg,
			  	"Password Will Expire in %d Days\n", retval);
			user_msg = umsg;
		}
	}

	authPwd = cut_attribute(&authreq->request, PW_PASSWORD);
	if( !authPwd ) {
		authPwd = cut_attribute(&authreq->request, PW_CHAP_PASSWORD);
	}
	if ( !authPwd ) {
		authPwd = cut_attribute(&authreq->request, PW_ASCEND_ARADES);
	}
	if( !authPwd ) {	/* MUST have received password! */
		log_err("Authenticate: %s: No pwd in request!\n",
			request_id (authreq));
		send_reject(authreq, (char *)NULL, activefd);
		authCleanup( &authInfo, checkList, replyList );
		return -1;
	}
	else {
		authInfo.authPwd = authPwd;
		cutCur->next = authPwd;
		cutCur = cutCur->next;
	}

	authState = cut_attribute(&authreq->request, PW_STATE);
	if( authState ) {
		authInfo.authState = authState;
		cutCur->next = authState;
		cutCur = cutCur->next;
	}

	/*
	 * Check token expiration time if we are doing token caching.
	 */
	tokExp = cut_attribute(&checkList, ASCEND_TOKEN_EXPIRY);
	if( tokExp ) {
		VALUE_PAIR *chkSecret;
		char	*sharedRcvSecret;
		UINT4	tokExpSecs;

		authInfo.tokExp = tokExp;
		cutCur->next = tokExp;
		cutCur = cutCur->next;

			/* Paranoia for the masses */
		tokExpSecs = (UINT4)0;
		sharedRcvSecret = (char *)0;

		DEBUG("rad_auth: checkList: ASCEND_TOKEN_EXPIRY = %ld\n",
			tokExp->lvalue);

		/* Is token cache expiry correctly specified */
		tokExpSecs = tokExp->lvalue * (UINT4)SECONDS_PER_MINUTE;
		if( tokExpSecs ) {
			chkSecret = cut_attribute(&replyList, ASCEND_RECV_SECRET);
			if( chkSecret ) {
				authInfo.chkSecret = chkSecret;
				cutCur->next = chkSecret;
				cutCur = cutCur->next;
				sharedRcvSecret = chkSecret->strvalue;
			} else {
				DEBUG ("rad_auth: checkList: No shared secret\n");
			}
		} else {
			DEBUG ("rad_auth: checkList: Zero Token Expiration\n");
		}
		if( tokExpSecs && sharedRcvSecret ) {
			authInfo.tokExpSecs = tokExpSecs;
		}
	}

	chkImmediate = cut_attribute(&checkList, ASCEND_TOKEN_IMMEDIATE);
	if( chkImmediate && (chkImmediate->lvalue == TOK_IMM_YES) ) {
		DEBUG("rad_auth: checkList: ASCEND_TOKEN_IMMEDIATE = %ld\n",
			chkImmediate->lvalue);
		authInfo.chkImmediate = chkImmediate;
		cutCur->next = chkImmediate;
		cutCur = cutCur->next;
	}

	/*
	 * Token Passcode might come from MAX as Ascend-Third-Prompt
	 * when in terminal-server non-immediate mode.
	 */
	auth3rd = cut_attribute(&authreq->request, ASCEND_THIRD_PROMPT);
	if (auth3rd) {
		cutCur->next = auth3rd;
		cutCur = cutCur->next;
	}
	authInfo.auth3rd = auth3rd;

	chkIdle = cut_attribute(&checkList, ASCEND_TOKEN_IDLE);
	if( chkIdle && (chkIdle->lvalue != 0) ) {
		DEBUG("rad_auth: checkList: ASCEND_TOKEN_IDLE = %ld\n",
			chkIdle->lvalue);

		chkIdle->lvalue *= SECONDS_PER_MINUTE;
		authInfo.chkIdle = chkIdle;
		cutCur->next = chkIdle;
		cutCur = cutCur->next;
	}

	/*
	 * Any future special attributes in the auth request should be
	 * dealt with here and gotten from the &authreq->request.
	 */


	/*
	 * Special handling for passwords which are encrypted,
	 * and sometimes authenticated against the UNIX passwd database.
	 * Also they can come using the Three-Way CHAP.
	 * Or be token passwords.
	 * Or be tokens with caching
	 * Or ...
	 */
	passwordType = getPwdType(chkPwd->strvalue);
	DEBUG("User record PASSWORD type is %s\n",
	      ((passwordType == PWD_UNIX) ? "Unix"
	       : ((passwordType == PWD_TOKEN) ? "Token"
		  : ((passwordType == PWD_RADIUS) ? "Radius"
		     : ((passwordType == PWD_SQL) ? "SQL"
		        : "Unknown")))));

	result = -1;
	switch ( authPwd->attribute ) {

	case PW_CHAP_PASSWORD:
		result = authChapPwd(&authInfo);
		break;

	case PW_PASSWORD:
		result = authPapPwd(&authInfo);
		break;

	case PW_ASCEND_ARADES:
		result = authAraDesPwd(&authInfo);
		break;

	default:
		break;
	}

	/*
	 * At this point all of the special attributes have been checked.
	 * A result of 0 means that they have been satisfied; negative
	 * result means failure, positive means special handling.
	 */
	if( result < 0 ) {	/* auth failure */
		send_reject(authreq, authInfo.user_msg, activefd);
		authCleanup( &authInfo, checkList, replyList );
		return result;
	}
	else if( result > 0 ) {	/* token stuff; neither pass nor fail */
		authCleanup( &authInfo, checkList, replyList );
		return result;
	}

	/*
	 * Look for the remaining matching check items
	 */
	user_msg = (char *)NULL;
	check_item = checkList;
	result = 0;
	while (result == 0 && check_item != NULL_PAIR) {
		/*
		 * Look for the matching attribute in the request.
		 */
		auth_item = cut_attribute(&authreq->request, check_item->attribute);
		if (auth_item == NULL_PAIR) {
			result = -1;
			break;
		}
		else {
			cutCur->next = auth_item;
			cutCur = cutCur->next;
			switch(check_item->type) {
			case PW_TYPE_STRING:
				if(strcmp(check_item->strvalue,
						auth_item->strvalue) != 0) {
					log_err ("rad_auth: %s: check item mismatch: `%s' != `%s'\n",
						 request_id (authreq),
						 check_item->strvalue, auth_item->strvalue);
					result = -1;
				}
				break;

			case PW_TYPE_PHONESTRING:
				if(phone_cmp(check_item->strvalue,
						auth_item->strvalue) != 0) {
					log_err ("rad_auth: %s: check item mismatch: `%s' not in `%s'\n",
						request_id (authreq),
 						auth_item->strvalue,
						check_item->strvalue );
					result = -1;
				}
				break;

			case PW_TYPE_INTEGER:
			case PW_TYPE_IPADDR:
				if(check_item->lvalue != auth_item->lvalue) {
					log_err ("rad_auth: %s: check item mismatch: %ld != %ld\n",
						 request_id (authreq),
						 check_item->lvalue, auth_item->lvalue);
					result = -1;
				}
				break;

			default:
				result = -1;
				break;
			}
		}
		check_item = check_item->next;
	}

	if(result != 0) {
		send_reject(authreq, user_msg, activefd);
	}
	else {	/* everything ok, except that the password may have expired */
		if( passwordExpired != FALSE ) {	/* pwd is too old */
			/* require that:
			 *	- password changing is allowed
			 *	- the expired pwd was given
			 */
		    if( allow_passchange == TRUE ) {
			switch( passwordType ) {
			default:
			case PWD_UNKNOWN:
			case PWD_UNIX:
			case PWD_TOKEN:
			case PWD_SQL:
				user_msg = "Password Has Expired\n";
				send_reject(authreq, user_msg, activefd);
				break;
			case PWD_RADIUS:
				user_msg = "Password Has Expired\n";
				send_pwexpired(authreq, user_msg, activefd);
				break;
			}
		    }
		    else {
			user_msg = "Password Has Expired\n";
			send_reject(authreq, user_msg, activefd);
		    }
		}
		else {
			send_accept(authreq, &replyList, user_msg, activefd);
		}
	}
	authCleanup( &authInfo, checkList, replyList );
	return result;
}

/*************************************************************************
 *
 *	Function: send_reject
 *
 *	Purpose: Reply to the request with a REJECT.  Also attach
 *		 any user message provided.
 *
 *************************************************************************/

void
send_reject(authreq, msg, activefd)
	AUTH_REQ	*authreq;
	CONST char	*msg;
	int		activefd;
{
	AUTH_HDR	*auth;
	int		length = AUTH_HDR_LEN;

	DEBUG("send_reject: %s\n", request_id (authreq));

	auth = make_send_buffer (authreq, (authreq->code == PW_PASSWORD_REQUEST
					   ? PW_PASSWORD_REJECT
					   : PW_AUTHENTICATION_REJECT));
	length += append_user_message (auth->data, msg, AUTH_STRING_LEN);
	send_answer (activefd, authreq, auth, length);
}

/*************************************************************************
 *
 *	Function: send_challenge
 *
 *	Purpose: Reply to the request with a CHALLENGE.  Also attach
 *		 any user message provided and a state value.
 *
 *************************************************************************/

void
send_challenge(authreq, msg, msgLen, state, stateLen, activefd)
	AUTH_REQ	*authreq;
	CONST char	*msg;
	int		msgLen;
	CONST char	*state;
	int		stateLen;
	int		activefd;
{
	AUTH_HDR	*auth;
	int		length = AUTH_HDR_LEN;
	u_char		*ptr;

	DEBUG("send_challenge:\n");

	auth = make_send_buffer (authreq, PW_ACCESS_CHALLENGE);
	ptr = auth->data;

	/* Append the challenge */
	if( msgLen <= AUTH_STRING_LEN ) {
		DEBUG("...Adding challenge, len=%d\n", msgLen);
		*ptr++ = PW_PORT_MESSAGE;
		*ptr++ = msgLen + 2;
		if (msg) {
			memcpy(ptr, msg, msgLen);
			if( debug_flag == 1 ) {
				debugf("challenge msg before encrypt: \"");
				fwrite(msg, 1, msgLen, stdout);
				puts("\"");
			}
			ptr += msgLen;
		}
		length += msgLen + 2;
	}

	/* Append the state info */
	if( state != (char *)NULL ) {
		DEBUG("...Adding state, len=%d\n", stateLen);
		*ptr++ = PW_STATE;
		*ptr++ = stateLen + 2;
		memcpy(ptr, state, stateLen);
		ptr += stateLen;
		length += stateLen + 2;
	}

	DEBUG("send_challenge: %s: length = %d\n",
	      request_id (authreq), length);

	send_answer (activefd, authreq, auth, length);
}

/*************************************************************************
 *
 *	Function: send_pwack
 *
 *	Purpose: Reply to the request with an ACKNOWLEDGE.
 *		 User password has been successfully changed.
 *
 *************************************************************************/

void
send_pwack(authreq, activefd)
	AUTH_REQ	*authreq;
	int		activefd;
{
	AUTH_HDR	*auth;

	DEBUG("send_pwack: %s\n", request_id (authreq));

	auth = make_send_buffer (authreq, PW_PASSWORD_ACK);
	send_answer (activefd, authreq, auth, AUTH_HDR_LEN);
}

/*************************************************************************
 *
 *	Function: send_pwexpired
 *
 *	Purpose: Reply to the request with a PW_PASSWORD_EXPIRED
 *
 *************************************************************************/

void
send_pwexpired(authreq, msg, activefd)
	AUTH_REQ	*authreq;
	CONST char	*msg;
	int		activefd;
{
	AUTH_HDR	*auth;
	int		length = AUTH_HDR_LEN;

	DEBUG("send_pwexpired: %s\n", request_id (authreq));

	auth = make_send_buffer (authreq, PW_PASSWORD_EXPIRED);
	length += append_user_message (auth->data, msg, AUTH_STRING_LEN);
	send_answer (activefd, authreq, auth, length);
}

#if defined( ASCEND_SECRET )

/*************************************************************************
 *
 *	Function: make_secret
 *
 *	Purpose: Build an encrypted secret value to return in a reply
 *		 packet.  The secret is hidden by xoring with a MD5 digest
 *		 created from the shared secret and the authentication
 *		 vector.  We put them into MD5 in the reverse order from
 *		 that used when encrypting passwords to RADIUS.
 *
 *************************************************************************/

void
make_secret(digest, vector, secret, value)
	u_char	*digest;
	u_char	*vector;
	u_char	*secret;
	char	*value;
{
	u_char	buffer[ AUTH_STRING_LEN ];
	int		secretLen = strlen( (CONST char *)secret );
	int		ix;

	memcpy( buffer, vector, AUTH_VECTOR_LEN );
	memcpy( buffer + AUTH_VECTOR_LEN, secret, secretLen );
	md5_calc( digest, buffer, AUTH_VECTOR_LEN + secretLen );
	memset( buffer, 0, AUTH_STRING_LEN );
	for ( ix = 0; ix < AUTH_VECTOR_LEN; ix += 1 ) {
		digest[ ix ] ^= value[ ix ];
	}
}

#endif /* ASCEND_SECRET */

/*************************************************************************
 *
 *	Function: encrypt_tunnel_password
 *
 *	Encrypt the VPN Tunnel password according to this scheme:
 *
 *       Construct  a  plaintext version of the String field by concate-
 *       nating the Data-Length and Password sub-fields.  If  necessary,
 *       pad  the  resulting  string  until its length (in octets) is an
 *       even multiple of 16.  It is recommended that zero octets (0x00)
 *       be used for padding.  Call this plaintext P.
 *
 *       Call  the  shared  secret  S, the pseudo-random 128-bit Request
 *       Authenticator (from the corresponding Access-Request packet) R,
 *       and  the  contents  of the Salt field A.  Break P into 16 octet
 *       chunks p(1),  p(2)...p(i),  where  i  =  len(P)/16.   Call  the
 *       ciphertext blocks c(1), c(2)...c(i) and the final ciphertext C.
 *       Intermediate values b(1), b(2)...c(i) are required.  Encryption
 *       is  performed in the following manner ('+' indicates concatena-
 *       tion):
 *
 *          b(1) = MD5(S + R + A)    c(1) = p(1) xor b(1)   C = c(1)
 *          b(2) = MD5(S + c(1))     c(2) = p(2) xor b(2)   C = C + c(2)
 *                      .                      .
 *                      .                      .
 *                      .                      .
 *          b(i) = MD5(S + c(i-1))   c(i) = p(i) xor b(i)   C = C + c(i)
 *
 *       The   resulting   encrypted   String   field    will    contain
 *       c(1)+c(2)+...+c(i).
 *
 *    On receipt, the process is reversed to yield the plaintext String.
 *
 *
 *************************************************************************/


void
encrypt_tunnel_password (vector_ptr, passwd, secret, out_buf, out_len)
    u_char	 *vector_ptr;
    char         *passwd;
    char         *secret;
    char         *out_buf;
    int          *out_len;
{
    int		len;   /* Length of passwd */
    int         olen;  /* output length (mult of 16) */
    int		i;
    int		secretlen;
    char	*ptr, *cp;
    u_char	digest[AUTH_VECTOR_LEN];
    u_char	md5buf[AUTH_PASS_LEN + AUTH_VECTOR_LEN];

    static char	*func = "encrypt_tunnel_password";

    DEBUG("%s: entered\n", func);
		
    /* Encrypt the Password */
    len = strlen (passwd);

    /* Raise output length to next multiple of 16 */
    olen = ( ( len + 1 + 15 ) / 16 ) * 16;
    if( olen > AUTH_STRING_LEN )
    {
        olen -= 16;
	len = olen - 1;
    }

    ptr = out_buf;
    *ptr++ = 0;  /* Tag */
    ++salt;      /* Not too random yet */
    salt |= 0x8000;
    *ptr++ = salt >> 8;
    *ptr++ = salt & 0xff;
    *ptr = len;
    memcpy((char *) ptr + 1, (char *) passwd, len);

    /* Pad to multiple of 16 with 0's */
    memset((char *) ptr + 1 + len, '\0', olen - len);

    /* Calculate the 1st MD5 Digest */
    secretlen = strlen ( secret );
    cp = md5buf;
    memcpy( cp, secret, secretlen );
    cp += secretlen;
    memcpy( cp, vector_ptr, AUTH_VECTOR_LEN );
    cp += AUTH_VECTOR_LEN;
    *cp++ = salt >> 8;
    *cp++ = salt & 0xff;

    md5_calc (digest, md5buf, secretlen + AUTH_VECTOR_LEN + 2);

    while (TRUE) {

	/* Xor the password into the MD5 digest */
	for (i = 0; i < 16; i++)
	{
	    ptr[i] ^= digest[i];
	}

	ptr += 16;

	if( ptr - out_buf >= ( olen + 3 ) )
	    break;

	memcpy ((char *) md5buf + secretlen, (char *) ptr - 16, 16);
	md5_calc (digest, md5buf, secretlen + 16);
    }

    memset( md5buf, 0, secretlen );
    *out_len = olen + 3;
    return; 
}


/*************************************************************************
 *
 *	Function: send_accept
 *
 *	Purpose: Reply to the request with an ACKNOWLEDGE.  Also attach
 *		 reply attribute value pairs and any user message provided.
 *
 *************************************************************************/

void
send_accept(authreq, reply_ptr, msg, activefd)
	AUTH_REQ	*authreq;
	VALUE_PAIR	**reply_ptr;
	CONST char	*msg;
	int		activefd;
{
	AUTH_HDR	*auth;
	int		length = AUTH_HDR_LEN;
	u_char		*ptr;
	int		len;
	UINT4		lvalue;
	u_char		digest[AUTH_VECTOR_LEN];
	VALUE_PAIR	*pool;
	VALUE_PAIR	*reply;
	int		global_pools = 0;
        int             tun_pw_len;

	DEBUG("send_accept: %s\n", request_id (authreq));

	auth = make_send_buffer (authreq, PW_AUTHENTICATION_ACK);

	while ((pool = cut_attribute(reply_ptr, ASCEND_ASSIGN_IP_GLOBAL_POOL))
	       != NULL_PAIR) {
		global_pools++;
		pairfree (pool);
	}
	if (global_pools) {
		do {
			pool = cut_attribute(reply_ptr, ASCEND_ASSIGN_IP_POOL);
			pairfree (pool);
		} while (pool);
		pool = make_pair (ASSIGN_IP_POOL_NAME, ASCEND_ASSIGN_IP_POOL, PW_TYPE_INTEGER);
		pool->lvalue = RADIPA_POOL_INDEX;
		insertValuePair(reply_ptr, pool);
	}

	/* Load up the configuration values for the user */
	ptr = auth->data;
	reply = *reply_ptr;
	while (reply != NULL_PAIR) {
		debug_pair("reply", stdout, reply);
		*ptr++ = reply->attribute;
		switch(reply->type) {
		case PW_TYPE_STRING:
		case PW_TYPE_PHONESTRING:
#if defined( ASCEND_SECRET )
			if (( reply->attribute == ASCEND_SEND_SECRET ) ||
			    ( reply->attribute == ASCEND_RECV_SECRET )) {
				make_secret( digest, authreq->vector,
					     authreq->secret, reply->strvalue );
				*ptr++ = AUTH_VECTOR_LEN + 2;
				memcpy( ptr, digest, AUTH_VECTOR_LEN );
				ptr += AUTH_VECTOR_LEN;
				length += AUTH_VECTOR_LEN + 2;
				break;
			}
#endif
			/* Tunnel password needs to be encrypted */
			if( reply->attribute == PW_TUNNEL_PASSWORD ) {
                            encrypt_tunnel_password (authreq->vector, 
						     reply->strvalue,
                                                     authreq->secret, 
                                                     ptr + 1, 
                                                     &tun_pw_len);
			    *ptr++ = tun_pw_len + 2;
			    ptr += tun_pw_len;
			    length += tun_pw_len + 2;
			    break;
			}

			len = strlen(reply->strvalue);
			if (len > AUTH_STRING_LEN) {
				len = AUTH_STRING_LEN;
			}
			*ptr++ = len + 2;
			memcpy(ptr, reply->strvalue,len);
			ptr += len;
			length += len + 2;
			break;

		case PW_TYPE_DATE:
		case PW_TYPE_INTEGER:
		case PW_TYPE_IPADDR:
			*ptr++ = sizeof(UINT4) + 2;
			lvalue = htonl(reply->lvalue);
			memcpy(ptr, &lvalue, sizeof(UINT4));
			ptr += sizeof(UINT4);
			length += sizeof(UINT4) + 2;
			break;

#if defined( BINARY_FILTERS )
		case PW_TYPE_FILTER_BINARY:

			/* The binary representation of the filter is in
			   reply->strvalue.  It's length is in reply->lvalue */

			*ptr++ = reply->lvalue + 2;
			memcpy( ptr, reply->strvalue, reply->lvalue );
			ptr += reply->lvalue;
			length += reply->lvalue + 2;
			break;
#endif /* BINARY_FILTERS */

		default:
			break;
		}

		reply = reply->next;
	}

	length += append_user_message (ptr, msg, AUTH_STRING_LEN);
	send_answer (activefd, authreq, auth, length);
}


/*************************************************************************
 *
 *	Function: unix_pass
 *
 *	Purpose: Check the users password against the standard UNIX
 *		 password table.
 *
 *************************************************************************/

int
unix_pass(name, passwd)
	CONST char	*name;
	char		*passwd;
{
	struct passwd	*pwd;
	char		*encpw;
	char		*encrypted_pass;
#if !defined(NOSHADOW)
#if defined(M_UNIX)
	struct passwd	*spwd;
#else
	struct spwd	*spwd;
#endif
#if defined(SHADOW_EXPIRE)              
#if !defined(DAY)
#define DAY     (60*60*24)
#endif /* !DAY */
        int             today = time(NULL)/DAY;
#endif /* SHADOW_EXPIRE */
#endif /* !NOSHADOW */

#if defined(ALTSHADOW)
	FILE		*the_shadowfile;
#define	SHADFILE	"/etc/raddb/shadow"
	int		passeq = -1, notdone = 1;

	if ((the_shadowfile = fopen (SHADFILE,"r")) == (FILE *)NULL) {
		return UNIX_GETPWNAME_ERR;
	}
	spwd = fgetspent(the_shadowfile);
	while (spwd) {
		if (!strcmp(spwd->sp_namp, name)) {
			encrypted_pass = spwd->sp_pwdp;
			break;
		} else {
			spwd = fgetspent(the_shadowfile);
		}
	}
	fclose(the_shadowfile);
	if (!spwd) {
		return UNIX_GETPWNAME_ERR;
	}
#else
	/* Get encrypted password from password file */
	if((pwd = getpwnam(name)) == NULL) {
		return UNIX_GETPWNAME_ERR;
	}
	encrypted_pass = pwd->pw_passwd;

#if !defined(NOSHADOW)
	if(strcmp(pwd->pw_passwd, "x") == 0) {
		if((spwd = getspnam(name)) == NULL) {
			return UNIX_GETSHDWNAME_ERR;
		}
#if defined(M_UNIX)
		encrypted_pass = spwd->pw_passwd;
#else
		encrypted_pass = spwd->sp_pwdp;
#endif	/* M_UNIX */
	}
#endif  /* NOSHADOW */
#endif  /* ALTSHADOW */
#if !defined(NOSHADOW)
#if defined(SHADOW_EXPIRE)
	if ((spwd->sp_expire > 0) && (spwd->sp_expire < today)) {
		/* It's expired */
		return -1;
	}
	if (spwd->sp_lstchg == 0) {
		/* Root expired it */  
		return -1;
	}
	if ((spwd->sp_max >= 0) && (spwd->sp_inact >= 0) &&
	    (spwd->sp_lstchg + spwd->sp_max + spwd->sp_inact < today)) {
		/* Password is too old */
		return -1;
	}
#endif  /* SHADOW_EXPIRE */
#endif	/* !NOSHADOW */

	/* Run encryption algorythm */
	if (!strncmp(encrypted_pass,"$1$",3)) {
		encpw = md5_crypt(passwd, encrypted_pass);
	} else {
		encpw = crypt(passwd, encrypted_pass);
	}
	/* Check it */
	if(strcmp(encpw, encrypted_pass)) {
		return UNIX_BAD_PASSWORD;
	}
	return(0);
}

/*************************************************************************
 *
 *	Function: sql_pass
 *
 *	Purpose: Check the users password against the standard UNIX
 *		 password table.
 *
 *************************************************************************/

int
sql_pass(name, passwd)
	CONST char	*name;
	char		*passwd;
{
	struct passwd	*pwd;
	char		*encpw;
	char		*encrypted_pass;
#if !defined(NOSHADOW)
#if defined(M_UNIX)
	struct passwd	*spwd;
#else
	struct spwd	*spwd;
#endif
#if defined(SHADOW_EXPIRE)              
#if !defined(DAY)
#define DAY     (60*60*24)
#endif /* !DAY */
        int             today = time(NULL)/DAY;
#endif /* SHADOW_EXPIRE */
#endif /* !NOSHADOW */

#if defined(ALTSHADOW)
	FILE		*the_shadowfile;
#define	SHADFILE	"/etc/raddd/shadow"
	int		passeq = -1, notdone = 1;

	if ((the_shadowfile = fopen (SHADFILE,"r")) == (FILE *)NULL) {
		return UNIX_GETPWNAME_ERR;
	}
	spwd = fgetspent(the_shadowfile);
	while (spwd) {
		if (!strcmp(spwd->sp_namp, name)) {
			encrypted_pass = spwd->sp_pwdp;
			break;
		} else {
			spwd = fgetspent(the_shadowfile);
		}
	}
	fclose(the_shadowfile);
	if (!spwd) {
		return UNIX_GETPWNAME_ERR;
	}
#else
	/* Get encrypted password from password file */
	if((pwd = getpwnam(name)) == NULL) {
		return UNIX_GETPWNAME_ERR;
	}
	encrypted_pass = pwd->pw_passwd;

#if !defined(NOSHADOW)
	if(strcmp(pwd->pw_passwd, "x") == 0) {
		if((spwd = getspnam(name)) == NULL) {
			return UNIX_GETSHDWNAME_ERR;
		}
#if defined(M_UNIX)
		encrypted_pass = spwd->pw_passwd;
#else
		encrypted_pass = spwd->sp_pwdp;
#endif	/* M_UNIX */
	}
#endif  /* NOSHADOW */
#endif  /* ALTSHADOW */
#if !defined(NOSHADOW)
#if defined(SHADOW_EXPIRE)
	if ((spwd->sp_expire > 0) && (spwd->sp_expire < today)) {
		/* It's expired */
		return -1;
	}
	if (spwd->sp_lstchg == 0) {
		/* Root expired it */  
		return -1;
	}
	if ((spwd->sp_max >= 0) && (spwd->sp_inact >= 0) &&
	    (spwd->sp_lstchg + spwd->sp_max + spwd->sp_inact < today)) {
		/* Password is too old */
		return -1;
	}
#endif  /* SHADOW_EXPIRE */
#endif	/* !NOSHADOW */

	/* Run encryption algorythm */
	if (!strncmp(encrypted_pass,"$1$",3)) {
		encpw = md5_crypt(passwd, encrypted_pass);
	} else {
		encpw = crypt(passwd, encrypted_pass);
	}
	/* Check it */
	if(strcmp(encpw, encrypted_pass)) {
		return UNIX_BAD_PASSWORD;
	}
	return(0);
}

/*************************************************************************
 *
 *	Function: get_client_info
 *
 *	Purpose: Return the IP address and secret of the given host
 *
 *	Input:	 rqstIpAddr
 *
 *	Returns: If rqstIpAddr match
 *			set IP addr, hostname, and secret
 *			return 0;
 *		 else
 *			set IP addr = 0
 *			return error code
 *
 *************************************************************************/

int
get_client_info(rqstIpAddr, ipaddr, secret, hostnm)
	UINT4	rqstIpAddr;
	UINT4	*ipaddr;
	u_char	*secret;
	char	*hostnm;
{
	FILE	*clientfp;
	char	buf[1024];

	*ipaddr = (UINT4)0;
	secret[0] = hostnm[0] = '\0';

	/* Find the client in the database */
	sprintf(buf, "%s/%s", radius_dir, RADIUS_CLIENTS);
	if((clientfp = fopen(buf, "r")) == (FILE *)NULL) {
		int error = errno;
		fprintf(stderr,
			"%s: err(%d): Couldn't open %s to find client\n",
				progname, error, buf);
		return CLIENTFILE_READ_ERR;
	}
	while(fgets(buf, sizeof(buf), clientfp) != (char *)NULL) {
		if(*buf == '#') {
			continue;
		}
		if(sscanf(buf, "%s%s", hostnm, secret) != 2) {
			continue;
		}

		/*
		 * Validate the requesting IP address -
		 * Not secure, but worth the check for accidental requests
		 */
		*ipaddr = get_ipaddr(hostnm);
		if(*ipaddr == rqstIpAddr) {
			break;
		}
		*ipaddr = (UINT4)0;
		secret[0] = hostnm[0] = '\0';
	}
	fclose(clientfp);

	return (*ipaddr) ? 0 : WRONG_NAS_ADDR;
}

/*************************************************************************
 *
 *	Function: calc_digest
 *
 *	Purpose: Validates the requesting client NAS.  Calculates the
 *		 digest to be used for decrypting the users password
 *		 based on the clients private key.
 *
 *************************************************************************/

int
calc_digest(digest, authreq)
	u_char		*digest;
	AUTH_REQ	*authreq;
{
	u_char	buffer[512];
	u_char	secret[256];
	char	hostnm[256];
	int	secretlen;
	UINT4	ipaddr;
	int	status;

	status = get_client_info( authreq->ipaddr, &ipaddr, secret, hostnm );
	memset(buffer, 0, sizeof(buffer));
	if( status != 0 ) {
		log_err("Calc_digest: %s: %s\n",
			request_id (authreq), get_errmsg(status));
		memset(secret, 0, sizeof(secret));
		return status;
	}

	/* Use the secret to setup the decryption digest */
	secretlen = strlen((char *)secret);
	strcpy((char *)buffer, (char *)secret);
	memcpy((char *)(buffer + secretlen), (char *)authreq->vector,
		AUTH_VECTOR_LEN);
	md5_calc(digest, buffer, secretlen + AUTH_VECTOR_LEN);
	strcpy((char *)authreq->secret, (char *)secret);
	memset(buffer, 0, sizeof(buffer));
	memset(secret, 0, sizeof(secret));
	return(0);
}

/*************************************************************************
 *
 *	Function: debug_pair
 *
 *	Purpose: Print the Attribute-value pair to the desired File.
 *
 *************************************************************************/

void
debug_pair(prefix, fd, pair)
	char CONST	*prefix;
	FILE		*fd;
	VALUE_PAIR	*pair;
{
	if(debug_flag) {
		fprintf(fd, "  %s: ", prefix);
		fprint_attr_val(fd, pair);
		fputs("\n", fd);
	}
}

/*************************************************************************
 *
 *	Function: usage
 *
 *	Purpose: Display the syntax for starting this program.
 *
 *************************************************************************/

static void
usage()
{
	fprintf(errf,
		"Usage: %s [ -a acct_dir ] [ -c ] [ -d db_dir ] [ -p ]\n\t[ -s ] [ -u db_file ] [ -v ] [ -w ] [ -x ] [ -A none|services|incr ]\n",
			progname);
	exit(-1);
}

/*************************************************************************
 *
 *	Function: log_err
 *
 *	Purpose: Log the error message provided to the error log with
		 a time stamp.
 *
 *************************************************************************/

int
#if __STDC__ == 1
log_err(CONST char *fmt, ...)
#else
log_err(va_alist) va_dcl
#endif
{
	FILE	*msgfp;
	char	buffer[128];
	time_t	timeval;

	sprintf(buffer, "%s/%s", radius_dir, RADIUS_LOG);
	if((msgfp = fopen(buffer, "a")) == (FILE *)NULL) {
		int error = errno;
		fprintf(errf,
			"%s: err(%d): Couldn't open %s for logging\n",
				progname, error, buffer);
		return LOGFILE_APPEND_ERR;
	} else {
	    va_list ap;
#if __STDC__ == 1
	    va_start (ap, fmt);
#else
	    CONST char *fmt;
	    va_start (ap);
	    fmt = va_arg(ap, char *);
#endif
	    timeval = time(0);
	    fprintf(msgfp, "%-24.24s: ", ctime(&timeval));
	    vfprintf (msgfp, fmt, ap);
	    vdebugf (fmt, ap);
	    va_end (ap);
	}
	fclose(msgfp);
	return(0);
}

/*************************************************************************
 *
 *	Function: config_init
 *
 *	Purpose: intializes configuration values:
 *
 *		 expiration_seconds - When updating a user password,
 *			the amount of time to add to the current time
 *			to set the time when the password will expire.
 *			This is stored as the VALUE Lifetime-In-Days
 *			in the dictionary as number of days.
 *
 *		warning_seconds - When acknowledging a user authentication
 *			time remaining for valid password to notify user
 *			of password expiration.
 *			This is stored as the VALUE Days-Of-Warning
 *			in the dictionary as number of days.
 *
 *************************************************************************/

int
config_init()
{
	DICT_VALUE	*dval;

	if((dval = dict_valfind(PW_LIFETIME)) == (DICT_VALUE *)NULL) {
		DEBUG("config_init: dict_valfind(%s) failed\n",
			PW_LIFETIME);
		expiration_seconds = (UINT4)0;
	}
	else {
		expiration_seconds = dval->value * (UINT4)SECONDS_PER_DAY;
	}
	if((dval = dict_valfind(PW_WARNTIME)) == (DICT_VALUE *)NULL) {
		warning_seconds = (UINT4)0;
	}
	else {
		warning_seconds = dval->value * (UINT4)SECONDS_PER_DAY;
	}
#if( 0 )
	DEBUG("config_init: expire-secs = %ld\n", expiration_seconds);
#endif
	return(0);
}

/*************************************************************************
 *
 *	Function: set_expiration
 *
 *	Purpose: Set the new expiration time by updating or adding
		 the Expiration attribute-value pair.
 *
 *************************************************************************/

int
set_expiration(user_check, expiration)
	VALUE_PAIR	*user_check;
	UINT4		expiration;
{
	VALUE_PAIR	*exppair;
	VALUE_PAIR	*prev;
	struct timeval	tp;

	if (user_check == NULL_PAIR) {
		return NULL_VALUEPAIR;
	}

	/* Look for an existing expiration entry */
	exppair = user_check;
	prev = NULL_PAIR;
	while (exppair != NULL_PAIR) {
		if(exppair->attribute == ASCEND_PW_EXPIRATION) {
			break;
		}
		prev = exppair;
		exppair = exppair->next;
	}
	if (exppair == NULL_PAIR) {
		/* Add a new attr-value pair */
		exppair = make_pair (PW_EXPIRATION_NAME, ASCEND_PW_EXPIRATION, PW_TYPE_DATE);
		/* Attach it to the list. */
		prev->next = exppair;
	}

	/* calculate a new expiration */
#if defined(_SVID_GETTOD) || defined(aix)
	gettimeofday(&tp);
#else
	gettimeofday(&tp, 0);
#endif
	exppair->lvalue = tp.tv_sec + expiration;
	return(0);
}

/*************************************************************************
 *
 *	Function: pw_expired
 *
 *	Purpose: Tests to see if the users password has expired.
 *
 *	Return: Number of days before expiration if a warning is required
 *		otherwise 0 for success and PWD_EXPIRED for failure.
 *
 *************************************************************************/

int
pw_expired(exptime, usr_exp_days, usr_warn_days)
	UINT4	exptime;
	UINT4	usr_exp_days;
	UINT4	usr_warn_days;
{
	UINT4	exp_secs;
	UINT4	warn_secs;
	struct timeval	tp;
	UINT4	exp_remain;
	int	exp_remain_int;

#if( 0 )
	DEBUG("pw_expired(time=%ld, exp=%ld, warn=%ld)\n",
		exptime, usr_exp_days, usr_warn_days);
#endif
	if( usr_exp_days ) {
		exp_secs = usr_exp_days * (UINT4)SECONDS_PER_DAY;
	}
	else {
		exp_secs = expiration_seconds;
	}
	if( usr_warn_days ) {
		warn_secs = usr_warn_days * (UINT4)SECONDS_PER_DAY;
	}
	else {
		warn_secs = warning_seconds;
	}

	if( exp_secs == (UINT4)0 ) {
		return(0);
	}

#if defined(_SVID_GETTOD) || defined(aix)
	gettimeofday(&tp);
#else
	gettimeofday(&tp, 0);
#endif
	if(tp.tv_sec > exptime) {
		return PWD_EXPIRED;
	}
	if(warn_secs != (UINT4)0) {
		if(tp.tv_sec > exptime - warn_secs) {
			exp_remain = exptime - tp.tv_sec;
			exp_remain /= (UINT4)SECONDS_PER_DAY;
			exp_remain_int = exp_remain;
			return(exp_remain_int);
		}
	}
	return(0);
}

static void
sig_fatal(sig)
	int	sig;
{
	if(acct_pid > 0) {
		kill(acct_pid, SIGKILL);
	}

	fprintf(errf, "%s: exit on signal (%d)\n", progname, sig);
	fflush(errf);
	exit(1);
}

static void
sig_hangup(sig)
	int	sig;
{
	signal (sig, sig_hangup);
	retransmit_flag = TRUE;
}

static void
sig_suicide(sig)
	int	sig;
{
	(void) sig;		/* unused */
	suicide_flag = TRUE;
}

/*************************************************************************
 *
 *	Function: send_nextcode
 *
 *	Purpose: Reply to the request with a Next Code request.  Append
 *		any message given, and also attach a state value.
 *
 *************************************************************************/

void
send_nextcode(authreq, msg, msgLen, state, stateLen, activefd)
	AUTH_REQ	*authreq;
	CONST char	*msg;
	int		msgLen;
	CONST char	*state;
	int		stateLen;
	int		activefd;
{
	AUTH_HDR	*auth;
	int		length = AUTH_HDR_LEN;
	u_char		*ptr;

	DEBUG("send_nextcode:\n");

	auth = make_send_buffer (authreq, PW_NEXT_PASSCODE);
	ptr = auth->data;

	/* Append the nextcode msg */
	if( msgLen <= AUTH_STRING_LEN ) {
		DEBUG("...Adding nextcode msg, len=%d\n", msgLen);
		*ptr++ = PW_PORT_MESSAGE;
		*ptr++ = msgLen + 2;
		memcpy(ptr, msg, msgLen);
		if( debug_flag == 1 ) {
			debugf("nextcode msg before encrypt: \"");
			fwrite(msg, 1, msgLen, stdout);
			puts("\"");
		}
		ptr += msgLen;
		length += msgLen + 2;
	}

	/* Append the state info */
	if( state != (char *)NULL ) {
	    DEBUG("...Adding state <%s>, len=%d\n", state, stateLen);
	    *ptr++ = PW_STATE;
	    *ptr++ = stateLen + 2;
	    memcpy(ptr, state, stateLen);
	    ptr += stateLen;
	    length += stateLen + 2;
	}

	DEBUG("send_nextcode: %s: length = %d\n", request_id (authreq), length);

	send_answer (activefd, authreq, auth, length);
}

/*************************************************************************
 *
 *	Function: send_newpin
 *
 *	Purpose: Reply to the request with a Next Code request.  Also
 *		attach a state value.
 *
 *************************************************************************/

	/* not implemented yet */
void
send_newpin(authreq, state, stateLen, activefd)
	AUTH_REQ	*authreq;
	CONST char	*state;
	int		stateLen;
	int		activefd;
{
	(void)authreq;
	(void)state;
	(void)stateLen;
	(void)activefd;
	DEBUG("send_newpin: NOT IMPLEMENTED\n");
}

/************************************************************************/
/*									*/
/*		Map error codes to messages				*/
/*									*/
/************************************************************************/

CONST char *
get_errmsg(errcode)
	int		errcode;
{
	CONST char *emsg = (char *)0;

	if( (errcode > FIRST_DBASE_ERROR) && (errcode < LAST_DBASE_ERROR) ) {
		int i = (errcode - FIRST_DBASE_ERROR) - 1;
		if( db_errmsgs[i].ecode == errcode ) {
			emsg = db_errmsgs[i].emsg;
		} else {
			emsg = "INTERNAL: ErrMsgTbl out of order";
			log_err("INTERNAL: ErrMsgs out of order for errcode %d\n",
				errcode);
		}
	} else {
		emsg = "INTERNAL: Unknown error";
		log_err("INTERNAL: Unknown error code %d\n", errcode);
	}
	return emsg;
}

static VALUE_PAIR *
copyValuePair(source)
	VALUE_PAIR	*source;
{
	VALUE_PAIR	*pair = MALLOC (VALUE_PAIR, 1);

	if (pair == NULL_PAIR) {
		fprintf(errf, "%s: no memory\n",progname);
		exit(MEMORY_ERR);
	}
	*pair = *source;
	pair->next = NULL_PAIR;
	return pair;
}

static void
insertValuePair(list,pair)
	VALUE_PAIR	**list;
	VALUE_PAIR	*pair;
{
	pair->next = *list;
	*list = pair;
}

#if defined(SAFEWORD)
/************************************************************************/
/*									*/
/*		Enigma Safeword Interface routines			*/
/*									*/
/************************************************************************/


	/* simple debug output */
void
dbgPblk(pb)
	struct pblk	*pb;
{
	if( debug_flag ) {
		debugf("\n");
		printf("    PBLK: uport    = <%s>\n", pb->uport);
		printf("    PBLK: id       = <%s>\n", pb->id);
		printf("    PBLK: chal     = <%s>\n", pb->chal);
		printf("    PBLK: dynpwd   = <%s>\n", pb->dynpwd);
		printf("    PBLK: fixpwd   = <%s>\n", pb->fixpwd);
		printf("    PBLK: nfixpwd  = <%s>\n", pb->nfixpwd);
		printf("    PBLK: errcode  = <%d>\n", pb->errcode);
		printf("    PBLK: lastmode = <%d>\n", pb->lastmode);
		printf("    PBLK: mode     = <%d>\n", pb->mode);
		printf("    PBLK: fixmin   = <%d>\n", pb->fixmin);
		printf("    PBLK: dynpwdf  = <%d>\n", pb->dynpwdf);
		printf("    PBLK: fixpwdf  = <%d>\n", pb->fixpwdf);
		printf("    PBLK: echodyn  = <%d>\n", pb->echodyn);
		printf("    PBLK: echofix  = <%d>\n", pb->echofix);
		printf("    PBLK: status   = <%d>\n", pb->status);
	}
}


/*************************************************************************
 *
 *	Function: safeword_chall
 *
 *	Purpose: Obtain a (possibly NUL) challenge
 *
 *************************************************************************/

int
safeword_chall(authInfo, pb)
	AuthInfo	*authInfo;
	struct pblk	*pb;
{
	DEBUG("safeword_chall:\n");

	/*
	 *
	 *  Fill in param block id field for use by the external
	 *  authentication server:
	 *
	 */
	initpb (pb);
       	strncpy(pb->id, authInfo->authName->strvalue, sizeof(pb->id));
	pb->id[sizeof(pb->id)-1] = 0;	/* ensure NUL-termimation */

	/*
	 * Invoke the SafeWord API
	 */
	pb->mode = CHALLENGE;
	pbmain(pb);

	/* diagnostic */
	DEBUG("after safeword {challenge}\n");
	dbgPblk(pb);

	switch( pb->status ) {
	case GOOD_USER:
		DEBUG("safeword_chall: SafeWord Challenging(%s).\n",
			pb->chal);
		return SAFEWORD_CHALLENGING;
	default:
		DEBUG("safeword_chall: SafeWord failed = %d.\n", pb->status);
		return SAFEWORD_FAILED;
	}
}

/*************************************************************************
 *
 *	Function: safeword_eval
 *
 *	Purpose: Check the users password against the Safeword
 *		 password info.
 *
 *************************************************************************/
int
safeword_eval(authInfo, pb)
	AuthInfo	*authInfo;
	struct pblk	*pb;
{
	VALUE_PAIR	*authPwd = authInfo->authPwd;
	VALUE_PAIR	*authState = authInfo->authState;
	char		swPwd[AUTH_PASS_LEN + 1];
	int		i;
	char		*fixed;

	DEBUG("safeword_eval:\n");

	decryptAuthPwd ("safeword_eval", authInfo, !authInfo->chkImmediate, swPwd);

	/* see if we have the <dynamic><,><fixed> format given to us */
	fixed = strchr(swPwd, ',');
	if( fixed ) {
		*fixed = '\0';	/* NUL-terminate dynamic password */
		fixed++;
		DEBUG("received fixed pwd %s\n", fixed);
	}

	memcpy(BEG_PB_1ST(pb), authState->strvalue, LEN_PB_1ST(pb));
	memcpy(BEG_PB_2ND(pb),
		authState->strvalue + LEN_PB_1ST(pb),
		LEN_PB_2ND(pb));
	for( i = 0; i < AUTH_PASS_LEN; i++ ) {
		if( (pb->chal[i] < ' ') || (pb->chal[i] > 0x7e) ) {
			pb->chal[i] = 0;
			break;
		}
	}
	DEBUG("Old challenge is <%s>\n", pb->chal);

	/* copy new dynamic password to the SafeWord param block */
	pb->dynpwd[0] = 0;
	if( pb->dynpwdf ) {
    		strncpy(pb->dynpwd, swPwd, sizeof(pb->dynpwd));
		pb->dynpwd[sizeof(pb->dynpwd)-1] = 0;
	}
	else {
		log_err("safeword_eval(): dynamic pwd flag not set!\n");
		return SAFEWORD_FAILED;
	}

	pb->fixpwd[0] = 0;
	if( fixed ) {
		/* copy fixed pwd *always* but bitch if the flag is clear */
    		strncpy(pb->fixpwd, fixed, sizeof(pb->fixpwd));
		pb->fixpwd[sizeof(pb->fixpwd)-1] = 0;
		if( !pb->fixpwdf ) {
			log_err("user %s, fixed pwd w/o fixpwdf, NAS %s\n",
					authInfo->authName->strvalue,
					request_id (authInfo->authreq));
		}
	}

	pb->mode = EVALUATE_ALL;
	pbmain(pb);

	/* diagnostic */
	DEBUG("after safeword {eval}\n");
	dbgPblk(pb);

	switch( pb->status ) {
	case PASS:
		DEBUG("safeword_eval: SafeWord PASSED.\n");
		return SAFEWORD_PASSED;
	default:
		DEBUG("safeword_eval: SafeWord FAILED, = %d.\n", pb->status);
		return SAFEWORD_FAILED;
	}
}

/*************************************************************************
 *
 *	Function: safeword_pass
 *
 *	Purpose: Check the users password against the Safeword
 *		 password info.
 *
 *************************************************************************/
	/*
	 * Since the Radius database has detected a default keyword as the
	 * special "SAFEWORD" flag, it is now appropriate to ask SafeWord
	 * if it has any data on him (or her). We check the given STATE;
	 * if NUL, then this is a challenge request, else it is
	 * an evaluation request
	 */
int
safeword_pass(authInfo, pb)
	AuthInfo	*authInfo;
	struct pblk	*pb;
{
	DEBUG("safeword_pass:\n");
	if (authInfo->authState != NULL_PAIR) {
		if( authInfo->authState->attribute == PW_STATE ) {
			if( authInfo->authState->size ) {
				return safeword_eval(authInfo, pb);
			}
			else {
				return safeword_chall(authInfo, pb);
			}
		}
	}
	DEBUG("safeword_pass: FAILED: no state attribute\n");
	return SAFEWORD_FAILED;
}


/* ---------------------------- initpb () ---------------------------- */

	/* initialize parameter block */
void
initpb(pb)
	struct pblk	*pb;
{
	memset(pb, 0, sizeof(*pb));

	strcpy (pb->uport, "custpb");
	pb->status = NO_STATUS;
}
#endif

#if defined(ACE)
/************************************************************************/
/*									*/
/*		Security Dynamics ACE/Server Interface routines		*/
/*									*/
/************************************************************************/


	/* simple debug output */
void
dbgSdClient(sd)
	struct SD_CLIENT *sd;
{
    if( debug_flag ) {
	debugf("\n");
	printf("    SD_CLIENT: appId    = <%d>\n", sd->application_id);
	printf("    SD_CLIENT: name     = <%s>\n", sd->username);

#if defined(UINT4_IS_UINT)
	printf("    SD_CLIENT: pc_time  = <%d>\n", sd->passcode_time);
#else
	printf("    SD_CLIENT: pc_time  = <%ld>\n", sd->passcode_time);
#endif
	printf("    SD_CLIENT: passcode = <%s>\n", sd->validated_passcode);
    }
}


/*************************************************************************
 *
 *	Function: ace_eval
 *
 *	Purpose: Check the users password against the ACE
 *		 password info.
 *
 *************************************************************************/
int
ace_eval(authInfo, sd)
	AuthInfo	*authInfo;
	struct SD_CLIENT *sd;
{
	VALUE_PAIR	*authName = authInfo->authName;
	VALUE_PAIR	*authPwd = authInfo->authPwd;
	int		status;
	char		acePwd[AUTH_PASS_LEN + 1];
	char		*user;
	char		username[LENACMNAME];
				/* N.B.: sdi_ath.h uses LENACMNAME although
				   one would think that LENUSERNAME would
				   be more appropriate */

	/*
	 * Establish communications with the server
	 */
	memset(sd, 0, sizeof(*sd));	/* clear shared param block */
	creadcfg();			/* accesses sdconf.rec */
	if( sd_init(sd) ) {
		DEBUG("ace_eval: Comm Server init failed!\n");
		return ACE_FAILED;
	}

	DEBUG("ace_eval: ace_username: %s\n", authName->strvalue);
	strncpy(username, authName->strvalue, sizeof(username));
	username[sizeof(username)-1] = 0;	/* ensure final NUL */
	user = (char *)NULL;

	decryptAuthPwd ("ace_eval", authInfo, !authInfo->chkImmediate, acePwd);

	/* see if we have the <password><.><username> format given to us */
	user = strchr(acePwd, '.');
	if( user ) {
		*user = '\0';	/* NUL-terminate dynamic password */
		user++;
		DEBUG("received username %s\n", user);
	}

	/* copy the username to the ACE param block */
	sd->username[0] = 0;
	if( !user ) {
		strcpy(sd->username, username);
	} else {
		strcpy(sd->username, user);
	}

	DEBUG_PWD (("Ready to try user %s with pwd %s\n", sd->username, acePwd));
	status = sd_check(acePwd, sd->username, sd);
	switch( status ) {
	case ACM_OK:
		DEBUG("user %s authenticated with pwd %s\n",
			sd->username, acePwd);
		status = ACE_PASSED;
		break;
	case ACM_ACCESS_DENIED:
		DEBUG("user %s with pwd %s NOT AUTHENTICATED!\n",
			sd->username, acePwd);
		status = ACE_FAILED;
		break;
	case ACM_NEXT_CODE_REQUIRED:
		DEBUG("user %s with pwd %s requires next code\n",
			sd->username, acePwd);
		status = ACE_NEXT_PASSCODE;
		break;
	case ACM_NEW_PIN_REQUIRED:
		DEBUG("user %s with pwd %s requires new pin\n",
			sd->username, acePwd);
		status = ACE_NEW_PIN;
		break;
	default:
		DEBUG("user %s with pwd %s : ERROR %d\n",
			sd->username, acePwd, status);
		status = ACE_FAILED;
		break;
	}
	dbgSdClient(sd);

	return status;
}

/*************************************************************************
 *
 *	Function: ace_next
 *
 *	Purpose: Check the users password against the ACE
 *		 password info.
 *
 *************************************************************************/
int
ace_next(authInfo, sd)
	AuthInfo	*authInfo;
	struct SD_CLIENT *sd;
{
	VALUE_PAIR	*authName = authInfo->authName;
	VALUE_PAIR	*authPwd = authInfo->authPwd;
	VALUE_PAIR	*authState = authInfo->authState;
	int		status;
	char		acePwd[AUTH_PASS_LEN + 1];
	char		*src;
	char		*user;
	char		username[LENACMNAME];
				/* N.B.: sdi_ath.h uses LENACMNAME although
				   one would think that LENUSERNAME would
				   be more appropriate */

	DEBUG("ace_next: ace_username: %s\n", authName->strvalue);
	strncpy(username, authName->strvalue, sizeof(username));
	username[sizeof(username)-1] = 0;	/* ensure final NUL */
	user = (char *)NULL;

	decryptAuthPwd ("ace_next", authInfo, 1, acePwd);

	/* see if we have the <password>.<username> format given to us */
	user = strchr(acePwd, '.');
	if( user ) {
		*user = '\0';	/* NUL-terminate dynamic password */
		user++;
		DEBUG("received username %s\n", user);
	}

	src = &authState->strvalue[1];	/* skip length field */
	memcpy(BEG_SD_1ST(sd), src, LEN_SD_1ST(sd));
	memcpy(BEG_SD_2ND(sd), src + LEN_SD_1ST(sd), LEN_SD_2ND(sd));
	memcpy(BEG_SD_3RD(sd), src + LEN_SD_1ST(sd) + LEN_SD_2ND(sd),
			LEN_SD_3RD(sd));

	/* copy the username to the ACE param block */
	sd->username[0] = 0;
	if( !user ) {
		strcpy(sd->username, username);
	} else {
		strcpy(sd->username, user);
	}

	DEBUG_PWD (("Ready to try user %s with pwd %s\n", sd->username, acePwd));
	status = sd_next(acePwd, sd);
	switch( status ) {
	case ACM_OK:
		DEBUG("user %s authenticated with pwd %s\n",
			sd->username, acePwd);
		status = ACE_PASSED;
		break;
	case ACM_ACCESS_DENIED:
		DEBUG("user %s with pwd %s NOT AUTHENTICATED!\n",
			sd->username, acePwd);
		status = ACE_FAILED;
		break;
	default:
		DEBUG("user %s with pwd %s : ERROR %d\n",
			sd->username, acePwd, status);
		break;
	}

	/* diagnostic */
	DEBUG("after ace {nextcode}\n");
	dbgSdClient(sd);

	return status;
}

/*************************************************************************
 *
 *	Function: ace_pass
 *
 *	Purpose: Check the users password against the ACE
 *		 password info.
 *
 *************************************************************************/
	/*
	 * Since the Radius database has detected a default keyword as the
	 * special "ACE" flag, it is now appropriate to ask Ace
	 * if it has any data on him (or her). We check the STATE;
	 * if STATE == DUMMY,
	 * then this is a challenge request,
	 * else if the contained STATE length == NEXTCODE STATE length
	 *	then this is a response to a NEXTCODE request
	 * else it is an evaluation request
	 */
int
ace_pass(authInfo, sd)
	AuthInfo	*authInfo;
	struct SD_CLIENT *sd;
{
	VALUE_PAIR	*authState = authInfo->authState;

	if (authState != NULL_PAIR) {
		if( !memcmp(authState->strvalue, ACE_DUMMY_STATE, LEN_ACE_DUMMY_STATE) ) {
			DEBUG("ace_pass(eval): state len = %d\n",
				(u_char)authState->strvalue[0]);
			return ace_eval(authInfo, sd);
		}
		else if( (u_char)(authState->strvalue[0])
			== (u_char)(LEN_SD_TOTAL(sd) + 1) )
		{
			DEBUG("ace_pass(next): state len = %u\n",
				(u_char)authState->strvalue[0]);
			return ace_next(authInfo, sd);
		}
		else {
			DEBUG("ace_pass(chall): state len = %u\n",
				(u_char)authState->strvalue[0]);
			return ACE_CHALLENGING;
		}
	}

	DEBUG("ace_pass: FAILED: no state attribute\n");
	return ACE_FAILED;
}

/* Scan recv_buffer far enough to determine whether or not this is an
   ACE authentication request.  If not, return -1.  If it's a new
   request, return 0.  If it's a NEXT PASSCODE request, return the pid
   of the radiusd child process that processed the initial
   authentication request.  The goal is to scan recv_buffer as quickly
   as possible to avoid burdening the parent process any more than
   necessary.  Heavyweight attribute/value parsing is left for the
   child process. */

static pid_t
is_ace_request (data, length)
	u_char		*data;
	int		length;
{
	u_char		*data_end = &data[length];
	int		auth_chap = 0;
	int		auth_pass = 0;
	int		immediate = 0;
	u_char		*user_name = NULL;
	u_char		*state = NULL;
	int		state_length = 0;
	int		result;
	struct SD_CLIENT *dummy_sd = 0;

	/* Parse only those few attributes we care about */
	while (data < data_end) {
		int attr = *data++;
		int size = *data++;
		if (size < 2)
			continue;
		size -= 2;
		switch (attr) {
		case PW_USER_NAME:
			user_name = data;
			break;
		case PW_STATE:
			state = data;
			state_length = size;
			break;
		case PW_PASSWORD:
			auth_pass++;
			break;
		case PW_CHAP_PASSWORD:
			auth_chap++;
			break;
		}
		data += size;
	}
	if (debug_flag) {
		if (!user_name)
			debugf("is_ace_request: no user name\n");
		if (!state)
			debugf("is_ace_request: no state\n");
		if (!(auth_pass || auth_chap))
			debugf("is_ace_request: not PAP or CHAP\n");
	}
	if (!user_name || !state || !(auth_pass || auth_chap)) {
		return -1;
	}
	result = user_wants_ace(user_name, &immediate);
	if (!result) {
		DEBUG("is_ace_request: user doesn't want ACE\n");
		return -1;
	}

	/* We now know that this request is destined for ACE.
	   Create a new child, or if this is a next-passcode
	   request, find an existing child to handle it.  */
	if ((state_length == LEN_ACE_DUMMY_STATE
		  && memcmp(state, ACE_DUMMY_STATE, LEN_ACE_DUMMY_STATE) == 0)
	    || (state_length == 0 && immediate)) {
		DEBUG("is_ace_request: initial request\n");
		return 0;
	} else if (state_length == LEN_SD_TOTAL(dummy_sd) + 1) {
		pid_t pid;
		memcpy(&pid, &state[state_length - sizeof(pid_t)], sizeof(pid_t));
		DEBUG("is_ace_request: next passcode request\n");
		return pid;
	}
	DEBUG("is_ace_request: failed: stlen=%d, immed=%d, dummylen=%d, sdlen=%d\n",
	      state_length, immediate, LEN_ACE_DUMMY_STATE,
	      LEN_SD_TOTAL(dummy_sd) + 1);
	return -1;
}

static AUTH_REQ *
dequeue_authreq_by_pid (pid)
	pid_t		pid;
{
	AUTH_REQ **req_ptr = &first_request;

	while (*req_ptr) {
		AUTH_REQ *authreq = *req_ptr;
		if (authreq->child_pid == pid) {
			return authreq;
		}
		*req_ptr = authreq->next;
	}
	log_err ("ACE child pid %ld not found!\n", pid);
	return NULL_REQ;
}

#endif	/* ACE */

/* printf-like interface to the debug trace.  */

void
#if __STDC__ == 1
debugf (CONST char *fmt, ...)
#else
debugf (va_alist) va_dcl
#endif
{
	va_list ap;
#if __STDC__ == 1
	va_start (ap, fmt);
#else
	CONST char *fmt;
	va_start (ap);
	fmt = va_arg(ap, char *);
#endif
	vdebugf (fmt, ap);
	va_end (ap);
}

/* vprintf-like interface to the debug trace.
   Prepends time stamp and pid to each message.  */

void
vdebugf(fmt, ap)
	CONST char *fmt;
	va_list ap;
{
	struct timeval tv;
	struct tm *tm;
	static char CONST *CONST month_names[] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};

#if defined(_SVID_GETTOD) || defined(aix)
	gettimeofday(&tv);
#else
	gettimeofday(&tv, 0);
#endif
	tm = localtime ((time_t *) &tv.tv_sec);
	printf ("%s %d %02d:%02d:%02d.%03d radiusd[%d] ",
		month_names[tm->tm_mon], tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec,
		tv.tv_usec / 1000, getpid());
	vprintf (fmt, ap);
	fflush (stdout);
}

static int
handle_radipa_response (sock, activefd)
	int sock;
	int activefd;
{
	char buf[RADIPA_BUFFER_SIZE];
	int bytes_received = recv (sock, buf, sizeof (buf), 0);
	if (bytes_received < 0) {
		log_err ("Can't recv from radipad socket: %s\n", xstrerror (errno));
		return FALSE;
	} else if (bytes_received == 0) {
		log_err ("EOF on radipad socket\n", xstrerror (errno));
		return FALSE;
	} else {
		struct radipa_packet *packet = radipa_packet_reorder_integers (buf);
		AUTH_REQ *authreq = (AUTH_REQ *) packet->rp_handle;
		forward_radipad_response (activefd, authreq, packet->rp_ip_address);
		return TRUE;
	}
}

static void
forward_radipad_response (sock, authreq, ip_address)
	int sock;
	AUTH_REQ *authreq;
	UINT4 ip_address;
{
	int length = AUTH_HDR_LEN;
	AUTH_HDR *auth = make_send_buffer (authreq, authreq->code);
	u_char *ptr = auth->data;

	if (authreq->code == PW_ASCEND_RADIPA_ALLOCATE) {
		int value_length = sizeof (UINT4) + 2;
		char ipaddr_string[32];
		ipaddr2str (ipaddr_string, ntohl (ip_address));
		DEBUG("Allocating global pool address %s for %s\n",
		      ipaddr_string, request_id (authreq));
		*ptr++ = PW_FRAMED_ADDRESS;
		*ptr++ = value_length;
		ip_address = htonl (ip_address);
		memcpy (ptr, &ip_address, sizeof(UINT4));
		length += value_length;
		ptr += value_length;
	} else {
		DEBUG("ACKing release of global pool address for %s\n",
		      request_id (authreq));
	}

	send_answer (sock, authreq, auth, length);
}

char *
get_user_name (authreq, where)
	AUTH_REQ *authreq;
	CONST char *where;
{
	VALUE_PAIR *request_pairs = authreq->request;

	while (request_pairs && request_pairs->attribute != PW_USER_NAME) {
		request_pairs = request_pairs->next;
	}
	if (request_pairs) {
		return request_pairs->strvalue;
	} else {
		log_err("%s: %s: No User name supplied\n",
			where, request_id (authreq));
		return (char *) NULL;
	}
}

char *
get_user_values (authreq, check_pairs, reply_pairs, where)
	AUTH_REQ *authreq;
	VALUE_PAIR **check_pairs;
	VALUE_PAIR **reply_pairs;
	CONST char *where;
{
	char *user_name = get_user_name (authreq, where);
	int error_code = user_find (user_name, check_pairs, reply_pairs);
	if (error_code) {
		log_err("%s: %s: user `%s': %s\n", where,
			request_id (authreq), user_name, get_errmsg (error_code));
		return (char *) NULL;
	}
	return user_name;
}

static int
maybe_init_radipa ()
{
	if (ipadfd >= 0) {
		return TRUE;
	}
	ipadfd = radipa_init ();
	if (ipadfd >= 0) {
		return TRUE;
	}
	return FALSE;
}

static void
allocate_ip_address_from_global_pool (authreq, activefd)
	AUTH_REQ *authreq;
	int activefd;
{
	struct address_chunk chunks_0[MAX_ADDRESS_CHUNKS];
	struct address_chunk *chunks = chunks_0;
	char *user_name = get_user_name (authreq, "Allocate IP Address");
	UINT4 router_ip_address = get_router_address (authreq, "Allocate IP Address");
	int count = radipa_parse_users_pool (user_name, &chunks);

	if (count == 0) {
		DEBUG("0 address chunks for %s\n", user_name);
		forward_radipad_response (activefd, authreq, 0L);
	} else if (maybe_init_radipa ()) {
		radipa_request_allocate_ip_address (ipadfd, authreq, router_ip_address,
							    chunks, count);
	} else {
		forward_radipad_response (activefd, authreq, 0L);
		dequeue_authreq (authreq);
		free_authreq (authreq);
	}
}

static void
release_ip_address_to_global_pool (authreq, activefd)
	AUTH_REQ *authreq;
	int activefd;
{
	UINT4 ip_address = get_framed_address (authreq, "Release IP Address");
	UINT4 router_ip_address = get_router_address (authreq, "Release IP Address");

	if (maybe_init_radipa ()) {
		radipa_request_release_ip_address (ipadfd, authreq, router_ip_address,
						   ip_address);
	} else {
		forward_radipad_response (activefd, authreq, 0L);
		dequeue_authreq (authreq);
		free_authreq (authreq);
	}
}

UINT4
get_router_address (authreq, where)
	AUTH_REQ *authreq;
	CONST char *where;
{
	VALUE_PAIR *request_pairs = authreq->request;

	while (request_pairs && request_pairs->attribute != PW_CLIENT_ID) {
		request_pairs = request_pairs->next;
	}
	if (request_pairs) {
		return htonl (request_pairs->lvalue);
	} else {
		log_err("%s: %s: No NAS-Identifier, using peer addr\n",
			where, request_id (authreq));
		return htonl (authreq->ipaddr);
	}
}

UINT4
get_framed_address (authreq, where)
	AUTH_REQ *authreq;
	CONST char *where;
{
	VALUE_PAIR *request_pairs = authreq->request;

	while (request_pairs && request_pairs->attribute != PW_FRAMED_ADDRESS) {
		request_pairs = request_pairs->next;
	}
	if (request_pairs) {
		return htonl (request_pairs->lvalue);
	} else {
		log_err("%s: %s: No Framed Address\n", where, request_id (authreq));
		return INADDR_NONE;
	}
}

struct sockaddr *
remote_sockaddr (authreq)
	AUTH_REQ *authreq;
{
	static struct sockaddr_in sins;

	memset ((char *) &sins, 0, sizeof (sins));
	sins.sin_family = AF_INET;
	sins.sin_addr.s_addr = htonl (authreq->ipaddr);
	sins.sin_port = htons (authreq->udp_port);

	return (struct sockaddr *) &sins;
}

static void
insert_response_md5_digest (auth, secret, length)
	AUTH_HDR *auth;
	CONST u_char *secret;
	int length;
{
	u_char digest[AUTH_VECTOR_LEN];
	int secret_length = strlen ((CONST char *) secret);
	char *tail = (char *) auth + length;

	auth->length = htons (length);
	memcpy (tail, secret, secret_length);
#if 0
	if (debug_flag) {
		u_char *buf = (u_char *) auth;
		u_char *end = &buf[length + secret_length];
		debugf("send_buffer before encrypt:\n");
		while (buf < end) {
			printf("%x ", *buf++);
		}
		putchar ('\n');
	}
#endif
	md5_calc (digest, (u_char *) auth, length + secret_length);
	memcpy (auth->vector, digest, AUTH_VECTOR_LEN);
	memset (tail, 0, secret_length);
}

int
append_user_message (destination, source, max_length)
	u_char *destination;
	CONST char *source;
	int max_length;
{
	int length = (source ? strlen (source) : 0);

	if (length && length <= max_length) {
		*destination++ = PW_PORT_MESSAGE;
		*destination++ = length + 2;
		memcpy (destination, source, length);
		length += 2;
	}
	return length;
}

AUTH_HDR *
make_send_buffer (authreq, code)
	AUTH_REQ *authreq;
	int code;
{
	AUTH_HDR *auth = (AUTH_HDR *) malloc (MAX_SNDBUF_SIZE);
	auth->code = code;
	auth->id = authreq->id;
	memcpy (auth->vector, authreq->vector, AUTH_VECTOR_LEN);
	return auth;
}

void
send_answer (fd, authreq, auth, length)
	int fd;
	AUTH_REQ *authreq;
	AUTH_HDR *auth;
	int length;
{
	insert_response_md5_digest (auth, authreq->secret, length);
	auth = (AUTH_HDR *) realloc (auth, length);
	authreq->answer = auth;
	authreq->answer_length = length;
	if (sendto (fd, (char *) auth, length, 0, remote_sockaddr (authreq),
		    sizeof(struct sockaddr_in)) < 0) {
		log_err ("Can't send response: %s\n", xstrerror (errno));
	}
}

static char *
request_id (authreq)
	AUTH_REQ *authreq;
{
	static char buf_0[256][2];
	static int slot = 0;
	char *buf = buf_0[slot = (slot + 1) % 2];

	sprintf (buf, "%.128s%d, id=%d", ip_hostname (authreq->ipaddr),
		 authreq->udp_port, authreq->id);
	return buf;
}

/*************************************************************************
 *
 *	Function: phone_cmp
 *
 *	Purpose: Compare a phone number attribute against a stored value
 *		 which may allow more than one possible match, using a 
 *		 particular syntax (delimiters mark alternate suffixes).
 *		 Return as though we were strcmp working on a normal string.
 *
 *************************************************************************/

static int
phone_cmp( CONST char *checkPhone, CONST char *authPhone )
{
    static char onePhone[ AUTH_STRING_LEN + 1 ];
    CONST char *checkPhoneStart = checkPhone;
    CONST char *checkPhoneDelim = NULL;
    int checkPhoneSegLen = 0;
    int compResult = -1;

    memset( onePhone, AUTH_STRING_LEN, '\0' );

    DEBUG("phone_cmp( \"%s\", \"%s\" )\n", checkPhone, authPhone );

	/*
	 * Hunt for delimiters, forward from the last-found delimiter.
	 * Look at the length of the string up to the delimiter, and
	 * replace that many characters of the existing onePhone.  Then
	 * see if the authPhone we were given compares equal to onePhone.
	 */
    for( checkPhoneStart = checkPhone;   *checkPhoneStart;
			 checkPhoneStart += checkPhoneSegLen ) {

	while( *checkPhoneStart == PW_PHONE_SUFFIX_DELIM ) {
	    checkPhoneStart++;
	}
	if( ! *checkPhoneStart ) {
		break;
	}
		
	    /*
	     * See if there's another segment after the one we're at.
	     * If so, the current segment ends with a delimiter;  if not,
	     * it ends at the end of the string.
	     */
	checkPhoneDelim = strchr( checkPhoneStart, PW_PHONE_SUFFIX_DELIM );
	checkPhoneSegLen = checkPhoneDelim ? checkPhoneDelim - checkPhoneStart :
					strlen( checkPhoneStart );

	if( checkPhoneSegLen ) {
	    int lenOnePhone;

		/*
		 * If the new segment is shorter than the current onePhone,
		 * replace the tail portion of onePhone.  Else, replace the
		 * entire onePhone.
		 */
	    lenOnePhone = strlen( onePhone );
	    if( checkPhoneSegLen <= lenOnePhone ) {
		strncpy( onePhone + lenOnePhone - checkPhoneSegLen,
			 checkPhoneStart, checkPhoneSegLen );
	    }
	    else if( checkPhoneSegLen < AUTH_STRING_LEN ) {
		strncpy( onePhone, checkPhoneStart, checkPhoneSegLen );
                onePhone[ checkPhoneSegLen ] = '\0';
            }

		/*
		 * Compare the calculated number with the one under test.
		 */
	    compResult = strcmp( onePhone, authPhone );
	    if( !compResult ) {
		break;
	    }
	}
    }
	/*
	 * Evidently we got out without matching anything.  Return the
	 * last (presumably unsuccessful) result from strcmp.
	 */
    return( compResult );
}
