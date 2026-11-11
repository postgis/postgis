/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright (C) 2011 Sandro Santilli <strk@kbt.io>
 * Copyright (C) 2008 Paul Ramsey
 * Copyright (C) 2026 Darafei Praliaskouski <me@komzpa.net>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
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

static void
test_ptarray_substring_3d(void)
{
	LWLINE *line;
	LWLINE *subline;
	POINTARRAY *subpa;
	POINT4D pt;
	char *wkt;

	line = lwgeom_as_lwline(lwgeom_from_text("LINESTRING Z (0 0 0,0 2 5,0 10 10)"));
	subpa = ptarray_substring_3d(line->points, 0.0, 0.5, 0.0);
	subline = lwline_construct(line->srid, NULL, subpa);
	wkt = lwgeom_to_text(lwline_as_lwgeom(subline));
	ASSERT_STRING_EQUAL(wkt, "LINESTRING Z (0 0 0,0 2 5,0 3.71669469 6.07293418)");
	lwfree(wkt);
	lwline_free(subline);
	lwline_free(line);

	line = lwgeom_as_lwline(lwgeom_from_text("LINESTRING ZM (0 0 0 0,0 0 4 8)"));
	subpa = ptarray_substring_3d(line->points, 0.25, 0.75, 0.0);
	subline = lwline_construct(line->srid, NULL, subpa);
	wkt = lwgeom_to_text(lwline_as_lwgeom(subline));
	ASSERT_STRING_EQUAL(wkt, "LINESTRING ZM (0 0 1 2,0 0 3 6)");
	lwfree(wkt);
	lwline_free(subline);
	lwline_free(line);

	line = lwgeom_as_lwline(lwgeom_from_text("LINESTRING (0 0,0 4)"));
	subpa = ptarray_substring_3d(line->points, 0.25, 0.75, 0.0);
	subline = lwline_construct(line->srid, NULL, subpa);
	wkt = lwgeom_to_text(lwline_as_lwgeom(subline));
	ASSERT_STRING_EQUAL(wkt, "LINESTRING(0 1,0 3)");
	lwfree(wkt);
	lwline_free(subline);
	lwline_free(line);

	line = lwgeom_as_lwline(lwgeom_from_text("LINESTRING Z (0 0 0,0 0 1,0 0 2)"));
	subpa = ptarray_substring_3d(line->points, 0.5, 0.5, 0.0);
	CU_ASSERT_EQUAL(subpa->npoints, 1);
	getPoint4d_p(subpa, 0, &pt);
	CU_ASSERT_DOUBLE_EQUAL(pt.x, 0.0, 0.0);
	CU_ASSERT_DOUBLE_EQUAL(pt.y, 0.0, 0.0);
	CU_ASSERT_DOUBLE_EQUAL(pt.z, 1.0, 0.0);
	ptarray_free(subpa);
	lwline_free(line);

	line = lwgeom_as_lwline(lwgeom_from_text("LINESTRING ZM (0 0 0 0,1 0 0 1,1 0 0 5)"));
	subpa = ptarray_substring_3d(line->points, 1.0, 1.0, 0.0);
	CU_ASSERT_EQUAL(subpa->npoints, 1);
	getPoint4d_p(subpa, 0, &pt);
	CU_ASSERT_DOUBLE_EQUAL(pt.x, 1.0, 0.0);
	CU_ASSERT_DOUBLE_EQUAL(pt.y, 0.0, 0.0);
	CU_ASSERT_DOUBLE_EQUAL(pt.z, 0.0, 0.0);
	CU_ASSERT_DOUBLE_EQUAL(pt.m, 5.0, 0.0);
	ptarray_free(subpa);
	lwline_free(line);

	line = lwgeom_as_lwline(lwgeom_from_text("LINESTRING ZM (0 0 0 0,0 0 0 5,0 0 1 6)"));
	subpa = ptarray_substring_3d(line->points, 0.0, 1.0, 0.0);
	subline = lwline_construct(line->srid, NULL, subpa);
	wkt = lwgeom_to_text(lwline_as_lwgeom(subline));
	ASSERT_STRING_EQUAL(wkt, "LINESTRING ZM (0 0 0 0,0 0 0 5,0 0 1 6)");
	lwfree(wkt);
	lwline_free(subline);
	lwline_free(line);

	line = lwgeom_as_lwline(lwgeom_from_text("LINESTRING Z (0 0 0,0 0 1)"));
	subpa = ptarray_substring(line->points, 1.0, 1.0, 0.0);
	CU_ASSERT_EQUAL(subpa->npoints, 1);
	getPoint4d_p(subpa, 0, &pt);
	CU_ASSERT_DOUBLE_EQUAL(pt.x, 0.0, 0.0);
	CU_ASSERT_DOUBLE_EQUAL(pt.y, 0.0, 0.0);
	CU_ASSERT_DOUBLE_EQUAL(pt.z, 1.0, 0.0);
	ptarray_free(subpa);
	lwline_free(line);
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

static void test_ptarray_signed_area(void)
{
	LWLINE *line;
	double area;

	/* parallelogram */
	line = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(0 0,1 1, 2 1, 1 0, 0 0)"));
	area = ptarray_signed_area(line->points);
	ASSERT_DOUBLE_EQUAL_TOLERANCE(area, 1.0, 0.0000001);
	lwline_free(line);

	/* square */
	line = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(0 0,0 2, 2 2, 2 0, 0 0)"));
	area = ptarray_signed_area(line->points);
	ASSERT_DOUBLE_EQUAL_TOLERANCE(area, 4.0, 0.0000001);
	lwline_free(line);

	/* square backwards */
	line = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(0 0,2 0, 2 2, 0 2, 0 0)"));
	area = ptarray_signed_area(line->points);
	//printf("%g\n",area);
	ASSERT_DOUBLE_EQUAL_TOLERANCE(area, -4.0, 0.0000001);
	lwline_free(line);

	/* Preserve small residuals through cancellation in the area sum. */
	line = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(0 0,10000000000000000 0,1 -1,10000000000000000 -1,0 0)"));
	area = ptarray_signed_area(line->points);
	ASSERT_DOUBLE_EQUAL_TOLERANCE(area, 0.5, 0.0000001);
	lwline_free(line);

	/* Preserve determinant tails when later large determinants cancel. */
	line = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(0 0,10000000000000000 1,1 1,10000000000000000 0,0 0)"));
	area = ptarray_signed_area(line->points);
	ASSERT_DOUBLE_EQUAL_TOLERANCE(area, 0.5, 0.0000001);
	CU_ASSERT_EQUAL(ptarray_isccw(line->points), 0);
	lwline_free(line);

	/* Preserve residual tails when large product tails cancel. */
	line =
	    lwgeom_as_lwline(lwgeom_from_text("LINESTRING(1.9029580275885475e-13 2.1370106480605854e-89,"
					      "2.5163957753386382e-93 -3002966181.933756,"
					      "4.77621563620871e-75 -1e-100,"
					      "-3.7953949389104665e74 -1e200,"
					      "-2.0800734986321869e-60 2.051992028072436e-55,"
					      "3.38586824330807e73 -7.962294473171359e74,"
					      "1.9029580275885475e-13 2.1370106480605854e-89)"));
	area = ptarray_signed_area(line->points);
	CU_ASSERT(area > 1e140);
	CU_ASSERT(area < 2e140);
	CU_ASSERT_EQUAL(ptarray_isccw(line->points), 0);
	lwline_free(line);

	line =
	    lwgeom_as_lwline(lwgeom_from_text("LINESTRING(1.9029580275885475e-13 2.1370106480605854e-89,"
					      "3.38586824330807e73 -7.962294473171359e74,"
					      "-2.0800734986321869e-60 2.051992028072436e-55,"
					      "-3.7953949389104665e74 -1e200,"
					      "4.77621563620871e-75 -1e-100,"
					      "2.5163957753386382e-93 -3002966181.933756,"
					      "1.9029580275885475e-13 2.1370106480605854e-89)"));
	area = ptarray_signed_area(line->points);
	CU_ASSERT(area < -1e140);
	CU_ASSERT(area > -2e140);
	CU_ASSERT_EQUAL(ptarray_isccw(line->points), 1);
	lwline_free(line);

	/* Preserve residual tails when scaled large product tails cancel. */
	line =
	    lwgeom_as_lwline(lwgeom_from_text("LINESTRING(1.9029580275885475e47 2.1370106480605854e-29,"
					      "2.5163957753386382e-33 -3.002966181933756e69,"
					      "4.77621563620871e-15 -1e-40,"
					      "-3.7953949389104665e134 -1e260,"
					      "-2.0800734986321869 2.051992028072436e5,"
					      "3.38586824330807e133 -7.962294473171359e134,"
					      "1.9029580275885475e47 2.1370106480605854e-29)"));
	area = ptarray_signed_area(line->points);
	CU_ASSERT(area > 1e260);
	CU_ASSERT(area < 2e260);
	CU_ASSERT_EQUAL(ptarray_isccw(line->points), 0);
	lwline_free(line);

	line =
	    lwgeom_as_lwline(lwgeom_from_text("LINESTRING(1.9029580275885475e47 2.1370106480605854e-29,"
					      "3.38586824330807e133 -7.962294473171359e134,"
					      "-2.0800734986321869 2.051992028072436e5,"
					      "-3.7953949389104665e134 -1e260,"
					      "4.77621563620871e-15 -1e-40,"
					      "2.5163957753386382e-33 -3.002966181933756e69,"
					      "1.9029580275885475e47 2.1370106480605854e-29)"));
	area = ptarray_signed_area(line->points);
	CU_ASSERT(area < -1e260);
	CU_ASSERT(area > -2e260);
	CU_ASSERT_EQUAL(ptarray_isccw(line->points), 1);
	lwline_free(line);

	/* Preserve product tails when determinant products nearly cancel. */
	line =
	    lwgeom_as_lwline(lwgeom_from_text("LINESTRING(10000000000000276 10000000000000280,"
					      "59999999999999680 59999999999999672,"
					      "89999999999999360 89999999999999360,"
					      "90000000000000112 90000000000000128,"
					      "90000000000000704 90000000000000704,"
					      "10000000000000276 10000000000000280)"));
	area = ptarray_signed_area(line->points);
	CU_ASSERT(area < 0.0);
	CU_ASSERT_EQUAL(ptarray_isccw(line->points), 1);
	lwline_free(line);

	/* Preserve subtraction tails when the origin shift hides unit-scale offsets. */
	line =
	    lwgeom_as_lwline(lwgeom_from_text("LINESTRING(-10000000000000000 -10000000000000000,"
					      "1 -1,1 0,1 1,-1 0,"
					      "-10000000000000000 -10000000000000000)"));
	area = ptarray_signed_area(line->points);
	CU_ASSERT(area < -1e16);
	CU_ASSERT(area > -2e16);
	CU_ASSERT_EQUAL(ptarray_isccw(line->points), 1);
	lwline_free(line);

	/* Overflowed determinants should remain infinite, not poison the sum with NaN. */
	line = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(0 0,0 1e200,1e200 1e200,1e200 0,0 0)"));
	area = ptarray_signed_area(line->points);
	CU_ASSERT(isinf(area) && area > 0);
	CU_ASSERT_EQUAL(ptarray_isccw(line->points), 0);
	lwline_free(line);

	/* Rescale overflowing fan products when the determinant itself is finite. */
	line =
	    lwgeom_as_lwline(lwgeom_from_text("LINESTRING(0 0,1 1e155,1e155 1e155,"
					      "1e155 1.0000000000000001e155,"
					      "1 1.0000000000000001e155,0 0)"));
	area = ptarray_signed_area(line->points);
	CU_ASSERT(isfinite(area));
	CU_ASSERT(area < -1e294);
	CU_ASSERT(area > -2e294);
	CU_ASSERT_EQUAL(ptarray_isccw(line->points), 1);
	lwline_free(line);

	line =
	    lwgeom_as_lwline(lwgeom_from_text("LINESTRING(0 0,1 1.0000000000000001e155,"
					      "1e155 1.0000000000000001e155,"
					      "1e155 1e155,1 1e155,0 0)"));
	area = ptarray_signed_area(line->points);
	CU_ASSERT(isfinite(area));
	CU_ASSERT(area > 1e294);
	CU_ASSERT(area < 2e294);
	CU_ASSERT_EQUAL(ptarray_isccw(line->points), 0);
	lwline_free(line);

	/* Preserve tiny finite products beside unrelated huge coordinates. */
	line = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(0 0,1e-200 0,1e300 1e300,0 0)"));
	area = ptarray_signed_area(line->points);
	CU_ASSERT(area < -4e99);
	CU_ASSERT(area > -6e99);
	CU_ASSERT_EQUAL(ptarray_isccw(line->points), 1);
	lwline_free(line);

	line = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(0 0,1e300 1e300,1e-200 0,0 0)"));
	area = ptarray_signed_area(line->points);
	CU_ASSERT(area > 4e99);
	CU_ASSERT(area < 6e99);
	CU_ASSERT_EQUAL(ptarray_isccw(line->points), 0);
	lwline_free(line);

	/* Detect residual tails after scaled determinant cancellation. */
	line =
	    lwgeom_as_lwline(lwgeom_from_text("LINESTRING(3e120 1.0000000000000002e220,"
					      "3e155 -1e-50,3e155 -1.0000000000000003e140,"
					      "3e120 1.0000000000000002e220)"));
	area = ptarray_signed_area(line->points);
	CU_ASSERT(isfinite(area));
	CU_ASSERT(area > 1e295);
	CU_ASSERT(area < 2e295);
	CU_ASSERT_EQUAL(ptarray_isccw(line->points), 0);
	lwline_free(line);

	line =
	    lwgeom_as_lwline(lwgeom_from_text("LINESTRING(3e120 1.0000000000000002e220,"
					      "3e155 -1.0000000000000003e140,3e155 -1e-50,"
					      "3e120 1.0000000000000002e220)"));
	area = ptarray_signed_area(line->points);
	CU_ASSERT(isfinite(area));
	CU_ASSERT(area < -1e295);
	CU_ASSERT(area > -2e295);
	CU_ASSERT_EQUAL(ptarray_isccw(line->points), 1);
	lwline_free(line);
}

static void test_ptarray_contains_point(void)
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


	/* Test for https://trac.osgeo.org/postgis/ticket/6023 */
	lwline = lwgeom_as_lwline(lwgeom_from_text("LINESTRING(11.230120879533454 62.84897119848748,11.230120879533905 62.8489711984873,11.23020501303477 62.84900750109812,11.230170431987244 62.84904481447776,11.230117909393426 62.8489943480894,11.230120879533454 62.84897119848748)"));
	pa = lwline->points;
	pt = getPoint2d(lwline->points, 0);
	rv = ptarray_contains_point(pa, &pt);
	ASSERT_INT_EQUAL(rv, LW_BOUNDARY);


	lwline_free(lwline);
}

