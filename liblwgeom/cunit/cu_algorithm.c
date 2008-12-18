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
	    (NULL == CU_add_test(pSuite, "test_lwline_clip()", test_lwline_clip)) 
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
POINTARRAY *pa51 = NULL;
POINTARRAY *pa52 = NULL;
LWLINE *l51 = NULL;
LWLINE *l52 = NULL;


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
	pa51 = ptarray_construct(0, 0, 5);
	pa52 = ptarray_construct(0, 0, 5);
	l51 = lwline_construct(-1, NULL, pa51);
	l52 = lwline_construct(-1, NULL, pa52);
	return 0;
}

/*
** The suite cleanup function.
** Frees any global objects.
*/
int clean_cg_suite(void)
{
	lwfree(p);
	lwfree(p1);
	lwfree(p2);
	lwfree(q1);
	lwfree(q2);
	lwfree(pa21);
	lwfree(pa22);
	lwfree(pa51);
	lwfree(pa52);
	lwfree(l21);
	lwfree(l22);
	lwfree(l51);
	lwfree(l52);
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

	int rv = 0;
	
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

	/* 
	** More complex test, longer lines and multiple crossings 
	*/

	/* Vertical line with vertices at y integers */
	p->x = 0.0;
	p->y = 0.0;
	setPoint4d(pa51, 0, p);
	p->y = 1.0;
	setPoint4d(pa51, 1, p);
	p->y = 2.0;
	setPoint4d(pa51, 2, p);
	p->y = 3.0;
	setPoint4d(pa51, 3, p);
	p->y = 4.0;
	setPoint4d(pa51, 4, p);

	/* Two crossings at segment midpoints */
	p->x = 1.0;
	p->y = 1.0;
	setPoint4d(pa52, 0, p);
	p->x = -1.0;
	p->y = 1.5;
	setPoint4d(pa52, 1, p);
	p->x = 1.0;
	p->y = 3.0;
	setPoint4d(pa52, 2, p);
	p->x = 1.0;
	p->y = 4.0;
	setPoint4d(pa52, 3, p);
	p->x = 1.0;
	p->y = 5.0;
	setPoint4d(pa52, 4, p);
	CU_ASSERT( lwline_crossing_direction(l51, l52) == LINE_MULTICROSS_END_SAME_FIRST_LEFT );

	/* One crossing at interior vertex */
	p->x = 1.0;
	p->y = 1.0;
	setPoint4d(pa52, 0, p);
	p->x = 0.0;
	p->y = 1.0;
	setPoint4d(pa52, 1, p);
	p->x = -1.0;
	p->y = 1.0;
	setPoint4d(pa52, 2, p);
	p->x = -1.0;
	p->y = 2.0;
	setPoint4d(pa52, 3, p);
	p->x = -1.0;
	p->y = 3.0;
	setPoint4d(pa52, 4, p);
	CU_ASSERT( lwline_crossing_direction(l51, l52) == LINE_CROSS_LEFT );

	/* Two crossings at interior vertices */
	p->x = 1.0;
	p->y = 1.0;
	setPoint4d(pa52, 0, p);
	p->x = 0.0;
	p->y = 1.0;
	setPoint4d(pa52, 1, p);
	p->x = -1.0;
	p->y = 1.0;
	setPoint4d(pa52, 2, p);
	p->x = 0.0;
	p->y = 3.0;
	setPoint4d(pa52, 3, p);
	p->x = 1.0;
	p->y = 3.0;
	setPoint4d(pa52, 4, p);
	CU_ASSERT( lwline_crossing_direction(l51, l52) == LINE_MULTICROSS_END_SAME_FIRST_LEFT );

	/* Two crossings, one at the first vertex on at interior vertex */
	p->x = 1.0;
	p->y = 0.0;
	setPoint4d(pa52, 0, p);
	p->x = 0.0;
	p->y = 0.0;
	setPoint4d(pa52, 1, p);
	p->x = -1.0;
	p->y = 1.0;
	setPoint4d(pa52, 2, p);
	p->x = 0.0;
	p->y = 3.0;
	setPoint4d(pa52, 3, p);
	p->x = 1.0;
	p->y = 3.0;
	setPoint4d(pa52, 4, p);
	CU_ASSERT( lwline_crossing_direction(l51, l52) == LINE_MULTICROSS_END_SAME_FIRST_LEFT );

	/* Two crossings, one at the first vertex on the next interior vertex */
	p->x = 1.0;
	p->y = 0.0;
	setPoint4d(pa52, 0, p);
	p->x = 0.0;
	p->y = 0.0;
	setPoint4d(pa52, 1, p);
	p->x = -1.0;
	p->y = 1.0;
	setPoint4d(pa52, 2, p);
	p->x = 0.0;
	p->y = 1.0;
	setPoint4d(pa52, 3, p);
	p->x = 1.0;
	p->y = 2.0;
	setPoint4d(pa52, 4, p);
	CU_ASSERT( lwline_crossing_direction(l51, l52) == LINE_MULTICROSS_END_SAME_FIRST_LEFT );

	/* Two crossings, one at the last vertex on the next interior vertex */
	p->x = 1.0;
	p->y = 4.0;
	setPoint4d(pa52, 0, p);
	p->x = 0.0;
	p->y = 4.0;
	setPoint4d(pa52, 1, p);
	p->x = -1.0;
	p->y = 3.0;
	setPoint4d(pa52, 2, p);
	p->x = 0.0;
	p->y = 3.0;
	setPoint4d(pa52, 3, p);
	p->x = 1.0;
	p->y = 3.0;
	setPoint4d(pa52, 4, p);
	CU_ASSERT( lwline_crossing_direction(l51, l52) == LINE_MULTICROSS_END_SAME_FIRST_LEFT );

	/* Three crossings, two at midpoints, one at vertex */
	p->x = 0.5;
	p->y = 1.0;
	setPoint4d(pa52, 0, p);
	p->x = -1.0;
	p->y = 0.5;
	setPoint4d(pa52, 1, p);
	p->x = 1.0;
	p->y = 2.0;
	setPoint4d(pa52, 2, p);
	p->x = -1.0;
	p->y = 2.0;
	setPoint4d(pa52, 3, p);
	p->x = -1.0;
	p->y = 3.0;
	setPoint4d(pa52, 4, p);
	CU_ASSERT( lwline_crossing_direction(l51, l52) == LINE_MULTICROSS_END_LEFT );

	/* One mid-point co-linear crossing */
	p->x = 1.0;
	p->y = 1.0;
	setPoint4d(pa52, 0, p);
	p->x = 0.0;
	p->y = 1.5;
	setPoint4d(pa52, 1, p);
	p->x = 0.0;
	p->y = 2.5;
	setPoint4d(pa52, 2, p);
	p->x = -1.0;
	p->y = 3.0;
	setPoint4d(pa52, 3, p);
	p->x = -1.0;
	p->y = 4.0;
	setPoint4d(pa52, 4, p);
	CU_ASSERT( lwline_crossing_direction(l51, l52) == LINE_CROSS_LEFT );

	/* One on-vertices co-linear crossing */
	p->x = 1.0;
	p->y = 1.0;
	setPoint4d(pa52, 0, p);
	p->x = 0.0;
	p->y = 1.0;
	setPoint4d(pa52, 1, p);
	p->x = 0.0;
	p->y = 2.0;
	setPoint4d(pa52, 2, p);
	p->x = -1.0;
	p->y = 3.0;
	setPoint4d(pa52, 3, p);
	p->x = -1.0;
	p->y = 4.0;
	setPoint4d(pa52, 4, p);
	CU_ASSERT( lwline_crossing_direction(l51, l52) == LINE_CROSS_LEFT );

	/* No crossing, but end on a co-linearity. */
	p->x = 1.0;
	p->y = 1.0;
	setPoint4d(pa52, 0, p);
	p->x = 1.0;
	p->y = 2.0;
	setPoint4d(pa52, 1, p);
	p->x = 1.0;
	p->y = 3.0;
	setPoint4d(pa52, 2, p);
	p->x = 0.0;
	p->y = 3.0;
	setPoint4d(pa52, 3, p);
	p->x = 0.0;
	p->y = 4.0;
	setPoint4d(pa52, 4, p);
	CU_ASSERT( lwline_crossing_direction(l51, l52) == LINE_NO_CROSS );


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

    lwfree(r);
    
}

void test_lwline_clip(void)
{
	int rv = 0;
	LWCOLLECTION *c;
	char *ewkt;
	
	c = lwline_clip_to_ordinate_range(l51, 1, 1.5, 2.5);
	ewkt = lwgeom_to_ewkt((LWGEOM*)c,0);
	printf("c = %s\n", ewkt);
	CU_ASSERT_STRING_EQUAL(ewkt, "MULTILINESTRING((0 1.5,0 2,0 2.5))");
	lwfree(ewkt);
	lwgeom_release(c);

	CU_ASSERT_STRING_EQUAL(ewkt, "MULTILINESTRING((0 1.5,0 2,0 2.5))");

	free(c);
	
}

