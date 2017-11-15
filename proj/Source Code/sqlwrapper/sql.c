#include <stdlib.h>
#include <glib.h>
#include <libpq-fe.h>

#include "coltypes.h"

typedef struct SQLColumn_t {
    char *str;
    unsigned int len;
    unsigned int datatype;
    int binary;
    void **binding;
} SQLColumn;

typedef struct SQLConn_t {
    void *c;
    GHashTable *parameters;
    void (*errorhandler)(char *, void *);
    void *errorarg;
} SQLConn;

typedef struct SQLResult_t {
    void *r;
    unsigned int nrows;
    unsigned int currrow;
    unsigned int ncolumns;
    SQLColumn **bindings;
} SQLResult;

SQLConn *SQLConnNew(void) {

    SQLConn *Conn;

    Conn = malloc(sizeof(SQLConn));
    Conn->c = NULL;
    Conn->parameters = g_hash_table_new(g_str_hash, g_str_equal);
    Conn->errorhandler = NULL;
    return Conn;
}

void SQLConnSetParameter(SQLConn *Conn, char *Parameter, char *Value) {

    g_hash_table_insert(Conn->parameters, Parameter, Value);
}

char *SQLConnGetParameter(SQLConn *Conn, char *Parameter) {

    return g_hash_table_lookup(Conn->parameters, Parameter);
}

void SQLConnSetErrorHandler(SQLConn *Conn, void (*errorhandler)(char *, void *), void *errorarg) {

    Conn->errorhandler = errorhandler;
    Conn->errorarg = errorarg;
}

void FormatPairs(gpointer key, gpointer value, gpointer str) {

    g_string_append_printf((GString *)str, " %s=%s", (char *)key, (char *)value);
}

int SQLConnOpen(SQLConn *Conn) {

    GString *str;

    str = g_string_new_len(NULL, 256);
    g_hash_table_foreach(Conn->parameters, FormatPairs, str);
    Conn->c = PQconnectdb(str->str);
    g_string_free(str, TRUE);
    if (PQstatus((PGconn *)(Conn->c)) != CONNECTION_OK) {
        if (Conn->errorhandler) {
            Conn->errorhandler(PQerrorMessage((PGconn *)(Conn->c)), Conn->errorarg);
        }
        return FALSE;
    }
    return TRUE;
}

void SQLConnClose(SQLConn *Conn) {

    if (Conn->c) {
        PQfinish((PGconn *)(Conn->c));
        Conn->c = NULL;
    }
}

SQLConn *SQLConnDestroy(SQLConn *Conn) {

    SQLConnClose(Conn);
    g_hash_table_destroy(Conn->parameters);
    free(Conn);
    return NULL;    
}

SQLResult *SQLExec(SQLConn *Conn, char *Statement) {

    SQLResult *Result;
    int status;

    Result = calloc(1, sizeof(SQLResult));
    Result->r = PQexec((PGconn *)(Conn->c), Statement);
    status = PQresultStatus((PGresult *)(Result->r));
    if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK) {
        PQclear((PGresult *)(Result->r));
        free(Result);
        if (Conn->errorhandler) {
            Conn->errorhandler(PQerrorMessage((PGconn *)(Conn->c)), Conn->errorarg);
        }
        return NULL;
    }
    if (status == PGRES_TUPLES_OK) {
        Result->nrows = PQntuples((PGresult *)Result->r);
        Result->ncolumns = PQnfields((PGresult *)Result->r);
        Result->bindings = calloc(Result->ncolumns, sizeof(void *));
        Result->currrow = 0;
    }
    return Result;
}

SQLResult *SQLResultFree(SQLResult *Result) {

    if (Result) {
        if (PQresultStatus((PGresult *)(Result->r)) == PGRES_TUPLES_OK) {
            free(Result->bindings);
        }
        PQclear((PGresult *)(Result->r));
        free(Result);
    }
    return NULL;
}

int SQLResultBindColumn(SQLResult *Result, const char *Column, SQLColumn *Var) {

    int index;

    if ((index = PQfnumber((PGresult *)(Result->r), Column)) == -1) {
        return FALSE;
    }
    Result->bindings[index] = Var;
    return TRUE;
}

int SQLResultSeekRow(SQLResult *Result, unsigned int Row) {

    if (!Row || Result->nrows < Row) {
        return 0;
    }
    Result->currrow = Row;
    return Row;
}

int SQLResultFirstRow(SQLResult *Result) {

    if (!Result->nrows) {
        return 0;
    }
    Result->currrow = 1;
    return 1;
}

int SQLResultLastRow(SQLResult *Result) {

    if (!Result->nrows) {
        return 0;
    }
    Result->currrow = Result->nrows;
    return Result->nrows;
}

int SQLResultPrevRow(SQLResult *Result) {

    if (!Result->nrows || (Result->currrow < 2)) {
        return 0;
    }
    Result->currrow--;
    return Result->currrow;
}

int SQLResultNextRow(SQLResult *Result) {

    if (!Result->nrows || (Result->currrow == Result->nrows)) {
        return 0;
    }
    Result->currrow++;
    return Result->currrow;
}

int SQLResultFetchRow(SQLResult *Result) {

    unsigned int i;

    if (!Result->currrow || !Result->nrows) {
        return FALSE;
    }
    for (i=0; i<Result->ncolumns; i++) {
        if (Result->bindings[i]) {
            Result->bindings[i]->str = PQgetvalue((PGresult *)(Result->r),
                                                  Result->currrow - 1, i);
            Result->bindings[i]->len = PQgetlength((PGresult *)(Result->r),
                                                   Result->currrow - 1, i);
            Result->bindings[i]->datatype = PQftype((PGresult *)(Result->r), i);
            Result->bindings[i]->binary = PQfformat((PGresult *)(Result->r), i);
        }
    }
    return TRUE;
}
