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

#include "../postgis_config.h"

#include "liblwgeom.h"
#include "lwgeom_pg.h"

/* Local prototypes */
Datum PGISDirectFunctionCall1(PGFunction func, Datum arg1);
Datum pgis_geometry_accum_transfn(PG_FUNCTION_ARGS);
Datum pgis_geometry_accum_finalfn(PG_FUNCTION_ARGS);
Datum pgis_geometry_union_finalfn(PG_FUNCTION_ARGS);
Datum pgis_geometry_collect_finalfn(PG_FUNCTION_ARGS);
Datum pgis_twkb_accum_finalfn(PG_FUNCTION_ARGS);
Datum pgis_twkb_accum_transfn(PG_FUNCTION_ARGS);
Datum pgis_geometry_polygonize_finalfn(PG_FUNCTION_ARGS);
Datum pgis_geometry_makeline_finalfn(PG_FUNCTION_ARGS);
Datum pgis_abs_in(PG_FUNCTION_ARGS);
Datum pgis_abs_out(PG_FUNCTION_ARGS);

/* External prototypes */
Datum pgis_union_geometry_array(PG_FUNCTION_ARGS);
Datum LWGEOM_collect_garray(PG_FUNCTION_ARGS);
Datum LWGEOM_twkb_garray(PG_FUNCTION_ARGS);
Datum polygonize_garray(PG_FUNCTION_ARGS);
Datum LWGEOM_makeline_garray(PG_FUNCTION_ARGS);


/** @file
** Versions of PostgreSQL < 8.4 perform array accumulation internally using
** pass by value, which is very slow working with large/many geometries.
** Hence PostGIS currently implements its own aggregate for building
** geometry arrays using pass by reference, which is significantly faster and
** similar to the method used in PostgreSQL 8.4.
**
** Hence we can revert this to the original aggregate functions from 1.3 at
** whatever point PostgreSQL 8.4 becomes the minimum version we support :)
*/


/**
** To pass the internal ArrayBuildState pointer between the
** transfn and finalfn we need to wrap it into a custom type first,
** the pgis_abs type in our case.
*/

typedef struct
{
	ArrayBuildState *a;
}
pgis_abs;

typedef struct
{
	Datum id;	//Id, from function parameter
	Datum geom;	//the geometry from function parameter
}
geom_id;


typedef struct
{
	uint8_t variant;
	int	precision;		//number of decimals in coordinates
	int	n_rows;		
	int	max_rows;
	geom_id *geoms;
	int method;
}
twkb_state;


/**
** We're never going to use this type externally so the in/out
** functions are dummies.
*/
PG_FUNCTION_INFO_V1(pgis_abs_in);
Datum
pgis_abs_in(PG_FUNCTION_ARGS)
{
	ereport(ERROR,(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
	               errmsg("function pgis_abs_in not implemented")));
	PG_RETURN_POINTER(NULL);
}
PG_FUNCTION_INFO_V1(pgis_abs_out);
Datum
pgis_abs_out(PG_FUNCTION_ARGS)
{
	ereport(ERROR,(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
	               errmsg("function pgis_abs_out not implemented")));
	PG_RETURN_POINTER(NULL);
}

/**
** The transfer function hooks into the PostgreSQL accumArrayResult()
** function (present since 8.0) to build an array in a side memory
** context.
*/
PG_FUNCTION_INFO_V1(pgis_geometry_accum_transfn);
Datum
pgis_geometry_accum_transfn(PG_FUNCTION_ARGS)
{
	Oid arg1_typeid = get_fn_expr_argtype(fcinfo->flinfo, 1);
	MemoryContext aggcontext;
	ArrayBuildState *state;
	pgis_abs *p;
	Datum elem;

	if (arg1_typeid == InvalidOid)
		ereport(ERROR,
		        (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
		         errmsg("could not determine input data type")));

	if (fcinfo->context && IsA(fcinfo->context, AggState))
		aggcontext = ((AggState *) fcinfo->context)->aggcontext;
	else if (fcinfo->context && IsA(fcinfo->context, WindowAggState))
		aggcontext = ((WindowAggState *) fcinfo->context)->aggcontext;

	else
	{
		/* cannot be called directly because of dummy-type argument */
		elog(ERROR, "array_agg_transfn called in non-aggregate context");
		aggcontext = NULL;  /* keep compiler quiet */
	}

	if ( PG_ARGISNULL(0) )
	{
		p = (pgis_abs*) palloc(sizeof(pgis_abs));
		p->a = NULL;
	}
	else
	{
		p = (pgis_abs*) PG_GETARG_POINTER(0);
	}
	state = p->a;
	elem = PG_ARGISNULL(1) ? (Datum) 0 : PG_GETARG_DATUM(1);
	state = accumArrayResult(state,
	                         elem,
	                         PG_ARGISNULL(1),
	                         arg1_typeid,
	                         aggcontext);
	p->a = state;

	PG_RETURN_POINTER(p);
}



