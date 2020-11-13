/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright (C) 2015 Sandro Santilli <strk@kbt.io>
 * Copyright (C) 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CUnit/Basic.h"

#include "liblwgeom_internal.h"
#include "cu_tester.h"
#include "measures.h"
#include "measures3d.h"
#include "lwtree.h"

static LWGEOM* lwgeom_from_text(const char *str)
{
	LWGEOM_PARSER_RESULT r;
	if( LW_FAILURE == lwgeom_parse_wkt(&r, (char*)str, LW_PARSER_CHECK_NONE) )
		return NULL;
	return r.geom;
}

#define DIST2DTEST(str1, str2, res, accepted_error) \
	do_test_mindistance_tolerance(str1, str2, res, __LINE__, lwgeom_mindistance2d_tolerance, accepted_error);\
	do_test_mindistance_tolerance(str2, str1, res, __LINE__, lwgeom_mindistance2d_tolerance, accepted_error)
#define DIST3DTEST(str1, str2, res, accepted_error) \
	do_test_mindistance_tolerance(str1, str2, res, __LINE__, lwgeom_mindistance3d_tolerance, accepted_error);\
	do_test_mindistance_tolerance(str2, str1, res, __LINE__, lwgeom_mindistance3d_tolerance, accepted_error)

static void
do_test_mindistance_tolerance(char *in1,
			      char *in2,
			      double expected_res,
			      int line,
			      double (*distancef)(const LWGEOM *, const LWGEOM *, double),
				  double accepted_error)
{
	LWGEOM *lw1;
	LWGEOM *lw2;
	double distance;
	char *msg1 = "test_mindistance2d_tolerance failed (got %g expected %g) at line %d\n";
	char *msg2 = "\n\ndo_test_mindistance2d_tolerance: NULL lwgeom generated from WKT\n  %s\n\n";

	lw1 = lwgeom_from_wkt(in1, LW_PARSER_CHECK_NONE);
	lw2 = lwgeom_from_wkt(in2, LW_PARSER_CHECK_NONE);

	if ( ! lw1 )
	{
		printf(msg2, in1);
		exit(1);
	}
	if ( ! lw2 )
	{
		printf(msg2, in2);
		exit(1);
	}

	FLAGS_SET_SOLID(lw1->flags, 1);
	FLAGS_SET_SOLID(lw2->flags, 1);

	distance = distancef(lw1, lw2, 0.0);
	lwgeom_free(lw1);
	lwgeom_free(lw2);

	if ( fabs(distance - expected_res) > accepted_error )
	{
		printf(msg1, distance, expected_res, line);
		CU_FAIL();
	}
	else
	{
		CU_PASS();
	}

}

static void test_mindistance2d_tolerance(void)
{
	double default_accepted_error = 0.00001;
	double zero_accepted_error = 0.0;
	/*
	** Simple case.
	*/
	DIST2DTEST("POINT(0 0)", "MULTIPOINT(0 1.5,0 2,0 2.5)", 1.5, default_accepted_error);

	/*
	** Point vs Geometry Collection.
	*/
	DIST2DTEST("POINT(0 0)", "GEOMETRYCOLLECTION(POINT(3 4))", 5.0, default_accepted_error);

	/*
	** Point vs Geometry Collection Collection.
	*/
	DIST2DTEST("POINT(0 0)", "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(POINT(3 4)))", 5.0, default_accepted_error);

	/*
	** Point vs Geometry Collection Collection Collection.
	*/
	DIST2DTEST("POINT(0 0)", "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(POINT(3 4))))", 5.0, default_accepted_error);

	/*
	** Point vs Geometry Collection Collection Collection Multipoint.
	*/
	DIST2DTEST("POINT(0 0)", "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(MULTIPOINT(3 4))))", 5.0, default_accepted_error);

	/*
	** Geometry Collection vs Geometry Collection
	*/
	DIST2DTEST("GEOMETRYCOLLECTION(POINT(0 0))", "GEOMETRYCOLLECTION(POINT(3 4))", 5.0, default_accepted_error);

	/*
	** Geometry Collection Collection vs Geometry Collection Collection
	*/
	DIST2DTEST("GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(POINT(0 0)))", "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(POINT(3 4)))", 5.0, default_accepted_error);

	/*
	** Geometry Collection Collection Multipoint vs Geometry Collection Collection Multipoint
	*/
	DIST2DTEST("GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(MULTIPOINT(0 0)))", "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(MULTIPOINT(3 4)))", 5.0, default_accepted_error);

	/*
	** Linestring vs its start point
	*/
	DIST2DTEST("LINESTRING(-2 0, -0.2 0)", "POINT(-2 0)", 0, zero_accepted_error);

	/*
	** Linestring vs its end point
	*/
	DIST2DTEST("LINESTRING(-0.2 0, -2 0)", "POINT(-2 0)", 0, zero_accepted_error);

	/*
	** Linestring vs its start point (tricky number, see #1459)
	*/
	DIST2DTEST("LINESTRING(-1e-8 0, -0.2 0)", "POINT(-1e-8 0)", 0, zero_accepted_error);

	/*
	** Linestring vs its end point (tricky number, see #1459)
	*/
	DIST2DTEST("LINESTRING(-0.2 0, -1e-8 0)", "POINT(-1e-8 0)", 0, zero_accepted_error);

	/*
	* Circular string and point
	*/
	DIST2DTEST("CIRCULARSTRING(-1 0, 0 1, 1 0)", "POINT(0 0)", 1, default_accepted_error);
	DIST2DTEST("CIRCULARSTRING(-3 0, -2 0, -1 0, 0 1, 1 0)", "POINT(0 0)", 1, default_accepted_error);

	/*
	* Circular string and Circular string
	*/
	DIST2DTEST("CIRCULARSTRING(-1 0, 0 1, 1 0)", "CIRCULARSTRING(0 0, 1 -1, 2 0)", 1, default_accepted_error);

	/*
	* CurvePolygon and Point
	*/
	static char *cs1 = "CURVEPOLYGON(COMPOUNDCURVE(CIRCULARSTRING(1 6, 6 1, 9 7),(9 7, 3 13, 1 6)),COMPOUNDCURVE((3 6, 5 4, 7 4, 7 6),CIRCULARSTRING(7 6,5 8,3 6)))";
	DIST2DTEST(cs1, "POINT(3 14)", 1, default_accepted_error);
	DIST2DTEST(cs1, "POINT(3 8)", 0, default_accepted_error);
	DIST2DTEST(cs1, "POINT(6 5)", 1, default_accepted_error);
	DIST2DTEST(cs1, "POINT(6 4)", 0, default_accepted_error);

	/*
	* CurvePolygon and Linestring
	*/
	DIST2DTEST(cs1, "LINESTRING(0 0, 50 0)", 0.917484, default_accepted_error);
	DIST2DTEST(cs1, "LINESTRING(6 0, 10 7)", 0, default_accepted_error);
	DIST2DTEST(cs1, "LINESTRING(4 4, 4 8)", 0, default_accepted_error);
	DIST2DTEST(cs1, "LINESTRING(4 7, 5 6, 6 7)", 0.585786, default_accepted_error);
	DIST2DTEST(cs1, "LINESTRING(10 0, 10 2, 10 0)", 1.52913, default_accepted_error);

	/*
	* CurvePolygon and Polygon
	*/
	DIST2DTEST(cs1, "POLYGON((10 4, 10 8, 13 8, 13 4, 10 4))", 0.58415, default_accepted_error);
	DIST2DTEST(cs1, "POLYGON((9 4, 9 8, 12 8, 12 4, 9 4))", 0, default_accepted_error);
	DIST2DTEST(cs1, "POLYGON((1 4, 1 8, 4 8, 4 4, 1 4))", 0, default_accepted_error);

	/*
	* CurvePolygon and CurvePolygon
	*/
	DIST2DTEST(cs1, "CURVEPOLYGON(CIRCULARSTRING(-1 4, 0 5, 1 4, 0 3, -1 4))", 0.0475666, default_accepted_error);
	DIST2DTEST(cs1, "CURVEPOLYGON(CIRCULARSTRING(1 4, 2 5, 3 4, 2 3, 1 4))", 0.0, default_accepted_error);

	/*
	* MultiSurface and CurvePolygon
	*/
	static char *cs2 = "MULTISURFACE(POLYGON((0 0,0 4,4 4,4 0,0 0)),CURVEPOLYGON(CIRCULARSTRING(8 2,10 4,12 2,10 0,8 2)))";
	DIST2DTEST(cs2, "CURVEPOLYGON(CIRCULARSTRING(5 2,6 3,7 2,6 1,5 2))", 1, default_accepted_error);
	DIST2DTEST(cs2, "CURVEPOLYGON(CIRCULARSTRING(4 2,5 3,6 2,5 1,4 2))", 0, default_accepted_error);
	DIST2DTEST(cs2, "CURVEPOLYGON(CIRCULARSTRING(5 3,6 2,5 1,4 2,5 3))", 0, default_accepted_error);
	DIST2DTEST(cs2, "CURVEPOLYGON(CIRCULARSTRING(4.5 3,5.5 2,4.5 1,3.5 2,4.5 3))", 0, default_accepted_error);
	DIST2DTEST(cs2, "CURVEPOLYGON(CIRCULARSTRING(5.5 3,6.5 2,5.5 1,4.5 2,5.5 3))", 0.5, default_accepted_error);
	DIST2DTEST(cs2, "CURVEPOLYGON(CIRCULARSTRING(10 3,11 2,10 1,9 2,10 3))", 0, default_accepted_error);
	DIST2DTEST(cs2, "CURVEPOLYGON(CIRCULARSTRING(2 3,3 2,2 1,1 2,2 3))", 0, default_accepted_error);
	DIST2DTEST(cs2, "CURVEPOLYGON(CIRCULARSTRING(5 7,6 8,7 7,6 6,5 7))", 2.60555, default_accepted_error);

	/*
	* MultiCurve and Linestring
	*/
	DIST2DTEST("LINESTRING(0.5 1,0.5 3)", "MULTICURVE(CIRCULARSTRING(2 3,3 2,2 1,1 2,2 3),(0 0, 0 5))", 0.5, default_accepted_error);

}

