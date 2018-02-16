/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright (C) 2011 Sandro Santilli <strk@kbt.io>
 * Copyright (C) 2008 Paul Ramsey
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CUnit/Basic.h"
#include "CUnit/CUnit.h"

#include "liblwgeom_internal.h"
#include "cu_tester.h"


static LWGEOM* lwgeom_from_text(const char *str)
{
	LWGEOM_PARSER_RESULT r;
	if( LW_FAILURE == lwgeom_parse_wkt(&r, (char*)str, LW_PARSER_CHECK_NONE) )
		return NULL;
	return r.geom;
}

static char* lwgeom_to_text(const LWGEOM *geom)
{
	return lwgeom_to_wkt(geom, WKT_ISO, 8, NULL);
}

static void test_ptarray_append_point(void)
{
	LWLINE *line;
	char *wkt;
	POINT4D p;

	line = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(0 0,1 1)"));
	p.x = 1;
	p.y = 1;
	ptarray_append_point(line->points, &p, LW_TRUE);
	wkt = lwgeom_to_text(lwline_as_lwgeom(line));
	ASSERT_STRING_EQUAL(wkt,"LINESTRING(0 0,1 1,1 1)");
	lwfree(wkt);

	ptarray_append_point(line->points, &p, LW_FALSE);
	wkt = lwgeom_to_text(lwline_as_lwgeom(line));
	ASSERT_STRING_EQUAL(wkt,"LINESTRING(0 0,1 1,1 1)");
	lwfree(wkt);

	lwline_free(line);
}

static void test_ptarray_insert_point(void)
{
	LWLINE *line;
	char *wkt;
	POINT4D p;

	line = lwgeom_as_lwline(lwgeom_from_text("LINESTRING EMPTY"));
	p.x = 1;
	p.y = 1;
	ptarray_insert_point(line->points, &p, 0);
	wkt = lwgeom_to_text(lwline_as_lwgeom(line));
	ASSERT_STRING_EQUAL(wkt,"LINESTRING(1 1)");
	lwfree(wkt);

	p.x = 2;
	p.y = 20;
	ptarray_insert_point(line->points, &p, 0);
	wkt = lwgeom_to_text(lwline_as_lwgeom(line));
	ASSERT_STRING_EQUAL(wkt,"LINESTRING(2 20,1 1)");
	lwfree(wkt);

	p.x = 3;
	p.y = 30;
	ptarray_insert_point(line->points, &p, 1);
	wkt = lwgeom_to_text(lwline_as_lwgeom(line));
	ASSERT_STRING_EQUAL(wkt,"LINESTRING(2 20,3 30,1 1)");
	lwfree(wkt);

	p.x = 4;
	p.y = 40;
	ptarray_insert_point(line->points, &p, 0);
	wkt = lwgeom_to_text(lwline_as_lwgeom(line));
	ASSERT_STRING_EQUAL(wkt,"LINESTRING(4 40,2 20,3 30,1 1)");
	lwfree(wkt);

	p.x = 5;
	p.y = 50;
	ptarray_insert_point(line->points, &p, 4);
	wkt = lwgeom_to_text(lwline_as_lwgeom(line));
	ASSERT_STRING_EQUAL(wkt,"LINESTRING(4 40,2 20,3 30,1 1,5 50)");
	lwfree(wkt);

	lwline_free(line);
}

