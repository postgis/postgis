/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2008 Paul Ramsey
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "cu_algorithm.h"

/*
** Called from test harness to register the tests in this file.
*/
CU_pSuite register_cg_suite(void)
{
	CU_pSuite pSuite;
	pSuite = CU_add_suite("PostGIS Computational Geometry Suite", init_cg_suite, clean_cg_suite);
	if (NULL == pSuite)
	{
		CU_cleanup_registry();
		return NULL;
	}

	if (
	    (NULL == CU_add_test(pSuite, "test_lw_segment_side()", test_lw_segment_side)) ||
	    (NULL == CU_add_test(pSuite, "test_lw_segment_intersects()", test_lw_segment_intersects)) ||
	    (NULL == CU_add_test(pSuite, "test_lwline_crossing_short_lines()", test_lwline_crossing_short_lines)) ||
	    (NULL == CU_add_test(pSuite, "test_lwline_crossing_long_lines()", test_lwline_crossing_long_lines)) ||
	    (NULL == CU_add_test(pSuite, "test_lwpoint_set_ordinate()", test_lwpoint_set_ordinate)) ||
	    (NULL == CU_add_test(pSuite, "test_lwpoint_get_ordinate()", test_lwpoint_get_ordinate)) ||
	    (NULL == CU_add_test(pSuite, "test_lwpoint_interpolate()", test_lwpoint_interpolate)) ||
	    (NULL == CU_add_test(pSuite, "test_lwline_clip()", test_lwline_clip)) ||
	    (NULL == CU_add_test(pSuite, "test_lwline_clip_big()", test_lwline_clip_big)) ||
	    (NULL == CU_add_test(pSuite, "test_lwmline_clip()", test_lwmline_clip)) ||
	    (NULL == CU_add_test(pSuite, "test_geohash_point()", test_geohash_point)) ||
	    (NULL == CU_add_test(pSuite, "test_geohash_precision()", test_geohash_precision)) ||
	    (NULL == CU_add_test(pSuite, "test_geohash()", test_geohash))
	)
	{
		CU_cleanup_registry();
		return NULL;
	}
	return pSuite;
}


/*
** Global variables used by tests below
*/
POINT2D *p1 = NULL;
POINT2D *p2 = NULL;
POINT2D *q1 = NULL;
POINT2D *q2 = NULL;
POINT4D *p = NULL;
POINT4D *q = NULL;
/* Two-point objects */
POINTARRAY *pa21 = NULL;
POINTARRAY *pa22 = NULL;
LWLINE *l21 = NULL;
LWLINE *l22 = NULL;
/* Five-point objects */
LWLINE *l51 = NULL;
LWLINE *l52 = NULL;
/* Parsing support */
LWGEOM_PARSER_RESULT parse_result;


/*
** The suite initialization function.
** Create any re-used objects.
*/
int init_cg_suite(void)
{
	p = lwalloc(sizeof(POINT4D));
	q = lwalloc(sizeof(POINT4D));
	p1 = lwalloc(sizeof(POINT2D));
	p2 = lwalloc(sizeof(POINT2D));
	q1 = lwalloc(sizeof(POINT2D));
	q2 = lwalloc(sizeof(POINT2D));
	pa21 = ptarray_construct(0, 0, 2);
	pa22 = ptarray_construct(0, 0, 2);
	l21 = lwline_construct(-1, NULL, pa21);
	l22 = lwline_construct(-1, NULL, pa22);
	return 0;

}

/*
** The suite cleanup function.
** Frees any global objects.
*/
int clean_cg_suite(void)
{
	if ( p ) lwfree(p);
	if ( p1 )lwfree(p1);
	if ( p2 ) lwfree(p2);
	if ( q1 ) lwfree(q1);
	if ( q2 ) lwfree(q2);
	if ( l21 ) lwline_free(l21);
	if ( l22 ) lwline_free(l22);
	return 0;
}

