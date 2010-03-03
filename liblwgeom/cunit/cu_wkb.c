/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
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
static int init_wkb_suite(void)
{
	s = NULL;
	t = NULL;
	return 0;
}

/*
** The suite cleanup function.
** Frees any global objects.
*/
static int clean_wkb_suite(void)
{
	if (s) free(s);
	if (t) free(t);
	s = NULL;
	t = NULL;
	return 0;
}

static void cu_wkb(char *wkt)
{
	LWGEOM *g = lwgeom_from_ewkt(wkt, PARSER_CHECK_NONE);
	if ( s ) free(s);
	if ( t ) free(t);
	s = lwgeom_to_wkb(g, WKB_HEX | WKB_NDR | WKB_EXTENDED, NULL);	
	t = lwgeom_to_hexwkb(g, PARSER_CHECK_NONE, NDR);
	lwgeom_free(g);
}

static void test_wkb_point(void)
{
	cu_wkb("POINT(0 0 0 0)");
	CU_ASSERT_STRING_EQUAL(s, t);

	cu_wkb("LINESTRING(0 0,1 1)");
	CU_ASSERT_STRING_EQUAL(s, t);

	cu_wkb("SRID=4;POLYGON((0 0 0,0 1 0,1 1 0,1 0 0,0 0 0))");
	CU_ASSERT_STRING_EQUAL(s, t);

	cu_wkb("SRID=14;GEOMETRYCOLLECTION(POLYGON((0 0 0,0 1 0,1 1 0,1 0 0,0 0 0)),POINT(1 1 1))");
	CU_ASSERT_STRING_EQUAL(s, t);

	cu_wkb("SRID=14;MULTIPOLYGON(((0 0 0,0 1 0,1 1 0,1 0 0,0 0 0)),((-1 -1 0,-1 2 0,2 2 0,2 -1 0,-1 -1 0),(0 0 0,0 1 0,1 1 0,1 0 0,0 0 0)))");
	CU_ASSERT_STRING_EQUAL(s, t);

//	printf("new: %s\nold: %s\n",s,t);

}

static void test_wkb_linestring(void){}

static void test_wkb_polygon(void){}

static void test_wkb_multipoint(void){}

static void test_wkb_multilinestring(void){}

static void test_wkb_multipolygon(void){}

static void test_wkb_collection(void){}

static void test_wkb_circularstring(void){}

static void test_wkb_compoundcurve(void){}

static void test_wkb_curvpolygon(void){}

static void test_wkb_multicurve(void){}

static void test_wkb_multisurface(void){}


/*
** Used by test harness to register the tests in this file.
*/

CU_TestInfo wkb_tests[] = {
	PG_TEST(test_wkb_point),
	PG_TEST(test_wkb_linestring),
	PG_TEST(test_wkb_polygon),
	PG_TEST(test_wkb_multipoint),
	PG_TEST(test_wkb_multilinestring),
	PG_TEST(test_wkb_multipolygon),
	PG_TEST(test_wkb_collection),
	PG_TEST(test_wkb_circularstring),
	PG_TEST(test_wkb_compoundcurve),
	PG_TEST(test_wkb_curvpolygon),
	PG_TEST(test_wkb_multicurve),
	PG_TEST(test_wkb_multisurface),
	CU_TEST_INFO_NULL
};
CU_SuiteInfo wkb_suite = {"WKB Suite",  init_wkb_suite,  clean_wkb_suite, wkb_tests};
