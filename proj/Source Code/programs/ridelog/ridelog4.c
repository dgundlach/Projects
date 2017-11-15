#include <stdio.h>
#include <time.h>
#include <string.h>
#include <dbi/dbi.h>
                         
#define H1     "    Date     Odometer  Mileage  Weekly               Comments\n"
#define H2     "------------------------------------------------------------------------------\n"
#define X      " xxxx-xx-xx   xxxxx.x   xxx.x    xxx.x   12345678901234567890123456789012345678"
#define DETAIL " %s  %8.1f   %5.1f   %5.1f   %-37s\n"
#define TOTAL1 "                       ------\n"
#define TOTAL2 "         Monthly Total %6.1f           Yearly Total  %.1f\n\n"

#define START_QUERY "select max(odometer) as odometer from ridelog where ridedate < '%04i-01-01'"
#define YR_QUERY    "select * from ridelog where ridedate >= '%04i-01-01' and ridedate < '%04i-01-01' order by odometer"

void ErrorCallback(dbi_conn Conn, void *message) {

    const char *errormsg;

    dbi_conn_error(Conn, &errormsg);
    if (message) {
        printf("%s\n", (char *)message);
    }
    if (errormsg) {
        printf("%s\n", errormsg);
    }
    dbi_conn_close(Conn);
    dbi_shutdown();
    exit(1);
}

int main(int argc, char **argv) {

    dbi_conn Conn;
    dbi_result Result;
    time_t ridedate;
    double odometer;
    double mileage;
    double weekly;
    double monthly;
    double yearly;
    double starting = -1;
    const char *comments;
    int i;
    int nrows;
    int currmonth;
    int curryear;
    int weekoffset;
    int weekno;
    int newweekno;
    struct tm *t, mt;
    char datef[15];
    char query[256];

    dbi_initialize(NULL);
    Conn = dbi_conn_new("pgsql");
    dbi_conn_error_handler(Conn, &ErrorCallback, NULL);
    dbi_conn_set_option(Conn, "host", "localhost");
    dbi_conn_set_option(Conn, "username", "dan");
    dbi_conn_set_option(Conn, "password", "8giowqm3");
    dbi_conn_set_option(Conn, "dbname", "dan");
    dbi_conn_connect(Conn);

    curryear = 2005;
    bzero ((void *)&mt, sizeof(struct tm));
    mt.tm_year = curryear - 1900;
    mt.tm_mday = 1;
    ridedate = mktime(&mt);
    t = gmtime(&ridedate);
    weekoffset = t->tm_wday;
    weekno = 0;    

    snprintf(query, sizeof(query), START_QUERY, curryear);
    Result = dbi_conn_query(Conn, query);
    nrows = dbi_result_get_numrows(Result);
    if (!nrows) {
        printf("3\n");
        dbi_result_free(Result);
        dbi_conn_close(Conn);
        dbi_shutdown();
    }
    dbi_result_next_row(Result);
    starting = dbi_result_get_double(Result, "odometer");
    dbi_result_free(Result);
    printf("\n" H1 H2);
    yearly = 0;
    monthly = 0;
    snprintf(query, sizeof(query), YR_QUERY, curryear, curryear + 1);
    Result = dbi_conn_query(Conn, query);
    nrows = dbi_result_get_numrows(Result);
    currmonth = 0;
    if (nrows) {
        for (i=1; i<=nrows; i++) {
            dbi_result_next_row(Result);
            ridedate = dbi_result_get_datetime(Result, "ridedate");
            t = gmtime(&ridedate);
            newweekno = (t->tm_yday + weekoffset) / 7;
            if (t->tm_mon != currmonth) {
                currmonth = t->tm_mon;
                if (monthly > 0) {
                    printf(TOTAL1 TOTAL2, monthly, yearly);
                    monthly = 0;
                }
            }
            strftime(datef, sizeof(datef), "%F", t);
            odometer = dbi_result_get_double(Result, "odometer");
            mileage = odometer - starting;
            starting = odometer;            
            monthly += mileage;
            yearly += mileage;
            if (newweekno == weekno) {
                weekly += mileage;
            } else {
                weekly = mileage;
                weekno = newweekno;
            }
            comments = dbi_result_get_string(Result, "comments");
            printf(DETAIL, datef, odometer, mileage, weekly, comments);
        }
        if (monthly) {
            printf(TOTAL1 TOTAL2, monthly, yearly);
        }
    }
    dbi_result_free(Result);
    dbi_conn_close(Conn);
    dbi_shutdown();

    exit(0);
}
