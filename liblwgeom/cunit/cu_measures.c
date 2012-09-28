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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CUnit/Basic.h"

#include "liblwgeom_internal.h"
#include "cu_tester.h"
#include "measures.h"
#include "lwtree.h"

static void do_test_mindistance2d_tolerance(char *in1, char *in2, double expected_res)
{
	LWGEOM *lw1;
	LWGEOM *lw2;
	double distance;

	lw1 = lwgeom_from_wkt(in1, LW_PARSER_CHECK_NONE);
	lw2 = lwgeom_from_wkt(in2, LW_PARSER_CHECK_NONE);

	distance = lwgeom_mindistance2d_tolerance(lw1, lw2, 0.0);
	CU_ASSERT_EQUAL(distance, expected_res);
	lwgeom_free(lw1);
	lwgeom_free(lw2);

}
static void test_mindistance2d_tolerance(void)
{
	/*
	** Simple case.
	*/
	do_test_mindistance2d_tolerance("POINT(0 0)", "MULTIPOINT(0 1.5,0 2,0 2.5)", 1.5);

	/*
	** Point vs Geometry Collection.
	*/
	do_test_mindistance2d_tolerance("POINT(0 0)", "GEOMETRYCOLLECTION(POINT(3 4))", 5.0);

	/*
	** Point vs Geometry Collection Collection.
	*/
	do_test_mindistance2d_tolerance("POINT(0 0)", "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(POINT(3 4)))", 5.0);

	/*
	** Point vs Geometry Collection Collection Collection.
	*/
	do_test_mindistance2d_tolerance("POINT(0 0)", "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(POINT(3 4))))", 5.0);

	/*
	** Point vs Geometry Collection Collection Collection Multipoint.
	*/
	do_test_mindistance2d_tolerance("POINT(0 0)", "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(MULTIPOINT(3 4))))", 5.0);

	/*
	** Geometry Collection vs Geometry Collection
	*/
	do_test_mindistance2d_tolerance("GEOMETRYCOLLECTION(POINT(0 0))", "GEOMETRYCOLLECTION(POINT(3 4))", 5.0);

	/*
	** Geometry Collection Collection vs Geometry Collection Collection
	*/
	do_test_mindistance2d_tolerance("GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(POINT(0 0)))", "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(POINT(3 4)))", 5.0);

	/*
	** Geometry Collection Collection Multipoint vs Geometry Collection Collection Multipoint
	*/
	do_test_mindistance2d_tolerance("GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(MULTIPOINT(0 0)))", "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION(MULTIPOINT(3 4)))", 5.0);

	/*
	** Linestring vs its start point 
	*/
	do_test_mindistance2d_tolerance("LINESTRING(-2 0, -0.2 0)", "POINT(-2 0)", 0);

	/*
	** Linestring vs its end point 
	*/
	do_test_mindistance2d_tolerance("LINESTRING(-0.2 0, -2 0)", "POINT(-2 0)", 0);

	/*
	** Linestring vs its start point (tricky number, see #1459)
	*/
	do_test_mindistance2d_tolerance("LINESTRING(-1e-8 0, -0.2 0)", "POINT(-1e-8 0)", 0);

	/*
	** Linestring vs its end point (tricky number, see #1459)
	*/
	do_test_mindistance2d_tolerance("LINESTRING(-0.2 0, -1e-8 0)", "POINT(-1e-8 0)", 0);

}

