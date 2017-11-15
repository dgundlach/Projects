#define TRIG_INSERT	1
#define TRIG_UPDATE	2
#define TRIG_DELETE	4
#define	TRIG_BEFORE	8
#define TRIG_AFTER	16
#define TRIG_ROW	32
#define TRIG_STATEMENT	64

struct triginfo {
  TriggerData *trigdata;
  Relation    rel;
  HeapTuple   oldtuple;
  HeapTuple   newtuple;
  TupleDesc   tupdesc;
  Trigger     *trigger;
};

typedef struct triginfo triginfo;

int init_trigger(triginfo *, char *, FunctionCallInfo, int);
