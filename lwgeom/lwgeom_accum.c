/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2009 Paul Ramsey <pramsey@opengeo.org>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "postgres.h"
#include "fmgr.h"
#include "funcapi.h"
#include "access/tupmacs.h"
#include "utils/array.h"
#include "utils/lsyscache.h"

#include "liblwgeom.h"
#include "lwgeom_pg.h"

Datum pgis_geometry_accum_transfn(PG_FUNCTION_ARGS);
Datum pgis_geometry_accum_finalfn(PG_FUNCTION_ARGS);

/*
 * ARRAY_AGG aggregate function
 */
PG_FUNCTION_INFO_V1(pgis_geometry_accum_transfn);
Datum
pgis_geometry_accum_transfn(PG_FUNCTION_ARGS)
{
	Oid arg1_typeid = get_fn_expr_argtype(fcinfo->flinfo, 1);
	MemoryContext aggcontext;
	ArrayBuildState *state;
	Datum		elem;

	if (arg1_typeid == InvalidOid)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("could not determine input data type")));

	if (fcinfo->context && IsA(fcinfo->context, AggState))
		aggcontext = ((AggState *) fcinfo->context)->aggcontext;
#if POSTGIS_PGSQL_VERSION >= 84
	else if (fcinfo->context && IsA(fcinfo->context, WindowAggState))
		aggcontext = ((WindowAggState *) fcinfo->context)->wincontext;
#endif
	else
	{
		/* cannot be called directly because of internal-type argument */
		elog(ERROR, "array_agg_transfn called in non-aggregate context");
		aggcontext = NULL;		/* keep compiler quiet */
	}

	state = PG_ARGISNULL(0) ? NULL : (ArrayBuildState *) PG_GETARG_POINTER(0);
	elem = PG_ARGISNULL(1) ? (Datum) 0 : PG_GETARG_DATUM(1);
	state = accumArrayResult(state,
							 elem,
							 PG_ARGISNULL(1),
							 arg1_typeid,
							 aggcontext);

	/*
	 * The transition type for array_agg() is declared to be "internal",
	 * which is a pass-by-value type the same size as a pointer.  So we
	 * can safely pass the ArrayBuildState pointer through nodeAgg.c's
	 * machinations.
	 */
	PG_RETURN_POINTER(state);
}

PG_FUNCTION_INFO_V1(pgis_geometry_accum_finalfn);
Datum
pgis_geometry_accum_finalfn(PG_FUNCTION_ARGS)
{
	Datum		result;
	ArrayBuildState *state;
	int			dims[1];
	int			lbs[1];

	/* cannot be called directly because of internal-type argument */
	Assert(fcinfo->context &&
		   (IsA(fcinfo->context, AggState) ||
			IsA(fcinfo->context, WindowAggState)));

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();   /* returns null iff no input values */

	state = (ArrayBuildState *) PG_GETARG_POINTER(0);

	dims[0] = state->nelems;
	lbs[0] = 1;

#if POSTGIS_PGSQL_VERSION < 84
    result = makeMdArrayResult(state, 1, dims, lbs,
                               CurrentMemoryContext);
#else
	/* Release working state if regular aggregate, but not if window agg */
 	result = makeMdArrayResult(state, 1, dims, lbs,
                       	       CurrentMemoryContext,
                       	       IsA(fcinfo->context, AggState));
#endif
	PG_RETURN_DATUM(result);
}