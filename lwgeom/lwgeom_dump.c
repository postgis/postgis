/*

-- To enable the dump() function copy this SQL into lwpostgis.sql.in
-- and run make, then open lwpostgis.sql and feed the corresponding
-- queries to your spatial db
--
-- Also, add lwgeom_dump.o to OBJS variable in Makefile

CREATE TYPE geometry_dump AS (path integer[], geom geometry);

CREATEFUNCTION dump(geometry)
	RETURNS SETOF geometry_dump
	AS '@MODULE_FILENAME@', 'LWGEOM_dump'
	LANGUAGE 'C' WITH (isstrict,iscachable);

*/

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "postgres.h"
#include "fmgr.h"
#include "utils/elog.h"
#include "utils/array.h"
#include "utils/geo_decls.h"
#include "funcapi.h"

#include "liblwgeom.h"
#include "lwgeom_pg.h"
#include "profile.h"
#include "wktparse.h"

Datum LWGEOM_dump(PG_FUNCTION_ARGS);

typedef struct GEOMDUMPNODE_T {
	int idx;
	LWCOLLECTION *geom;
} GEOMDUMPNODE;

#define MAXDEPTH 32
typedef struct GEOMDUMPSTATE {
	int stacklen;
	GEOMDUMPNODE *stack[MAXDEPTH];
	LWGEOM *root;
	LWGEOM *geom;
} GEOMDUMPSTATE;

#define PUSH(x,y) ((x)->stack[(x)->stacklen++]=(y))
#define LAST(x) ((x)->stack[(x)->stacklen-1])
#define POP(x) (--((x)->stacklen))

PG_FUNCTION_INFO_V1(LWGEOM_dump);
Datum LWGEOM_dump(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *pglwgeom;
	LWCOLLECTION *lwcoll;
	LWGEOM *lwgeom;
	FuncCallContext *funcctx;
	GEOMDUMPSTATE *state;
	GEOMDUMPNODE *node;
	TupleDesc tupdesc;
	TupleTableSlot *slot;
	HeapTuple tuple;
	AttInMetadata *attinmeta;
	MemoryContext oldcontext;
	Datum result;
	char address[256];
	char *ptr;
	unsigned int i;
	char *values[2];

	if (SRF_IS_FIRSTCALL())
	{
		funcctx = SRF_FIRSTCALL_INIT();

		oldcontext = MemoryContextSwitchTo(
			funcctx->multi_call_memory_ctx);

		pglwgeom = (PG_LWGEOM *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));
		lwgeom = lwgeom_deserialize(SERIALIZED_FORM(pglwgeom));

		/* Create function state */
		state = lwalloc(sizeof(GEOMDUMPSTATE));
		state->root = lwgeom;
		state->stacklen=0;

		if ( TYPE_GETTYPE(lwgeom->type) >= MULTIPOINTTYPE )
		{
			/*
			 * Push a GEOMDUMPNODE on the state stack
			 */
			node = lwalloc(sizeof(GEOMDUMPNODE));
			node->idx=0;
			node->geom = (LWCOLLECTION *)lwgeom;
			PUSH(state, node);
		}

		funcctx->user_fctx = state;

		/*
		 * Build a tuple description for an
		 * geometry_dump tuple
		 */
		tupdesc = RelationNameGetTupleDesc("geometry_dump");

		/* allocate a slot for a tuple with this tupdesc */
		slot = TupleDescGetSlot(tupdesc);

		/* allocate a slot for a tuple with this tupdesc */
		slot = TupleDescGetSlot(tupdesc);

		/* assign slot to function context */
		funcctx->slot = slot;

		/*
		 * generate attribute metadata needed later to produce
		 * tuples from raw C strings
		 */
		attinmeta = TupleDescGetAttInMetadata(tupdesc);
		funcctx->attinmeta = attinmeta;

		MemoryContextSwitchTo(oldcontext);
	}

	/* stuff done on every call of the function */
	funcctx = SRF_PERCALL_SETUP();

	/* get state */
	state = funcctx->user_fctx;

	/* Handled simple geometries */
	if ( ! state->root ) SRF_RETURN_DONE(funcctx);
	if ( TYPE_GETTYPE(state->root->type) < MULTIPOINTTYPE )
	{
		values[0] = "{}";
		values[1] = lwgeom_to_hexwkb(state->root, -1);
		tuple = BuildTupleFromCStrings(funcctx->attinmeta, values);
		result = TupleGetDatum(funcctx->slot, tuple);

		//lwnotice("%s", address);
		//pglwgeom = pglwgeom_serialize(lwgeom);
		//result = PointerGetDatum(pglwgeom);
		state->root = NULL;
		SRF_RETURN_NEXT(funcctx, result);
	}

	while (1)
	{
		node = LAST(state);
		lwcoll = node->geom;

		if ( node->idx < lwcoll->ngeoms )
		{
			lwgeom = lwcoll->geoms[node->idx];
			if ( TYPE_GETTYPE(lwgeom->type) < MULTIPOINTTYPE )
			{
	/* write address of current geom */
	ptr=address; *ptr++='{';
	for (i=0; i<state->stacklen; i++)
	{
		if ( i ) ptr += sprintf(ptr, ",");
		ptr += sprintf(ptr, "%d", state->stack[i]->idx+1);
	}
	*ptr++='}'; *ptr='\0';

				break;
			}

			/*
			 * It's a collection, increment index
			 * of current node, push a new one on the
			 * stack
			 */
			node = lwalloc(sizeof(GEOMDUMPNODE));
			node->idx=0;
			node->geom = (LWCOLLECTION *)lwgeom;
			PUSH(state, node);
			continue;
		}

		if ( ! POP(state) ) SRF_RETURN_DONE(funcctx);
		LAST(state)->idx++;
	}

	lwgeom->SRID = state->root->SRID;

	values[0] = address;
	values[1] = lwgeom_to_hexwkb(lwgeom, -1);
	tuple = BuildTupleFromCStrings(funcctx->attinmeta, values);
	result = TupleGetDatum(funcctx->slot, tuple);
	node->idx++;
	SRF_RETURN_NEXT(funcctx, result);
}