static void test_rect_tree_contains_point(void)
{
	LWPOLY *poly;
	POINT2D p;
	RECT_NODE* tree;
	int result;
	int boundary = 0;

	/* square */
	poly = (LWPOLY*)lwgeom_from_wkt("POLYGON((0 0, 0 1, 1 1, 1 0, 0 0))", LW_PARSER_CHECK_NONE);
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
	poly = (LWPOLY*)lwgeom_from_wkt("POLYGON((0 0, 1 3, 2 0, 3 3, 4 0, 4 5, 0 5, 0 0))", LW_PARSER_CHECK_NONE);
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
	poly = (LWPOLY*)lwgeom_from_wkt("POLYGON((0 0, 3 1, 0 2, 3 3, 0 4, 3 5, 0 6, 5 6, 5 0, 0 0))", LW_PARSER_CHECK_NONE);
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

static void test_rect_tree_intersects_tree(void)
{
	LWPOLY *poly1, *poly2;
	RECT_NODE *tree1, *tree2;
	int result;

	/* total overlap, A == B */
	poly1 = (LWPOLY*)lwgeom_from_wkt("POLYGON((0 0, 3 1, 0 2, 3 3, 0 4, 3 5, 0 6, 5 6, 5 0, 0 0))", LW_PARSER_CHECK_NONE);
	poly2 = (LWPOLY*)lwgeom_from_wkt("POLYGON((0 0, 3 1, 0 2, 3 3, 0 4, 3 5, 0 6, 5 6, 5 0, 0 0))", LW_PARSER_CHECK_NONE);
	tree1 = rect_tree_new(poly1->rings[0]);
	tree2 = rect_tree_new(poly2->rings[0]);
	result = rect_tree_intersects_tree(tree1, tree2);
	CU_ASSERT_EQUAL(result, LW_TRUE);
	lwpoly_free(poly1);
	lwpoly_free(poly2);
	rect_tree_free(tree1);
	rect_tree_free(tree2);

	/* hiding between the tines of the comb */
	poly1 = (LWPOLY*)lwgeom_from_wkt("POLYGON((0 0, 3 1, 0 2, 3 3, 0 4, 3 5, 0 6, 5 6, 5 0, 0 0))", LW_PARSER_CHECK_NONE);
	poly2 = (LWPOLY*)lwgeom_from_wkt("POLYGON((0.3 0.7, 0.3 0.8, 0.4 0.8, 0.4 0.7, 0.3 0.7))", LW_PARSER_CHECK_NONE);
	tree1 = rect_tree_new(poly1->rings[0]);
	tree2 = rect_tree_new(poly2->rings[0]);
	result = rect_tree_intersects_tree(tree1, tree2);
	CU_ASSERT_EQUAL(result, LW_FALSE);
	lwpoly_free(poly1);
	lwpoly_free(poly2);
	rect_tree_free(tree1);
	rect_tree_free(tree2);

	/* between the tines, but with a corner overlapping */
	poly1 = (LWPOLY*)lwgeom_from_wkt("POLYGON((0 0, 3 1, 0 2, 3 3, 0 4, 3 5, 0 6, 5 6, 5 0, 0 0))", LW_PARSER_CHECK_NONE);
	poly2 = (LWPOLY*)lwgeom_from_wkt("POLYGON((0.3 0.7, 0.3 0.8, 0.4 0.8, 1.3 0.3, 0.3 0.7))", LW_PARSER_CHECK_NONE);
	tree1 = rect_tree_new(poly1->rings[0]);
	tree2 = rect_tree_new(poly2->rings[0]);
	result = rect_tree_intersects_tree(tree1, tree2);
	CU_ASSERT_EQUAL(result, LW_TRUE);
	lwpoly_free(poly1);
	lwpoly_free(poly2);
	rect_tree_free(tree1);
	rect_tree_free(tree2);

	/* Just touching the top left corner of the comb */
	poly1 = (LWPOLY*)lwgeom_from_wkt("POLYGON((0 0, 3 1, 0 2, 3 3, 0 4, 3 5, 0 6, 5 6, 5 0, 0 0))", LW_PARSER_CHECK_NONE);
	poly2 = (LWPOLY*)lwgeom_from_wkt("POLYGON((-1 5, 0 5, 0 7, -1 7, -1 5))", LW_PARSER_CHECK_NONE);
	tree1 = rect_tree_new(poly1->rings[0]);
	tree2 = rect_tree_new(poly2->rings[0]);
	result = rect_tree_intersects_tree(tree1, tree2);
	CU_ASSERT_EQUAL(result, LW_TRUE);
	lwpoly_free(poly1);
	lwpoly_free(poly2);
	rect_tree_free(tree1);
	rect_tree_free(tree2);


}

static void
test_lwgeom_segmentize2d(void)
{
	LWGEOM *linein = lwgeom_from_wkt("LINESTRING(0 0,10 0)", LW_PARSER_CHECK_NONE);
	LWGEOM *lineout = lwgeom_segmentize2d(linein, 5);
	char *strout = lwgeom_to_ewkt(lineout);
	CU_ASSERT_STRING_EQUAL(strout, "LINESTRING(0 0,5 0,10 0)");
	lwfree(strout);
	lwgeom_free(linein);
	lwgeom_free(lineout);
}

static void
test_lwgeom_locate_along(void)
{
	LWGEOM *geom = NULL;
	LWGEOM *out = NULL;
	double measure = 105.0;
	char *str;
	
	/* ST_Locatealong(ST_GeomFromText('MULTILINESTRING M ((1 2 3, 5 4 5), (50 50 1, 60 60 200))'), 105) */
	geom = lwgeom_from_wkt("MULTILINESTRING M ((1 2 3, 5 4 5), (50 50 1, 60 60 200))", LW_PARSER_CHECK_NONE);
	out = lwgeom_locate_along(geom, measure, 0.0);
	str = lwgeom_to_wkt(out, WKT_ISO, 8, NULL);
	lwgeom_free(geom);
	lwgeom_free(out);
	CU_ASSERT_STRING_EQUAL("MULTIPOINT M (55.226131 55.226131 105)", str);
	lwfree(str);
	
	/* ST_Locatealong(ST_GeomFromText('MULTILINESTRING M ((1 2 3, 5 4 5), (50 50 1, 60 60 200))'), 105) */
	geom = lwgeom_from_wkt("MULTILINESTRING M ((1 2 3, 3 4 2, 9 4 3), (1 2 3, 5 4 5), (50 50 1, 60 60 200))", LW_PARSER_CHECK_NONE);
	out = lwgeom_locate_along(geom, measure, 0.0);
	str = lwgeom_to_wkt(out, WKT_ISO, 8, NULL);
	lwgeom_free(geom);
	lwgeom_free(out);
	CU_ASSERT_STRING_EQUAL("MULTIPOINT M (55.226131 55.226131 105)", str);
	lwfree(str);
}

static void
test_lw_dist2d_pt_arc(void)
{
	/* int lw_dist2d_pt_arc(const POINT2D* P, const POINT2D* A1, const POINT2D* A2, const POINT2D* A3, DISTPTS* dl) */
	DISTPTS dl;
	POINT2D P, A1, A2, A3;
	int rv;

	
	/* Point within unit semicircle, 0.5 units from arc */
	A1.x = -1; A1.y = 0;
	A2.x = 0 ; A2.y = 1;
	A3.x = 1 ; A3.y = 0;
	P.x  = 0 ; P.y  = 0.5;	

	lw_dist2d_distpts_init(&dl, DIST_MIN);
	rv = lw_dist2d_pt_arc(&P, &A1, &A2, &A3, &dl);
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 0.5, 0.000001);

	/* Point outside unit semicircle, 0.5 units from arc */
	P.x  = 0 ; P.y  = 1.5;	
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	rv = lw_dist2d_pt_arc(&P, &A1, &A2, &A3, &dl);
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 0.5, 0.000001);

	/* Point outside unit semicircle, sqrt(2) units from arc end point*/
	P.x  = 0 ; P.y  = -1;	
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	rv = lw_dist2d_pt_arc(&P, &A1, &A2, &A3, &dl);
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, sqrt(2.0), 0.000001);

	/* Point outside unit semicircle, sqrt(2)-1 units from arc end point*/
	P.x  = 1 ; P.y  = 1;	
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	rv = lw_dist2d_pt_arc(&P, &A1, &A2, &A3, &dl);
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, sqrt(2.0)-1, 0.000001);

	/* Point on unit semicircle midpoint */
	P.x  = 0 ; P.y  = 1;	
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	rv = lw_dist2d_pt_arc(&P, &A1, &A2, &A3, &dl);
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 0, 0.000001);

	/* Point on unit semicircle endpoint */
	P.x  = 1 ; P.y  = 0;	
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	rv = lw_dist2d_pt_arc(&P, &A1, &A2, &A3, &dl);
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 0, 0.000001);

	/* Point inside closed circle */
	P.x  = 0 ; P.y  = 0.5;
	A2.x = 1; A2.y = 0;
	A3 = A1;
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	rv = lw_dist2d_pt_arc(&P, &A1, &A2, &A3, &dl);
	//printf("distance %g\n", dl.distance);
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 0.5, 0.000001);	
}

