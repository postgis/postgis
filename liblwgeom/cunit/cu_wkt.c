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
** Global variable to hold WKT strings
*/
char *s;

/*
** The suite initialization function.
** Create any re-used objects.
*/
static int init_wkt_suite(void)
{
	s = NULL;
	return 0;
}

/*
** The suite cleanup function.
** Frees any global objects.
*/
static int clean_wkt_suite(void)
{
	free(s);
	s = NULL;
	return 0;
}

static char* cu_wkt(char *wkt, uchar variant)
{
	LWGEOM *g = lwgeom_from_ewkt(wkt, PARSER_CHECK_NONE);
	if ( s ) free(s);
	s = lwgeom_to_wkt(g, variant, 8, NULL);	
	lwgeom_free(g);
	return s;
}

static void test_wkt_point(void)
{
	CU_ASSERT_STRING_EQUAL(cu_wkt("POINT(0.1111 0.1111 0.1111 0)",WKT_ISO), "POINT ZM (0.1111 0.1111 0.1111 0)");
	CU_ASSERT_STRING_EQUAL(cu_wkt("POINT(0 0 0 0)",WKT_EXTENDED), "POINT(0 0 0 0)");
	CU_ASSERT_STRING_EQUAL(cu_wkt("POINT(0 0 0 0)",WKT_SFSQL), "POINT(0 0)");

	CU_ASSERT_STRING_EQUAL(cu_wkt("POINTM(0 0 0)",WKT_ISO), "POINT M (0 0 0)");
	CU_ASSERT_STRING_EQUAL(cu_wkt("POINTM(0 0 0)",WKT_EXTENDED), "POINTM(0 0 0)");
	CU_ASSERT_STRING_EQUAL(cu_wkt("POINTM(0 0 0)",WKT_SFSQL), "POINT(0 0)");

	CU_ASSERT_STRING_EQUAL(cu_wkt("POINT(100 100)",WKT_ISO), "POINT(100 100)");
	CU_ASSERT_STRING_EQUAL(cu_wkt("POINT(100 100)",WKT_EXTENDED), "POINT(100 100)");
	CU_ASSERT_STRING_EQUAL(cu_wkt("POINT(100 100)",WKT_SFSQL), "POINT(100 100)");

	CU_ASSERT_STRING_EQUAL(cu_wkt("POINT(100.1 100 12 12)",WKT_ISO), "POINT ZM (100.1 100 12 12)");
	CU_ASSERT_STRING_EQUAL(cu_wkt("POINT(100.1 100 12 12)",WKT_EXTENDED), "POINT(100.1 100 12 12)");
	CU_ASSERT_STRING_EQUAL(cu_wkt("POINT(100.1 100 12 12)",WKT_SFSQL), "POINT(100.1 100)");

	CU_ASSERT_STRING_EQUAL(cu_wkt("SRID=100;POINT(100.1 100 12 12)",WKT_SFSQL), "POINT(100.1 100)");
	CU_ASSERT_STRING_EQUAL(cu_wkt("SRID=100;POINT(100.1 100 12 12)",WKT_EXTENDED), "SRID=100;POINT(100.1 100 12 12)");
//	printf("%s\n",cu_wkt("SRID=100;POINT(100.1 100 12 12)",WKT_EXTENDED));

}

static void test_wkt_linestring(void) 
{
	CU_ASSERT_STRING_EQUAL(cu_wkt("LINESTRING(1 2 3 4,5 6 7 8)",WKT_ISO), "LINESTRING ZM (1 2 3 4,5 6 7 8)");
	CU_ASSERT_STRING_EQUAL(cu_wkt("LINESTRING(1 2 3,5 6 7)",WKT_ISO), "LINESTRING Z (1 2 3,5 6 7)");
	CU_ASSERT_STRING_EQUAL(cu_wkt("LINESTRINGM(1 2 3,5 6 7)",WKT_ISO), "LINESTRING M (1 2 3,5 6 7)");
}

