/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "cu_measures.h"

/*
** Called from test harness to register the tests in this file.
*/
CU_pSuite register_measures_suite(void)
{
	CU_pSuite pSuite;
	pSuite = CU_add_suite("PostGIS Measures Suite", init_measures_suite, clean_measures_suite);
	if (NULL == pSuite)
	{
		CU_cleanup_registry();
		return NULL;
	}

	if (
	    (NULL == CU_add_test(pSuite, "test_mindistance2d_tolerance()", test_mindistance2d_tolerance)) ||
	    (NULL == CU_add_test(pSuite, "test_rect_tree_contains_point()", test_rect_tree_contains_point)) ||
	    (NULL == CU_add_test(pSuite, "test_rect_tree_intersects_tree()", test_rect_tree_intersects_tree))
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
int init_measures_suite(void)
{
	return 0;
}

/*
** The suite cleanup function.
** Frees any global objects.
*/
int clean_measures_suite(void)
{
	return 0;
}

void test_mindistance2d_tolerance(void)
{
	LWGEOM_PARSER_RESULT gp1;
	LWGEOM_PARSER_RESULT gp2;
	double distance;
	int result1, result2;

	/*
	** Simple case.
	*/
	result1 = serialized_lwgeom_from_ewkt(&gp1, "POINT(0 0)", PARSER_CHECK_NONE);
	result2 = serialized_lwgeom_from_ewkt(&gp2, "MULTIPOINT(0 1.5,0 2,0 2.5)", PARSER_CHECK_NONE);
	distance = lwgeom_mindistance2d_tolerance(gp1.serialized_lwgeom, gp2.serialized_lwgeom, 0.0);
	//printf("\ndistance #1 = %g\n",distance);
	CU_ASSERT_EQUAL(distance, 1.5);
	free(gp1.serialized_lwgeom);
	free(gp2.serialized_lwgeom);

	/*
	** Point vs Geometry Collection.
	*/
	result1 = serialized_lwgeom_from_ewkt(&gp1, "POINT(0 0)", PARSER_CHECK_NONE);
	result2 = serialized_lwgeom_from_ewkt(&gp2, "GEOMETRYCOLLECTION(POINT(3 4))", PARSER_CHECK_NONE);
	distance = lwgeom_mindistance2d_tolerance(gp1.serialized_lwgeom, gp2.serialized_lwgeom, 0.0);
	//printf("distance #2 = %g\n",distance);
	CU_ASSERT_EQUAL(distance, 5.0);
	free(gp1.serialized_lwgeom);
	free(gp2.serialized_lwgeom);

	/*
	** Point vs Geometry Collection Collection.
	*/
	result1 = serialized_lwgeom_from_ewkt(&gp1, "POINT(0 0)", PARSER_CHECK_NONE);
	result2 = serialized_lwgeom_from_ewkt(&gp2, "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(POINT(3 4)))", PARSER_CHECK_NONE);
	distance = lwgeom_mindistance2d_tolerance(gp1.serialized_lwgeom, gp2.serialized_lwgeom, 0.0);
	//printf("distance #3 = %g\n",distance);
	CU_ASSERT_EQUAL(distance, 5.0);
	free(gp1.serialized_lwgeom);
	free(gp2.serialized_lwgeom);

	/*
	** Point vs Geometry Collection Collection Collection.
	*/
	result1 = serialized_lwgeom_from_ewkt(&gp1, "POINT(0 0)", PARSER_CHECK_NONE);
	result2 = serialized_lwgeom_from_ewkt(&gp2, "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(POINT(3 4))))", PARSER_CHECK_NONE);
	distance = lwgeom_mindistance2d_tolerance(gp1.serialized_lwgeom, gp2.serialized_lwgeom, 0.0);
	//printf("distance #4 = %g\n",distance);
	CU_ASSERT_EQUAL(distance, 5.0);
	free(gp1.serialized_lwgeom);
	free(gp2.serialized_lwgeom);

	/*
	** Point vs Geometry Collection Collection Collection Multipoint.
	*/
	result1 = serialized_lwgeom_from_ewkt(&gp1, "POINT(0 0)", PARSER_CHECK_NONE);
	result2 = serialized_lwgeom_from_ewkt(&gp2, "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(MULTIPOINT(3 4))))", PARSER_CHECK_NONE);
	distance = lwgeom_mindistance2d_tolerance(gp1.serialized_lwgeom, gp2.serialized_lwgeom, 0.0);
	//printf("distance #5 = %g\n",distance);
	CU_ASSERT_EQUAL(distance, 5.0);
	free(gp1.serialized_lwgeom);
	free(gp2.serialized_lwgeom);

	/*
	** Geometry Collection vs Geometry Collection
	*/
	result1 = serialized_lwgeom_from_ewkt(&gp1, "GEOMETRYCOLLECTION(POINT(0 0))", PARSER_CHECK_NONE);
	result2 = serialized_lwgeom_from_ewkt(&gp2, "GEOMETRYCOLLECTION(POINT(3 4))", PARSER_CHECK_NONE);
	distance = lwgeom_mindistance2d_tolerance(gp1.serialized_lwgeom, gp2.serialized_lwgeom, 0.0);
	//printf("distance #6 = %g\n",distance);
	CU_ASSERT_EQUAL(distance, 5.0);
	free(gp1.serialized_lwgeom);
	free(gp2.serialized_lwgeom);

	/*
	** Geometry Collection Collection vs Geometry Collection Collection
	*/
	result1 = serialized_lwgeom_from_ewkt(&gp1, "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(POINT(0 0)))", PARSER_CHECK_NONE);
	result2 = serialized_lwgeom_from_ewkt(&gp2, "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(POINT(3 4)))", PARSER_CHECK_NONE);
	distance = lwgeom_mindistance2d_tolerance(gp1.serialized_lwgeom, gp2.serialized_lwgeom, 0.0);
	//printf("distance #7 = %g\n",distance);
	CU_ASSERT_EQUAL(distance, 5.0);
	free(gp1.serialized_lwgeom);
	free(gp2.serialized_lwgeom);

	/*
	** Geometry Collection Collection Multipoint vs Geometry Collection Collection Multipoint
	*/
	result1 = serialized_lwgeom_from_ewkt(&gp1, "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(MULTIPOINT(0 0)))", PARSER_CHECK_NONE);
	result2 = serialized_lwgeom_from_ewkt(&gp2, "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(MULTIPOINT(3 4)))", PARSER_CHECK_NONE);
	distance = lwgeom_mindistance2d_tolerance(gp1.serialized_lwgeom, gp2.serialized_lwgeom, 0.0);
	//printf("distance #8 = %g\n",distance);
	CU_ASSERT_EQUAL(distance, 5.0);
	free(gp1.serialized_lwgeom);
	free(gp2.serialized_lwgeom);

}

void test_rect_tree_contains_point(void)
{
	LWPOLY *poly;
	POINT2D p;
	RECT_NODE* tree;
	int result;
	int boundary = 0;
	
	/* square */
	poly = (LWPOLY*)lwgeom_from_ewkt("POLYGON((0 0, 0 1, 1 1, 1 0, 0 0))", PARSER_CHECK_NONE);
	tree = rect_tree_new(poly->rings[0]);

	/* inside square */
	boundary = 0;
	p.x = 0.5;
	p.y = 0.5;
	result = rect_tree_contains_point(tree, &p, &boundary);
	CU_ASSERT_NOT_EQUAL(result, 0);

	/* outside square */
	boundary = 0;
	p.x = 1.5;
	p.y = 0.5;
	result = rect_tree_contains_point(tree, &p, &boundary);
	CU_ASSERT_EQUAL(result, 0);

	rect_tree_free(tree);
	lwpoly_free(poly);

	/* ziggy zaggy horizontal saw tooth polygon */
	poly = (LWPOLY*)lwgeom_from_ewkt("POLYGON((0 0, 1 3, 2 0, 3 3, 4 0, 4 5, 0 5, 0 0))", PARSER_CHECK_NONE);
	tree = rect_tree_new(poly->rings[0]);

	/* not in, left side */
	boundary = 0;
	p.x = -0.5;
	p.y = 0.5;
	result = rect_tree_contains_point(tree, &p, &boundary);
	CU_ASSERT_EQUAL(result, 0);

	/* not in, right side */
	boundary = 0;
	p.x = 3.0;
	p.y = 1.0;
	result = rect_tree_contains_point(tree, &p, &boundary);
	CU_ASSERT_EQUAL(result, 0);

	/* inside */
	boundary = 0;
	p.x = 2.0;
	p.y = 1.0;
	result = rect_tree_contains_point(tree, &p, &boundary);
	CU_ASSERT_NOT_EQUAL(result, 0);

	/* on left border */
	boundary = 0;
	p.x = 0.0;
	p.y = 1.0;
	result = rect_tree_contains_point(tree, &p, &boundary);
	CU_ASSERT_EQUAL(boundary, 1);
	
	/* on right border */
	boundary = 0;
	p.x = 4.0;
	p.y = 0.0;
	result = rect_tree_contains_point(tree, &p, &boundary);
	CU_ASSERT_EQUAL(boundary, 1);
	
	/* on tooth concave */
	boundary = 0;
	p.x = 3.0;
	p.y = 3.0;
	result = rect_tree_contains_point(tree, &p, &boundary);
	CU_ASSERT_EQUAL(boundary, 1);

	/* on tooth convex */
	boundary = 0;
	p.x = 2.0;
	p.y = 0.0;
	result = rect_tree_contains_point(tree, &p, &boundary);
	CU_ASSERT_EQUAL(boundary, 1);

	rect_tree_free(tree);
	lwpoly_free(poly);

	/* ziggy zaggy vertical saw tooth polygon */
	poly = (LWPOLY*)lwgeom_from_ewkt("POLYGON((0 0, 3 1, 0 2, 3 3, 0 4, 3 5, 0 6, 5 6, 5 0, 0 0))", PARSER_CHECK_NONE);
	tree = rect_tree_new(poly->rings[0]);

	/* not in, left side */
	boundary = 0;
	p.x = -0.5;
	p.y = 3.5;
	result = rect_tree_contains_point(tree, &p, &boundary);
	CU_ASSERT_EQUAL(result, 0);

	/* not in, right side */
	boundary = 0;
	p.x = 6.0;
	p.y = 2.2;
	result = rect_tree_contains_point(tree, &p, &boundary);
	CU_ASSERT_EQUAL(result, 0);

	/* inside */
	boundary = 0;
	p.x = 3.0;
	p.y = 2.0;
	result = rect_tree_contains_point(tree, &p, &boundary);
	CU_ASSERT_NOT_EQUAL(result, 0);

	/* on bottom border */
	boundary = 0;
	p.x = 1.0;
	p.y = 0.0;
	result = rect_tree_contains_point(tree, &p, &boundary);
	CU_ASSERT_EQUAL(boundary, 1);
	
	/* on top border */
	boundary = 0;
	p.x = 3.0;
	p.y = 6.0;
	result = rect_tree_contains_point(tree, &p, &boundary);
	CU_ASSERT_EQUAL(boundary, 1);
	
	/* on tooth concave */
	boundary = 0;
	p.x = 3.0;
	p.y = 1.0;
	result = rect_tree_contains_point(tree, &p, &boundary);
	CU_ASSERT_EQUAL(boundary, 1);

	/* on tooth convex */
	boundary = 0;
	p.x = 0.0;
	p.y = 2.0;
	result = rect_tree_contains_point(tree, &p, &boundary);
	CU_ASSERT_EQUAL(boundary, 1);

	/* on tooth convex */
	boundary = 0;
	p.x = 0.0;
	p.y = 6.0;
	result = rect_tree_contains_point(tree, &p, &boundary);
	CU_ASSERT_EQUAL(boundary, 1);

	rect_tree_free(tree);
	lwpoly_free(poly);

}

void test_rect_tree_intersects_tree(void)
{
	LWPOLY *poly1, *poly2;
	RECT_NODE *tree1, *tree2;
	int result;

	poly1 = (LWPOLY*)lwgeom_from_ewkt("POLYGON((0 0, 3 1, 0 2, 3 3, 0 4, 3 5, 0 6, 5 6, 5 0, 0 0))", PARSER_CHECK_NONE);
	poly2 = (LWPOLY*)lwgeom_from_ewkt("POLYGON((0 0, 3 1, 0 2, 3 3, 0 4, 3 5, 0 6, 5 6, 5 0, 0 0))", PARSER_CHECK_NONE);
	tree1 = rect_tree_new(poly1->rings[0]);
	tree2 = rect_tree_new(poly2->rings[0]);
	result = rect_tree_intersects_tree(tree1, tree2);
	CU_ASSERT_EQUAL(result, LW_TRUE);
	lwpoly_free(poly1);
	lwpoly_free(poly2);
	rect_tree_free(tree1);
	rect_tree_free(tree2);
	
	poly1 = (LWPOLY*)lwgeom_from_ewkt("POLYGON((0 0, 3 1, 0 2, 3 3, 0 4, 3 5, 0 6, 5 6, 5 0, 0 0))", PARSER_CHECK_NONE);
	poly2 = (LWPOLY*)lwgeom_from_ewkt("POLYGON((0.3 0.7, 0.3 0.8, 0.4 0.8, 0.4 0.7, 0.3 0.7))", PARSER_CHECK_NONE);
	tree1 = rect_tree_new(poly1->rings[0]);
	tree2 = rect_tree_new(poly2->rings[0]);
	result = rect_tree_intersects_tree(tree1, tree2);
	CU_ASSERT_EQUAL(result, LW_FALSE);
	lwpoly_free(poly1);
	lwpoly_free(poly2);
	rect_tree_free(tree1);
	rect_tree_free(tree2);

	poly1 = (LWPOLY*)lwgeom_from_ewkt("POLYGON((0 0, 3 1, 0 2, 3 3, 0 4, 3 5, 0 6, 5 6, 5 0, 0 0))", PARSER_CHECK_NONE);
	poly2 = (LWPOLY*)lwgeom_from_ewkt("POLYGON((0.3 0.7, 0.3 0.8, 0.4 0.8, 1.3 0.3, 0.3 0.7))", PARSER_CHECK_NONE);
	tree1 = rect_tree_new(poly1->rings[0]);
	tree2 = rect_tree_new(poly2->rings[0]);
	result = rect_tree_intersects_tree(tree1, tree2);
	CU_ASSERT_EQUAL(result, LW_TRUE);
	lwpoly_free(poly1);
	lwpoly_free(poly2);
	rect_tree_free(tree1);
	rect_tree_free(tree2);
/*	
	poly1 = (LWPOLY*)lwgeom_from_ewkt("POLYGON((0 0, 3 1, 0 2, 3 3, 0 4, 3 5, 0 6, 5 6, 5 0, 0 0))", PARSER_CHECK_NONE);
	poly2 = (LWPOLY*)lwgeom_from_ewkt("POLYGON((-1 5, 0 5, 0 7, -1 7, -1 5))", PARSER_CHECK_NONE);
	tree1 = rect_tree_new(poly1->rings[0]);
	tree2 = rect_tree_new(poly2->rings[0]);
	result = rect_tree_intersects_tree(tree1, tree2);
	CU_ASSERT_EQUAL(result, LW_TRUE);
	lwpoly_free(poly1);
	lwpoly_free(poly2);
	rect_tree_free(tree1);
	rect_tree_free(tree2);
*/

}

