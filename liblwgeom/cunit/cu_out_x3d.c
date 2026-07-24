/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 * Copyright 2011-2016 Regina Obe
 * Copyright 2026 Darafei Praliaskouski <me@komzpa.net>
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

static void
do_x3d3_test(char *in, char *out, int precision, int option)
{
	LWGEOM *g = lwgeom_from_wkt(in, LW_PARSER_CHECK_NONE);
	lwvarlena_t *v = lwgeom_to_x3d3(g, precision, option, "");

	ASSERT_VARLENA_EQUAL(v, out);

	lwgeom_free(g);
	lwfree(v);
}

static char *
x3d3_as_cstring(const lwvarlena_t *v)
{
	size_t len = LWSIZE_GET(v->size) - LWVARHDRSZ;
	char *out = lwalloc(len + 1);
	memcpy(out, v->data, len);
	out[len] = '\0';
	return out;
}

static void
do_x3d3_contains(char *in, int precision, int option, const char *expected)
{
	LWGEOM *g = lwgeom_from_wkt(in, LW_PARSER_CHECK_NONE);
	lwvarlena_t *v = lwgeom_to_x3d3(g, precision, option, "");
	if (!v)
	{
		fprintf(stderr, "[%s:%d]\n Serialization returned NULL for input: %s\n", __FILE__, __LINE__, in);
		CU_FAIL();
		lwgeom_free(g);
		return;
	}
	char *out = x3d3_as_cstring(v);

	if (!strstr(out, expected))
	{
		fprintf(stderr, "[%s:%d]\n Expected substring: %s\n Obtained: %s\n", __FILE__, __LINE__, expected, out);
		CU_FAIL();
	}
	else
		CU_PASS();

	lwfree(v);
	lwfree(out);
	lwgeom_free(g);
}

static void
do_x3d3_not_contains(char *in, int precision, int option, const char *unexpected)
{
	LWGEOM *g = lwgeom_from_wkt(in, LW_PARSER_CHECK_NONE);
	lwvarlena_t *v = lwgeom_to_x3d3(g, precision, option, "");
	if (!v)
	{
		fprintf(stderr, "[%s:%d]\n Serialization returned NULL for input: %s\n", __FILE__, __LINE__, in);
		CU_FAIL();
		lwgeom_free(g);
		return;
	}
	char *out = x3d3_as_cstring(v);

	if (strstr(out, unexpected))
	{
		fprintf(
		    stderr, "[%s:%d]\n Unexpected substring: %s\n Obtained: %s\n", __FILE__, __LINE__, unexpected, out);
		CU_FAIL();
	}
	else
		CU_PASS();

	lwfree(v);
	lwfree(out);
	lwgeom_free(g);
}

static void
out_x3d3_test_precision(void)
{
	/* 0 precision, i.e a round */
	do_x3d3_test("POINT(1.1111111111111 1.1111111111111 2.11111111111111)", "1 1 2", 0, 0);

	/* 3 digits precision */
	do_x3d3_test("POINT(1.1111111111111 1.1111111111111 2.11111111111111)", "1.111 1.111 2.111", 3, 0);

	/* 9 digits precision */
	do_x3d3_test(
	    "POINT(1.2345678901234 1.2345678901234 4.123456789001)", "1.23456789 1.23456789 4.123456789", 9, 0);

	/* huge data */
	do_x3d3_test("POINT(1E300 -105E-153 4E300)", "1e+300 -1e-151 4e+300", 0, 0);
}

