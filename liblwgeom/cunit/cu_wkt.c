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

#include "cu_wkt.h"

/*
** Called from test harness to register the tests in this file.
*/
CU_pSuite register_wkt_suite(void)
{
	CU_pSuite pSuite;
	pSuite = CU_add_suite("WKT Suite", init_wkt_suite, clean_wkt_suite);
	if (NULL == pSuite)
	{
		CU_cleanup_registry();
		return NULL;
	}

	if (
	    (NULL == CU_add_test(pSuite, "test_wkt_point()", test_wkt_point)) ||
	    (NULL == CU_add_test(pSuite, "test_wkt_linestring()", test_wkt_linestring)) ||
	    (NULL == CU_add_test(pSuite, "test_wkt_polygon()", test_wkt_polygon)) ||
	    (NULL == CU_add_test(pSuite, "test_wkt_multipoint()", test_wkt_multipoint)) ||
	    (NULL == CU_add_test(pSuite, "test_wkt_multilinestring()", test_wkt_multilinestring)) ||
	    (NULL == CU_add_test(pSuite, "test_wkt_multipolygon()", test_wkt_multipolygon)) ||
	    (NULL == CU_add_test(pSuite, "test_wkt_collection()", test_wkt_collection)) ||
	    (NULL == CU_add_test(pSuite, "test_wkt_circularstring()", test_wkt_circularstring)) ||
	    (NULL == CU_add_test(pSuite, "test_wkt_compoundcurve()", test_wkt_compoundcurve)) ||
	    (NULL == CU_add_test(pSuite, "test_wkt_curvpolygon()", test_wkt_curvpolygon)) || 
	    (NULL == CU_add_test(pSuite, "test_wkt_multicurve()", test_wkt_multicurve)) ||
	    (NULL == CU_add_test(pSuite, "test_wkt_multisurface()", test_wkt_multisurface)) 
	)
	{
		CU_cleanup_registry();
		return NULL;
	}
	return pSuite;
}

/*
** Global variable to hold WKT strings
*/
char *s;


/*
** The suite initialization function.
** Create any re-used objects.
*/
int init_wkt_suite(void)
{
	s = NULL;
	return 0;
}

/*
** The suite cleanup function.
** Frees any global objects.
*/
int clean_wkt_suite(void)
{
	free(s);
	s = NULL;
	return 0;
}

static char* cu_wkt(char *wkt, uchar variant)
{
	LWGEOM *g = lwgeom_from_ewkt(wkt, PARSER_CHECK_NONE);
	if ( s ) free(s);
	s = lwgeom_to_wkt(g, 8, variant);	
	lwgeom_free(g);
	return s;
}

void test_wkt_point(void)
{
	CU_ASSERT_STRING_EQUAL(cu_wkt("POINT(0 0 0 0)",WKT_ISO), "POINTZM(0 0 0 0)");
	CU_ASSERT_STRING_EQUAL(cu_wkt("POINT(0 0 0 0)",WKT_EXTENDED), "POINT(0 0 0 0)");
	CU_ASSERT_STRING_EQUAL(cu_wkt("POINT(0 0 0 0)",WKT_SFSQL), "POINT(0 0)");

	CU_ASSERT_STRING_EQUAL(cu_wkt("POINTM(0 0 0)",WKT_ISO), "POINTM(0 0 0)");
	CU_ASSERT_STRING_EQUAL(cu_wkt("POINTM(0 0 0)",WKT_EXTENDED), "POINTM(0 0 0)");
	CU_ASSERT_STRING_EQUAL(cu_wkt("POINTM(0 0 0)",WKT_SFSQL), "POINT(0 0)");

	CU_ASSERT_STRING_EQUAL(cu_wkt("POINT(100 100)",WKT_ISO), "POINT(100 100)");
	CU_ASSERT_STRING_EQUAL(cu_wkt("POINT(100 100)",WKT_EXTENDED), "POINT(100 100)");
	CU_ASSERT_STRING_EQUAL(cu_wkt("POINT(100 100)",WKT_SFSQL), "POINT(100 100)");

	CU_ASSERT_STRING_EQUAL(cu_wkt("POINT(100.1 100 12 12)",WKT_ISO), "POINTZM(100.1 100 12 12)");
	CU_ASSERT_STRING_EQUAL(cu_wkt("POINT(100.1 100 12 12)",WKT_EXTENDED), "POINT(100.1 100 12 12)");
	CU_ASSERT_STRING_EQUAL(cu_wkt("POINT(100.1 100 12 12)",WKT_SFSQL), "POINT(100.1 100)");
}

