
/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2003 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 * 
 **********************************************************************
 * $Log$
 * Revision 1.13.2.1  2005/06/15 16:01:17  strk
 * fault tolerant btree ops
 *
 * Revision 1.13  2004/04/28 22:26:02  pramsey
 * Fixed spelling mistake in header text.
 *
 * Revision 1.12  2003/11/20 15:34:02  strk
 * expected in-transaction memory release for btree operators
 *
 * Revision 1.11  2003/11/19 15:26:57  strk
 * Added geometry_le, geometry_ge, geometry_cmp functions,
 * modified geometry_lt, geometry_gt, geometry_eq to be consistent.
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

#define SHOW_DIGS_DOUBLE 15
#define MAX_DIGS_DOUBLE (SHOW_DIGS_DOUBLE + 6 + 1 + 3 +1)


//given a bvol, make a "fake" geometry that contains it (for indexing)
GEOMETRY *make_bvol_geometry(BOX3D *box)
{
	return (GEOMETRY *) DatumGetPointer(DirectFunctionCall1(get_geometry_of_bbox,PointerGetDatum(box) ) );
}

//   ***********************************
//          GEOMETRY indexing (operator) fns
//	NOTE:
//		These work on the bbox of the geometry, not the 
//		individual components (except same)
//   ***********************************


PG_FUNCTION_INFO_V1(box3d_same);
Datum box3d_same(PG_FUNCTION_ARGS)
{
	BOX3D		   *box1 = (BOX3D *)  PG_GETARG_POINTER(0);
	BOX3D		   *box2 = (BOX3D *) PG_GETARG_POINTER(1);

	bool		result;

	result = (	   FPeq(box1->LLB.x	, box2->LLB.x) &&
				   FPeq(box1->LLB.y	, box2->LLB.y) &&
				   FPeq(box1->URT.x	, box2->URT.x) &&
				   FPeq(box1->URT.y	, box2->URT.y)
				);
	PG_RETURN_BOOL(result);
}




PG_FUNCTION_INFO_V1(geometry_overleft);
Datum geometry_overleft(PG_FUNCTION_ARGS)
{
	GEOMETRY		   *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		   *geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	bool	result;

//printf("in geometry_overleft\n");
//print_box(&geom1->bvol);
//print_box(&geom2->bvol);

	if (geom1->SRID != geom2->SRID)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}

	result = FPle(geom1->bvol.URT.x, geom2->bvol.URT.x);

	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(geometry_left);
Datum geometry_left(PG_FUNCTION_ARGS)
{
	GEOMETRY		   *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		   *geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	bool	result;

//printf("in geometry_left\n");

	if (geom1->SRID != geom2->SRID)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}


	result = FPlt(geom1->bvol.URT.x, geom2->bvol.LLB.x);


	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(geometry_right);
Datum geometry_right(PG_FUNCTION_ARGS)
{
	GEOMETRY		   *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		   *geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	bool	result;
//printf("in geometry_right\n");

	if (geom1->SRID != geom2->SRID)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}


	result = FPgt(geom1->bvol.LLB.x, geom2->bvol.URT.x);
	
	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(geometry_overright);
Datum geometry_overright(PG_FUNCTION_ARGS)
{
	GEOMETRY		   *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		   *geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	bool	result;
//printf("in geometry_overright\n");

	if (geom1->SRID != geom2->SRID)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}


	result = FPge(geom1->bvol.LLB.x, geom2->bvol.LLB.x);
	


	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(geometry_contained);
Datum geometry_contained(PG_FUNCTION_ARGS)
{
	GEOMETRY		   *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		   *geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	bool	result;

	if (geom1->SRID != geom2->SRID)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}


//printf("in geometry_contained\n");


	result = FPle(geom1->bvol.URT.x, geom2->bvol.URT.x) &&
				   FPge(geom1->bvol.LLB.x, geom2->bvol.LLB.x) &&
				   FPle(geom1->bvol.URT.y, geom2->bvol.URT.y) &&
				   FPge(geom1->bvol.LLB.y, geom2->bvol.LLB.y);
	
//printf("returning geometry_contained\n");
	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(geometry_contain);
