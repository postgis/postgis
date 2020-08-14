/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright (C) 2011-2015 Sandro Santilli <strk@kbt.io>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "CUnit/Basic.h"
#include "cu_tester.h"

#include "liblwgeom.h"
#include "liblwgeom_internal.h"
#include "../lwgeom_geos.h"

static void test_lwline_split_by_point_to(void)
{
	LWLINE *line;
	LWPOINT *point;
	LWMLINE *coll;
	int ret;

	/* Because i don't trust that much prior tests...  ;) */
	cu_error_msg_reset();

	coll = lwmline_construct_empty(SRID_UNKNOWN, 0, 0);
	CU_ASSERT_EQUAL(coll->ngeoms, 0);

	line = lwgeom_as_lwline(lwgeom_from_wkt("LINESTRING(0 0,5 5, 10 0)",
		LW_PARSER_CHECK_NONE));
	CU_ASSERT(line != NULL);

	point = lwgeom_as_lwpoint(lwgeom_from_wkt(
		"POINT(0 0)",
		LW_PARSER_CHECK_NONE));
	ret = lwline_split_by_point_to(line, point, coll);
	CU_ASSERT_EQUAL(ret, 1);
	CU_ASSERT_EQUAL(coll->ngeoms, 0);
	lwpoint_free(point);

	point = lwgeom_as_lwpoint(lwgeom_from_wkt(
		"POINT(10 0)",
		LW_PARSER_CHECK_NONE));
	ret = lwline_split_by_point_to(line, point, coll);
	CU_ASSERT_EQUAL(ret, 1);
	CU_ASSERT_EQUAL(coll->ngeoms, 0);
	lwpoint_free(point);

	point = lwgeom_as_lwpoint(lwgeom_from_wkt(
		"POINT(5 0)",
		LW_PARSER_CHECK_NONE));
	ret = lwline_split_by_point_to(line, point, coll);
	CU_ASSERT_EQUAL(ret, 0);
	CU_ASSERT_EQUAL(coll->ngeoms, 0);
	lwpoint_free(point);

	point = lwgeom_as_lwpoint(lwgeom_from_wkt(
		"POINT(5 5)",
		LW_PARSER_CHECK_NONE));
	ret = lwline_split_by_point_to(line, point, coll);
	CU_ASSERT_EQUAL(ret, 2);
	CU_ASSERT_EQUAL(coll->ngeoms, 2);
	lwpoint_free(point);

	point = lwgeom_as_lwpoint(lwgeom_from_wkt(
		"POINT(2 2)",
		LW_PARSER_CHECK_NONE));
	ret = lwline_split_by_point_to(line, point, coll);
	CU_ASSERT_EQUAL(ret, 2);
	CU_ASSERT_EQUAL(coll->ngeoms, 4);
	lwpoint_free(point);

	lwcollection_free((LWCOLLECTION*)coll);
	lwline_free(line);
}