static void test_ptarrayarc_contains_point(void)
{
	/* int ptarrayarc_contains_point(const POINTARRAY *pa, const POINT2D *pt) */

	LWCIRCSTRING *lwcirc;
	POINTARRAY *pa;
	POINT2D pt;
	int rv;

	/*** Collection of semi-circles surrounding unit square ***/
	lwcirc = lwgeom_as_lwcircstring(lwgeom_from_text("CIRCULARSTRING(-1 -1, -2 0, -1 1, 0 2, 1 1, 2 0, 1 -1, 0 -2, -1 -1)"));
	pa = lwcirc->points;

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
	lwcircstring_free(lwcirc);
	lwcirc = lwgeom_as_lwcircstring(lwgeom_from_text("CIRCULARSTRING(-1 0, 0 1, 1 0, 0 -1, -1 0)"));
	pa = lwcirc->points;

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
	lwcircstring_free(lwcirc);
	lwcirc = lwgeom_as_lwcircstring(lwgeom_from_text("CIRCULARSTRING(1 6, 6 1, 9 7, 6 10, 1 6)"));
	pa = lwcirc->points;

	/* Point to left of ring */
	pt.x = 20;
	pt.y = 4;
	rv = ptarrayarc_contains_point(pa, &pt);
	CU_ASSERT_EQUAL(rv, LW_OUTSIDE);

	/*** One-edge ring, closed circle ***/
	lwcircstring_free(lwcirc);
	lwcirc = lwgeom_as_lwcircstring(lwgeom_from_text("CIRCULARSTRING(-1 0, 1 0, -1 0)"));
	pa = lwcirc->points;

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
	lwcircstring_free(lwcirc);
	lwcirc = lwgeom_as_lwcircstring(lwgeom_from_text("CIRCULARSTRING(-1 0, 1 0)"));
	pa = lwcirc->points;
	cu_error_msg_reset();
	rv = ptarrayarc_contains_point(pa, &pt);
	//printf("%s\n", cu_error_msg);
	ASSERT_STRING_EQUAL(cu_error_msg, "ptarrayarc_raycast_intersections called with even number of points");

	/*** Unclosed ring ***/
	lwcircstring_free(lwcirc);
	lwcirc = lwgeom_as_lwcircstring(lwgeom_from_text("CIRCULARSTRING(-1 0, 1 0, 2 0)"));
	pa = lwcirc->points;
	cu_error_msg_reset();
	rv = ptarrayarc_contains_point(pa, &pt);
	ASSERT_STRING_EQUAL(cu_error_msg, "ptarrayarc_contains_point called on unclosed ring");

	lwcircstring_free(lwcirc);
}

