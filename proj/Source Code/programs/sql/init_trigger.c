#include <postgres.h>
#include <executor/spi.h>
#include <commands/trigger.h>
#include "triggers.h"

int init_trigger(triginfo *ti, char *caller, FunctionCallInfo fcinfo,
		 int context) {

  char *format = "%s not designed for %s events.";
  TriggerEvent event;

  ti->trigdata = (TriggerData *) fcinfo->context;
  event = ti->trigdata->tg_event;
  
  if (!CALLED_AS_TRIGGER(fcinfo))
    elog(ERROR, "%s: not fired by trigger manager.", caller);
  if (TRIGGER_FIRED_FOR_STATEMENT(event) && !(context & TRIG_STATEMENT))
    elog(ERROR, format, caller, "STATEMENT");
  if (TRIGGER_FIRED_FOR_ROW(event) && !(context & TRIG_ROW))
    elog(ERROR, format, caller, "ROW");
  if (TRIGGER_FIRED_BEFORE(event) && !(context & TRIG_BEFORE))
    elog(ERROR, format, caller, "TRIGGER BEFORE");
  if (TRIGGER_FIRED_AFTER(event) && !(context & TRIG_AFTER))
    elog(ERROR, format, caller, "TRIGGER AFTER");
  if (TRIGGER_FIRED_BY_INSERT(event) && !(context & TRIG_INSERT))
    elog(ERROR, format, caller, "INSERT");
  if (TRIGGER_FIRED_BY_UPDATE(event) && !(context & TRIG_UPDATE))
    elog(ERROR, format, caller, "UPDATE");
  if (TRIGGER_FIRED_BY_DELETE(event) && !(context & TRIG_DELETE))
    elog(ERROR, format, caller, "DELETE");

  ti->rel      = ti->trigdata->tg_relation;
  ti->oldtuple = ti->trigdata->tg_trigtuple;
  ti->newtuple = ti->trigdata->tg_newtuple;
  ti->tupdesc  = ti->rel->rd_att;
  ti->trigger  = ti->trigdata->tg_trigger;
  return 0;
}
