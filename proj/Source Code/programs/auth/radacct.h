#define CONF_FILE		"/etc/raddb/raddetail"

#define WTMP_TABLE		"wtmp"
#define	WTMP_USERNAME		"username"
#define	WTMP_USERNAME_X		0
#define	WTMP_IPADDRESS		"ipaddress"
#define	WTMP_IPADDRESS_X	1
#define	WTMP_SESSIONID		"sessionid"
#define	WTMP_SESSIONID_X	2
#define	WTMP_SI			"si"
#define	WTMP_SI_X		3
#define	WTMP_PKTSSENT		"pktssent"
#define	WTMP_PKTSSENT_X		4
#define	WTMP_PKTSRECVD		"pktsrecvd"
#define	WTMP_PKTSRECVD_X	5
#define	WTMP_BYTESSENT		"bytessent"
#define	WTMP_BYTESSENT_X	6
#define	WTMP_BYTESRECVD		"bytesrecvd"
#define	WTMP_BYTESRECVD_X	7
#define	WTMP_XMITRATE		"xmitrate"
#define	WTMP_XMITRATE_X		8
#define	WTMP_DATARATE		"datarate"
#define	WTMP_DATARATE_X		9
#define	WTMP_PORTNO		"portno"
#define	WTMP_PORTNO_X		10
#define	WTMP_SLOTNO		"slotno"
#define	WTMP_SLOTNO_X		11
#define WTMP_TELEPHONE		"telephone"
#define WTMP_TELEPHONE_X	12
#define WTMP_STARTTIME		"starttime"
#define WTMP_STARTTIME_X	13
#define WTMP_SESSIONLEN		"sessionlen"
#define WTMP_SESSIONLEN_X	14



#define LASTLOG_TABLE		"lastlog"
#define LASTLOG_USERNAME	"username"
#define LASTLOG_USERNAME_X	0
#define LASTLOG_IPADDRESS	"ipaddress"
#define LASTLOG_IPADDRESS_X	1
/*
 * "login" is no longer used.
 */
#define LASTLOG_LOGIN		"login"
#define LASTLOG_LOGIN_X		2	
#define LASTLOG_SI		"si"
#define LASTLOG_SI_X		3
#define LASTLOG_STARTTIME	"starttime"
#define LASTLOG_STARTTIME_X	4



#define SERVERS_TABLE		"servers"
#define SERVERS_SI		"si"
#define SERVERS_SI_X		0
#define SERVERS_SERVERNAME	"servername"
#define SERVERS_SERVERNAME_X	1
#define SERVERS_IPADDRESS	"ipaddress"
#define SERVERS_IPADDRESS_X	2
#define SERVERS_MODEMCOUNT	"modemcount"
#define SERVERS_MODEMCOUNT_X	3
#define SERVERS_TELEPHONE	"telephone"
#define SERVERS_TELEPHONE_X     4


#define STATS_TABLE		"stats"
#define STATS_USERNAME		"username"
#define STATS_USERNAME_X	0
#define STATS_NSESSIONS		"nsessions"
#define STATS_NSESSIONS_X	1
#define STATS_TOTALTIME		"totaltime"
#define STATS_TOTALTIME_X	2
#define STATS_AVGTIME		"avgtime"
#define STATS_AVGTIME_X		3
#define STATS_MINTIME		"mintime"
#define STATS_MINTIME_X		4
#define STATS_MAXTIME		"maxtime"
#define STATS_MAXTIME_X		5
#define STATS_TOTALSENT		"totalsent"
#define STATS_TOTALSENT_X	6
#define STATS_AVGSENT		"avgsent"
#define STATS_AVGSENT_X		7
#define STATS_MINSENT		"minsent"
#define STATS_MINSENT_X		8
#define STATS_MAXSENT		"maxsent"
#define STATS_MAXSENT_X		9
#define STATS_TOTALRECVD	"totalrecvd"
#define STATS_TOTALRECVD_X	10
#define STATS_AVGRECVD		"avgrecvd"
#define STATS_AVGRECVD_X	11
#define STATS_MINRECVD		"minrecvd"
#define STATS_MINRECVD_X	12
#define STATS_MAXRECVD		"maxrecvd"
#define STATS_MAXRECVD_X	13
#define STATS_MINXMIT		"minxmit"
#define STATS_MINXMIT_X		14
#define STATS_MAXXMIT		"maxxmit"
#define STATS_MAXXMIT_X		15
#define STATS_AVGXMIT		"avgxmit"
#define STATS_AVGXMIT_X		16
#define STATS_MINDATA		"mindata"
#define STATS_MINDATA_X		17
#define STATS_MAXDATA		"maxdata"
#define STATS_MAXDATA_X		18
#define STATS_AVGDATA		"avgdata"
#define STATS_AVGDATA_X		19



#define NDETAILS 28

#define CONNECT_SCRIPT		"/etc/raddb/scripts/login"
#define DISCONNECT_SCRIPT	"/etc/raddb/scripts/logout"
#define FAIL_SCRIPT		"/etc/raddb/scripts/fail"

typedef struct detail_list {
  char *key;
  char *value;
  char *env;
  char *dbfieldname;
  int dbfieldposition;
} detail_list;

int compare_details(const void *,const void *);
detail_list *build_detail_list(void);
detail_list *update_detail(detail_list *,char *,char *);
#define lookup_detail(key,det) \
	bsearch(key,det,NDETAILS,sizeof(struct detail_list),compare_details)
void set_detail_environ(detail_list *);
char *retrieve_detail(detail_list *, char *);