static void test_ptarray_scale(void)
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

static void test_ptarray_scroll(void)
{
  LWLINE *line;
  POINTARRAY *pa;
  POINT4D scroll;
  const char *wkt;
  char *wktout;
	int rv;

  wkt = "LINESTRING ZM (1 1 1 1,2 2 2 2,3 3 3 3,4 4 4 4,1 1 1 1)";
  line = lwgeom_as_lwline(lwgeom_from_text(wkt));
  pa = line->points;

	scroll.x = scroll.y = scroll.z = scroll.m = 2;
  rv = ptarray_scroll_in_place(pa, &scroll);
	CU_ASSERT_EQUAL(rv, LW_SUCCESS);
  wktout = lwgeom_to_text(lwline_as_lwgeom(line));
  wkt = "LINESTRING ZM (2 2 2 2,3 3 3 3,4 4 4 4,1 1 1 1,2 2 2 2)";
  ASSERT_STRING_EQUAL(wktout, wkt);
  lwfree(wktout);

	scroll.x = scroll.y = scroll.z = scroll.m = 1;
  rv = ptarray_scroll_in_place(pa, &scroll);
	CU_ASSERT_EQUAL(rv, LW_SUCCESS);
  wktout = lwgeom_to_text(lwline_as_lwgeom(line));
  wkt = "LINESTRING ZM (1 1 1 1,2 2 2 2,3 3 3 3,4 4 4 4,1 1 1 1)";
  ASSERT_STRING_EQUAL(wktout, wkt);
  lwfree(wktout);

	scroll.x = scroll.y = scroll.z = scroll.m = 9;
  rv = ptarray_scroll_in_place(pa, &scroll);
	CU_ASSERT_EQUAL(rv, LW_FAILURE);
	ASSERT_STRING_EQUAL(cu_error_msg, "ptarray_scroll_in_place: input POINTARRAY does not contain the given point");

  lwline_free(line);
}