/*
** Test left/right side.
*/
void test_lw_segment_side(void)
{
	double rv = 0.0;
	POINT2D *q = NULL;
	q = lwalloc(sizeof(POINT2D));

	/* Vertical line at x=0 */
	p1->x = 0.0;
	p1->y = 0.0;
	p2->x = 0.0;
	p2->y = 1.0;

	/* On the left */
	q->x = -2.0;
	q->y = 1.5;
	rv = lw_segment_side(p1, p2, q);
	//printf("left %g\n",rv);
	CU_ASSERT(rv < 0.0);

	/* On the right */
	q->x = 2.0;
	rv = lw_segment_side(p1, p2, q);
	//printf("right %g\n",rv);
	CU_ASSERT(rv > 0.0);

	/* On the line */
	q->x = 0.0;
	rv = lw_segment_side(p1, p2, q);
	//printf("on line %g\n",rv);
	CU_ASSERT_EQUAL(rv, 0.0);

	lwfree(q);

}

/*
** Test crossings side.
*/
void test_lw_segment_intersects(void)
{

	/* P: Vertical line at x=0 */
	p1->x = 0.0;
	p1->y = 0.0;
	p2->x = 0.0;
	p2->y = 1.0;

	/* Q: Horizontal line crossing left to right */
	q1->x = -0.5;
	q1->y = 0.5;
	q2->x = 0.5;
	q2->y = 0.5;
	CU_ASSERT( lw_segment_intersects(p1, p2, q1, q2) == SEG_CROSS_RIGHT );

	/* Q: Horizontal line crossing right to left */
	q1->x = 0.5;
	q1->y = 0.5;
	q2->x = -0.5;
	q2->y = 0.5;
	CU_ASSERT( lw_segment_intersects(p1, p2, q1, q2) == SEG_CROSS_LEFT );

	/* Q: Horizontal line not crossing right to left */
	q1->x = 0.5;
	q1->y = 1.5;
	q2->x = -0.5;
	q2->y = 1.5;
	CU_ASSERT( lw_segment_intersects(p1, p2, q1, q2) == SEG_NO_INTERSECTION );

	/* Q: Horizontal line crossing at vertex right to left */
	q1->x = 0.5;
	q1->y = 1.0;
	q2->x = -0.5;
	q2->y = 1.0;
	CU_ASSERT( lw_segment_intersects(p1, p2, q1, q2) == SEG_CROSS_LEFT );

	/* Q: Diagonal line with large range crossing at vertex right to left */
	q1->x = 0.5;
	q1->y = 10.0;
	q2->x = -0.5;
	q2->y = -10.0;
	CU_ASSERT( lw_segment_intersects(p1, p2, q1, q2) == SEG_CROSS_LEFT );

	/* Q: Diagonal line with large range crossing at vertex right to left */
	q1->x = 0.5;
	q1->y = 11.0;
	q2->x = -0.5;
	q2->y = -9.0;
	CU_ASSERT( lw_segment_intersects(p1, p2, q1, q2) == SEG_CROSS_LEFT );

	/* Q: Horizontal touching from left */
	q1->x = -0.5;
	q1->y = 0.5;
	q2->x = 0.0;
	q2->y = 0.5;
	CU_ASSERT( lw_segment_intersects(p1, p2, q1, q2) == SEG_TOUCH_LEFT );

	/* Q: Horizontal touching from right */
	q1->x = 0.5;
	q1->y = 0.5;
	q2->x = 0.0;
	q2->y = 0.5;
	CU_ASSERT( lw_segment_intersects(p1, p2, q1, q2) == SEG_TOUCH_RIGHT );

	/* Q: Horizontal touching from left and far below*/
	q1->x = -0.5;
	q1->y = -10.5;
	q2->x = 0.0;
	q2->y = 0.5;
	CU_ASSERT( lw_segment_intersects(p1, p2, q1, q2) == SEG_TOUCH_LEFT );

	/* Q: Horizontal touching from right and far above */
	q1->x = 0.5;
	q1->y = 10.5;
	q2->x = 0.0;
	q2->y = 0.5;
	CU_ASSERT( lw_segment_intersects(p1, p2, q1, q2) == SEG_TOUCH_RIGHT );

	/* Q: Co-linear from top */
	q1->x = 0.0;
	q1->y = 10.0;
	q2->x = 0.0;
	q2->y = 0.5;
	CU_ASSERT( lw_segment_intersects(p1, p2, q1, q2) == SEG_COLINEAR );

	/* Q: Co-linear from bottom */
	q1->x = 0.0;
	q1->y = -10.0;
	q2->x = 0.0;
	q2->y = 0.5;
	CU_ASSERT( lw_segment_intersects(p1, p2, q1, q2) == SEG_COLINEAR );

	/* Q: Co-linear contained */
	q1->x = 0.0;
	q1->y = 0.4;
	q2->x = 0.0;
	q2->y = 0.5;
	CU_ASSERT( lw_segment_intersects(p1, p2, q1, q2) == SEG_COLINEAR );

	/* Q: Horizontal touching at end point from left */
	q1->x = -0.5;
	q1->y = 1.0;
	q2->x = 0.0;
	q2->y = 1.0;
	CU_ASSERT( lw_segment_intersects(p1, p2, q1, q2) == SEG_TOUCH_LEFT );

	/* Q: Horizontal touching at end point from right */
	q1->x = 0.5;
	q1->y = 1.0;
	q2->x = 0.0;
	q2->y = 1.0;
	CU_ASSERT( lw_segment_intersects(p1, p2, q1, q2) == SEG_TOUCH_RIGHT );

	/* Q: Horizontal touching at start point from left */
	q1->x = -0.5;
	q1->y = 0.0;
	q2->x = 0.0;
	q2->y = 0.0;
	CU_ASSERT( lw_segment_intersects(p1, p2, q1, q2) == SEG_TOUCH_LEFT );

	/* Q: Horizontal touching at start point from right */
	q1->x = 0.5;
	q1->y = 0.0;
	q2->x = 0.0;
	q2->y = 0.0;
	CU_ASSERT( lw_segment_intersects(p1, p2, q1, q2) == SEG_TOUCH_RIGHT );

}

