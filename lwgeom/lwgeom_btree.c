/***********************************************************
 *
 * $Id$
 *
 * Comparision function for use in Binary Tree searches
 * (ORDER BY, GROUP BY, DISTINCT)
 *
 ***********************************************************/

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "postgres.h"
#include "fmgr.h"

#include "lwgeom.h"

Datum lwgeom_lt(PG_FUNCTION_ARGS);
Datum lwgeom_le(PG_FUNCTION_ARGS);
Datum lwgeom_eq(PG_FUNCTION_ARGS);
Datum lwgeom_ge(PG_FUNCTION_ARGS);
Datum lwgeom_gt(PG_FUNCTION_ARGS);
Datum lwgeom_cmp(PG_FUNCTION_ARGS);

//#define DEBUG

PG_FUNCTION_INFO_V1(lwgeom_lt);
Datum lwgeom_lt(PG_FUNCTION_ARGS)
{
	LWGEOM *geom1 = (LWGEOM *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *geom2 = (LWGEOM *) PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	BOX2DFLOAT4 box1;
	BOX2DFLOAT4 box2;

#ifdef DEBUG
	elog(NOTICE, "lwgeom_lt called");
#endif

	if (lwgeom_getSRID(geom1) != lwgeom_getSRID(geom2))
	{
		elog(ERROR,
			"Operation on two GEOMETRIES with different SRIDs\n");
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 )
			pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 )
			pfree(geom2);
		PG_RETURN_NULL();
	}

#ifdef DEBUG
	elog(NOTICE, "lwgeom_lt passed getSRID test");
#endif

	getbox2d_p(SERIALIZED_FORM(geom1), &box1);
	getbox2d_p(SERIALIZED_FORM(geom2), &box2);

#ifdef DEBUG
	elog(NOTICE, "lwgeom_lt getbox2d_p passed");
#endif

	if  ( ! FPeq(box1.xmin , box2.xmin) ) {
		if  (box1.xmin < box2.xmin)
			PG_RETURN_BOOL(TRUE);
	}

	if  ( ! FPeq(box1.ymin , box2.ymin) ) {
		if  (box1.ymin < box2.ymin)
			PG_RETURN_BOOL(TRUE);
	}

	if  ( ! FPeq(box1.xmax , box2.xmax) ) {
		if  (box1.xmax < box2.xmax)
			PG_RETURN_BOOL(TRUE);
	}

	if  ( ! FPeq(box1.ymax , box2.ymax) ) {
		if  (box1.ymax < box2.ymax)
			PG_RETURN_BOOL(TRUE);
	}

	PG_RETURN_BOOL(FALSE);
}

PG_FUNCTION_INFO_V1(lwgeom_le);
Datum lwgeom_le(PG_FUNCTION_ARGS)
{
	LWGEOM *geom1 = (LWGEOM *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *geom2 = (LWGEOM *) PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	BOX2DFLOAT4 box1;
	BOX2DFLOAT4 box2;

#ifdef DEBUG
	elog(NOTICE, "lwgeom_le called");
#endif

	if (lwgeom_getSRID(geom1) != lwgeom_getSRID(geom2))
	{
		elog(ERROR,
			"Operation on two GEOMETRIES with different SRIDs\n");
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 )
			pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 )
			pfree(geom2);
		PG_RETURN_NULL();
	}

	getbox2d_p(SERIALIZED_FORM(geom1), &box1);
	getbox2d_p(SERIALIZED_FORM(geom2), &box2);

	if  ( ! FPeq(box1.xmin , box2.xmin) ) {
		if  (box1.xmin < box2.xmin)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_BOOL(TRUE);
		}
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
		PG_RETURN_BOOL(FALSE);
	}

	if  ( ! FPeq(box1.ymin , box2.ymin) ) {
		if  (box1.ymin < box2.ymin)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_BOOL(TRUE);
		}
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
		PG_RETURN_BOOL(FALSE);
	}

	if  ( ! FPeq(box1.xmax , box2.xmax) ) {
		if  (box1.xmax < box2.xmax)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_BOOL(TRUE);
		}
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
		PG_RETURN_BOOL(FALSE);
	}

	if  ( ! FPeq(box1.ymax , box2.ymax) ) {
		if  (box1.ymax < box2.ymax)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_BOOL(TRUE);
		}
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
		PG_RETURN_BOOL(FALSE);
	}

	if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
	if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);

	PG_RETURN_BOOL(TRUE);
}