void test_wkt_linestring(void) 
{
	CU_ASSERT_STRING_EQUAL(cu_wkt("LINESTRING(1 2 3 4,5 6 7 8)",WKT_ISO), "LINESTRINGZM(1 2 3 4,5 6 7 8)");
	CU_ASSERT_STRING_EQUAL(cu_wkt("LINESTRING(1 2 3,5 6 7)",WKT_ISO), "LINESTRINGZ(1 2 3,5 6 7)");
	CU_ASSERT_STRING_EQUAL(cu_wkt("LINESTRINGM(1 2 3,5 6 7)",WKT_ISO), "LINESTRINGM(1 2 3,5 6 7)");
}

void test_wkt_polygon(void) 
{
	CU_ASSERT_STRING_EQUAL(
	cu_wkt("POLYGON((100 100 2, 100 200 2, 200 200 2, 200 100 2, 100 100 2))",WKT_ISO), 
           "POLYGONZ((100 100 2,100 200 2,200 200 2,200 100 2,100 100 2))");
	CU_ASSERT_STRING_EQUAL(
	cu_wkt("POLYGON((100 100 2, 100 200 2, 200 200 2, 200 100 2, 100 100 2))",WKT_EXTENDED), 
	       "POLYGON((100 100 2,100 200 2,200 200 2,200 100 2,100 100 2))");
}
void test_wkt_multipoint(void) 
{
	CU_ASSERT_STRING_EQUAL(cu_wkt("MULTIPOINT(1 2 3 4,5 6 7 8)",WKT_ISO), "MULTIPOINTZM(1 2 3 4,5 6 7 8)");
	CU_ASSERT_STRING_EQUAL(cu_wkt("MULTIPOINT(1 2 3,5 6 7)",WKT_ISO), "MULTIPOINTZ(1 2 3,5 6 7)");
	CU_ASSERT_STRING_EQUAL(cu_wkt("MULTIPOINTM(1 2 3,5 6 7)",WKT_ISO), "MULTIPOINTM(1 2 3,5 6 7)");
	
}

void test_wkt_multilinestring(void) 
{
	CU_ASSERT_STRING_EQUAL(
	cu_wkt("MULTILINESTRING((1 2 3 4,5 6 7 8))",WKT_ISO), 
	       "MULTILINESTRINGZM((1 2 3 4,5 6 7 8))");
	CU_ASSERT_STRING_EQUAL(
	cu_wkt("MULTILINESTRING((1 2 3,5 6 7))",WKT_ISO), 
	       "MULTILINESTRINGZ((1 2 3,5 6 7))");
	CU_ASSERT_STRING_EQUAL(
	cu_wkt("MULTILINESTRINGM((1 2 3,5 6 7))",WKT_ISO), 
	       "MULTILINESTRINGM((1 2 3,5 6 7))");	
}

void test_wkt_multipolygon(void) 
{
	CU_ASSERT_STRING_EQUAL(
	cu_wkt("MULTIPOLYGON(((100 100 2, 100 200 2, 200 200 2, 200 100 2, 100 100 2)))",WKT_ISO), 
           "MULTIPOLYGONZ(((100 100 2,100 200 2,200 200 2,200 100 2,100 100 2)))");
	CU_ASSERT_STRING_EQUAL(
	cu_wkt("MULTIPOLYGON(((100 100 2, 100 200 2, 200 200 2, 200 100 2, 100 100 2)))",WKT_EXTENDED), 
	       "MULTIPOLYGON(((100 100 2,100 200 2,200 200 2,200 100 2,100 100 2)))");
}