Datum geometry_contain(PG_FUNCTION_ARGS)
{
	GEOMETRY		   *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		   *geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	bool	result;
//printf("in geometry_contain\n");

	if (geom1->SRID != geom2->SRID)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}


	result = FPge(geom1->bvol.URT.x, geom2->bvol.URT.x) &&
				   FPle(geom1->bvol.LLB.x, geom2->bvol.LLB.x) &&
				   FPge(geom1->bvol.URT.y, geom2->bvol.URT.y) &&
				   FPle(geom1->bvol.LLB.y, geom2->bvol.LLB.y);
	


	PG_RETURN_BOOL(result);
}

bool box3d_ov(BOX3D *box1, BOX3D *box2)
{
	bool	result;

	result = 

	/*overlap in x*/
		 ((    FPge(box1->URT.x, box2->URT.x) &&
			 FPle(box1->LLB.x, box2->URT.x)) ||
			(FPge(box2->URT.x, box1->URT.x) &&
			 FPle(box2->LLB.x, box1->URT.x)))
	&&
	/*overlap in y*/
	((FPge(box1->URT.y, box2->URT.y) &&
	  FPle(box1->LLB.y, box2->URT.y)) ||
	 (FPge(box2->URT.y, box1->URT.y) &&
	  FPle(box2->LLB.y, box1->URT.y)));

//printf("box3d_ov about to return %i\n",(int) result);	
	return result;
}


PG_FUNCTION_INFO_V1(geometry_overlap);
Datum geometry_overlap(PG_FUNCTION_ARGS)
{
	GEOMETRY		   *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		   *geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

//printf("in geometry_overlap\n");

	if (geom1->SRID != geom2->SRID)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}


	PG_RETURN_BOOL(box3d_ov(&(geom1->bvol), &(geom2->bvol)));
}


bool is_same_point(POINT3D	*p1, POINT3D	*p2)
{
	return ( FPeq(p1->x, p2->x ) && FPeq(p1->y, p2->y ) &&FPeq(p1->z, p2->z ) );
}

bool is_same_line(LINE3D	*l1, LINE3D	*l2)
{
	int i;
		//lines are directed, so all the points should be the same

	if (l1->npoints != l2->npoints)
		return FALSE; //simple case, not the same # of points

	for (i=0;i<l2->npoints; i++)
	{
		if (!( is_same_point( &l1->points[i], &l2->points[i] ) ) )
		{
			return FALSE;
		}
	}
	return TRUE;
}


/***********************************************************
 *
 * Comparision function for use in Binary Tree searches
 * (ORDER BY, GROUP BY, DISTINCT)
 *
 ***********************************************************/

#if USE_VERSION == 72
#define BTREE_SRID_MISMATCH_SEVERITY NOTICE
#else
#if USE_VERSION < 80
#define BTREE_SRID_MISMATCH_SEVERITY WARNING
#else
#define BTREE_SRID_MISMATCH_SEVERITY ERROR
#endif
#endif


PG_FUNCTION_INFO_V1(geometry_lt);
Datum geometry_lt(PG_FUNCTION_ARGS)
{
	GEOMETRY *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY *geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	//elog(NOTICE, "geometry_lt called");

	// ERROR not used in pg<800 to avoid failure of 'analyze' operation
	if (geom1->SRID != geom2->SRID)
	{
		elog(BTREE_SRID_MISMATCH_SEVERITY,
			"Operation on two GEOMETRIES with different SRIDs\n");
		//PG_RETURN_NULL();
	}

	if  ( ! FPeq(geom1->bvol.LLB.x , geom2->bvol.LLB.x) ) {
		if  (geom1->bvol.LLB.x < geom2->bvol.LLB.x)
			PG_RETURN_BOOL(TRUE);
	}

	if  ( ! FPeq(geom1->bvol.LLB.y , geom2->bvol.LLB.y) ) {
		if  (geom1->bvol.LLB.y < geom2->bvol.LLB.y)
			PG_RETURN_BOOL(TRUE);
	}

	if  ( ! FPeq(geom1->bvol.LLB.z , geom2->bvol.LLB.z) ) {
		if  (geom1->bvol.LLB.z < geom2->bvol.LLB.z)
			PG_RETURN_BOOL(TRUE);
	}

	if  ( ! FPeq(geom1->bvol.URT.x , geom2->bvol.URT.x) ) {
		if  (geom1->bvol.URT.x < geom2->bvol.URT.x)
			PG_RETURN_BOOL(TRUE);
	}

	if  ( ! FPeq(geom1->bvol.URT.y , geom2->bvol.URT.y) ) {
		if  (geom1->bvol.URT.y < geom2->bvol.URT.y)
			PG_RETURN_BOOL(TRUE);
	}

	if  ( ! FPeq(geom1->bvol.URT.z , geom2->bvol.URT.z) ) {
		if  (geom1->bvol.URT.z < geom2->bvol.URT.z)
			PG_RETURN_BOOL(TRUE);
	}

	PG_RETURN_BOOL(FALSE);
}