PG_FUNCTION_INFO_V1(lwgeom_eq);
Datum lwgeom_eq(PG_FUNCTION_ARGS)
{
	LWGEOM *geom1 = (LWGEOM *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *geom2 = (LWGEOM *) PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	BOX2DFLOAT4 box1;
	BOX2DFLOAT4 box2;

#ifdef DEBUG
	elog(NOTICE, "lwgeom_eq called");
#endif

	if (lwgeom_getSRID(geom1) != lwgeom_getSRID(geom2))
	{
		elog(ERROR,
			"Operation on two GEOMETRIES with different SRIDs\n");
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 )
			pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 )
			pfree(geom2);
		PG_RETURN_NULL();
	}

	getbox2d_p(SERIALIZED_FORM(geom1), &box1);
	getbox2d_p(SERIALIZED_FORM(geom2), &box2);

	if  ( ! FPeq(box1.xmin , box2.xmin) ) 
	{
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
		PG_RETURN_BOOL(FALSE);
	}

	if  ( ! FPeq(box1.ymin , box2.ymin) ) 
	{
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
		PG_RETURN_BOOL(FALSE);
	}

	if  ( ! FPeq(box1.xmax , box2.xmax) ) 
	{
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
		PG_RETURN_BOOL(FALSE);
	}

	if  ( ! FPeq(box1.ymax , box2.ymax) )
	{
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
		PG_RETURN_BOOL(FALSE);
	}

	if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
	if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);

	PG_RETURN_BOOL(TRUE);
}

PG_FUNCTION_INFO_V1(lwgeom_ge);
Datum lwgeom_ge(PG_FUNCTION_ARGS)
{
	LWGEOM *geom1 = (LWGEOM *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *geom2 = (LWGEOM *) PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	BOX2DFLOAT4 box1;
	BOX2DFLOAT4 box2;

#ifdef DEBUG
	elog(NOTICE, "lwgeom_ge called");
#endif

	if (lwgeom_getSRID(geom1) != lwgeom_getSRID(geom2))
	{
		elog(ERROR,
			"Operation on two GEOMETRIES with different SRIDs\n");
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 )
			pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 )
			pfree(geom2);
		PG_RETURN_NULL();
	}

	getbox2d_p(SERIALIZED_FORM(geom1), &box1);
	getbox2d_p(SERIALIZED_FORM(geom2), &box2);

	if  ( ! FPeq(box1.xmin , box2.xmin) ) {
		if  (box1.xmin > box2.xmin)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_BOOL(TRUE);
		}
		PG_RETURN_BOOL(FALSE);
	}

	if  ( ! FPeq(box1.ymin , box2.ymin) ) {
		if  (box1.ymin > box2.ymin)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_BOOL(TRUE);
		}
		PG_RETURN_BOOL(FALSE);
	}

	if  ( ! FPeq(box1.xmax , box2.xmax) ) {
		if  (box1.xmax > box2.xmax)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_BOOL(TRUE);
		}
		PG_RETURN_BOOL(FALSE);
	}

	if  ( ! FPeq(box1.ymax , box2.ymax) ) {
		if  (box1.ymax > box2.ymax)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_BOOL(TRUE);
		}
		PG_RETURN_BOOL(FALSE);
	}

	if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
	if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);

	PG_RETURN_BOOL(TRUE);
}

