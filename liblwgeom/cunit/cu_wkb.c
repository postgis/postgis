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

/*
** The suite initialization function.
** Create any re-used objects.
*/
static int init_wkb_suite(void)
{
	s = NULL;
	return 0;
}

/*
** The suite cleanup function.
** Frees any global objects.
*/
static int clean_wkb_suite(void)
{
	free(s);
	s = NULL;
	return 0;
}

static char* cu_wkb(char *wkt, uchar variant)
{
	LWGEOM *g = lwgeom_from_ewkt(wkt, PARSER_CHECK_NONE);
	if ( s ) free(s);
	s = lwgeom_to_wkb(g, variant | WKB_HEX, NULL);	
	lwgeom_free(g);
	return s;
}

static void test_wkb_point(void)
{
	//printf("%s\n", cu_wkb("POINT(0 0 0 0)", WKB_ISO ));
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