static void
test_lw_dist2d_seg_arc(void)
{
	/* int lw_dist2d_seg_arc(const POINT2D *A1, const POINT2D *A2, const POINT2D *B1, const POINT2D *B2, const POINT2D *B3, DISTPTS *dl) */

	DISTPTS dl;
	POINT2D A1, A2, B1, B2, B3;
	int rv;
	
	/* Unit semicircle */
	B1.x = -1; B1.y = 0;
	B2.x = 0 ; B2.y = 1;
	B3.x = 1 ; B3.y = 0;

	/* Edge above the unit semicircle */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = -2; A1.y = 2;
	A2.x = 2 ; A2.y = 2;
	rv = lw_dist2d_seg_arc(&A1, &A2, &B1, &B2, &B3, &dl);
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 1, 0.000001);

	/* Edge to the right of the unit semicircle */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = 2; A1.y = -2;
	A2.x = 2; A2.y = 2;
	rv = lw_dist2d_seg_arc(&A1, &A2, &B1, &B2, &B3, &dl);
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 1, 0.000001);

	/* Edge to the left of the unit semicircle */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = -2; A1.y = -2;
	A2.x = -2; A2.y = 2;
	rv = lw_dist2d_seg_arc(&A1, &A2, &B1, &B2, &B3, &dl);
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 1, 0.000001);

	/* Edge within the unit semicircle */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = 0; A1.y = 0;
	A2.x = 0; A2.y = 0.5;
	rv = lw_dist2d_seg_arc(&A1, &A2, &B1, &B2, &B3, &dl);
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 0.5, 0.000001);

	/* Edge grazing the unit semicircle */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = -2; A1.y = 1;
	A2.x =  2; A2.y = 1;
	rv = lw_dist2d_seg_arc(&A1, &A2, &B1, &B2, &B3, &dl);
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 0., 0.000001);

	/* Line grazing the unit semicircle, but edge not */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = 1; A1.y = 1;
	A2.x = 2; A2.y = 1;
	rv = lw_dist2d_seg_arc(&A1, &A2, &B1, &B2, &B3, &dl);
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, sqrt(2.0)-1, 0.000001);

	/* Edge intersecting the unit semicircle */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = 0; A1.y = 0;
	A2.x = 2; A2.y = 2;
	rv = lw_dist2d_seg_arc(&A1, &A2, &B1, &B2, &B3, &dl);
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 0, 0.000001);

	/* Line intersecting the unit semicircle, but edge not */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = -1; A1.y = 1;
	A2.x = -2; A2.y = 2;
	rv = lw_dist2d_seg_arc(&A1, &A2, &B1, &B2, &B3, &dl);
	//printf("distance %g\n", dl.distance);
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, sqrt(2.0)-1, 0.000001);
}