void test_lwline_crossing_short_lines(void)
{

	/*
	** Simple test, two two-point lines 
	*/

	/* Vertical line from 0,0 to 1,1 */
	p->x = 0.0;
	p->y = 0.0;
	setPoint4d(pa21, 0, p);
	p->y = 1.0;
	setPoint4d(pa21, 1, p);

	/* Horizontal, crossing mid-segment */
	p->x = -0.5;
	p->y = 0.5;
	setPoint4d(pa22, 0, p);
	p->x = 0.5;
	setPoint4d(pa22, 1, p);

	CU_ASSERT( lwline_crossing_direction(l21, l22) == LINE_CROSS_RIGHT );

	/* Horizontal, crossing at top end vertex */
	p->x = -0.5;
	p->y = 1.0;
	setPoint4d(pa22, 0, p);
	p->x = 0.5;
	setPoint4d(pa22, 1, p);

	CU_ASSERT( lwline_crossing_direction(l21, l22) == LINE_CROSS_RIGHT );

	/* Horizontal, crossing at bottom end vertex */
	p->x = -0.5;
	p->y = 0.0;
	setPoint4d(pa22, 0, p);
	p->x = 0.5;
	setPoint4d(pa22, 1, p);

	CU_ASSERT( lwline_crossing_direction(l21, l22) == LINE_CROSS_RIGHT );

	/* Horizontal, no crossing */
	p->x = -0.5;
	p->y = 2.0;
	setPoint4d(pa22, 0, p);
	p->x = 0.5;
	setPoint4d(pa22, 1, p);

	CU_ASSERT( lwline_crossing_direction(l21, l22) == LINE_NO_CROSS );

	/* Vertical, no crossing */
	p->x = -0.5;
	p->y = 0.0;
	setPoint4d(pa22, 0, p);
	p->y = 1.0;
	setPoint4d(pa22, 1, p);

	CU_ASSERT( lwline_crossing_direction(l21, l22) == LINE_NO_CROSS );

}

