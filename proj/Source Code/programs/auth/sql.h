typedef struct {
  void *Result;
  int NFields;
  int NTuples;
  int NextTuple;
} SQLResult;

void SQLExitNicely(int, char *, ...);
int SQLConnect(char *);
int SQLDisconnect(int);
int SQLExec(int, char *); 
char **SQLGetNext(SQLResult *);