static void
test_mindistance3d_tolerance(void)
{
	double default_accepted_error = 0.00001;
	double zero_accepted_error = 0.0;
	/* 2D [Z=0] should work just the same */
	DIST3DTEST("POINT(0 0 0)", "MULTIPOINT(0 1.5 0, 0 2 0, 0 2.5 0)", 1.5, default_accepted_error);
	DIST3DTEST("POINT(0 0 0)", "MULTIPOINT(0 1.5 0, 0 2 0, 0 2.5 0)", 1.5, default_accepted_error);
	DIST3DTEST("POINT(0 0 0)", "GEOMETRYCOLLECTION(POINT(3 4 0))", 5.0, default_accepted_error);
	DIST3DTEST("POINT(0 0 0)", "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(POINT(3 4 0)))", 5.0, default_accepted_error);
	DIST3DTEST("POINT(0 0 0)", "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(POINT(3 4 0))))", 5.0, default_accepted_error);
	DIST3DTEST("POINT(0 0)", "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(MULTIPOINT(3 4))))", 5.0, default_accepted_error);
	DIST3DTEST("GEOMETRYCOLLECTION(POINT(0 0 0))", "GEOMETRYCOLLECTION(POINT(3 4 0))", 5.0, default_accepted_error);
	DIST3DTEST("GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(POINT(0 0 0)))",
		   "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(POINT(3 4 0)))",
		   5.0, default_accepted_error);
	DIST3DTEST("GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(MULTIPOINT(0 0 0)))",
		   "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(MULTIPOINT(3 4 0)))",
		   5.0, default_accepted_error);
	DIST3DTEST("LINESTRING(-2 0 0, -0.2 0 0)", "POINT(-2 0 0)", 0, zero_accepted_error);
	DIST3DTEST("LINESTRING(-0.2 0 0, -2 0 0)", "POINT(-2 0 0)", 0, zero_accepted_error);
	DIST3DTEST("LINESTRING(-1e-8 0 0, -0.2 0 0)", "POINT(-1e-8 0 0)", 0, zero_accepted_error);
	DIST3DTEST("LINESTRING(-0.2 0 0, -1e-8 0 0)", "POINT(-1e-8 0 0)", 0, zero_accepted_error);

	/* Tests around intersections */
	DIST3DTEST("LINESTRING(1 0 0 , 2 0 0)", "POLYGON((1 1 0, 2 1 0, 2 2 0, 1 2 0, 1 1 0))", 1.0, default_accepted_error);
	DIST3DTEST("LINESTRING(1 1 1 , 2 1 0)", "POLYGON((1 1 0, 2 1 0, 2 2 0, 1 2 0, 1 1 0))", 0.0, zero_accepted_error);
	DIST3DTEST("LINESTRING(1 1 1 , 2 1 1)", "POLYGON((1 1 0, 2 1 0, 2 2 0, 1 2 0, 1 1 0))", 1.0, default_accepted_error);

	/* Same but triangles */
	DIST3DTEST("LINESTRING(1 0 0 , 2 0 0)", "TRIANGLE((1 1 0, 2 1 0, 1 2 0, 1 1 0))", 1.0, default_accepted_error);
	DIST3DTEST("LINESTRING(1 1 1 , 2 1 0)", "TRIANGLE((1 1 0, 2 1 0, 1 2 0, 1 1 0))", 0.0, zero_accepted_error);
	DIST3DTEST("LINESTRING(1 1 1 , 2 1 1)", "TRIANGLE((1 1 0, 2 1 0, 1 2 0, 1 1 0))", 1.0, default_accepted_error);

	/* Triangle to triangle*/
	DIST3DTEST("TRIANGLE((-1 1 0, -2 1 0, -1 2 0, -1 1 0))", "TRIANGLE((1 1 0, 2 1 0, 1 2 0, 1 1 0))", 2.0, default_accepted_error);

	/* Line in polygon */
	DIST3DTEST("LINESTRING(1 1 1 , 2 2 2)", "POLYGON((0 0 0, 2 2 2, 3 3 1, 0 0 0))", 0.0, zero_accepted_error);

	/* Line has a point in the same plane as the polygon but isn't the closest*/
	DIST3DTEST("LINESTRING(-10000 -10000 0, 0 0 1)", "POLYGON((0 0 0, 1 0 0, 1 1 0, 0 1 0, 0 0 0))", 1, default_accepted_error);

	/* This is an invalid polygon since it defines just a line */
	DIST3DTEST("LINESTRING(1 1 1, 2 2 2)", "POLYGON((0 0 0, 2 2 2, 3 3 3, 0 0 0))", 0, zero_accepted_error);
	DIST3DTEST("TRIANGLE((1 1 1, 2 2 2, 3 3 3, 1 1 1))", "POLYGON((0 0 0, 2 2 2, 3 3 3, 0 0 0))", 0, zero_accepted_error);
	DIST3DTEST("POLYGON((0 0 0, 2 2 2, 3 3 3, 0 0 0))", "TRIANGLE((1 1 1, 2 2 2, 3 3 3, 1 1 1))", 0, zero_accepted_error);
	DIST3DTEST("TRIANGLE((0 0 0, 2 2 2, 3 3 3, 0 0 0))", "LINESTRING(1 1 1, 2 2 2)", 0, zero_accepted_error);

	/* A box in a box: two solids, one inside another */
	DIST3DTEST(
	    "POLYHEDRALSURFACE Z (((0 0 0,0 1 0,1 1 0,1 0 0,0 0 0)),((0 0 1,1 0 1,1 1 1,0 1 1,0 0 1)),((0 0 0,0 0 1,0 1 1,0 1 0,0 0 0)),((1 0 0,1 1 0,1 1 1,1 0 1,1 0 0)),((0 0 0,1 0 0,1 0 1,0 0 1,0 0 0)),((0 1 0,0 1 1,1 1 1,1 1 0,0 1 0)))",
	    "POLYHEDRALSURFACE Z (((-1 -1 -1,-1 2 -1,2 2 -1,2 -1 -1,-1 -1 -1)),((-1 -1 2,2 -1 2,2 2 2,-1 2 2,-1 -1 2)),((-1 -1 -1,-1 -1 2,-1 2 2,-1 2 -1,-1 -1 -1)),((2 -1 -1,2 2 -1,2 2 2,2 -1 2,2 -1 -1)),((-1 -1 -1,2 -1 -1,2 -1 2,-1 -1 2,-1 -1 -1)),((-1 2 -1,-1 2 2,2 2 2,2 2 -1,-1 2 -1)))",
	    0, zero_accepted_error);

	/* A box in a box with a hat: two solids, one inside another, Z ray up is hitting hat */
	DIST3DTEST(
	    "POLYHEDRALSURFACE Z (((0 0 0,0 1 0,1 1 0,1 0 0,0 0 0)),((0 0 1,1 0 1,1 1 1,0 1 1,0 0 1)),((0 0 0,0 0 1,0 1 1,0 1 0,0 0 0)),((1 0 0,1 1 0,1 1 1,1 0 1,1 0 0)),((0 0 0,1 0 0,1 0 1,0 0 1,0 0 0)),((0 1 0,0 1 1,1 1 1,1 1 0,0 1 0)))",
	    "POLYHEDRALSURFACE Z (((0 0 2,0 1 2,-1 2 2,0 0 2)),((0 1 2,0 0 2,0 1 4,0 1 2)),((-1 2 2,0 1 2,1 1 2,-1 2 2)),((0 0 2,-1 2 2,-1 -1 2,0 0 2)),((0 1 4,0 0 2,0 0 4,0 1 4)),((0 1 2,0 1 4,1 1 4,0 1 2)),((1 1 2,0 1 2,1 1 4,1 1 2)),((-1 2 2,1 1 2,2 2 2,-1 2 2)),((-1 -1 2,-1 2 2,-1 -1 -1,-1 -1 2)),((0 0 2,-1 -1 2,1 0 2,0 0 2)),((0 0 4,0 0 2,1 0 2,0 0 4)),((0 1 4,0 0 4,1 0 4,0 1 4)),((1 1 4,0 1 4,1 0 4,1 1 4)),((1 1 2,1 1 4,1 0 4,1 1 2)),((2 2 2,1 1 2,2 -1 2,2 2 2)),((-1 2 2,2 2 2,-1 2 -1,-1 2 2)),((-1 -1 -1,-1 2 2,-1 2 -1,-1 -1 -1)),((-1 -1 2,-1 -1 -1,2 -1 -1,-1 -1 2)),((1 0 2,-1 -1 2,2 -1 2,1 0 2)),((0 0 4,1 0 2,1 0 4,0 0 4)),((1 1 2,1 0 4,1 0 2,1 1 2)),((2 -1 2,1 1 2,1 0 2,2 -1 2)),((2 2 2,2 -1 2,2 2 -1,2 2 2)),((-1 2 -1,2 2 2,2 2 -1,-1 2 -1)),((-1 -1 -1,-1 2 -1,2 2 -1,-1 -1 -1)),((2 -1 -1,-1 -1 -1,2 2 -1,2 -1 -1)),((-1 -1 2,2 -1 -1,2 -1 2,-1 -1 2)),((2 2 -1,2 -1 2,2 -1 -1,2 2 -1)))",
	    0, zero_accepted_error);

	/* Same but as TIN */
	DIST3DTEST(
	    "TIN Z (((0 0 2,0 1 2,-1 2 2,0 0 2)),((0 1 2,0 0 2,0 1 4,0 1 2)),((-1 2 2,0 1 2,1 1 2,-1 2 2)),((0 0 2,-1 2 2,-1 -1 2,0 0 2)),((0 1 4,0 0 2,0 0 4,0 1 4)),((0 1 2,0 1 4,1 1 4,0 1 2)),((1 1 2,0 1 2,1 1 4,1 1 2)),((-1 2 2,1 1 2,2 2 2,-1 2 2)),((-1 -1 2,-1 2 2,-1 -1 -1,-1 -1 2)),((0 0 2,-1 -1 2,1 0 2,0 0 2)),((0 0 4,0 0 2,1 0 2,0 0 4)),((0 1 4,0 0 4,1 0 4,0 1 4)),((1 1 4,0 1 4,1 0 4,1 1 4)),((1 1 2,1 1 4,1 0 4,1 1 2)),((2 2 2,1 1 2,2 -1 2,2 2 2)),((-1 2 2,2 2 2,-1 2 -1,-1 2 2)),((-1 -1 -1,-1 2 2,-1 2 -1,-1 -1 -1)),((-1 -1 2,-1 -1 -1,2 -1 -1,-1 -1 2)),((1 0 2,-1 -1 2,2 -1 2,1 0 2)),((0 0 4,1 0 2,1 0 4,0 0 4)),((1 1 2,1 0 4,1 0 2,1 1 2)),((2 -1 2,1 1 2,1 0 2,2 -1 2)),((2 2 2,2 -1 2,2 2 -1,2 2 2)),((-1 2 -1,2 2 2,2 2 -1,-1 2 -1)),((-1 -1 -1,-1 2 -1,2 2 -1,-1 -1 -1)),((2 -1 -1,-1 -1 -1,2 2 -1,2 -1 -1)),((-1 -1 2,2 -1 -1,2 -1 2,-1 -1 2)),((2 2 -1,2 -1 2,2 -1 -1,2 2 -1)))",
	    "POLYHEDRALSURFACE Z (((0 0 0,0 1 0,1 1 0,1 0 0,0 0 0)),((0 0 1,1 0 1,1 1 1,0 1 1,0 0 1)),((0 0 0,0 0 1,0 1 1,0 1 0,0 0 0)),((1 0 0,1 1 0,1 1 1,1 0 1,1 0 0)),((0 0 0,1 0 0,1 0 1,0 0 1,0 0 0)),((0 1 0,0 1 1,1 1 1,1 1 0,0 1 0)))",
	    0, zero_accepted_error);

	/* Same but both are TIN */
	DIST3DTEST(
	    "TIN Z (((0 0 2,0 1 2,-1 2 2,0 0 2)),((0 1 2,0 0 2,0 1 4,0 1 2)),((-1 2 2,0 1 2,1 1 2,-1 2 2)),((0 0 2,-1 2 2,-1 -1 2,0 0 2)),((0 1 4,0 0 2,0 0 4,0 1 4)),((0 1 2,0 1 4,1 1 4,0 1 2)),((1 1 2,0 1 2,1 1 4,1 1 2)),((-1 2 2,1 1 2,2 2 2,-1 2 2)),((-1 -1 2,-1 2 2,-1 -1 -1,-1 -1 2)),((0 0 2,-1 -1 2,1 0 2,0 0 2)),((0 0 4,0 0 2,1 0 2,0 0 4)),((0 1 4,0 0 4,1 0 4,0 1 4)),((1 1 4,0 1 4,1 0 4,1 1 4)),((1 1 2,1 1 4,1 0 4,1 1 2)),((2 2 2,1 1 2,2 -1 2,2 2 2)),((-1 2 2,2 2 2,-1 2 -1,-1 2 2)),((-1 -1 -1,-1 2 2,-1 2 -1,-1 -1 -1)),((-1 -1 2,-1 -1 -1,2 -1 -1,-1 -1 2)),((1 0 2,-1 -1 2,2 -1 2,1 0 2)),((0 0 4,1 0 2,1 0 4,0 0 4)),((1 1 2,1 0 4,1 0 2,1 1 2)),((2 -1 2,1 1 2,1 0 2,2 -1 2)),((2 2 2,2 -1 2,2 2 -1,2 2 2)),((-1 2 -1,2 2 2,2 2 -1,-1 2 -1)),((-1 -1 -1,-1 2 -1,2 2 -1,-1 -1 -1)),((2 -1 -1,-1 -1 -1,2 2 -1,2 -1 -1)),((-1 -1 2,2 -1 -1,2 -1 2,-1 -1 2)),((2 2 -1,2 -1 2,2 -1 -1,2 2 -1)))",
	    "TIN Z (((0 0 0,0 1 0,1 1 0,0 0 0)),((1 0 0,0 0 0,1 1 0,1 0 0)),((0 1 1,1 0 1,1 1 1,0 1 1)),((0 1 1,0 0 1,1 0 1,0 1 1)),((0 0 0,0 0 1,0 1 1,0 0 0)),((0 1 0,0 0 0,0 1 1,0 1 0)),((1 0 1,1 1 0,1 1 1,1 0 1)),((1 0 1,1 0 0,1 1 0,1 0 1)),((0 0 1,1 0 0,1 0 1,0 0 1)),((0 0 1,0 0 0,1 0 0,0 0 1)),((0 1 0,0 1 1,1 1 1,0 1 0)),((1 1 0,0 1 0,1 1 1,1 1 0)))",
	    0, zero_accepted_error);

	/* Point inside TIN */
	DIST3DTEST(
	    "TIN Z (((0 0 2,0 1 2,-1 2 2,0 0 2)),((0 1 2,0 0 2,0 1 4,0 1 2)),((-1 2 2,0 1 2,1 1 2,-1 2 2)),((0 0 2,-1 2 2,-1 -1 2,0 0 2)),((0 1 4,0 0 2,0 0 4,0 1 4)),((0 1 2,0 1 4,1 1 4,0 1 2)),((1 1 2,0 1 2,1 1 4,1 1 2)),((-1 2 2,1 1 2,2 2 2,-1 2 2)),((-1 -1 2,-1 2 2,-1 -1 -1,-1 -1 2)),((0 0 2,-1 -1 2,1 0 2,0 0 2)),((0 0 4,0 0 2,1 0 2,0 0 4)),((0 1 4,0 0 4,1 0 4,0 1 4)),((1 1 4,0 1 4,1 0 4,1 1 4)),((1 1 2,1 1 4,1 0 4,1 1 2)),((2 2 2,1 1 2,2 -1 2,2 2 2)),((-1 2 2,2 2 2,-1 2 -1,-1 2 2)),((-1 -1 -1,-1 2 2,-1 2 -1,-1 -1 -1)),((-1 -1 2,-1 -1 -1,2 -1 -1,-1 -1 2)),((1 0 2,-1 -1 2,2 -1 2,1 0 2)),((0 0 4,1 0 2,1 0 4,0 0 4)),((1 1 2,1 0 4,1 0 2,1 1 2)),((2 -1 2,1 1 2,1 0 2,2 -1 2)),((2 2 2,2 -1 2,2 2 -1,2 2 2)),((-1 2 -1,2 2 2,2 2 -1,-1 2 -1)),((-1 -1 -1,-1 2 -1,2 2 -1,-1 -1 -1)),((2 -1 -1,-1 -1 -1,2 2 -1,2 -1 -1)),((-1 -1 2,2 -1 -1,2 -1 2,-1 -1 2)),((2 2 -1,2 -1 2,2 -1 -1,2 2 -1)))",
	    "POINT(0 0 0)",
	    0, zero_accepted_error);

	/* A point hits vertical Z edge */
	DIST3DTEST(
	    "POLYHEDRALSURFACE Z (((0 -1 1,-1 -1 1,-1 -1 -1,0 -1 -1,1 -1 -1,0 -1 2,0 -1 1)),((0 1 1,0 1 2,1 1 -1,0 1 -1,-1 1 -1,-1 1 1,0 1 1)),((0 -1 1,0 1 1,-1 1 1,-1 -1 1,0 -1 1)),((-1 -1 1,-1 1 1,-1 1 -1,-1 -1 -1,-1 -1 1)),((-1 -1 -1,-1 1 -1,0 1 -1,0 -1 -1,-1 -1 -1)),((0 -1 -1,0 1 -1,1 1 -1,1 -1 -1,0 -1 -1)),((1 -1 -1,1 1 -1,0 1 2,0 -1 2,1 -1 -1)),((0 -1 2,0 1 2,0 1 1,0 -1 1,0 -1 2)))",
	    "POINT(0 0 0)",
	    0, zero_accepted_error);

	/* A point in the middle of a hole of extruded polygon */
	DIST3DTEST(
	    "POLYHEDRALSURFACE Z (((-3 -3 0,-3 3 0,3 3 0,3 -3 0,-3 -3 0),(-1 -1 0,1 -1 0,1 1 0,-1 1 0,-1 -1 0)),((-3 -3 3,3 -3 3,3 3 3,-3 3 3,-3 -3 3),(-1 -1 3,-1 1 3,1 1 3,1 -1 3,-1 -1 3)),((-3 -3 0,-3 -3 3,-3 3 3,-3 3 0,-3 -3 0)),((-3 3 0,-3 3 3,3 3 3,3 3 0,-3 3 0)),((3 3 0,3 3 3,3 -3 3,3 -3 0,3 3 0)),((3 -3 0,3 -3 3,-3 -3 3,-3 -3 0,3 -3 0)),((-1 -1 0,-1 -1 3,1 -1 3,1 -1 0,-1 -1 0)),((1 -1 0,1 -1 3,1 1 3,1 1 0,1 -1 0)),((1 1 0,1 1 3,-1 1 3,-1 1 0,1 1 0)),((-1 1 0,-1 1 3,-1 -1 3,-1 -1 0,-1 1 0)))",
	    "POINT(0 0 1)",
	    1, zero_accepted_error);

	/* A point at the face of a hole of extruded polygon */
	DIST3DTEST(
	    "POLYHEDRALSURFACE Z (((-3 -3 0,-3 3 0,3 3 0,3 -3 0,-3 -3 0),(-1 -1 0,1 -1 0,1 1 0,-1 1 0,-1 -1 0)),((-3 -3 3,3 -3 3,3 3 3,-3 3 3,-3 -3 3),(-1 -1 3,-1 1 3,1 1 3,1 -1 3,-1 -1 3)),((-3 -3 0,-3 -3 3,-3 3 3,-3 3 0,-3 -3 0)),((-3 3 0,-3 3 3,3 3 3,3 3 0,-3 3 0)),((3 3 0,3 3 3,3 -3 3,3 -3 0,3 3 0)),((3 -3 0,3 -3 3,-3 -3 3,-3 -3 0,3 -3 0)),((-1 -1 0,-1 -1 3,1 -1 3,1 -1 0,-1 -1 0)),((1 -1 0,1 -1 3,1 1 3,1 1 0,1 -1 0)),((1 1 0,1 1 3,-1 1 3,-1 1 0,1 1 0)),((-1 1 0,-1 1 3,-1 -1 3,-1 -1 0,-1 1 0)))",
	    "POINT(1 1 2)",
	    0, zero_accepted_error);

	/* A point at the face of a hole of extruded polygon */
	DIST3DTEST(
	    "LINESTRING Z (-27974.1264 -110211.5032 148.9768,-27975.4229 -110210.9441 149.0093)",
	    "LINESTRING Z (-27995.4183 -110201.8041 149.3354,-27975.4229 -110210.9441 149.0093)",
	    0, zero_accepted_error);
}