void test_lwline_crossing_long_lines(void)
{
	LWLINE *l51;
	LWLINE *l52;
	/*
	** More complex test, longer lines and multiple crossings 
	*/

	/* Vertical line with vertices at y integers */
	l51 = (LWLINE*)lwgeom_from_ewkt("LINESTRING(0 0, 0 1, 0 2, 0 3, 0 4)", PARSER_CHECK_NONE);

	/* Two crossings at segment midpoints */
	l52 = (LWLINE*)lwgeom_from_ewkt("LINESTRING(1 1, -1 1.5, 1 3, 1 4, 1 5)", PARSER_CHECK_NONE);
	CU_ASSERT( lwline_crossing_direction(l51, l52) == LINE_MULTICROSS_END_SAME_FIRST_LEFT );
	lwline_free(l52);

	/* One crossing at interior vertex */
	l52 = (LWLINE*)lwgeom_from_ewkt("LINESTRING(1 1, 0 1, -1 1, -1 2, -1 3)", PARSER_CHECK_NONE);
	CU_ASSERT( lwline_crossing_direction(l51, l52) == LINE_CROSS_LEFT );
	lwline_free(l52);

	/* Two crossings at interior vertices */
	l52 = (LWLINE*)lwgeom_from_ewkt("LINESTRING(1 1, 0 1, -1 1, 0 3, 1 3)", PARSER_CHECK_NONE);
	CU_ASSERT( lwline_crossing_direction(l51, l52) == LINE_MULTICROSS_END_SAME_FIRST_LEFT );
	lwline_free(l52);

	/* Two crossings, one at the first vertex on at interior vertex */
	l52 = (LWLINE*)lwgeom_from_ewkt("LINESTRING(1 0, 0 0, -1 1, 0 3, 1 3)", PARSER_CHECK_NONE);
	CU_ASSERT( lwline_crossing_direction(l51, l52) == LINE_MULTICROSS_END_SAME_FIRST_LEFT );
	lwline_free(l52);

	/* Two crossings, one at the first vertex on the next interior vertex */
	l52 = (LWLINE*)lwgeom_from_ewkt("LINESTRING(1 0, 0 0, -1 1, 0 1, 1 2)", PARSER_CHECK_NONE);
	CU_ASSERT( lwline_crossing_direction(l51, l52) == LINE_MULTICROSS_END_SAME_FIRST_LEFT );
	lwline_free(l52);

	/* Two crossings, one at the last vertex on the next interior vertex */
	l52 = (LWLINE*)lwgeom_from_ewkt("LINESTRING(1 4, 0 4, -1 3, 0 3, 1 3)", PARSER_CHECK_NONE);
	CU_ASSERT( lwline_crossing_direction(l51, l52) == LINE_MULTICROSS_END_SAME_FIRST_LEFT );
	lwline_free(l52);

	/* Three crossings, two at midpoints, one at vertex */
	l52 = (LWLINE*)lwgeom_from_ewkt("LINESTRING(0.5 1, -1 0.5, 1 2, -1 2, -1 3)", PARSER_CHECK_NONE);
	CU_ASSERT( lwline_crossing_direction(l51, l52) == LINE_MULTICROSS_END_LEFT );
	lwline_free(l52);

	/* One mid-point co-linear crossing */
	l52 = (LWLINE*)lwgeom_from_ewkt("LINESTRING(1 1, 0 1.5, 0 2.5, -1 3, -1 4)", PARSER_CHECK_NONE);
	CU_ASSERT( lwline_crossing_direction(l51, l52) == LINE_CROSS_LEFT );
	lwline_free(l52);

	/* One on-vertices co-linear crossing */
	l52 = (LWLINE*)lwgeom_from_ewkt("LINESTRING(1 1, 0 1, 0 2, -1 4, -1 4)", PARSER_CHECK_NONE);
	CU_ASSERT( lwline_crossing_direction(l51, l52) == LINE_CROSS_LEFT );
	lwline_free(l52);

	/* No crossing, but end on a co-linearity. */
	l52 = (LWLINE*)lwgeom_from_ewkt("LINESTRING(1 1, 1 2, 1 3, 0 3, 0 4)", PARSER_CHECK_NONE);
	CU_ASSERT( lwline_crossing_direction(l51, l52) == LINE_NO_CROSS );
	lwline_free(l52);

	lwline_free(l51);

}

