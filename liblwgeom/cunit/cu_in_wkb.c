/**********************************************************************
 * $Id: cu_out_wkb.c 6036 2010-10-03 18:14:35Z pramsey $
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2010 Paul Ramsey <pramsey@cleverelephant.ca>
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

/*
** Global variable to hold WKB strings
*/
char *hex_a;
char *hex_b;

/*
** The suite initialization function.
** Create any re-used objects.
*/
static int init_wkb_in_suite(void)
{
	hex_a = NULL;
	hex_b = NULL;
	return 0;
}

/*
** The suite cleanup function.
** Frees any global objects.
*/
static int clean_wkb_in_suite(void)
{
	if (hex_a) free(hex_a);
	if (hex_b) free(hex_b);
	hex_a = NULL;
	hex_b = NULL;
	return 0;
}

static void cu_wkb_in(char *wkt)
{
	LWGEOM_PARSER_RESULT pr;
	LWGEOM *g_a, *g_b;
	uint8_t *wkb_a, *wkb_b;
	size_t wkb_size_a, wkb_size_b;
	/* int i; char *hex; */
	
	if ( hex_a ) free(hex_a);
	if ( hex_b ) free(hex_b);

	/* Turn WKT into geom */
	lwgeom_parse_wkt(&pr, wkt, LW_PARSER_CHECK_NONE);
	if ( pr.errcode ) 
	{
		printf("ERROR: %s\n", pr.message);
		printf("POSITION: %d\n", pr.errlocation);
		exit(0);
	}

	/* Get the geom */
	g_a = pr.geom;
	
	/* Turn geom into WKB */
	wkb_a = lwgeom_to_wkb(g_a, WKB_NDR | WKB_EXTENDED, &wkb_size_a);

	/* Turn WKB back into geom  */
	g_b = lwgeom_from_wkb(wkb_a, wkb_size_a, LW_PARSER_CHECK_NONE);

	/* Turn geom to WKB again */
	wkb_b = lwgeom_to_wkb(g_b, WKB_NDR | WKB_EXTENDED, &wkb_size_b);

	/* Turn geoms into WKB for comparisons */
	hex_a = hexbytes_from_bytes(wkb_a, wkb_size_a);
	hex_b = hexbytes_from_bytes(wkb_b, wkb_size_b);

	/* Clean up */
	lwfree(wkb_a);
	lwfree(wkb_b);
	lwgeom_parser_result_free(&pr);
	lwgeom_free(g_b);
}

static void test_wkb_in_point(void)
{
	cu_wkb_in("POINT(0 0 0 0)");
//	printf("old: %s\nnew: %s\n",hex_a, hex_b);
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

	cu_wkb_in("SRID=4;POINTM(1 1 1)");
//	printf("old: %s\nnew: %s\n",hex_a, hex_b);
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);
}

static void test_wkb_in_linestring(void)
{
	cu_wkb_in("LINESTRING(0 0,1 1)");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

	cu_wkb_in("LINESTRING(0 0 1,1 1 2,2 2 3)");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);
}

static void test_wkb_in_polygon(void)
{
	cu_wkb_in("SRID=4;POLYGON((0 0 0,0 1 0,1 1 0,1 0 0,0 0 0))");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

	cu_wkb_in("SRID=14;POLYGON((0 0 0 1,0 1 0 2,1 1 0 3,1 0 0 4,0 0 0 5))");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

	cu_wkb_in("SRID=4;POLYGON((0 0 0 1,0 1 0 2,1 1 0 3,1 0 0 4,0 0 0 5))");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

	cu_wkb_in("POLYGON EMPTY");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);
}

static void test_wkb_in_multipoint(void) 
{
	cu_wkb_in("SRID=4;MULTIPOINT(0 0 0,0 1 0,1 1 0,1 0 0,0 0 1)");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

	cu_wkb_in("MULTIPOINT(0 0 0, 0.26794919243112270647255365849413 1 3)");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);
}

static void test_wkb_in_multilinestring(void) {}

static void test_wkb_in_multipolygon(void)
{
	cu_wkb_in("SRID=14;MULTIPOLYGON(((0 0 0,0 1 0,1 1 0,1 0 0,0 0 0)),((-1 -1 0,-1 2 0,2 2 0,2 -1 0,-1 -1 0),(0 0 0,0 1 0,1 1 0,1 0 0,0 0 0)))");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);
	//printf("old: %s\nnew: %s\n",hex_a, hex_b);
}

static void test_wkb_in_collection(void)
{
	cu_wkb_in("SRID=14;GEOMETRYCOLLECTION(POLYGON((0 0 0,0 1 0,1 1 0,1 0 0,0 0 0)),POINT(1 1 1))");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

	cu_wkb_in("GEOMETRYCOLLECTION EMPTY");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

	cu_wkb_in("SRID=14;GEOMETRYCOLLECTION(MULTIPOLYGON(((0 0 0,0 1 0,1 1 0,1 0 0,0 0 0))),POLYGON((0 0 0,0 1 0,1 1 0,1 0 0,0 0 0)),POINT(1 1 1),LINESTRING(0 0 0, 1 1 1))");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

}

static void test_wkb_in_circularstring(void) 
{
	cu_wkb_in("CIRCULARSTRING(0 -2,-2 0,0 2,2 0,0 -2)");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

	cu_wkb_in("CIRCULARSTRING(-5 0 0 4, 0 5 1 3, 5 0 2 2, 10 -5 3 1, 15 0 4 0)");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

	cu_wkb_in("SRID=43;CIRCULARSTRING(-5 0 0 4, 0 5 1 3, 5 0 2 2, 10 -5 3 1, 15 0 4 0)");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);
}

static void test_wkb_in_compoundcurve(void) 
{
	cu_wkb_in("COMPOUNDCURVE(CIRCULARSTRING(0 0 0, 0.26794919243112270647255365849413 1 3, 0.5857864376269049511983112757903 1.4142135623730950488016887242097 1),(0.5857864376269049511983112757903 1.4142135623730950488016887242097 1,2 0 0,0 0 0))");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);
}

static void test_wkb_in_curvpolygon(void) 
{
	cu_wkb_in("CURVEPOLYGON(CIRCULARSTRING(-2 0 0 0,-1 -1 1 2,0 0 2 4,1 -1 3 6,2 0 4 8,0 2 2 4,-2 0 0 0),(-1 0 1 2,0 0.5 2 4,1 0 3 6,0 1 3 4,-1 0 1 2))");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);
}

static void test_wkb_in_multicurve(void) {}

static void test_wkb_in_multisurface(void) {}


/*
** Used by test harness to register the tests in this file.
*/

CU_TestInfo wkb_in_tests[] =
{
	PG_TEST(test_wkb_in_point),
	PG_TEST(test_wkb_in_linestring),
	PG_TEST(test_wkb_in_polygon),
	PG_TEST(test_wkb_in_multipoint),
	PG_TEST(test_wkb_in_multilinestring),
	PG_TEST(test_wkb_in_multipolygon),
	PG_TEST(test_wkb_in_collection),
	PG_TEST(test_wkb_in_circularstring),
	PG_TEST(test_wkb_in_compoundcurve),
	PG_TEST(test_wkb_in_curvpolygon),
	PG_TEST(test_wkb_in_multicurve),
	PG_TEST(test_wkb_in_multisurface),
	CU_TEST_INFO_NULL
};
CU_SuiteInfo wkb_in_suite = {"WKB In Suite",  init_wkb_in_suite,  clean_wkb_in_suite, wkb_in_tests};
