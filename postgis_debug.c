
/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2003 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of hte GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************
 * $Log$
 * Revision 1.13  2003/11/19 18:01:31  strk
 * CR removed
 *
 * Revision 1.12  2003/09/16 20:27:12  dblasby
 * added ability to delete geometries.
 *
 * Revision 1.11  2003/08/08 18:19:20  dblasby
 * Conformance changes.
 * Removed junk from postgis_debug.c and added the first run of the long
 * transaction locking support.  (this will change - dont use it)
 * conformance tests were corrected
 * some dos cr/lf removed
 * empty geometries i.e. GEOMETRYCOLLECT(EMPTY) added (with indexing support)
 * pointN(<linestring>,1) now returns the first point (used to return 2nd)
 *
 * Revision 1.10  2003/07/25 17:08:37  pramsey
 * Moved Cygwin endian define out of source files into postgis.h common
 * header file.
 *
 * Revision 1.9  2003/07/01 18:30:55  pramsey
 * Added CVS revision headers.
 *
 *
 **********************************************************************/

#include "postgres.h"


#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "access/gist.h"
#include "access/itup.h"
#include "access/rtree.h"


#include "fmgr.h"


#include "postgis.h"
#include "utils/elog.h"

#include "executor/spi.h"
#include "commands/trigger.h"


#define SHOW_DIGS_DOUBLE 15
#define MAX_DIGS_DOUBLE (SHOW_DIGS_DOUBLE + 6 + 1 + 3 +1)


// #define DEBUG_GIST
//#define DEBUG_GIST2





//find the size of geometry
PG_FUNCTION_INFO_V1(mem_size);
Datum mem_size(PG_FUNCTION_ARGS)
{
	GEOMETRY		      *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	PG_RETURN_INT32(geom1->size);
}



//get summary info on a GEOMETRY
PG_FUNCTION_INFO_V1(summary);
Datum summary(PG_FUNCTION_ARGS)
{
	GEOMETRY		      *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	int32				*offsets1;
	char				*o1;
	int32				type1,j,i;
	POLYGON3D			*poly;
	LINE3D			*line;
	char				*result;
	int				size;
	char				tmp[100];
	text				*mytext;


	size = 1;
	result = palloc(1);
	result[0] = 0; //null terminate it


	offsets1 = (int32 *) ( ((char *) &(geom1->objType[0] ))+ sizeof(int32) * geom1->nobjs ) ;

	//now have to do a scan of each object

	for (j=0; j< geom1->nobjs; j++)		//for each object in geom1
	{
		o1 = (char *) geom1 +offsets1[j] ;
		type1=  geom1->objType[j];

		if (type1 == POINTTYPE)	//point
		{
			size += 30;
			result = repalloc(result,size);
			sprintf(tmp,"Object %i is a POINT()\n",j);
			strcat(result,tmp);
		}

		if (type1 == LINETYPE)	//line
		{
			line = (LINE3D *) o1;

			size += 57;
			result = repalloc(result,size);
			sprintf(tmp,"Object %i is a LINESTRING() with %i points\n",j,line->npoints);
			strcat(result,tmp);
		}

		if (type1 == POLYGONTYPE)	//POLYGON
		{
			poly = (POLYGON3D *) o1;

			size += 57*(poly->nrings +1);
			result = repalloc(result,size);
			sprintf(tmp,"Object %i is a POLYGON() with %i rings\n",j,poly->nrings);
			strcat(result,tmp);
			for (i=0; i<poly->nrings;i++)
			{
				sprintf(tmp,"     + ring %i has %i points\n",i,poly->npoints[i]);
				strcat(result,tmp);
			}
		}
	}

		// create a text obj to return
	mytext = (text *) palloc(VARHDRSZ  + strlen(result) );
	VARATT_SIZEP(mytext) = VARHDRSZ + strlen(result) ;
	memcpy(VARDATA(mytext) , result, strlen(result) );
	pfree(result);
	PG_RETURN_POINTER(mytext);
}