void test_lwpoint_set_ordinate(void)
{
	p->x = 0.0;
	p->y = 0.0;
	p->z = 0.0;
	p->m = 0.0;

	lwpoint_set_ordinate(p, 0, 1.5);
	CU_ASSERT_EQUAL( p->x, 1.5 );

	lwpoint_set_ordinate(p, 3, 2.5);
	CU_ASSERT_EQUAL( p->m, 2.5 );

	lwpoint_set_ordinate(p, 2, 3.5);
	CU_ASSERT_EQUAL( p->z, 3.5 );

}

void test_lwpoint_get_ordinate(void)
{

	p->x = 10.0;
	p->y = 20.0;
	p->z = 30.0;
	p->m = 40.0;

	CU_ASSERT_EQUAL( lwpoint_get_ordinate(p, 0), 10.0 );
	CU_ASSERT_EQUAL( lwpoint_get_ordinate(p, 1), 20.0 );
	CU_ASSERT_EQUAL( lwpoint_get_ordinate(p, 2), 30.0 );
	CU_ASSERT_EQUAL( lwpoint_get_ordinate(p, 3), 40.0 );

}

void test_lwpoint_interpolate(void)
{
	POINT4D *r = NULL;
	r = lwalloc(sizeof(POINT4D));
	int rv = 0;

	p->x = 10.0;
	p->y = 20.0;
	p->z = 30.0;
	p->m = 40.0;
	q->x = 20.0;
	q->y = 30.0;
	q->z = 40.0;
	q->m = 50.0;

	rv = lwpoint_interpolate(p, q, r, 4, 2, 35.0);
	CU_ASSERT_EQUAL( r->x, 15.0);

	rv = lwpoint_interpolate(p, q, r, 4, 3, 41.0);
	CU_ASSERT_EQUAL( r->y, 21.0);

	rv = lwpoint_interpolate(p, q, r, 4, 3, 50.0);
	CU_ASSERT_EQUAL( r->y, 30.0);

	rv = lwpoint_interpolate(p, q, r, 4, 3, 40.0);
	CU_ASSERT_EQUAL( r->y, 20.0);

	lwfree(r);

}

