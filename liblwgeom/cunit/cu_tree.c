/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 *
 * Copyright 2011 Sandro Santilli <strk@keybit.net>
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
#include "lwgeodetic.h";
#include "lwgeodetic_tree.h";
#include "cu_tester.h"


static void test_tree_circ_create(void)
{
	LWLINE *g;
	CIRC_NODE *c;
	g = lwgeom_as_lwline(lwgeom_from_wkt("LINESTRING(0 88,0 89,0 90,180 89,180 88)", LW_PARSER_CHECK_NONE));
	c = circ_tree_new(g->points);
//	circ_tree_print(c, 0);
	CU_ASSERT(c->num_nodes == 2);
	circ_tree_free(c);
	lwline_free(g);
}


static void test_tree_circ_pip(void)
{
	LWLINE *g;
	CIRC_NODE *c;
	POINT2D pt, pt_outside;
	int rv, on_boundary;
	
	pt.x = 0.0;
	pt.y = 0.0;
	pt_outside.x = -2.0;
	pt_outside.y = 0.0;
	
	/* Point in square */
	g = lwgeom_as_lwline(lwgeom_from_wkt("LINESTRING(-1 -1,1 -1,1 1,-1 1,-1 -1)", LW_PARSER_CHECK_NONE));
	c = circ_tree_new(g->points);
	rv = circ_tree_contains_point(c, &pt, &pt_outside, &on_boundary);
	CU_ASSERT_EQUAL(rv, 1);

	/* Point on other side of square */
	pt.x = 2.0;
	pt.y = 0.0;
	rv = circ_tree_contains_point(c, &pt, &pt_outside, &on_boundary);
	CU_ASSERT_EQUAL(rv, 2);

	/* Clean and do new shape */
	circ_tree_free(c);
	lwline_free(g);

	/* Point in square, stab passing through vertex */
	pt.x = 0.0;
	pt.y = 0.0;
	g = lwgeom_as_lwline(lwgeom_from_wkt("LINESTRING(-1 -1,0 -1,1 -1,1 0,1 1,0 1,-1 1,-1 0,-1 -1)", LW_PARSER_CHECK_NONE));
	c = circ_tree_new(g->points);
	//circ_tree_print(c, 0);
	rv = circ_tree_contains_point(c, &pt, &pt_outside, &on_boundary);
	CU_ASSERT_EQUAL(rv, 1);

	/* Point on other side of square, stab passing through vertex */
	pt.x = 2.0;
	pt.y = 0.0;
	rv = circ_tree_contains_point(c, &pt, &pt_outside, &on_boundary);
	CU_ASSERT_EQUAL(rv, 2);

	/* Clean and do new shape */
	circ_tree_free(c);
	lwline_free(g);

	/* Point outside "w" thing, stab passing through vertexes and grazing pointy thing */
	pt.x = 2.0;
	pt.y = 0.0;
	g = lwgeom_as_lwline(lwgeom_from_wkt("LINESTRING(-1 -1,0 -1,1 -1,1 0,1 1,0 0,-1 1,-1 0,-1 -1)", LW_PARSER_CHECK_NONE));
	c = circ_tree_new(g->points);
	//circ_tree_print(c, 0);
	rv = circ_tree_contains_point(c, &pt, &pt_outside, &on_boundary);
	//printf("rv %d\n", rv);
	CU_ASSERT_EQUAL(rv, 2);

	/* Point inside "w" thing, stab passing through vertexes and grazing pointy thing */
	pt.x = 0.8;
	pt.y = 0.0;
	rv = circ_tree_contains_point(c, &pt, &pt_outside, &on_boundary);
	//printf("rv %d\n", rv);
	CU_ASSERT_EQUAL(rv, 1);

	/* Clean and do new shape */
	circ_tree_free(c);
	lwline_free(g);
}