void test_wkt_collection(void) 
{
	CU_ASSERT_STRING_EQUAL(
	cu_wkt("GEOMETRYCOLLECTION(POLYGON((100 100 2, 100 200 2, 200 200 2, 200 100 2, 100 100 2)),POINT(.5 .5 .5),CIRCULARSTRING(.8 .8 .8,.8 .8 .8,.8 .8 .8))",WKT_ISO), 
           "GEOMETRYCOLLECTIONZ(POLYGONZ((100 100 2,100 200 2,200 200 2,200 100 2,100 100 2)),POINTZ(0.5 0.5 0.5),CIRCULARSTRINGZ(0.8 0.8 0.8,0.8 0.8 0.8,0.8 0.8 0.8))");
	CU_ASSERT_STRING_EQUAL(
	cu_wkt("GEOMETRYCOLLECTION(MULTIPOLYGON(((100 100 2, 100 200 2, 200 200 2, 200 100 2, 100 100 2))),MULTIPOINT(.5 .5 .5,1 1 1),CURVEPOLYGON((.8 .8 .8,.8 .8 .8,.8 .8 .8)))",WKT_ISO), 
           "GEOMETRYCOLLECTIONZ(MULTIPOLYGONZ(((100 100 2,100 200 2,200 200 2,200 100 2,100 100 2))),MULTIPOINTZ(0.5 0.5 0.5,1 1 1),CURVEPOLYGONZ((0.8 0.8 0.8,0.8 0.8 0.8,0.8 0.8 0.8)))");
	//printf("%s\n",cu_wkt("GEOMETRYCOLLECTION(MULTIPOLYGON(((100 100 2, 100 200 2, 200 200 2, 200 100 2, 100 100 2))),MULTIPOINT(.5 .5 .5,1 1 1),CURVEPOLYGON((.8 .8 .8,.8 .8 .8,.8 .8 .8)))",WKT_ISO));
}

void test_wkt_circularstring(void) 
{
	CU_ASSERT_STRING_EQUAL(
	cu_wkt("CIRCULARSTRING(1 2 3 4,4 5 6 7,7 8 9 0)",WKT_ISO), 
           "CIRCULARSTRINGZM(1 2 3 4,4 5 6 7,7 8 9 0)");
	CU_ASSERT_STRING_EQUAL(
	cu_wkt("CIRCULARSTRING(1 2 3 4,4 5 6 7,7 8 9 0)",WKT_EXTENDED), 
	       "CIRCULARSTRING(1 2 3 4,4 5 6 7,7 8 9 0)");
	//printf("%s\n",cu_wkt("GEOMETRYCOLLECTION(MULTIPOLYGON(((100 100 2, 100 200 2, 200 200 2, 200 100 2, 100 100 2))),MULTIPOINT(.5 .5 .5,1 1 1),CURVEPOLYGON((.8 .8 .8,.8 .8 .8,.8 .8 .8)))",WKT_ISO));
}

void test_wkt_compoundcurve(void)
{
	CU_ASSERT_STRING_EQUAL(
	cu_wkt("COMPOUNDCURVE((1 2 3 4,4 5 6 7,7 8 9 0),CIRCULARSTRING(1 2 3 4,4 5 6 7,7 8 9 0))",WKT_ISO), 
           "COMPOUNDCURVEZM((1 2 3 4,4 5 6 7,7 8 9 0),CIRCULARSTRINGZM(1 2 3 4,4 5 6 7,7 8 9 0))");
}

void test_wkt_curvpolygon(void)
{
	CU_ASSERT_STRING_EQUAL(
	cu_wkt("CURVEPOLYGON((1 2 3 4,4 5 6 7,7 8 9 0),CIRCULARSTRING(1 2 3 4,4 5 6 7,7 8 9 0))",WKT_ISO), 
           "CURVEPOLYGONZM((1 2 3 4,4 5 6 7,7 8 9 0),CIRCULARSTRINGZM(1 2 3 4,4 5 6 7,7 8 9 0))");
}

void test_wkt_multicurve(void)
{
	CU_ASSERT_STRING_EQUAL(
	cu_wkt("MULTICURVE((1 2 3 4,4 5 6 7,7 8 9 0),CIRCULARSTRING(1 2 3 4,4 5 6 7,7 8 9 0))",WKT_ISO), 
           "MULTICURVEZM((1 2 3 4,4 5 6 7,7 8 9 0),CIRCULARSTRINGZM(1 2 3 4,4 5 6 7,7 8 9 0))");
}

void test_wkt_multisurface(void)
{
	CU_ASSERT_STRING_EQUAL(
	cu_wkt("MULTISURFACE(((1 2 3 4,4 5 6 7,7 8 9 0)),CURVEPOLYGON((1 2 3 4,4 5 6 7,7 8 9 0)))",WKT_ISO), 
           "MULTISURFACEZM(((1 2 3 4,4 5 6 7,7 8 9 0)),CURVEPOLYGONZM((1 2 3 4,4 5 6 7,7 8 9 0)))");
	
}