static void test_ptarray_append_ptarray(void)
{
	LWLINE *line1, *line2;
	int ret;
	char *wkt;

	/* Empty first line */
	line1 = lwgeom_as_lwline(lwgeom_from_text("LINESTRING EMPTY"));
	line2 = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(0 0,0 10,5 5)"));
	ret = ptarray_append_ptarray(line1->points, line2->points, -1);
	CU_ASSERT(ret == LW_SUCCESS);
	wkt = lwgeom_to_text(lwline_as_lwgeom(line1));
	ASSERT_STRING_EQUAL(wkt, "LINESTRING(0 0,0 10,5 5)");
	lwfree(wkt);
	lwline_free(line2);
	lwline_free(line1);

	/* Empty second line */
	line1 = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(0 0, 5 5, 6 3)"));
	line2 = lwgeom_as_lwline(lwgeom_from_text("LINESTRING EMPTY"));
	ret = ptarray_append_ptarray(line1->points, line2->points, -1);
	CU_ASSERT(ret == LW_SUCCESS);
	wkt = lwgeom_to_text(lwline_as_lwgeom(line1));
	ASSERT_STRING_EQUAL(wkt, "LINESTRING(0 0,5 5,6 3)");
	lwfree(wkt);
	lwline_free(line2);
	lwline_free(line1);

	/* Both lines empty */
	line1 = lwgeom_as_lwline(lwgeom_from_text("LINESTRING EMPTY"));
	line2 = lwgeom_as_lwline(lwgeom_from_text("LINESTRING EMPTY"));
	ret = ptarray_append_ptarray(line1->points, line2->points, -1);
	CU_ASSERT(ret == LW_SUCCESS);
	wkt = lwgeom_to_text(lwline_as_lwgeom(line1));
	ASSERT_STRING_EQUAL(wkt, "LINESTRING EMPTY");
	lwfree(wkt);
	lwline_free(line2);
	lwline_free(line1);

	/* Sane sewing */
	line1 = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(10 4, 0 0,5 7)"));
	line2 = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(5 7,12 43, 42 15)"));
	ret = ptarray_append_ptarray(line1->points, line2->points, 0);
	CU_ASSERT(ret == LW_SUCCESS);
	wkt = lwgeom_to_text(lwline_as_lwgeom(line1));
	ASSERT_STRING_EQUAL(wkt, "LINESTRING(10 4,0 0,5 7,12 43,42 15)");
	lwfree(wkt);
	lwline_free(line2);
	lwline_free(line1);

	/* Untolerated sewing */
	line1 = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(10 4, 0 0,5 7)"));
	line2 = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(5.5 7,12 43, 42 15)"));
	ret = ptarray_append_ptarray(line1->points, line2->points, 0);
	CU_ASSERT(ret == LW_FAILURE);
	lwline_free(line2);
	lwline_free(line1);

	/* Tolerated sewing */
	line1 = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(10 4, 0 0,5 7)"));
	line2 = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(5.5 7,12 43, 42 15)"));
	ret = ptarray_append_ptarray(line1->points, line2->points, .7);
	CU_ASSERT(ret == LW_SUCCESS);
	wkt = lwgeom_to_text(lwline_as_lwgeom(line1));
	ASSERT_STRING_EQUAL(wkt, "LINESTRING(10 4,0 0,5 7,5.5 7,12 43,42 15)");
	lwfree(wkt);
	lwline_free(line2);
	lwline_free(line1);

	/* Check user input trust (creates non-simple line */
	line1 = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(0 0,0 10)"));
	line2 = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(0 0,0 10)"));
	ret = ptarray_append_ptarray(line1->points, line2->points, -1);
	CU_ASSERT(ret == LW_SUCCESS);
	wkt = lwgeom_to_text(lwline_as_lwgeom(line1));
	ASSERT_STRING_EQUAL(wkt, "LINESTRING(0 0,0 10,0 0,0 10)");
	lwfree(wkt);
	lwline_free(line2);
	lwline_free(line1);

	/* Mixed dimensionality is not allowed */
	line1 = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(0 10 0, 10 0 0)"));
	line2 = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(10 0,11 0)"));
	ret = ptarray_append_ptarray(line1->points, line2->points, -1);
	CU_ASSERT(ret == LW_FAILURE);
	lwline_free(line2);
	lwline_free(line1);

	/* Appending a read-only pointarray is allowed */
	line1 = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(0 10, 10 0)"));
	line2 = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(10 0,11 0)"));
	FLAGS_SET_READONLY(line2->points->flags, 1);
	ret = ptarray_append_ptarray(line1->points, line2->points, -1);
	CU_ASSERT(ret == LW_SUCCESS);
	wkt = lwgeom_to_text(lwline_as_lwgeom(line1));
	ASSERT_STRING_EQUAL(wkt, "LINESTRING(0 10,10 0,11 0)");
	lwfree(wkt);
	FLAGS_SET_READONLY(line2->points->flags, 0); /* for lwline_free */
	lwline_free(line2);
	lwline_free(line1);

	/* Appending to a read-only pointarray is forbidden */
	line1 = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(0 10, 10 0)"));
	line2 = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(10 0,11 0)"));
	FLAGS_SET_READONLY(line1->points->flags, 1);
	ret = ptarray_append_ptarray(line1->points, line2->points, -1);
	CU_ASSERT(ret == LW_FAILURE);
	lwline_free(line2);
	FLAGS_SET_READONLY(line1->points->flags, 0); /* for lwline_free */
	lwline_free(line1);

}

