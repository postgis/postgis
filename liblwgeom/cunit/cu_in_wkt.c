/**********************************************************************
 * $Id: cu_out_wkt.c 6036 2010-10-03 18:14:35Z pramsey $
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
** The suite initialization function.
** Create any re-used objects.
*/
static int init_wkt_in_suite(void)
{
	return 0;
}

/*
** The suite cleanup function.
** Frees any global objects.
*/
static int clean_wkt_in_suite(void)
{
	return 0;
}

/* 
* Return a char* that results from taking the input 
* WKT, creating an LWGEOM, then writing it back out
* as an output WKT using the supplied variant.
* If there is an error, return that.
*/
static char* cu_wkt_in(char *wkt, uchar variant)
{
	LWGEOM_PARSER_RESULT p;
	int rv = 0;
	char *s = 0;
	
	rv = lwgeom_from_wkt(&p, wkt, 0);
	if( p.errcode ) {
		return strdup(p.message);
	}
	s = lwgeom_to_wkt(p.geom, variant, 8, NULL);
	lwgeom_parser_result_free(&p);
	return s;
}

static void test_wkt_in_point(void)
{
	char *s;
	char *r;
	
	s = "POINT(0 0)";
	r = cu_wkt_in(s, WKT_SFSQL);
	CU_ASSERT_STRING_EQUAL(r,s);
	lwfree(r);
}

static void test_wkt_in_linestring(void)
{
	char *s;
	char *r;
	
	s = "LINESTRING(0 0,1 1)";
	r = cu_wkt_in(s, WKT_SFSQL);
	CU_ASSERT_STRING_EQUAL(r,s);
	lwfree(r);

	s = "LINESTRING(0 0 0,1 1 1)";
	r = cu_wkt_in(s, WKT_EXTENDED);
	CU_ASSERT_STRING_EQUAL(r,s);
	lwfree(r);

	s = "LINESTRING M (0 0 0,1 1 1)";
	r = cu_wkt_in(s, WKT_ISO);
	CU_ASSERT_STRING_EQUAL(r,s);
	lwfree(r);

	s = "LINESTRING ZM (0 0 0 1,1 1 1 1,2 2 2 2,0.141231 4 5 4)";
	r = cu_wkt_in(s, WKT_ISO);
	CU_ASSERT_STRING_EQUAL(r,s);
	lwfree(r);

	s = "LINESTRINGM(0 0 0,1 1 1)";
	r = cu_wkt_in(s, WKT_EXTENDED);
	CU_ASSERT_STRING_EQUAL(r,s);
	//printf("\nIN:  %s\nOUT: %s\n",s,r);
	lwfree(r);
}

static void test_wkt_in_polygon(void)
{
}

static void test_wkt_in_multipoint(void)
{
	char *s;
	char *r;

	s = "MULTIPOINT(0 0)";
	r = cu_wkt_in(s, WKT_SFSQL);
	CU_ASSERT_STRING_EQUAL(r,s);
	lwfree(r);

	s = "MULTIPOINT(0 0,1 1)";
	r = cu_wkt_in(s, WKT_SFSQL);
	CU_ASSERT_STRING_EQUAL(r,s);
	//printf("\nIN:  %s\nOUT: %s\n",s,r);
	lwfree(r);
}

static void test_wkt_in_multilinestring(void)
{
}

static void test_wkt_in_multipolygon(void)
{
}

static void test_wkt_in_collection(void)
{
}

static void test_wkt_in_circularstring(void)
{
}

static void test_wkt_in_compoundcurve(void)
{
}

static void test_wkt_in_curvpolygon(void)
{
}

static void test_wkt_in_multicurve(void)
{
}

static void test_wkt_in_multisurface(void)
{
}

/*
** Used by test harness to register the tests in this file.
*/

CU_TestInfo wkt_in_tests[] =
{
	PG_TEST(test_wkt_in_point),
	PG_TEST(test_wkt_in_linestring),
	PG_TEST(test_wkt_in_polygon),
	PG_TEST(test_wkt_in_multipoint),
	PG_TEST(test_wkt_in_multilinestring),
	PG_TEST(test_wkt_in_multipolygon),
	PG_TEST(test_wkt_in_collection),
	PG_TEST(test_wkt_in_circularstring),
	PG_TEST(test_wkt_in_compoundcurve),
	PG_TEST(test_wkt_in_curvpolygon),
	PG_TEST(test_wkt_in_multicurve),
	PG_TEST(test_wkt_in_multisurface),
	CU_TEST_INFO_NULL
};
CU_SuiteInfo wkt_in_suite = {"WKT In Suite",  init_wkt_in_suite,  clean_wkt_in_suite, wkt_in_tests};