static int tree_pt(RECT_NODE *tree, double x, double y)
{
	POINT2D pt;
	pt.x = x; pt.y = y;
	return rect_tree_contains_point(tree, &pt);
}

static void test_rect_tree_contains_point(void)
{
	LWGEOM *poly;
	RECT_NODE* tree;

	/**********************************************************************
	* curvepolygon
	*/
	poly = lwgeom_from_wkt("CURVEPOLYGON(COMPOUNDCURVE(CIRCULARSTRING(0 0,1 5,0 10),(0 10,10 10,10 0, 0 0)),COMPOUNDCURVE(CIRCULARSTRING(3 7,5 8,7 7),(7 7,7 3,3 3, 3 7)))", LW_PARSER_CHECK_NONE);
	tree = rect_tree_from_lwgeom(poly);
	// char *wkt = rect_tree_to_wkt(tree);
	// printf("%s\n", wkt);
	// lwfree(wkt);
	// return;

	/* in hole, within arc stroke */
	CU_ASSERT_EQUAL(tree_pt(tree, 5, 7.5), 0);
	/* inside */
	CU_ASSERT_EQUAL(tree_pt(tree, 8, 9), 1);
	/* outside */
	CU_ASSERT_EQUAL(tree_pt(tree, -1, 5), 0);
	/* outside */
	CU_ASSERT_EQUAL(tree_pt(tree, -1, 7.5), 0);
	/* outside, within arc stroke */
	CU_ASSERT_EQUAL(tree_pt(tree, 0.2, 7.5), 0);
	/* inside, within arc stroke */
	CU_ASSERT_EQUAL(tree_pt(tree, 0.5, 0.5), 1);
	/* inside, crossing arc stroke */
	CU_ASSERT_EQUAL(tree_pt(tree, 2, 7.5), 1);
	/* touching hole corner */
	CU_ASSERT_EQUAL(tree_pt(tree, 7, 7), 1);

	rect_tree_free(tree);
	lwgeom_free(poly);


	/**********************************************************************
	* polygon with hole and concavities
	*/
	poly = lwgeom_from_wkt("POLYGON((0 0,0 10,10 10,10 0,9 0,9 9,8 6,8 0,2 0,2 9,1 6,1 0,0 0),(4 4,4 6,6 6,6 4,4 4))", LW_PARSER_CHECK_NONE);
	tree = rect_tree_from_lwgeom(poly);

	/* inside, many grazings */
	CU_ASSERT_EQUAL(tree_pt(tree, 3, 6), 1);
	/* inside */
	CU_ASSERT_EQUAL(tree_pt(tree, 3, 5.5), 1);
	/* outside */
	CU_ASSERT_EQUAL(tree_pt(tree, -3, 5.5), 0);
	/* touching interior ring */
	CU_ASSERT_EQUAL(tree_pt(tree, 4, 4), 1);
	CU_ASSERT_EQUAL(tree_pt(tree, 6, 6), 1);
	/* touching interior ring */
	CU_ASSERT_EQUAL(tree_pt(tree, 4.5, 4), 1);
	/* touching exterior ring */
	CU_ASSERT_EQUAL(tree_pt(tree, 8, 0), 1);
	CU_ASSERT_EQUAL(tree_pt(tree, 9, 0), 1);
	CU_ASSERT_EQUAL(tree_pt(tree, 10, 1), 1);
	CU_ASSERT_EQUAL(tree_pt(tree, 9.5, 1), 1);
	CU_ASSERT_EQUAL(tree_pt(tree, 0, 10), 1);
	/* touching grazing spike */
	CU_ASSERT_EQUAL(tree_pt(tree, 1, 6), 1);
	/* outide, many grazings */
	CU_ASSERT_EQUAL(tree_pt(tree, -1, 6), 0);
	/* within hole */
	CU_ASSERT_EQUAL(tree_pt(tree, 5, 5), 0);
	/* within */
	CU_ASSERT_EQUAL(tree_pt(tree, 0.5, 4), 1);
	CU_ASSERT_EQUAL(tree_pt(tree, 0.5, 6), 1);
	CU_ASSERT_EQUAL(tree_pt(tree, 0.5, 9), 1);

	rect_tree_free(tree);
	lwgeom_free(poly);

	/**********************************************************************
	* square
	*/
	poly = lwgeom_from_wkt("POLYGON((0 0, 0 1, 1 1, 1 0, 0 0))", LW_PARSER_CHECK_NONE);
	tree = rect_tree_from_lwgeom(poly);

	/* inside square */
	CU_ASSERT_EQUAL(tree_pt(tree, 0.5, 0.5), 1);
	/* outside square */
	CU_ASSERT_EQUAL(tree_pt(tree, 1.5, 0.5), 0);
	/* outside square grazing some edges */
	CU_ASSERT_EQUAL(tree_pt(tree, -1, 1), 0);
	/* inside square on corner */
	CU_ASSERT_EQUAL(tree_pt(tree, 1, 1), 1);
	/* inside square on top edge */
	CU_ASSERT_EQUAL(tree_pt(tree, 0.5, 1), 1);
	/* inside square on side edge */
	CU_ASSERT_EQUAL(tree_pt(tree, 1, 0.5), 1);

	rect_tree_free(tree);
	lwgeom_free(poly);

	/* ziggy zaggy horizontal saw tooth polygon */
	poly = lwgeom_from_wkt("POLYGON((0 0, 1 3, 2 0, 3 3, 4 0, 4 5, 0 5, 0 0))", LW_PARSER_CHECK_NONE);
	tree = rect_tree_from_lwgeom(poly);

	/* not in, left side */
	CU_ASSERT_EQUAL(tree_pt(tree, -0.5, 0.5), 0);
	/* not in, right side */
	CU_ASSERT_EQUAL(tree_pt(tree, 3, 1), 0);
	/* inside */
	CU_ASSERT_EQUAL(tree_pt(tree, 2, 1), 1);
	/* on left border */
	CU_ASSERT_EQUAL(tree_pt(tree, 0, 1), 1);
	/* on left border, grazing */
	CU_ASSERT_EQUAL(tree_pt(tree, 0, 3), 1);
	/* on right border */
	CU_ASSERT_EQUAL(tree_pt(tree, 4, 0), 1);
	/* on tooth concave */
	CU_ASSERT_EQUAL(tree_pt(tree, 3, 3), 1);
	/* on tooth convex */
	CU_ASSERT_EQUAL(tree_pt(tree, 2, 0), 1);

	rect_tree_free(tree);
	lwgeom_free(poly);

	/**********************************************************************
	* ziggy zaggy vertical saw tooth polygon
	*/
	poly = lwgeom_from_wkt("POLYGON((0 0, 3 1, 0 2, 3 3, 0 4, 3 5, 0 6, 5 6, 5 0, 0 0))", LW_PARSER_CHECK_NONE);
	tree = rect_tree_from_lwgeom(poly);

	/* not in, left side */
	CU_ASSERT_EQUAL(tree_pt(tree, -0.5, 3.5), 0);
	/* not in, right side */
	CU_ASSERT_EQUAL(tree_pt(tree, 6.0, 2.2), 0);
	/* inside */
	CU_ASSERT_EQUAL(tree_pt(tree, 3, 2), 1);
	/* on bottom border */
	CU_ASSERT_EQUAL(tree_pt(tree, 1, 0), 1);
	/* on top border */
	CU_ASSERT_EQUAL(tree_pt(tree, 3, 6), 1);
	/* on tooth concave */
	CU_ASSERT_EQUAL(tree_pt(tree, 3, 1), 1);
	/* on tooth convex */
	CU_ASSERT_EQUAL(tree_pt(tree, 0, 2), 1);
	/* on tooth convex */
	CU_ASSERT_EQUAL(tree_pt(tree, 0, 6), 1);

	rect_tree_free(tree);
	lwgeom_free(poly);

}

