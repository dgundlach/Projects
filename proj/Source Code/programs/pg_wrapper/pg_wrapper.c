#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libpq-fe.h>

typedef void * db_conn;
typedef void * db_result;
typedef void (*db_conn_error_handler)(db_conn, void *);

struct oi {
    int index;
    char *option;
} db_conn_option_keys[] = {
            {0, "connect_timeout"}, {1,  "dbname"},   {2, "host"},
            {3, "hostaddr"},        {4,  "password"}, {5, "port"},
            {6, "user"},            {7,  "options"},  {8, "requiressl"},
            {9, "service"},         {10, "sslmode"}, {11, "tty"}
};

#define num_options (sizeof(db_conn_option_keys)/sizeof(struct oi))

typedef struct db_conn_s {
    void *conn;
    char *options[num_options];
    db_conn_error_handler error_handler;
    void *error_handler_data;
} db_conn_t;

int compoi(const void *o1, const void *o2) {
    struct oi *oi1 = (struct oi *) o1;
    struct oi *oi2 = (struct oi *) o2;
    return strcmp(oi1->option, oi2->option);
}

db_conn db_conn_new(void) {

    db_conn nc;

    if (!(nc = malloc(sizeof(struct db_conn_s)))) {
        return NULL;
    }
    bzero(nc, sizeof(struct db_conn_s));
    return nc;
}






int db_conn_set_option(db_conn Conn, char *option, char *value)  {

    struct oi key, *result;

    key.option = option;
    if (!(result = bsearch(&key, db_conn_option_keys, num_options,
                               sizeof(struct oi), compoi))) {
        return 0;
    }
    ((db_conn_t *)Conn)->options[result->index] = value;
    return 1;
}









int main(int argc, char **argv) {

    printf("%i\n", strcmp("host", "hostaddr"));

    exit(0);
}
