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

static void do_encoded_polyline_test(char * in, int precision, char * out)
{
	LWGEOM *g;
	char * h;
	size_t size;

	g = lwgeom_from_encoded_polyline(in, precision);
	h = lwgeom_to_wkt(g, WKT_EXTENDED, 15, &size);

	if (strcmp(h, out))
		fprintf(stderr, "\nIn:   %s\nOut:  %s\nTheo: %s\n", in, h, out);

	CU_ASSERT_STRING_EQUAL(h, out);

	lwgeom_free(g);
	if ( h ) lwfree(h);
}

static void in_encoded_polyline_test_geoms(void)
{
	do_encoded_polyline_test(
	    "_p~iF~ps|U_ulLnnqC_mqNvxq`@",
	    5,
			"SRID=4326;LINESTRING(-120.2 38.5,-120.95 40.7,-126.453 43.252)");
}

static void in_encoded_polyline_test_precision(void)
{
	do_encoded_polyline_test(
			"kj_~}U_wvjjw@sohih@rtnkMke}ts^zod{gB",
	    7,
	    "SRID=4326;LINESTRING(-120.2 38.53354789,-120.95446664 40.7,-126.45322 -120.954466644666)");
}

/*
** Used by test harness to register the tests in this file.
*/
CU_TestInfo in_encoded_polyline_tests[] =
{
	PG_TEST(in_encoded_polyline_test_geoms),
	PG_TEST(in_encoded_polyline_test_precision),
	CU_TEST_INFO_NULL
};
CU_SuiteInfo in_encoded_polyline_suite = {"Encoded Polyline In Suite",  NULL,  NULL, in_encoded_polyline_tests};
