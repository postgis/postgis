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
	    (NULL == CU_add_test(pSuite, "test_wkt_collection()", test_wkt_collection)) 
	)
	{
		CU_cleanup_registry();
		return NULL;
	}
	return pSuite;
}

/*
** The suite initialization function.
** Create any re-used objects.
*/
int init_wkt_suite(void)
{
	return 0;
}

/*
** The suite cleanup function.
** Frees any global objects.
*/
int clean_wkt_suite(void)
{
	return 0;
}

void test_wkt_point(void)
{
	LWGEOM *g;
	char *s;

	g = lwgeom_from_ewkt("POINT(0 0 0 0)", PARSER_CHECK_NONE);
	s = lwgeom_to_wkt(g, 14, WKT_ISO);
	CU_ASSERT_STRING_EQUAL(s, "POINTZM(0 0 0 0)");
	lwfree(s);

	s = lwgeom_to_wkt(g, 14, WKT_EXTENDED);
	CU_ASSERT_STRING_EQUAL(s, "POINT(0 0 0 0)");
	lwfree(s);

	s = lwgeom_to_wkt(g, 14, WKT_SFSQL);
	CU_ASSERT_STRING_EQUAL(s, "POINT(0 0)");
	lwfree(s);
	lwgeom_free(g);

	g = lwgeom_from_ewkt("POINTM(0 0 0)", PARSER_CHECK_NONE);
	s = lwgeom_to_wkt(g, 14, WKT_ISO);
	CU_ASSERT_STRING_EQUAL(s, "POINTM(0 0 0)");
	lwfree(s);

	s = lwgeom_to_wkt(g, 14, WKT_EXTENDED);
	CU_ASSERT_STRING_EQUAL(s, "POINTM(0 0 0)");
	lwfree(s);

	s = lwgeom_to_wkt(g, 14, WKT_SFSQL);
	CU_ASSERT_STRING_EQUAL(s, "POINT(0 0)");
	lwfree(s);
	lwgeom_free(g);

}

void test_wkt_linestring(void) 
{

}

void test_wkt_polygon(void) 
{

}
void test_wkt_multipoint(void) 
{
		
}

void test_wkt_multilinestring(void) 
{
	
}

void test_wkt_multipolygon(void) {}
void test_wkt_collection(void) {}

