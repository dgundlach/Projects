#include <syslog.h>
#include <stdarg.h>
#include <stdio.h>

/* log functions */
#if DEBUG>0
static void _nss_pgsql_log (int err, const char *format,...)
{
	va_list args;
	FILE *f;
        char buffer[500];

	va_start (args, format);
	openlog ("nss_pgsql", LOG_PID, LOG_AUTH);
	vsyslog (err, format, args);
/*
	f=fopen("/home/dan/log", "a");
        vsprintf (buffer, "%d ",err);
	vsprintf (buffer, format, args);
*/
	va_end (args);
	closelog ();
}
#endif
