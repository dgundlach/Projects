#include <stdio.h>
#include <string.h>
#include <isql.h>
#include <isqlext.h>

#define DSNSIZE		256

SQLHENV henv;
SQLHDBC hdbc;
SQLHSTMT hstmt;
int connected;

int DB_Connect (char *DataSetName, char *host, char *database, char *uid,
		char *pwd, char *readonly, char *servertype, int fetchbuffersz,
		char *options)
{
  short bufflen;
  char buf[257];
  SQLCHAR *dataSource = NULL;
  SQLCHAR *sptr;
  int p;
  SQLCHAR dsn[33];
  SQLCHAR desc[255];
  SWORD len1, len2;
  int status;

#if (ODBCVER < 0x300)
  if (SQLAllocEnv (&henv) != SQL_SUCCESS)
    return -1;

  if (SQLAllocConnect (henv, &hdbc) != SQL_SUCCESS)
    return -1;
#else
  if (SQLAllocHandle (SQL_HANDLE_ENV, NULL, &henv) != SQL_SUCCESS)
    return -1;

  SQLSetEnvAttr (henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER) SQL_OV_ODBC3,
		SQL_IS_UINTEGER);

  if (SQLAllocHandle (SQL_HANDLE_DBC, henv, &hdbc) != SQL_SUCCESS)
    return -1;
#endif
  
  
  dataSource = malloc((strlen(DataSetName) + 5) 
			+ (host ? strlen(host) + 6 : 0)
			+ (database ? strlen(database) + 10 : 0)
			+ (uid ? strlen(uid) + 5 : 0)
			+ (pwd ? strlen(pwd) + 5 : 0)
			+ (readonly ? strlen(readonly) + 10 : 0)
			+ (servertype ? strlen(servertype) + 12 : 0)
			+ (fetchbuffersz ? 12 : 0)
			+ (options ? strlen(options) + 9));

  sptr = dataSource;
  sptr += sprintf(sptr, "DSN=%s", DataSetName);
  if (host)          sptr += sprintf(sptr, ";HOST=%s", host);
  if (database)      sptr += sprintf(sptr, ";DATABASE=%s", database);
  if (uid)           sptr += sprintf(sptr, ";UID=%s", uid);
  if (pwd)           sptr += sprintf(sptr, ";PWD=%s", pwd);
  if (readonly)      sptr += sprintf(sptr, ";READONLY=%s", readonly);
  if (servertype)    sptr += sprintf(sptr, ";SVT=%s", servertype);
  if (fetchbuffersz) sptr += sprintf(sptr, ";FBS=%d", fetchbuffersz);
  if (options)       sptr += sprintf(sptr, ";OPTIONS=%s", options);

  status = SQLDriverConnect (hdbc, 0, (UCHAR *) dataSource, SQL_NTS,
	(UCHAR *) buf, sizeof(buf), &buflen, SQL_DRIVER_COMPLETE);

  if (status != SQL_SUCCESS && status != SQL_SUCCESS_WITH_INFO)
    return -1;

#if (ODBCVER < 0x0300)
  if (SQLAllocStmt (hdbc, &hstmt) != SQL_SUCCESS)
    return -1;
#else
  if (SQLAllocHandle (SQL_HANDLE_STMT, hdbc, &hstmt) != SQL_SUCCESS)
    return -1;
#endif

  return 0;
}

int DB_Disconnect (void)
{
#if (ODBCVER < 0x0300)
  if (hstmt)
    SQLFreeStmt (hstmt, SQL_DROP);

  if (connected)
    SQLDisconnect (hdbc);

  if (hdbc)
    SQLFreeConnect (hdbc);

  if (henv)
    SQLFreeEnv (henv);
#else
  if (hstmt)
    {
       int sts;
       sts = SQLCloseCursor (hstmt);
       if (sts != SQL_ERROR)
           DB_Errors ("CloseCursor");
       SQLFreeHandle (SQL_HANDLE_STMT, hstmt);   
    }

  if (connected)
    SQLDisconnect (hdbc);

  if (hdbc)
    SQLFreeHandle (SQL_HANDLE_DBC, hdbc);
 
  if (henv)  
    SQLFreeHandle (SQL_HANDLE_ENV, henv);
#endif

  return 0;
}  

void DB_MesgHandler (char *reason)
{
  fprintf (stderr, "DB_MesgHandler: %s\n", reason);
}

int DB_Errors (char *where)
{
  unsigned char buf[250];
  unsigned char sqlstate[15];
    
  /*
   *  Get statement errors
   */
  while (SQLError (henv, hdbc, hstmt, sqlstate, NULL,
      buf, sizeof(buf), NULL) == SQL_SUCCESS)
    {
      fprintf (stderr, "%s, SQLSTATE=%s\n", buf, sqlstate);
    }
 
  /*
   *  Get connection errors
   */
  while (SQLError (henv, hdbc, SQL_NULL_HSTMT, sqlstate, NULL, 
      buf, sizeof(buf), NULL) == SQL_SUCCESS)
    {
      fprintf (stderr, "%s, SQLSTATE=%s\n", buf, sqlstate);
    }
  
  /*
   *  Get environmental errors
   */
  while (SQLError (henv, SQL_NULL_HDBC, SQL_NULL_HSTMT, sqlstate, NULL,
      buf, sizeof(buf), NULL) == SQL_SUCCESS)
    {
      fprintf (stderr, "%s, SQLSTATE=%s\n", buf, sqlstate);
    }  
       
  return -1;
}
