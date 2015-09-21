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

#include "../lwboundingcircle.c"

static void mbc_test(LWGEOM* g)
{
	LW_BOUNDINGCIRCLE result;
	CU_ASSERT_TRUE(lwgeom_calculate_mbc(g, &result));

	LWITERATOR it;
	lwiterator_create(g, &it);

	POINT2D p;
	POINT4D p4;
	while (lwiterator_has_next(&it))
	{
		lwiterator_next(&it, &p4);
		p.x = p4.x;
		p.y = p4.y;

		CU_ASSERT_TRUE(distance2d_pt_pt(&(result.centre), &p) <= result.radius);
	}

	lwiterator_destroy(&it);
}

static void basic_test(void)
{
	uint32_t i;

	char* inputs[] =
	{
		"POLYGON((26426 65078,26531 65242,26075 65136,26096 65427,26426 65078))",
		"POINT (17 253)",
		"TRIANGLE ((0 0, 10 0, 10 10, 0 0))",
		"LINESTRING (17 253, -44 28, 33 11, 26 44)",
		"MULTIPOINT ((0 0), (0 0), (0 0), (0 0))",
		"POLYGON ((0 0, 1 0, 1 1, 0 1, 0 0))"
	};

	for (i = 0; i < sizeof(inputs)/sizeof(LWGEOM*); i++)
	{
		LWGEOM* input = lwgeom_from_wkt(inputs[i], LW_PARSER_CHECK_NONE);
		mbc_test(input);
		lwgeom_free(input);
	}

}

/*
 ** Used by test harness to register the tests in this file.
 */
void minimum_bounding_circle_suite_setup(void);
void minimum_bounding_circle_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("minimum_bounding_circle", NULL, NULL);
	PG_ADD_TEST(suite, basic_test);
}
