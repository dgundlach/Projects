#include <stdio.h>
#include <time.h>
#include <string.h>
#include <getopt.h>
#include <dbi/dbi.h>
#define H0     "                            R I D E L O G   %c %c %c %c\n"                         
#define H1     " Date    Wk  Odometer  Mileage  Weekly               Comments\n"
#define H2     "------------------------------------------------------------------------------\n"
#define X      " xxxx-xx-xx   xxxxx.x   xxx.x    xxx.x   12345678901234567890123456789012345678"
#define Y      " xx/xx   xx   xxxxx.x" 



#define DETAIL " %02i/%02i   %2i  %8.1f   %5.1f   %5.1f   %-37s\n"
#define TOTAL1 "                       ------\n"
#define TOTAL2 "         Monthly Total %6.1f (%i)"
#define TOTAL3 "      Yearly Total  %.1f (%i)\n\n"

#define START_QUERY "select * from ridelog where ridedate < '%04i-01-01' order by odometer desc limit 1"
#define YR_QUERY    "select * from ridelog where ridedate >= '%04i-01-01' and ridedate < '%04i-01-01' order by odometer"
#define DEL_QUERY   "delete from ridelog where odometer = %s"
#define INS_QUERY   "insert into ridelog (odometer"

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

void usage(char *prog) {

    char *bn;

    if ((bn = strrchr(prog, '/'))) {
        bn++;
    } else {
        bn = prog;
    }
    printf("Usage: %s [-c comments] [-d date] odometer\n", bn);
    printf("       %s -r odometer\n", bn);
    printf("       %s [-y year]\n", bn);
}

int main(int argc, char **argv) {

    dbi_conn Conn;
    dbi_result Result;
    time_t ridedate;
    double odometer, mileage, weekly, monthly, yearly, starting = -1;
    const char *comments;
    int i, nrows, currmonth, curryear, weekoffset, weekno, newweekno, delete;
    int need_odo, mon_out, yr_out;
    struct tm t;
    char query[512];
    char pnum[20];
    char c;

    char *odo_in, *date_in, *comments_in, *year_in;

    odo_in = NULL;
    date_in = NULL;
    comments_in = NULL;
    year_in = NULL;
    delete = 0;
    need_odo = 0;



    ridedate = time(NULL);
    gmtime_r(&ridedate, &t);
    curryear = t.tm_year + 1900;
    while (1) {
        if ((c = getopt(argc, argv, "c:d:ry:")) == -1) {
            break;
        }
        switch(c) {
        case 'c':
            if (delete || year_in) {
                usage(argv[0]);
                exit(1);
            }
            need_odo = 1;
	    comments_in = optarg;
            break;
        case 'd':
            if (delete || year_in) {
                usage(argv[0]);
                exit(1);
            }
            need_odo = 1;
            date_in = optarg;
            break;
        case 'r':
            if (date_in || comments_in || year_in) {
                usage(argv[0]);
                exit(1);
            }
            need_odo = 1;
            delete = 1;
            break;
        case 'y':
            if (need_odo) {
                usage(argv[0]);
                exit(1);
            }
            year_in = optarg;
            curryear = atoi(optarg);
            break;
        default:        
            usage(argv[0]);
            exit(1);
        }
    }
    odo_in = argv[optind];
    if (need_odo && !odo_in) {
        usage(argv[0]);
        exit(1);
    }

    dbi_initialize(NULL);
    Conn = dbi_conn_new("pgsql");
    dbi_conn_error_handler(Conn, &ErrorCallback, NULL);
    dbi_conn_set_option(Conn, "host", "localhost");
    dbi_conn_set_option(Conn, "username", "dan");
    dbi_conn_set_option(Conn, "password", "8giowqm3");
    dbi_conn_set_option(Conn, "dbname", "dan");
    dbi_conn_connect(Conn);

    if (odo_in) {
        if (delete) {
            snprintf(query, sizeof(query), DEL_QUERY, odo_in);
        } else {
            i = snprintf(query, sizeof(query), INS_QUERY);
            if (comments_in) {
                i += snprintf(query + i, sizeof(query) + i, ",comments");
            }
            if (date_in) {
                i += snprintf(query + i, sizeof(query) + i, ",ridedate");
            }
            i += snprintf(query +i, sizeof(query) + i, ") values (%s", odo_in);
            if (comments_in) {
                i += snprintf(query + i, sizeof(query) + i, ",'%s'", comments_in);
            }
            if (date_in) {
                i += snprintf(query + i, sizeof(query) + i, ",'%s'", date_in);
            }
            i += snprintf(query + i, sizeof(query) + i, ")");
        }   
        Result = dbi_conn_query(Conn, query);
        dbi_result_free(Result);
    }
    bzero ((void *)&t, sizeof(struct tm));
    t.tm_year = curryear - 1900;
    t.tm_mday = 1;
    ridedate = mktime(&t);
    gmtime_r(&ridedate, &t);
    weekoffset = t.tm_wday;
    weekno = 0;    
        snprintf(query, sizeof(query), START_QUERY, curryear);
    Result = dbi_conn_query(Conn, query);
    nrows = dbi_result_get_numrows(Result);
    if (!nrows) {
        dbi_result_free(Result);
        dbi_conn_close(Conn);
        dbi_shutdown();
        exit(0);
    }
    dbi_result_next_row(Result);
    starting = dbi_result_get_double(Result, "odometer");
    dbi_result_free(Result);
    snprintf(query, sizeof(query), YR_QUERY, curryear, curryear + 1);
    Result = dbi_conn_query(Conn, query);
    nrows = dbi_result_get_numrows(Result);
    currmonth = 0;
    if (nrows) {
        snprintf(pnum, sizeof(pnum), "%i", curryear);
        printf("\n" H0 H1 H2, pnum[0], pnum[1], pnum[2], pnum[3]);
        yearly = 0;
        monthly = 0;
        mon_out = 0;
        yr_out = 0;
        for (i=1; i<=nrows; i++) {
            dbi_result_next_row(Result);

            comments = dbi_result_get_string(Result, "comments");

            ridedate = dbi_result_get_datetime(Result, "ridedate");

            gmtime_r(&ridedate, &t);
            if (t.tm_mon != currmonth) {
                currmonth = t.tm_mon;
                if (monthly > 0) {
                    printf(TOTAL1 TOTAL2, monthly, mon_out);
                    if (mon_out < 10) {
                        printf(" ");
                    }
                    printf(TOTAL3, yearly, yr_out);
                    monthly = 0;
                    mon_out = 0;
                }
            }

            odometer = dbi_result_get_double(Result, "odometer");
            mileage = odometer - starting;
            starting = odometer;            
            monthly += mileage;
            yearly += mileage;
            mon_out++;
            yr_out++;
            newweekno = (t.tm_yday + weekoffset) / 7;
            if (newweekno == weekno) {
                weekly += mileage;
            } else {
                weekly = mileage;
                weekno = newweekno;
            }

            printf(DETAIL, currmonth + 1, t.tm_mday, weekno + 1, odometer, 
                                                mileage, weekly, comments);
        }
        if (monthly) {
            printf(TOTAL1 TOTAL2, monthly, mon_out);
            if (mon_out < 10) {
                printf(" ");
            }
            printf(TOTAL3, yearly, yr_out);
        }
    }
    dbi_result_free(Result);
    dbi_conn_close(Conn);
    dbi_shutdown();

    exit(0);
}
