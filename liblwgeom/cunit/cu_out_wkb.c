/**********************************************************************
 * $Id$
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
char *s;
char *t;

/*
** The suite initialization function.
** Create any re-used objects.
*/
static int init_wkb_out_suite(void)
{
	s = NULL;
	t = NULL;
	return 0;
}

/*
** The suite cleanup function.
** Frees any global objects.
*/
static int clean_wkb_out_suite(void)
{
	if (s) free(s);
	if (t) free(t);
	s = NULL;
	t = NULL;
	return 0;
}

static void cu_wkb_out(char *wkt)
{
	LWGEOM *g = lwgeom_from_ewkt(wkt, PARSER_CHECK_NONE);
	if ( s ) free(s);
	if ( t ) free(t);
	s = lwgeom_to_wkb(g, WKB_HEX | WKB_NDR | WKB_EXTENDED, NULL);
	t = lwgeom_to_hexwkb(g, PARSER_CHECK_NONE, NDR);
	lwgeom_free(g);
}

static void test_wkb_out_point(void)
{
	cu_wkb_out("POINT(0 0 0 0)");
	CU_ASSERT_STRING_EQUAL(s, t);

	cu_wkb_out("SRID=4;POINTM(1 1 1)");
	CU_ASSERT_STRING_EQUAL(s, t);

	//printf("new: %s\nold: %s\n",s,t);
}

static void test_wkb_out_linestring(void)
{
	cu_wkb_out("LINESTRING(0 0,1 1)");
	CU_ASSERT_STRING_EQUAL(s, t);

	cu_wkb_out("LINESTRING(0 0 1,1 1 2,2 2 3)");
	CU_ASSERT_STRING_EQUAL(s, t);
}

static void test_wkb_out_polygon(void)
{
	cu_wkb_out("SRID=4;POLYGON((0 0 0,0 1 0,1 1 0,1 0 0,0 0 0))");
	CU_ASSERT_STRING_EQUAL(s, t);

	cu_wkb_out("SRID=14;POLYGON((0 0 0 1,0 1 0 2,1 1 0 3,1 0 0 4,0 0 0 5))");
	CU_ASSERT_STRING_EQUAL(s, t);

	cu_wkb_out("SRID=4;POLYGON((0 0 0 1,0 1 0 2,1 1 0 3,1 0 0 4,0 0 0 5))");
	CU_ASSERT_STRING_EQUAL(s, t);

	cu_wkb_out("POLYGON EMPTY");
	CU_ASSERT_STRING_EQUAL(s, t);
}

static void test_wkb_out_multipoint(void) {}

static void test_wkb_out_multilinestring(void) {}

static void test_wkb_out_multipolygon(void)
{
	cu_wkb_out("SRID=14;MULTIPOLYGON(((0 0 0,0 1 0,1 1 0,1 0 0,0 0 0)),((-1 -1 0,-1 2 0,2 2 0,2 -1 0,-1 -1 0),(0 0 0,0 1 0,1 1 0,1 0 0,0 0 0)))");
	CU_ASSERT_STRING_EQUAL(s, t);
}

static void test_wkb_out_collection(void)
{
	cu_wkb_out("SRID=14;GEOMETRYCOLLECTION(POLYGON((0 0 0,0 1 0,1 1 0,1 0 0,0 0 0)),POINT(1 1 1))");
	CU_ASSERT_STRING_EQUAL(s, t);

	cu_wkb_out("GEOMETRYCOLLECTION EMPTY");
	CU_ASSERT_STRING_EQUAL(s, t);
}

static void test_wkb_out_circularstring(void) {}

static void test_wkb_out_compoundcurve(void) {}

static void test_wkb_out_curvpolygon(void) {}

static void test_wkb_out_multicurve(void) {}

static void test_wkb_out_multisurface(void) {}


/*
** Used by test harness to register the tests in this file.
*/

CU_TestInfo wkb_out_tests[] =
{
	PG_TEST(test_wkb_out_point),
	PG_TEST(test_wkb_out_linestring),
	PG_TEST(test_wkb_out_polygon),
	PG_TEST(test_wkb_out_multipoint),
	PG_TEST(test_wkb_out_multilinestring),
	PG_TEST(test_wkb_out_multipolygon),
	PG_TEST(test_wkb_out_collection),
	PG_TEST(test_wkb_out_circularstring),
	PG_TEST(test_wkb_out_compoundcurve),
	PG_TEST(test_wkb_out_curvpolygon),
	PG_TEST(test_wkb_out_multicurve),
	PG_TEST(test_wkb_out_multisurface),
	CU_TEST_INFO_NULL
};
CU_SuiteInfo wkb_out_suite = {"WKB Out Suite",  init_wkb_out_suite,  clean_wkb_out_suite, wkb_out_tests};
