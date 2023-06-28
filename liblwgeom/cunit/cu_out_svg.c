/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 * Copyright 2010 Olivier Courtin <olivier.courtin@oslandia.com>
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

static void do_svg_test(char * in, char * out, int precision, int relative)
{
	LWGEOM *g = lwgeom_from_wkt(in, LW_PARSER_CHECK_NONE);
	lwvarlena_t *v = lwgeom_to_svg(g, precision, relative);

	ASSERT_VARLENA_EQUAL(v, out);

	lwgeom_free(g);
	lwfree(v);
}


static void do_svg_unsupported(char * in, char * out)
{
	LWGEOM *g = lwgeom_from_wkt(in, LW_PARSER_CHECK_NONE);
	lwvarlena_t *v = lwgeom_to_svg(g, 0, 0);

	ASSERT_STRING_EQUAL(cu_error_msg, out);
	cu_error_msg_reset();

	lwgeom_free(g);
	lwfree(v);
}


static void out_svg_test_precision(void)
{
	/* 0 precision, i.e a round - with Circle point */
	do_svg_test(
	    "POINT(1.1111111111111 1.1111111111111)",
	    "cx=\"1\" cy=\"-1\"",
	    0, 0);

	/* 0 precision, i.e a round - with Point */
	do_svg_test(
	    "POINT(1.1111111111111 1.1111111111111)",
	    "x=\"1\" y=\"-1\"",
	    0, 1);

	/* 0 precision, i.e a round - with PointArray */
	do_svg_test(
	    "LINESTRING(1.1111111111111 1.1111111111111,1.1111111111111 1.1111111111111)",
	    "M 1 -1 L 1 -1",
	    0, 0);

	/* 0 precision, i.e a round - with relative PointArray */
	do_svg_test(
	    "LINESTRING(1.1111111111111 1.1111111111111,1.1111111111111 1.1111111111111)",
	    "M 1 -1 l 0 0",
	    0, 1);


	/* 9 digits precision - with Circle point */
	do_svg_test(
	    "POINT(1.2345678901234 1.2345678901234)",
	    "cx=\"1.23456789\" cy=\"-1.23456789\"",
	    9, 0);

	/* 9 digits precision - with Point */
	do_svg_test(
	    "POINT(1.2345678901234 1.2345678901234)",
	    "x=\"1.23456789\" y=\"-1.23456789\"",
	    9, 1);

	/* 9 digits precision - with PointArray */
	do_svg_test(
	    "LINESTRING(1.2345678901234 1.2345678901234,2.3456789012345 2.3456789012345)",
	    "M 1.23456789 -1.23456789 L 2.345678901 -2.345678901",
	    9, 0);

	/* 9 digits precision - with relative PointArray */
	do_svg_test(
	    "LINESTRING(1.2345678901234 1.2345678901234,2.3456789012345 2.3456789012345)",
	    "M 1.23456789 -1.23456789 l 1.111111011 -1.111111011",
	    9, 1);


	/* huge data - with Circle point */
	do_svg_test(
	    "POINT(1E300 -1E300)",
	    "cx=\"1e+300\" cy=\"1e+300\"",
	    0, 0);

	/* huge data - with Point */
	do_svg_test(
	    "POINT(1E300 -1E300)",
	    "x=\"1e+300\" y=\"1e+300\"",
	    0, 1);

	/* huge data - with PointArray */
	do_svg_test(
	    "LINESTRING(1E300 -1E300,1E301 -1E301)",
	    "M 1e+300 1e+300 L 1e+301 1e+301",
	    0, 0);

	/* huge data - with relative PointArray */
	do_svg_test("LINESTRING(1E300 -1E300,1E301 -1E301)", "M 1e+300 1e+300 l 9e+300 9e+300", 0, 1);
}


