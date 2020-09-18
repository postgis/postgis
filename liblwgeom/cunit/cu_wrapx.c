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
#include "../lwgeom_geos.h"

static void test_lwgeom_wrapx(void)
{
	LWGEOM *geom, *ret, *tmp, *tmp2;
	char *exp_wkt, *obt_wkt;

	geom = lwgeom_from_wkt(
		"POLYGON EMPTY",
		LW_PARSER_CHECK_NONE);
	CU_ASSERT_FATAL(geom != NULL);
	ret = lwgeom_wrapx(geom, 0, 20);
	CU_ASSERT_FATAL(ret != NULL);
	obt_wkt = lwgeom_to_ewkt(ret);
	exp_wkt = "POLYGON EMPTY";
	ASSERT_STRING_EQUAL(obt_wkt, exp_wkt);
	lwfree(obt_wkt);
	lwgeom_free(ret);
	lwgeom_free(geom);

	geom = lwgeom_from_wkt(
		"POINT(0 0)",
		LW_PARSER_CHECK_NONE);
	CU_ASSERT_FATAL(geom != NULL);
	ret = lwgeom_wrapx(geom, 2, 10);
	CU_ASSERT_FATAL(ret != NULL);
	obt_wkt = lwgeom_to_ewkt(ret);
	exp_wkt = "POINT(10 0)";
	ASSERT_STRING_EQUAL(obt_wkt, exp_wkt);
	lwfree(obt_wkt);
	lwgeom_free(ret);
	lwgeom_free(geom);

	geom = lwgeom_from_wkt(
		"POINT(0 0)",
		LW_PARSER_CHECK_NONE);
	CU_ASSERT_FATAL(geom != NULL);
	ret = lwgeom_wrapx(geom, 0, 20);
	CU_ASSERT_FATAL(ret != NULL);
	obt_wkt = lwgeom_to_ewkt(ret);
	exp_wkt = "POINT(0 0)";
	ASSERT_STRING_EQUAL(obt_wkt, exp_wkt);
	lwfree(obt_wkt);
	lwgeom_free(ret);
	lwgeom_free(geom);

	geom = lwgeom_from_wkt(
		"POINT(0 0)",
		LW_PARSER_CHECK_NONE);
	CU_ASSERT_FATAL(geom != NULL);
	ret = lwgeom_wrapx(geom, 0, -20);
	CU_ASSERT_FATAL(ret != NULL);
	obt_wkt = lwgeom_to_ewkt(ret);
	exp_wkt = "POINT(0 0)";
	ASSERT_STRING_EQUAL(obt_wkt, exp_wkt);
	lwfree(obt_wkt);
	lwgeom_free(ret);
	lwgeom_free(geom);

	geom = lwgeom_from_wkt(
		"LINESTRING(0 0,10 0)",
		LW_PARSER_CHECK_NONE);
	CU_ASSERT_FATAL(geom != NULL);
	tmp = lwgeom_wrapx(geom, 8, -10);
	ret = lwgeom_normalize(tmp);
	lwgeom_free(tmp);
	CU_ASSERT_FATAL(ret != NULL);
	obt_wkt = lwgeom_to_ewkt(ret);
	tmp = lwgeom_from_wkt(
					"MULTILINESTRING((0 0,8 0),(-2 0,0 0))",
					LW_PARSER_CHECK_NONE);
	tmp2 = lwgeom_normalize(tmp);
	lwgeom_free(tmp);
	exp_wkt = lwgeom_to_ewkt(tmp2);
	lwgeom_free(tmp2);
	ASSERT_STRING_EQUAL(obt_wkt, exp_wkt);
	lwfree(obt_wkt);
	lwfree(exp_wkt);
	lwgeom_free(ret);
	lwgeom_free(geom);

	geom = lwgeom_from_wkt(
		"MULTILINESTRING((-5 -2,0 0),(0 0,10 10))",
		LW_PARSER_CHECK_NONE);
	CU_ASSERT_FATAL(geom != NULL);
	tmp = lwgeom_wrapx(geom, 0, 20);
	ret = lwgeom_normalize(tmp);
	lwgeom_free(tmp);
	CU_ASSERT_FATAL(ret != NULL);
	obt_wkt = lwgeom_to_ewkt(ret);
	tmp = lwgeom_from_wkt("MULTILINESTRING((15 -2,20 0),(0 0,10 10))", LW_PARSER_CHECK_NONE);
	tmp2 = lwgeom_normalize(tmp);
	lwgeom_free(tmp);
	exp_wkt = lwgeom_to_ewkt(tmp2);
	lwgeom_free(tmp2);
	ASSERT_STRING_EQUAL(obt_wkt, exp_wkt);
	lwfree(obt_wkt);
	lwfree(exp_wkt);
	lwgeom_free(ret);
	lwgeom_free(geom);

	geom = lwgeom_from_wkt(
		"MULTIPOLYGON("
		" ((0 0,10 0,10 10,0 10,0 0),(2 2,4 2,4 4,2 4,2 2)),"
		" ((0 11,10 11,10 21,0 21,0 11),(2 13,4 13,4 15,2 15,2 13))"
		")",
		LW_PARSER_CHECK_NONE);
	CU_ASSERT_FATAL(geom != NULL);
	tmp = lwgeom_wrapx(geom, 2, 20);
	ret = lwgeom_normalize(tmp);
	lwgeom_free(tmp);
	CU_ASSERT_FATAL(ret != NULL);
	obt_wkt = lwgeom_to_ewkt(ret);
	tmp = lwgeom_from_wkt("GEOMETRYCOLLECTION("
							    "MULTIPOLYGON("
							    "((22 0,20 0,20 10,22 10,22 4,22 2,22 0)),"
							    "((2 10,10 10,10 0,2 0,2 2,4 2,4 4,2 4,2 10))"
							    "),"
							    "MULTIPOLYGON("
							    "((22 11,20 11,20 21,22 21,22 15,22 13,22 11)),"
							    "((2 21,10 21,10 11,2 11,2 13,4 13,4 15,2 15,2 21))"
							    ")"
							    ")",
							    LW_PARSER_CHECK_NONE);
	tmp2 = lwgeom_normalize(tmp);
	lwgeom_free(tmp);
	exp_wkt = lwgeom_to_ewkt(tmp2);
	lwgeom_free(tmp2);
	ASSERT_STRING_EQUAL(obt_wkt, exp_wkt);
	lwfree(obt_wkt);
	lwfree(exp_wkt);
	lwgeom_free(ret);
	lwgeom_free(geom);

	geom = lwgeom_from_wkt(
		"GEOMETRYCOLLECTION("
		" MULTILINESTRING((-5 -2,0 0),(0 0,10 10)),"
		" POINT(-5 0),"
		" POLYGON EMPTY"
		")",
		LW_PARSER_CHECK_NONE);
	CU_ASSERT_FATAL(geom != NULL);
	tmp = lwgeom_wrapx(geom, 0, 20);
	ret = lwgeom_normalize(tmp);
	lwgeom_free(tmp);
	CU_ASSERT_FATAL(ret != NULL);
	obt_wkt = lwgeom_to_ewkt(ret);
	tmp = lwgeom_from_wkt(
	    "GEOMETRYCOLLECTION("
	    "MULTILINESTRING((15 -2,20 0),(0 0,10 10)),"
	    "POINT(15 0),"
	    "POLYGON EMPTY"
	    ")",
	    LW_PARSER_CHECK_NONE);
	tmp2 = lwgeom_normalize(tmp);
	lwgeom_free(tmp);
	exp_wkt = lwgeom_to_ewkt(tmp2);
	lwgeom_free(tmp2);
	ASSERT_STRING_EQUAL(obt_wkt, exp_wkt);
	lwfree(obt_wkt);
	lwfree(exp_wkt);
	lwgeom_free(ret);
	lwgeom_free(geom);
}

static int
clean_geos_wrapx_suite(void)
{
	finishGEOS();
	return 0;
}

/*
** Used by test harness to register the tests in this file.
*/
void wrapx_suite_setup(void);
void wrapx_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("wrapx", NULL, clean_geos_wrapx_suite);
	PG_ADD_TEST(suite, test_lwgeom_wrapx);
}