static void test_ptarray_locate_point(void)
{
	LWLINE *line;
	double loc, dist;
	POINT4D p, l;

	line = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(0 3,20 4)"));

	p = getPoint4d(line->points, 0);
	loc = ptarray_locate_point(line->points, &p, &dist, &l);
	CU_ASSERT_EQUAL(loc, 0);
	CU_ASSERT_EQUAL(dist, 0.0);

	p = getPoint4d(line->points, 1);
	loc = ptarray_locate_point(line->points, &p, &dist, &l);
	CU_ASSERT_EQUAL(loc, 1);
	CU_ASSERT_EQUAL(dist, 0.0);

	p.x = 21; p.y = 4;
	loc = ptarray_locate_point(line->points, &p, &dist, NULL);
	CU_ASSERT_EQUAL(loc, 1);
	CU_ASSERT_EQUAL(dist, 1.0);

	p.x = 0; p.y = 2;
	loc = ptarray_locate_point(line->points, &p, &dist, &l);
	CU_ASSERT_EQUAL(loc, 0);
	CU_ASSERT_EQUAL(dist, 1.0);

	lwline_free(line);
	line = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(0 0,20 0,40 0)"));

	p.x = 20; p.y = 0;
	loc = ptarray_locate_point(line->points, &p, &dist, &l);
	CU_ASSERT_EQUAL(loc, 0.5);
	CU_ASSERT_EQUAL(dist, 0.0);

	lwline_free(line);
	line = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(-40 0,0 0,20 0,40 0)"));

	p.x = 20; p.y = 0;
	loc = ptarray_locate_point(line->points, &p, &dist, &l);
	CU_ASSERT_EQUAL(loc, 0.75);
	CU_ASSERT_EQUAL(dist, 0.0);

	lwline_free(line);
	line = lwgeom_as_lwline(lwgeom_from_text("LINESTRING M (0 0 0, 10 0 20)"));

	p.x = 5; p.y = 0;
	loc = ptarray_locate_point(line->points, &p, &dist, &l);
	CU_ASSERT_EQUAL(loc, 0.5);
	CU_ASSERT_EQUAL(dist, 0.0);
	CU_ASSERT_EQUAL(l.m, 10.0);

	lwline_free(line);

}

static void test_ptarray_isccw(void)
{
	LWLINE *line;
	LWPOLY* poly;
	int ccw;

	/* clockwise rectangle */
	line = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(0 0,0 10,10 10,10 0, 0 0)"));
	ccw = ptarray_isccw(line->points);
	CU_ASSERT_EQUAL(ccw, 0);
	lwline_free(line);

	/* clockwise triangle */
	line = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(0 3,20 4,20 3, 0 3)"));
	ccw = ptarray_isccw(line->points);
	CU_ASSERT_EQUAL(ccw, 0);
	lwline_free(line);

	/* counterclockwise triangle */
	line = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(0 3,20 3,20 4, 0 3)"));
	ccw = ptarray_isccw(line->points);
	CU_ASSERT_EQUAL(ccw, 1);
	lwline_free(line);

	/* counterclockwise narrow ring (see ticket #1302) */
	line = lwgeom_as_lwline(lwgeom_from_hexwkb("01020000000500000000917E9BA468294100917E9B8AEA284137894120A4682941C976BE9F8AEA2841B39ABE1FA46829415ACCC29F8AEA2841C976BE1FA4682941C976BE9F8AEA284100917E9BA468294100917E9B8AEA2841", LW_PARSER_CHECK_NONE));
	ccw = ptarray_isccw(line->points);
	CU_ASSERT_EQUAL(ccw, 1);
	lwline_free(line);

	/* clockwise narrow ring (see ticket #1302) */
	line = lwgeom_as_lwline(lwgeom_from_hexwkb("01020000000500000000917E9BA468294100917E9B8AEA2841C976BE1FA4682941C976BE9F8AEA2841B39ABE1FA46829415ACCC29F8AEA284137894120A4682941C976BE9F8AEA284100917E9BA468294100917E9B8AEA2841", LW_PARSER_CHECK_NONE));
	ccw = ptarray_isccw(line->points);
	CU_ASSERT_EQUAL(ccw, 0);
	lwline_free(line);

	/* Clockwise narrow ring (see ticket #1302) */
	poly = lwgeom_as_lwpoly(lwgeom_from_hexwkb("0103000000010000000500000000917E9BA468294100917E9B8AEA2841C976BE1FA4682941C976BE9F8AEA2841B39ABE1FA46829415ACCC29F8AEA284137894120A4682941C976BE9F8AEA284100917E9BA468294100917E9B8AEA2841", LW_PARSER_CHECK_NONE));
	ccw = ptarray_isccw(poly->rings[0]);
	CU_ASSERT_EQUAL(ccw, 0);
	lwpoly_free(poly);
}