static void out_svg_test_dims(void)
{
	/* 4D - with Circle point */
	do_svg_test(
	    "POINT(0 1 2 3)",
	    "cx=\"0\" cy=\"-1\"",
	    0, 0);

	/* 4D - with Point */
	do_svg_test(
	    "POINT(0 1 2 3)",
	    "x=\"0\" y=\"-1\"",
	    0, 1);

	/* 4D - with PointArray */
	do_svg_test(
	    "LINESTRING(0 1 2 3,4 5 6 7)",
	    "M 0 -1 L 4 -5",
	    0, 0);

	/* 4D - with relative PointArray */
	do_svg_test(
	    "LINESTRING(0 1 2 3,4 5 6 7)",
	    "M 0 -1 l 4 -4",
	    0, 1);
}


static void out_svg_test_geoms(void)
{
	/* Linestring */
	do_svg_test(
	    "LINESTRING(0 1,2 3,4 5)",
	    "M 0 -1 L 2 -3 4 -5",
	    0, 0);

	/* Polygon */
	do_svg_test(
	    "POLYGON((0 1,2 3,4 5,0 1))",
	    "M 0 -1 L 2 -3 4 -5 Z",
	    0, 0);

	/* Polygon - with internal ring */
	do_svg_test(
	    "POLYGON((0 1,2 3,4 5,0 1),(6 7,8 9,10 11,6 7))",
	    "M 0 -1 L 2 -3 4 -5 Z M 6 -7 L 8 -9 10 -11 Z",
	    0, 0);

	/* MultiPoint */
	do_svg_test(
	    "MULTIPOINT(0 1,2 3)",
	    "cx=\"0\" cy=\"-1\",cx=\"2\" cy=\"-3\"",
	    0, 0);

	/* MultiLine */
	do_svg_test(
	    "MULTILINESTRING((0 1,2 3,4 5),(6 7,8 9,10 11))",
	    "M 0 -1 L 2 -3 4 -5 M 6 -7 L 8 -9 10 -11",
	    0, 0);

	/* MultiPolygon */
	do_svg_test(
	    "MULTIPOLYGON(((0 1,2 3,4 5,0 1)),((6 7,8 9,10 11,6 7)))",
	    "M 0 -1 L 2 -3 4 -5 Z M 6 -7 L 8 -9 10 -11 Z",
	    0, 0);

	/* GeometryCollection */
	do_svg_test(
	    "GEOMETRYCOLLECTION(POINT(0 1),LINESTRING(2 3,4 5))",
	    "cx=\"0\" cy=\"-1\";M 2 -3 L 4 -5",
	    0, 0);

	/* CircularString */
	do_svg_test(
	    "CIRCULARSTRING(-2 0,0 2,2 0,0 2,2 4)",
	    "M -2 0 A 2 2 0 0 1 2 0 A 2 2 0 0 1 2 -4",
			0, 0);

	/* : Circle */
	do_svg_test(
	    "CIRCULARSTRING(4 2,-2 2,4 2)",
	    "M 1 -2 m 3 0 a 3 3 0 1 0 -6 0 a 3 3 0 1 0 6 0",
			0, 0);

	/* CompoundCurve */
	do_svg_test(
	    "COMPOUNDCURVE(CIRCULARSTRING(0 0,1 1,1 0),(1 0,0 1))",
	    "M 0 0 A 1 1 0 1 1 1 0 L 0 -1", 0, 0);

	/* MultiCurve */
	do_svg_test(
	    "MULTICURVE((5 5,3 5,3 3,0 3),CIRCULARSTRING(0 0,2 1,2 2))",
	    "M 5 -5 L 3 -5 3 -3 0 -3 M 0 0 A 2 2 0 0 0 2 -2", 0, 0);

	/* CurvePolygon */
	do_svg_test(
	    "CURVEPOLYGON(CIRCULARSTRING(-2 0,-1 -1,0 0,1 -1,2 0,0 2,-2 0),(-1 0,0 0.5,1 0,0 1,-1 0))",
	    "M -2 0 A 1 1 0 0 0 0 0 A 1 1 0 0 0 2 0 A 2 2 0 0 0 -2 0 Z M -1 0 L 0 0 1 0 0 -1 -1 0 Z", 0, 0);

	/* MultiSurface */
	do_svg_test(
	    "MULTISURFACE(CURVEPOLYGON(CIRCULARSTRING(-2 0,-1 -1,0 0,1 -1,2 0,0 2,-2 0),(-1 0,0 0.5,1 0,0 1,-1 0)),((7 8,10 10,6 14,4 11,7 8)))",
	    "M -2 0 A 1 1 0 0 0 0 0 A 1 1 0 0 0 2 0 A 2 2 0 0 0 -2 0 Z M -1 0 L 0 0 1 0 0 -1 -1 0 Z M 7 -8 L 10 -10 6 -14 4 -11 Z", 0, 0);

	/* Empty GeometryCollection */
	do_svg_test(
	    "GEOMETRYCOLLECTION EMPTY",
	    "",
	    0, 0);

	/* Nested GeometryCollection */
	do_svg_unsupported(
	    "GEOMETRYCOLLECTION(POINT(0 1),GEOMETRYCOLLECTION(LINESTRING(2 3,4 5)))",
	    "assvg_geom_buf: 'GeometryCollection' geometry type not supported.");

}

