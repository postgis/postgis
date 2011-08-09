/**********************************************************************
 * $Id: cu_print.c 6160 2010-11-01 01:28:12Z pramsey $
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
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


static LWGEOM* lwgeom_from_text(const char *str)
{
	LWGEOM_PARSER_RESULT r;
	if( LW_FAILURE == lwgeom_parse_wkt(&r, (char*)str, PARSER_CHECK_NONE) )
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
	CU_ASSERT_STRING_EQUAL(wkt,"LINESTRING(0 0,1 1,1 1)");
	lwfree(wkt);

	ptarray_append_point(line->points, &p, LW_FALSE);
	wkt = lwgeom_to_text(lwline_as_lwgeom(line));
	CU_ASSERT_STRING_EQUAL(wkt,"LINESTRING(0 0,1 1,1 1)");
	lwfree(wkt);

	lwline_free(line);
}

static void test_ptarray_append_ptarray(void)
{
}


/*
** Used by the test harness to register the tests in this file.
*/
CU_TestInfo ptarray_tests[] =
{
	PG_TEST(test_ptarray_append_point),
	PG_TEST(test_ptarray_append_ptarray),
	CU_TEST_INFO_NULL
};
CU_SuiteInfo ptarray_suite = {"PointArray Suite", NULL, NULL, ptarray_tests };
