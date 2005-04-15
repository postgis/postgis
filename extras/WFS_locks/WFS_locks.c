#include "postgres.h"
#include "executor/spi.h"       /* this is what you need to work with SPI */
#include "commands/trigger.h"   /* ... and triggers */
#include "utils/lsyscache.h"	/* for get_namespace_name() */

//#define PGIS_DEBUG 1

Datum check_authorization(PG_FUNCTION_ARGS);

/*
 * This trigger will check for authorization before
 * allowing UPDATE or DELETE of specific rows.
 * Rows are identified by the provided column.
 * Authorization info is extracted by the
 * "authorization_table"
 *
 */
PG_FUNCTION_INFO_V1(check_authorization);
Datum check_authorization(PG_FUNCTION_ARGS)
{
	TriggerData *trigdata = (TriggerData *) fcinfo->context;
	char *colname;
	HeapTuple rettuple;
	TupleDesc tupdesc;
	int SPIcode;
	char query[1024];
	const char *pk_id = NULL;
	SPITupleTable *tuptable;
	HeapTuple tuple;
	char *lockcode;
	char *authtable = "authorization_table";


	/* Make sure trigdata is pointing at what I expect */
	if (!CALLED_AS_TRIGGER(fcinfo))
		elog(ERROR,"check_authorization: not fired by trigger manager");

	if (! (TRIGGER_FIRED_BY_UPDATE(trigdata->tg_event) ||
		TRIGGER_FIRED_BY_DELETE(trigdata->tg_event)) )
		elog(ERROR,"check_authorization: not fired by update or delete");

	rettuple = trigdata->tg_newtuple;
	tupdesc = trigdata->tg_relation->rd_att;

	/* Connect to SPI manager */
	SPIcode = SPI_connect();

	if (SPIcode  != SPI_OK_CONNECT)
	{
		elog(ERROR,"check_authorization: could not connect to SPI");
		PG_RETURN_NULL() ;
	}

	colname  = trigdata->tg_trigger->tgargs[0];
	pk_id = SPI_getvalue(trigdata->tg_trigtuple, tupdesc,
		SPI_fnumber(tupdesc, colname));

#if PGIS_DEBUG
	elog(NOTICE,"check_authorization(%s.%s.%s=%s)",
		nspname, relname, colname, pk_id);
#endif

	sprintf(query,"SELECT authid FROM \"%s\" WHERE expires >= now() AND toid = '%d' AND rid = '%s'", authtable, trigdata->tg_relation->rd_id, pk_id);

#if PGIS_DEBUG > 1
	elog(NOTICE,"about to execute :%s", query);
#endif

	SPIcode = SPI_exec(query,0);
	if (SPIcode !=SPI_OK_SELECT )
		elog(ERROR,"couldnt execute to test for lock :%s",query);

	if (!SPI_processed )
	{
#if PGIS_DEBUG
		elog(NOTICE,"there is NOT a lock on row '%s'", pk_id);
#endif
		SPI_finish();
		return PointerGetDatum(trigdata->tg_trigtuple);
	}

	// there is a lock - check to see if I have rights to it!

	tuptable = SPI_tuptable;
	tupdesc = SPI_tuptable->tupdesc;
	tuple = tuptable->vals[0];
	lockcode = SPI_getvalue(tuple, tupdesc, 1);

#if PGIS_DEBUG
	elog(NOTICE,"there is a lock on this row!");
#endif

	// check to see if temp_lock_have_table table exists
	// (it might not exist if they own no locks
	sprintf(query,"SELECT * FROM pg_class WHERE relname = 'temp_lock_have_table'");
	SPIcode = SPI_exec(query,0);
	if (SPIcode != SPI_OK_SELECT )
		elog(ERROR,"couldnt execute to test for lockkey temp table :%s",query);
	if (SPI_processed==0)
	{
		SPI_finish();

		elog(ERROR,"Unauthorized modification (requires auth: '%s')",
			lockcode);
		elog(NOTICE,"Modificaton of row '%s' requires authorization '%s'",
			pk_id, lockcode);
		// ignore requested delete or update
		return PointerGetDatum(trigdata->tg_trigtuple);
	}

	sprintf(query, "SELECT * FROM temp_lock_have_table WHERE lockcode ='%s'",lockcode);

#if PGIS_DEBUG
	elog(NOTICE,"about to execute :%s", query);
#endif

	SPIcode = SPI_exec(query,0);
	if (SPIcode != SPI_OK_SELECT )
		elog(ERROR,"couldnt execute to test for lock aquire: %s", query);

	if (SPI_processed >0)
	{
#if PGIS_DEBUG
		elog(NOTICE,"I own the lock - I can modify the row");
#endif
		SPI_finish();
		return PointerGetDatum(rettuple);
	}

	SPI_finish();

	elog(ERROR,"Unauthorized modification (requires auth: '%s')",
		lockcode);
	elog(NOTICE,"Modificaton of row '%s' requires authorization '%s'",
		pk_id, lockcode);
	// ignore the command
	return PointerGetDatum(trigdata->tg_trigtuple);

}