PG_FUNCTION_INFO_V1(geometry_le);
Datum geometry_le(PG_FUNCTION_ARGS)
{
	GEOMETRY *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY *geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	//elog(NOTICE, "geometry_le called");

	if (geom1->SRID != geom2->SRID)
	{
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
		elog(BTREE_SRID_MISMATCH_SEVERITY,
			"Operation on two GEOMETRIES with different SRIDs\n");
		//PG_RETURN_NULL();
	}

	if  ( ! FPeq(geom1->bvol.LLB.x , geom2->bvol.LLB.x) ) {
		if  (geom1->bvol.LLB.x < geom2->bvol.LLB.x)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_BOOL(TRUE);
		}
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
		PG_RETURN_BOOL(FALSE);
	}

	if  ( ! FPeq(geom1->bvol.LLB.y , geom2->bvol.LLB.y) ) {
		if  (geom1->bvol.LLB.y < geom2->bvol.LLB.y)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_BOOL(TRUE);
		}
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
		PG_RETURN_BOOL(FALSE);
	}

	if  ( ! FPeq(geom1->bvol.LLB.z , geom2->bvol.LLB.z) ) {
		if  (geom1->bvol.LLB.z < geom2->bvol.LLB.z)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_BOOL(TRUE);
		}
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
		PG_RETURN_BOOL(FALSE);
	}

	if  ( ! FPeq(geom1->bvol.URT.x , geom2->bvol.URT.x) ) {
		if  (geom1->bvol.URT.x < geom2->bvol.URT.x)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_BOOL(TRUE);
		}
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
		PG_RETURN_BOOL(FALSE);
	}

	if  ( ! FPeq(geom1->bvol.URT.y , geom2->bvol.URT.y) ) {
		if  (geom1->bvol.URT.y < geom2->bvol.URT.y)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_BOOL(TRUE);
		}
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
		PG_RETURN_BOOL(FALSE);
	}

	if  ( ! FPeq(geom1->bvol.URT.z , geom2->bvol.URT.z) ) {
		if  (geom1->bvol.URT.z < geom2->bvol.URT.z)
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
Datum geometry_eq(PG_FUNCTION_ARGS)
{
	GEOMETRY *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY *geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	//elog(NOTICE, "geometry_eq called");

	if (geom1->SRID != geom2->SRID)
	{
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
		elog(BTREE_SRID_MISMATCH_SEVERITY,
			"Operation on two GEOMETRIES with different SRIDs\n");
		//PG_RETURN_NULL();
	}

	if  ( ! FPeq(geom1->bvol.LLB.x , geom2->bvol.LLB.x) ) 
	{
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
		PG_RETURN_BOOL(FALSE);
	}

	if  ( ! FPeq(geom1->bvol.LLB.y , geom2->bvol.LLB.y) ) 
	{
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
		PG_RETURN_BOOL(FALSE);
	}

	if  ( ! FPeq(geom1->bvol.LLB.z , geom2->bvol.LLB.z) ) 
	{
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
		PG_RETURN_BOOL(FALSE);
	}

	if  ( ! FPeq(geom1->bvol.URT.x , geom2->bvol.URT.x) ) 
	{
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
		PG_RETURN_BOOL(FALSE);
	}

	if  ( ! FPeq(geom1->bvol.URT.y , geom2->bvol.URT.y) )
	{
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
		PG_RETURN_BOOL(FALSE);
	}

	if  ( ! FPeq(geom1->bvol.URT.z , geom2->bvol.URT.z) ) 
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
Datum geometry_ge(PG_FUNCTION_ARGS)
{
	GEOMETRY *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY *geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	//elog(NOTICE, "geometry_ge called");

	if (geom1->SRID != geom2->SRID)
	{
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
		elog(BTREE_SRID_MISMATCH_SEVERITY,
			"Operation on two GEOMETRIES with different SRIDs\n");
		//PG_RETURN_NULL();
	}

	if  ( ! FPeq(geom1->bvol.LLB.x , geom2->bvol.LLB.x) ) {
		if  (geom1->bvol.LLB.x > geom2->bvol.LLB.x)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_BOOL(TRUE);
		}
		PG_RETURN_BOOL(FALSE);
	}

	if  ( ! FPeq(geom1->bvol.LLB.y , geom2->bvol.LLB.y) ) {
		if  (geom1->bvol.LLB.y > geom2->bvol.LLB.y)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_BOOL(TRUE);
		}
		PG_RETURN_BOOL(FALSE);
	}

	if  ( ! FPeq(geom1->bvol.LLB.z , geom2->bvol.LLB.z) ) {
		if  (geom1->bvol.LLB.z > geom2->bvol.LLB.z)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_BOOL(TRUE);
		}
		PG_RETURN_BOOL(FALSE);
	}

	if  ( ! FPeq(geom1->bvol.URT.x , geom2->bvol.URT.x) ) {
		if  (geom1->bvol.URT.x > geom2->bvol.URT.x)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_BOOL(TRUE);
		}
		PG_RETURN_BOOL(FALSE);
	}

	if  ( ! FPeq(geom1->bvol.URT.y , geom2->bvol.URT.y) ) {
		if  (geom1->bvol.URT.y > geom2->bvol.URT.y)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_BOOL(TRUE);
		}
		PG_RETURN_BOOL(FALSE);
	}

	if  ( ! FPeq(geom1->bvol.URT.z , geom2->bvol.URT.z) ) {
		if  (geom1->bvol.URT.z > geom2->bvol.URT.z)
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
Datum geometry_gt(PG_FUNCTION_ARGS)
{
	GEOMETRY *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY *geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	//elog(NOTICE, "geometry_gt called");

	if (geom1->SRID != geom2->SRID)
	{
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 )
			pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 )
			pfree(geom2);
		elog(BTREE_SRID_MISMATCH_SEVERITY,
			"Operation on two GEOMETRIES with different SRIDs\n");
		//PG_RETURN_NULL();
	}

	if  ( ! FPeq(geom1->bvol.LLB.x , geom2->bvol.LLB.x) ) {
		if  (geom1->bvol.LLB.x > geom2->bvol.LLB.x)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_BOOL(TRUE);
		}
	}

	if  ( ! FPeq(geom1->bvol.LLB.y , geom2->bvol.LLB.y) ) {
		if  (geom1->bvol.LLB.y > geom2->bvol.LLB.y)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_BOOL(TRUE);
		}
	}

	if  ( ! FPeq(geom1->bvol.LLB.z , geom2->bvol.LLB.z) ) {
		if  (geom1->bvol.LLB.z > geom2->bvol.LLB.z)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_BOOL(TRUE);
		}
	}

	if  ( ! FPeq(geom1->bvol.URT.x , geom2->bvol.URT.x) ) {
		if  (geom1->bvol.URT.x > geom2->bvol.URT.x)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_BOOL(TRUE);
		}
	}

	if  ( ! FPeq(geom1->bvol.URT.y , geom2->bvol.URT.y) ) {
		if  (geom1->bvol.URT.y > geom2->bvol.URT.y)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_BOOL(TRUE);
		}
	}

	if  ( ! FPeq(geom1->bvol.URT.z , geom2->bvol.URT.z) ) {
		if  (geom1->bvol.URT.z > geom2->bvol.URT.z)
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
Datum geometry_cmp(PG_FUNCTION_ARGS)
{
	GEOMETRY *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY *geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	//elog(NOTICE, "geometry_cmp called");

	if (geom1->SRID != geom2->SRID)
	{
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 )
			pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 )
			pfree(geom2);
		elog(BTREE_SRID_MISMATCH_SEVERITY,
			"Operation on two GEOMETRIES with different SRIDs\n");
		//PG_RETURN_NULL();
	}

	if  ( ! FPeq(geom1->bvol.LLB.x , geom2->bvol.LLB.x) ) {
		if  (geom1->bvol.LLB.x < geom2->bvol.LLB.x)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_INT32(-1);
		}
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
		PG_RETURN_INT32(1);
	}

	if  ( ! FPeq(geom1->bvol.LLB.y , geom2->bvol.LLB.y) ) {
		if  (geom1->bvol.LLB.y < geom2->bvol.LLB.y)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_INT32(-1);
		}
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
		PG_RETURN_INT32(1);
	}

	if  ( ! FPeq(geom1->bvol.LLB.z , geom2->bvol.LLB.z) ) {
		if  (geom1->bvol.LLB.z < geom2->bvol.LLB.z)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_INT32(-1);
		}
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
		PG_RETURN_INT32(1);
	}

	if  ( ! FPeq(geom1->bvol.URT.x , geom2->bvol.URT.x) ) {
		if  (geom1->bvol.URT.x < geom2->bvol.URT.x)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_INT32(-1);
		}
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
		PG_RETURN_INT32(1);
	}

	if  ( ! FPeq(geom1->bvol.URT.y , geom2->bvol.URT.y) ) {
		if  (geom1->bvol.URT.y < geom2->bvol.URT.y)
		{
			if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
			if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
			PG_RETURN_INT32(-1);
		}
		if ( (Pointer *)PG_GETARG_DATUM(0) != (Pointer *)geom1 ) pfree(geom1);
		if ( (Pointer *)PG_GETARG_DATUM(1) != (Pointer *)geom2 ) pfree(geom2);
		PG_RETURN_INT32(1);
	}

	if  ( ! FPeq(geom1->bvol.URT.z , geom2->bvol.URT.z) ) {
		if  (geom1->bvol.URT.z < geom2->bvol.URT.z)
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


//order of points in a ring IS important
//order of rings is also important
bool is_same_polygon(POLYGON3D	*poly1, POLYGON3D	*poly2)
{
	int	i;
	int	numb_points;
	POINT3D	*pts1, *pts2;


	if (poly1->nrings != poly2->nrings)
		return FALSE;
	
	numb_points = 0;
	for (i=0; i< poly1->nrings; i++)
	{
		if (poly1->npoints[i] != poly2->npoints[i])
			return FALSE;
		numb_points += poly1->npoints[i];
	}

	pts1 = (POINT3D *) ( (char *)&(poly1->npoints[poly1->nrings] )  );
	pts1 = (POINT3D *) MAXALIGN(pts1);
	

	pts2 = (POINT3D *) ( (char *)&(poly2->npoints[poly2->nrings] )  );
	pts2 = (POINT3D *) MAXALIGN(pts2);

	for (i=0; i< numb_points; i++)
	{
		if (!( is_same_point( &pts1[i], &pts2[i] ) ) )
		{
			return FALSE;
		}

	}
	return TRUE;
}



//FIXED: 
// geom1 same as geom2
//  iff  
//	+ have same type
//	+ have same # objects
//	+ have same bvol
//	+ each object in geom1 has a corresponding object in geom2 (see above)
PG_FUNCTION_INFO_V1(geometry_same);
Datum geometry_same(PG_FUNCTION_ARGS)
{
	GEOMETRY		   *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		   *geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	bool				result;
	int				i,j;
	bool				*already_hit;
	int32				type1,type2;
	int32				*offsets1,*offsets2;
	char				*o1,*o2;

	if (geom1->SRID != geom2->SRID)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}


//printf("in geometry_same\n");

	//This was removed because this doesnt actually refer to the geometry, but how it is represented
	// to the user
	//easy tests
	//if (geom1->type != geom2-> type)
	//	PG_RETURN_BOOL(FALSE);

	

	if (geom1->nobjs != geom2->nobjs)
		PG_RETURN_BOOL(FALSE);	

	result = DatumGetBool(DirectFunctionCall2(box3d_same,
								PointerGetDatum(&geom1->bvol), 
								PointerGetDatum(&geom2->bvol)   )     );

	if (result == FALSE)
		PG_RETURN_BOOL(FALSE);

	if (geom1->nobjs<1)
		return FALSE; // no objects (probably a BOXONLYTYPE) and bvols dont match

//	printf(" 	+have to do it the hard way");

	already_hit = (bool *) palloc (geom2->nobjs * sizeof(bool) );
	memset(already_hit, 0, geom2->nobjs * sizeof(bool) ); //all false
	

	offsets1 = (int32 *) ( ((char *) &(geom1->objType[0] ))+ sizeof(int32) * geom1->nobjs ) ;
	offsets2 = (int32 *) ( ((char *) &(geom2->objType[0] ))+ sizeof(int32) * geom2->nobjs ) ;


	//now have to do a scan of each object

	for (j=0; j< geom1->nobjs; j++)		//for each object in geom1
	{
		o1 = (char *) geom1 +offsets1[j] ;  
		type1=  geom1->objType[j];

		for(i=0;i<geom1->nobjs; i++)		//for each object in geom2
		{
			o2 = (char *) geom2 +offsets2[i] ;  
			type2=  geom2->objType[j];
			
			if (  (type1 == type2) && (!(already_hit[i])) )
			{
				//see if they match
				if (type1 == POINTTYPE)
				{
					if (is_same_point((POINT3D *) o1,(POINT3D *) o2))
					{
						already_hit[i] = TRUE;
						break;
					} 
				}
				if (type1 == LINETYPE)
				{
					if (is_same_line((LINE3D *) o1,(LINE3D *) o2))
					{
						already_hit[i] = TRUE;
						break;
					} 

				}
				if (type2 == POLYGONTYPE)
				{
					if (is_same_polygon((POLYGON3D *) o1,(POLYGON3D *) o2))
					{
						already_hit[i] = TRUE;
						break;
					} 
				}	
			}
		}
		if (i == geom1->nobjs)
			PG_RETURN_BOOL(FALSE);	 // couldnt find match
	}
	PG_RETURN_BOOL(TRUE);
}

