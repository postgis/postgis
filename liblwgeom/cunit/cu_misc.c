/**********************************************************************
 * $Id: cu_print.c 6160 2010-11-01 01:28:12Z pramsey $
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2008 Paul Ramsey
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


static void test_misc_force_2d(void)
{
	LWGEOM *geom;
	LWGEOM *geom2d;
	char *wkt_out;

	geom = lwgeom_from_wkt("CIRCULARSTRINGM(-5 0 4,0 5 3,5 0 2,10 -5 1,15 0 0)", PARSER_CHECK_NONE);
	geom2d = lwgeom_force_2d(geom);
	wkt_out = lwgeom_to_ewkt(geom2d);
	CU_ASSERT_STRING_EQUAL("CIRCULARSTRING(-5 0,0 5,5 0,10 -5,15 0)",wkt_out);
	lwgeom_free(geom);
	lwgeom_free(geom2d);
	lwfree(wkt_out);

	geom = lwgeom_from_wkt("GEOMETRYCOLLECTION(POINT(0 0 0),LINESTRING(1 1 1,2 2 2),POLYGON((0 0 1,0 1 1,1 1 1,1 0 1,0 0 1)),CURVEPOLYGON(CIRCULARSTRING(0 0 0,1 1 1,2 2 2,1 1 1,0 0 0)))", PARSER_CHECK_NONE);
	geom2d = lwgeom_force_2d(geom);
	wkt_out = lwgeom_to_ewkt(geom2d);
	CU_ASSERT_STRING_EQUAL("GEOMETRYCOLLECTION(POINT(0 0),LINESTRING(1 1,2 2),POLYGON((0 0,0 1,1 1,1 0,0 0)),CURVEPOLYGON(CIRCULARSTRING(0 0,1 1,2 2,1 1,0 0)))",wkt_out);
	lwgeom_free(geom);
	lwgeom_free(geom2d);
	lwfree(wkt_out);
}

static void test_misc_simplify(void)
{
	LWGEOM *geom;
	LWGEOM *geom2d;
	char *wkt_out;

	geom = lwgeom_from_wkt("LINESTRING(0 0,0 10,0 51,50 20,30 20,7 32)", PARSER_CHECK_NONE);
	geom2d = lwgeom_simplify(geom,2);
	wkt_out = lwgeom_to_ewkt(geom2d);
	CU_ASSERT_STRING_EQUAL("LINESTRING(0 0,0 51,50 20,30 20,7 32)",wkt_out);
	lwgeom_free(geom);
	lwgeom_free(geom2d);
	lwfree(wkt_out);

	geom = lwgeom_from_wkt("MULTILINESTRING((0 0,0 10,0 51,50 20,30 20,7 32))", PARSER_CHECK_NONE);
	geom2d = lwgeom_simplify(geom,2);
	wkt_out = lwgeom_to_ewkt(geom2d);
	CU_ASSERT_STRING_EQUAL("MULTILINESTRING((0 0,0 51,50 20,30 20,7 32))",wkt_out);
	lwgeom_free(geom);
	lwgeom_free(geom2d);
	lwfree(wkt_out);
}

static void test_misc_count_vertices(void)
{
	LWGEOM *geom;
	int count;

	geom = lwgeom_from_wkt("GEOMETRYCOLLECTION(POINT(0 0),LINESTRING(0 0,1 1),POLYGON((0 0,0 1,1 0,0 0)),CIRCULARSTRING(0 0,0 1,1 1),CURVEPOLYGON(CIRCULARSTRING(0 0,0 1,1 1)))", PARSER_CHECK_NONE);
	count = lwgeom_count_vertices(geom);
	CU_ASSERT_EQUAL(count,13);
	lwgeom_free(geom);

	geom = lwgeom_from_wkt("GEOMETRYCOLLECTION(CIRCULARSTRING(0 0,0 1,1 1),POINT(0 0),CURVEPOLYGON(CIRCULARSTRING(0 0,0 1,1 1,1 0,0 0)))", PARSER_CHECK_NONE);
	count = lwgeom_count_vertices(geom);
	CU_ASSERT_EQUAL(count,9);
	lwgeom_free(geom);

	geom = lwgeom_from_wkt("CURVEPOLYGON((0 0,1 0,0 1,0 0),CIRCULARSTRING(0 0,1 0,1 1,1 0,0 0))", PARSER_CHECK_NONE);
	count = lwgeom_count_vertices(geom);
	CU_ASSERT_EQUAL(count,9);
	lwgeom_free(geom);


	geom = lwgeom_from_wkt("POLYGON((0 0,1 0,0 1,0 0))", PARSER_CHECK_NONE);
	count = lwgeom_count_vertices(geom);
	CU_ASSERT_EQUAL(count,4);
	lwgeom_free(geom);

	geom = lwgeom_from_wkt("CURVEPOLYGON((0 0,1 0,0 1,0 0),CIRCULARSTRING(0 0,1 0,1 1,1 0,0 0))", PARSER_CHECK_NONE);
	count = lwgeom_count_vertices(geom);
	CU_ASSERT_EQUAL(count,9);
	lwgeom_free(geom);	
}

static void test_misc_area(void)
{
	LWGEOM *geom;
	double area;

	geom = lwgeom_from_wkt("LINESTRING EMPTY", PARSER_CHECK_ALL);
	area = lwgeom_area(geom);
	CU_ASSERT_DOUBLE_EQUAL(area, 0.0, 0.0001);	
	lwgeom_free(geom);
}

static void test_misc_wkb(void)
{
	static char *wkb = "010A0000000200000001080000000700000000000000000000C00000000000000000000000000000F0BF000000000000F0BF00000000000000000000000000000000000000000000F03F000000000000F0BF000000000000004000000000000000000000000000000000000000000000004000000000000000C00000000000000000010200000005000000000000000000F0BF00000000000000000000000000000000000000000000E03F000000000000F03F00000000000000000000000000000000000000000000F03F000000000000F0BF0000000000000000";
	LWGEOM *geom = lwgeom_from_hexwkb(wkb, PARSER_CHECK_ALL);
	char *str = lwgeom_to_wkt(geom, WKB_ISO, 8, 0);
	CU_ASSERT_STRING_EQUAL(str, "CURVEPOLYGON(CIRCULARSTRING(-2 0,-1 -1,0 0,1 -1,2 0,0 2,-2 0),(-1 0,0 0.5,1 0,0 1,-1 0))");
	lwfree(str);
	lwgeom_free(geom);
		
}

/*
** Used by the test harness to register the tests in this file.
*/
CU_TestInfo misc_tests[] =
{
	PG_TEST(test_misc_force_2d),
	PG_TEST(test_misc_simplify),
	PG_TEST(test_misc_count_vertices),
	PG_TEST(test_misc_area),
	PG_TEST(test_misc_wkb),
	CU_TEST_INFO_NULL
};
CU_SuiteInfo misc_suite = {"Misc Suite", NULL, NULL, misc_tests };
