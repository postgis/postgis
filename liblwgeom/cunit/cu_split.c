/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 *
 * Copyright (C) 2011 Sandro Santilli <strk@keybit.net>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "CUnit/Basic.h"
#include "cu_tester.h"

#include "liblwgeom.h"
#include "liblwgeom_internal.h"

static void test_lwline_split_by_point_to(void)
{
#if POSTGIS_GEOS_VERSION >= 33
	LWLINE *line;
	LWPOINT *point;
	LWMLINE *coll;
	int ret;

	/* Because i don't trust that much prior tests...  ;) */
	cu_error_msg_reset();

	coll = lwmline_construct_empty(SRID_UNKNOWN, 0, 0);
	CU_ASSERT_EQUAL(coll->ngeoms, 0);

	line = lwgeom_as_lwline(lwgeom_from_wkt("LINESTRING(0 0,5 5, 10 0))",
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

#endif /* POSTGIS_GEOS_VERSION >= 33 */
}

static void test_lwgeom_split(void)
{
	LWGEOM *geom, *blade, *ret;
	char *wkt, *in_wkt;

	geom = lwgeom_from_wkt(
"MULTILINESTRING((-5 -2,0 0),(0 0,10 10))",
	LW_PARSER_CHECK_NONE);
	CU_ASSERT(geom != NULL);
	blade = lwgeom_from_wkt(
		"POINT(0 0)",
		LW_PARSER_CHECK_NONE);
	CU_ASSERT(blade != NULL);
	ret = lwgeom_split(geom, blade);
	CU_ASSERT(ret != NULL);
	wkt = lwgeom_to_ewkt(ret);
	in_wkt = "GEOMETRYCOLLECTION(LINESTRING(-5 -2,0 0),LINESTRING(0 0,10 10))";
        if (strcmp(in_wkt, wkt))
                fprintf(stderr, "\nExp:  %s\nObt:  %s\n", in_wkt, wkt);
	CU_ASSERT_STRING_EQUAL(wkt, in_wkt);
	lwfree(wkt);
	lwgeom_free(ret);
	lwgeom_free(geom);
	lwgeom_free(blade);

        /* See #1311 */
        geom = lwgeom_from_wkt(
                "LINESTRING(0 0,10 0,20 4,0 3)",
                LW_PARSER_CHECK_NONE);
        CU_ASSERT(geom != NULL);
        blade = lwgeom_from_wkt("POINT(10 0)", LW_PARSER_CHECK_NONE);
        ret = lwgeom_split(geom, blade);
        CU_ASSERT(ret != NULL);
        wkt = lwgeom_to_ewkt(ret);
	in_wkt = "GEOMETRYCOLLECTION(LINESTRING(0 0,10 0),LINESTRING(10 0,20 4,0 3))";
        if (strcmp(in_wkt, wkt))
                fprintf(stderr, "\nExp:  %s\nObt:  %s\n", in_wkt, wkt);
        CU_ASSERT_STRING_EQUAL(wkt, in_wkt);
        lwfree(wkt);
	lwgeom_free(ret);
	lwgeom_free(geom);
	lwgeom_free(blade);
}


/*
** Used by test harness to register the tests in this file.
*/
CU_TestInfo split_tests[] =
{
	PG_TEST(test_lwline_split_by_point_to),
	PG_TEST(test_lwgeom_split),
	CU_TEST_INFO_NULL
};
CU_SuiteInfo split_suite = {"split",  NULL,  NULL, split_tests};