static void test_wkt_polygon(void) 
{
	CU_ASSERT_STRING_EQUAL(
	  cu_wkt("POLYGON((100 100 2, 100 200 2, 200 200 2, 200 100 2, 100 100 2))",WKT_ISO), 
      "POLYGON Z ((100 100 2,100 200 2,200 200 2,200 100 2,100 100 2))"
    );
	CU_ASSERT_STRING_EQUAL(
	  cu_wkt("POLYGON((100 100 2, 100 200 2, 200 200 2, 200 100 2, 100 100 2))",WKT_EXTENDED), 
	  "POLYGON((100 100 2,100 200 2,200 200 2,200 100 2,100 100 2))"
	);
}
static void test_wkt_multipoint(void) 
{
	CU_ASSERT_STRING_EQUAL(cu_wkt("MULTIPOINT(1 2 3 4,5 6 7 8)",WKT_ISO), "MULTIPOINT ZM (1 2 3 4,5 6 7 8)");
	CU_ASSERT_STRING_EQUAL(cu_wkt("MULTIPOINT(1 2 3,5 6 7)",WKT_ISO), "MULTIPOINT Z (1 2 3,5 6 7)");
	CU_ASSERT_STRING_EQUAL(cu_wkt("MULTIPOINTM(1 2 3,5 6 7)",WKT_ISO), "MULTIPOINT M (1 2 3,5 6 7)");
	
}

static void test_wkt_multilinestring(void) 
{
	CU_ASSERT_STRING_EQUAL(
	  cu_wkt("MULTILINESTRING((1 2 3 4,5 6 7 8))",WKT_ISO), 
	  "MULTILINESTRING ZM ((1 2 3 4,5 6 7 8))"
	);
	CU_ASSERT_STRING_EQUAL(
	  cu_wkt("MULTILINESTRING((1 2 3,5 6 7))",WKT_ISO), 
	  "MULTILINESTRING Z ((1 2 3,5 6 7))"
	);
	CU_ASSERT_STRING_EQUAL(
	  cu_wkt("MULTILINESTRINGM((1 2 3,5 6 7))",WKT_ISO), 
	  "MULTILINESTRING M ((1 2 3,5 6 7))"
	);
}

static void test_wkt_multipolygon(void) 
{
	CU_ASSERT_STRING_EQUAL(
	  cu_wkt("MULTIPOLYGON(((100 100 2, 100 200 2, 200 200 2, 200 100 2, 100 100 2)))",WKT_ISO), 
      "MULTIPOLYGON Z (((100 100 2,100 200 2,200 200 2,200 100 2,100 100 2)))"
	);
	CU_ASSERT_STRING_EQUAL(
	  cu_wkt("MULTIPOLYGON(((100 100 2, 100 200 2, 200 200 2, 200 100 2, 100 100 2)))",WKT_EXTENDED), 
	  "MULTIPOLYGON(((100 100 2,100 200 2,200 200 2,200 100 2,100 100 2)))"
	);
}

static void test_wkt_collection(void) 
{
	//printf("%s\n",cu_wkt("GEOMETRYCOLLECTION(MULTIPOLYGON(((100 100 2, 100 200 2, 200 200 2, 200 100 2, 100 100 2))),MULTIPOINT(.5 .5 .5,1 1 1),CURVEPOLYGON((.8 .8 .8,.8 .8 .8,.8 .8 .8)))",WKT_ISO));
	CU_ASSERT_STRING_EQUAL(
	  cu_wkt("GEOMETRYCOLLECTION(POLYGON((100 100 2, 100 200 2, 200 200 2, 200 100 2, 100 100 2)),POINT(.5 .5 .5),CIRCULARSTRING(.8 .8 .8,.8 .8 .8,.8 .8 .8))",WKT_ISO), 
      "GEOMETRYCOLLECTION Z (POLYGON Z ((100 100 2,100 200 2,200 200 2,200 100 2,100 100 2)),POINT Z (0.5 0.5 0.5),CIRCULARSTRING Z (0.8 0.8 0.8,0.8 0.8 0.8,0.8 0.8 0.8))"
	);
	CU_ASSERT_STRING_EQUAL(
	  cu_wkt("GEOMETRYCOLLECTION(MULTIPOLYGON(((100 100 2, 100 200 2, 200 200 2, 200 100 2, 100 100 2))),MULTIPOINT(.5 .5 .5,1 1 1),CURVEPOLYGON((.8 .8 .8,.8 .8 .8,.8 .8 .8)))",WKT_ISO), 
      "GEOMETRYCOLLECTION Z (MULTIPOLYGON Z (((100 100 2,100 200 2,200 200 2,200 100 2,100 100 2))),MULTIPOINT Z (0.5 0.5 0.5,1 1 1),CURVEPOLYGON Z ((0.8 0.8 0.8,0.8 0.8 0.8,0.8 0.8 0.8)))"
	);
}

