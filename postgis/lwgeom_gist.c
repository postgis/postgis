#include "postgres.h"
#include "access/gist.h"
#include "access/itup.h"
#include "access/skey.h"

#include "fmgr.h"
#include "utils/elog.h"

#include "../postgis_config.h"
#include "liblwgeom.h"

#include "lwgeom_pg.h"

#include <math.h>
#if HAVE_IEEEFP_H
#include <ieeefp.h>
#endif
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>


/**
 * @file implementation GiST support and basic LWGEOM operations (like &&)
 */



Datum LWGEOM_overlap(PG_FUNCTION_ARGS);
Datum LWGEOM_overleft(PG_FUNCTION_ARGS);
Datum LWGEOM_left(PG_FUNCTION_ARGS);
Datum LWGEOM_right(PG_FUNCTION_ARGS);
Datum LWGEOM_overright(PG_FUNCTION_ARGS);
Datum LWGEOM_overbelow(PG_FUNCTION_ARGS);
Datum LWGEOM_below(PG_FUNCTION_ARGS);
Datum LWGEOM_above(PG_FUNCTION_ARGS);
Datum LWGEOM_overabove(PG_FUNCTION_ARGS);
Datum LWGEOM_contained(PG_FUNCTION_ARGS);
Datum LWGEOM_samebox(PG_FUNCTION_ARGS);
Datum LWGEOM_contain(PG_FUNCTION_ARGS);
Datum LWGEOM_gist_compress(PG_FUNCTION_ARGS);
Datum LWGEOM_gist_consistent(PG_FUNCTION_ARGS);
Datum LWGEOM_gist_decompress(PG_FUNCTION_ARGS);
Datum LWGEOM_gist_union(PG_FUNCTION_ARGS);
Datum LWGEOM_gist_penalty(PG_FUNCTION_ARGS);
Datum LWGEOM_gist_same(PG_FUNCTION_ARGS);
Datum LWGEOM_gist_picksplit(PG_FUNCTION_ARGS);



static float size_box2d(Datum box);

static bool lwgeom_rtree_internal_consistent(BOX2DFLOAT4 *key, BOX2DFLOAT4 *query, StrategyNumber strategy);
static bool lwgeom_rtree_leaf_consistent(BOX2DFLOAT4 *key,BOX2DFLOAT4 *query,	StrategyNumber strategy);



/* for debugging */
static int counter_leaf = 0;
static int counter_intern = 0;


/** GiST strategies (modified from src/include/access/rtree.h) */
#define RTLeftStrategyNumber			1
#define RTOverLeftStrategyNumber		2
#define RTOverlapStrategyNumber			3
#define RTOverRightStrategyNumber		4
#define RTRightStrategyNumber			5
#define RTSameStrategyNumber			6
#define RTContainsStrategyNumber		7
#define RTContainedByStrategyNumber		8
#define RTOverBelowStrategyNumber		9
#define RTBelowStrategyNumber			10
#define RTAboveStrategyNumber			11
#define RTOverAboveStrategyNumber		12


/**
 * all the lwgeom_<same,overlpa,overleft,left,right,overright,overbelow,below,above,overabove,contained,contain>
 *  work the same.
 *  1. get lwgeom1
 *  2. get lwgeom2
 *  3. get box2d for lwgeom1
 *  4. get box2d for lwgeom2
 *  7. call the appropriate BOX2DFLOAT4 function
 *  8. return result;
 */

