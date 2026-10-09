/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "CUnit/Basic.h"
#include "CUnit/CUnit.h"

#include "liblwgeom_internal.h"
#include "topo/liblwgeom_topo.h"
#include "lwgeom_geos.h"
#include "cu_tester.h"

static LWGEOM *
lwgeom_from_text(const char *str)
{
	LWGEOM_PARSER_RESULT r;
	if (LW_FAILURE == lwgeom_parse_wkt(&r, (char *)str, LW_PARSER_CHECK_NONE))
		return NULL;
	return r.geom;
}

static void
test_lwt_IsTopoRingCCW_large_finite_coordinates(void)
{
	LWGEOM *geom = lwgeom_from_text("POLYGON((0 1e308,1e308 0,0 0,0 1e308))");
	LWPOLY *poly;

	CU_ASSERT_PTR_NOT_NULL(geom);
	if (!geom)
		return;

	poly = lwgeom_as_lwpoly(geom);
	CU_ASSERT_PTR_NOT_NULL(poly);
	if (!poly)
	{
		lwgeom_free(geom);
		return;
	}

	CU_ASSERT_EQUAL(lwt_IsTopoRingCCW(poly->rings[0]), LW_FALSE);

	lwgeom_free(geom);
}

/*
** Assert the non-boundary intersection status of a pair of lines
** described by WKT, as reported by the exact arithmetic predicate
** used by the topology edge crossing checks.
**
** The check must be symmetric, so both argument orders are verified.
*/
#define ASSERT_NONBOUNDARY_INTERSECTION(wkt1, wkt2, expected) \
	do \
	{ \
		LWLINE *l1 = (LWLINE *)lwgeom_from_text(wkt1); \
		LWLINE *l2 = (LWLINE *)lwgeom_from_text(wkt2); \
		int exp = (expected); \
		CU_ASSERT_EQUAL(lwt_LineHaveNonBoundary2DIntersection(l1, l2), exp); \
		CU_ASSERT_EQUAL(lwt_LineHaveNonBoundary2DIntersection(l2, l1), exp); \
		lwline_free(l1); \
		lwline_free(l2); \
	} while (0)