void test_lwline_clip(void)
{
	LWCOLLECTION *c;
	LWLINE *line = NULL;
	LWLINE *l51 = NULL;
	char *ewkt;

	/* Vertical line with vertices at y integers */
	l51 = (LWLINE*)lwgeom_from_ewkt("LINESTRING(0 0, 0 1, 0 2, 0 3, 0 4)", PARSER_CHECK_NONE);

	/* Clip in the middle, mid-range. */
	c = lwline_clip_to_ordinate_range(l51, 1, 1.5, 2.5);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c, PARSER_CHECK_NONE);
	//printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTILINESTRING((0 1.5,0 2,0 2.5))");
	lwfree(ewkt);
	lwcollection_free(c);

	/* Clip off the top. */
	c = lwline_clip_to_ordinate_range(l51, 1, 3.5, 5.5);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c, PARSER_CHECK_NONE);
	//printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTILINESTRING((0 3.5,0 4))");
	lwfree(ewkt);
	lwcollection_free(c);

	/* Clip off the bottom. */
	c = lwline_clip_to_ordinate_range(l51, 1, -1.5, 2.5);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c, PARSER_CHECK_NONE);
	//printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTILINESTRING((0 0,0 1,0 2,0 2.5))" );
	lwfree(ewkt);
	lwcollection_free(c);

	/* Range holds entire object. */
	c = lwline_clip_to_ordinate_range(l51, 1, -1.5, 5.5);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c, PARSER_CHECK_NONE);
	//printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTILINESTRING((0 0,0 1,0 2,0 3,0 4))" );
	lwfree(ewkt);
	lwcollection_free(c);

	/* Clip on vertices. */
	c = lwline_clip_to_ordinate_range(l51, 1, 1.0, 2.0);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c, PARSER_CHECK_NONE);
	//printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTILINESTRING((0 1,0 2))" );
	lwfree(ewkt);
	lwcollection_free(c);

	/* Clip on vertices off the bottom. */
	c = lwline_clip_to_ordinate_range(l51, 1, -1.0, 2.0);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c, PARSER_CHECK_NONE);
	//printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTILINESTRING((0 0,0 1,0 2))" );
	lwfree(ewkt);
	lwcollection_free(c);

	/* Clip on top. */
	c = lwline_clip_to_ordinate_range(l51, 1, -1.0, 0.0);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c, PARSER_CHECK_NONE);
	//printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "GEOMETRYCOLLECTION(POINT(0 0))" );
	lwfree(ewkt);
	lwcollection_free(c);

	/* ST_LocateBetweenElevations(ST_GeomFromEWKT('LINESTRING(1 2 3, 4 5 6, 6 6 6, 1 1 1)'), 1, 2)) */
	line = (LWLINE*)lwgeom_from_ewkt("LINESTRING(1 2 3, 4 5 6, 6 6 6, 1 1 1)", PARSER_CHECK_NONE);
	c = lwline_clip_to_ordinate_range(line, 2, 1.0, 2.0);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c, PARSER_CHECK_NONE);
	//printf("a = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTILINESTRING((2 2 2,1 1 1))" );
	lwfree(ewkt);
	lwcollection_free(c);
	lwline_free(line);

	/* ST_LocateBetweenElevations('LINESTRING(1 2 3, 4 5 6, 6 6 6, 1 1 1)', 1, 2)) */
	line = (LWLINE*)lwgeom_from_ewkt("LINESTRING(1 2 3, 4 5 6, 6 6 6, 1 1 1)", PARSER_CHECK_NONE);
	c = lwline_clip_to_ordinate_range(line, 2, 1.0, 2.0);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c, PARSER_CHECK_NONE);
	//printf("a = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTILINESTRING((2 2 2,1 1 1))" );
	lwfree(ewkt);
	lwcollection_free(c);
	lwline_free(line);

	/* ST_LocateBetweenElevations('LINESTRING(1 2 3, 4 5 6, 6 6 6, 1 1 1)', 1, 1)) */
	line = (LWLINE*)lwgeom_from_ewkt("LINESTRING(1 2 3, 4 5 6, 6 6 6, 1 1 1)", PARSER_CHECK_NONE);
	c = lwline_clip_to_ordinate_range(line, 2, 1.0, 1.0);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c, PARSER_CHECK_NONE);
	//printf("b = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "GEOMETRYCOLLECTION(POINT(1 1 1))" );
	lwfree(ewkt);
	lwcollection_free(c);
	lwline_free(line);

	/* ST_LocateBetweenElevations('LINESTRING(1 1 1, 1 2 2)', 1,1) */
	line = (LWLINE*)lwgeom_from_ewkt("LINESTRING(1 1 1, 1 2 2)", PARSER_CHECK_NONE);
	c = lwline_clip_to_ordinate_range(line, 2, 1.0, 1.0);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c, PARSER_CHECK_NONE);
	//printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "GEOMETRYCOLLECTION(POINT(1 1 1))" );
	lwfree(ewkt);
	lwcollection_free(c);
	lwline_free(line);

	lwline_free(l51);

}

