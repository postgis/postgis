/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright (C) 2016 Sandro Santilli <strk@kbt.io>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "CUnit/Basic.h"
#include "cu_tester.h"

#include "liblwgeom.h"
#include "liblwgeom_internal.h"

static void test_lwgeom_wrapx(void)
{
	LWGEOM *geom, *ret;
	char *exp_wkt, *obt_wkt;

	geom = lwgeom_from_wkt(
		"POLYGON EMPTY",
		LW_PARSER_CHECK_NONE);
	CU_ASSERT(geom != NULL);
	ret = lwgeom_wrapx(geom, 0, 20);
	CU_ASSERT(ret != NULL);
	obt_wkt = lwgeom_to_ewkt(ret);
	exp_wkt = "POLYGON EMPTY";
	ASSERT_STRING_EQUAL(obt_wkt, exp_wkt);
	lwfree(obt_wkt);
	lwgeom_free(ret);
	lwgeom_free(geom);

	geom = lwgeom_from_wkt(
		"POINT(0 0)",
		LW_PARSER_CHECK_NONE);
	CU_ASSERT(geom != NULL);
	ret = lwgeom_wrapx(geom, 2, 10);
	CU_ASSERT(ret != NULL);
	obt_wkt = lwgeom_to_ewkt(ret);
	exp_wkt = "POINT(10 0)";
	ASSERT_STRING_EQUAL(obt_wkt, exp_wkt);
	lwfree(obt_wkt);
	lwgeom_free(ret);
	lwgeom_free(geom);

	geom = lwgeom_from_wkt(
		"POINT(0 0)",
		LW_PARSER_CHECK_NONE);
	CU_ASSERT(geom != NULL);
	ret = lwgeom_wrapx(geom, 0, 20);
	CU_ASSERT(ret != NULL);
	obt_wkt = lwgeom_to_ewkt(ret);
	exp_wkt = "POINT(0 0)";
	ASSERT_STRING_EQUAL(obt_wkt, exp_wkt);
	lwfree(obt_wkt);
	lwgeom_free(ret);
	lwgeom_free(geom);

	geom = lwgeom_from_wkt(
		"POINT(0 0)",
		LW_PARSER_CHECK_NONE);
	CU_ASSERT(geom != NULL);
	ret = lwgeom_wrapx(geom, 0, -20);
	CU_ASSERT(ret != NULL);
	obt_wkt = lwgeom_to_ewkt(ret);
	exp_wkt = "POINT(0 0)";
	ASSERT_STRING_EQUAL(obt_wkt, exp_wkt);
	lwfree(obt_wkt);
	lwgeom_free(ret);
	lwgeom_free(geom);

	geom = lwgeom_from_wkt(
		"LINESTRING(0 0,10 0)",
		LW_PARSER_CHECK_NONE);
	CU_ASSERT(geom != NULL);
	ret = lwgeom_wrapx(geom, 8, -10);
	CU_ASSERT(ret != NULL);
	obt_wkt = lwgeom_to_ewkt(ret);
	exp_wkt = "MULTILINESTRING((0 0,8 0),(-2 0,0 0))";
	ASSERT_STRING_EQUAL(obt_wkt, exp_wkt);
	lwfree(obt_wkt);
	lwgeom_free(ret);
	lwgeom_free(geom);

	geom = lwgeom_from_wkt(
		"MULTILINESTRING((-5 -2,0 0),(0 0,10 10))",
		LW_PARSER_CHECK_NONE);
	CU_ASSERT(geom != NULL);
	ret = lwgeom_wrapx(geom, 0, 20);
	CU_ASSERT(ret != NULL);
	obt_wkt = lwgeom_to_ewkt(ret);
	exp_wkt = "MULTILINESTRING((15 -2,20 0),(0 0,10 10))";
	ASSERT_STRING_EQUAL(obt_wkt, exp_wkt);
	lwfree(obt_wkt);
	lwgeom_free(ret);
	lwgeom_free(geom);

	geom = lwgeom_from_wkt(
		"GEOMETRYCOLLECTION("
		" MULTILINESTRING((-5 -2,0 0),(0 0,10 10)),"
		" POINT(-5 0),"
		" POLYGON EMPTY"
		")",
		LW_PARSER_CHECK_NONE);
	CU_ASSERT(geom != NULL);
	ret = lwgeom_wrapx(geom, 0, 20);
	CU_ASSERT(ret != NULL);
	obt_wkt = lwgeom_to_ewkt(ret);
	exp_wkt = "GEOMETRYCOLLECTION("
						"MULTILINESTRING((15 -2,20 0),(0 0,10 10)),"
						"POINT(15 0),"
						"POLYGON EMPTY"
						")";
	ASSERT_STRING_EQUAL(obt_wkt, exp_wkt);
	lwfree(obt_wkt);
	lwgeom_free(ret);
	lwgeom_free(geom);

}


/*
** Used by test harness to register the tests in this file.
*/
void wrapx_suite_setup(void);
void wrapx_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("wrapx", NULL, NULL);
	PG_ADD_TEST(suite, test_lwgeom_wrapx);
}
