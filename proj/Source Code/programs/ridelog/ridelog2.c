#include <stdio.h>
#include <time.h>
#include <string.h>
#include <dbi/dbi.h>
                         
#define H1     " Ride Date   Odometer  Mileage                   Comments\n"
#define H2     "------------------------------------------------------------------------------\n"
#define X      " xxxx-xx-xx  xxxx    xxxxxx   "
#define DETAIL " %s   %7.1f    %4.1f   %-45s\n"
#define TOTAL1 "                        -----\n"
#define TOTAL2 "          Weekly Total %6.1f   Yearly Total  %.1f\n\n"

#define H3     "            Monthly    Yearly\n"
#define H4     "   Month     Total     Total\n"
#define H5     "------------------------------\n"
#define D2     " %-9s   %6.1f   %7.1f\n"



#define START_QUERY "select max(odometer) as odometer from ridelog where ridedate < '%04i-01-01'"
#define MON_QUERY   "select * from ridelog where ridedate >= '%04i-%02i-%02i' and ridedate < '%04i-%02i-%02i' order by odometer"

void ErrorCallback(dbi_conn Conn, void *message) {

    const char *errormsg;

    dbi_conn_error(Conn, &errormsg);
    if (message) {
        printf("%s\n", message);
    }
    if (errormsg) {
        printf("%s\n", errormsg);
    }
    dbi_conn_close(Conn);
    dbi_shutdown();
    exit(1);
}

char *monthnames[] = {
                "January",
                "February",
                "March",
                "April",
                "May",
                "June",
                "July",
                "August",
                "September",
                "October",
                "November",
                "December"
};

int main(int argc, char **argv) {

    dbi_conn Conn;
    dbi_result Result;
    time_t ridedate;
    double odometer;
    double mileage;
    double weekly;
    double yearly;
    double starting = -1;
    const char *comments;
    char *datatypes[] = {"NULL", "int", "decimal", "string",
                         "binary", "enum", "set", "datetime"};
    int nfields;
    int i;
    int nrows;
    int nextday;
    int nextmonth;
    int curryear;
    int nextyear;
    int founddata = 0;
    double monthtotals[] = {0,0,0,0,0,0,0,0,0,0,0,0};

    const char *fieldname;
    unsigned short fieldtype;
    struct tm *t, *c, *n, mt;
    time_t weekstart;
    time_t nextweek;
    char datef[15];
    char *ridedate_s;
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
    nextyear = curryear;
    nextmonth = 1;
    bzero ((void *)&mt, sizeof(struct tm));
    mt.tm_year = curryear - 1900;
    mt.tm_mon = 0;
    mt.tm_mday = 1;
    weekstart = mktime(&mt);
    t = gmtime(&weekstart);
    nextweek = weekstart + (86400 * ((7 - t->tm_wday)));

    while (nextyear == curryear) {
        weekly = 0;
        n = gmtime(&nextweek);
        nextday = n->tm_mday;
        nextmonth = n->tm_mon + 1;
        nextyear = n->tm_year + 1900;
        if (nextyear != curryear) {
            nextday = 1;
        }
        c = gmtime(&weekstart);
        weekstart = nextweek;
        nextweek += 86400 * 7;
        snprintf(query, sizeof(query), MON_QUERY, c->tm_year + 1900, c->tm_mon + 1, c->tm_mday,
                                                  nextyear, nextmonth, nextday);
        Result = dbi_conn_query(Conn, query);
        nrows = dbi_result_get_numrows(Result);
        if (nrows) {
            founddata = 1;
            for (i=1; i<=nrows; i++) {
                dbi_result_next_row(Result);
                ridedate = dbi_result_get_datetime(Result, "ridedate");
                t = gmtime(&ridedate);
                strftime(datef, sizeof(datef), "%F", t);
                odometer = dbi_result_get_double(Result, "odometer");
                mileage = odometer - starting;
                starting = odometer;            
                weekly += mileage;
                yearly += mileage;
                monthtotals[t->tm_mon] += mileage;
                comments = dbi_result_get_string(Result, "comments");
                printf(DETAIL, datef, odometer, mileage, comments);
            }
            printf(TOTAL1 TOTAL2, weekly, yearly);
        }
        dbi_result_free(Result);
    }
    dbi_conn_close(Conn);
    dbi_shutdown();
    if (founddata) {
        printf(H3 H4 H5);
        yearly = 0;
        for (i = 0; i < 12; i++) {
            if (monthtotals[i]) {
                yearly += monthtotals[i];
                printf(D2, monthnames[i], monthtotals[i], yearly);
            }
        }
        printf("\n");
    }





    exit(0);
}
