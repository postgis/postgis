/***********************************************************
 *
 * $Id$
 *
 * Comparision function for use in Binary Tree searches
 * (ORDER BY, GROUP BY, DISTINCT)
 *
 ***********************************************************/

#include "postgres.h"

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

//#include "access/gist.h"
//#include "access/itup.h"
//#include "access/rtree.h"

#include "fmgr.h"
//#include "utils/elog.h"

#include "lwgeom.h"

Datum lwgeom_lt(PG_FUNCTION_ARGS);
Datum lwgeom_le(PG_FUNCTION_ARGS);
Datum lwgeom_eq(PG_FUNCTION_ARGS);
Datum lwgeom_ge(PG_FUNCTION_ARGS);
Datum lwgeom_gt(PG_FUNCTION_ARGS);
Datum lwgeom_cmp(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(geometry_lt);
Datum lwgeom_lt(PG_FUNCTION_ARGS)
{
	char *geom1 = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	char *geom2 = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	BOX2DFLOAT4 box1;
	BOX2DFLOAT4 box2;

	//elog(NOTICE, "geometry_lt called");

	if (lwgeom_getSRID(geom1+4) != lwgeom_getSRID(geom2+4))
	{
		elog(ERROR,
			"Operation on two GEOMETRIES with different SRIDs\n");
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 )
			pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 )
			pfree(geom2);
		PG_RETURN_NULL();
	}

	getbox2d_p(geom1+4, &box1);
	getbox2d_p(geom2+4, &box2);

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

PG_FUNCTION_INFO_V1(geometry_le);
Datum lwgeom_le(PG_FUNCTION_ARGS)
{
	char *geom1 = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	char *geom2 = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	BOX2DFLOAT4 box1;
	BOX2DFLOAT4 box2;

	//elog(NOTICE, "geometry_le called");

	if (lwgeom_getSRID(geom1+4) != lwgeom_getSRID(geom2+4))
	{
		elog(ERROR,
			"Operation on two GEOMETRIES with different SRIDs\n");
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 )
			pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 )
			pfree(geom2);
		PG_RETURN_NULL();
	}

	getbox2d_p(geom1+4, &box1);
	getbox2d_p(geom2+4, &box2);

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

PG_FUNCTION_INFO_V1(geometry_eq);
Datum lwgeom_eq(PG_FUNCTION_ARGS)
{
	char *geom1 = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	char *geom2 = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	BOX2DFLOAT4 box1;
	BOX2DFLOAT4 box2;

	//elog(NOTICE, "geometry_eq called");

	if (lwgeom_getSRID(geom1+4) != lwgeom_getSRID(geom2+4))
	{
		elog(ERROR,
			"Operation on two GEOMETRIES with different SRIDs\n");
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 )
			pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 )
			pfree(geom2);
		PG_RETURN_NULL();
	}

	getbox2d_p(geom1+4, &box1);
	getbox2d_p(geom2+4, &box2);

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

PG_FUNCTION_INFO_V1(geometry_ge);
Datum lwgeom_ge(PG_FUNCTION_ARGS)
{
	char *geom1 = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	char *geom2 = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	BOX2DFLOAT4 box1;
	BOX2DFLOAT4 box2;

	//elog(NOTICE, "geometry_ge called");

	if (lwgeom_getSRID(geom1+4) != lwgeom_getSRID(geom2+4))
	{
		elog(ERROR,
			"Operation on two GEOMETRIES with different SRIDs\n");
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 )
			pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 )
			pfree(geom2);
		PG_RETURN_NULL();
	}

	getbox2d_p(geom1+4, &box1);
	getbox2d_p(geom2+4, &box2);

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

PG_FUNCTION_INFO_V1(geometry_gt);
Datum lwgeom_gt(PG_FUNCTION_ARGS)
{
	char *geom1 = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	char *geom2 = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	BOX2DFLOAT4 box1;
	BOX2DFLOAT4 box2;

	//elog(NOTICE, "geometry_gt called");

	if (lwgeom_getSRID(geom1+4) != lwgeom_getSRID(geom2+4))
	{
		elog(ERROR,
			"Operation on two GEOMETRIES with different SRIDs\n");
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 )
			pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 )
			pfree(geom2);
		PG_RETURN_NULL();
	}

	getbox2d_p(geom1+4, &box1);
	getbox2d_p(geom2+4, &box2);

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

PG_FUNCTION_INFO_V1(geometry_cmp);
Datum lwgeom_cmp(PG_FUNCTION_ARGS)
{
	char *geom1 = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	char *geom2 = (char *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	BOX2DFLOAT4 box1;
	BOX2DFLOAT4 box2;

	//elog(NOTICE, "geometry_cmp called");

	if (lwgeom_getSRID(geom1+4) != lwgeom_getSRID(geom2+4))
	{
		elog(ERROR,
			"Operation on two GEOMETRIES with different SRIDs\n");
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 )
			pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 )
			pfree(geom2);
		PG_RETURN_NULL();
	}

	getbox2d_p(geom1+4, &box1);
	getbox2d_p(geom2+4, &box2);

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
 * Revision 1.1  2004/08/19 12:28:30  strk
 * added btree index support
 *
 * (ORDER BY, GROUP BY, DISTINCT)
 *
 ***********************************************************/