static void
out_x3d3_test_geoms(void)
{
	/* Linestring */
	do_x3d3_test("LINESTRING(0 1 5,2 3 6,4 5 7)",
		     "<LineSet  vertexCount='3'><Coordinate point='0 1 5 2 3 6 4 5 7' /></LineSet>",
		     0,
		     0);

	/* 2D Linestring */
	do_x3d3_test("LINESTRING(0 1,2 3,4 5)",
		     "<LineSet  vertexCount='3'><Coordinate point='0 1 0 2 3 0 4 5 0' /></LineSet>",
		     0,
		     0);

	/* Closed linestring keeps the closing coordinate because LineSet has no index to close the path. */
	do_x3d3_test("LINESTRING(0 0,1 0,0 0)",
		     "<LineSet  vertexCount='3'><Coordinate point='0 0 0 1 0 0 0 0 0' /></LineSet>",
		     0,
		     0);

	/* Polygon **/
	do_x3d3_test(
	    "POLYGON((15 10 3,13.536 6.464 3,10 5 3,6.464 6.464 3,5 10 3,6.464 13.536 3,10 15 3,13.536 13.536 3,15 10 3))",
	    "<IndexedFaceSet  convex='false' coordIndex='0 1 2 3 4 5 6 7'><Coordinate point='15 10 3 13.536 6.464 3 10 5 3 6.464 6.464 3 5 10 3 6.464 13.536 3 10 15 3 13.536 13.536 3 ' /></IndexedFaceSet>",
	    3,
	    0);

	/* 2D Polygon */
	do_x3d3_test(
	    "POLYGON((0 0,1 0,1 1,0 0))",
	    "<IndexedFaceSet  convex='false' coordIndex='0 1 2'><Coordinate point='0 0 0 1 0 0 1 1 0 ' /></IndexedFaceSet>",
	    0,
	    0);

	/* TODO: Polygon - with internal ring - the answer is clearly wrong */
	/** do_x3d3_test(
	    "POLYGON((0 1 3,2 3 3,4 5 3,0 1 3),(6 7 3,8 9 3,10 11 3,6 7 3))",
		"",
	    NULL, 0); **/

	/* 2D MultiPoint */
	do_x3d3_test("MULTIPOINT(0 1,2 3,4 5)", "<Polypoint2D  point='0 1 2 3 4 5 ' />", 0, 0);

	/* 3D MultiPoint */
	do_x3d3_test(
	    "MULTIPOINT Z(0 1 1,2 3 4,4 5 5)", "<PointSet ><Coordinate point='0 1 1 2 3 4 4 5 5 ' /></PointSet>", 0, 0);
	/* 3D Multiline */
	do_x3d3_test(
	    "MULTILINESTRING Z((0 1 1,2 3 4,4 5 5),(6 7 5,8 9 8,10 11 5))",
	    "<IndexedLineSet  coordIndex='0 1 2 -1 3 4 5'><Coordinate point='0 1 1 2 3 4 4 5 5 6 7 5 8 9 8 10 11 5 ' /></IndexedLineSet>",
	    0,
	    0);

	/* 2D Multiline */
	do_x3d3_test(
	    "MULTILINESTRING((0 1,2 3,4 5),(6 7,8 9,10 11))",
	    "<IndexedLineSet  coordIndex='0 1 2 -1 3 4 5'><Coordinate point='0 1 0 2 3 0 4 5 0 6 7 0 8 9 0 10 11 0 ' /></IndexedLineSet>",
	    0,
	    0);

	/* MultiPolygon */
	do_x3d3_test(
	    "MULTIPOLYGON(((0 1 1,2 3 1,4 5 1,0 1 1)),((6 7 1,8 9 1,10 11 1,6 7 1)))",
	    "<IndexedFaceSet  convex='false' coordIndex='0 1 2 -1 3 4 5'><Coordinate point='0 1 1 2 3 1 4 5 1 6 7 1 8 9 1 10 11 1 ' /></IndexedFaceSet>",
	    0,
	    0);

	/* 2D MultiPolygon */
	do_x3d3_test(
	    "MULTIPOLYGON(((0 1,2 3,4 5,0 1)),((6 7,8 9,10 11,6 7)))",
	    "<IndexedFaceSet  convex='false' coordIndex='0 1 2 -1 3 4 5'><Coordinate point='0 1 0 2 3 0 4 5 0 6 7 0 8 9 0 10 11 0 ' /></IndexedFaceSet>",
	    0,
	    0);

	/* PolyhedralSurface */
	do_x3d3_test(
	    "POLYHEDRALSURFACE( ((0 0 0, 0 0 1, 0 1 1, 0 1 0, 0 0 0)), ((0 0 0, 0 1 0, 1 1 0, 1 0 0, 0 0 0)), ((0 0 0, 1 0 0, 1 0 1, 0 0 1, 0 0 0)), ((1 1 0, 1 1 1, 1 0 1, 1 0 0, 1 1 0)), ((0 1 0, 0 1 1, 1 1 1, 1 1 0, 0 1 0)), ((0 0 1, 1 0 1, 1 1 1, 0 1 1, 0 0 1)) )",
	    "<IndexedFaceSet convex='false'  coordIndex='0 1 2 3 -1 4 5 6 7 -1 8 9 10 11 -1 12 13 14 15 -1 16 17 18 19 -1 20 21 22 23'><Coordinate point='0 0 0 0 0 1 0 1 1 0 1 0 0 0 0 0 1 0 1 1 0 1 0 0 0 0 0 1 0 0 1 0 1 0 0 1 1 1 0 1 1 1 1 0 1 1 0 0 0 1 0 0 1 1 1 1 1 1 1 0 0 0 1 1 0 1 1 1 1 0 1 1' /></IndexedFaceSet>",
	    0,
	    0);

	/* GeometryCollection with 2D Polygon */
	do_x3d3_test(
	    "GEOMETRYCOLLECTION(POLYGON((0 0,1 0,1 1,0 0)))",
	    "<Shape><IndexedFaceSet  convex='false' coordIndex='0 1 2'><Coordinate point='0 0 0 1 0 0 1 1 0 ' /></IndexedFaceSet></Shape>",
	    0,
	    0);

	/* TODO:  Implement Empty GeometryCollection correctly or throw a not-implemented */
	/** do_x3d3_test(
	    "GEOMETRYCOLLECTION EMPTY",
	    "",
	    NULL, 0); **/

	/* Curved inputs are linearized before X3D serialization.  These checks
	 * avoid depending on the full stroked coordinate list while still proving
	 * that each curved family reaches the expected X3D node type. */
	do_x3d3_contains(
	    "CIRCULARSTRING(0 0 1,1 1 1,2 0 1)", 3, 0, "<LineSet  vertexCount='65'><Coordinate point='0 0 1");

	do_x3d3_contains("CIRCULARSTRING(0 0 1,1 1 1,2 0 1,1 -1 1,0 0 1)",
			 0,
			 0,
			 "<LineSet  vertexCount='129'><Coordinate point='0 0 1");
	do_x3d3_contains("CIRCULARSTRING(0 0 1,1 1 1,2 0 1,1 -1 1,0 0 1)", 3, 0, "0 0 1' /></LineSet>");
	do_x3d3_contains("COMPOUNDCURVE(CIRCULARSTRING(0 0 1,1 1 1,2 0 1,1 -1 1,0 0 1))", 3, 0, "0 0 1' /></LineSet>");
	do_x3d3_contains(
	    "GEOMETRYCOLLECTION(CIRCULARSTRING(0 0 1,1 1 1,2 0 1,1 -1 1,0 0 1))", 3, 0, "0 0 1' /></LineSet></Shape>");

	do_x3d3_contains("COMPOUNDCURVE(CIRCULARSTRING(0 0 1,1 1 1,2 0 1),(2 0 1,3 0 1))", 3, 0, "3 0 1' /></LineSet>");

	do_x3d3_contains(
	    "CURVEPOLYGON(CIRCULARSTRING(0 0,1 1,2 0,1 -1,0 0))", 3, 0, "<IndexedFaceSet  convex='false' coordIndex='");

	do_x3d3_contains("MULTICURVE((0 0,1 1),CIRCULARSTRING(1 1,2 2,3 1))", 3, 0, "<IndexedLineSet  coordIndex='");

	do_x3d3_contains("MULTISURFACE(CURVEPOLYGON(CIRCULARSTRING(0 0,1 1,2 0,1 -1,0 0)))",
			 3,
			 0,
			 "<IndexedFaceSet  convex='false' coordIndex='");

	do_x3d3_contains(
	    "NURBSCURVE(2, (0 0, 5 10, 10 0))", 3, 0, "<LineSet  vertexCount='129'><Coordinate point='0 0 0");

	/* A general GeometryCollection must wrap each renderable child in Shape,
	 * including curved children and 3D surface families. Nested collections
	 * are flattened to sibling Shapes, because X3D Shape nodes cannot contain
	 * other Shape nodes as geometry. */
	do_x3d3_contains(
	    "GEOMETRYCOLLECTION(CIRCULARSTRING(0 0,1 1,2 0),POLYHEDRALSURFACE(((0 0 0,0 1 0,1 1 0,0 0 0))),TIN(((0 0,1 1,0 1,0 0))))",
	    3,
	    0,
	    "<Shape><LineSet");

	do_x3d3_contains(
	    "GEOMETRYCOLLECTION(CIRCULARSTRING(0 0,1 1,2 0),POLYHEDRALSURFACE(((0 0 0,0 1 0,1 1 0,0 0 0))),TIN(((0 0,1 1,0 1,0 0))))",
	    3,
	    0,
	    "</IndexedTriangleSet></Shape>");

	do_x3d3_test(
	    "GEOMETRYCOLLECTION(TRIANGLE((0 0,1 1,0 1,0 0)))",
	    "<Shape><IndexedTriangleSet  index='0 1 2'><Coordinate point='0 0 0 1 1 0 0 1 0'/></IndexedTriangleSet></Shape>",
	    0,
	    0);

	do_x3d3_not_contains(
	    "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(POINT(0 0),LINESTRING(0 0,1 1)))", 0, 0, "<Shape><Shape>");
}