void test_lwmline_clip(void)
{
	LWCOLLECTION *c;
	char *ewkt;
	LWMLINE *mline = NULL;
	LWLINE *line = NULL;

	/*
	** Set up the input line. Trivial one-member case. 
	*/
	mline = (LWMLINE*)lwgeom_from_ewkt("MULTILINESTRING((0 0,0 1,0 2,0 3,0 4))", PARSER_CHECK_NONE);

	/* Clip in the middle, mid-range. */
	c = lwmline_clip_to_ordinate_range(mline, 1, 1.5, 2.5);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c,0);
	//printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTILINESTRING((0 1.5,0 2,0 2.5))");
	lwfree(ewkt);
	lwcollection_free(c);

	lwmline_free(mline);

	/*
	** Set up the input line. Two-member case. 
	*/
	mline = (LWMLINE*)lwgeom_from_ewkt("MULTILINESTRING((1 0,1 1,1 2,1 3,1 4), (0 0,0 1,0 2,0 3,0 4))", PARSER_CHECK_NONE);

	/* Clip off the top. */
	c = lwmline_clip_to_ordinate_range(mline, 1, 3.5, 5.5);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c, PARSER_CHECK_NONE);
	//printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTILINESTRING((1 3.5,1 4),(0 3.5,0 4))");
	lwfree(ewkt);
	lwcollection_free(c);

	lwmline_free(mline);

	/*
	** Set up staggered input line to create multi-type output. 
	*/
	mline = (LWMLINE*)lwgeom_from_ewkt("MULTILINESTRING((1 0,1 -1,1 -2,1 -3,1 -4), (0 0,0 1,0 2,0 3,0 4))", PARSER_CHECK_NONE);

	/* Clip from 0 upwards.. */
	c = lwmline_clip_to_ordinate_range(mline, 1, 0.0, 2.5);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c, PARSER_CHECK_NONE);
	//printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "GEOMETRYCOLLECTION(POINT(1 0),LINESTRING(0 0,0 1,0 2,0 2.5))");
	lwfree(ewkt);
	lwcollection_free(c);

	lwmline_free(mline);

	/*
	** Set up input line from MAC
	*/
	line = (LWLINE*)lwgeom_from_ewkt("LINESTRING(0 0 0 0,1 1 1 1,2 2 2 2,3 3 3 3,4 4 4 4,3 3 3 5,2 2 2 6,1 1 1 7,0 0 0 8)", PARSER_CHECK_NONE);

	/* Clip from 3 to 3.5 */
	c = lwline_clip_to_ordinate_range(line, 2, 3.0, 3.5);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c, PARSER_CHECK_NONE);
	//printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTILINESTRING((3 3 3 3,3.5 3.5 3.5 3.5),(3.5 3.5 3.5 4.5,3 3 3 5))");
	lwfree(ewkt);
	lwcollection_free(c);

	/* Clip from 2 to 3.5 */
	c = lwline_clip_to_ordinate_range(line, 2, 2.0, 3.5);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c, PARSER_CHECK_NONE);
	//printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTILINESTRING((2 2 2 2,3 3 3 3,3.5 3.5 3.5 3.5),(3.5 3.5 3.5 4.5,3 3 3 5,2 2 2 6))");
	lwfree(ewkt);
	lwcollection_free(c);

	/* Clip from 3 to 4 */
	c = lwline_clip_to_ordinate_range(line, 2, 3.0, 4.0);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c, PARSER_CHECK_NONE);
	//printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTILINESTRING((3 3 3 3,4 4 4 4,3 3 3 5))");
	lwfree(ewkt);
	lwcollection_free(c);

	/* Clip from 2 to 3 */
	c = lwline_clip_to_ordinate_range(line, 2, 2.0, 3.0);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c, PARSER_CHECK_NONE);
	//printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTILINESTRING((2 2 2 2,3 3 3 3),(3 3 3 5,2 2 2 6))");
	lwfree(ewkt);
	lwcollection_free(c);


	lwline_free(line);

}



void test_lwline_clip_big(void)
{
	POINTARRAY *pa = ptarray_construct(1, 0, 3);
	LWLINE *line = lwline_construct(-1, NULL, pa);
	LWCOLLECTION *c;
	char *ewkt;

	p->x = 0.0;
	p->y = 0.0;
	p->z = 0.0;
	setPoint4d(pa, 0, p);

	p->x = 1.0;
	p->y = 1.0;
	p->z = 1.0;
	setPoint4d(pa, 1, p);

	p->x = 2.0;
	p->y = 2.0;
	p->z = 2.0;
	setPoint4d(pa, 2, p);

	c = lwline_clip_to_ordinate_range(line, 2, 0.5, 1.5);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c, PARSER_CHECK_NONE);
	//printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTILINESTRING((0.5 0.5 0.5,1 1 1,1.5 1.5 1.5))" );

	lwfree(ewkt);
	lwcollection_free(c);
	lwline_free(line);
}

