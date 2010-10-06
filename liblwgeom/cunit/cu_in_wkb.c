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

#include "libgeom.h"
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

static void cu_wkb_out(char *wkt)
{
	LWGEOM *g, *g_a, *g_b;
	char *wkb, *hex;
	size_t wkb_size, wkb_size_a, wkb_size_b;
	int i;
	
	if ( hex_a ) free(hex_a);
	if ( hex_b ) free(hex_b);

	/* Turn WKT into geom */
	g = lwgeom_from_ewkt(wkt, PARSER_CHECK_NONE);
	
	/* Turn geom into WKB */
	wkb = lwgeom_to_wkb(g, WKB_NDR | WKB_EXTENDED, &wkb_size);
	/* Flip WKB into hex for string comparisons */
	hex = lwalloc(2*wkb_size+1);
	for (i=0; i<wkb_size; i++)
		deparse_hex(wkb[i], &hex[i*2]);
	hex[2*wkb_size] = '\0';
	//printf("%s\n",hex);
	lwfree(hex);

	/* Turn WKB back into geom with old function */
	g_a = lwgeom_from_ewkb((uchar*)wkb, 0, wkb_size);
	/* Turn WKB back into geom with new function */
	g_b = lwgeom_from_wkb((uchar*)wkb, wkb_size, 0);

	/* Turn geoms into WKB for comparisons */
	hex_a = lwgeom_to_wkb(g_a, WKB_HEX | WKB_NDR | WKB_EXTENDED, &wkb_size_a);
	hex_b = lwgeom_to_wkb(g_b, WKB_HEX | WKB_NDR | WKB_EXTENDED, &wkb_size_b);

	/* Clean up */
	lwfree(wkb);
	lwgeom_free(g);
	lwgeom_free(g_a);
	lwgeom_free(g_b);
}

static void test_wkb_in_point(void)
{
	cu_wkb_out("POINT(0 0 0 0)");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);
//	printf("old: %s\nnew: %s\n",hex_a, hex_b);

	cu_wkb_out("SRID=4;POINTM(1 1 1)");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);
//	printf("old: %s\nnew: %s\n",hex_a, hex_b);
}

static void test_wkb_in_linestring(void)
{
	cu_wkb_out("LINESTRING(0 0,1 1)");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

	cu_wkb_out("LINESTRING(0 0 1,1 1 2,2 2 3)");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);
}

static void test_wkb_in_polygon(void)
{
	cu_wkb_out("SRID=4;POLYGON((0 0 0,0 1 0,1 1 0,1 0 0,0 0 0))");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

	cu_wkb_out("SRID=14;POLYGON((0 0 0 1,0 1 0 2,1 1 0 3,1 0 0 4,0 0 0 5))");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

	cu_wkb_out("SRID=4;POLYGON((0 0 0 1,0 1 0 2,1 1 0 3,1 0 0 4,0 0 0 5))");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

	cu_wkb_out("POLYGON EMPTY");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);
}

static void test_wkb_in_multipoint(void) {}

static void test_wkb_in_multilinestring(void) {}

static void test_wkb_in_multipolygon(void)
{
	cu_wkb_out("SRID=14;MULTIPOLYGON(((0 0 0,0 1 0,1 1 0,1 0 0,0 0 0)),((-1 -1 0,-1 2 0,2 2 0,2 -1 0,-1 -1 0),(0 0 0,0 1 0,1 1 0,1 0 0,0 0 0)))");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);
	//printf("old: %s\nnew: %s\n",hex_a, hex_b);
}

static void test_wkb_in_collection(void)
{
	cu_wkb_out("SRID=14;GEOMETRYCOLLECTION(POLYGON((0 0 0,0 1 0,1 1 0,1 0 0,0 0 0)),POINT(1 1 1))");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);

	cu_wkb_out("GEOMETRYCOLLECTION EMPTY");
	CU_ASSERT_STRING_EQUAL(hex_a, hex_b);
}

static void test_wkb_in_circularstring(void) {}

static void test_wkb_in_compoundcurve(void) {}

static void test_wkb_in_curvpolygon(void) {}

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
