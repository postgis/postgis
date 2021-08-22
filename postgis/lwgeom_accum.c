/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * PostGIS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * PostGIS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PostGIS.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 *
 * Copyright 2009 Paul Ramsey <pramsey@opengeo.org>
 *
 **********************************************************************/


#include "postgres.h"
#include "fmgr.h"
#include "funcapi.h"
#include "access/tupmacs.h"
#include "utils/datum.h"
#include "utils/array.h"
#include "utils/lsyscache.h"

#include "../postgis_config.h"

#include "liblwgeom.h"
#include "lwgeom_geos.h"
#include "lwgeom_pg.h"
#include "lwgeom_transform.h"
#include "lwgeom_accum.h"

/* Local prototypes */
Datum PGISDirectFunctionCall1(PGFunction func, Datum arg1);
Datum PGISDirectFunctionCall2(PGFunction func, Datum arg1, Datum arg2);
Datum pgis_geometry_accum_transfn(PG_FUNCTION_ARGS);
Datum pgis_geometry_collect_finalfn(PG_FUNCTION_ARGS);
Datum pgis_geometry_polygonize_finalfn(PG_FUNCTION_ARGS);
Datum pgis_geometry_makeline_finalfn(PG_FUNCTION_ARGS);
Datum pgis_geometry_clusterintersecting_finalfn(PG_FUNCTION_ARGS);
Datum pgis_geometry_clusterwithin_finalfn(PG_FUNCTION_ARGS);

/* External prototypes */
Datum pgis_union_geometry_array(PG_FUNCTION_ARGS);
Datum LWGEOM_collect_garray(PG_FUNCTION_ARGS);
Datum polygonize_garray(PG_FUNCTION_ARGS);
Datum clusterintersecting_garray(PG_FUNCTION_ARGS);
Datum cluster_within_distance_garray(PG_FUNCTION_ARGS);
Datum LWGEOM_makeline_garray(PG_FUNCTION_ARGS);


/**
** The transfer function builds a List of LWGEOM* allocated
** in the aggregate memory context. The pgis_accum_finalfn
** converts that List into a Pg Array.
*/
PG_FUNCTION_INFO_V1(pgis_geometry_accum_transfn);
Datum
pgis_geometry_accum_transfn(PG_FUNCTION_ARGS)
{
	MemoryContext aggcontext, old;
	CollectionBuildState *state;
	LWGEOM *geom = NULL;
	GSERIALIZED *gser = NULL;
	Datum argType = get_fn_expr_argtype(fcinfo->flinfo, 1);
	double gridSize = -1.0;

	if (argType == InvalidOid)
		ereport(ERROR,
		        (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
		         errmsg("could not determine input data type")));

	if ( ! AggCheckCallContext(fcinfo, &aggcontext) )
	{
		/* cannot be called directly because of dummy-type argument */
		elog(ERROR, "%s called in non-aggregate context", __func__);
		aggcontext = NULL;  /* keep compiler quiet */
	}

	if ( PG_ARGISNULL(0) )
	{
		int n = ((PG_NARGS()-2) <= CollectionBuildStateDataSize) ? (PG_NARGS()-2) : CollectionBuildStateDataSize;

		state = MemoryContextAlloc(aggcontext, sizeof(CollectionBuildState));
		state->geoms = NULL;
		state->geomOid = argType;
		state->gridSize = gridSize;

		for (int i = 0; i < n; i++)
		{
			Datum argument = PG_GETARG_DATUM(i+2);
			Oid dataOid = get_fn_expr_argtype(fcinfo->flinfo, i+2);
			old = MemoryContextSwitchTo(aggcontext);
			state->data[i] = datumCopy(argument, get_typbyval(dataOid), get_typlen(dataOid));
			MemoryContextSwitchTo(old);
		}
	}
	else
	{
		state = (CollectionBuildState*) PG_GETARG_POINTER(0);
	}

	if (!PG_ARGISNULL(1))
		gser = PG_GETARG_GSERIALIZED_P(1);

	if (PG_NARGS()>2 && !PG_ARGISNULL(2))
	{
		gridSize = PG_GETARG_FLOAT8(2);
		/*lwnotice("Passed gridSize %g", gridSize);*/
		if ( gridSize > state->gridSize ) state->gridSize = gridSize;
	}

	/* Take a copy of the geometry into the aggregate context */
	old = MemoryContextSwitchTo(aggcontext);
	if (gser)
		geom = lwgeom_clone_deep(lwgeom_from_gserialized(gser));

	/* Initialize or append to list as necessary */
	if (state->geoms)
		state->geoms = lappend(state->geoms, geom);
	else
		state->geoms = list_make1(geom);

	MemoryContextSwitchTo(old);

	PG_RETURN_POINTER(state);
}