static void
test_lw_dist2d_arc_arc(void)
{
	/* lw_dist2d_arc_arc(const POINT2D *A1, const POINT2D *A2, const POINT2D *A3, 
	                     const POINT2D *B1, const POINT2D *B2, const POINT2D *B3,
	                     DISTPTS *dl) */
	DISTPTS dl;
	POINT2D A1, A2, A3, B1, B2, B3;
	int rv;
	
	/* Unit semicircle at 0,0 */
	B1.x = -1; B1.y = 0;
	B2.x = 0 ; B2.y = 1;
	B3.x = 1 ; B3.y = 0;

	/* Arc above the unit semicircle */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = -1; A1.y = 3;
	A2.x = 0 ; A2.y = 2;
	A3.x = 1 ; A3.y = 3;
	rv = lw_dist2d_arc_arc(&A1, &A2, &A3, &B1, &B2, &B3, &dl);
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 1, 0.000001);

	/* Arc grazes the unit semicircle */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = -1; A1.y = 2;
	A2.x = 0 ; A2.y = 1;
	A3.x = 1 ; A3.y = 2;
	rv = lw_dist2d_arc_arc(&A1, &A2, &A3, &B1, &B2, &B3, &dl);
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 0, 0.000001);

	/* Circles intersect, but arcs do not */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = -1; A1.y =  1;
	A2.x =  0; A2.y =  2;
	A3.x =  1; A3.y =  1;
	rv = lw_dist2d_arc_arc(&A1, &A2, &A3, &B1, &B2, &B3, &dl);
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, sqrt(2)-1, 0.000001);

	/* Circles and arcs intersect */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = -1; A1.y =  1;
	A2.x =  0; A2.y =  0;
	A3.x =  1; A3.y =  1;
	rv = lw_dist2d_arc_arc(&A1, &A2, &A3, &B1, &B2, &B3, &dl);
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 0, 0.000001);

	/* inscribed and closest on arcs */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = -0.5; A1.y = 0.0;
	A2.x =  0.0; A2.y = 0.5;
	A3.x =  0.5; A3.y = 0.0;
	rv = lw_dist2d_arc_arc(&A1, &A2, &A3, &B1, &B2, &B3, &dl);
	//printf("distance %g\n", dl.distance);
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 0.5, 0.000001);

	/* inscribed and closest not on arcs */
	lw_dist2d_distpts_init(&dl, DIST_MIN);
	A1.x = -0.5; A1.y =  0.0;
	A2.x =  0.0; A2.y = -0.5;
	A3.x =  0.5; A3.y =  0.0;
	rv = lw_dist2d_arc_arc(&A1, &A2, &A3, &B1, &B2, &B3, &dl);
	//printf("distance %g\n", dl.distance);
	CU_ASSERT_DOUBLE_EQUAL(dl.distance, 0.5, 0.000001);
}