static void
out_x3d3_test_option(void)
{
	/* 0 precision, flip coordinates*/
	do_x3d3_test("POINT(3.1111111111111 1.1111111111111 2.11111111111111)", "1 3 2", 0, 1);

	/* geocoordinate long,lat*/
	do_x3d3_test(
	    "SRID=4326;POLYGON((15 10 3,13.536 6.464 3,10 5 3,6.464 6.464 3,5 10 3,6.464 13.536 3,10 15 3,13.536 13.536 3,15 10 3))",
	    "<IndexedFaceSet  convex='false' coordIndex='0 1 2 3 4 5 6 7'><GeoCoordinate geoSystem='\"GD\" \"WE\" \"longitude_first\"' point='15 10 3 13.536 6.464 3 10 5 3 6.464 6.464 3 5 10 3 6.464 13.536 3 10 15 3 13.536 13.536 3 ' /></IndexedFaceSet>",
	    3,
	    2);

	/* geocoordinate lat long*/
	do_x3d3_test(
	    "SRID=4326;POLYGON((15 10 3,13.536 6.464 3,10 5 3,6.464 6.464 3,5 10 3,6.464 13.536 3,10 15 3,13.536 13.536 3,15 10 3))",
	    "<IndexedFaceSet  convex='false' coordIndex='0 1 2 3 4 5 6 7'><GeoCoordinate geoSystem='\"GD\" \"WE\" \"latitude_first\"' point='10 15 3 6.464 13.536 3 5 10 3 6.464 6.464 3 10 5 3 13.536 6.464 3 15 10 3 13.536 13.536 3 ' /></IndexedFaceSet>",
	    3,
	    3);
}

/*
** Used by test harness to register the tests in this file.
*/
void out_x3d_suite_setup(void);
void
out_x3d_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("x3d_output", NULL, NULL);
	PG_ADD_TEST(suite, out_x3d3_test_precision);
	PG_ADD_TEST(suite, out_x3d3_test_geoms);
	PG_ADD_TEST(suite, out_x3d3_test_option);
}