static int tree_inter(const char *wkt1, const char *wkt2)
{
	LWGEOM *g1 = lwgeom_from_wkt(wkt1, LW_PARSER_CHECK_NONE);
	LWGEOM *g2 = lwgeom_from_wkt(wkt2, LW_PARSER_CHECK_NONE);
	RECT_NODE *t1 = rect_tree_from_lwgeom(g1);
	RECT_NODE *t2 = rect_tree_from_lwgeom(g2);
	int result = rect_tree_intersects_tree(t1, t2);
	rect_tree_free(t1);
	rect_tree_free(t2);
	lwgeom_free(g1);
	lwgeom_free(g2);
	return result;
}

static void test_rect_tree_intersects_tree(void)
{
	/* total overlap, A == B */
	CU_ASSERT_EQUAL(tree_inter(
		"POLYGON((0 0, 3 1, 0 2, 3 3, 0 4, 3 5, 0 6, 5 6, 5 0, 0 0))",
		"POLYGON((0 0, 3 1, 0 2, 3 3, 0 4, 3 5, 0 6, 5 6, 5 0, 0 0))"),
		LW_TRUE
		);

	/* hiding between the tines of the comb */
	CU_ASSERT_EQUAL(tree_inter(
		"POLYGON((0 0, 3 1, 0 2, 3 3, 0 4, 3 5, 0 6, 5 6, 5 0, 0 0))",
		"POLYGON((0.3 0.7, 0.3 0.8, 0.4 0.8, 0.4 0.7, 0.3 0.7))"),
		LW_FALSE
		);

	/* between the tines, but with a corner overlapping */
	CU_ASSERT_EQUAL(tree_inter(
		"POLYGON((0 0, 3 1, 0 2, 3 3, 0 4, 3 5, 0 6, 5 6, 5 0, 0 0))",
		"POLYGON((0.3 0.7, 0.3 0.8, 0.4 0.8, 1.3 0.3, 0.3 0.7))"),
		LW_TRUE
		);

	/* Just touching the top left corner of the comb */
	CU_ASSERT_EQUAL(tree_inter(
		"POLYGON((0 0, 3 1, 0 2, 3 3, 0 4, 3 5, 0 6, 5 6, 5 0, 0 0))",
		"POLYGON((-1 5, 0 5, 0 7, -1 7, -1 5))"),
		LW_TRUE
		);

	/* Contained, complex */
	CU_ASSERT_EQUAL(tree_inter(
		"POLYGON((0 0, 3 1, 0 2, 3 3, 0 4, 3 5, 0 6, 5 6, 5 0, 0 0))",
		"GEOMETRYCOLLECTION(MULTILINESTRING((1 2, 3 2)),POINT(1 2))"),
		LW_TRUE
		);

	/* Touching, complex */
	CU_ASSERT_EQUAL(tree_inter(
		"POLYGON((0 0, 3 1, 0 2, 3 3, 0 4, 3 5, 0 6, 5 6, 5 0, 0 0))",
		"GEOMETRYCOLLECTION(MULTILINESTRING((6 3, 8 4)),POINT(5 3))"),
		LW_TRUE
		);

	/* Not Touching, complex */
	CU_ASSERT_EQUAL(tree_inter(
		"POLYGON((0 0, 3 1, 0 2, 3 3, 0 4, 3 5, 0 6, 5 6, 5 0, 0 0))",
		"GEOMETRYCOLLECTION(MULTILINESTRING((6 3, 8 4)),POINT(1 3.5))"),
		LW_FALSE
		);

	/* Crossing, complex */
	CU_ASSERT_EQUAL(tree_inter(
		"POLYGON((0 0, 3 1, 0 2, 3 3, 0 4, 3 5, 0 6, 5 6, 5 0, 0 0))",
		"GEOMETRYCOLLECTION(MULTILINESTRING((1.5 4.1, 1.6 2)),POINT(1 3.5))"),
		LW_TRUE
		);
}


static double
test_rect_tree_distance_tree_case(const char *wkt1, const char *wkt2)
{
	LWGEOM *lw1 = lwgeom_from_wkt(wkt1, LW_PARSER_CHECK_NONE);
	LWGEOM *lw2 = lwgeom_from_wkt(wkt2, LW_PARSER_CHECK_NONE);
	RECT_NODE *n1 = rect_tree_from_lwgeom(lw1);
	RECT_NODE *n2 = rect_tree_from_lwgeom(lw2);

	// rect_tree_printf(n1, 0);
	// rect_tree_printf(n2, 0);
	//
	// printf("%s\n", rect_tree_to_wkt(n1));
	// printf("%s\n", rect_tree_to_wkt(n2));

	double dist = rect_tree_distance_tree(n1, n2, 0.0);
	// printf("%g\n", dist);
	rect_tree_free(n1);
	rect_tree_free(n2);
	lwgeom_free(lw1);
	lwgeom_free(lw2);
	return dist;
}

#define TDT(w1, w2, d) CU_ASSERT_DOUBLE_EQUAL(test_rect_tree_distance_tree_case(w1, w2), d, 0.00001);