void test_geohash_precision(void)
{
	BOX3D bbox;
    BOX3D bounds;
	int precision = 0;
	
	bbox.xmin = 23.0;
	bbox.xmax = 23.0;
	bbox.ymin = 25.2;
	bbox.ymax = 25.2;
	precision = lwgeom_geohash_precision(bbox, &bounds);
	printf("\nprecision %d\n",precision);
	CU_ASSERT_EQUAL(precision, 20);

	bbox.xmin = 23.0;
	bbox.ymin = 23.0;
	bbox.xmax = 23.1;
	bbox.ymax = 23.1;
	precision = lwgeom_geohash_precision(bbox, &bounds);
	printf("precision %d\n",precision);
	CU_ASSERT_EQUAL(precision, 2);

	bbox.xmin = 23.0;
	bbox.ymin = 23.0;
	bbox.xmax = 23.0001;
	bbox.ymax = 23.0001;
	precision = lwgeom_geohash_precision(bbox, &bounds);
	printf("precision %d\n",precision);
	CU_ASSERT_EQUAL(precision, 6);

}

void test_geohash_point(void)
{
    char *geohash;
	
    geohash = geohash_point(0, 0, 16);
	//printf("\ngeohash %s\n",geohash);
	CU_ASSERT_STRING_EQUAL(geohash, "7zzzzzzzzzzzzzzz");
	lwfree(geohash);

    geohash = geohash_point(90, 0, 16);
	//printf("\ngeohash %s\n",geohash);
	CU_ASSERT_STRING_EQUAL(geohash, "gzzzzzzzzzzzzzzz");
	lwfree(geohash);

    geohash = geohash_point(20.012345, -20.012345, 15);
	//printf("\ngeohash %s\n",geohash);
	CU_ASSERT_STRING_EQUAL(geohash, "ee9cbe5kqe6pbku");
	lwfree(geohash);

}

void test_geohash(void)
{
	LWPOINT *lwpoint = NULL;
	LWLINE *lwline = NULL;
	LWMLINE *lwmline = NULL;
	char *geohash = NULL;
	
	lwpoint = (LWPOINT*)lwgeom_from_ewkt("POINT(23.0 25.2)", PARSER_CHECK_NONE);
	geohash = lwgeom_geohash((LWGEOM*)lwpoint);
	printf("\ngeohash %s\n",geohash);
	CU_ASSERT_STRING_EQUAL(geohash, "20");
	lwfree(lwpoint);
	lwfree(geohash);

	lwline = (LWLINE*)lwgeom_from_ewkt("LINESTRING(23.0 23.0,23.1 23.1)", PARSER_CHECK_NONE);
	geohash = lwgeom_geohash((LWGEOM*)lwline);
	printf("geohash %s\n",geohash);
	CU_ASSERT_STRING_EQUAL(geohash, "20");
	lwfree(lwline);
	lwfree(geohash);

	lwline = (LWLINE*)lwgeom_from_ewkt("LINESTRING(23.0 23.0,23.001 23.001)", PARSER_CHECK_NONE);
	geohash = lwgeom_geohash((LWGEOM*)lwline);
	printf("geohash %s\n",geohash);
	CU_ASSERT_STRING_EQUAL(geohash, "20");
	lwfree(lwline);
	lwfree(geohash);

	lwmline = (LWMLINE*)lwgeom_from_ewkt("MULTILINESTRING((23.0 23.0,23.1 23.1),(23.0 23.0,23.1 23.1))", PARSER_CHECK_NONE);
	geohash = lwgeom_geohash((LWGEOM*)lwmline);
	printf("geohash %s\n",geohash);
	CU_ASSERT_STRING_EQUAL(geohash, "20");
	lwfree(lwmline);
	lwfree(geohash);
}


