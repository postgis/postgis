/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 * Copyright 2011-2016 Regina Obe
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

static void do_x3d3_test(char * in, char * out, int precision, int option)
{
	LWGEOM *g = lwgeom_from_wkt(in, LW_PARSER_CHECK_NONE);
	lwvarlena_t *v = lwgeom_to_x3d3(g, precision, option, "");

	ASSERT_VARLENA_EQUAL(v, out);

	lwgeom_free(g);
	lwfree(v);
}


static void do_x3d3_unsupported(char * in, char * out)
{
	LWGEOM *g = lwgeom_from_wkt(in, LW_PARSER_CHECK_NONE);
	lwvarlena_t *v = lwgeom_to_x3d3(g, 0, 0, "");

	ASSERT_STRING_EQUAL(out, cu_error_msg);
	cu_error_msg_reset();

	lwfree(v);
	lwgeom_free(g);
}


static void out_x3d3_test_precision(void)
{
	/* 0 precision, i.e a round */
	do_x3d3_test(
	    "POINT(1.1111111111111 1.1111111111111 2.11111111111111)",
	    "1 1 2",
	    0, 0);

	/* 3 digits precision */
	do_x3d3_test(
	    "POINT(1.1111111111111 1.1111111111111 2.11111111111111)",
	    "1.111 1.111 2.111",
	    3, 0);

	/* 9 digits precision */
	do_x3d3_test(
	    "POINT(1.2345678901234 1.2345678901234 4.123456789001)",
	    "1.23456789 1.23456789 4.123456789",
	    9, 0);

	/* huge data */
	do_x3d3_test("POINT(1E300 -105E-153 4E300)", "1e+300 -1e-151 4e+300", 0, 0);
}


static void out_x3d3_test_geoms(void)
{
	/* Linestring */
	do_x3d3_test(
	    "LINESTRING(0 1 5,2 3 6,4 5 7)",
	    "<LineSet  vertexCount='3'><Coordinate point='0 1 5 2 3 6 4 5 7' /></LineSet>",
	    0, 0);

	/* Polygon **/
	do_x3d3_test(
	    "POLYGON((15 10 3,13.536 6.464 3,10 5 3,6.464 6.464 3,5 10 3,6.464 13.536 3,10 15 3,13.536 13.536 3,15 10 3))",
	    "<IndexedFaceSet  convex='false' coordIndex='0 1 2 3 4 5 6 7'><Coordinate point='15 10 3 13.536 6.464 3 10 5 3 6.464 6.464 3 5 10 3 6.464 13.536 3 10 15 3 13.536 13.536 3 ' /></IndexedFaceSet>",
	    3, 0);

	/* TODO: Polygon - with internal ring - the answer is clearly wrong */
	/** do_x3d3_test(
	    "POLYGON((0 1 3,2 3 3,4 5 3,0 1 3),(6 7 3,8 9 3,10 11 3,6 7 3))",
	        "",
	    NULL, 0); **/

	/* 2D MultiPoint */
	do_x3d3_test(
	    "MULTIPOINT(0 1,2 3,4 5)",
	    "<Polypoint2D  point='0 1 2 3 4 5 ' />",
	    0, 0);

	/* 3D MultiPoint */
	do_x3d3_test(
	    "MULTIPOINT Z(0 1 1,2 3 4,4 5 5)",
	    "<PointSet ><Coordinate point='0 1 1 2 3 4 4 5 5 ' /></PointSet>",
	    0, 0);
	/* 3D Multiline */
	do_x3d3_test(
	    "MULTILINESTRING Z((0 1 1,2 3 4,4 5 5),(6 7 5,8 9 8,10 11 5))",
	    "<IndexedLineSet  coordIndex='0 1 2 -1 3 4 5'><Coordinate point='0 1 1 2 3 4 4 5 5 6 7 5 8 9 8 10 11 5 ' /></IndexedLineSet>",
	    0, 0);

	/* MultiPolygon */
	do_x3d3_test(
	    "MULTIPOLYGON(((0 1 1,2 3 1,4 5 1,0 1 1)),((6 7 1,8 9 1,10 11 1,6 7 1)))",
	    "<IndexedFaceSet  convex='false' coordIndex='0 1 2 -1 3 4 5'><Coordinate point='0 1 1 2 3 1 4 5 1 6 7 1 8 9 1 10 11 1 ' /></IndexedFaceSet>",
	    0, 0);

	/* PolyhedralSurface */
	do_x3d3_test(
	    "POLYHEDRALSURFACE( ((0 0 0, 0 0 1, 0 1 1, 0 1 0, 0 0 0)), ((0 0 0, 0 1 0, 1 1 0, 1 0 0, 0 0 0)), ((0 0 0, 1 0 0, 1 0 1, 0 0 1, 0 0 0)), ((1 1 0, 1 1 1, 1 0 1, 1 0 0, 1 1 0)), ((0 1 0, 0 1 1, 1 1 1, 1 1 0, 0 1 0)), ((0 0 1, 1 0 1, 1 1 1, 0 1 1, 0 0 1)) )",
	    "<IndexedFaceSet convex='false'  coordIndex='0 1 2 3 -1 4 5 6 7 -1 8 9 10 11 -1 12 13 14 15 -1 16 17 18 19 -1 20 21 22 23'><Coordinate point='0 0 0 0 0 1 0 1 1 0 1 0 0 0 0 0 1 0 1 1 0 1 0 0 0 0 0 1 0 0 1 0 1 0 0 1 1 1 0 1 1 1 1 0 1 1 0 0 0 1 0 0 1 1 1 1 1 1 1 0 0 0 1 1 0 1 1 1 1 0 1 1' /></IndexedFaceSet>",
	    0, 0);

	/* TODO: returns garbage at moment correctly implement GeometryCollection -- */
	/** do_x3d3_test(
	    "GEOMETRYCOLLECTION(POINT(0 1 3),LINESTRING(2 3 3,4 5 3))",
	    "",
	    NULL, 0); **/

	/* TODO:  Implement Empty GeometryCollection correctly or throw a not-implemented */
	/** do_x3d3_test(
	    "GEOMETRYCOLLECTION EMPTY",
	    "",
	    NULL, 0); **/

	/* CircularString */
	do_x3d3_unsupported(
	    "CIRCULARSTRING(-2 0 1,0 2 1,2 0 1,0 2 1,2 4 1)",
	    "lwgeom_to_x3d3: 'CircularString' geometry type not supported");

	/* CompoundCurve */
	do_x3d3_unsupported(
	    "COMPOUNDCURVE(CIRCULARSTRING(0 0 1,1 1 1,1 0 1),(1 0 1,0 1 1))",
	    "lwgeom_to_x3d3: 'CompoundCurve' geometry type not supported");

}