static void
test_rect_tree_distance_tree(void)
{
	const char *wkt;

	wkt = "MULTIPOLYGON(((-123.35702791281 48.4232302445918,-123.35689654493 48.4237265810249,-123.354053908057 48.4234039978588,-123.35417179975 48.4229151379279,-123.354369811539 48.4220987102936,-123.355779071731 48.4222571534228,-123.357238860904 48.4224209369449,-123.35702791281 48.4232302445918)))";
	TDT(wkt, "MULTIPOLYGON(((-123.353452578038 48.4259519079838,-123.35072012771 48.4256699150083,-123.347337809991 48.4254740864963,-123.347469111645 48.4245757659326,-123.349409235923 48.4246224093429,-123.349966167324 48.4246562342604,-123.353650661317 48.4250703224683,-123.353452578038 48.4259519079838)))", 0.0017144228293396);

	wkt = "POINT(0 0)";
	TDT(wkt, "MULTIPOINT(0 1.5,0 2,0 2.5)", 1.5);
	TDT(wkt, "GEOMETRYCOLLECTION(POINT(3 4))", 5.0);
	TDT(wkt, "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(POINT(3 4)))", 5.0);
	TDT(wkt, "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(POINT(3 4))))", 5.0);
	TDT(wkt, "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(MULTIPOINT(3 4))))", 5.0);

	TDT("LINESTRING(-2 0, -0.2 0)", "POINT(-2 0)", 0);
	TDT("LINESTRING(-0.2 0, -2 0)", "POINT(-2 0)", 0);
	TDT("LINESTRING(-1e-8 0, -0.2 0)", "POINT(-1e-8 0)", 0);
	TDT("LINESTRING(-0.2 0, -1e-8 0)", "POINT(-1e-8 0)", 0);

	wkt = "CURVEPOLYGON(COMPOUNDCURVE(CIRCULARSTRING(1 6, 6 1, 9 7),(9 7, 3 13, 1 6)),COMPOUNDCURVE((3 6, 5 4, 7 4, 7 6),CIRCULARSTRING(7 6,5 8,3 6)))";
	TDT(wkt, "POINT(3 14)", 1);
	TDT(wkt, "POINT(3 8)", 0);
	TDT(wkt, "POINT(6 5)", 1);
	TDT(wkt, "POINT(6 4)", 0);

	wkt = "MULTISURFACE(POLYGON((0 0,0 4,4 4,4 0,0 0)),CURVEPOLYGON(CIRCULARSTRING(8 2,10 4,12 2,10 0,8 2)))";
	TDT(wkt, "CURVEPOLYGON(CIRCULARSTRING(5 7,6 8,7 7,6 6,5 7))", 2.60555);
	TDT(wkt, "CURVEPOLYGON(CIRCULARSTRING(5 2,6 3,7 2,6 1,5 2))", 1);
	TDT(wkt, "CURVEPOLYGON(CIRCULARSTRING(4 2,5 3,6 2,5 1,4 2))", 0);
	TDT(wkt, "CURVEPOLYGON(CIRCULARSTRING(5 3,6 2,5 1,4 2,5 3))", 0);
	TDT(wkt, "CURVEPOLYGON(CIRCULARSTRING(4.5 3,5.5 2,4.5 1,3.5 2,4.5 3))", 0);
	TDT(wkt, "CURVEPOLYGON(CIRCULARSTRING(5.5 3,6.5 2,5.5 1,4.5 2,5.5 3))", 0.5);
	TDT(wkt, "CURVEPOLYGON(CIRCULARSTRING(10 3,11 2,10 1,9 2,10 3))", 0);
	TDT(wkt, "CURVEPOLYGON(CIRCULARSTRING(2 3,3 2,2 1,1 2,2 3))", 0);

	wkt = "CURVEPOLYGON(COMPOUNDCURVE(CIRCULARSTRING(0 0,5 0,0 0)))";
	TDT(wkt, "POINT(3 0)", 0.0);
	TDT(wkt, "POINT(5 0)", 0.0);
	TDT(wkt, "POINT(7 0)", 2.0);
	TDT(wkt, "POINT(2.5 3.5)", 1.0);

	wkt = "POINT(0 0)";
	TDT(wkt, "POINT(0 1)", 1.0);
	TDT(wkt, "POINT(1 0)", 1.0);

	wkt = "LINESTRING(0 0,1 0)";
	TDT(wkt, "LINESTRING(1 0,1 1)", 0.0);
	TDT(wkt, "LINESTRING(0 1,1 1)", 1.0);

	wkt = "POLYGON((0 0,0 1,1 1,1 0,0 0))";
	TDT(wkt, "POINT(2 2)", sqrt(2));
	TDT(wkt, "POINT(0.5 0.5)", 0);
	TDT(wkt, "POINT(1 1)", 0);

	wkt = "POLYGON((0 0,0 10,10 10,10 0,0 0), (4 4,4 6,6 6,6 4,4 4))";
	TDT(wkt, "POINT(5 5)", 1);
	TDT(wkt, "POLYGON((5 5,5 5.5,5.5 5.5,5.5 5, 5 5))", 0.5);
}


static void
test_lwgeom_segmentize2d(void)
{
	LWGEOM *linein = lwgeom_from_wkt("LINESTRING(0 0,10 0)", LW_PARSER_CHECK_NONE);
	LWGEOM *lineout = lwgeom_segmentize2d(linein, 5);
	char *strout = lwgeom_to_ewkt(lineout);
	ASSERT_STRING_EQUAL(strout, "LINESTRING(0 0,5 0,10 0)");
	lwfree(strout);
	lwgeom_free(linein);
	lwgeom_free(lineout);

	/* test that segmentize is proportional - request every 6, get every 5 */
	linein = lwgeom_from_wkt("LINESTRING(0 0, 20 0)", LW_PARSER_CHECK_NONE);
	lineout = lwgeom_segmentize2d(linein, 6);
	strout = lwgeom_to_ewkt(lineout);
	ASSERT_STRING_EQUAL(strout, "LINESTRING(0 0,5 0,10 0,15 0,20 0)");
	lwfree(strout);
	lwgeom_free(linein);
	lwgeom_free(lineout);

	/* test too many segments */
	linein = lwgeom_from_wkt("LINESTRING(0 0,10 0)", LW_PARSER_CHECK_NONE);
	lineout = lwgeom_segmentize2d(linein, 1e-100);
	CU_ASSERT_EQUAL(lineout, NULL);
	lwgeom_free(linein);

	/* test interruption */

	linein = lwgeom_from_wkt("LINESTRING(0 0,10 0)", LW_PARSER_CHECK_NONE);
	lwgeom_request_interrupt();
	lineout = lwgeom_segmentize2d(linein, INT32_MAX);
	CU_ASSERT_EQUAL(lineout, NULL);
	lwgeom_free(linein);

	linein = lwgeom_from_wkt("MULTILINESTRING((0 0,10 0),(20 0, 30 0))", LW_PARSER_CHECK_NONE);
	lwgeom_request_interrupt();
	lineout = lwgeom_segmentize2d(linein, INT32_MAX);
	CU_ASSERT_EQUAL(lineout, NULL);
	lwgeom_free(linein);

	linein = lwgeom_from_wkt(
	  "MULTIPOLYGON(((0 0,20 0,20 20,0 20,0 0),(2 2,2 4,4 4,4 2,2 2),(6 6,6 8,8 8,8 6,6 6)),((40 0,40 20,60 20,60 0,40 0),(42 2,42 4,44 4,44 2,42 2)))"
	  , LW_PARSER_CHECK_NONE);
	lwgeom_request_interrupt();
	lineout = lwgeom_segmentize2d(linein, INT32_MAX);
	CU_ASSERT_EQUAL(lineout, NULL);
	lwgeom_free(linein);

	linein = lwgeom_from_wkt(
	  "GEOMETRYCOLLECTION(MULTIPOLYGON(((0 0,20 0,20 20,0 20,0 0),(2 2,2 4,4 4,4 2,2 2),(6 6,6 8,8 8,8 6,6 6)),((40 0,40 20,60 20,60 0,40 0),(42 2,42 4,44 4,44 2,42 2))),MULTILINESTRING((0 0,10 0),(20 0, 30 0)),MULTIPOINT(0 0, 3 4))"
	  , LW_PARSER_CHECK_NONE);
	CU_ASSERT_FATAL(linein != NULL);
	lwgeom_request_interrupt();
	lineout = lwgeom_segmentize2d(linein, INT32_MAX);
	CU_ASSERT_EQUAL(lineout, NULL);
	lwgeom_free(linein);

	linein = lwgeom_from_wkt("LINESTRING(20 0, 30 0)", LW_PARSER_CHECK_NONE);
	CU_ASSERT_FATAL(linein != NULL);
	/* NOT INTERRUPTED */
	lineout = lwgeom_segmentize2d(linein, 5);
	CU_ASSERT_NOT_EQUAL_FATAL(lineout, NULL);
	strout = lwgeom_to_ewkt(lineout);
	ASSERT_STRING_EQUAL(strout, "LINESTRING(20 0,25 0,30 0)");
	lwfree(strout);
	lwgeom_free(linein);
	lwgeom_free(lineout);
}

static void
test_lwgeom_locate_along(void)
{
	LWGEOM *geom = NULL;
	LWGEOM *out = NULL;
	double measure = 105.0;
	char *str;

	/* ST_Locatealong(ST_GeomFromText('MULTILINESTRING M ((1 2 3, 5 4 5), (50 50 1, 60 60 200))'), 105) */
	geom = lwgeom_from_wkt("MULTILINESTRING M ((1 2 3, 5 4 5), (50 50 1, 60 60 200))", LW_PARSER_CHECK_NONE);
	out = lwgeom_locate_along(geom, measure, 0.0);
	str = lwgeom_to_wkt(out, WKT_ISO, 6, NULL);
	lwgeom_free(geom);
	lwgeom_free(out);
	ASSERT_STRING_EQUAL(str, "MULTIPOINT M (55.226131 55.226131 105)");
	lwfree(str);

	/* ST_Locatealong(ST_GeomFromText('MULTILINESTRING M ((1 2 3, 5 4 5), (50 50 1, 60 60 200))'), 105) */
	geom = lwgeom_from_wkt("MULTILINESTRING M ((1 2 3, 3 4 2, 9 4 3), (1 2 3, 5 4 5), (50 50 1, 60 60 200))", LW_PARSER_CHECK_NONE);
	out = lwgeom_locate_along(geom, measure, 0.0);
	str = lwgeom_to_wkt(out, WKT_ISO, 6, NULL);
	lwgeom_free(geom);
	lwgeom_free(out);
	ASSERT_STRING_EQUAL(str, "MULTIPOINT M (55.226131 55.226131 105)");
	lwfree(str);
}

static void
test_lw_dist2d_pt_arc(void)
{
	/* int lw_dist2d_pt_arc(const POINT2D* P, const POINT2D* A1, const POINT2D* A2, const POINT2D* A3, DISTPTS* dl) */
	DISTPTS dl;
	POINT2D P, A1, A2, A3;
	int rv;


	/* Point within unit semicircle, 0.5 units from arc */
	A1.x = -1; A1.y = 0;
	A2.x = 0 ; A2.y = 1;
	A3.x = 1 ; A3.y = 0;
	P.x  = 0 ; P.y  = 0.5;

	lw_dist2d_distpts_init(&dl, DIST_MIN);
	rv = lw_dist2d_pt_arc(&P, &A1, &A2, &A3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 0.5, 0.000001);

	/* Point outside unit semicircle, 0.5 units from arc */
	P.x  = 0 ; P.y  = 1.5;
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	rv = lw_dist2d_pt_arc(&P, &A1, &A2, &A3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 0.5, 0.000001);

	/* Point outside unit semicircle, sqrt(2) units from arc end point*/
	P.x  = 0 ; P.y  = -1;
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	rv = lw_dist2d_pt_arc(&P, &A1, &A2, &A3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, sqrt(2.0), 0.000001);

	/* Point outside unit semicircle, sqrt(2)-1 units from arc end point*/
	P.x  = 1 ; P.y  = 1;
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	rv = lw_dist2d_pt_arc(&P, &A1, &A2, &A3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, sqrt(2.0)-1, 0.000001);

	/* Point on unit semicircle midpoint */
	P.x  = 0 ; P.y  = 1;
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	rv = lw_dist2d_pt_arc(&P, &A1, &A2, &A3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 0, 0.000001);

	/* Point on unit semicircle endpoint */
	P.x  = 1 ; P.y  = 0;
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	rv = lw_dist2d_pt_arc(&P, &A1, &A2, &A3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 0, 0.000001);

	/* Point on semicircle center */
	P.x  = 0 ; P.y  = 0;
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	rv = lw_dist2d_pt_arc(&P, &A1, &A2, &A3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 1, 0.000001);

	/* Point inside closed circle */
	P.x  = 0 ; P.y  = 0.5;
	A2.x = 1; A2.y = 0;
	A3 = A1;
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	rv = lw_dist2d_pt_arc(&P, &A1, &A2, &A3, &dl);
	//printf("distance %g\n", dl.distance);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 0.5, 0.000001);
}