PG_FUNCTION_INFO_V1(pgis_twkb_accum_transfn);
Datum
pgis_twkb_accum_transfn(PG_FUNCTION_ARGS)
{
	MemoryContext aggcontext;
	MemoryContext oldcontext;	
	twkb_state* state;
	int32 newlen;
	GSERIALIZED *geom;
	uint8_t variant = 0;

if (!AggCheckCallContext(fcinfo, &aggcontext))
	{
		/* cannot be called directly because of dummy-type argument */
		elog(ERROR, "array_agg_transfn called in non-aggregate context");
		aggcontext = NULL;  /* keep compiler quiet */
	}
	oldcontext = MemoryContextSwitchTo(aggcontext);
	if ( PG_ARGISNULL(0) )
	{
		/*state gets palloced and 10 geometry elements too
		don't forget to free*/
	 
		state=palloc(sizeof(twkb_state));
		state->geoms = palloc(10*sizeof(geom_id));
		state->max_rows = 10;
		state->n_rows = 0;	
	
		/* If user specified precision, respect it */
		state->precision = PG_ARGISNULL(2) ? (int) 0 : PG_GETARG_INT32(2); 
				
		/* If user specified method, respect it
		This will probably be taken away when we can decide which compression method that is best	*/
		if ((PG_NARGS()>5) && (!PG_ARGISNULL(5)))
		{
			state->method = PG_GETARG_INT32(5); 
		}
		else
		{
			state->method = 1;
		}
	
		//state->method = ((PG_NARGS()>5) && PG_ARGISNULL(5)) ? (int) 1 : PG_GETARG_INT32(5); 

	}
	else
	{
		state = (twkb_state*) PG_GETARG_POINTER(0);
		
		if(!((state->n_rows)<(state->max_rows)))
		{
			    newlen = (state->max_rows)*2; 			
			    /* switch to aggregate memory context */
			    
			    state->geoms = (geom_id*)repalloc((void*)(state->geoms),newlen*sizeof(geom_id));
				
			    state->max_rows = newlen;			    
		}	
	
	}	

	geom = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	((state->geoms)+state->n_rows)->geom = PG_ARGISNULL(1) ? (Datum) 0 : PointerGetDatum(PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(1)));      
	
	
	if ((PG_NARGS()>3) && (!PG_ARGISNULL(3)))
	{
		variant = variant | (WKB_ID);
		((state->geoms)+state->n_rows)->id = PG_GETARG_INT64(3); 
	}
	else
	{
		variant = variant & WKB_NO_ID;
		((state->geoms)+state->n_rows)->id = 0;
	}
	state->variant=variant;
	(state->n_rows)++;	
	MemoryContextSwitchTo(oldcontext); 
	
	PG_RETURN_POINTER(state);
}

Datum pgis_accum_finalfn(pgis_abs *p, MemoryContext mctx, FunctionCallInfo fcinfo);

/**
** The final function rescues the built array from the side memory context
** using the PostgreSQL built-in function makeMdArrayResult
*/
Datum
pgis_accum_finalfn(pgis_abs *p, MemoryContext mctx, FunctionCallInfo fcinfo)
{
	int dims[1];
	int lbs[1];
	ArrayBuildState *state;
	Datum result;

	/* cannot be called directly because of internal-type argument */
	Assert(fcinfo->context &&
	       (IsA(fcinfo->context, AggState) ||
	        IsA(fcinfo->context, WindowAggState))
	       );

	state = p->a;
	dims[0] = state->nelems;
	lbs[0] = 1;
	result = makeMdArrayResult(state, 1, dims, lbs, mctx, false);
	return result;
}