static void test_ptarray_signed_area()
{
	LWLINE *line;
	double area;

	/* parallelogram */
	line = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(0 0,1 1, 2 1, 1 0, 0 0)"));
	area = ptarray_signed_area(line->points);
	CU_ASSERT_DOUBLE_EQUAL(area, 1.0, 0.0000001);
	lwline_free(line);

	/* square */
	line = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(0 0,0 2, 2 2, 2 0, 0 0)"));
	area = ptarray_signed_area(line->points);
	CU_ASSERT_DOUBLE_EQUAL(area, 4.0, 0.0000001);
	lwline_free(line);

	/* square backwares*/
	line = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(0 0,2 0, 2 2, 0 2, 0 0)"));
	area = ptarray_signed_area(line->points);
	//printf("%g\n",area);
	CU_ASSERT_DOUBLE_EQUAL(area, -4.0, 0.0000001);
	lwline_free(line);

}

static void test_ptarray_contains_point()
{
/* int ptarray_contains_point(const POINTARRAY *pa, const POINT2D *pt, int *winding_number) */

	LWLINE *lwline;
	POINTARRAY *pa;
	POINT2D pt;
	int rv;

	lwline = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(0 0, 0 1, 1 1, 1 0, 0 0)"));
	pa = lwline->points;

	/* Point in middle of square */
	pt.x = 0.5;
	pt.y = 0.5;
	rv = ptarray_contains_point(pa, &pt);
	CU_ASSERT_EQUAL(rv, LW_INSIDE);

	/* Point on left edge of square */
	pt.x = 0;
	pt.y = 0.5;
	rv = ptarray_contains_point(pa, &pt);
	CU_ASSERT_EQUAL(rv, LW_BOUNDARY);

	/* Point on top edge of square */
	pt.x = 0.5;
	pt.y = 1;
	rv = ptarray_contains_point(pa, &pt);
	CU_ASSERT_EQUAL(rv, LW_BOUNDARY);

	/* Point on bottom left corner of square */
	pt.x = 0;
	pt.y = 0;
	rv = ptarray_contains_point(pa, &pt);
	CU_ASSERT_EQUAL(rv, LW_BOUNDARY);

	/* Point on top left corner of square */
	pt.x = 0;
	pt.y = 1;
	rv = ptarray_contains_point(pa, &pt);
	CU_ASSERT_EQUAL(rv, LW_BOUNDARY);

	/* Point outside top left corner of square */
	pt.x = -0.1;
	pt.y = 1;
	rv = ptarray_contains_point(pa, &pt);
	CU_ASSERT_EQUAL(rv, LW_OUTSIDE);

	/* Point outside top left corner of square */
	pt.x = 0;
	pt.y = 1.1;
	rv = ptarray_contains_point(pa, &pt);
	CU_ASSERT_EQUAL(rv, LW_OUTSIDE);

	/* Point outside left side of square */
	pt.x = -0.2;
	pt.y = 0.5;
	rv = ptarray_contains_point(pa, &pt);
	CU_ASSERT_EQUAL(rv, LW_OUTSIDE);

	lwline_free(lwline);
	lwline = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(0 0, 1 1, 2 0, 0 0)"));
	pa = lwline->points;

	/* Point outside grazing top of triangle */
	pt.x = 0;
	pt.y = 1;
	rv = ptarray_contains_point(pa, &pt);
	CU_ASSERT_EQUAL(rv, LW_OUTSIDE);

	lwline_free(lwline);
	lwline = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(0 0, 0 4, 1 4, 2 2, 3 4, 4 4, 4 0, 0 0)"));
	pa = lwline->points;

	/* Point outside grazing top of triangle */
	pt.x = 1;
	pt.y = 2;
	rv = ptarray_contains_point(pa, &pt);
	CU_ASSERT_EQUAL(rv, LW_INSIDE);

	/* Point outside grazing top of triangle */
	pt.x = 3;
	pt.y = 2;
	rv = ptarray_contains_point(pa, &pt);
	CU_ASSERT_EQUAL(rv, LW_INSIDE);

	lwline_free(lwline);
}