static void test_tree_circ_distance(void)
{
	LWLINE *line;
	LWPOINT *point;
	CIRC_NODE *cline, *cpoint;
	SPHEROID s;
	double distance_tree, distance_geom;
	double mind = MAXFLOAT, maxd = MAXFLOAT;
	double threshold = 0.0;
	
	spheroid_init(&s, 1.0, 1.0);

	line = lwgeom_as_lwline(lwgeom_from_wkt("LINESTRING(-1 -1,0 -1,1 -1,1 0,1 1,0 0,-1 1,-1 0,-1 -1)", LW_PARSER_CHECK_NONE));
	point = lwgeom_as_lwpoint(lwgeom_from_wkt("POINT(-2 0)", LW_PARSER_CHECK_NONE));
	cline = circ_tree_new(line->points);
	cpoint = circ_tree_new(point->point);
	distance_tree = circ_tree_distance_tree(cpoint, cline, threshold, &mind, &maxd);
	distance_geom = lwgeom_distance_spheroid((LWGEOM*)line, (LWGEOM*)point, &s, threshold);
	circ_tree_free(cline);
	circ_tree_free(cpoint);
	lwline_free(line);
	lwpoint_free(point);
	CU_ASSERT_DOUBLE_EQUAL(distance_geom, distance_geom, 0.0001);

	line = lwgeom_as_lwline(lwgeom_from_wkt("LINESTRING(-1 -1,0 -1,1 -1,1 0,1 1,0 0,-1 1,-1 0,-1 -1)", LW_PARSER_CHECK_NONE));
	point = lwgeom_as_lwpoint(lwgeom_from_wkt("POINT(2 2)", LW_PARSER_CHECK_NONE));
	cline = circ_tree_new(line->points);
	cpoint = circ_tree_new(point->point);
	distance_tree = circ_tree_distance_tree(cpoint, cline, threshold, &mind, &maxd);
	distance_geom = lwgeom_distance_spheroid((LWGEOM*)line, (LWGEOM*)point, &s, threshold);
	circ_tree_free(cline);
	circ_tree_free(cpoint);
	lwline_free(line);
	lwpoint_free(point);
	CU_ASSERT_DOUBLE_EQUAL(distance_geom, distance_geom, 0.0001);

	line = lwgeom_as_lwline(lwgeom_from_wkt("LINESTRING(-1 -1,0 -1,1 -1,1 0,1 1,0 0,-1 1,-1 0,-1 -1)", LW_PARSER_CHECK_NONE));
	point = lwgeom_as_lwpoint(lwgeom_from_wkt("POINT(1 1)", LW_PARSER_CHECK_NONE));
	cline = circ_tree_new(line->points);
	cpoint = circ_tree_new(point->point);
	distance_tree = circ_tree_distance_tree(cpoint, cline, threshold, &mind, &maxd);
	distance_geom = lwgeom_distance_spheroid((LWGEOM*)line, (LWGEOM*)point, &s, threshold);
	circ_tree_free(cline);
	circ_tree_free(cpoint);
	lwline_free(line);
	lwpoint_free(point);
	CU_ASSERT_DOUBLE_EQUAL(distance_tree, distance_geom, 0.0001);

	line = lwgeom_as_lwline(lwgeom_from_wkt("LINESTRING(-1 -1,0 -1,1 -1,1 0,1 1,0 0,-1 1,-1 0,-1 -1)", LW_PARSER_CHECK_NONE));
	point = lwgeom_as_lwpoint(lwgeom_from_wkt("POINT(1 0.5)", LW_PARSER_CHECK_NONE));
	cline = circ_tree_new(line->points);
	cpoint = circ_tree_new(point->point);
	distance_tree = circ_tree_distance_tree(cpoint, cline, threshold, &mind, &maxd);
	distance_geom = lwgeom_distance_spheroid((LWGEOM*)line, (LWGEOM*)point, &s, threshold);
//	printf("distance_tree %g\n", distance_tree);
//	printf("distance_geom %g\n", distance_geom);
//	circ_tree_print(cline, 0);
//	circ_tree_print(cpoint, 0);
	circ_tree_free(cline);
	circ_tree_free(cpoint);
	lwline_free(line);
	lwpoint_free(point);
	CU_ASSERT_DOUBLE_EQUAL(distance_tree, distance_geom, 0.0001);
}

/*
** Used by test harness to register the tests in this file.
*/
CU_TestInfo tree_tests[] =
{
	PG_TEST(test_tree_circ_create),
	PG_TEST(test_tree_circ_pip),
	PG_TEST(test_tree_circ_distance),
	CU_TEST_INFO_NULL
};
CU_SuiteInfo tree_suite = {"Internal Spatial Trees",  NULL,  NULL, tree_tests};

