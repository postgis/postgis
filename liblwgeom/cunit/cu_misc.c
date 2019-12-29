/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 * Copyright 2008 Paul Ramsey
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


static void test_misc_simplify(void)
{
	LWGEOM *geom;
	LWGEOM *geom2d;
	char *wkt_out;

	geom = lwgeom_from_wkt("LINESTRING(0 0,0 10,0 51,50 20,30 20,7 32)", LW_PARSER_CHECK_NONE);
	geom2d = lwgeom_simplify(geom, 2, LW_FALSE);
	wkt_out = lwgeom_to_ewkt(geom2d);
	CU_ASSERT_STRING_EQUAL("LINESTRING(0 0,0 51,50 20,30 20,7 32)",wkt_out);
	lwgeom_free(geom);
	lwgeom_free(geom2d);
	lwfree(wkt_out);

	geom = lwgeom_from_wkt("MULTILINESTRING((0 0,0 10,0 51,50 20,30 20,7 32))", LW_PARSER_CHECK_NONE);
	geom2d = lwgeom_simplify(geom, 2, LW_FALSE);
	wkt_out = lwgeom_to_ewkt(geom2d);
	CU_ASSERT_STRING_EQUAL("MULTILINESTRING((0 0,0 51,50 20,30 20,7 32))",wkt_out);
	lwgeom_free(geom);
	lwgeom_free(geom2d);
	lwfree(wkt_out);

	geom = lwgeom_from_wkt("POLYGON((0 0,1 1,1 3,0 4,-2 3,-1 1,0 0))", LW_PARSER_CHECK_NONE);
	geom2d = lwgeom_simplify(geom, 1, LW_FALSE);
	wkt_out = lwgeom_to_ewkt(geom2d);
	CU_ASSERT_STRING_EQUAL("POLYGON((0 0,0 4,-2 3,0 0))", wkt_out);
	lwgeom_free(geom);
	lwgeom_free(geom2d);
	lwfree(wkt_out);
}

static void test_misc_count_vertices(void)
{
	LWGEOM *geom;
	int count;

	geom = lwgeom_from_wkt("GEOMETRYCOLLECTION(POINT(0 0),LINESTRING(0 0,1 1),POLYGON((0 0,0 1,1 0,0 0)),CIRCULARSTRING(0 0,0 1,1 1),CURVEPOLYGON(CIRCULARSTRING(0 0,0 1,1 1)))", LW_PARSER_CHECK_NONE);
	count = lwgeom_count_vertices(geom);
	CU_ASSERT_EQUAL(count,13);
	lwgeom_free(geom);

	geom = lwgeom_from_wkt("GEOMETRYCOLLECTION(CIRCULARSTRING(0 0,0 1,1 1),POINT(0 0),CURVEPOLYGON(CIRCULARSTRING(0 0,0 1,1 1,1 0,0 0)))", LW_PARSER_CHECK_NONE);
	count = lwgeom_count_vertices(geom);
	CU_ASSERT_EQUAL(count,9);
	lwgeom_free(geom);

	geom = lwgeom_from_wkt("CURVEPOLYGON((0 0,1 0,0 1,0 0),CIRCULARSTRING(0 0,1 0,1 1,1 0,0 0))", LW_PARSER_CHECK_NONE);
	count = lwgeom_count_vertices(geom);
	CU_ASSERT_EQUAL(count,9);
	lwgeom_free(geom);


	geom = lwgeom_from_wkt("POLYGON((0 0,1 0,0 1,0 0))", LW_PARSER_CHECK_NONE);
	count = lwgeom_count_vertices(geom);
	CU_ASSERT_EQUAL(count,4);
	lwgeom_free(geom);

	geom = lwgeom_from_wkt("CURVEPOLYGON((0 0,1 0,0 1,0 0),CIRCULARSTRING(0 0,1 0,1 1,1 0,0 0))", LW_PARSER_CHECK_NONE);
	count = lwgeom_count_vertices(geom);
	CU_ASSERT_EQUAL(count,9);
	lwgeom_free(geom);
}

static void test_misc_area(void)
{
	LWGEOM *geom;
	double area;

	geom = lwgeom_from_wkt("LINESTRING EMPTY", LW_PARSER_CHECK_ALL);
	area = lwgeom_area(geom);
	CU_ASSERT_DOUBLE_EQUAL(area, 0.0, 0.0001);
	lwgeom_free(geom);
}