static void test_ptarrayarc_contains_point()
{
	/* int ptarrayarc_contains_point(const POINTARRAY *pa, const POINT2D *pt) */

	LWLINE *lwline;
	POINTARRAY *pa;
	POINT2D pt;
	int rv;

	/*** Collection of semi-circles surrounding unit square ***/
	lwline = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(-1 -1, -2 0, -1 1, 0 2, 1 1, 2 0, 1 -1, 0 -2, -1 -1)"));
	pa = lwline->points;

	/* Point in middle of square */
	pt.x = 0;
	pt.y = 0;
	rv = ptarrayarc_contains_point(pa, &pt);
	CU_ASSERT_EQUAL(rv, LW_INSIDE);

	/* Point in left lobe */
	pt.x = -1.1;
	pt.y = 0.1;
	rv = ptarrayarc_contains_point(pa, &pt);
	CU_ASSERT_EQUAL(rv, LW_INSIDE);

	/* Point on boundary of left lobe */
	pt.x = -1;
	pt.y = 0;
	rv = ptarrayarc_contains_point(pa, &pt);
	CU_ASSERT_EQUAL(rv, LW_INSIDE);

	/* Point on boundary vertex */
	pt.x = -1;
	pt.y = 1;
	rv = ptarrayarc_contains_point(pa, &pt);
	CU_ASSERT_EQUAL(rv, LW_BOUNDARY);

	/* Point outside */
	pt.x = -1.5;
	pt.y = 1.5;
	rv = ptarrayarc_contains_point(pa, &pt);
	CU_ASSERT_EQUAL(rv, LW_OUTSIDE);

	/*** Two-edge ring made up of semi-circles (really, a circle) ***/
	lwline_free(lwline);
	lwline = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(-1 0, 0 1, 1 0, 0 -1, -1 0)"));
	pa = lwline->points;

	/* Point outside */
	pt.x = -1.5;
	pt.y = 1.5;
	rv = ptarrayarc_contains_point(pa, &pt);
	CU_ASSERT_EQUAL(rv, LW_OUTSIDE);

	/* Point more outside */
	pt.x = 2.5;
	pt.y = 1.5;
	rv = ptarrayarc_contains_point(pa, &pt);
	CU_ASSERT_EQUAL(rv, LW_OUTSIDE);

	/* Point more outside */
	pt.x = 2.5;
	pt.y = 2.5;
	rv = ptarrayarc_contains_point(pa, &pt);
	CU_ASSERT_EQUAL(rv, LW_OUTSIDE);

	/* Point inside at middle */
	pt.x = 0;
	pt.y = 0;
	rv = ptarrayarc_contains_point(pa, &pt);
	CU_ASSERT_EQUAL(rv, LW_INSIDE);

	/* Point inside offset from middle */
	pt.x = 0.01;
	pt.y = 0.01;
	rv = ptarrayarc_contains_point(pa, &pt);
	CU_ASSERT_EQUAL(rv, LW_INSIDE);

	/* Point on edge vertex */
	pt.x = 0;
	pt.y = 1;
	rv = ptarrayarc_contains_point(pa, &pt);
	CU_ASSERT_EQUAL(rv, LW_BOUNDARY);

	/*** Two-edge ring, closed ***/
	lwline_free(lwline);
	lwline = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(1 6, 6 1, 9 7, 6 10, 1 6)"));
	pa = lwline->points;

	/* Point to left of ring */
	pt.x = 20;
	pt.y = 4;
	rv = ptarrayarc_contains_point(pa, &pt);
	CU_ASSERT_EQUAL(rv, LW_OUTSIDE);

	/*** One-edge ring, closed circle ***/
	lwline_free(lwline);
	lwline = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(-1 0, 1 0, -1 0)"));
	pa = lwline->points;

	/* Point inside */
	pt.x = 0;
	pt.y = 0;
	rv = ptarrayarc_contains_point(pa, &pt);
	CU_ASSERT_EQUAL(rv, LW_INSIDE);

	/* Point outside */
	pt.x = 0;
	pt.y = 2;
	rv = ptarrayarc_contains_point(pa, &pt);
	CU_ASSERT_EQUAL(rv, LW_OUTSIDE);

	/* Point on boundary */
	pt.x = 0;
	pt.y = 1;
	rv = ptarrayarc_contains_point(pa, &pt);
	CU_ASSERT_EQUAL(rv, LW_BOUNDARY);

	/*** Overshort ring ***/
	lwline_free(lwline);
	lwline = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(-1 0, 1 0)"));
	pa = lwline->points;
	cu_error_msg_reset();
	rv = ptarrayarc_contains_point(pa, &pt);
	//printf("%s\n", cu_error_msg);
	ASSERT_STRING_EQUAL("ptarrayarc_contains_point called with even number of points", cu_error_msg);

	/*** Unclosed ring ***/
	lwline_free(lwline);
	lwline = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(-1 0, 1 0, 2 0)"));
	pa = lwline->points;
	cu_error_msg_reset();
	rv = ptarrayarc_contains_point(pa, &pt);
	ASSERT_STRING_EQUAL("ptarrayarc_contains_point called on unclosed ring", cu_error_msg);

	lwline_free(lwline);
}