/*******************************************************
 *Box3d routines
 *******************************************************/



/*		box_overlap		-		does box1 overlap box2?
 */
PG_FUNCTION_INFO_V1(box3d_overlap);
Datum box3d_overlap(PG_FUNCTION_ARGS)
{
	BOX3D		   *box1 = (BOX3D *) PG_GETARG_POINTER(0);
	BOX3D		   *box2 = (BOX3D *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(box3d_ov(box1, box2));
}

/*		box_overleft	-		is the right edge of box1 to the left of
 *								the right edge of box2?
 *
 *		This is "less than or equal" for the end of a time range,
 *		when time ranges are stored as rectangles.
 */
PG_FUNCTION_INFO_V1(box3d_overleft);
Datum box3d_overleft(PG_FUNCTION_ARGS)
{
	BOX3D		   *box1 = (BOX3D *) PG_GETARG_POINTER(0);
	BOX3D		   *box2 = (BOX3D *) PG_GETARG_POINTER(1);

	bool	result;

	result = FPle(box1->URT.x, box2->URT.x);

//printf("box3d_overleft about to return %i\n",(int) result);	

	PG_RETURN_BOOL(result);
}

/*		box_right		-		is box1 strictly right of box2?
 */
PG_FUNCTION_INFO_V1(box3d_right);
Datum box3d_right(PG_FUNCTION_ARGS)
{
	BOX3D		   *box1 = (BOX3D *) PG_GETARG_POINTER(0);
	BOX3D		   *box2 = (BOX3D *) PG_GETARG_POINTER(1);

	bool	result;

	result = FPgt(box1->LLB.x, box2->URT.x);
//printf("box3d_rightabout to return %i\n",(int) result);	

	PG_RETURN_BOOL(result);
}

/*		box_contain		-		does box1 contain box2?
 */
PG_FUNCTION_INFO_V1(box3d_contain);
Datum box3d_contain(PG_FUNCTION_ARGS)
{
	BOX3D		   *box1 = (BOX3D *) PG_GETARG_POINTER(0);
	BOX3D		   *box2 = (BOX3D *) PG_GETARG_POINTER(1);

	bool result;

	result = FPge(box1->URT.x, box2->URT.x) &&
				   FPle(box1->LLB.x, box2->LLB.x) &&
				   FPge(box1->URT.y, box2->URT.y) &&
				   FPle(box1->LLB.y, box2->LLB.y);
	//printf("box3d_contain about to return %i\n",(int) result);	


	PG_RETURN_BOOL(result);
}


/******************************************************
 * RTREE index requires these 3 functions
 ******************************************************/

PG_FUNCTION_INFO_V1(geometry_union);
Datum geometry_union(PG_FUNCTION_ARGS)
{
	GEOMETRY		   *geom1 =  (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		   *geom2 =  (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	GEOMETRY		   *result = (GEOMETRY *) palloc(sizeof(GEOMETRY) );
	BOX3D				*n;

	if (geom1->SRID != geom2->SRID)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}


	result->size = sizeof(GEOMETRY);
	result->type = BBOXONLYTYPE;
	result->nobjs = -1;
	result->SRID = geom1->SRID;
	result->scale = geom1->scale;
	result->offsetX = geom1->offsetX;
	result->offsetY = geom1->offsetY;


	n = &result->bvol;

	//cheat - just change the bbox

        n->URT.x = max(geom1->bvol.URT.x, geom2->bvol.URT.x);
        n->URT.y = max(geom1->bvol.URT.y, geom2->bvol.URT.y);
        n->LLB.x = min(geom1->bvol.LLB.x, geom2->bvol.LLB.x);
        n->LLB.y = min(geom1->bvol.LLB.y, geom2->bvol.LLB.y);

	
        n->URT.z = max(geom1->bvol.URT.z, geom2->bvol.URT.z);
        n->LLB.z = min(geom1->bvol.LLB.z, geom2->bvol.LLB.z);

	  PG_FREE_IF_COPY(geom1, 0);
        PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(geometry_inter);
Datum geometry_inter(PG_FUNCTION_ARGS)
{
	GEOMETRY		   *geom1 =  (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	GEOMETRY		   *geom2 =  (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	GEOMETRY		   *result = (GEOMETRY *) palloc(sizeof(GEOMETRY) );

	if (geom1->SRID != geom2->SRID)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}

	result->size = sizeof(GEOMETRY);
	result->type = BBOXONLYTYPE;
	result->nobjs = -1;
	result->SRID = geom1->SRID;
	result->scale = geom1->scale;
	result->offsetX = geom1->offsetX;
	result->offsetY = geom1->offsetY;

	result->bvol.URT.x = min(geom1->bvol.URT.x, geom2->bvol.URT.x);
	result->bvol.URT.y = min(geom1->bvol.URT.y, geom2->bvol.URT.y);
	result->bvol.URT.z = min(geom1->bvol.URT.z, geom2->bvol.URT.z);

	result->bvol.LLB.x = max(geom1->bvol.LLB.x, geom2->bvol.LLB.x);
	result->bvol.LLB.y = max(geom1->bvol.LLB.y, geom2->bvol.LLB.y);
	result->bvol.LLB.z = max(geom1->bvol.LLB.z, geom2->bvol.LLB.z);

    if (result->bvol.URT.x < result->bvol.LLB.x || result->bvol.URT.y < result->bvol.LLB.y)
    {
		pfree(result);
        /* Indicate "no intersection" by returning NULL pointer */
        result = NULL;
    }
	PG_FREE_IF_COPY(geom1, 0);
    PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(geometry_size);
Datum geometry_size(PG_FUNCTION_ARGS)
{
	Pointer        	 aptr = PG_GETARG_POINTER(0);

    float      	*size = (float *) PG_GETARG_POINTER(1);
    GEOMETRY    	*a;
	float			xdim,ydim;

    if (aptr == NULL)
    {
        *size = 0.0;
		//printf("	+ aprt==null return in 0.0\n");
        PG_RETURN_VOID();
    }

	a = (GEOMETRY *)   PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

    if (a->bvol.URT.x <= a->bvol.LLB.x ||
        a->bvol.URT.y <= a->bvol.LLB.y)
        *size = 0.0;
    else
    {
		xdim = (a->bvol.URT.x - a->bvol.LLB.x);
        ydim = (a->bvol.URT.y - a->bvol.LLB.y);

        *size = (float) (xdim * ydim);
    }

	//printf("	+ about to return (and free) with %e\n",*size);

    /* Avoid leaking memory when handed toasted input. */
	PG_FREE_IF_COPY(a, 0);
	
	PG_RETURN_VOID();
}
