/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright 2014 Kashif Rasul <kashif.rasul@gmail.com> and
 *                Shoaib Burq <saburq@gmail.com>
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
do_encoded_polyline_test(char* in, int precision, char* out)
{
	LWGEOM *g = lwgeom_from_wkt(in, LW_PARSER_CHECK_NONE);
	lwvarlena_t *v = lwgeom_to_encoded_polyline(g, precision);

	ASSERT_VARLENA_EQUAL(v, out);

	lwgeom_free(g);
	lwfree(v);
}

static void
out_encoded_polyline_test_geoms(void)
{
	/* Magic Linestring */
	do_encoded_polyline_test(
		"SRID=4326;LINESTRING(33.6729 38.7071,33.6692 38.701,"
		"33.6673 38.6972,33.6626 38.6871)",
		5,
		"k~fkFsvolEbe@bVvVzJb~@j\\");

	/* Linestring */
	do_encoded_polyline_test(
		"LINESTRING(-120.2 38.5,-120.95 40.7,-126.453 43.252)",
		5,
		"_p~iF~ps|U_ulLnnqC_mqNvxq`@");
	do_encoded_polyline_test("LINESTRING EMPTY", 5, "");

	/* MultiPoint */
	do_encoded_polyline_test(
		"MULTIPOINT(-120.2 38.5,-120.95 40.7)", 5, "_p~iF~ps|U_ulLnnqC");
	do_encoded_polyline_test("MULTIPOINT EMPTY", 5, "");
}

static void
out_encoded_polyline_test_srid(void)
{

	/* SRID - with PointArray */
	do_encoded_polyline_test(
		"SRID=4326;LINESTRING(0 1,2 3)", 5, "_ibE?_seK_seK");

	/* wrong SRID */
	do_encoded_polyline_test(
		"SRID=4327;LINESTRING(0 1,2 3)", 5, "_ibE?_seK_seK");
}

static void
out_encoded_polyline_test_precision(void)
{

	/* Linestring */
	do_encoded_polyline_test(
		"LINESTRING(-0.250691 49.283048, -0.250633 49.283376,"
		"-0.250502 49.283972, -0.251245 49.284028, -0.251938 "
		"49.284232, -0.251938 49.2842)",
		6,
		"o}~~|AdshNoSsBgd@eGoBlm@wKhj@~@?");

	/* MultiPoint */
	do_encoded_polyline_test(
		"MULTIPOINT(-120.2 38.5,-120.95 40.7)", 3, "gejAnwiFohCzm@");
}

/*
** Used by test harness to register the tests in this file.
*/
void out_encoded_polyline_suite_setup(void);
void
out_encoded_polyline_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("encoded_polyline_output", NULL, NULL);
	PG_ADD_TEST(suite, out_encoded_polyline_test_geoms);
	PG_ADD_TEST(suite, out_encoded_polyline_test_srid);
	PG_ADD_TEST(suite, out_encoded_polyline_test_precision);
}