/**
** The "accum" final function just returns the geometry[]
*/
PG_FUNCTION_INFO_V1(pgis_geometry_accum_finalfn);
Datum
pgis_geometry_accum_finalfn(PG_FUNCTION_ARGS)
{
	pgis_abs *p;
	Datum result = 0;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();   /* returns null iff no input values */

	p = (pgis_abs*) PG_GETARG_POINTER(0);

	result = pgis_accum_finalfn(p, CurrentMemoryContext, fcinfo);

	PG_RETURN_DATUM(result);

}

/**
* The "accum" final function passes the geometry[] to a union
* conversion before returning the result.
*/
PG_FUNCTION_INFO_V1(pgis_geometry_union_finalfn);
Datum
pgis_geometry_union_finalfn(PG_FUNCTION_ARGS)
{
	pgis_abs *p;
	Datum result = 0;
	Datum geometry_array = 0;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();   /* returns null iff no input values */

	p = (pgis_abs*) PG_GETARG_POINTER(0);

	geometry_array = pgis_accum_finalfn(p, CurrentMemoryContext, fcinfo);
	result = PGISDirectFunctionCall1( pgis_union_geometry_array, geometry_array );
	if (!result)
		PG_RETURN_NULL();

	PG_RETURN_DATUM(result);
}

/**
* The "collect" final function passes the geometry[] to a geometrycollection
* conversion before returning the result.
*/
PG_FUNCTION_INFO_V1(pgis_geometry_collect_finalfn);
Datum
pgis_geometry_collect_finalfn(PG_FUNCTION_ARGS)
{
	pgis_abs *p;
	Datum result = 0;
	Datum geometry_array = 0;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();   /* returns null iff no input values */

	p = (pgis_abs*) PG_GETARG_POINTER(0);

	geometry_array = pgis_accum_finalfn(p, CurrentMemoryContext, fcinfo);
	result = PGISDirectFunctionCall1( LWGEOM_collect_garray, geometry_array );
	if (!result)
		PG_RETURN_NULL();

	PG_RETURN_DATUM(result);
}