static void test_misc_wkb(void)
{
	static char *wkb = "010A0000000200000001080000000700000000000000000000C00000000000000000000000000000F0BF000000000000F0BF00000000000000000000000000000000000000000000F03F000000000000F0BF000000000000004000000000000000000000000000000000000000000000004000000000000000C00000000000000000010200000005000000000000000000F0BF00000000000000000000000000000000000000000000E03F000000000000F03F00000000000000000000000000000000000000000000F03F000000000000F0BF0000000000000000";
	LWGEOM *geom = lwgeom_from_hexwkb(wkb, LW_PARSER_CHECK_ALL);
	char *str = lwgeom_to_wkt(geom, WKB_ISO, 8, 0);
	CU_ASSERT_STRING_EQUAL(str, "CURVEPOLYGON(CIRCULARSTRING(-2 0,-1 -1,0 0,1 -1,2 0,0 2,-2 0),(-1 0,0 0.5,1 0,0 1,-1 0))");
	lwfree(str);
	lwgeom_free(geom);

}


static void test_grid(void)
{
	gridspec grid;
	static char *wkt = "MULTIPOLYGON(((0 0, 10 0, 10 10, 0 10, 0 0)))";
	LWGEOM *geom = lwgeom_from_wkt(wkt, LW_PARSER_CHECK_ALL);
	LWGEOM *geomgrid;
	char *str;

	grid.ipx = grid.ipy = 0;
	grid.xsize = grid.ysize = 20;

	geomgrid = lwgeom_grid(geom, &grid);
	str = lwgeom_to_ewkt(geomgrid);
	CU_ASSERT_STRING_EQUAL(str, "MULTIPOLYGON EMPTY");
	lwfree(str);
	lwgeom_free(geom);
	lwgeom_free(geomgrid);
}

static void do_grid_test(const char *wkt_in, const char *wkt_out, double size)
{
	char *wkt_result, *wkt_norm;
	gridspec grid;
	LWGEOM *g = lwgeom_from_wkt(wkt_in, LW_PARSER_CHECK_ALL);
	LWGEOM *go = lwgeom_from_wkt(wkt_out, LW_PARSER_CHECK_ALL);
	wkt_norm = lwgeom_to_ewkt(go);
	memset(&grid, 0, sizeof(gridspec));
	grid.xsize = grid.ysize = grid.zsize = grid.msize = size;
	lwgeom_grid_in_place(g, &grid);
	wkt_result = lwgeom_to_ewkt(g);
    // printf("%s ==%ld==> %s == %s\n", wkt_in, lround(size), wkt_result, wkt_out);
	CU_ASSERT_STRING_EQUAL(wkt_result, wkt_norm);
	lwfree(wkt_result);
	lwfree(wkt_norm);
	lwgeom_free(g);
	lwgeom_free(go);
}

static void test_grid_in_place(void)
{
	do_grid_test(
		"POINT ZM (5.1423999999 5.1423999999 5.1423999999 5.1423999999)",
		"POINT(5.1424 5.1424 5.1424 5.1424)",
		0.0001
	);
	do_grid_test(
		"MULTIPOLYGON(((0 0,10 0,10 10,0 10,0 0)))",
		"MULTIPOLYGON EMPTY",
		20
	);
	do_grid_test(
		"MULTIPOLYGON(((0 0,10 0,10 10,0 10,0 0)))",
		"MULTIPOLYGON(((0 0,10 0,10 10, 0 10,0 0)))",
		1
	);
	do_grid_test(
		"LINESTRING(0 0,1 1, 2 2, 3 3, 4 4, 5 5)",
		"LINESTRING(0 0,2 2,4 4)",
		2
	);
	do_grid_test(
		"MULTIPOINT(0 0,1 1, 2 2, 3 3, 4 4, 5 5)",
		/* This preserves current behaviour, but is probably not right */
		"MULTIPOINT(0 0,0 0,2 2,4 4,4 4,4 4)",
		2
	);
	do_grid_test(
		"MULTIPOLYGON(((0 0,10 0,10 10,0 10,0 0),(4 4, 4 5, 5 5, 5 4, 4 4)))",
		"MULTIPOLYGON(((0 0,10 0,10 10, 0 10,0 0)))",
		2
	);
	do_grid_test(
		"MULTIPOLYGON(((0 0,10 0,10 10,0 10,0 0),(4 4, 4 5, 5 5, 5 4, 4 4)))",
		"MULTIPOLYGON EMPTY",
		20
	);
	do_grid_test(
		"POINT Z (5 5 5)",
		"POINT(0 0 0)",
		20
	);
}