PG_FUNCTION_INFO_V1(LWGEOM_overlap);
Datum LWGEOM_overlap(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *lwgeom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	bool result;
	BOX2DFLOAT4 box1;
	BOX2DFLOAT4 box2;

	POSTGIS_DEBUG(2, "GIST: LWGEOM_overlap --entry");

	if ( pglwgeom_get_srid(lwgeom1) != pglwgeom_get_srid(lwgeom2) )
	{
		PG_FREE_IF_COPY(lwgeom1, 0);
		PG_FREE_IF_COPY(lwgeom2, 1);
		elog(ERROR, "Operation on two geometries with different SRIDs");
		PG_RETURN_NULL();
	}

	if ( ! (pglwgeom_getbox2d_p(lwgeom1, &box1) && pglwgeom_getbox2d_p(lwgeom2, &box2)) )
	{
		PG_FREE_IF_COPY(lwgeom1, 0);
		PG_FREE_IF_COPY(lwgeom2, 1);
		/* One or both are empty geoms */
		PG_RETURN_BOOL(FALSE);
	}

	result = DatumGetBool(DirectFunctionCall2(BOX2D_overlap,
	                      PointerGetDatum(&box1), PointerGetDatum(&box2)));

	PG_FREE_IF_COPY(lwgeom1, 0);
	PG_FREE_IF_COPY(lwgeom2, 1);

	POSTGIS_DEBUGF(3, "GIST: lwgeom_overlap:\n(%f %f, %f %f) (%f %f %f %f) = %i",
	               box1.xmin, box1.ymax, box1.xmax, box1.ymax,
	               box2.xmin, box2.ymax, box2.xmax, box2.ymax,
	               result
	              );

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(LWGEOM_overleft);
Datum LWGEOM_overleft(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *lwgeom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	bool result;
	BOX2DFLOAT4 box1;
	BOX2DFLOAT4 box2;

	POSTGIS_DEBUG(2, "GIST: LWGEOM_overleft --entry");

	if ( pglwgeom_get_srid(lwgeom1) != pglwgeom_get_srid(lwgeom2) )
	{
		PG_FREE_IF_COPY(lwgeom1, 0);
		PG_FREE_IF_COPY(lwgeom2, 1);
		elog(ERROR, "Operation on two geometries with different SRIDs");
		PG_RETURN_NULL();
	}

	if ( ! (pglwgeom_getbox2d_p(lwgeom1, &box1) && pglwgeom_getbox2d_p(lwgeom2, &box2)) )
	{
		PG_FREE_IF_COPY(lwgeom1, 0);
		PG_FREE_IF_COPY(lwgeom2, 1);
		PG_RETURN_BOOL(FALSE);
	}


	result = DatumGetBool(DirectFunctionCall2(BOX2D_overleft,
	                      PointerGetDatum(&box1), PointerGetDatum(&box2)));

	PG_FREE_IF_COPY(lwgeom1, 0);
	PG_FREE_IF_COPY(lwgeom2, 1);

	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(LWGEOM_left);
Datum LWGEOM_left(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *lwgeom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	bool result;
	BOX2DFLOAT4 box1;
	BOX2DFLOAT4 box2;

	POSTGIS_DEBUG(2, "GIST: LWGEOM_left --entry");

	error_if_srid_mismatch(pglwgeom_get_srid(lwgeom1), pglwgeom_get_srid(lwgeom2));

	if ( ! (pglwgeom_getbox2d_p(lwgeom1, &box1) && pglwgeom_getbox2d_p(lwgeom2, &box2)) )
	{
		PG_FREE_IF_COPY(lwgeom1, 0);
		PG_FREE_IF_COPY(lwgeom2, 1);
		PG_RETURN_BOOL(FALSE);
	}

	result = DatumGetBool(DirectFunctionCall2(BOX2D_left,
	                      PointerGetDatum(&box1), PointerGetDatum(&box2)));

	PG_FREE_IF_COPY(lwgeom1, 0);
	PG_FREE_IF_COPY(lwgeom2, 1);

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(LWGEOM_right);
Datum LWGEOM_right(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *lwgeom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	bool result;
	BOX2DFLOAT4 box1;
	BOX2DFLOAT4 box2;

	POSTGIS_DEBUG(2, "GIST: LWGEOM_right --entry");

	error_if_srid_mismatch(pglwgeom_get_srid(lwgeom1), pglwgeom_get_srid(lwgeom2));

	if ( ! (pglwgeom_getbox2d_p(lwgeom1, &box1) && pglwgeom_getbox2d_p(lwgeom2, &box2)) )
	{
		PG_FREE_IF_COPY(lwgeom1, 0);
		PG_FREE_IF_COPY(lwgeom2, 1);
		PG_RETURN_BOOL(FALSE);
	}

	result = DatumGetBool(DirectFunctionCall2(BOX2D_right,
	                      PointerGetDatum(&box1), PointerGetDatum(&box2)));

	PG_FREE_IF_COPY(lwgeom1, 0);
	PG_FREE_IF_COPY(lwgeom2, 1);

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(LWGEOM_overright);
Datum LWGEOM_overright(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *lwgeom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	bool result;
	BOX2DFLOAT4 box1;
	BOX2DFLOAT4 box2;

	POSTGIS_DEBUG(2, "GIST: LWGEOM_overright --entry");

	error_if_srid_mismatch(pglwgeom_get_srid(lwgeom1), pglwgeom_get_srid(lwgeom2));

	if ( ! (pglwgeom_getbox2d_p(lwgeom1, &box1) && pglwgeom_getbox2d_p(lwgeom2, &box2)) )
	{
		PG_FREE_IF_COPY(lwgeom1, 0);
		PG_FREE_IF_COPY(lwgeom2, 1);
		PG_RETURN_BOOL(FALSE);
	}

	result = DatumGetBool(DirectFunctionCall2(BOX2D_overright,
	                      PointerGetDatum(&box1), PointerGetDatum(&box2)));

	PG_FREE_IF_COPY(lwgeom1, 0);
	PG_FREE_IF_COPY(lwgeom2, 1);

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(LWGEOM_overbelow);
Datum LWGEOM_overbelow(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *lwgeom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	bool result;
	BOX2DFLOAT4 box1;
	BOX2DFLOAT4 box2;

	POSTGIS_DEBUG(2, "GIST: LWGEOM_overbelow --entry");

	error_if_srid_mismatch(pglwgeom_get_srid(lwgeom1), pglwgeom_get_srid(lwgeom2));

	if ( ! (pglwgeom_getbox2d_p(lwgeom1, &box1) && pglwgeom_getbox2d_p(lwgeom2, &box2)) )
	{
		PG_FREE_IF_COPY(lwgeom1, 0);
		PG_FREE_IF_COPY(lwgeom2, 1);
		PG_RETURN_BOOL(FALSE);
	}


	result = DatumGetBool(DirectFunctionCall2(BOX2D_overbelow,
	                      PointerGetDatum(&box1), PointerGetDatum(&box2)));

	PG_FREE_IF_COPY(lwgeom1, 0);
	PG_FREE_IF_COPY(lwgeom2, 1);

	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(LWGEOM_below);
Datum LWGEOM_below(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *lwgeom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	bool result;
	BOX2DFLOAT4 box1;
	BOX2DFLOAT4 box2;

	POSTGIS_DEBUG(2, "GIST: LWGEOM_below --entry");

	error_if_srid_mismatch(pglwgeom_get_srid(lwgeom1), pglwgeom_get_srid(lwgeom2));

	if ( ! (pglwgeom_getbox2d_p(lwgeom1, &box1) && pglwgeom_getbox2d_p(lwgeom2, &box2)) )
	{
		PG_FREE_IF_COPY(lwgeom1, 0);
		PG_FREE_IF_COPY(lwgeom2, 1);
		PG_RETURN_BOOL(FALSE);
	}

	result = DatumGetBool(DirectFunctionCall2(BOX2D_below,
	                      PointerGetDatum(&box1), PointerGetDatum(&box2)));

	PG_FREE_IF_COPY(lwgeom1, 0);
	PG_FREE_IF_COPY(lwgeom2, 1);

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(LWGEOM_above);
Datum LWGEOM_above(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *lwgeom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	bool result;
	BOX2DFLOAT4 box1;
	BOX2DFLOAT4 box2;

	POSTGIS_DEBUG(2, "GIST: LWGEOM_above --entry");

	error_if_srid_mismatch(pglwgeom_get_srid(lwgeom1), pglwgeom_get_srid(lwgeom2));

	if ( ! (pglwgeom_getbox2d_p(lwgeom1, &box1) && pglwgeom_getbox2d_p(lwgeom2, &box2)) )
	{
		PG_FREE_IF_COPY(lwgeom1, 0);
		PG_FREE_IF_COPY(lwgeom2, 1);
		PG_RETURN_BOOL(FALSE);
	}

	result = DatumGetBool(DirectFunctionCall2(BOX2D_above,
	                      PointerGetDatum(&box1), PointerGetDatum(&box2)));

	PG_FREE_IF_COPY(lwgeom1, 0);
	PG_FREE_IF_COPY(lwgeom2, 1);

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(LWGEOM_overabove);
Datum LWGEOM_overabove(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *lwgeom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	bool result;
	BOX2DFLOAT4 box1;
	BOX2DFLOAT4 box2;

	POSTGIS_DEBUG(2, "GIST: LWGEOM_overabove --entry");

	error_if_srid_mismatch(pglwgeom_get_srid(lwgeom1), pglwgeom_get_srid(lwgeom2));

	if ( ! (pglwgeom_getbox2d_p(lwgeom1, &box1) && pglwgeom_getbox2d_p(lwgeom2, &box2)) )
	{
		PG_FREE_IF_COPY(lwgeom1, 0);
		PG_FREE_IF_COPY(lwgeom2, 1);
		PG_RETURN_BOOL(FALSE);
	}

	result = DatumGetBool(DirectFunctionCall2(BOX2D_overabove,
	                      PointerGetDatum(&box1), PointerGetDatum(&box2)));

	PG_FREE_IF_COPY(lwgeom1, 0);
	PG_FREE_IF_COPY(lwgeom2, 1);

	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(LWGEOM_samebox);
Datum LWGEOM_samebox(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *lwgeom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	bool result;
	BOX2DFLOAT4 box1;
	BOX2DFLOAT4 box2;

	POSTGIS_DEBUG(2, "GIST: LWGEOM_samebox --entry");

	error_if_srid_mismatch(pglwgeom_get_srid(lwgeom1), pglwgeom_get_srid(lwgeom2));

	if ( ! (pglwgeom_getbox2d_p(lwgeom1, &box1) && pglwgeom_getbox2d_p(lwgeom2, &box2)) )
	{
		PG_FREE_IF_COPY(lwgeom1, 0);
		PG_FREE_IF_COPY(lwgeom2, 1);
		PG_RETURN_BOOL(FALSE);
	}

	result = DatumGetBool(DirectFunctionCall2(BOX2D_same,
	                      PointerGetDatum(&box1), PointerGetDatum(&box2)));

	PG_FREE_IF_COPY(lwgeom1, 0);
	PG_FREE_IF_COPY(lwgeom2, 1);

	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(LWGEOM_contained);
Datum LWGEOM_contained(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *lwgeom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	bool result;
	BOX2DFLOAT4 box1;
	BOX2DFLOAT4 box2;

	POSTGIS_DEBUG(2, "GIST: LWGEOM_contained --entry");

	error_if_srid_mismatch(pglwgeom_get_srid(lwgeom1), pglwgeom_get_srid(lwgeom2));

	if ( ! (pglwgeom_getbox2d_p(lwgeom1, &box1) && pglwgeom_getbox2d_p(lwgeom2, &box2)) )
	{
		PG_FREE_IF_COPY(lwgeom1, 0);
		PG_FREE_IF_COPY(lwgeom2, 1);
		PG_RETURN_BOOL(FALSE);
	}

	result = DatumGetBool(DirectFunctionCall2(BOX2D_contained,
	                      PointerGetDatum(&box1), PointerGetDatum(&box2)));

	PG_FREE_IF_COPY(lwgeom1, 0);
	PG_FREE_IF_COPY(lwgeom2, 1);

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(LWGEOM_contain);
Datum LWGEOM_contain(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *lwgeom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	bool result;
	BOX2DFLOAT4 box1;
	BOX2DFLOAT4 box2;

	POSTGIS_DEBUG(2, "GIST: LWGEOM_contain --entry");

	error_if_srid_mismatch(pglwgeom_get_srid(lwgeom1), pglwgeom_get_srid(lwgeom2));

	if ( ! (pglwgeom_getbox2d_p(lwgeom1, &box1) && pglwgeom_getbox2d_p(lwgeom2, &box2)) )
	{
		PG_FREE_IF_COPY(lwgeom1, 0);
		PG_FREE_IF_COPY(lwgeom2, 1);
		PG_RETURN_BOOL(FALSE);
	}

	result = DatumGetBool(DirectFunctionCall2(BOX2D_contain,
	                      PointerGetDatum(&box1), PointerGetDatum(&box2)));

	PG_FREE_IF_COPY(lwgeom1, 0);
	PG_FREE_IF_COPY(lwgeom2, 1);

	PG_RETURN_BOOL(result);
}


/* These functions are taken from the postgis_gist_72.c file */


/*
 * GiST Compress methods for geometry
 */
PG_FUNCTION_INFO_V1(LWGEOM_gist_compress);
Datum LWGEOM_gist_compress(PG_FUNCTION_ARGS)
{
	GISTENTRY *entry=(GISTENTRY*)PG_GETARG_POINTER(0);
	GISTENTRY *retval;

	PG_LWGEOM *in; /* lwgeom serialized */
	BOX2DFLOAT4 *rr;

	POSTGIS_DEBUG(2, "GIST: LWGEOM_gist_compress called");

	if (entry->leafkey)
	{
		POSTGIS_DEBUG(4, "GIST: LWGEOM_gist_compress got a leafkey");

		retval = palloc(sizeof(GISTENTRY));
		if ( DatumGetPointer(entry->key) != NULL )
		{
			POSTGIS_DEBUG(4, "GIST: LWGEOM_gist_compress got a non-NULL key");

			/* lwgeom serialized form */
			in = (PG_LWGEOM*)PG_DETOAST_DATUM(entry->key);

			if (in == NULL)
			{
				elog(ERROR, "PG_DETOAST_DATUM(<notnull>) returned NULL ??");
				PG_RETURN_POINTER(entry);
			}

			rr = (BOX2DFLOAT4*) palloc(sizeof(BOX2DFLOAT4));

			if (    ! pglwgeom_getbox2d_p(in, rr) ||
			        ! finite(rr->xmin) ||
			        ! finite(rr->ymin) ||
			        ! finite(rr->xmax) ||
			        ! finite(rr->ymax) )
			{

				POSTGIS_DEBUG(4, "found empty or infinite geometry");

				pfree(rr);
				if (in!=(PG_LWGEOM*)DatumGetPointer(entry->key))
					pfree(in);  /* PG_FREE_IF_COPY */
				PG_RETURN_POINTER(entry);
			}

			POSTGIS_DEBUG(4, "GIST: LWGEOM_gist_compress got box2d");

			POSTGIS_DEBUGF(3, "GIST: LWGEOM_gist_compress -- got box2d BOX(%.15g %.15g,%.15g %.15g)", rr->xmin, rr->ymin, rr->xmax, rr->ymax);

			if (in != (PG_LWGEOM*)DatumGetPointer(entry->key))
				pfree(in);  /* PG_FREE_IF_COPY */

			gistentryinit(*retval, PointerGetDatum(rr),
			              entry->rel, entry->page,
			              entry->offset,
			              FALSE);


		}
		else
		{
			POSTGIS_DEBUG(4, "GIST: LWGEOM_gist_compress got a NULL key");

			gistentryinit(*retval, (Datum) 0, entry->rel,
			              entry->page, entry->offset, FALSE);

		}

	}
	else
	{
		POSTGIS_DEBUG(4, "GIST: LWGEOM_gist_compress got a non-leafkey");

		retval = entry;
	}

	PG_RETURN_POINTER(retval);
}


PG_FUNCTION_INFO_V1(LWGEOM_gist_consistent);
Datum LWGEOM_gist_consistent(PG_FUNCTION_ARGS)
{
	GISTENTRY *entry = (GISTENTRY*) PG_GETARG_POINTER(0);
	PG_LWGEOM *query ; /* lwgeom serialized form */
	StrategyNumber strategy = (StrategyNumber) PG_GETARG_UINT16(2);
	bool result;
	BOX2DFLOAT4  box;

#if POSTGIS_PGSQL_VERSION >= 84
	/* PostgreSQL 8.4 and later require the RECHECK flag to be set here,
	   rather than being supplied as part of the operator class definition */
	bool *recheck = (bool *) PG_GETARG_POINTER(4);

	/* We set recheck to false to avoid repeatedly pulling every "possibly matched" geometry
	   out during index scans. For cases when the geometries are large, rechecking
	   can make things twice as slow. */
	*recheck = false;

#endif

	POSTGIS_DEBUG(2, "GIST: LWGEOM_gist_consistent called");

	if ( ((Pointer *) PG_GETARG_DATUM(1)) == NULL )
	{
		/*elog(NOTICE,"LWGEOM_gist_consistent:: got null query!"); */
		PG_RETURN_BOOL(false); /* null query - this is screwy! */
	}

	/*
	** First pull only a small amount of the tuple, enough to
	** get the bounding box, if one exists.
	** size = header + type + box2df4?
	*/
	query = (PG_LWGEOM*)PG_DETOAST_DATUM_SLICE(PG_GETARG_DATUM(1), 0, VARHDRSZ + 1 + sizeof(BOX2DFLOAT4) );

	if ( ! (DatumGetPointer(entry->key) != NULL && query) )
	{
		PG_FREE_IF_COPY(query, 1);
		elog(ERROR, "LWGEOM_gist_consistent got either NULL query or entry->key");
		PG_RETURN_BOOL(FALSE);
	}

	/*
	** If the bounding box exists, copy it into the working variable.
	** If not, pull the full toasted data out, and call the standard box
	** retrieval function, which will calculate the box from scratch.
	*/
	if ( pglwgeom_has_bbox(query) )
	{
		pglwgeom_getbox2d_p(query, &box);
	}
	else
	{
		query = (PG_LWGEOM*)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
		if ( ! pglwgeom_getbox2d_p(query, &box) )
		{
			PG_FREE_IF_COPY(query, 1);
			PG_RETURN_BOOL(FALSE);
		}
	}

	if (GIST_LEAF(entry))
		result = lwgeom_rtree_leaf_consistent((BOX2DFLOAT4 *)
		                                      DatumGetPointer(entry->key), &box, strategy );
	else
		result = lwgeom_rtree_internal_consistent(
		             (BOX2DFLOAT4 *) DatumGetPointer(entry->key),
		             &box, strategy );

	PG_FREE_IF_COPY(query, 1);
	PG_RETURN_BOOL(result);
}


static bool
lwgeom_rtree_internal_consistent(BOX2DFLOAT4 *key, BOX2DFLOAT4 *query,
                                 StrategyNumber strategy)
{
	bool retval;

	POSTGIS_DEBUGF(2, "GIST: lwgeom_rtree_internal_consistent called with strategy=%i", strategy);

	switch (strategy)
	{
	case RTLeftStrategyNumber:
		retval = !DatumGetBool( DirectFunctionCall2( BOX2D_overright, PointerGetDatum(key), PointerGetDatum(query) ) );
		break;
	case RTOverLeftStrategyNumber:
		retval = !DatumGetBool( DirectFunctionCall2( BOX2D_right, PointerGetDatum(key), PointerGetDatum(query) ) );
		break;
	case RTOverlapStrategyNumber:  /*optimized for speed */

		retval = (((key->xmax>= query->xmax) &&
		           (key->xmin <= query->xmax)) ||
		          ((query->xmax>= key->xmax) &&
		           (query->xmin<= key->xmax)))
		         &&
		         (((key->ymax>= query->ymax) &&
		           (key->ymin<= query->ymax)) ||
		          ((query->ymax>= key->ymax) &&
		           (query->ymin<= key->ymax)));


		POSTGIS_DEBUGF(4, "%i:(int)<%.8g %.8g,%.8g %.8g>&&<%.8g %.8g,%.8g %.8g> %i",counter_intern++,key->xmin,key->ymin,key->xmax,key->ymax,
		               query->xmin,query->ymin,query->xmax,query->ymax,   (int) retval);

		return(retval);
		break;

	case RTOverRightStrategyNumber:
		retval = !DatumGetBool( DirectFunctionCall2( BOX2D_left, PointerGetDatum(key), PointerGetDatum(query) ) );
		break;
	case RTRightStrategyNumber:
		retval = !DatumGetBool( DirectFunctionCall2( BOX2D_overleft, PointerGetDatum(key), PointerGetDatum(query) ) );
		break;
	case RTOverBelowStrategyNumber:
		retval = !DatumGetBool( DirectFunctionCall2( BOX2D_above, PointerGetDatum(key), PointerGetDatum(query) ) );
		break;
	case RTBelowStrategyNumber:
		retval = !DatumGetBool( DirectFunctionCall2( BOX2D_overabove, PointerGetDatum(key), PointerGetDatum(query) ) );
		break;
	case RTAboveStrategyNumber:
		retval = !DatumGetBool( DirectFunctionCall2( BOX2D_overbelow, PointerGetDatum(key), PointerGetDatum(query) ) );
		break;
	case RTOverAboveStrategyNumber:
		retval = !DatumGetBool( DirectFunctionCall2( BOX2D_below, PointerGetDatum(key), PointerGetDatum(query) ) );
		break;
	case RTSameStrategyNumber:
	case RTContainsStrategyNumber:
		retval = DatumGetBool( DirectFunctionCall2( BOX2D_contain, PointerGetDatum(key), PointerGetDatum(query) ) );
		break;
	case RTContainedByStrategyNumber:
		retval = DatumGetBool( DirectFunctionCall2( BOX2D_overlap, PointerGetDatum(key), PointerGetDatum(query) ) );
		break;
	default:
		retval = FALSE;
	}
	return(retval);
}


static bool
lwgeom_rtree_leaf_consistent(BOX2DFLOAT4 *key,
                             BOX2DFLOAT4 *query, StrategyNumber strategy)
{
	bool retval;

	POSTGIS_DEBUGF(2, "GIST: rtree_leaf_consist called with strategy=%d",strategy);

	switch (strategy)
	{
	case RTLeftStrategyNumber:
		retval = DatumGetBool(DirectFunctionCall2(BOX2D_left, PointerGetDatum(key), PointerGetDatum(query)));
		break;
	case RTOverLeftStrategyNumber:
		retval = DatumGetBool(DirectFunctionCall2(BOX2D_overleft, PointerGetDatum(key), PointerGetDatum(query)));
		break;
	case RTOverlapStrategyNumber: /*optimized for speed */
		retval = (((key->xmax>= query->xmax) &&
		           (key->xmin <= query->xmax)) ||
		          ((query->xmax>= key->xmax) &&
		           (query->xmin<= key->xmax)))
		         &&
		         (((key->ymax>= query->ymax) &&
		           (key->ymin<= query->ymax)) ||
		          ((query->ymax>= key->ymax) &&
		           (query->ymin<= key->ymax)));

		/*keep track and report info about how many times this is called */
		POSTGIS_DEBUGF(4, "%i:gist test (leaf) <%.6g %.6g,%.6g %.6g> &&  <%.6g %.6g,%.6g %.6g> --> %i",
		               counter_leaf++,key->xmin,key->ymin,key->xmax,key->ymax,
		               query->xmin,query->ymin,query->xmax,query->ymax,   (int) retval);

		return(retval);

		break;
	case RTOverRightStrategyNumber:
		retval = DatumGetBool(DirectFunctionCall2(BOX2D_overright, PointerGetDatum(key), PointerGetDatum(query)));
		break;
	case RTRightStrategyNumber:
		retval = DatumGetBool(DirectFunctionCall2(BOX2D_right, PointerGetDatum(key), PointerGetDatum(query)));
		break;
	case RTOverBelowStrategyNumber:
		retval = DatumGetBool(DirectFunctionCall2(BOX2D_overbelow, PointerGetDatum(key), PointerGetDatum(query)));
		break;
	case RTBelowStrategyNumber:
		retval = DatumGetBool(DirectFunctionCall2(BOX2D_below, PointerGetDatum(key), PointerGetDatum(query)));
		break;
	case RTAboveStrategyNumber:
		retval = DatumGetBool(DirectFunctionCall2(BOX2D_above, PointerGetDatum(key), PointerGetDatum(query)));
		break;
	case RTOverAboveStrategyNumber:
		retval = DatumGetBool(DirectFunctionCall2(BOX2D_overabove, PointerGetDatum(key), PointerGetDatum(query)));
		break;
	case RTSameStrategyNumber:
		retval = DatumGetBool(DirectFunctionCall2(BOX2D_same, PointerGetDatum(key), PointerGetDatum(query)));
		break;
	case RTContainsStrategyNumber:
		retval = DatumGetBool(DirectFunctionCall2(BOX2D_contain, PointerGetDatum(key), PointerGetDatum(query)));
		break;
	case RTContainedByStrategyNumber:
		retval = DatumGetBool(DirectFunctionCall2(BOX2D_contained, PointerGetDatum(key), PointerGetDatum(query)));
		break;
	default:
		retval = FALSE;
	}
	return (retval);
}



PG_FUNCTION_INFO_V1(LWGEOM_gist_decompress);
Datum LWGEOM_gist_decompress(PG_FUNCTION_ARGS)
{
#if POSTGIS_DEBUG_LEVEL >= 4
	static uint32 counter2 = 0;
	counter2++;
	POSTGIS_DEBUGF(4, "GIST: LWGEOM_gist_decompress called %i",counter2);
#endif

	PG_RETURN_POINTER(PG_GETARG_POINTER(0));
}



/**
 * The GiST Union method for boxes
 * returns the minimal bounding box that encloses all the entries in entryvec
 */
PG_FUNCTION_INFO_V1(LWGEOM_gist_union);
Datum LWGEOM_gist_union(PG_FUNCTION_ARGS)
{
	GistEntryVector	*entryvec = (GistEntryVector *) PG_GETARG_POINTER(0);

	int		   *sizep = (int *) PG_GETARG_POINTER(1);
	int			numranges,
	i;
	BOX2DFLOAT4 *cur,
	*pageunion;

	POSTGIS_DEBUG(2, "GIST: LWGEOM_gist_union called\n");

	numranges = entryvec->n;
	cur = (BOX2DFLOAT4 *) DatumGetPointer(entryvec->vector[0].key);

	pageunion = (BOX2DFLOAT4 *) palloc(sizeof(BOX2DFLOAT4));
	memcpy((void *) pageunion, (void *) cur, sizeof(BOX2DFLOAT4));




	for (i = 1; i < numranges; i++)
	{
		cur = (BOX2DFLOAT4*) DatumGetPointer(entryvec->vector[i].key);

		if (pageunion->xmax < cur->xmax)
			pageunion->xmax = cur->xmax;
		if (pageunion->xmin > cur->xmin)
			pageunion->xmin = cur->xmin;
		if (pageunion->ymax < cur->ymax)
			pageunion->ymax = cur->ymax;
		if (pageunion->ymin > cur->ymin)
			pageunion->ymin = cur->ymin;
	}

	*sizep = sizeof(BOX2DFLOAT4);

	POSTGIS_DEBUGF(3, "GIST: gbox_union called with numranges=%i pageunion is: <%.16g %.16g,%.16g %.16g>", numranges,pageunion->xmin, pageunion->ymin, pageunion->xmax, pageunion->ymax);

	PG_RETURN_POINTER(pageunion);
}


/**
 * size of a box is width*height
 * we do this in double precision because width and height can be very very small
 * and it gives screwy results
 */
static float
size_box2d(Datum box2d)
{
	float result;

	POSTGIS_DEBUG(2, "GIST: size_box2d called");

	if (DatumGetPointer(box2d) != NULL)
	{
		BOX2DFLOAT4 *a = (BOX2DFLOAT4*) DatumGetPointer(box2d);

		if ( a == NULL || a->xmax <= a->xmin || a->ymax <= a->ymin)
		{
			result =  (float) 0.0;
		}
		else
		{
			result = (((double) a->xmax)-((double) a->xmin)) * (((double) a->ymax)-((double) a->ymin));
		}
	}
	else result = (float) 0.0;

	POSTGIS_DEBUGF(3, "GIST: size_box2d called - returning %.15g",result);

	return result;
}

/**
 * size of a box is width*height
 * we do this in double precision because width and height can be very very small
 * and it gives screwy results
 * this version returns a double
 */
static double size_box2d_double(Datum box2d);
static double size_box2d_double(Datum box2d)
{
	double result;

	POSTGIS_DEBUG(2, "GIST: size_box2d_double called");

	if (DatumGetPointer(box2d) != NULL)
	{
		BOX2DFLOAT4 *a = (BOX2DFLOAT4*) DatumGetPointer(box2d);

		if (a == (BOX2DFLOAT4 *) NULL || a->xmax <= a->xmin || a->ymax <= a->ymin)
		{
			result =  (double) 0.0;
		}
		else
		{
			result = (((double) a->xmax)-((double) a->xmin)) * (((double) a->ymax)-((double) a->ymin));
		}
	}
	else
	{
		result = (double) 0.0;
	}

	POSTGIS_DEBUGF(3, "GIST: size_box2d_double called - returning %.15g",result);

	return result;
}



/**
** The GiST Penalty method for boxes
** As in the R-tree paper, we use change in area as our penalty metric
*/
PG_FUNCTION_INFO_V1(LWGEOM_gist_penalty);
Datum LWGEOM_gist_penalty(PG_FUNCTION_ARGS)
{
	GISTENTRY  *origentry = (GISTENTRY *) PG_GETARG_POINTER(0);
	GISTENTRY  *newentry = (GISTENTRY *) PG_GETARG_POINTER(1);
	float	   *result = (float *) PG_GETARG_POINTER(2);
	Datum		ud;
	double		tmp1;

	POSTGIS_DEBUG(2, "GIST: LWGEOM_gist_penalty called");

	if (DatumGetPointer(origentry->key) == NULL && DatumGetPointer(newentry->key) == NULL)
	{
		POSTGIS_DEBUG(4, "GIST: LWGEOM_gist_penalty called with both inputs NULL");

		*result = 0;
	}
	else
	{
		ud = DirectFunctionCall2(BOX2D_union, origentry->key, newentry->key);
		/*ud = BOX2D_union(origentry->key, newentry->key); */
		tmp1 = size_box2d_double(ud);
		if (DatumGetPointer(ud) != NULL)
			pfree(DatumGetPointer(ud));
		*result = tmp1 - size_box2d_double(origentry->key);
	}


	POSTGIS_DEBUGF(4, "GIST: LWGEOM_gist_penalty called and returning %.15g", *result);

	PG_RETURN_POINTER(result);
}

typedef struct
{
	BOX2DFLOAT4 	*key;
	int 	pos;
}
KBsort;

static int
compare_KB(const void* a, const void* b)
{
	BOX2DFLOAT4 *abox = ((KBsort*)a)->key;
	BOX2DFLOAT4 *bbox = ((KBsort*)b)->key;
	float sa = (abox->xmax - abox->xmin) * (abox->ymax - abox->ymin);
	float sb = (bbox->xmax - bbox->xmin) * (bbox->ymax - bbox->ymin);

	if ( sa==sb ) return 0;
	return ( sa>sb ) ? 1 : -1;
}

/**
** Equality method
*/
PG_FUNCTION_INFO_V1(LWGEOM_gist_same);
Datum LWGEOM_gist_same(PG_FUNCTION_ARGS)
{
	BOX2DFLOAT4 *b1 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(0);
	BOX2DFLOAT4 *b2 = (BOX2DFLOAT4 *) PG_GETARG_POINTER(1);
	bool *result = (bool *)PG_GETARG_POINTER(2);

	POSTGIS_DEBUG(2, "GIST: LWGEOM_gist_same called");

	if (b1 && b2)
		*result = DatumGetBool(DirectFunctionCall2(BOX2D_same, PointerGetDatum(b1), PointerGetDatum(b2)));
	else
		*result = (b1 == NULL && b2 == NULL) ? TRUE : FALSE;
	PG_RETURN_POINTER(result);
}



/**
** The GiST PickSplit method
** New linear algorithm, see 'New Linear Node Splitting Algorithm for R-tree',
** C.H.Ang and T.C.Tan
*/
PG_FUNCTION_INFO_V1(LWGEOM_gist_picksplit);
Datum LWGEOM_gist_picksplit(PG_FUNCTION_ARGS)
{
	GistEntryVector	*entryvec = (GistEntryVector *) PG_GETARG_POINTER(0);

	GIST_SPLITVEC *v = (GIST_SPLITVEC *) PG_GETARG_POINTER(1);
	OffsetNumber i;
	OffsetNumber *listL, *listR, *listB, *listT;
	BOX2DFLOAT4 *unionL, *unionR, *unionB, *unionT;
	int posL, posR, posB, posT;
	BOX2DFLOAT4 pageunion;
	BOX2DFLOAT4 *cur;
	char direction = ' ';
	bool allisequal = true;
	OffsetNumber maxoff;
	int nbytes;

	POSTGIS_DEBUG(2, "GIST: LWGEOM_gist_picksplit called");

	posL = posR = posB = posT = 0;

	maxoff = entryvec->n - 1;
	cur = (BOX2DFLOAT4*) DatumGetPointer(entryvec->vector[FirstOffsetNumber].key);

	memcpy((void *) &pageunion, (void *) cur, sizeof(BOX2DFLOAT4));

	POSTGIS_DEBUGF(4, "   cur is: <%.16g %.16g,%.16g %.16g>", cur->xmin, cur->ymin, cur->xmax, cur->ymax);


	/* find MBR */
	for (i = OffsetNumberNext(FirstOffsetNumber); i <= maxoff; i = OffsetNumberNext(i))
	{
		cur = (BOX2DFLOAT4 *) DatumGetPointer(entryvec->vector[i].key);

		if ( allisequal == true &&  (
		            pageunion.xmax != cur->xmax ||
		            pageunion.ymax != cur->ymax ||
		            pageunion.xmin != cur->xmin ||
		            pageunion.ymin != cur->ymin
		        ) )
			allisequal = false;

		if (pageunion.xmax < cur->xmax)
			pageunion.xmax = cur->xmax;
		if (pageunion.xmin > cur->xmin)
			pageunion.xmin = cur->xmin;
		if (pageunion.ymax < cur->ymax)
			pageunion.ymax = cur->ymax;
		if (pageunion.ymin> cur->ymin)
			pageunion.ymin = cur->ymin;
	}

	POSTGIS_DEBUGF(4, "   pageunion is: <%.16g %.16g,%.16g %.16g>", pageunion.xmin, pageunion.ymin, pageunion.xmax, pageunion.ymax);

	nbytes = (maxoff + 2) * sizeof(OffsetNumber);
	listL = (OffsetNumber *) palloc(nbytes);
	listR = (OffsetNumber *) palloc(nbytes);
	unionL = (BOX2DFLOAT4 *) palloc(sizeof(BOX2DFLOAT4));
	unionR = (BOX2DFLOAT4 *) palloc(sizeof(BOX2DFLOAT4));

	if (allisequal)
	{
		POSTGIS_DEBUG(4, " AllIsEqual!");

		cur = (BOX2DFLOAT4*) DatumGetPointer(entryvec->vector[OffsetNumberNext(FirstOffsetNumber)].key);


		if (memcmp((void *) cur, (void *) &pageunion, sizeof(BOX2DFLOAT4)) == 0)
		{
			v->spl_left = listL;
			v->spl_right = listR;
			v->spl_nleft = v->spl_nright = 0;
			memcpy((void *) unionL, (void *) &pageunion, sizeof(BOX2DFLOAT4));
			memcpy((void *) unionR, (void *) &pageunion, sizeof(BOX2DFLOAT4));

			for (i = FirstOffsetNumber; i <= maxoff; i = OffsetNumberNext(i))
			{
				if (i <= (maxoff - FirstOffsetNumber + 1) / 2)
				{
					v->spl_left[v->spl_nleft] = i;
					v->spl_nleft++;
				}
				else
				{
					v->spl_right[v->spl_nright] = i;
					v->spl_nright++;
				}
			}
			v->spl_ldatum = PointerGetDatum(unionL);
			v->spl_rdatum = PointerGetDatum(unionR);

			PG_RETURN_POINTER(v);
		}
	}


	listB = (OffsetNumber *) palloc(nbytes);
	listT = (OffsetNumber *) palloc(nbytes);
	unionB = (BOX2DFLOAT4 *) palloc(sizeof(BOX2DFLOAT4));
	unionT = (BOX2DFLOAT4 *) palloc(sizeof(BOX2DFLOAT4));

#define ADDLIST( list, unionD, pos, num ) do { \
	if ( pos ) { \
		if ( unionD->xmax < cur->xmax )    unionD->xmax	= cur->xmax; \
		if ( unionD->xmin	> cur->xmin  ) unionD->xmin	= cur->xmin; \
		if ( unionD->ymax < cur->ymax )    unionD->ymax	= cur->ymax; \
		if ( unionD->ymin	> cur->ymin  ) unionD->ymin	= cur->ymin; \
	} else { \
			memcpy( (void*)unionD, (void*) cur, sizeof( BOX2DFLOAT4 ) );  \
	} \
	list[pos] = num; \
	(pos)++; \
} while(0)

	for (i = FirstOffsetNumber; i <= maxoff; i = OffsetNumberNext(i))
	{
		cur = (BOX2DFLOAT4*) DatumGetPointer(entryvec->vector[i].key);

		if (cur->xmin - pageunion.xmin < pageunion.xmax - cur->xmax)
			ADDLIST(listL, unionL, posL,i);
		else
			ADDLIST(listR, unionR, posR,i);
		if (cur->ymin - pageunion.ymin < pageunion.ymax - cur->ymax)
			ADDLIST(listB, unionB, posB,i);
		else
			ADDLIST(listT, unionT, posT,i);
	}

	POSTGIS_DEBUGF(4, "   unionL is: <%.16g %.16g,%.16g %.16g>", unionL->xmin, unionL->ymin, unionL->xmax, unionL->ymax);
	POSTGIS_DEBUGF(4, "   unionR is: <%.16g %.16g,%.16g %.16g>", unionR->xmin, unionR->ymin, unionR->xmax, unionR->ymax);
	POSTGIS_DEBUGF(4, "   unionT is: <%.16g %.16g,%.16g %.16g>", unionT->xmin, unionT->ymin, unionT->xmax, unionT->ymax);
	POSTGIS_DEBUGF(4, "   unionB is: <%.16g %.16g,%.16g %.16g>", unionB->xmin, unionB->ymin, unionB->xmax, unionB->ymax);



	/* bad disposition, sort by ascending and resplit */
	if ( (posR==0 || posL==0) && (posT==0 || posB==0) )
	{
		KBsort *arr = (KBsort*)palloc( sizeof(KBsort) * maxoff );
		posL = posR = posB = posT = 0;
		for (i = FirstOffsetNumber; i <= maxoff; i = OffsetNumberNext(i))
		{
			arr[i-1].key = (BOX2DFLOAT4*) DatumGetPointer(entryvec->vector[i].key);
			arr[i-1].pos = i;
		}
		qsort( arr, maxoff, sizeof(KBsort), compare_KB );
		for (i = FirstOffsetNumber; i <= maxoff; i = OffsetNumberNext(i))
		{
			cur = arr[i-1].key;
			if (cur->xmin - pageunion.xmin < pageunion.xmax - cur->xmax)
				ADDLIST(listL, unionL, posL,arr[i-1].pos);
			else if ( cur->xmin - pageunion.xmin == pageunion.xmax - cur->xmax )
			{
				if ( posL>posR )
					ADDLIST(listR, unionR, posR,arr[i-1].pos);
				else
					ADDLIST(listL, unionL, posL,arr[i-1].pos);
			}
			else
				ADDLIST(listR, unionR, posR,arr[i-1].pos);

			if (cur->ymin - pageunion.ymin < pageunion.ymax - cur->ymax)
				ADDLIST(listB, unionB, posB,arr[i-1].pos);
			else if ( cur->ymin - pageunion.ymin == pageunion.ymax - cur->ymax )
			{
				if ( posB>posT )
					ADDLIST(listT, unionT, posT,arr[i-1].pos);
				else
					ADDLIST(listB, unionB, posB,arr[i-1].pos);
			}
			else
				ADDLIST(listT, unionT, posT,arr[i-1].pos);
		}
		pfree(arr);
	}

	/* which split more optimal? */
	if (Max(posL, posR) < Max(posB, posT))
		direction = 'x';
	else if (Max(posL, posR) > Max(posB, posT))
		direction = 'y';
	else
	{
		Datum interLR = DirectFunctionCall2(BOX2D_intersects,
		                                    PointerGetDatum(unionL), PointerGetDatum(unionR));
		Datum interBT = DirectFunctionCall2(BOX2D_intersects,
		                                    PointerGetDatum(unionB), PointerGetDatum(unionT));
		float sizeLR, sizeBT;

		/*elog(NOTICE,"direction is ambiguous"); */

		sizeLR = size_box2d(interLR);
		sizeBT = size_box2d(interBT);

		if ( DatumGetPointer(interLR) != NULL )
			pfree(DatumGetPointer(interLR));
		if ( DatumGetPointer(interBT) != NULL )
			pfree(DatumGetPointer(interBT));

		if (sizeLR < sizeBT)
			direction = 'x';
		else
			direction = 'y';
	}

	if (direction == 'x')
	{
		pfree(unionB);
		pfree(listB);
		pfree(unionT);
		pfree(listT);

		v->spl_left = listL;
		v->spl_right = listR;
		v->spl_nleft = posL;
		v->spl_nright = posR;
		v->spl_ldatum = PointerGetDatum(unionL);
		v->spl_rdatum = PointerGetDatum(unionR);

#if POSTGIS_DEBUG_LEVEL >= 4
		{
			char aaa[5000],bbb[100];
			aaa[0] = 0;

			POSTGIS_DEBUGF(4, "   split direction was '%c'", direction);
			POSTGIS_DEBUGF(4, "   posL = %i, posR=%i", posL,posR);
			POSTGIS_DEBUG(4, "   posL's (nleft) offset numbers:");

			for (i=0; i<posL; i++)
			{
				sprintf(bbb," %i", listL[i]);
				strcat(aaa,bbb);
			}
			POSTGIS_DEBUGF(4, "%s", aaa);
			aaa[0]=0;
			POSTGIS_DEBUG(4, "   posR's (nright) offset numbers:");
			for (i=0; i<posR; i++)
			{
				sprintf(bbb," %i", listR[i]);
				strcat(aaa,bbb);
			}
			POSTGIS_DEBUGF(4, "%s", aaa);
		}
#endif


	}
	else
	{
		pfree(unionR);
		pfree(listR);
		pfree(unionL);
		pfree(listL);

		v->spl_left = listB;
		v->spl_right = listT;
		v->spl_nleft = posB;
		v->spl_nright = posT;
		v->spl_ldatum = PointerGetDatum(unionB);
		v->spl_rdatum = PointerGetDatum(unionT);

#if POSTGIS_DEBUG_LEVEL >= 4
		{
			char aaa[5000],bbb[100];
			aaa[0]=0;

			POSTGIS_DEBUGF(4, "   split direction was '%c'", direction);
			POSTGIS_DEBUGF(4, "   posB = %i, posT=%i", posB,posT);
			POSTGIS_DEBUG(4, "   posB's (nleft) offset numbers:");

			for (i=0; i<posB; i++)
			{
				sprintf(bbb," %i", listB[i]);
				strcat(aaa,bbb);
			}
			POSTGIS_DEBUGF(4, "%s", aaa);
			aaa[0]=0;
			POSTGIS_DEBUG(4, "   posT's (nright) offset numbers:");
			for (i=0; i<posT; i++)
			{
				sprintf(bbb," %i", listT[i]);
				strcat(aaa,bbb);
			}
			POSTGIS_DEBUGF(4, "%s", aaa);
		}
#endif


	}
	PG_RETURN_POINTER(v);
}


/** debug function */
Datum report_lwgeom_gist_activity(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(report_lwgeom_gist_activity);
Datum report_lwgeom_gist_activity(PG_FUNCTION_ARGS)
{
	elog(NOTICE,"lwgeom gist activity - internal consistency= %i, leaf consistency = %i",counter_intern,counter_leaf);
	counter_intern =0;  /*reset */
	counter_leaf = 0;
	PG_RETURN_NULL();
}



