/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright 2011 Sandro Santilli <strk@kbt.io>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CUnit/Basic.h"

#include "lwgeom_geos.h"
#include "cu_tester.h"

#include "liblwgeom_internal.h"

static void
test_geos_noop(void)
{
	size_t i;
	char *in_ewkt;
	char *out_ewkt;
	LWGEOM *geom_in;
	LWGEOM *geom_out;

	char *ewkt[] = {
	    "POINT(0 0.2)",
	    "LINESTRING(-1 -1,-1 2.5,2 2,2 -1)",
	    "MULTIPOINT(0.9 0.9,0.9 0.9,0.9 0.9,0.9 0.9,0.9 0.9,0.9 0.9)",
	    "SRID=1;MULTILINESTRING((-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1))",
	    "SRID=1;MULTILINESTRING((-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1))",
	    "POLYGON((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0))",
	    "SRID=4326;POLYGON((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0))",
	    "SRID=4326;POLYGON((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5))",
	    "SRID=100000;POLYGON((-1 -1 3,-1 2.5 3,2 2 3,2 -1 3,-1 -1 3),(0 0 3,0 1 3,1 1 3,1 0 3,0 0 3),(-0.5 -0.5 3,-0.5 -0.4 3,-0.4 -0.4 3,-0.4 -0.5 3,-0.5 -0.5 3))",
	    "SRID=4326;MULTIPOLYGON(((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5)),((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5)))",
	    "SRID=4326;GEOMETRYCOLLECTION(POINT(0 1),POLYGON((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0)),MULTIPOLYGON(((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5))))",
	};

	for (i = 0; i < (sizeof ewkt / sizeof(char *)); i++)
	{
		in_ewkt = ewkt[i];
		geom_in = lwgeom_from_wkt(in_ewkt, LW_PARSER_CHECK_NONE);
		geom_out = lwgeom_geos_noop(geom_in);
		CU_ASSERT_PTR_NOT_NULL_FATAL(geom_out);
		out_ewkt = lwgeom_to_ewkt(geom_out);
		ASSERT_STRING_EQUAL(out_ewkt, in_ewkt);
		lwfree(out_ewkt);
		lwgeom_free(geom_out);
		lwgeom_free(geom_in);
	}

	/* TINs become collections of Polygons */
	in_ewkt = "TIN(((0 0, 1 1, 2 2, 0 0)), ((0 0, 1 1, 2 2, 0 0)))";
	geom_in = lwgeom_from_wkt(in_ewkt, LW_PARSER_CHECK_NONE);
	geom_out = lwgeom_geos_noop(geom_in);
	out_ewkt = lwgeom_to_ewkt(geom_out);
	ASSERT_STRING_EQUAL(out_ewkt, "GEOMETRYCOLLECTION(POLYGON((0 0,1 1,2 2,0 0)),POLYGON((0 0,1 1,2 2,0 0)))");
	lwfree(out_ewkt);
	lwgeom_free(geom_in);
	lwgeom_free(geom_out);

	/* Empties disappear */
	in_ewkt = "GEOMETRYCOLLECTION( LINESTRING (1 1, 2 2), POINT EMPTY, TRIANGLE ((0 0, 1 0, 1 1, 0 0)) )";
	geom_in = lwgeom_from_wkt(in_ewkt, LW_PARSER_CHECK_NONE);
	geom_out = lwgeom_geos_noop(geom_in);
	out_ewkt = lwgeom_to_ewkt(geom_out);
	ASSERT_STRING_EQUAL(out_ewkt, "GEOMETRYCOLLECTION(LINESTRING(1 1,2 2),POINT EMPTY,POLYGON((0 0,1 0,1 1,0 0)))");
	lwfree(out_ewkt);
	lwgeom_free(geom_in);
	lwgeom_free(geom_out);
}