static void test_clone(void)
{
	static char *wkt = "GEOMETRYCOLLECTION(MULTIPOLYGON(((0 0, 10 0, 10 10, 0 10, 0 0))),POINT(1 1),LINESTRING(2 3,4 5))";
	LWGEOM *geom1 = lwgeom_from_wkt(wkt, LW_PARSER_CHECK_ALL);
	LWGEOM *geom2;

	/* Free in "backwards" order */
	geom2 = lwgeom_clone(geom1);
	lwgeom_free(geom1);
	lwgeom_free(geom2);

	/* Free in "forewards" order */
	geom1 = lwgeom_from_wkt(wkt, LW_PARSER_CHECK_ALL);
	geom2 = lwgeom_clone(geom1);
	lwgeom_free(geom2);
	lwgeom_free(geom1);
}

static void test_lwmpoint_from_lwgeom(void)
{
	/* This cast is so ugly, we only want to do it once.  And not even that. */
	LWGEOM* (*to_points)(LWGEOM*) = (LWGEOM* (*)(LWGEOM*)) &lwmpoint_from_lwgeom;

	do_fn_test(to_points, "MULTIPOLYGON (EMPTY)", "MULTIPOINT EMPTY");
	do_fn_test(to_points, "POINT (30 10)", "MULTIPOINT ((30 10))");
	do_fn_test(to_points, "LINESTRING Z (30 10 4,10 30 5,40 40 6)", "MULTIPOINT Z (30 10 4,10 30 5, 40 40 6)");
	do_fn_test(to_points, "POLYGON((35 10,45 45,15 40,10 20,35 10),(20 30,35 35,30 20,20 30))", "MULTIPOINT(35 10,45 45,15 40,10 20,35 10,20 30,35 35,30 20,20 30)");
	do_fn_test(to_points, "MULTIPOINT M (10 40 1,40 30 2,20 20 3,30 10 4)", "MULTIPOINT M (10 40 1,40 30 2,20 20 3,30 10 4)");
	do_fn_test(to_points, "COMPOUNDCURVE(CIRCULARSTRING(0 0,2 0, 2 1, 2 3, 4 3),(4 3, 4 5, 1 4, 0 0))", "MULTIPOINT(0 0, 2 0, 2 1, 2 3, 4 3, 4 3, 4 5, 1 4, 0 0)");
	do_fn_test(to_points, "TIN(((80 130,50 160,80 70,80 130)),((50 160,10 190,10 70,50 160)))", "MULTIPOINT (80 130, 50 160, 80 70, 80 130, 50 160, 10 190, 10 70, 50 160)");
}

static void test_gbox_serialized_size(void)
{
	lwflags_t flags = lwflags(0, 0, 0);
	CU_ASSERT_EQUAL(gbox_serialized_size(flags),16);
	FLAGS_SET_BBOX(flags, 1);
	CU_ASSERT_EQUAL(gbox_serialized_size(flags),16);
	FLAGS_SET_Z(flags, 1);
	CU_ASSERT_EQUAL(gbox_serialized_size(flags),24);
	FLAGS_SET_M(flags, 1);
	CU_ASSERT_EQUAL(gbox_serialized_size(flags),32);
	FLAGS_SET_GEODETIC(flags, 1);
	CU_ASSERT_EQUAL(gbox_serialized_size(flags),24);
}



/*
** Used by the test harness to register the tests in this file.
*/
void misc_suite_setup(void);
void misc_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("miscellaneous", NULL, NULL);
	PG_ADD_TEST(suite, test_misc_simplify);
	PG_ADD_TEST(suite, test_misc_count_vertices);
	PG_ADD_TEST(suite, test_misc_area);
	PG_ADD_TEST(suite, test_misc_wkb);
	PG_ADD_TEST(suite, test_grid);
	PG_ADD_TEST(suite, test_grid_in_place);
	PG_ADD_TEST(suite, test_clone);
	PG_ADD_TEST(suite, test_lwmpoint_from_lwgeom);
	PG_ADD_TEST(suite, test_gbox_serialized_size);
}