static void
test_lwt_LineHaveNonBoundary2DIntersection(void)
{
	/* Non intersecting lines */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 100 0)", "LINESTRING(200 0, 400 0)", 0);
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 100 0)", "LINESTRING(0 50, 100 50)", 0);
	/* The two lines would cross, but only outside of both segments */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 100 0)", "LINESTRING(200 100, 200 -100)", 0);
	/* The two lines would touch, but only outside of both segments */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 100 0)", "LINESTRING(-100 0, -50 0)", 0);
	/* Lines parallel and apart, also on a diagonal */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 100 100)", "LINESTRING(0 50, 100 150)", 0);

	/* Touching on a boundary point of both lines */
	/* End to start */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 100 0)", "LINESTRING(100 0, 200 0)", 0);
	/* Start to start */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 100 0)", "LINESTRING(0 0, 0 100)", 0);
	/* Start to end */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 100 0)", "LINESTRING(100 0, 100 100)", 0);
	/* End to end */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 100 0)", "LINESTRING(200 0, 100 0)", 0);
	/* Same point, opposite directions */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(100 0, 0 0)", "LINESTRING(0 0, -100 0)", 0);
	/* Collinear, touching end to start on a vertical line */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 0 100)", "LINESTRING(0 100, 0 200)", 0);
	/* Collinear, touching end to start on a diagonal */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 100 100)", "LINESTRING(100 100, 200 200)", 0);
	/* Collinear, touching on the interior vertex of the first line */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 50 0, 100 0)", "LINESTRING(100 0, 150 0)", 0);
	/* Diverging from the shared start point */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 100 0)", "LINESTRING(0 0, 50 50)", 0);
	/* The endpoint of the second line is the start point of the first */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 100 0)", "LINESTRING(-50 0, 0 0)", 0);
	/* Zero length second line, on the boundary of the first one */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 100 0)", "LINESTRING(100 0, 100 0)", 0);
	/* Zero length lines, on the same point */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(50 50, 50 50)", "LINESTRING(50 50, 50 50)", 0);
	/* Closed line, touching another line on its boundary point */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 100 0, 100 100, 0 100, 0 0)",
	                               "LINESTRING(0 0, -100 0)",
	                               0);

	/* Crossing lines */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 100 0)", "LINESTRING(50 10, 50 -10)", 1);
	/* Crossing on a diagonal */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 100 100)", "LINESTRING(0 100, 100 0)", 1);
	/* Crossing, but one line is zero length and on the other one interior */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 100 0)", "LINESTRING(50 0, 50 0)", 1);

	/* Identical lines */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 100 0)", "LINESTRING(0 0, 100 0)", 1);
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 50 0, 100 0)", "LINESTRING(0 0, 50 0, 100 0)", 1);
	/* Identical lines, opposite direction, on a vertical */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 0 100)", "LINESTRING(0 100, 0 0)", 1);

	/* Endpoint of one line on the interior of the other one */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 100 0)", "LINESTRING(50 0, 50 50)", 1);
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 100 0)", "LINESTRING(50 -50, 50 0)", 1);
	/* Zero length first line, on the interior of the second one */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(50 0, 50 0)", "LINESTRING(0 0, 100 0)", 1);
	/* Collinear, zero length second line on the interior of the first one */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 50 0, 100 0)", "LINESTRING(50 0, 50 0)", 1);
	/* Interior vertex of the first line, endpoint of the second one */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 50 0, 100 0)", "LINESTRING(50 0, 50 100)", 1);
	/* Interior vertex of the first line, interior of the second one */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 50 0, 100 0)", "LINESTRING(50 -50, 50 50)", 1);
	/* Collinear, overlapping for a positive length */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 100 0)", "LINESTRING(50 0, 150 0)", 1);
	/* Collinear, overlapping for a positive length, opposite direction */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(100 0, 0 0)", "LINESTRING(-50 0, 50 0)", 1);
	/* Collinear, second line contained in the first one */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 100 0)", "LINESTRING(25 0, 75 0)", 1);
	/* Collinear, overlapping on a diagonal */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 100 100)", "LINESTRING(50 50, 150 150)", 1);
	/* Collinear, overlapping on a vertical */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 0 100)", "LINESTRING(0 50, 0 200)", 1);
	/* Collinear, overlapping, the shared point is on the line boundary */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 100 0)", "LINESTRING(0 0, 50 0)", 1);
	/* Collinear, overlapping across multiple segments */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 100 0)", "LINESTRING(-50 0, 25 0, 150 0)", 1);
	/* Collinear, the second line is back and forth over the first one */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 100 0)", "LINESTRING(50 0, 100 100, 50 0)", 1);
	/* Second line touches an interior vertex of first line */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 100 0, 200 0)", "LINESTRING(100 0, 100 -50)", 1);

	/*
	** Multiple intersections on the same pair of lines, the shared
	** boundary point must not hide the non-boundary ones
	*/
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 100 0)", "LINESTRING(0 0, 50 50, 50 -50, 0 0)", 1);
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 100 0)",
	                               "LINESTRING(0 0, 50 50, 100 0, 50 -50, 0 0)",
	                               1);

	/* Z and M coordinates are ignored */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING Z (0 0 100, 100 0 200)",
	                               "LINESTRING M (50 10 1, 50 -10 2)",
	                               1);
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING Z (0 0 1, 100 0 1)", "LINESTRING Z (0 0 2, 100 0 2)", 1);
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING ZM (0 0 1 1, 100 0 1 1)",
	                               "LINESTRING Z (100 0 9, 100 0 9)",
	                               0);

	/* Empty lines have nothing to intersect */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING EMPTY", "LINESTRING(0 0, 100 0)", 0);
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(0 0, 100 0)", "LINESTRING EMPTY", 0);
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING EMPTY", "LINESTRING EMPTY", 0);

	/* Single point lines have no segment to intersect */
	{
		POINT4D pt;
		LWLINE *single, *line;

		pt.x = 0;
		pt.y = 0;
		pt.z = 0;
		pt.m = 0;

		single = lwline_construct_empty(SRID_UNKNOWN, 0, 0);
		ptarray_append_point(single->points, &pt, LW_TRUE);
		line = (LWLINE *)lwgeom_from_text("LINESTRING(0 0, 100 0)");

		CU_ASSERT_EQUAL( lwt_LineHaveNonBoundary2DIntersection(single, line), 0 );
		CU_ASSERT_EQUAL( lwt_LineHaveNonBoundary2DIntersection(line, single), 0 );
		CU_ASSERT_EQUAL( lwt_LineHaveNonBoundary2DIntersection(single, single), 0 );

		lwline_free(single);
		lwline_free(line);
	}

	/* NULL lines are handled gracefully */
	CU_ASSERT_EQUAL( lwt_LineHaveNonBoundary2DIntersection(NULL, NULL), 0 );

	/* The pair of lines below is disjoint
	 * See https://trac.osgeo.org/postgis/ticket/6143 */
	ASSERT_NONBOUNDARY_INTERSECTION("LINESTRING(-1e12 -1e12, -1 1e-5)", "LINESTRING(1e5 -1e5, -1 -1e-11)", 0);

}

void
topo_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("topology", NULL, NULL);
	PG_ADD_TEST(suite, test_lwt_IsTopoRingCCW_large_finite_coordinates);
	PG_ADD_TEST(suite, test_lwt_LineHaveNonBoundary2DIntersection);
}
