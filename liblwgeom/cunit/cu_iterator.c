/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 * Copyright 2015 Daniel Baston
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "CUnit/Basic.h"
#include "cu_tester.h"

#include "../lwiterator.c"

static uint32_t count_points_using_iterator(LWGEOM* g)
{
	uint32_t count = 0;

	LWITERATOR it;
	lwiterator_create(g, &it);

	POINT4D p;

	while (lwiterator_has_next(&it))
	{
		CU_ASSERT_TRUE(lwiterator_next(&it, &p));
		count++;
	}

	lwiterator_destroy(&it);

	return count;
}

static void basic_test(void)
{
	uint32_t i;

	char* inputs[] =
	{
		"POINT (17 253)",
		"TRIANGLE ((0 0, 10 0, 10 10, 0 0))",
		"LINESTRING (17 253, -44 28, 33 11, 26 44)",
		"POLYGON((26426 65078,26531 65242,26075 65136,26096 65427,26426 65078))",
		"MULTIPOINT ((1 1), (1 1))",
		"MULTILINESTRING ((1 1, 2 2), (3 3, 4 4))",
		"MULTIPOLYGON (((0 0, 10 0, 10 10, 0 10, 0 0), (1 1, 1 2, 2 2, 2 1, 1 1)), ((20 20, 20 30, 30 30, 20 20)))",
		"POINT EMPTY",
		"LINESTRING EMPTY",
		"POLYGON EMPTY",
		"GEOMETRYCOLLECTION EMPTY",
		"GEOMETRYCOLLECTION (MULTIPOINT ((14 80), (22 12)))",
		"GEOMETRYCOLLECTION (POINT (3 7), LINESTRING (0 0, 14 3), GEOMETRYCOLLECTION(POINT (2 8)))",
		"GEOMETRYCOLLECTION (POINT (3 7), GEOMETRYCOLLECTION(MULTIPOINT ((2 8))))",
		"GEOMETRYCOLLECTION (POINT (3 7), GEOMETRYCOLLECTION(LINESTRING (2 8, 4 3), POLYGON EMPTY, MULTIPOINT ((2 8), (17 3), EMPTY)))",
	};

	for (i = 0; i < sizeof(inputs)/sizeof(LWGEOM*); i++)
	{
		LWGEOM* input = lwgeom_from_wkt(inputs[i], LW_PARSER_CHECK_NONE);

		uint32_t itercount = count_points_using_iterator(input);

		CU_ASSERT_EQUAL(lwgeom_count_vertices(input), itercount);
		lwgeom_free(input);
	}
}

// TODO show that unsupported types are handled gracefully

/*
** Used by test harness to register the tests in this file.
*/
void iterator_suite_setup(void);
void iterator_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("iterator", NULL, NULL);
	PG_ADD_TEST(suite, basic_test);
}
