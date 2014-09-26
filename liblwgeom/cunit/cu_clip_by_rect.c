/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright (C) 2014 Sandro Santilli <strk@keybit.net>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "CUnit/Basic.h"
#include "cu_tester.h"

#include "liblwgeom.h"
#include "liblwgeom_internal.h"

static void test_lwgeom_clip_by_rect(void)
{
#if POSTGIS_GEOS_VERSION >= 35
	LWGEOM *in, *out;
	const char *wkt;
	char *tmp;

	/* Because i don't trust that much prior tests...  ;) */
	cu_error_msg_reset();

	wkt = "LINESTRING(0 0, 5 5, 10 0)";
	in = lwgeom_from_wkt(wkt, LW_PARSER_CHECK_NONE);
	out = lwgeom_clip_by_rect(in, 5, 0, 10, 10);
	tmp = lwgeom_to_ewkt(out);
	/* printf("%s\n", tmp); */
	CU_ASSERT_STRING_EQUAL("LINESTRING(5 5,10 0)", tmp)
	lwfree(tmp); lwgeom_free(out); lwgeom_free(in);

	wkt = "LINESTRING EMPTY";
	in = lwgeom_from_wkt(wkt, LW_PARSER_CHECK_NONE);
	out = lwgeom_clip_by_rect(in, 5, 0, 10, 10);
	tmp = lwgeom_to_ewkt(out);
	/* printf("%s\n", tmp); */
	CU_ASSERT_STRING_EQUAL(wkt, tmp)
	lwfree(tmp); lwgeom_free(out); lwgeom_free(in);

#endif /* POSTGIS_GEOS_VERSION >= 35 */
}

/*
** Used by test harness to register the tests in this file.
*/
CU_TestInfo clip_by_rect_tests[] =
{
	PG_TEST(test_lwgeom_clip_by_rect),
	CU_TEST_INFO_NULL
};
CU_SuiteInfo clip_by_rect_suite = {"clip_by_rect",  NULL,  NULL, clip_by_rect_tests};
