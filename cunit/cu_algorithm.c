
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
	    (NULL == CU_add_test(pSuite, "testSegmentSide()", testSegmentSide)) ||
	    (NULL == CU_add_test(pSuite, "testSegmentIntersects()", testSegmentIntersects)) ||
	    (NULL == CU_add_test(pSuite, "testLineCrossingShortLines()", testLineCrossingShortLines)) ||
	    (NULL == CU_add_test(pSuite, "testLineCrossingLongLines()", testLineCrossingLongLines)) 
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
void testSegmentSide(void)
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
	rv = segmentSide(p1, p2, q);
	//printf("left %g\n",rv);
	CU_ASSERT(rv < 0.0);
	
	/* On the right */
	q->x = 2.0;
	rv = segmentSide(p1, p2, q);
	//printf("right %g\n",rv);
	CU_ASSERT(rv > 0.0);
	
	/* On the line */
	q->x = 0.0;
	rv = segmentSide(p1, p2, q);
	//printf("on line %g\n",rv);
	CU_ASSERT_EQUAL(rv, 0.0);
	
	lwfree(q);
	
}

/*
** Test crossings side.
*/
void testSegmentIntersects(void)
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
	CU_ASSERT( segmentIntersects(p1, p2, q1, q2) == SEG_CROSS_RIGHT );

	/* Q: Horizontal line crossing right to left */
	q1->x = 0.5;
	q1->y = 0.5;
	q2->x = -0.5;
	q2->y = 0.5;
	CU_ASSERT( segmentIntersects(p1, p2, q1, q2) == SEG_CROSS_LEFT );

	/* Q: Horizontal line not crossing right to left */
	q1->x = 0.5;
	q1->y = 1.5;
	q2->x = -0.5;
	q2->y = 1.5;
	CU_ASSERT( segmentIntersects(p1, p2, q1, q2) == SEG_NO_INTERSECTION );

	/* Q: Horizontal line crossing at vertex right to left */
	q1->x = 0.5;
	q1->y = 1.0;
	q2->x = -0.5;
	q2->y = 1.0;
	CU_ASSERT( segmentIntersects(p1, p2, q1, q2) == SEG_CROSS_LEFT );

	/* Q: Diagonal line with large range crossing at vertex right to left */
	q1->x = 0.5;
	q1->y = 10.0;
	q2->x = -0.5;
	q2->y = -10.0;
	CU_ASSERT( segmentIntersects(p1, p2, q1, q2) == SEG_CROSS_LEFT );

	/* Q: Diagonal line with large range crossing at vertex right to left */
	q1->x = 0.5;
	q1->y = 11.0;
	q2->x = -0.5;
	q2->y = -9.0;
	CU_ASSERT( segmentIntersects(p1, p2, q1, q2) == SEG_CROSS_LEFT );

	/* Q: Horizontal touching from left */
	q1->x = -0.5;
	q1->y = 0.5;
	q2->x = 0.0;
	q2->y = 0.5;
	CU_ASSERT( segmentIntersects(p1, p2, q1, q2) == SEG_TOUCH_LEFT );

	/* Q: Horizontal touching from right */
	q1->x = 0.5;
	q1->y = 0.5;
	q2->x = 0.0;
	q2->y = 0.5;
	CU_ASSERT( segmentIntersects(p1, p2, q1, q2) == SEG_TOUCH_RIGHT );

	/* Q: Horizontal touching from left and far below*/
	q1->x = -0.5;
	q1->y = -10.5;
	q2->x = 0.0;
	q2->y = 0.5;
	CU_ASSERT( segmentIntersects(p1, p2, q1, q2) == SEG_TOUCH_LEFT );

	/* Q: Horizontal touching from right and far above */
	q1->x = 0.5;
	q1->y = 10.5;
	q2->x = 0.0;
	q2->y = 0.5;
	CU_ASSERT( segmentIntersects(p1, p2, q1, q2) == SEG_TOUCH_RIGHT );

	/* Q: Co-linear from top */
	q1->x = 0.0;
	q1->y = 10.0;
	q2->x = 0.0;
	q2->y = 0.5;
	CU_ASSERT( segmentIntersects(p1, p2, q1, q2) == SEG_COLINEAR );

	/* Q: Co-linear from bottom */
	q1->x = 0.0;
	q1->y = -10.0;
	q2->x = 0.0;
	q2->y = 0.5;
	CU_ASSERT( segmentIntersects(p1, p2, q1, q2) == SEG_COLINEAR );

	/* Q: Co-linear contained */
	q1->x = 0.0;
	q1->y = 0.4;
	q2->x = 0.0;
	q2->y = 0.5;
	CU_ASSERT( segmentIntersects(p1, p2, q1, q2) == SEG_COLINEAR );

	/* Q: Horizontal touching at end point from left */
	q1->x = -0.5;
	q1->y = 1.0;
	q2->x = 0.0;
	q2->y = 1.0;
	CU_ASSERT( segmentIntersects(p1, p2, q1, q2) == SEG_TOUCH_LEFT );

	/* Q: Horizontal touching at end point from right */
	q1->x = 0.5;
	q1->y = 1.0;
	q2->x = 0.0;
	q2->y = 1.0;
	CU_ASSERT( segmentIntersects(p1, p2, q1, q2) == SEG_TOUCH_RIGHT );

	/* Q: Horizontal touching at start point from left */
	q1->x = -0.5;
	q1->y = 0.0;
	q2->x = 0.0;
	q2->y = 0.0;
	CU_ASSERT( segmentIntersects(p1, p2, q1, q2) == SEG_TOUCH_LEFT );

	/* Q: Horizontal touching at start point from right */
	q1->x = 0.5;
	q1->y = 0.0;
	q2->x = 0.0;
	q2->y = 0.0;
	CU_ASSERT( segmentIntersects(p1, p2, q1, q2) == SEG_TOUCH_RIGHT );

}