static void
test_lw_dist2d_seg_arc(void)
{
	/* int lw_dist2d_seg_arc(const POINT2D *A1, const POINT2D *A2, const POINT2D *B1, const POINT2D *B2, const POINT2D *B3, DISTPTS *dl) */

	DISTPTS dl;
	POINT2D A1, A2, B1, B2, B3;
	int rv;

	/* Unit semicircle */
	B1.x = -1; B1.y = 0;
	B2.x = 0 ; B2.y = 1;
	B3.x = 1 ; B3.y = 0;

	/* Edge above the unit semicircle */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = -2; A1.y = 2;
	A2.x = 2 ; A2.y = 2;
	rv = lw_dist2d_seg_arc(&A1, &A2, &B1, &B2, &B3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 1, 0.000001);

	/* Edge to the right of the unit semicircle */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = 2; A1.y = -2;
	A2.x = 2; A2.y = 2;
	rv = lw_dist2d_seg_arc(&A1, &A2, &B1, &B2, &B3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 1, 0.000001);

	/* Edge to the left of the unit semicircle */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = -2; A1.y = -2;
	A2.x = -2; A2.y = 2;
	rv = lw_dist2d_seg_arc(&A1, &A2, &B1, &B2, &B3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 1, 0.000001);

	/* Edge within the unit semicircle */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = 0; A1.y = 0;
	A2.x = 0; A2.y = 0.5;
	rv = lw_dist2d_seg_arc(&A1, &A2, &B1, &B2, &B3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 0.5, 0.000001);

	/* Edge grazing the unit semicircle */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = -2; A1.y = 1;
	A2.x =  2; A2.y = 1;
	rv = lw_dist2d_seg_arc(&A1, &A2, &B1, &B2, &B3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 0., 0.000001);

	/* Line grazing the unit semicircle, but edge not */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = 1; A1.y = 1;
	A2.x = 2; A2.y = 1;
	rv = lw_dist2d_seg_arc(&A1, &A2, &B1, &B2, &B3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, sqrt(2.0)-1, 0.000001);

	/* Edge intersecting the unit semicircle */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = 0; A1.y = 0;
	A2.x = 2; A2.y = 2;
	rv = lw_dist2d_seg_arc(&A1, &A2, &B1, &B2, &B3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 0, 0.000001);

	/* Line intersecting the unit semicircle, but edge not */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = -1; A1.y = 1;
	A2.x = -2; A2.y = 2;
	rv = lw_dist2d_seg_arc(&A1, &A2, &B1, &B2, &B3, &dl);
	//printf("distance %g\n", dl.distance);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, sqrt(2.0)-1, 0.000001);
}

static void
test_lw_dist2d_arc_arc(void)
{
	/* lw_dist2d_arc_arc(const POINT2D *A1, const POINT2D *A2, const POINT2D *A3,
	                     const POINT2D *B1, const POINT2D *B2, const POINT2D *B3,
	                     DISTPTS *dl) */
	DISTPTS dl;
	POINT2D A1, A2, A3, B1, B2, B3;
	int rv;

	/* Ticket #4326 */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = -1.0; A1.y =  4.0;
	A2.x =  0.0; A2.y =  5.0;
	A3.x =  1.0; A3.y =  4.0;
	B1.x =  1.0; B1.y =  6.0;
	B2.x =  6.0; B2.y =  1.0;
	B3.x =  9.0; B3.y =  7.0;
	rv = lw_dist2d_arc_arc(&A1, &A2, &A3, &B1, &B2, &B3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 0.0475666, 0.000001);

	/* Unit semicircle at 0,0 */
	B1.x = -1; B1.y = 0;
	B2.x = 0 ; B2.y = 1;
	B3.x = 1 ; B3.y = 0;

	/* Arc above the unit semicircle */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = -1; A1.y = 3;
	A2.x = 0 ; A2.y = 2;
	A3.x = 1 ; A3.y = 3;
	rv = lw_dist2d_arc_arc(&A1, &A2, &A3, &B1, &B2, &B3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 1, 0.000001);

	/* Arc grazes the unit semicircle */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = -1; A1.y = 2;
	A2.x = 0 ; A2.y = 1;
	A3.x = 1 ; A3.y = 2;
	rv = lw_dist2d_arc_arc(&A1, &A2, &A3, &B1, &B2, &B3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 0, 0.000001);

	/* Circles intersect, but arcs do not */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = -1; A1.y =  1;
	A2.x =  0; A2.y =  2;
	A3.x =  1; A3.y =  1;
	rv = lw_dist2d_arc_arc(&A1, &A2, &A3, &B1, &B2, &B3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, sqrt(2)-1, 0.000001);

	/* Circles and arcs intersect */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = -1; A1.y =  1;
	A2.x =  0; A2.y =  0;
	A3.x =  1; A3.y =  1;
	rv = lw_dist2d_arc_arc(&A1, &A2, &A3, &B1, &B2, &B3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 0, 0.000001);

	/* Concentric: and fully parallel */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = -2.0; A1.y = 0.0;
	A2.x =  0.0; A2.y = 2.0;
	A3.x =  2.0; A3.y = 0.0;
	rv = lw_dist2d_arc_arc(&A1, &A2, &A3, &B1, &B2, &B3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 1.0, 0.000001);

	/* Concentric: with A fully included in B's range */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = -0.5 / sqrt(2.0); A1.y =  0.5 / sqrt(2.0);
	A2.x =  0.0;             A2.y =  0.5;
	A3.x =  0.5 / sqrt(2.0); A3.y =  0.5 / sqrt(2.0);
	rv = lw_dist2d_arc_arc(&A1, &A2, &A3, &B1, &B2, &B3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 0.5, 0.000001);

	/* Concentric: with A partially included in B's range */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = -0.5 / sqrt(2.0); A1.y = -0.5 / sqrt(2.0);
	A2.x = -0.5;             A2.y =  0.0;
	A3.x = -0.5 / sqrt(2.0); A3.y = 0.5 / sqrt(2.0);
	rv = lw_dist2d_arc_arc(&A1, &A2, &A3, &B1, &B2, &B3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 0.5, 0.000001);

	/* Concentric: with A and B without parallel segments */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = -0.5 / sqrt(2.0); A1.y = -0.5 / sqrt(2.0);
	A2.x =  0.0;             A2.y = -0.5;
	A3.x =  0.5 / sqrt(2.0); A3.y = -0.5 / sqrt(2.0);
	rv = lw_dist2d_arc_arc(&A1, &A2, &A3, &B1, &B2, &B3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 0.736813, 0.000001);

	/* Concentric: Arcs are the same */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = -1.0; A1.y =  0.0;
	A2.x =  0.0; A2.y =  1.0;
	A3.x =  1.0; A3.y =  0.0;
	rv = lw_dist2d_arc_arc(&A1, &A2, &A3, &B1, &B2, &B3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 0.0, 0.000001);

	/* Check different orientations */
	B1.x = -10.0;  B1.y =  0.0;
	B2.x =   0.0 ; B2.y = 10.0;
	B3.x =  10.0 ; B3.y =  0.0;

	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = -22.0; A1.y =   0.0;
	A2.x = -17.0; A2.y =  -5.0;
	A3.x = -12.0; A3.y =   0.0;
	rv = lw_dist2d_arc_arc(&A1, &A2, &A3, &B1, &B2, &B3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 2.0, 0.000001);

	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = -19.0; A1.y =   0.0;
	A2.x = -14.0; A2.y =  -5.0;
	A3.x = - 9.0; A3.y =   0.0;
	rv = lw_dist2d_arc_arc(&A1, &A2, &A3, &B1, &B2, &B3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 1.0, 0.000001);

	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = -9.0;  A1.y =   0.0;
	A2.x = -4.0;  A2.y =  -5.0;
	A3.x =  1.0;  A3.y =   0.0;
	rv = lw_dist2d_arc_arc(&A1, &A2, &A3, &B1, &B2, &B3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 1.0, 0.000001);

	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = -1.0;  A1.y =   0.0;
	A2.x =  4.0;  A2.y =  -5.0;
	A3.x =  9.0;  A3.y =   0.0;
	rv = lw_dist2d_arc_arc(&A1, &A2, &A3, &B1, &B2, &B3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 1.0, 0.000001);

	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x =  1.0;  A1.y =   0.0;
	A2.x =  6.0;  A2.y =  -5.0;
	A3.x = 11.0;  A3.y =   0.0;
	rv = lw_dist2d_arc_arc(&A1, &A2, &A3, &B1, &B2, &B3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 1.0, 0.000001);

	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = 11.0;  A1.y =   0.0;
	A2.x = 16.0;  A2.y =  -5.0;
	A3.x = 21.0;  A3.y =   0.0;
	rv = lw_dist2d_arc_arc(&A1, &A2, &A3, &B1, &B2, &B3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 1.0, 0.000001);


	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = -15.0; A1.y =  -6.0;
	A2.x = -10.0; A2.y =  -1.0;
	A3.x = - 5.0; A3.y =  -6.0;
	rv = lw_dist2d_arc_arc(&A1, &A2, &A3, &B1, &B2, &B3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 1.0, 0.000001);

	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = -5.0; A1.y =  0.0;
	A2.x =  0.0; A2.y =  5.0;
	A3.x =  5.0; A3.y =  0.0;
	rv = lw_dist2d_arc_arc(&A1, &A2, &A3, &B1, &B2, &B3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 5.0, 0.000001);

	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = -5.0; A1.y =  0.0;
	A2.x =  0.0; A2.y = -5.0;
	A3.x =  5.0; A3.y =  0.0;
	rv = lw_dist2d_arc_arc(&A1, &A2, &A3, &B1, &B2, &B3, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 5.0, 0.000001);

}