static void test_geos_linemerge(void)
{
	char *ewkt;
	char *out_ewkt;
	LWGEOM *geom1;
	LWGEOM *geom2;

	ewkt = "MULTILINESTRING((0 0, 0 100),(0 -5, 0 0))";
	geom1 = lwgeom_from_wkt(ewkt, LW_PARSER_CHECK_NONE);
	geom2 = lwgeom_linemerge(geom1);
	out_ewkt = lwgeom_to_ewkt((LWGEOM*)geom2);
	ASSERT_STRING_EQUAL(out_ewkt, "LINESTRING(0 -5,0 0,0 100)");
	lwfree(out_ewkt);
	lwgeom_free(geom1);
	lwgeom_free(geom2);

	ewkt = "MULTILINESTRING EMPTY";
	geom1 = lwgeom_from_wkt(ewkt, LW_PARSER_CHECK_NONE);
	geom2 = lwgeom_linemerge(geom1);
	out_ewkt = lwgeom_to_ewkt((LWGEOM*)geom2);
	ASSERT_STRING_EQUAL(out_ewkt, "MULTILINESTRING EMPTY");
	lwfree(out_ewkt);
	lwgeom_free(geom1);
	lwgeom_free(geom2);
}

static void
test_geos_offsetcurve(void)
{
	char* ewkt;
	char* out_ewkt;
	LWGEOM* geom1;
	LWGEOM* geom2;

	ewkt = "MULTILINESTRING((-10 0, -10 100), (0 -5, 0 0))";
	geom1 = lwgeom_from_wkt(ewkt, LW_PARSER_CHECK_NONE);
	geom2 = lwgeom_offsetcurve(geom1, 2, 10, 1, 1);
	out_ewkt = lwgeom_to_ewkt((LWGEOM*)geom2);
	ASSERT_STRING_EQUAL(out_ewkt, "MULTILINESTRING((-12 0,-12 100),(-2 -5,-2 0))");
	lwfree(out_ewkt);
	lwgeom_free(geom1);
	lwgeom_free(geom2);
}

static void
test_geos_offsetcurve_crash(void)
{
	// Test for Trac #4143. The specific output or lack of output is not important,
	// we're just testing that we don't crash.
	LWGEOM* in = lwgeom_from_wkt("LINESTRING(362194.505 5649993.044,362197.451 5649994.125,362194.624 5650001.876,362189.684 5650000.114,362192.542 5649992.324,362194.505 5649993.044)", LW_PARSER_CHECK_NONE);
	LWGEOM* out = lwgeom_offsetcurve(in, -0.045, 8, 2, 5.0);

	lwgeom_free(in);
	if (out) {
		lwgeom_free(out);
	}
}

static void
test_geos_makevalid(void)
{
	uint8_t* wkb;
	char* out_ewkt;
	LWGEOM* geom1;
	LWGEOM* geom2;
	LWGEOM* geom3;

	wkb = (uint8_t*) "\001\003\000\000\000\001\000\000\000\011\000\000\000b\020X9 }\366@7\211A\340\235I\034A\316\326t18}\366@\306g\347\323\230I\034Ay\351&18}\366@\331\316\367\323\230I\034A\372~j\274\370}\366@\315\314\314LpI\034A\343\245\233\304R}\366@R\270\036\005?I\034A\315\314\314\314Z~\366@\343\245\233\304\007I\034A\004V\016-\242}\366@\252\361\322M\323H\034A\351&1\010\306{\366@H\341z\0247I\034Ab\020X9 }\366@7\211A\340\235I\034A";
	geom1 = lwgeom_from_wkb(wkb, 157, LW_PARSER_CHECK_NONE);
	geom2 = lwgeom_make_valid(geom1);
	geom3 = lwgeom_normalize(geom2); //so GEOS 3.9 and 3.10 agree
	out_ewkt = lwgeom_to_ewkt((LWGEOM*)geom3);

	ASSERT_STRING_EQUAL(
	    out_ewkt,
	    "POLYGON((92092.377 463437.77,92114.014 463463.469,92115.512 463462.207,92115.51207431706 463462.2069374289,92127.546 463452.075,92117.173 463439.755,92133.675 463425.942,92122.136 463412.826,92092.377 463437.77))");
	lwfree(out_ewkt);
	lwgeom_free(geom1);
	lwgeom_free(geom2);
	lwgeom_free(geom3);
}