void testLineCrossingShortLines(void) 
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

	CU_ASSERT( lineCrossingDirection(l21, l22) == LINE_CROSS_RIGHT );

	/* Horizontal, crossing at top end vertex */
	p->x = -0.5;
	p->y = 1.0;
	setPoint4d(pa22, 0, p);
	p->x = 0.5;
	setPoint4d(pa22, 1, p);

	CU_ASSERT( lineCrossingDirection(l21, l22) == LINE_CROSS_RIGHT );

	/* Horizontal, crossing at bottom end vertex */
	p->x = -0.5;
	p->y = 0.0;
	setPoint4d(pa22, 0, p);
	p->x = 0.5;
	setPoint4d(pa22, 1, p);

	CU_ASSERT( lineCrossingDirection(l21, l22) == LINE_CROSS_RIGHT );

	/* Horizontal, no crossing */
	p->x = -0.5;
	p->y = 2.0;
	setPoint4d(pa22, 0, p);
	p->x = 0.5;
	setPoint4d(pa22, 1, p);

	CU_ASSERT( lineCrossingDirection(l21, l22) == LINE_NO_CROSS );

	/* Vertical, no crossing */
	p->x = -0.5;
	p->y = 0.0;
	setPoint4d(pa22, 0, p);
	p->y = 1.0;
	setPoint4d(pa22, 1, p);

	CU_ASSERT( lineCrossingDirection(l21, l22) == LINE_NO_CROSS );

}
	
void testLineCrossingLongLines(void) 
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
	CU_ASSERT( lineCrossingDirection(l51, l52) == LINE_MULTICROSS_END_SAME_FIRST_LEFT );

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
	CU_ASSERT( lineCrossingDirection(l51, l52) == LINE_CROSS_LEFT );

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
	CU_ASSERT( lineCrossingDirection(l51, l52) == LINE_MULTICROSS_END_SAME_FIRST_LEFT );

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
	CU_ASSERT( lineCrossingDirection(l51, l52) == LINE_MULTICROSS_END_SAME_FIRST_LEFT );

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
	CU_ASSERT( lineCrossingDirection(l51, l52) == LINE_MULTICROSS_END_SAME_FIRST_LEFT );

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
	CU_ASSERT( lineCrossingDirection(l51, l52) == LINE_MULTICROSS_END_SAME_FIRST_LEFT );

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
	CU_ASSERT( lineCrossingDirection(l51, l52) == LINE_MULTICROSS_END_LEFT );

}