static void
test_lw_arc_length(void)
{
/* double lw_arc_length(const POINT2D *A1, const POINT2D *A2, const POINT2D *A3) */

	POINT2D A1, A2, A3;
	double d;
	
	/* Unit semicircle at 0,0 */
	A1.x = -1; A1.y = 0;
	A2.x = 0 ; A2.y = 1;
	A3.x = 1 ; A3.y = 0;

	/* Arc above the unit semicircle */
	d = lw_arc_length(&A1, &A2, &A3);
	CU_ASSERT_DOUBLE_EQUAL(d, M_PI, 0.000001);
	d = lw_arc_length(&A3, &A2, &A1);
	CU_ASSERT_DOUBLE_EQUAL(d, M_PI, 0.000001);

	/* Unit semicircle at 0,0 */
	A1.x = 0; A1.y = 1;
	A2.x = 1; A2.y = 0;
	A3.x = 0; A3.y = -1;

	/* Arc to right of the unit semicircle */
	d = lw_arc_length(&A1, &A2, &A3);
	CU_ASSERT_DOUBLE_EQUAL(d, M_PI, 0.000001);
	d = lw_arc_length(&A3, &A2, &A1);
	CU_ASSERT_DOUBLE_EQUAL(d, M_PI, 0.000001);

	/* Unit 3/4 circle at 0,0 */
	A1.x = -1; A1.y = 0;
	A2.x = 1; A2.y = 0;
	A3.x = 0; A3.y = -1;

	/* Arc to right of the unit semicircle */
	d = lw_arc_length(&A1, &A2, &A3);
	CU_ASSERT_DOUBLE_EQUAL(d, 3*M_PI/2, 0.000001);
	d = lw_arc_length(&A3, &A2, &A1);
	CU_ASSERT_DOUBLE_EQUAL(d, 3*M_PI/2, 0.000001);	
}

/*
** Used by test harness to register the tests in this file.
*/
CU_TestInfo measures_tests[] =
{
	PG_TEST(test_mindistance2d_tolerance),
	PG_TEST(test_rect_tree_contains_point),
	PG_TEST(test_rect_tree_intersects_tree),
	PG_TEST(test_lwgeom_segmentize2d),
	PG_TEST(test_lwgeom_locate_along),
	PG_TEST(test_lw_dist2d_pt_arc),
	PG_TEST(test_lw_dist2d_seg_arc),
	PG_TEST(test_lw_dist2d_arc_arc),
	PG_TEST(test_lw_arc_length),
	CU_TEST_INFO_NULL
};
CU_SuiteInfo measures_suite = {"PostGIS Measures Suite",  NULL,  NULL, measures_tests};