static void
test_geos_intersection_rectangle_shortcut(void)
{
	LWGEOM *geom1;
	LWGEOM *geom2;
	LWGEOM *out;
	char *out_ewkt;

	geom1 = lwgeom_from_wkt("SRID=3857;LINESTRING ZM(1 1 5 7,2 2 6 8)", LW_PARSER_CHECK_NONE);
	geom2 = lwgeom_from_wkt("SRID=3857;POLYGON((0 0,0 3,3 3,3 0,0 0))", LW_PARSER_CHECK_NONE);
	out = lwgeom_intersection(geom1, geom2);
	out_ewkt = lwgeom_to_ewkt(out);
	ASSERT_STRING_EQUAL(out_ewkt, "SRID=3857;LINESTRING(1 1 5,2 2 6)");
	lwfree(out_ewkt);
	lwgeom_free(out);
	lwgeom_free(geom1);
	lwgeom_free(geom2);

	geom1 = lwgeom_from_wkt("POLYGON((0 0,4 0,4 4,0 4,0 0))", LW_PARSER_CHECK_NONE);
	geom2 = lwgeom_from_wkt("MULTIPOINT((1 1),(2 2))", LW_PARSER_CHECK_NONE);
	out = lwgeom_intersection(geom1, geom2);
	out_ewkt = lwgeom_to_ewkt(out);
	ASSERT_STRING_EQUAL(out_ewkt, "MULTIPOINT(1 1,2 2)");
	lwfree(out_ewkt);
	lwgeom_free(out);
	lwgeom_free(geom1);
	lwgeom_free(geom2);
}

static LWGEOM *
geos_intersection_fallback(const LWGEOM *geom1, const LWGEOM *geom2)
{
	GEOSGeometry *g1;
	GEOSGeometry *g2;
	GEOSGeometry *g3;
	LWGEOM *result;
	uint8_t is3d = FLAGS_GET_Z(geom1->flags) || FLAGS_GET_Z(geom2->flags);

	initGEOS(lwnotice, lwgeom_geos_error);
	g1 = LWGEOM2GEOS(geom1, LW_TRUE);
	g2 = LWGEOM2GEOS(geom2, LW_TRUE);
	if (!g1 || !g2)
	{
		if (g1)
			GEOSGeom_destroy(g1);
		if (g2)
			GEOSGeom_destroy(g2);
		return NULL;
	}

	g3 = GEOSIntersection(g1, g2);
	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	if (!g3)
		return NULL;

	result = GEOS2LWGEOM(g3, is3d);
	GEOSGeom_destroy(g3);
	return result;
}

static void
assert_intersection_matches_fallback(const char *geom1_wkt,
				     const char *geom2_wkt,
				     uint8_t expected_type,
				     uint32_t expected_npoints)
{
	LWGEOM *geom1 = lwgeom_from_wkt(geom1_wkt, LW_PARSER_CHECK_NONE);
	LWGEOM *geom2 = lwgeom_from_wkt(geom2_wkt, LW_PARSER_CHECK_NONE);
	LWGEOM *out = lwgeom_intersection(geom1, geom2);
	LWGEOM *expected = geos_intersection_fallback(geom1, geom2);
	char *out_ewkt;
	char *expected_ewkt;

	CU_ASSERT_PTR_NOT_NULL_FATAL(out);
	CU_ASSERT_PTR_NOT_NULL_FATAL(expected);
	out_ewkt = lwgeom_to_ewkt(out);
	expected_ewkt = lwgeom_to_ewkt(expected);
	ASSERT_STRING_EQUAL(out_ewkt, expected_ewkt);
	ASSERT_INT_EQUAL(out->type, expected->type);
	if (expected_npoints)
	{
		ASSERT_INT_EQUAL(out->type, expected_type);
		ASSERT_INT_EQUAL(lwgeom_count_vertices(out), expected_npoints);
	}
	lwfree(expected_ewkt);
	lwfree(out_ewkt);
	lwgeom_free(expected);
	lwgeom_free(out);
	lwgeom_free(geom2);
	lwgeom_free(geom1);
}