static void
test_lw_arc_length(void)
{
/* double lw_arc_length(const POINT2D *A1, const POINT2D *A2, const POINT2D *A3) */

	POINT2D A1, A2, A3;
	double d;

	/* Unit semicircle at 0,0 */
	A1.x = -1; A1.y = 0;
	A2.x = 0 ; A2.y = 1;
	A3.x = 1 ; A3.y = 0;

	/* Arc above the unit semicircle */
	d = lw_arc_length(&A1, &A2, &A3);
	CU_ASSERT_DOUBLE_EQUAL(d, M_PI, 0.000001);
	d = lw_arc_length(&A3, &A2, &A1);
	CU_ASSERT_DOUBLE_EQUAL(d, M_PI, 0.000001);

	/* Unit semicircle at 0,0 */
	A1.x = 0; A1.y = 1;
	A2.x = 1; A2.y = 0;
	A3.x = 0; A3.y = -1;

	/* Arc to right of the unit semicircle */
	d = lw_arc_length(&A1, &A2, &A3);
	CU_ASSERT_DOUBLE_EQUAL(d, M_PI, 0.000001);
	d = lw_arc_length(&A3, &A2, &A1);
	CU_ASSERT_DOUBLE_EQUAL(d, M_PI, 0.000001);

	/* Unit 3/4 circle at 0,0 */
	A1.x = -1; A1.y = 0;
	A2.x = 1; A2.y = 0;
	A3.x = 0; A3.y = -1;

	/* Arc to right of the unit semicircle */
	d = lw_arc_length(&A1, &A2, &A3);
	CU_ASSERT_DOUBLE_EQUAL(d, 3*M_PI_2, 0.000001);
	d = lw_arc_length(&A3, &A2, &A1);
	CU_ASSERT_DOUBLE_EQUAL(d, 3*M_PI_2, 0.000001);
}

static void
test_lw_dist2d_pt_ptarrayarc(void)
{
	/* lw_dist2d_pt_ptarrayarc(const POINT2D *p, const POINTARRAY *pa, DISTPTS *dl) */
	DISTPTS dl;
	int rv;
	LWLINE *lwline;
	POINT2D P;

	/* Unit semi-circle above X axis */
	lwline = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(-1 0, 0 1, 1 0)"));

	/* Point at origin */
	P.x = P.y = 0;
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	rv = lw_dist2d_pt_ptarrayarc(&P, lwline->points, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 1, 0.000001);

	/* Point above arc on Y axis */
	P.y = 2;
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	rv = lw_dist2d_pt_ptarrayarc(&P, lwline->points, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 1, 0.000001);

	/* Point 45 degrees off arc, 2 radii from center */
	P.y = P.x = 2 * cos(M_PI_4);
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	rv = lw_dist2d_pt_ptarrayarc(&P, lwline->points, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 1, 0.000001);

	/* Four unit semi-circles surrounding the 2x2 box around origin */
	lwline_free(lwline);
	lwline = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(-1 -1, -2 0, -1 1, 0 2, 1 1, 2 0, 1 -1, 0 -2, -1 -1)"));

	/* Point at origin */
	P.x = P.y = 0;
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	rv = lw_dist2d_pt_ptarrayarc(&P, lwline->points, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, sqrt(2.0), 0.000001);

	/* Point on box edge */
	P.x = -1; P.y = 0;
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	rv = lw_dist2d_pt_ptarrayarc(&P, lwline->points, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 1, 0.000001);

	/* Point within a semicircle lobe */
	P.x = -1.5; P.y = 0;
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	rv = lw_dist2d_pt_ptarrayarc(&P, lwline->points, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 0.5, 0.000001);

	/* Point outside a semicircle lobe */
	P.x = -2.5; P.y = 0;
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	rv = lw_dist2d_pt_ptarrayarc(&P, lwline->points, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 0.5, 0.000001);

	/* Point outside a semicircle lobe */
	P.y = -2.5; P.x = 0;
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	rv = lw_dist2d_pt_ptarrayarc(&P, lwline->points, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 0.5, 0.000001);

	/* Point outside a semicircle lobe */
	P.y = 2; P.x = 1;
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	rv = lw_dist2d_pt_ptarrayarc(&P, lwline->points, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, sqrt(2.0)-1.0, 0.000001);

	/* Clean up */
	lwline_free(lwline);
}

static void
test_lw_dist2d_ptarray_ptarrayarc(void)
{
	/* int lw_dist2d_ptarray_ptarrayarc(const POINTARRAY *pa, const POINTARRAY *pb, DISTPTS *dl) */
	DISTPTS dl;
	int rv;
	LWLINE *lwline1;
	LWLINE *lwline2;

	/* Unit semi-circle above X axis */
	lwline1 = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(-1 0, 0 1, 1 0)"));

	/* Line above top of semi-circle */
	lwline2 = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(-2 2, -1 2, 1 2, 2 2)"));
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	rv = lw_dist2d_ptarray_ptarrayarc(lwline2->points, lwline1->points, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 1, 0.000001);

	/* Reversed arguments, should fail */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	cu_error_msg_reset();
	rv = lw_dist2d_ptarray_ptarrayarc(lwline1->points, lwline2->points, &dl);
	//printf("%s\n", cu_error_msg);
	CU_ASSERT_EQUAL( rv, LW_FAILURE );
	ASSERT_STRING_EQUAL(
	    cu_error_msg,
	    "lw_dist2d_ptarray_ptarrayarc called with non-arc input");

	lwline_free(lwline2);

	/* Line along side of semi-circle */
	lwline2 = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(-2 -3, -2 -2, -2 2, -2 3)"));
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	rv = lw_dist2d_ptarray_ptarrayarc(lwline2->points, lwline1->points, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 1, 0.000001);

	/* Four unit semi-circles surrounding the 2x2 box around origin */
	lwline_free(lwline1);
	lwline_free(lwline2);
	lwline1 = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(-1 -1, -2 0, -1 1, 0 2, 1 1, 2 0, 1 -1, 0 -2, -1 -1)"));
	lwline2 = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(-2.5 -3, -2.5 -2, -2.5 2, -2.5 3)"));
	rv = lw_dist2d_ptarray_ptarrayarc(lwline2->points, lwline1->points, &dl);
	CU_ASSERT_EQUAL( rv, LW_SUCCESS );
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 0.5, 0.000001);

	lwline_free(lwline2);
	lwline_free(lwline1);
}