static void test_ptarray_closest_vertex_2d(void)
{
	LWLINE *line;
	POINTARRAY *pa;
	double dist;
	POINT2D qp;
	const char *wkt;
	int rv;

	wkt = "LINESTRING (0 0 0, 1 0 0, 2 0 0, 3 0 10)";
	line = lwgeom_as_lwline(lwgeom_from_text(wkt));
	pa = line->points;

	qp.x = qp.y = 0;
	rv = ptarray_closest_vertex_2d(pa, &qp, &dist);
	ASSERT_INT_EQUAL(rv, 0);
	ASSERT_DOUBLE_EQUAL(dist, 0);

	qp.x = qp.y = 1;
	rv = ptarray_closest_vertex_2d(pa, &qp, &dist);
	ASSERT_INT_EQUAL(rv, 1);
	ASSERT_DOUBLE_EQUAL(dist, 1);

	qp.x = 5; qp.y = 0;
	rv = ptarray_closest_vertex_2d(pa, &qp, &dist);
	ASSERT_INT_EQUAL(rv, 3);
	ASSERT_DOUBLE_EQUAL(dist, 2);


  lwline_free(line);
}

static void test_ptarray_closest_segment_2d(void)
{
	LWLINE *line;
	POINTARRAY *pa;
	double dist;
	POINT2D qp;
	const char *wkt;
	int rv;

	wkt = "LINESTRING (0 0 0, 1 0 0, 2 0 0, 3 0 10)";
	line = lwgeom_as_lwline(lwgeom_from_text(wkt));
	pa = line->points;

	qp.x = qp.y = 0;
	rv = ptarray_closest_segment_2d(pa, &qp, &dist);
	ASSERT_INT_EQUAL(rv, 0);
	ASSERT_DOUBLE_EQUAL(dist, 0);

	qp.x = 1;
	rv = ptarray_closest_segment_2d(pa, &qp, &dist);
	ASSERT_INT_EQUAL(rv, 0);
	ASSERT_DOUBLE_EQUAL(dist, 0);

	qp.y = 1;
	rv = ptarray_closest_segment_2d(pa, &qp, &dist);
	ASSERT_INT_EQUAL(rv, 0);
	ASSERT_DOUBLE_EQUAL(dist, 1);

	qp.x = 5; qp.y = 0;
	rv = ptarray_closest_segment_2d(pa, &qp, &dist);
	ASSERT_INT_EQUAL(rv, 2);
	ASSERT_DOUBLE_EQUAL(dist, 2);


	lwline_free(line);

	/* See https://trac.osgeo.org/postgis/ticket/4990 */
	/* Test modified to give more stable results */
	wkt = "LINESTRING(4 31,7 31,7 34,4 34,4 31)";
	line = lwgeom_as_lwline(lwgeom_from_text(wkt));
	pa = line->points;
	qp.x = 7.1; qp.y = 31.1;
	rv = ptarray_closest_segment_2d(pa, &qp, &dist);
	ASSERT_INT_EQUAL(rv, 1);
	lwline_free(line);
}