Datum pgis_accum_finalfn(CollectionBuildState *state, MemoryContext mctx, FunctionCallInfo fcinfo);

/**
** The final function reads the List of LWGEOM* from the aggregate
** memory context and constructs an Array using construct_md_array()
*/
Datum
pgis_accum_finalfn(CollectionBuildState *state, MemoryContext mctx, __attribute__((__unused__)) FunctionCallInfo fcinfo)
{
	ListCell *l;
	size_t nelems = 0;
	Datum *elems;
	bool *nulls;
	int16 elmlen;
	bool elmbyval;
	char elmalign;
	size_t i = 0;
	ArrayType *arr;
	int dims[1];
	int lbs[1] = {1};

	/* cannot be called directly because of internal-type argument */
	Assert(fcinfo->context &&
	       (IsA(fcinfo->context, AggState) ||
	        IsA(fcinfo->context, WindowAggState))
	       );

	/* Retrieve geometry type metadata */
	get_typlenbyvalalign(state->geomOid, &elmlen, &elmbyval, &elmalign);
	nelems = list_length(state->geoms);

	/* Build up an array, because that's what we pass to all the */
	/* specific final functions */
	elems = palloc(nelems * sizeof(Datum));
	nulls = palloc(nelems * sizeof(bool));

	foreach (l, state->geoms)
	{
		LWGEOM *geom = (LWGEOM*)(lfirst(l));
		Datum elem = (Datum)0;
		bool isNull = true;
		if (geom)
		{
			GSERIALIZED *gser = geometry_serialize(geom);
			elem = PointerGetDatum(gser);
			isNull = false;
		}
		elems[i] = elem;
		nulls[i] = isNull;
		i++;

		if (i >= nelems)
			break;
	}

	/* Turn element array into PgSQL array */
	dims[0] = nelems;
	arr = construct_md_array(elems, nulls, 1, dims, lbs, state->geomOid,
	                         elmlen, elmbyval, elmalign);

	return PointerGetDatum(arr);
}

/**
* The "collect" final function passes the geometry[] to a geometrycollection
* conversion before returning the result.
*/
PG_FUNCTION_INFO_V1(pgis_geometry_collect_finalfn);
Datum
pgis_geometry_collect_finalfn(PG_FUNCTION_ARGS)
{
	CollectionBuildState *p;
	Datum result = 0;
	Datum geometry_array = 0;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();   /* returns null iff no input values */

	p = (CollectionBuildState*) PG_GETARG_POINTER(0);

	geometry_array = pgis_accum_finalfn(p, CurrentMemoryContext, fcinfo);
	result = PGISDirectFunctionCall1( LWGEOM_collect_garray, geometry_array );
	if (!result)
		PG_RETURN_NULL();

	PG_RETURN_DATUM(result);
}


/**
* The "polygonize" final function passes the geometry[] to a polygonization
* before returning the result.
*/
PG_FUNCTION_INFO_V1(pgis_geometry_polygonize_finalfn);
Datum
pgis_geometry_polygonize_finalfn(PG_FUNCTION_ARGS)
{
	CollectionBuildState *p;
	Datum result = 0;
	Datum geometry_array = 0;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();   /* returns null iff no input values */

	p = (CollectionBuildState*) PG_GETARG_POINTER(0);

	geometry_array = pgis_accum_finalfn(p, CurrentMemoryContext, fcinfo);
	result = PGISDirectFunctionCall1( polygonize_garray, geometry_array );
	if (!result)
		PG_RETURN_NULL();

	PG_RETURN_DATUM(result);
}

/**
* The "makeline" final function passes the geometry[] to a line builder
* before returning the result.
*/
PG_FUNCTION_INFO_V1(pgis_geometry_makeline_finalfn);
Datum
pgis_geometry_makeline_finalfn(PG_FUNCTION_ARGS)
{
	CollectionBuildState *p;
	Datum result = 0;
	Datum geometry_array = 0;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();   /* returns null iff no input values */

	p = (CollectionBuildState*) PG_GETARG_POINTER(0);

	geometry_array = pgis_accum_finalfn(p, CurrentMemoryContext, fcinfo);
	result = PGISDirectFunctionCall1( LWGEOM_makeline_garray, geometry_array );
	if (!result)
		PG_RETURN_NULL();

	PG_RETURN_DATUM(result);
}