PG_FUNCTION_INFO_V1(lwgeom_gt);
Datum lwgeom_gt(PG_FUNCTION_ARGS)
{
	LWGEOM *geom1 = (LWGEOM *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *geom2 = (LWGEOM *) PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	BOX2DFLOAT4 box1;
	BOX2DFLOAT4 box2;

#ifdef DEBUG
	elog(NOTICE, "lwgeom_gt called");
#endif

	if (lwgeom_getSRID(geom1) != lwgeom_getSRID(geom2))
	{
		elog(ERROR,
			"Operation on two GEOMETRIES with different SRIDs\n");
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 )
			pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 )
			pfree(geom2);
		PG_RETURN_NULL();
	}

	getbox2d_p(SERIALIZED_FORM(geom1), &box1);
	getbox2d_p(SERIALIZED_FORM(geom2), &box2);

	if  ( ! FPeq(box1.xmin , box2.xmin) ) {
		if  (box1.xmin > box2.xmin)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_BOOL(TRUE);
		}
	}

	if  ( ! FPeq(box1.ymin , box2.ymin) ) {
		if  (box1.ymin > box2.ymin)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_BOOL(TRUE);
		}
	}

	if  ( ! FPeq(box1.xmax , box2.xmax) ) {
		if  (box1.xmax > box2.xmax)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_BOOL(TRUE);
		}
	}

	if  ( ! FPeq(box1.ymax , box2.ymax) ) {
		if  (box1.ymax > box2.ymax)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_BOOL(TRUE);
		}
	}

	if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
	if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);

	PG_RETURN_BOOL(FALSE);
}

PG_FUNCTION_INFO_V1(lwgeom_cmp);
Datum lwgeom_cmp(PG_FUNCTION_ARGS)
{
	LWGEOM *geom1 = (LWGEOM *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *geom2 = (LWGEOM *) PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	BOX2DFLOAT4 box1;
	BOX2DFLOAT4 box2;

#ifdef DEBUG
	elog(NOTICE, "lwgeom_cmp called");
#endif

	if (lwgeom_getSRID(geom1) != lwgeom_getSRID(geom2))
	{
		elog(ERROR,
			"Operation on two GEOMETRIES with different SRIDs\n");
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 )
			pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 )
			pfree(geom2);
		PG_RETURN_NULL();
	}

	getbox2d_p(SERIALIZED_FORM(geom1), &box1);
	getbox2d_p(SERIALIZED_FORM(geom2), &box2);

	if  ( ! FPeq(box1.xmin , box2.xmin) ) {
		if  (box1.xmin < box2.xmin)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_INT32(-1);
		}
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
		PG_RETURN_INT32(1);
	}

	if  ( ! FPeq(box1.ymin , box2.ymin) ) {
		if  (box1.ymin < box2.ymin)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_INT32(-1);
		}
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
		PG_RETURN_INT32(1);
	}

	if  ( ! FPeq(box1.xmax , box2.xmax) ) {
		if  (box1.xmax < box2.xmax)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_INT32(-1);
		}
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
		PG_RETURN_INT32(1);
	}

	if  ( ! FPeq(box1.ymax , box2.ymax) ) {
		if  (box1.ymax < box2.ymax)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_INT32(-1);
		}
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
		PG_RETURN_INT32(1);
	}

	if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
	if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);

	PG_RETURN_INT32(0);
}

/***********************************************************
 *
 * $Log$
 * Revision 1.3  2004/08/20 14:08:41  strk
 * Added Geom{etry,}FromWkb(<geometry>,[<int4>]) funx.
 * Added LWGEOM typedef and SERIALIZED_FORM(LWGEOM) macro.
 * Made lwgeom_setSRID an API function.
 * Added LWGEOM_setAllocator().
 *
 * Revision 1.2  2004/08/19 13:10:13  strk
 * fixed typos
 *
 * (ORDER BY, GROUP BY, DISTINCT)
 *
 ***********************************************************/
