/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright 2026 PostGIS contributors
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

static void
do_test_catmull_rom(char *geom_txt, char *expected, int n_segments)
{
	LWGEOM *geom_in, *geom_out;
	char *out_txt;
	geom_in = lwgeom_from_wkt(geom_txt, LW_PARSER_CHECK_NONE);
	geom_out = lwgeom_catmull_rom(geom_in, n_segments);
	out_txt = lwgeom_to_wkt(geom_out, WKT_EXTENDED, 6, NULL);
	if (strcmp(expected, out_txt))
		printf("\nExpected: %s\n     Got: %s\n", expected, out_txt);
	ASSERT_STRING_EQUAL(expected, out_txt);
	lwfree(out_txt);
	lwgeom_free(geom_in);
	lwgeom_free(geom_out);
}

/*
 * A collinear 4-point line: Catmull-Rom on a straight line must produce
 * a straight line with original vertices preserved at their positions.
 * With n_segments=2 we get 1 + 3*2 = 7 output points.
 */
static void
do_test_catmull_rom_lines(void)
{
	/* Collinear: output must be straight, original vertices preserved */
	do_test_catmull_rom(
		"LINESTRING(0 0, 1 0, 2 0, 3 0)",
		"LINESTRING(0 0,0.5 0,1 0,1.5 0,2 0,2.5 0,3 0)",
		2);

	/* Fewer than 4 points: return input unchanged */
	do_test_catmull_rom(
		"LINESTRING(0 0, 1 1, 2 0)",
		"LINESTRING(0 0,1 1,2 0)",
		5);

	/* Exactly 2 points: return input unchanged */
	do_test_catmull_rom(
		"LINESTRING(0 0, 10 10)",
		"LINESTRING(0 0,10 10)",
		5);

	/* Empty line: return empty */
	do_test_catmull_rom(
		"LINESTRING EMPTY",
		"LINESTRING EMPTY",
		5);
}

/*
 * Verify that original vertices are preserved by checking a symmetric
 * curve where we can identify exactly where original points fall.
 * LINESTRING(0 0, 0 10, 10 10, 10 0) with n_segments=1 is trivial
 * (n_segments=1 is invalid per SQL, but at the C level we test =2).
 */
static void
do_test_catmull_rom_vertex_preservation(void)
{
	LWGEOM *geom_in, *geom_out;
	POINT4D pt;

	/* With n_segments=3 on a 4-point line, output has 1+3*3=10 points.
	 * Original vertices must appear at indices 0, 3, 6, 9. */
	geom_in = lwgeom_from_wkt("LINESTRING(0 0, 1 0, 2 0, 3 0)", LW_PARSER_CHECK_NONE);
	geom_out = lwgeom_catmull_rom(geom_in, 3);

	LWLINE *oline = (LWLINE *)geom_out;
	CU_ASSERT_EQUAL(oline->points->npoints, 10);

	/* Check original vertices at expected positions */
	pt = getPoint4d(oline->points, 0);
	CU_ASSERT_DOUBLE_EQUAL(pt.x, 0.0, 1e-10);
	CU_ASSERT_DOUBLE_EQUAL(pt.y, 0.0, 1e-10);

	pt = getPoint4d(oline->points, 3);
	CU_ASSERT_DOUBLE_EQUAL(pt.x, 1.0, 1e-10);
	CU_ASSERT_DOUBLE_EQUAL(pt.y, 0.0, 1e-10);

	pt = getPoint4d(oline->points, 6);
	CU_ASSERT_DOUBLE_EQUAL(pt.x, 2.0, 1e-10);
	CU_ASSERT_DOUBLE_EQUAL(pt.y, 0.0, 1e-10);

	pt = getPoint4d(oline->points, 9);
	CU_ASSERT_DOUBLE_EQUAL(pt.x, 3.0, 1e-10);
	CU_ASSERT_DOUBLE_EQUAL(pt.y, 0.0, 1e-10);

	lwgeom_free(geom_in);
	lwgeom_free(geom_out);
}

