/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright (C) 2015 Daniel Baston <dbaston@gmail.com>
 * Copyright (C) 2012 Sandro Santilli <strk@kbt.io>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "CUnit/Basic.h"
#include "cu_tester.h"

#include "liblwgeom_internal.h"
#include "../lwgeom_geos.h"

static void
test_lwgeom_delaunay_triangulation(void)
{
	LWGEOM *in, *tmp, *out;
	char *wkt, *exp_wkt;

	/* Because i don't trust that much prior tests...  ;) */
	cu_error_msg_reset();

	in = lwgeom_from_wkt("MULTIPOINT(10 0, 20 0, 5 5)", LW_PARSER_CHECK_NONE);

	tmp = lwgeom_delaunay_triangulation(in, 0, 0);
	lwgeom_free(in);
	out = lwgeom_normalize(tmp);
	lwgeom_free(tmp);

	wkt = lwgeom_to_ewkt(out);
	lwgeom_free(out);

	exp_wkt = "GEOMETRYCOLLECTION(POLYGON((5 5,20 0,10 0,5 5)))";
	if (strcmp(wkt, exp_wkt))
		fprintf(stderr, "\nExp:  %s\nObt:  %s\n", exp_wkt, wkt);

	CU_ASSERT_STRING_EQUAL(wkt, exp_wkt);
	lwfree(wkt);
}

static void
test_lwgeom_voronoi_diagram(void)
{
	LWGEOM *in = lwgeom_from_wkt("MULTIPOINT(4 4, 5 5, 6 6)", LW_PARSER_CHECK_NONE);

	LWGEOM *out_boundaries = lwgeom_voronoi_diagram(in, NULL, 0, 0);
	LWGEOM *out_lines = lwgeom_voronoi_diagram(in, NULL, 0, 1);

	/* For boundaries we get a generic LWCOLLECTION */
	CU_ASSERT_EQUAL(COLLECTIONTYPE, lwgeom_get_type(out_boundaries));
	/* For lines we get a MULTILINETYPE */
	CU_ASSERT_EQUAL(MULTILINETYPE, lwgeom_get_type(out_lines));

	lwgeom_free(in);
	lwgeom_free(out_boundaries);
	lwgeom_free(out_lines);
}

static void
test_lwgeom_voronoi_diagram_custom_envelope(void)
{
	LWGEOM *in = lwgeom_from_wkt("MULTIPOINT(4 4, 5 5, 6 6)", LW_PARSER_CHECK_NONE);
	LWGEOM *for_extent = lwgeom_from_wkt("LINESTRING (-10 -10, 10 10)", LW_PARSER_CHECK_NONE);
	const GBOX *clipping_extent = lwgeom_get_bbox(for_extent);

	LWGEOM *out = lwgeom_voronoi_diagram(in, clipping_extent, 0, 0);
	const GBOX *output_extent = lwgeom_get_bbox(out);

	CU_ASSERT_TRUE(gbox_same(clipping_extent, output_extent));

	lwgeom_free(in);
	lwgeom_free(for_extent);
	lwgeom_free(out);
}

static void
assert_empty_diagram(char *wkt, double tolerance)
{
	LWGEOM *in = lwgeom_from_wkt(wkt, LW_PARSER_CHECK_NONE);
	LWGEOM *out = lwgeom_voronoi_diagram(in, NULL, tolerance, 0);

	CU_ASSERT_TRUE(lwgeom_is_collection(out));
	CU_ASSERT_EQUAL(COLLECTIONTYPE, lwgeom_get_type(out));

	lwgeom_free(in);
	lwgeom_free(out);
}

static void
test_lwgeom_voronoi_diagram_expected_empty(void)
{
	assert_empty_diagram("POLYGON EMPTY", 0);
	assert_empty_diagram("POINT (1 2)", 0);

	/* This one produces an empty diagram because our two unqiue points
	 * collapse onto one after considering the tolerance. */
	assert_empty_diagram("MULTIPOINT (0 0, 0 0.00001)", 0.001);
}

static int
clean_geos_triangulate_suite(void)
{
	finishGEOS();
	return 0;
}

/*
** Used by test harness to register the tests in this file.
*/
void triangulate_suite_setup(void);
void
triangulate_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("triangulate", NULL, clean_geos_triangulate_suite);
	PG_ADD_TEST(suite, test_lwgeom_delaunay_triangulation);
	PG_ADD_TEST(suite, test_lwgeom_voronoi_diagram);
	PG_ADD_TEST(suite, test_lwgeom_voronoi_diagram_expected_empty);
	PG_ADD_TEST(suite, test_lwgeom_voronoi_diagram_custom_envelope);
}