static void test_lwgeom_split(void)
{
	LWGEOM *geom, *blade, *ret, *tmp1, *tmp2;
	char *wkt, *in_wkt;

	geom = lwgeom_from_wkt("MULTILINESTRING((-5 -2,0 0),(0 0,10 10))", LW_PARSER_CHECK_NONE);
	CU_ASSERT(geom != NULL);
	blade = lwgeom_from_wkt("POINT(0 0)", LW_PARSER_CHECK_NONE);
	CU_ASSERT(blade != NULL);
	ret = lwgeom_split(geom, blade);
	CU_ASSERT(ret != NULL);
	wkt = lwgeom_to_ewkt(ret);
	in_wkt = "GEOMETRYCOLLECTION(LINESTRING(-5 -2,0 0),LINESTRING(0 0,10 10))";
	ASSERT_STRING_EQUAL(wkt, in_wkt);
	lwfree(wkt);
	lwgeom_free(ret);
	lwgeom_free(geom);
	lwgeom_free(blade);

        /* See #1311 */
	geom = lwgeom_from_wkt("LINESTRING(0 0,10 0,20 4,0 3)", LW_PARSER_CHECK_NONE);
	CU_ASSERT(geom != NULL);
	blade = lwgeom_from_wkt("POINT(10 0)", LW_PARSER_CHECK_NONE);
	ret = lwgeom_split(geom, blade);
	CU_ASSERT(ret != NULL);
	wkt = lwgeom_to_ewkt(ret);
	in_wkt = "GEOMETRYCOLLECTION(LINESTRING(0 0,10 0),LINESTRING(10 0,20 4,0 3))";
	ASSERT_STRING_EQUAL(wkt, in_wkt);
	lwfree(wkt);
	lwgeom_free(ret);
	lwgeom_free(geom);
	lwgeom_free(blade);

  /* See #2528 (1) -- memory leak test, needs valgrind to check */
	geom = lwgeom_from_wkt("SRID=1;LINESTRING(0 1,10 1)", LW_PARSER_CHECK_NONE);
	CU_ASSERT(geom != NULL);
	blade = lwgeom_from_wkt("LINESTRING(7 0,7 3)", LW_PARSER_CHECK_NONE);
	ret = lwgeom_split(geom, blade);
	CU_ASSERT(ret != NULL);
	wkt = lwgeom_to_ewkt(ret);
	in_wkt = "SRID=1;GEOMETRYCOLLECTION(LINESTRING(0 1,7 1),LINESTRING(7 1,10 1))";
	ASSERT_STRING_EQUAL(wkt, in_wkt);
	lwfree(wkt);
	lwgeom_free(ret);
	lwgeom_free(geom);
	lwgeom_free(blade);

  /* See #2528 (2) -- memory leak test, needs valgrind to check */
	geom = lwgeom_from_wkt("SRID=1;POLYGON((0 1, 10 1, 10 10, 0 10, 0 1))", LW_PARSER_CHECK_NONE);
	CU_ASSERT(geom != NULL);
	blade = lwgeom_from_wkt("LINESTRING(7 0,7 20)", LW_PARSER_CHECK_NONE);
	tmp1 = lwgeom_split(geom, blade);
	ret = lwgeom_normalize(tmp1);
	lwgeom_free(tmp1);
	CU_ASSERT(ret != NULL);
	wkt = lwgeom_to_ewkt(ret);
	tmp1 = lwgeom_from_wkt(
	    "SRID=1;GEOMETRYCOLLECTION(POLYGON((7 1,0 1,0 10,7 10,7 1)),POLYGON((7 10,10 10,10 1,7 1,7 10)))",
	    LW_PARSER_CHECK_NONE);
	tmp2 = lwgeom_normalize(tmp1);
	in_wkt = lwgeom_to_ewkt(tmp2);
	ASSERT_STRING_EQUAL(wkt, in_wkt);
	lwfree(wkt);
	lwfree(in_wkt);
	lwgeom_free(tmp1);
	lwgeom_free(tmp2);
	lwgeom_free(ret);
	lwgeom_free(geom);
	lwgeom_free(blade);

  /* Split line by multiline */
	geom = lwgeom_from_wkt("LINESTRING(0 0, 10 0)", LW_PARSER_CHECK_NONE);
	CU_ASSERT_FATAL(geom != NULL);
	blade = lwgeom_from_wkt("MULTILINESTRING((1 1,1 -1),(2 1,2 -1,3 -1,3 1))", LW_PARSER_CHECK_NONE);
	ret = lwgeom_split(geom, blade);
	if (!ret)
		printf("%s", cu_error_msg);
	CU_ASSERT_FATAL(ret != NULL);
	wkt = lwgeom_to_ewkt(ret);
	CU_ASSERT_FATAL(wkt != NULL);
	in_wkt = "GEOMETRYCOLLECTION(LINESTRING(0 0,1 0),LINESTRING(1 0,2 0),LINESTRING(2 0,3 0),LINESTRING(3 0,10 0))";
	ASSERT_STRING_EQUAL(wkt, in_wkt);
	lwfree(wkt);
	lwgeom_free(ret);
	lwgeom_free(geom);
	lwgeom_free(blade);

  /* Split line by polygon (boundary) */
	geom = lwgeom_from_wkt("LINESTRING(0 0, 10 0)", LW_PARSER_CHECK_NONE);
	CU_ASSERT_FATAL(geom != NULL);
	blade = lwgeom_from_wkt("POLYGON((1 -2,1 1,2 1,2 -1,3 -1,3 1,11 1,11 -2,1 -2))", LW_PARSER_CHECK_NONE);
	ret = lwgeom_split(geom, blade);
	if (!ret)
		printf("%s", cu_error_msg);
	CU_ASSERT_FATAL(ret != NULL);
	wkt = lwgeom_to_ewkt(ret);
	CU_ASSERT_FATAL(wkt != NULL);
	in_wkt = "GEOMETRYCOLLECTION(LINESTRING(0 0,1 0),LINESTRING(1 0,2 0),LINESTRING(2 0,3 0),LINESTRING(3 0,10 0))";
	ASSERT_STRING_EQUAL(wkt, in_wkt);
	lwfree(wkt);
	lwgeom_free(ret);
	lwgeom_free(geom);
	lwgeom_free(blade);

	/* Split line by EMPTY polygon (boundary) */
	geom = lwgeom_from_wkt("LINESTRING(0 0, 10 0)", LW_PARSER_CHECK_NONE);
	CU_ASSERT_FATAL(geom != NULL);
	blade = lwgeom_from_wkt("POLYGON EMPTY", LW_PARSER_CHECK_NONE);
	ret = lwgeom_split(geom, blade);
	if (!ret)
		printf("%s", cu_error_msg);
	CU_ASSERT_FATAL(ret != NULL);
	wkt = lwgeom_to_ewkt(ret);
	CU_ASSERT_FATAL(wkt != NULL);
	in_wkt = "GEOMETRYCOLLECTION(LINESTRING(0 0,10 0))";
	ASSERT_STRING_EQUAL(wkt, in_wkt);
	lwfree(wkt);
	lwgeom_free(ret);
	lwgeom_free(geom);
	lwgeom_free(blade);

  /* Split line by multipolygon (boundary) */
	geom = lwgeom_from_wkt("LINESTRING(0 0, 10 0)", LW_PARSER_CHECK_NONE);
	CU_ASSERT_FATAL(geom != NULL);
	blade = lwgeom_from_wkt("MULTIPOLYGON(((1 -1,1 1,2 1,2 -1,1 -1)),((3 -1,3 1,11 1,11 -1,3 -1)))",
				LW_PARSER_CHECK_NONE);
	ret = lwgeom_split(geom, blade);
	if (!ret)
		printf("%s", cu_error_msg);
	CU_ASSERT_FATAL(ret != NULL);
	wkt = lwgeom_to_ewkt(ret);
	CU_ASSERT_FATAL(wkt != NULL);
	in_wkt = "GEOMETRYCOLLECTION(LINESTRING(0 0,1 0),LINESTRING(1 0,2 0),LINESTRING(2 0,3 0),LINESTRING(3 0,10 0))";
	ASSERT_STRING_EQUAL(wkt, in_wkt);
	lwfree(wkt);
	lwgeom_free(ret);
	lwgeom_free(geom);
	lwgeom_free(blade);

	/* Split line by multipoint */
	geom = lwgeom_from_wkt("LINESTRING(0 0, 10 0)", LW_PARSER_CHECK_NONE);
	CU_ASSERT_FATAL(geom != NULL);
	blade = lwgeom_from_wkt("MULTIPOINT(2 0,8 0,4 0)", LW_PARSER_CHECK_NONE);
	ret = lwgeom_split(geom, blade);
	if (!ret)
		printf("%s", cu_error_msg);
	CU_ASSERT_FATAL(ret != NULL);
	wkt = lwgeom_to_ewkt(ret);
	CU_ASSERT_FATAL(wkt != NULL);
	in_wkt = "GEOMETRYCOLLECTION(LINESTRING(8 0,10 0),LINESTRING(0 0,2 0),LINESTRING(4 0,8 0),LINESTRING(2 0,4 0))";
	ASSERT_STRING_EQUAL(wkt, in_wkt);
	lwfree(wkt);
	lwgeom_free(ret);
	lwgeom_free(geom);
	lwgeom_free(blade);

	/* See #3401 -- robustness issue */
	geom = lwgeom_from_wkt("LINESTRING(-180 0,0 0)", LW_PARSER_CHECK_NONE);
	CU_ASSERT(geom != NULL);
	blade = lwgeom_from_wkt("POINT(-20 0)", LW_PARSER_CHECK_NONE);
	ret = lwgeom_split(geom, blade);
	CU_ASSERT(ret != NULL);
	{
		LWCOLLECTION *split = lwgeom_as_lwcollection(ret);
		LWLINE *l1, *l2;
		POINT2D pt;
		CU_ASSERT(split != NULL);
		l1 = lwgeom_as_lwline(split->geoms[0]);
		CU_ASSERT(l1 != NULL);
		getPoint2d_p(l1->points, 1, &pt);
		ASSERT_DOUBLE_EQUAL(pt.x, -20);
		ASSERT_DOUBLE_EQUAL(pt.y, 0);
		l2 = lwgeom_as_lwline(split->geoms[1]);
		CU_ASSERT(l2 != NULL);
		getPoint2d_p(l2->points, 0, &pt);
		ASSERT_DOUBLE_EQUAL(pt.x, -20);
		ASSERT_DOUBLE_EQUAL(pt.y, 0);
	}
	lwgeom_free(ret);
	lwgeom_free(geom);
	lwgeom_free(blade);
}

static int
clean_geos_split_suite(void)
{
	finishGEOS();
	return 0;
}

/*
** Used by test harness to register the tests in this file.
*/
void split_suite_setup(void);
void split_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("split", NULL, clean_geos_split_suite);
	PG_ADD_TEST(suite, test_lwline_split_by_point_to);
	PG_ADD_TEST(suite, test_lwgeom_split);
}
