/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright (C) 2014 Sandro Santilli <strk@kbt.io>
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

	wkt = "MULTIPOINT EMPTY";
	in = lwgeom_from_wkt(wkt, LW_PARSER_CHECK_NONE);
	out = lwgeom_clip_by_rect(in, 5, 0, 10, 10);
	tmp = lwgeom_to_ewkt(out);
	/* printf("%s\n", tmp); */
	CU_ASSERT_STRING_EQUAL(wkt, tmp)
	lwfree(tmp); lwgeom_free(out); lwgeom_free(in);

	wkt = "MULTIPOINT(0 0, 6 6, 7 5)";
	in = lwgeom_from_wkt(wkt, LW_PARSER_CHECK_NONE);
	out = lwgeom_clip_by_rect(in, 5, 0, 10, 10);
	tmp = lwgeom_to_ewkt(out);
	/* printf("%s\n", tmp); */
	CU_ASSERT_STRING_EQUAL("MULTIPOINT(6 6,7 5)", tmp)
	lwfree(tmp); lwgeom_free(out); lwgeom_free(in);

	/* Disjoint polygon */
	wkt = "POLYGON((311017 4773762,311016 4773749,311006 4773744,310990 4773748,310980 4773758,310985 4773771,311003 4773776,311017 4773762))";
	in = lwgeom_from_wkt(wkt, LW_PARSER_CHECK_NONE);
	out = lwgeom_clip_by_rect(in, -80, -80, 80, 80);
	//tmp = lwgeom_to_ewkt(out); printf("%s\n", tmp); lwfree(tmp);
	CU_ASSERT(lwgeom_is_empty(out));
	lwgeom_free(out); lwgeom_free(in);

	/* Returns NULL with an invalid polygon (line) */
	wkt = "POLYGON((1410 2055, 1410 2056, 1410 2057, 1410 2055))";
	in = lwgeom_from_wkt(wkt, LW_PARSER_CHECK_NONE);
	out = lwgeom_clip_by_rect(in, -8.000000, -8.000000, 2056.000000, 2056.000000);
	CU_ASSERT_PTR_NULL(out);
	lwgeom_free(in);
}

/*
** Used by test harness to register the tests in this file.
*/
void clip_by_rect_suite_setup(void);
void clip_by_rect_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("clip_by_rectangle", NULL, NULL);
	PG_ADD_TEST(suite, test_lwgeom_clip_by_rect);
}