/**
 * The "clusterintersecting" final function passes the geometry[] to a
 * clustering function before returning the result.
*/
PG_FUNCTION_INFO_V1(pgis_geometry_clusterintersecting_finalfn);
Datum
pgis_geometry_clusterintersecting_finalfn(PG_FUNCTION_ARGS)
{
	CollectionBuildState *p;
	Datum result = 0;
	Datum geometry_array = 0;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	p = (CollectionBuildState*) PG_GETARG_POINTER(0);
	geometry_array = pgis_accum_finalfn(p, CurrentMemoryContext, fcinfo);
	result = PGISDirectFunctionCall1( clusterintersecting_garray, geometry_array );
	if (!result)
		PG_RETURN_NULL();

	PG_RETURN_DATUM(result);
}

/**
 * The "cluster_within_distance" final function passes the geometry[] to a
 * clustering function before returning the result.
*/
PG_FUNCTION_INFO_V1(pgis_geometry_clusterwithin_finalfn);
Datum
pgis_geometry_clusterwithin_finalfn(PG_FUNCTION_ARGS)
{
	CollectionBuildState *p;
	Datum result = 0;
	Datum geometry_array = 0;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	p = (CollectionBuildState*) PG_GETARG_POINTER(0);

	if (!p->data[0])
	{
		elog(ERROR, "Tolerance not defined");
		PG_RETURN_NULL();
	}

	geometry_array = pgis_accum_finalfn(p, CurrentMemoryContext, fcinfo);
	result = PGISDirectFunctionCall2( cluster_within_distance_garray, geometry_array, p->data[0]);
	if (!result)
		PG_RETURN_NULL();

	PG_RETURN_DATUM(result);
}

/**
* A modified version of PostgreSQL's DirectFunctionCall1 which allows NULL results; this
* is required for aggregates that return NULL.
*/
Datum
PGISDirectFunctionCall1(PGFunction func, Datum arg1)
{
#if POSTGIS_PGSQL_VERSION < 120
	FunctionCallInfoData fcinfo;
	Datum           result;


	InitFunctionCallInfoData(fcinfo, NULL, 1, InvalidOid, NULL, NULL);


	fcinfo.arg[0] = arg1;
	fcinfo.argnull[0] = false;

	result = (*func) (&fcinfo);

	/* Check for null result, returning a "NULL" Datum if indicated */
	if (fcinfo.isnull)
		return (Datum) 0;

	return result;
#else
	LOCAL_FCINFO(fcinfo, 1);
	Datum result;

	InitFunctionCallInfoData(*fcinfo, NULL, 1, InvalidOid, NULL, NULL);

	fcinfo->args[0].value = arg1;
	fcinfo->args[0].isnull = false;

	result = (*func)(fcinfo);

	/* Check for null result, returning a "NULL" Datum if indicated */
	if (fcinfo->isnull)
		return (Datum)0;

	return result;
#endif /* POSTGIS_PGSQL_VERSION < 120 */
}

/**
* A modified version of PostgreSQL's DirectFunctionCall2 which allows NULL results; this
* is required for aggregates that return NULL.
*/
Datum
PGISDirectFunctionCall2(PGFunction func, Datum arg1, Datum arg2)
{
#if POSTGIS_PGSQL_VERSION < 120
	FunctionCallInfoData fcinfo;
	Datum           result;

	InitFunctionCallInfoData(fcinfo, NULL, 2, InvalidOid, NULL, NULL);

	fcinfo.arg[0] = arg1;
	fcinfo.arg[1] = arg2;
	fcinfo.argnull[0] = false;
	fcinfo.argnull[1] = false;

	result = (*func) (&fcinfo);

	/* Check for null result, returning a "NULL" Datum if indicated */
	if (fcinfo.isnull)
		return (Datum) 0;

	return result;
#else
	LOCAL_FCINFO(fcinfo, 2);
	Datum result;

	InitFunctionCallInfoData(*fcinfo, NULL, 2, InvalidOid, NULL, NULL);

	fcinfo->args[0].value = arg1;
	fcinfo->args[1].value = arg2;
	fcinfo->args[0].isnull = false;
	fcinfo->args[1].isnull = false;

	result = (*func)(fcinfo);

	/* Check for null result, returning a "NULL" Datum if indicated */
	if (fcinfo->isnull)
		return (Datum)0;

	return result;
#endif /* POSTGIS_PGSQL_VERSION < 120 */
}