static void out_svg_test_relative(void)
{
	/* Linestring */
	do_svg_test(
	    "LINESTRING(0 1,2 3,4 5)",
	    "M 0 -1 l 2 -2 2 -2",
	    0, 1);

	/* Polygon */
	do_svg_test(
	    "POLYGON((0 1,2 3,4 5,0 1))",
	    "M 0 -1 l 2 -2 2 -2 z",
	    0, 1);

	/* Polygon - with internal ring */
	do_svg_test(
	    "POLYGON((0 1,2 3,4 5,0 1),(6 7,8 9,10 11,6 7))",
	    "M 0 -1 l 2 -2 2 -2 z M 6 -7 l 2 -2 2 -2 z",
	    0, 1);

	/* MultiPoint */
	do_svg_test(
	    "MULTIPOINT(0 1,2 3)",
	    "x=\"0\" y=\"-1\",x=\"2\" y=\"-3\"",
	    0, 1);

	/* MultiLine */
	do_svg_test(
	    "MULTILINESTRING((0 1,2 3,4 5),(6 7,8 9,10 11))",
	    "M 0 -1 l 2 -2 2 -2 M 6 -7 l 2 -2 2 -2",
	    0, 1);

	/* MultiPolygon */
	do_svg_test(
	    "MULTIPOLYGON(((0 1,2 3,4 5,0 1)),((6 7,8 9,10 11,6 7)))",
	    "M 0 -1 l 2 -2 2 -2 z M 6 -7 l 2 -2 2 -2 z",
	    0, 1);

	/* GeometryCollection */
	do_svg_test(
	    "GEOMETRYCOLLECTION(POINT(0 1),LINESTRING(2 3,4 5))",
	    "x=\"0\" y=\"-1\";M 2 -3 l 2 -2",
	    0, 1);
}

static void out_svg_test_srid(void)
{
	/* SRID - with Circle point */
	do_svg_test(
	    "SRID=4326;POINT(0 1)",
	    "cx=\"0\" cy=\"-1\"",
	    0, 0);

	/* SRID - with Point */
	do_svg_test(
	    "SRID=4326;POINT(0 1)",
	    "x=\"0\" y=\"-1\"",
	    0, 1);

	/* SRID - with PointArray */
	do_svg_test(
	    "SRID=4326;LINESTRING(0 1,2 3)",
	    "M 0 -1 L 2 -3",
	    0, 0);

	/* SRID - with relative PointArray */
	do_svg_test(
	    "SRID=4326;LINESTRING(0 1,2 3)",
	    "M 0 -1 l 2 -2",
	    0, 1);
}

/*
** Used by test harness to register the tests in this file.
*/
void out_svg_suite_setup(void);
void out_svg_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("svg_output", NULL, NULL);
	PG_ADD_TEST(suite, out_svg_test_precision);
	PG_ADD_TEST(suite, out_svg_test_dims);
	PG_ADD_TEST(suite, out_svg_test_relative);
	PG_ADD_TEST(suite, out_svg_test_geoms);
	PG_ADD_TEST(suite, out_svg_test_srid);
}