static void
test_geos_intersection_rectangle_shortcut_guards(void)
{
	LWGEOM *geom1;
	LWGEOM *geom2;
	LWGEOM *expected;
	LWGEOM *out;
	char *out_ewkt;

	geom1 = lwgeom_from_wkt("CIRCULARSTRING(1 1,2 2,3 1)", LW_PARSER_CHECK_NONE);
	geom2 = lwgeom_from_wkt("POLYGON((0 0,0 4,4 4,4 0,0 0))", LW_PARSER_CHECK_NONE);
	out = lwgeom_intersection(geom1, geom2);
	out_ewkt = lwgeom_to_ewkt(out);
	CU_ASSERT(strncmp(out_ewkt, "LINESTRING(", 11) == 0);
	lwfree(out_ewkt);
	lwgeom_free(out);
	lwgeom_free(geom1);
	lwgeom_free(geom2);

	geom1 = lwgeom_from_wkt("GEOMETRYCOLLECTION(CIRCULARSTRING(1 1,2 2,3 1))", LW_PARSER_CHECK_NONE);
	geom2 = lwgeom_from_wkt("POLYGON((0 0,0 4,4 4,4 0,0 0))", LW_PARSER_CHECK_NONE);
	out = lwgeom_intersection(geom1, geom2);
	out_ewkt = lwgeom_to_ewkt(out);
	CU_ASSERT_PTR_NULL(strstr(out_ewkt, "CIRCULARSTRING"));
	lwfree(out_ewkt);
	lwgeom_free(out);
	lwgeom_free(geom1);
	lwgeom_free(geom2);

	geom1 = lwgeom_from_wkt("CURVEPOLYGON((1 1,1 2,2 2,2 1,1 1))", LW_PARSER_CHECK_NONE);
	geom2 = lwgeom_from_wkt("POLYGON((0 0,0 4,4 4,4 0,0 0))", LW_PARSER_CHECK_NONE);
	out = lwgeom_intersection(geom1, geom2);
	expected = lwgeom_from_wkt("POLYGON((1 2,2 2,2 1,1 1,1 2))", LW_PARSER_CHECK_NONE);
	ASSERT_INT_EQUAL(out->type, POLYGONTYPE);
	ASSERT_NORMALIZED_GEOM_SAME(out, expected);
	lwgeom_free(expected);
	lwgeom_free(out);
	lwgeom_free(geom1);
	lwgeom_free(geom2);

	geom1 = lwgeom_from_wkt("TRIANGLE((1 1,2 1,1 2,1 1))", LW_PARSER_CHECK_NONE);
	geom2 = lwgeom_from_wkt("POLYGON((0 0,0 4,4 4,4 0,0 0))", LW_PARSER_CHECK_NONE);
	out = lwgeom_intersection(geom1, geom2);
	expected = lwgeom_from_wkt("POLYGON((1 2,2 1,1 1,1 2))", LW_PARSER_CHECK_NONE);
	ASSERT_INT_EQUAL(out->type, POLYGONTYPE);
	ASSERT_NORMALIZED_GEOM_SAME(out, expected);
	lwgeom_free(expected);
	lwgeom_free(out);
	lwgeom_free(geom1);
	lwgeom_free(geom2);

	geom1 = lwgeom_from_wkt("TIN(((1 1,2 1,1 2,1 1)))", LW_PARSER_CHECK_NONE);
	geom2 = lwgeom_from_wkt("POLYGON((0 0,0 4,4 4,4 0,0 0))", LW_PARSER_CHECK_NONE);
	out = lwgeom_intersection(geom1, geom2);
	expected = lwgeom_from_wkt("POLYGON((1 2,2 1,1 1,1 2))", LW_PARSER_CHECK_NONE);
	ASSERT_INT_EQUAL(out->type, POLYGONTYPE);
	ASSERT_NORMALIZED_GEOM_SAME(out, expected);
	lwgeom_free(expected);
	lwgeom_free(out);
	lwgeom_free(geom1);
	lwgeom_free(geom2);

	geom1 = lwgeom_from_wkt("LINESTRING(1 1,1 1)", LW_PARSER_CHECK_NONE);
	geom2 = lwgeom_from_wkt("POLYGON((0 0,0 4,4 4,4 0,0 0))", LW_PARSER_CHECK_NONE);
	out = lwgeom_intersection(geom1, geom2);
	expected = lwgeom_intersection_prec(geom1, geom2, 1);
	ASSERT_NORMALIZED_GEOM_SAME(out, expected);
	lwgeom_free(expected);
	lwgeom_free(out);
	lwgeom_free(geom1);
	lwgeom_free(geom2);

	geom1 = lwgeom_from_wkt("POINT(0 0.5)", LW_PARSER_CHECK_NONE);
	geom2 = lwgeom_from_wkt("POLYGON((0 0,0.0000000000001 1,1 1,1 0,0 0))", LW_PARSER_CHECK_NONE);
	out = lwgeom_intersection(geom1, geom2);
	out_ewkt = lwgeom_to_ewkt(out);
	ASSERT_STRING_EQUAL(out_ewkt, "POINT EMPTY");
	lwfree(out_ewkt);
	lwgeom_free(out);
	lwgeom_free(geom1);
	lwgeom_free(geom2);

	geom1 = lwgeom_from_wkt("LINESTRING(0.4 0.4,1.6 1.6)", LW_PARSER_CHECK_NONE);
	geom2 = lwgeom_from_wkt("POLYGON((0 0,0 2,2 2,2 0,0 0))", LW_PARSER_CHECK_NONE);
	out = lwgeom_intersection_prec(geom1, geom2, 1);
	out_ewkt = lwgeom_to_ewkt(out);
	ASSERT_STRING_EQUAL(out_ewkt, "LINESTRING(0 0,2 2)");
	lwfree(out_ewkt);
	lwgeom_free(out);
	lwgeom_free(geom1);
	lwgeom_free(geom2);

	/* GEOS supplies output Z when only the rectangle has Z. */
	assert_intersection_matches_fallback("POINT(1 1)", "POLYGON Z((0 0 1,0 4 2,4 4 3,4 0 4,0 0 1))", POINTTYPE, 1);
	assert_intersection_matches_fallback(
	    "LINESTRING(1 1,3 3)", "POLYGON Z((0 0 1,0 4 2,4 4 3,4 0 4,0 0 1))", LINETYPE, 2);
	assert_intersection_matches_fallback(
	    "LINESTRING Z(0 0 10,4 0 20)", "POLYGON Z((0 0 1,0 4 2,4 4 3,4 0 4,0 0 1))", LINETYPE, 2);
	assert_intersection_matches_fallback(
	    "POLYGON Z((0 0 1,0 4 2,4 4 3,4 0 4,0 0 1))", "LINESTRING Z(0 0 10,4 0 20)", LINETYPE, 2);
	assert_intersection_matches_fallback(
	    "POLYGON((0 0,0 4,4 4,4 0,0 0))", "POLYGON((1 1,3 1,3 3,1 3,1 1))", POLYGONTYPE, 5);

	/* GEOS nodes simple lines whose internal vertices touch the clip boundary. */
	/* GEOS versions differ on whether these are split; match this build's fallback exactly. */
	assert_intersection_matches_fallback("LINESTRING(0 0,4 0,4 4)", "POLYGON((0 0,0 4,4 4,4 0,0 0))", 0, 0);
	assert_intersection_matches_fallback("LINESTRING(1 1,0 2,1 3)", "POLYGON((0 0,0 4,4 4,4 0,0 0))", 0, 0);
	assert_intersection_matches_fallback("LINESTRING(0 0,2 0,4 0)", "POLYGON((0 0,0 4,4 4,4 0,0 0))", 0, 0);

	/* GEOS overlay collapses singleton multi-geometries to their base type. */
	assert_intersection_matches_fallback("MULTIPOINT((1 1))", "POLYGON((0 0,0 4,4 4,4 0,0 0))", POINTTYPE, 1);
	assert_intersection_matches_fallback(
	    "MULTILINESTRING((1 1,3 3))", "POLYGON((0 0,0 4,4 4,4 0,0 0))", LINETYPE, 2);
	assert_intersection_matches_fallback(
	    "MULTIPOLYGON(((1 1,1 3,3 3,3 1,1 1)))", "POLYGON((0 0,0 4,4 4,4 0,0 0))", POLYGONTYPE, 5);

	/* GEOS overlay removes consecutive duplicate XY vertices. */
	assert_intersection_matches_fallback("LINESTRING(1 1,1 1,2 2)", "POLYGON((0 0,0 4,4 4,4 0,0 0))", LINETYPE, 2);
	assert_intersection_matches_fallback(
	    "POLYGON((1 1,1 3,3 3,3 3,3 1,1 1))", "POLYGON((0 0,0 4,4 4,4 0,0 0))", POLYGONTYPE, 5);

	/* Nonclosed polygon input must keep the legacy GEOS autofix behavior. */
	geom1 = lwgeom_from_wkt("POLYGON((1 1,1 3,3 3,3 1))", LW_PARSER_CHECK_NONE);
	geom2 = lwgeom_from_wkt("POLYGON((0 0,0 4,4 4,4 0,0 0))", LW_PARSER_CHECK_NONE);
	out = lwgeom_intersection(geom1, geom2);
	expected = geos_intersection_fallback(geom1, geom2);
	ASSERT_INT_EQUAL(out->type, expected->type);
	ASSERT_NORMALIZED_GEOM_SAME(out, expected);
	ASSERT_INT_EQUAL(out->type, POLYGONTYPE);
	CU_ASSERT(ptarray_is_closed_2d(((LWPOLY *)out)->rings[0]));
	lwgeom_free(expected);
	lwgeom_free(out);
	lwgeom_free(geom1);
	lwgeom_free(geom2);

	/* Invalid polygonal inputs must retain the GEOS overlay failure. */
	geom1 = lwgeom_from_wkt("POLYGON((1 1,3 3,1 3,3 1,1 1))", LW_PARSER_CHECK_NONE);
	geom2 = lwgeom_from_wkt("POLYGON((0 0,0 4,4 4,4 0,0 0))", LW_PARSER_CHECK_NONE);
	out = lwgeom_intersection(geom1, geom2);
	CU_ASSERT_PTR_NULL(out);
	lwgeom_free(geom1);
	lwgeom_free(geom2);

	geom1 = lwgeom_from_wkt("LINESTRING(1 1,3 3,1 3,3 1)", LW_PARSER_CHECK_NONE);
	geom2 = lwgeom_from_wkt("POLYGON((0 0,0 4,4 4,4 0,0 0))", LW_PARSER_CHECK_NONE);
	out = lwgeom_intersection(geom1, geom2);
	expected = geos_intersection_fallback(geom1, geom2);
	ASSERT_INT_EQUAL(out->type, expected->type);
	ASSERT_NORMALIZED_GEOM_SAME(out, expected);
	lwgeom_free(expected);
	lwgeom_free(out);
	lwgeom_free(geom1);
	lwgeom_free(geom2);

	geom1 = lwgeom_from_wkt("LINESTRING(1 1,3 1,2 1,4 1)", LW_PARSER_CHECK_NONE);
	geom2 = lwgeom_from_wkt("POLYGON((0 0,0 4,4 4,4 0,0 0))", LW_PARSER_CHECK_NONE);
	out = lwgeom_intersection(geom1, geom2);
	expected = geos_intersection_fallback(geom1, geom2);
	ASSERT_INT_EQUAL(out->type, expected->type);
	ASSERT_NORMALIZED_GEOM_SAME(out, expected);
	lwgeom_free(expected);
	lwgeom_free(out);
	lwgeom_free(geom1);
	lwgeom_free(geom2);

	geom1 = lwgeom_from_wkt("MULTILINESTRING((1 1,3 1),(2 1,4 1))", LW_PARSER_CHECK_NONE);
	geom2 = lwgeom_from_wkt("POLYGON((0 0,0 5,5 5,5 0,0 0))", LW_PARSER_CHECK_NONE);
	out = lwgeom_intersection(geom1, geom2);
	expected = geos_intersection_fallback(geom1, geom2);
	ASSERT_INT_EQUAL(out->type, expected->type);
	ASSERT_NORMALIZED_GEOM_SAME(out, expected);
	lwgeom_free(expected);
	lwgeom_free(out);
	lwgeom_free(geom1);
	lwgeom_free(geom2);

	geom1 = lwgeom_from_wkt("MULTIPOINT((1 1),(1 1))", LW_PARSER_CHECK_NONE);
	geom2 = lwgeom_from_wkt("POLYGON((0 0,0 4,4 4,4 0,0 0))", LW_PARSER_CHECK_NONE);
	out = lwgeom_intersection(geom1, geom2);
	expected = geos_intersection_fallback(geom1, geom2);
	ASSERT_INT_EQUAL(out->type, expected->type);
	ASSERT_NORMALIZED_GEOM_SAME(out, expected);
	lwgeom_free(expected);
	lwgeom_free(out);
	lwgeom_free(geom1);
	lwgeom_free(geom2);
}