static void
test_lwgeom_tcpa(void)
{
	LWGEOM *g1, *g2;
	double m, dist;

	/* Invalid input, lack of dimensions */

	g1 = lwgeom_from_wkt("LINESTRING M(0 0 1, 0 0 1)", LW_PARSER_CHECK_NONE);
	g2 = lwgeom_from_wkt("LINESTRING (0 0 2, 0 0 5)", LW_PARSER_CHECK_NONE);
	m = lwgeom_tcpa(g1, g2, NULL);
	lwgeom_free(g1);
	lwgeom_free(g2);
	ASSERT_DOUBLE_EQUAL(m, -1.0);
	ASSERT_STRING_EQUAL(
	    cu_error_msg,
	    "Both input geometries must have a measure dimension");

	/* Invalid input, not linestrings */

	g1 = lwgeom_from_wkt("LINESTRING M(0 0 1, 0 0 1)", LW_PARSER_CHECK_NONE);
	g2 = lwgeom_from_wkt("POINT M (0 0 2)", LW_PARSER_CHECK_NONE);
	m = lwgeom_tcpa(g1, g2, NULL);
	lwgeom_free(g1);
	lwgeom_free(g2);
	ASSERT_DOUBLE_EQUAL(m, -1.0);
	ASSERT_STRING_EQUAL(cu_error_msg,
			    "Both input geometries must be linestrings");

	/* Invalid input, too short linestring */

	g1 = lwgeom_from_wkt("LINESTRING M(0 0 1, 10 0 10)", LW_PARSER_CHECK_NONE);
	g2 = lwgeom_from_wkt("LINESTRING M(2 0 1)", LW_PARSER_CHECK_NONE);
	dist = -77;
	m = lwgeom_tcpa(g1, g2, &dist);
	lwgeom_free(g1);
	lwgeom_free(g2);
	ASSERT_DOUBLE_EQUAL(dist, -77.0); /* not touched */
	ASSERT_DOUBLE_EQUAL(m, -1.0);
	ASSERT_STRING_EQUAL(
	    cu_error_msg,
	    "Both input lines must have at least 2 points" /* should be accepted
							      ? */

	);

	/* Invalid input, empty linestring */

	g1 = lwgeom_from_wkt("LINESTRING M(0 0 1, 10 0 10)", LW_PARSER_CHECK_NONE);
	g2 = lwgeom_from_wkt("LINESTRING M EMPTY", LW_PARSER_CHECK_NONE);
	m = lwgeom_tcpa(g1, g2, NULL);
	lwgeom_free(g1);
	lwgeom_free(g2);
	ASSERT_DOUBLE_EQUAL(m, -1.0);
	ASSERT_STRING_EQUAL(cu_error_msg,
			    "Both input lines must have at least 2 points"

	);

	/* Timeranges do not overlap */

	g1 = lwgeom_from_wkt("LINESTRING M(0 0 1, 0 0 1)", LW_PARSER_CHECK_NONE);
	g2 = lwgeom_from_wkt("LINESTRING M(0 0 2, 0 0 5)", LW_PARSER_CHECK_NONE);
	m = lwgeom_tcpa(g1, g2, NULL);
	lwgeom_free(g1);
	lwgeom_free(g2);
	ASSERT_DOUBLE_EQUAL(m, -2.0); /* means timeranges do not overlap */

	/* One of the tracks is still, the other passes to that point */

	g1 = lwgeom_from_wkt("LINESTRING M(0 0 1, 0 0 1)", LW_PARSER_CHECK_NONE);
	g2 = lwgeom_from_wkt("LINESTRING M(0 0 1, 0 0 5)", LW_PARSER_CHECK_NONE);
	dist = -1;
	m = lwgeom_tcpa(g1, g2, &dist);
	ASSERT_DOUBLE_EQUAL(m, 1.0);
	ASSERT_DOUBLE_EQUAL(dist, 0.0);
	CU_ASSERT( lwgeom_cpa_within(g1, g2, 0.0) == LW_TRUE );
	lwgeom_free(g1);
	lwgeom_free(g2);

	/* One of the tracks is still, the other passes at 10 meters from point */

	g1 = lwgeom_from_wkt("LINESTRING M(0 0 1, 0 0 5)", LW_PARSER_CHECK_NONE);
	g2 = lwgeom_from_wkt("LINESTRING M(-10 10 1, 10 10 5)", LW_PARSER_CHECK_NONE);
	dist = -1;
	m = lwgeom_tcpa(g1, g2, &dist);
	ASSERT_DOUBLE_EQUAL(m, 3.0);
	ASSERT_DOUBLE_EQUAL(dist, 10.0);
	CU_ASSERT( lwgeom_cpa_within(g1, g2, 11.0) == LW_TRUE );
	CU_ASSERT( lwgeom_cpa_within(g1, g2, 10.0) == LW_TRUE );
	CU_ASSERT( lwgeom_cpa_within(g1, g2, 9.0) == LW_FALSE );
	lwgeom_free(g1);
	lwgeom_free(g2);

	/* Equal tracks, 2d */

	g1 = lwgeom_from_wkt("LINESTRING M(0 0 10, 10 0 20)", LW_PARSER_CHECK_NONE);
	g2 = lwgeom_from_wkt("LINESTRING M(0 0 10, 10 0 20)", LW_PARSER_CHECK_NONE);
	dist = -1;
	m = lwgeom_tcpa(g1, g2, &dist);
	lwgeom_free(g1);
	lwgeom_free(g2);
	ASSERT_DOUBLE_EQUAL(m, 10.0);
	ASSERT_DOUBLE_EQUAL(dist, 0.0);

	/* Reversed tracks, 2d */

	g1 = lwgeom_from_wkt("LINESTRING M(0 0 10, 10 0 20)", LW_PARSER_CHECK_NONE);
	g2 = lwgeom_from_wkt("LINESTRING M(10 0 10, 0 0 20)", LW_PARSER_CHECK_NONE);
	dist = -1;
	m = lwgeom_tcpa(g1, g2, &dist);
	lwgeom_free(g1);
	lwgeom_free(g2);
	ASSERT_DOUBLE_EQUAL(m, 15.0);
	ASSERT_DOUBLE_EQUAL(dist, 0.0);

	/*  Parallel tracks, same speed, 2d */

	g1 = lwgeom_from_wkt("LINESTRING M(2 0 10, 12 0 20)", LW_PARSER_CHECK_NONE);
	g2 = lwgeom_from_wkt("LINESTRING M(13 0 10, 23 0 20)", LW_PARSER_CHECK_NONE);
	dist = -1;
	m = lwgeom_tcpa(g1, g2, &dist);
	lwgeom_free(g1);
	lwgeom_free(g2);
	ASSERT_DOUBLE_EQUAL(m, 10.0);
	ASSERT_DOUBLE_EQUAL(dist, 11.0);

	/*  Parallel tracks, different speed (g2 gets closer as time passes), 2d */

	g1 = lwgeom_from_wkt("LINESTRING M(4 0 10, 10 0 20)", LW_PARSER_CHECK_NONE);
	g2 = lwgeom_from_wkt("LINESTRING M(2 0 10,  9 0 20)", LW_PARSER_CHECK_NONE);
	dist = -1;
	m = lwgeom_tcpa(g1, g2, &dist);
	lwgeom_free(g1);
	lwgeom_free(g2);
	ASSERT_DOUBLE_EQUAL(m, 20.0);
	ASSERT_DOUBLE_EQUAL(dist, 1.0);

	/*  Parallel tracks, different speed (g2 left behind as time passes), 2d */

	g1 = lwgeom_from_wkt("LINESTRING M(4 0 10, 10 0 20)", LW_PARSER_CHECK_NONE);
	g2 = lwgeom_from_wkt("LINESTRING M(2 0 10,  6 0 20)", LW_PARSER_CHECK_NONE);
	dist = -1;
	m = lwgeom_tcpa(g1, g2, &dist);
	lwgeom_free(g1);
	lwgeom_free(g2);
	ASSERT_DOUBLE_EQUAL(m, 10.0);
	ASSERT_DOUBLE_EQUAL(dist, 2.0);

	/* Tracks, colliding, 2d */

	g1 = lwgeom_from_wkt("LINESTRING M(0 0 0, 10 0 10)", LW_PARSER_CHECK_NONE);
	g2 = lwgeom_from_wkt("LINESTRING M(5 -8 0,  5 8 10)", LW_PARSER_CHECK_NONE);
	dist = -1;
	m = lwgeom_tcpa(g1, g2, &dist);
	lwgeom_free(g1);
	lwgeom_free(g2);
	ASSERT_DOUBLE_EQUAL(m, 5.0);
	ASSERT_DOUBLE_EQUAL(dist, 0.0);

	/* Tracks crossing, NOT colliding, 2d */

	g1 = lwgeom_from_wkt("LINESTRING M(0 0 0, 10 0 10)", LW_PARSER_CHECK_NONE);
	g2 = lwgeom_from_wkt("LINESTRING M(8 -5 0,  8 5 10)", LW_PARSER_CHECK_NONE);
	dist = -1;
	m = lwgeom_tcpa(g1, g2, &dist);
	lwgeom_free(g1);
	lwgeom_free(g2);
	ASSERT_DOUBLE_EQUAL(m, 6.5);
	ASSERT_DOUBLE_EQUAL(rint(dist*100), 212.0);

	/*  Same origin, different direction, 2d */

	g1 = lwgeom_from_wkt("LINESTRING M(0 0 1, 10 0 10)", LW_PARSER_CHECK_NONE);
	g2 = lwgeom_from_wkt("LINESTRING M(0 0 1, -100 0 10)", LW_PARSER_CHECK_NONE);
	dist = -1;
	m = lwgeom_tcpa(g1, g2, &dist);
	lwgeom_free(g1);
	lwgeom_free(g2);
	ASSERT_DOUBLE_EQUAL(m, 1.0);
	ASSERT_DOUBLE_EQUAL(dist, 0.0);

	/*  Same ending, different direction, 2d */

	g1 = lwgeom_from_wkt("LINESTRING M(10 0 1, 0 0 10)", LW_PARSER_CHECK_NONE);
	g2 = lwgeom_from_wkt("LINESTRING M(0 -100 1, 0 0 10)", LW_PARSER_CHECK_NONE);
	dist = -1;
	m = lwgeom_tcpa(g1, g2, &dist);
	lwgeom_free(g1);
	lwgeom_free(g2);
	ASSERT_DOUBLE_EQUAL(m, 10.0);
	ASSERT_DOUBLE_EQUAL(dist, 0.0);

	/* Converging tracks, 3d */

	g1 = lwgeom_from_wkt("LINESTRING ZM(0 0 0 10, 10 0 0 20)", LW_PARSER_CHECK_NONE);
	g2 = lwgeom_from_wkt("LINESTRING ZM(0 0 8 10, 10 0 5 20)", LW_PARSER_CHECK_NONE);
	dist = -1;
	m = lwgeom_tcpa(g1, g2, &dist);
	lwgeom_free(g1);
	lwgeom_free(g2);
	ASSERT_DOUBLE_EQUAL(m, 20.0);
	ASSERT_DOUBLE_EQUAL(dist, 5.0);

	/* G1 stops at t=1 until t=4 to let G2 pass by, then continues */
	/* G2 passes at 1 meter from G1 t=3 */

	g1 = lwgeom_from_wkt("LINESTRING M(0 0 0, 0 1 1, 0 1 4, 0 10 13)", LW_PARSER_CHECK_NONE);
	g2 = lwgeom_from_wkt("LINESTRING M(-10 2 0, 0 2 3, 12 2 13)", LW_PARSER_CHECK_NONE);
	dist = -1;
	m = lwgeom_tcpa(g1, g2, &dist);
	lwgeom_free(g1);
	lwgeom_free(g2);
	ASSERT_DOUBLE_EQUAL(m, 3.0);
	ASSERT_DOUBLE_EQUAL(dist, 1.0);

	/* Test for https://trac.osgeo.org/postgis/ticket/3136 */

	g1 = lwgeom_from_wkt("LINESTRING M (0 0 1432291464,2 0 1432291536) ", LW_PARSER_CHECK_NONE);
	g2 = lwgeom_from_wkt("LINESTRING M (0 0 1432291464,1 0 1432291466.25,2 0 1432291500)", LW_PARSER_CHECK_NONE);
	dist = -1;
	m = lwgeom_tcpa(g1, g2, &dist);
	lwgeom_free(g1);
	lwgeom_free(g2);
	ASSERT_DOUBLE_EQUAL(m, 1432291464.0);
	ASSERT_DOUBLE_EQUAL(dist, 0.0);

	/* Tracks share a single point in time */

	g1 = lwgeom_from_wkt("LINESTRINGM(0 0 0, 1 0 2)", LW_PARSER_CHECK_NONE);
	g2 = lwgeom_from_wkt("LINESTRINGM(0 0 2, 1 0 3)", LW_PARSER_CHECK_NONE);
	dist = -1;
	m = lwgeom_tcpa(g1, g2, &dist);
	lwgeom_free(g1);
	lwgeom_free(g2);
	ASSERT_DOUBLE_EQUAL(m, 2.0);
	ASSERT_DOUBLE_EQUAL(dist, 1.0);

}

static void
test_lwgeom_is_trajectory(void)
{
	LWGEOM *g;
	int ret;

	g = lwgeom_from_wkt("POINT M(0 0 1)", LW_PARSER_CHECK_NONE);
	ret = lwgeom_is_trajectory(g);
	lwgeom_free(g);
	ASSERT_INT_EQUAL(ret, LW_FALSE); /* not a linestring */

	g = lwgeom_from_wkt("LINESTRING Z(0 0 1, 0 0 1)", LW_PARSER_CHECK_NONE);
	ret = lwgeom_is_trajectory(g);
	lwgeom_free(g);
	ASSERT_INT_EQUAL(ret, LW_FALSE); /* no measure */

	g = lwgeom_from_wkt("LINESTRING M(0 0 1, 0 0 1)", LW_PARSER_CHECK_NONE);
	ret = lwgeom_is_trajectory(g);
	lwgeom_free(g);
	ASSERT_INT_EQUAL(ret, LW_FALSE); /* same measure in two points */

	g = lwgeom_from_wkt("LINESTRING M(0 0 1, 0 0 0)", LW_PARSER_CHECK_NONE);
	ret = lwgeom_is_trajectory(g);
	lwgeom_free(g);
	ASSERT_INT_EQUAL(ret, LW_FALSE); /* backward measure */

	g = lwgeom_from_wkt("LINESTRING M(0 0 1, 1 0 3, 2 2 2)", LW_PARSER_CHECK_NONE);
	ret = lwgeom_is_trajectory(g);
	lwgeom_free(g);
	ASSERT_INT_EQUAL(ret, LW_FALSE); /* backward measure */

	g = lwgeom_from_wkt("LINESTRING M(0 0 1, 0 0 2)", LW_PARSER_CHECK_NONE);
	ret = lwgeom_is_trajectory(g);
	lwgeom_free(g);
	ASSERT_INT_EQUAL(ret, LW_TRUE); /* ok (still) */

	g = lwgeom_from_wkt("LINESTRING M EMPTY", LW_PARSER_CHECK_NONE);
	ret = lwgeom_is_trajectory(g);
	lwgeom_free(g);
	ASSERT_INT_EQUAL(ret, LW_TRUE); /* ok (empty) */

	g = lwgeom_from_wkt("LINESTRING M (0 0 1)", LW_PARSER_CHECK_NONE);
	ret = lwgeom_is_trajectory(g);
	lwgeom_free(g);
	ASSERT_INT_EQUAL(ret, LW_TRUE); /* ok (corner case) */
}

/*
** Used by test harness to register the tests in this file.
*/
void measures_suite_setup(void);
void measures_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("measures", NULL, NULL);
	PG_ADD_TEST(suite, test_mindistance2d_tolerance);
	PG_ADD_TEST(suite, test_mindistance3d_tolerance);
	PG_ADD_TEST(suite, test_rect_tree_contains_point);
	PG_ADD_TEST(suite, test_rect_tree_intersects_tree);
	PG_ADD_TEST(suite, test_lwgeom_segmentize2d);
	PG_ADD_TEST(suite, test_lwgeom_locate_along);
	PG_ADD_TEST(suite, test_lw_dist2d_pt_arc);
	PG_ADD_TEST(suite, test_lw_dist2d_seg_arc);
	PG_ADD_TEST(suite, test_lw_dist2d_arc_arc);
	PG_ADD_TEST(suite, test_lw_arc_length);
	PG_ADD_TEST(suite, test_lw_dist2d_pt_ptarrayarc);
	PG_ADD_TEST(suite, test_lw_dist2d_ptarray_ptarrayarc);
	PG_ADD_TEST(suite, test_lwgeom_tcpa);
	PG_ADD_TEST(suite, test_lwgeom_is_trajectory);
	PG_ADD_TEST(suite, test_rect_tree_distance_tree);
}