static void test_wkt_circularstring(void) 
{
	CU_ASSERT_STRING_EQUAL(
	  cu_wkt("CIRCULARSTRING(1 2 3 4,4 5 6 7,7 8 9 0)",WKT_ISO), 
      "CIRCULARSTRING ZM (1 2 3 4,4 5 6 7,7 8 9 0)"
	);
	CU_ASSERT_STRING_EQUAL(
	  cu_wkt("CIRCULARSTRING(1 2 3 4,4 5 6 7,7 8 9 0)",WKT_EXTENDED), 
	  "CIRCULARSTRING(1 2 3 4,4 5 6 7,7 8 9 0)"
	);
	//printf("%s\n",cu_wkt("GEOMETRYCOLLECTION(MULTIPOLYGON(((100 100 2, 100 200 2, 200 200 2, 200 100 2, 100 100 2))),MULTIPOINT(.5 .5 .5,1 1 1),CURVEPOLYGON((.8 .8 .8,.8 .8 .8,.8 .8 .8)))",WKT_ISO));
}

static void test_wkt_compoundcurve(void)
{
	CU_ASSERT_STRING_EQUAL(
	  cu_wkt("COMPOUNDCURVE((1 2 3 4,4 5 6 7,7 8 9 0),CIRCULARSTRING(1 2 3 4,4 5 6 7,7 8 9 0))",WKT_ISO), 
      "COMPOUNDCURVE ZM ((1 2 3 4,4 5 6 7,7 8 9 0),CIRCULARSTRING ZM (1 2 3 4,4 5 6 7,7 8 9 0))"
	);
}

static void test_wkt_curvpolygon(void)
{
	CU_ASSERT_STRING_EQUAL(
	  cu_wkt("CURVEPOLYGON((1 2 3 4,4 5 6 7,7 8 9 0),CIRCULARSTRING(1 2 3 4,4 5 6 7,7 8 9 0))",WKT_ISO), 
      "CURVEPOLYGON ZM ((1 2 3 4,4 5 6 7,7 8 9 0),CIRCULARSTRING ZM (1 2 3 4,4 5 6 7,7 8 9 0))"
	);
}

static void test_wkt_multicurve(void)
{
	CU_ASSERT_STRING_EQUAL(
	  cu_wkt("MULTICURVE((1 2 3 4,4 5 6 7,7 8 9 0),CIRCULARSTRING(1 2 3 4,4 5 6 7,7 8 9 0))",WKT_ISO), 
      "MULTICURVE ZM ((1 2 3 4,4 5 6 7,7 8 9 0),CIRCULARSTRING ZM (1 2 3 4,4 5 6 7,7 8 9 0))"
	);
}

static void test_wkt_multisurface(void)
{
	CU_ASSERT_STRING_EQUAL(
	  cu_wkt("MULTISURFACE(((1 2 3 4,4 5 6 7,7 8 9 0)),CURVEPOLYGON((1 2 3 4,4 5 6 7,7 8 9 0)))",WKT_ISO), 
      "MULTISURFACE ZM (((1 2 3 4,4 5 6 7,7 8 9 0)),CURVEPOLYGON ZM ((1 2 3 4,4 5 6 7,7 8 9 0)))"
	);
	
}

/*
** Used by test harness to register the tests in this file.
*/

CU_TestInfo wkt_tests[] = {
	PG_TEST(test_wkt_point),
	PG_TEST(test_wkt_linestring),
	PG_TEST(test_wkt_polygon),
	PG_TEST(test_wkt_multipoint),
	PG_TEST(test_wkt_multilinestring),
	PG_TEST(test_wkt_multipolygon),
	PG_TEST(test_wkt_collection),
	PG_TEST(test_wkt_circularstring),
	PG_TEST(test_wkt_compoundcurve),
	PG_TEST(test_wkt_curvpolygon),
	PG_TEST(test_wkt_multicurve),
	PG_TEST(test_wkt_multisurface),
	CU_TEST_INFO_NULL
};
CU_SuiteInfo wkt_suite = {"WKT Suite",  init_wkt_suite,  clean_wkt_suite, wkt_tests};