extern Datum lockcheck(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(lockcheck);

Datum lockcheck (PG_FUNCTION_ARGS)
{
	TriggerData *trigdata =      (TriggerData *) fcinfo->context;
	char		*colname ;
	int			id;
	HeapTuple   rettuple;
	TupleDesc   tupdesc;
	int         SPIcode;
	char		*relname;
	bool		isnull;
	char		query[1024];
	int			pk_type_oid;
	char		*pk_id = NULL;


	elog(NOTICE,"in lockcheck()");

	/* Make sure trigdata is pointing at what I expect */
    if (!CALLED_AS_TRIGGER(fcinfo))
        elog(ERROR, "lockcheck: not fired by trigger manager");

	rettuple = trigdata->tg_newtuple;
	tupdesc = trigdata->tg_relation->rd_att;

	    /* Connect to SPI manager */
    SPIcode = SPI_connect();

	if (SPIcode  != SPI_OK_CONNECT)
	{
		elog(ERROR,"lockcheck: couldnt open a connection to SPI");
		PG_RETURN_NULL() ;
	}

	relname = SPI_getrelname(trigdata->tg_relation);

	colname  =       trigdata->tg_trigger->tgargs[0];




	elog(NOTICE,"trigger was executed on the relation: '%s', with attribute named '%s', with locktable named '%s'", relname,colname,"authorization_table");

	//get the value

	pk_type_oid =SPI_gettypeid ( tupdesc  , SPI_fnumber(tupdesc,colname));
	elog(NOTICE,"primary key type # = %i", pk_type_oid  ) ;

	if (pk_type_oid == 23) //int4
	{
		int id = (int) DatumGetInt32( SPI_getbinval(rettuple, tupdesc, SPI_fnumber(tupdesc,colname), &isnull) );
		if (isnull)
		{
			elog(ERROR,"lockcheck - column (%s) of table (%s) is null!",colname,relname);
		}
		pk_id = palloc(100);
		sprintf(pk_id,"%i",id);
	}
	else if (pk_type_oid == 25) // text
	{
		 Datum id = ( SPI_getbinval(rettuple, tupdesc, SPI_fnumber(tupdesc,colname), &isnull) );
		 if (isnull)
		 {
		 			elog(ERROR,"lockcheck - column (%s) of table (%s) is null!",colname,relname);
		 }
		 pk_id  = DatumGetCString(DirectFunctionCall1(textout,id));

	}
	else if (pk_type_oid == 1043) // varchar
	{
		Datum id = ( SPI_getbinval(rettuple, tupdesc, SPI_fnumber(tupdesc,colname), &isnull) );
		 if (isnull)
		 {
					elog(ERROR,"lockcheck - column (%s) of table (%s) is null!",colname,relname);
		 }
		 pk_id  = DatumGetCString(DirectFunctionCall1(varcharout,id));
	}
	else
	{
		elog(ERROR,"id column (%s) of table (%s) has to be either int4, text, or varchar.  Its - %s (oid %i)",colname,relname,SPI_gettype ( tupdesc  , SPI_fnumber(tupdesc,colname) ) ,pk_type_oid );
	}


	id = (int) DatumGetInt32( SPI_getbinval(rettuple, tupdesc, SPI_fnumber(tupdesc,colname), &isnull) );

	sprintf(query,"SELECT authid FROM %s WHERE expires >= now() AND tname = '%s' and  fid = '%s'::text", "authorization_table",relname,pk_id);
	elog(NOTICE,"about to execute :%s", query);

	SPIcode = SPI_exec(query,0);
	if (SPIcode !=SPI_OK_SELECT )
		elog(ERROR,"couldnt execute to test for lock :%s",query);


	if (SPI_processed >0)
	{
		// there is a lock - check to see if I have rights to it!

		TupleDesc tupdesc = SPI_tuptable->tupdesc;
		SPITupleTable *tuptable = SPI_tuptable;
		HeapTuple tuple = tuptable->vals[0];
		char	*lockcode = SPI_getvalue(tuple, tupdesc, 1);

		elog(NOTICE,"there is a lock on this row!");

		// check to see if temp_lock_have_table table exists (it might not exist if they own no locks
		sprintf(query,"SELECT * FROM pg_class WHERE relname = 'temp_lock_have_table'");
		SPIcode = SPI_exec(query,0);
		if (SPIcode !=SPI_OK_SELECT )
			elog(ERROR,"couldnt execute to test for lockkey temp table :%s",query);
		if (SPI_processed ==0)
		{
			elog(NOTICE,"I do not own any locks (no lock table) - I cannot modify the row");
			//PG_RETURN_NULL();
			SPI_finish();
			//return NULL;
			elog(ERROR,"attemted to modify a locked row that I do not have authorization for!");
		}


		sprintf(query,"SELECT * FROM temp_lock_have_table  WHERE xideq(transid , getTransactionID() ) AND lockcode ='%s'",lockcode);
		elog(NOTICE,"about to execute :%s", query);

	SPIcode = SPI_exec(query,0);
	if (SPIcode !=SPI_OK_SELECT )
		elog(ERROR,"couldnt execute to test for lock aquire:%s",query);

	if (SPI_processed >0)
	{
		elog(NOTICE,"I own the lock - I can modify the row");
		SPI_finish();
		return PointerGetDatum(rettuple);
	}

		elog(NOTICE,"I do not own the lock - I cannot modify the row");
		//PG_RETURN_NULL();
		SPI_finish();
		//return NULL;
			elog(ERROR,"attemted to modify a locked row that I do not have authorization for!");
	}
	else
	{
		elog(NOTICE,"there is NOT a lock on this row!");
		SPI_finish();
		return PointerGetDatum(rettuple);
	}

}


extern Datum getTransactionID(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(getTransactionID);

Datum getTransactionID(PG_FUNCTION_ARGS)
{
    TransactionId xid = GetCurrentTransactionId();
    PG_RETURN_DATUM( TransactionIdGetDatum(xid) );
}


