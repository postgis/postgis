/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2009 Olivier Courtin <olivier.courtin@oslandia.com>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "cu_homogenize.h"

/*
** Called from test harness to register the tests in this file.
*/
CU_pSuite register_homogenize_suite(void)
{
	CU_pSuite pSuite;
	pSuite = CU_add_suite("Homogenize Suite", init_homogenize_suite, clean_homogenize_suite);
	if (NULL == pSuite)
	{
		CU_cleanup_registry();
		return NULL;
	}

	if ( 	(NULL == CU_add_test(pSuite, "test_coll_point()", test_coll_point)) ||
	        (NULL == CU_add_test(pSuite, "test_coll_line()", test_coll_line))   ||
	        (NULL == CU_add_test(pSuite, "test_coll_poly()", test_coll_poly))   ||
	        (NULL == CU_add_test(pSuite, "test_coll_coll()", test_coll_coll))   ||
	        (NULL == CU_add_test(pSuite, "test_geom()", test_geom))
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
int init_homogenize_suite(void)
{
	return 0;
}

/*
** The suite cleanup function.
** Frees any global objects.
*/
int clean_homogenize_suite(void)
{
	return 0;
}


static do_geom_test(char * in, char * out)
{
	LWGEOM *g, *h;

	g = lwgeom_from_ewkt(in, PARSER_CHECK_NONE);
	h = lwgeom_homogenize(g);
	if (strcmp(lwgeom_to_ewkt(h, PARSER_CHECK_NONE), out))
		fprintf(stderr, "\nIn:   %s\nOut:  %s\nTheo: %s\n",
		        in, lwgeom_to_ewkt(h, PARSER_CHECK_NONE), out);
	CU_ASSERT_STRING_EQUAL(lwgeom_to_ewkt(h, PARSER_CHECK_NONE), out);
	lwfree(g);
	lwgeom_free(h);
}


static do_coll_test(char * in, char * out)
{
	LWGEOM *g, *h;

	g = lwgeom_from_ewkt(in, PARSER_CHECK_NONE);
	h = lwcollection_homogenize((LWCOLLECTION *) g);
	if (strcmp(lwgeom_to_ewkt(h, PARSER_CHECK_NONE), out))
		fprintf(stderr, "\nIn:   %s\nOut:  %s\nTheo: %s\n",
		        in, lwgeom_to_ewkt(h, PARSER_CHECK_NONE), out);
	CU_ASSERT_STRING_EQUAL(lwgeom_to_ewkt(h, PARSER_CHECK_NONE), out);
	lwfree(g);
	lwgeom_free(h);
}


void test_coll_point(void)
{
	do_coll_test("GEOMETRYCOLLECTION(POINT(1 2))",
	             "POINT(1 2)");

	do_coll_test("GEOMETRYCOLLECTION(POINT(1 2),POINT(3 4))",
	             "MULTIPOINT(1 2,3 4)");

	do_coll_test("GEOMETRYCOLLECTION(POINT(1 2),POINT(3 4),POINT(5 6))",
	             "MULTIPOINT(1 2,3 4,5 6)");

	do_coll_test("GEOMETRYCOLLECTION(MULTIPOINT(1 2,3 4),POINT(5 6))",
	             "MULTIPOINT(1 2,3 4,5 6)");

	do_coll_test("GEOMETRYCOLLECTION(POINT(1 2),MULTIPOINT(3 4,5 6))",
	             "MULTIPOINT(1 2,3 4,5 6)");

	do_coll_test("GEOMETRYCOLLECTION(MULTIPOINT(1 2,3 4),MULTIPOINT(5 6,7 8))",
	             "MULTIPOINT(1 2,3 4,5 6,7 8)");
}


void test_coll_line(void)
{
	do_coll_test("GEOMETRYCOLLECTION(LINESTRING(1 2,3 4))",
	             "LINESTRING(1 2,3 4)");

	do_coll_test("GEOMETRYCOLLECTION(LINESTRING(1 2,3 4),LINESTRING(5 6,7 8))",
	             "MULTILINESTRING((1 2,3 4),(5 6,7 8))");

	do_coll_test("GEOMETRYCOLLECTION(LINESTRING(1 2,3 4),LINESTRING(5 6,7 8),LINESTRING(9 10,11 12))",
	             "MULTILINESTRING((1 2,3 4),(5 6,7 8),(9 10,11 12))");

	do_coll_test("GEOMETRYCOLLECTION(MULTILINESTRING((1 2,3 4),(5 6,7 8)),LINESTRING(9 10,11 12))",
	             "MULTILINESTRING((1 2,3 4),(5 6,7 8),(9 10,11 12))");

	do_coll_test("GEOMETRYCOLLECTION(LINESTRING(1 2,3 4),MULTILINESTRING((5 6,7 8),(9 10,11 12)))",
	             "MULTILINESTRING((1 2,3 4),(5 6,7 8),(9 10,11 12))");

	do_coll_test("GEOMETRYCOLLECTION(MULTILINESTRING((1 2,3 4),(5 6,7 8)),MULTILINESTRING((9 10,11 12),(13 14,15 16)))",
	             "MULTILINESTRING((1 2,3 4),(5 6,7 8),(9 10,11 12),(13 14,15 16))");
}


void test_coll_poly(void)
{
	do_coll_test("GEOMETRYCOLLECTION(POLYGON((1 2,3 4,5 6,1 2)))",
	             "POLYGON((1 2,3 4,5 6,1 2))");

	do_coll_test("GEOMETRYCOLLECTION(POLYGON((1 2,3 4,5 6,1 2)),POLYGON((7 8,9 10,11 12,7 8)))",
	             "MULTIPOLYGON(((1 2,3 4,5 6,1 2)),((7 8,9 10,11 12,7 8)))");

	do_coll_test("GEOMETRYCOLLECTION(POLYGON((1 2,3 4,5 6,1 2)),POLYGON((7 8,9 10,11 12,7 8)),POLYGON((13 14,15 16,17 18,13 14)))",
	             "MULTIPOLYGON(((1 2,3 4,5 6,1 2)),((7 8,9 10,11 12,7 8)),((13 14,15 16,17 18,13 14)))");

	do_coll_test("GEOMETRYCOLLECTION(MULTIPOLYGON(((1 2,3 4,5 6,1 2)),((7 8,9 10,11 12,7 8))),POLYGON((13 14,15 16,17 18,13 14)))",
	             "MULTIPOLYGON(((1 2,3 4,5 6,1 2)),((7 8,9 10,11 12,7 8)),((13 14,15 16,17 18,13 14)))");

	do_coll_test("GEOMETRYCOLLECTION(POLYGON((1 2,3 4,5 6,1 2)),MULTIPOLYGON(((7 8,9 10,11 12,7 8)),((13 14,15 16,17 18,13 14))))",
	             "MULTIPOLYGON(((1 2,3 4,5 6,1 2)),((7 8,9 10,11 12,7 8)),((13 14,15 16,17 18,13 14)))");

	do_coll_test("GEOMETRYCOLLECTION(MULTIPOLYGON(((1 2,3 4,5 6,1 2)),((7 8,9 10,11 12,7 8))),MULTIPOLYGON(((13 14,15 16,17 18,13 14)),((19 20,21 22,23 24,19 20))))",
	             "MULTIPOLYGON(((1 2,3 4,5 6,1 2)),((7 8,9 10,11 12,7 8)),((13 14,15 16,17 18,13 14)),((19 20,21 22,23 24,19 20)))");
}


void test_coll_coll(void)
{
	/* Two different types together must produce a Collection as output */
	do_coll_test("GEOMETRYCOLLECTION(POINT(1 2),LINESTRING(3 4,5 6))",
	             "GEOMETRYCOLLECTION(POINT(1 2),LINESTRING(3 4,5 6))");

	do_coll_test("GEOMETRYCOLLECTION(LINESTRING(1 2,3 4),POLYGON((5 6,7 8,9 10,5 6)))",
	             "GEOMETRYCOLLECTION(LINESTRING(1 2,3 4),POLYGON((5 6,7 8,9 10,5 6)))");


	/* Ability to produce a single MULTI with same type */
	do_coll_test("GEOMETRYCOLLECTION(POINT(1 2),LINESTRING(3 4,5 6),POINT(7 8))",
	             "GEOMETRYCOLLECTION(MULTIPOINT(1 2,7 8),LINESTRING(3 4,5 6))");

	do_coll_test("GEOMETRYCOLLECTION(POINT(1 2),LINESTRING(3 4,5 6),MULTIPOINT(7 8,9 10))",
	             "GEOMETRYCOLLECTION(MULTIPOINT(1 2,7 8,9 10),LINESTRING(3 4,5 6))");


	/* Recursive Collection handle */
	do_coll_test("GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(POINT(1 2))))",
	             "POINT(1 2)");

	do_coll_test("GEOMETRYCOLLECTION(POINT(1 2),GEOMETRYCOLLECTION(LINESTRING(3 4,5 6)))",
	             "GEOMETRYCOLLECTION(POINT(1 2),LINESTRING(3 4,5 6))");


	/* EMPTY Collection */
	do_coll_test("GEOMETRYCOLLECTION EMPTY",
	             "GEOMETRYCOLLECTION EMPTY");


	/* Recursive EMPTY Collection */
	do_coll_test("GEOMETRYCOLLECTION(GEOMETRYCOLLECTION EMPTY)",
	             "GEOMETRYCOLLECTION EMPTY");

	do_coll_test("GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(GEOMETRYCOLLECTION EMPTY))",
	             "GEOMETRYCOLLECTION EMPTY");
}


void test_geom(void)
{
	/* Already simple geometry */
	do_geom_test("POINT(1 2)",
	             "POINT(1 2)");

	do_geom_test("LINESTRING(1 2,3 4)",
	             "LINESTRING(1 2,3 4)");

	do_geom_test("POLYGON((1 2,3 4,5 6,1 2))",
	             "POLYGON((1 2,3 4,5 6,1 2))");

	do_geom_test("POLYGON((1 2,3 4,5 6,1 2),(7 8,9 10,11 12,7 8))",
	             "POLYGON((1 2,3 4,5 6,1 2),(7 8,9 10,11 12,7 8))");


	/* Empty geometry */
	do_geom_test("GEOMETRYCOLLECTION EMPTY",
	             "GEOMETRYCOLLECTION EMPTY");


	/* A MULTI with a single geometry inside */
	do_geom_test("MULTIPOINT(1 2)",
	             "POINT(1 2)");

	do_geom_test("MULTILINESTRING((1 2,3 4))",
	             "LINESTRING(1 2,3 4)");

	do_geom_test("MULTIPOLYGON(((1 2,3 4,5 6,1 2)))",
	             "POLYGON((1 2,3 4,5 6,1 2))");

	do_geom_test("MULTIPOLYGON(((1 2,3 4,5 6,1 2),(7 8,9 10,11 12,7 8)))",
	             "POLYGON((1 2,3 4,5 6,1 2),(7 8,9 10,11 12,7 8))");


	/* A real MULTI */
	do_geom_test("MULTIPOINT(1 2,3 4)",
	             "MULTIPOINT(1 2,3 4)");

	do_geom_test("MULTILINESTRING((1 2,3 4),(5 6,7 8))",
	             "MULTILINESTRING((1 2,3 4),(5 6,7 8))");

	do_geom_test("MULTIPOLYGON(((1 2,3 4,5 6,1 2)),((7 8,9 10,11 12,7 8)))",
	             "MULTIPOLYGON(((1 2,3 4,5 6,1 2)),((7 8,9 10,11 12,7 8)))");

	do_geom_test("MULTIPOLYGON(((1 2,3 4,5 6,1 2),(7 8,9 10,11 12,7 8)),((13 14,15 16,17 18,13 14)))",
	             "MULTIPOLYGON(((1 2,3 4,5 6,1 2),(7 8,9 10,11 12,7 8)),((13 14,15 16,17 18,13 14)))");


	/* A Collection */
	do_geom_test("GEOMETRYCOLLECTION(POINT(1 2),LINESTRING(3 4,5 6))",
	             "GEOMETRYCOLLECTION(POINT(1 2),LINESTRING(3 4,5 6))");


	/* SRID */
	do_geom_test("SRID=4326;GEOMETRYCOLLECTION EMPTY",
	             "SRID=4326;GEOMETRYCOLLECTION EMPTY");

	do_geom_test("SRID=4326;POINT(1 2)",
	             "SRID=4326;POINT(1 2)");

	do_geom_test("SRID=4326;MULTIPOINT(1 2)",
	             "SRID=4326;POINT(1 2)");

	do_geom_test("SRID=4326;MULTIPOINT(1 2,3 4)",
	             "SRID=4326;MULTIPOINT(1 2,3 4)");

	do_geom_test("SRID=4326;MULTILINESTRING((1 2,3 4))",
	             "SRID=4326;LINESTRING(1 2,3 4)");

	do_geom_test("SRID=4326;MULTILINESTRING((1 2,3 4),(5 6,7 8))",
	             "SRID=4326;MULTILINESTRING((1 2,3 4),(5 6,7 8))");

	/* 3D and 4D */
	do_geom_test("POINT(1 2 3)",
	             "POINT(1 2 3)");

	do_geom_test("POINTM(1 2 3)",
	             "POINTM(1 2 3)");

	do_geom_test("POINT(1 2 3 4)",
	             "POINT(1 2 3 4)");
}