static void test_geos_subdivide(void)
{
	char *ewkt = "LINESTRING(0 0, 10 10)";
	char *out_ewkt;
	LWGEOM *geom1 = lwgeom_from_wkt(ewkt, LW_PARSER_CHECK_NONE);
	/* segmentize as geography to generate a non-simple curve */
	LWGEOM *geom2 = lwgeom_segmentize_sphere(geom1, 0.002);

	LWCOLLECTION *geom3 = lwgeom_subdivide(geom2, 80);
	out_ewkt = lwgeom_to_ewkt((LWGEOM*)geom3);
	// printf("\n--------\n%s\n--------\n", out_ewkt);
	CU_ASSERT_EQUAL(2, geom3->ngeoms);
	lwfree(out_ewkt);
	lwcollection_free(geom3);

	geom3 = lwgeom_subdivide(geom2, 20);
	out_ewkt = lwgeom_to_ewkt((LWGEOM*)geom3);
	// printf("\n--------\n%s\n--------\n", out_ewkt);
	CU_ASSERT_EQUAL(8, geom3->ngeoms);
	lwfree(out_ewkt);
	lwcollection_free(geom3);

	lwgeom_free(geom2);
	lwgeom_free(geom1);
}

/*
** Used by test harness to register the tests in this file.
*/
void geos_suite_setup(void);
void geos_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("geos", NULL, NULL);
	PG_ADD_TEST(suite, test_geos_noop);
	PG_ADD_TEST(suite, test_geos_subdivide);
	PG_ADD_TEST(suite, test_geos_linemerge);
	PG_ADD_TEST(suite, test_geos_offsetcurve);
	PG_ADD_TEST(suite, test_geos_offsetcurve_crash);
	PG_ADD_TEST(suite, test_geos_makevalid);
	PG_ADD_TEST(suite, test_geos_intersection_rectangle_shortcut);
	PG_ADD_TEST(suite, test_geos_intersection_rectangle_shortcut_guards);
}