static void test_ptarray_scale()
{
  LWLINE *line;
  POINTARRAY *pa;
  POINT4D factor;
  const char *wkt;
  char *wktout;

  wkt = "LINESTRING ZM (0 1 2 3,1 2 3 0,-2 -3 0 -1,-3 0 -1 -2)";
  line = lwgeom_as_lwline(lwgeom_from_text(wkt));
  pa = line->points;

  factor.x = factor.y = factor.z = factor.m = 1;
  ptarray_scale(pa, &factor);
  wktout = lwgeom_to_text(lwline_as_lwgeom(line));
  ASSERT_STRING_EQUAL(wktout, wkt);
  lwfree(wktout);

  factor.x = 2;
  wkt = "LINESTRING ZM (0 1 2 3,2 2 3 0,-4 -3 0 -1,-6 0 -1 -2)";
  ptarray_scale(pa, &factor);
  wktout = lwgeom_to_text(lwline_as_lwgeom(line));
  ASSERT_STRING_EQUAL(wktout, wkt);
  lwfree(wktout);

  factor.x = 1; factor.y = 3;
  wkt = "LINESTRING ZM (0 3 2 3,2 6 3 0,-4 -9 0 -1,-6 0 -1 -2)";
  ptarray_scale(pa, &factor);
  wktout = lwgeom_to_text(lwline_as_lwgeom(line));
  ASSERT_STRING_EQUAL(wktout, wkt);
  lwfree(wktout);

  factor.x = 1; factor.y = 1; factor.z = -2;
  wkt = "LINESTRING ZM (0 3 -4 3,2 6 -6 0,-4 -9 0 -1,-6 0 2 -2)";
  ptarray_scale(pa, &factor);
  wktout = lwgeom_to_text(lwline_as_lwgeom(line));
  ASSERT_STRING_EQUAL(wktout, wkt);
  lwfree(wktout);

  factor.x = 1; factor.y = 1; factor.z = 1; factor.m = 2;
  wkt = "LINESTRING ZM (0 3 -4 6,2 6 -6 0,-4 -9 0 -2,-6 0 2 -4)";
  ptarray_scale(pa, &factor);
  wktout = lwgeom_to_text(lwline_as_lwgeom(line));
  ASSERT_STRING_EQUAL(wktout, wkt);
  lwfree(wktout);

  lwline_free(line);
}


/*
** Used by the test harness to register the tests in this file.
*/
void ptarray_suite_setup(void);
void ptarray_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("ptarray", NULL, NULL);
	PG_ADD_TEST(suite, test_ptarray_append_point);
	PG_ADD_TEST(suite, test_ptarray_append_ptarray);
	PG_ADD_TEST(suite, test_ptarray_locate_point);
	PG_ADD_TEST(suite, test_ptarray_isccw);
	PG_ADD_TEST(suite, test_ptarray_signed_area);
	PG_ADD_TEST(suite, test_ptarray_insert_point);
	PG_ADD_TEST(suite, test_ptarray_contains_point);
	PG_ADD_TEST(suite, test_ptarrayarc_contains_point);
	PG_ADD_TEST(suite, test_ptarray_scale);
}