/**
* The "twkb" final function passes the geometry[] to a twkb
* conversion before returning the result.
*/
PG_FUNCTION_INFO_V1(pgis_twkb_accum_finalfn);
Datum
pgis_twkb_accum_finalfn(PG_FUNCTION_ARGS)
{
	int i;
	twkb_state *state;
	geom_id *geom_array;
	twkb_geom_arrays lwgeom_arrays;
	GSERIALIZED *geom;
	LWGEOM *lwgeom;
	size_t twkb_size;	
	uint8_t *twkb;
	bytea *result;
	state =  (twkb_state*)  PG_GETARG_POINTER(0);
	lwgeom_arrays.n_points=lwgeom_arrays.n_linestrings=lwgeom_arrays.n_polygons=lwgeom_arrays.n_collections=0;
	geom_array=state->geoms;


	
	
	for (i=0;i<state->n_rows;i++)
	{
		geom = (GSERIALIZED*)PG_DETOAST_DATUM((geom_array+i)->geom);
		lwgeom = lwgeom_from_gserialized(geom);
		switch ( lwgeom->type )
		{
			case POINTTYPE:
				if (lwgeom_arrays.n_points==0)
					lwgeom_arrays.points = palloc(((state->n_rows)-i)*sizeof(geom_id));
				
				(lwgeom_arrays.points+lwgeom_arrays.n_points)->geom=lwgeom;
				(lwgeom_arrays.points+lwgeom_arrays.n_points)->id=(geom_array+i)->id;
				(lwgeom_arrays.n_points)++;
				break;
			/* LineString and CircularString both have 'points' elements */
			case LINETYPE:
				if (lwgeom_arrays.n_linestrings==0)
					lwgeom_arrays.linestrings = palloc(((state->n_rows)-i)*sizeof(geom_id));
				
				(lwgeom_arrays.linestrings+lwgeom_arrays.n_linestrings)->geom=lwgeom;
				(lwgeom_arrays.linestrings+lwgeom_arrays.n_linestrings)->id=(geom_array+i)->id;
				(lwgeom_arrays.n_linestrings)++;
				break;

			/* Polygon has 'nrings' and 'rings' elements */
			case POLYGONTYPE:
				if (lwgeom_arrays.n_polygons==0)
					lwgeom_arrays.polygons = palloc(((state->n_rows)-i)*sizeof(geom_id));
				
				(lwgeom_arrays.polygons+lwgeom_arrays.n_polygons)->geom=lwgeom;
				(lwgeom_arrays.polygons+lwgeom_arrays.n_polygons)->id=(geom_array+i)->id;
				(lwgeom_arrays.n_polygons)++;
				break;

			/* Triangle has one ring of three points 
			case TRIANGLETYPE:
				return lwtriangle_to_twkb_buf((LWTRIANGLE*)geom, buf, variant);
	*/
			/* All these Collection types have 'ngeoms' and 'geoms' elements */
			case MULTIPOINTTYPE:
			case MULTILINETYPE:
			case MULTIPOLYGONTYPE:
			case COLLECTIONTYPE:
				if (lwgeom_arrays.n_collections==0)
					lwgeom_arrays.collections = palloc(((state->n_rows)-i)*sizeof(geom_id));
				
				(lwgeom_arrays.collections+lwgeom_arrays.n_collections)->geom=lwgeom;
				(lwgeom_arrays.collections+lwgeom_arrays.n_collections)->id=(geom_array+i)->id;
				(lwgeom_arrays.n_collections)++;
				break;

			/* Unknown type! */
			default:
				lwerror("Unsupported geometry type: %s [%d]", lwtype_name(lwgeom->type), lwgeom->type);
		}
	
	}		
	twkb = lwgeom_agg_to_twkb(&lwgeom_arrays, state->variant , &twkb_size,(int8_t) state->precision,state->method);


		/* Clean up and return */
	/* Prepare the PgSQL text return type */
	
	result = palloc(twkb_size + VARHDRSZ);
	SET_VARSIZE(result, twkb_size+VARHDRSZ);
	memcpy(VARDATA(result), twkb, twkb_size);

	/* Clean up and return */
	pfree(twkb);

	PG_RETURN_BYTEA_P(result);
}


/**
* The "polygonize" final function passes the geometry[] to a polygonization
* before returning the result.
*/
PG_FUNCTION_INFO_V1(pgis_geometry_polygonize_finalfn);
Datum
pgis_geometry_polygonize_finalfn(PG_FUNCTION_ARGS)
{
	pgis_abs *p;
	Datum result = 0;
	Datum geometry_array = 0;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();   /* returns null iff no input values */

	p = (pgis_abs*) PG_GETARG_POINTER(0);

	geometry_array = pgis_accum_finalfn(p, CurrentMemoryContext, fcinfo);
	result = DirectFunctionCall1( polygonize_garray, geometry_array );

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
	pgis_abs *p;
	Datum result = 0;
	Datum geometry_array = 0;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();   /* returns null iff no input values */

	p = (pgis_abs*) PG_GETARG_POINTER(0);

	geometry_array = pgis_accum_finalfn(p, CurrentMemoryContext, fcinfo);
	result = PGISDirectFunctionCall1( LWGEOM_makeline_garray, geometry_array );
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
	FunctionCallInfoData fcinfo;
	Datum           result;

#if POSTGIS_PGSQL_VERSION > 90

	InitFunctionCallInfoData(fcinfo, NULL, 1, InvalidOid, NULL, NULL);
#else

	InitFunctionCallInfoData(fcinfo, NULL, 1, NULL, NULL);
#endif

	fcinfo.arg[0] = arg1;
	fcinfo.argnull[0] = false;

	result = (*func) (&fcinfo);

	/* Check for null result, returning a "NULL" Datum if indicated */
	if (fcinfo.isnull)
		return (Datum) 0;

	return result;
}