static void out_x3d3_test_option(void)
{
	/* 0 precision, flip coordinates*/
	do_x3d3_test(
	    "POINT(3.1111111111111 1.1111111111111 2.11111111111111)",
	    "1 3 2",
	    0, 1);

	/* geocoordinate long,lat*/
	do_x3d3_test(
	    "SRID=4326;POLYGON((15 10 3,13.536 6.464 3,10 5 3,6.464 6.464 3,5 10 3,6.464 13.536 3,10 15 3,13.536 13.536 3,15 10 3))",
	    "<IndexedFaceSet  convex='false' coordIndex='0 1 2 3 4 5 6 7'><GeoCoordinate geoSystem='\"GD\" \"WE\" \"longitude_first\"' point='15 10 3 13.536 6.464 3 10 5 3 6.464 6.464 3 5 10 3 6.464 13.536 3 10 15 3 13.536 13.536 3 ' /></IndexedFaceSet>",
	    3, 2);

	/* geocoordinate lat long*/
	do_x3d3_test(
	    "SRID=4326;POLYGON((15 10 3,13.536 6.464 3,10 5 3,6.464 6.464 3,5 10 3,6.464 13.536 3,10 15 3,13.536 13.536 3,15 10 3))",
	    "<IndexedFaceSet  convex='false' coordIndex='0 1 2 3 4 5 6 7'><GeoCoordinate geoSystem='\"GD\" \"WE\" \"latitude_first\"' point='10 15 3 6.464 13.536 3 5 10 3 6.464 6.464 3 10 5 3 13.536 6.464 3 15 10 3 13.536 13.536 3 ' /></IndexedFaceSet>",
	    3, 3);
}


/*
** Used by test harness to register the tests in this file.
*/
void out_x3d_suite_setup(void);
void out_x3d_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("x3d_output", NULL, NULL);
	PG_ADD_TEST(suite, out_x3d3_test_precision);
	PG_ADD_TEST(suite, out_x3d3_test_geoms);
	PG_ADD_TEST(suite, out_x3d3_test_option);
}