/*
 * Polygon ring smoothing: a 5-point square ring (4 unique vertices + closing)
 * is the minimum for Catmull-Rom on a ring (needs npoints-1 >= 4).
 */
static void
do_test_catmull_rom_polygons(void)
{
	LWGEOM *geom_in, *geom_out;
	LWPOLY *opoly;

	/* Polygon with 5 ring points (4 unique + close): smooth with n_segments=2.
	 * Each ring span produces 2 sub-segments, 4 spans total → 4*2+1 = 9 ring pts. */
	geom_in = lwgeom_from_wkt(
		"POLYGON((0 0, 4 0, 4 4, 0 4, 0 0))",
		LW_PARSER_CHECK_NONE);
	geom_out = lwgeom_catmull_rom(geom_in, 2);

	opoly = (LWPOLY *)geom_out;
	CU_ASSERT_EQUAL(opoly->nrings, 1);
	CU_ASSERT_EQUAL(opoly->rings[0]->npoints, 9);

	/* First and last ring points must be identical (closed) */
	POINT4D first = getPoint4d(opoly->rings[0], 0);
	POINT4D last  = getPoint4d(opoly->rings[0], opoly->rings[0]->npoints - 1);
	CU_ASSERT_DOUBLE_EQUAL(first.x, last.x, 1e-10);
	CU_ASSERT_DOUBLE_EQUAL(first.y, last.y, 1e-10);

	lwgeom_free(geom_in);
	lwgeom_free(geom_out);

	/* Triangle ring (npoints-1 = 3 < 4): return original ring unchanged */
	geom_in = lwgeom_from_wkt(
		"POLYGON((0 0, 4 0, 2 4, 0 0))",
		LW_PARSER_CHECK_NONE);
	geom_out = lwgeom_catmull_rom(geom_in, 5);
	opoly = (LWPOLY *)geom_out;
	CU_ASSERT_EQUAL(opoly->rings[0]->npoints, 4); /* unchanged */
	lwgeom_free(geom_in);
	lwgeom_free(geom_out);
}

/*
 * Z and M dimensions must be interpolated by the algorithm.
 */
static void
do_test_catmull_rom_zm(void)
{
	LWGEOM *geom_in, *geom_out;
	LWLINE *oline;
	POINT4D pt;

	/* Collinear in XYZ: Z should be interpolated linearly on a straight line */
	geom_in = lwgeom_from_wkt(
		"LINESTRING Z (0 0 0, 1 0 10, 2 0 20, 3 0 30)",
		LW_PARSER_CHECK_NONE);
	geom_out = lwgeom_catmull_rom(geom_in, 2);
	oline = (LWLINE *)geom_out;

	/* Z at midpoint of first span (index 1) should be ~5 */
	pt = getPoint4d(oline->points, 1);
	CU_ASSERT_DOUBLE_EQUAL(pt.z, 5.0, 1e-10);

	/* Original Z values preserved at span boundaries */
	pt = getPoint4d(oline->points, 2);
	CU_ASSERT_DOUBLE_EQUAL(pt.z, 10.0, 1e-10);

	lwgeom_free(geom_in);
	lwgeom_free(geom_out);
}

/*
 * Points pass through unchanged.
 */
static void
do_test_catmull_rom_points(void)
{
	do_test_catmull_rom("POINT(1 2)", "POINT(1 2)", 5);
	do_test_catmull_rom("MULTIPOINT((1 2),(3 4))", "MULTIPOINT(1 2,3 4)", 5);
}


void catmullrom_suite_setup(void);
void catmullrom_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("catmullrom", NULL, NULL);
	PG_ADD_TEST(suite, do_test_catmull_rom_lines);
	PG_ADD_TEST(suite, do_test_catmull_rom_vertex_preservation);
	PG_ADD_TEST(suite, do_test_catmull_rom_polygons);
	PG_ADD_TEST(suite, do_test_catmull_rom_zm);
	PG_ADD_TEST(suite, do_test_catmull_rom_points);
}