static void test_ptarray_closest_point_on_segment(void)
{
	POINT4D s0, s1, qp, cp;

	s0.x = s0.y = 0; s0.z = 10; s0.m = 20;
	s1.x = 0; s1.y = 10; s1.z = 0; s1.m = 10;

	/* Closest is bottom point */

	qp.x = -0.1; qp.y = 0;
	closest_point_on_segment(&qp, &s0, &s1, &cp);
	ASSERT_DOUBLE_EQUAL(cp.x, 0);
	ASSERT_DOUBLE_EQUAL(cp.y, 0);
	ASSERT_DOUBLE_EQUAL(cp.z, 10);
	ASSERT_DOUBLE_EQUAL(cp.m, 20);

	qp.x = 0.1; qp.y = 0;
	closest_point_on_segment(&qp, &s0, &s1, &cp);
	ASSERT_DOUBLE_EQUAL(cp.x, 0);
	ASSERT_DOUBLE_EQUAL(cp.y, 0);
	ASSERT_DOUBLE_EQUAL(cp.z, 10);
	ASSERT_DOUBLE_EQUAL(cp.m, 20);

	qp.x = 0; qp.y = -0.1;
	closest_point_on_segment(&qp, &s0, &s1, &cp);
	ASSERT_DOUBLE_EQUAL(cp.x, 0);
	ASSERT_DOUBLE_EQUAL(cp.y, 0);
	ASSERT_DOUBLE_EQUAL(cp.z, 10);
	ASSERT_DOUBLE_EQUAL(cp.m, 20);

	/* Closest is top point */

	qp.x = 0; qp.y = 10.1;
	closest_point_on_segment(&qp, &s0, &s1, &cp);
	ASSERT_DOUBLE_EQUAL(cp.x, 0);
	ASSERT_DOUBLE_EQUAL(cp.y, 10);
	ASSERT_DOUBLE_EQUAL(cp.z, 0);
	ASSERT_DOUBLE_EQUAL(cp.m, 10);

	qp.x = 0.1; qp.y = 10;
	closest_point_on_segment(&qp, &s0, &s1, &cp);
	ASSERT_DOUBLE_EQUAL(cp.x, 0);
	ASSERT_DOUBLE_EQUAL(cp.y, 10);
	ASSERT_DOUBLE_EQUAL(cp.z, 0);
	ASSERT_DOUBLE_EQUAL(cp.m, 10);

	qp.x = -0.1; qp.y = 10;
	closest_point_on_segment(&qp, &s0, &s1, &cp);
	ASSERT_DOUBLE_EQUAL(cp.x, 0);
	ASSERT_DOUBLE_EQUAL(cp.y, 10);
	ASSERT_DOUBLE_EQUAL(cp.z, 0);
	ASSERT_DOUBLE_EQUAL(cp.m, 10);

	/* Closest is mid point */

	qp.x = 0.1; qp.y = 5;
	closest_point_on_segment(&qp, &s0, &s1, &cp);
	ASSERT_DOUBLE_EQUAL(cp.x, 0);
	ASSERT_DOUBLE_EQUAL(cp.y, 5);
	ASSERT_DOUBLE_EQUAL(cp.z, 5);
	ASSERT_DOUBLE_EQUAL(cp.m, 15);

	qp.x = -0.1; qp.y = 5;
	closest_point_on_segment(&qp, &s0, &s1, &cp);
	ASSERT_DOUBLE_EQUAL(cp.x, 0);
	ASSERT_DOUBLE_EQUAL(cp.y, 5);
	ASSERT_DOUBLE_EQUAL(cp.z, 5);
	ASSERT_DOUBLE_EQUAL(cp.m, 15);

	qp.x = 0.1; qp.y = 2;
	closest_point_on_segment(&qp, &s0, &s1, &cp);
	ASSERT_DOUBLE_EQUAL(cp.x, 0);
	ASSERT_DOUBLE_EQUAL(cp.y, 2);
	ASSERT_DOUBLE_EQUAL(cp.z, 8);
	ASSERT_DOUBLE_EQUAL(cp.m, 18);

	qp.x = -0.1; qp.y = 2;
	closest_point_on_segment(&qp, &s0, &s1, &cp);
	ASSERT_DOUBLE_EQUAL(cp.x, 0);
	ASSERT_DOUBLE_EQUAL(cp.y, 2);
	ASSERT_DOUBLE_EQUAL(cp.z, 8);
	ASSERT_DOUBLE_EQUAL(cp.m, 18);

	qp.x = 0.1; qp.y = 8;
	closest_point_on_segment(&qp, &s0, &s1, &cp);
	ASSERT_DOUBLE_EQUAL(cp.x, 0);
	ASSERT_DOUBLE_EQUAL(cp.y, 8);
	ASSERT_DOUBLE_EQUAL_TOLERANCE(cp.z, 2, 1e-5);
	ASSERT_DOUBLE_EQUAL(cp.m, 12);

	qp.x = -0.1; qp.y = 8;
	closest_point_on_segment(&qp, &s0, &s1, &cp);
	ASSERT_DOUBLE_EQUAL(cp.x, 0);
	ASSERT_DOUBLE_EQUAL(cp.y, 8);
	ASSERT_DOUBLE_EQUAL_TOLERANCE(cp.z, 2, 1e-5);
	ASSERT_DOUBLE_EQUAL(cp.m, 12);


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
	PG_ADD_TEST(suite, test_ptarray_substring_3d);
	PG_ADD_TEST(suite, test_ptarray_locate_point);
	PG_ADD_TEST(suite, test_ptarray_isccw);
	PG_ADD_TEST(suite, test_ptarray_signed_area);
	PG_ADD_TEST(suite, test_ptarray_insert_point);
	PG_ADD_TEST(suite, test_ptarray_contains_point);
	PG_ADD_TEST(suite, test_ptarrayarc_contains_point);
	PG_ADD_TEST(suite, test_ptarray_scale);
	PG_ADD_TEST(suite, test_ptarray_scroll);
	PG_ADD_TEST(suite, test_ptarray_closest_vertex_2d);
	PG_ADD_TEST(suite, test_ptarray_closest_segment_2d);
	PG_ADD_TEST(suite, test_ptarray_closest_point_on_segment);
}
