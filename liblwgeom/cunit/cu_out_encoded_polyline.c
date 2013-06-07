/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 *
 * Copyright 2013 Kashif Rasul <kashif.rasul@gmail.com> and
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

static void do_encoded_polyline_test(char * in, int precision, char * out)
{
	LWGEOM *g;
	char * h;

	g = lwgeom_from_wkt(in, LW_PARSER_CHECK_NONE);
	h = lwgeom_to_encoded_polyline(g, precision);

	if (strcmp(h, out))
		fprintf(stderr, "\nIn:   %s\nOut:  %s\nTheo: %s\n", in, h, out);

	CU_ASSERT_STRING_EQUAL(h, out);

	lwgeom_free(g);
	lwfree(h);
}


static void do_encoded_polyline_unsupported(char * in, int precision, char * out)
{
	LWGEOM *g;
	char *h;

	g = lwgeom_from_wkt(in, LW_PARSER_CHECK_NONE);
	h = lwgeom_to_encoded_polyline(g, precision);

	if (strcmp(cu_error_msg, out))
		fprintf(stderr, "\nIn:   %s\nOut:  %s\nTheo: %s\n",
		        in, cu_error_msg, out);

	CU_ASSERT_STRING_EQUAL(out, cu_error_msg);
	cu_error_msg_reset();

	lwfree(h);
	lwgeom_free(g);
}

static void out_encoded_polyline_test_geoms(void)
{
	/* Linestring */
	do_encoded_polyline_test(
	    "LINESTRING(-120.2 38.5,-120.95 40.7,-126.453 43.252)",
	    5,
	    "_p~iF~ps|U_ulLnnqC_mqNvxq`@");

	/* MultiPoint */
	do_encoded_polyline_test(
	    "MULTIPOINT(-120.2 38.5,-120.95 40.7)",
	    5,
	    "_p~iF~ps|U_ulLnnqC");
}

static void out_encoded_polyline_test_srid(void)
{

	/* SRID - with PointArray */
	do_encoded_polyline_test(
	    "SRID=4326;LINESTRING(0 1,2 3)",
	    5,
	    "_ibE?_seK_seK");

	/* wrong SRID */
	do_encoded_polyline_test(
	    "SRID=4327;LINESTRING(0 1,2 3)",
	    5,
	    "_ibE?_seK_seK");
}

/*
** Used by test harness to register the tests in this file.
*/
CU_TestInfo out_encoded_polyline_tests[] =
{
	PG_TEST(out_encoded_polyline_test_geoms),
	PG_TEST(out_encoded_polyline_test_srid),
	CU_TEST_INFO_NULL
};
CU_SuiteInfo out_encoded_polyline_suite = {"Encoded Polyline Out Suite",  NULL,  NULL, out_encoded_polyline_tests};
