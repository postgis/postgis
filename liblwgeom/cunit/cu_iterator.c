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

#include "../liblwgeom_internal.h"

char* inputs[] =
{
	"POINT (17 253)",
	"POINT Z (17 253 018)",
	"TRIANGLE ((0 0, 10 0, 10 10, 0 0))",
	"LINESTRING (17 253, -44 28, 33 11, 26 44)",
	"LINESTRING M (17 253 0, -44 28 1, 33 11 2, 26 44 3)",
	"POLYGON((26426 65078,26531 65242,26075 65136,26096 65427,26426 65078))",
	"MULTIPOINT ((1 1), (1 1))",
	"MULTILINESTRING Z ((1 1 0, 2 2 0), (3 3 1, 4 4 1))",
	"MULTIPOLYGON (((0 0, 10 0, 10 10, 0 10, 0 0), (1 1, 1 2, 2 2, 2 1, 1 1)), ((20 20, 20 30, 30 30, 20 20)))",
	"POINT EMPTY",
	"LINESTRING M EMPTY",
	"POLYGON Z EMPTY",
	"GEOMETRYCOLLECTION EMPTY",
	"GEOMETRYCOLLECTION (MULTIPOINT ((14 80), (22 12)))",
	"GEOMETRYCOLLECTION (POINT (3 7), LINESTRING (0 0, 14 3), GEOMETRYCOLLECTION(POINT (2 8)))",
	"GEOMETRYCOLLECTION (POINT (3 7), GEOMETRYCOLLECTION(MULTIPOINT ((2 8))))",
	"GEOMETRYCOLLECTION (POINT (3 7), GEOMETRYCOLLECTION(LINESTRING (2 8, 4 3), POLYGON EMPTY, MULTIPOINT ((2 8), (17 3), EMPTY)))",
	"CIRCULARSTRING(0 0,2 0, 2 1, 2 3, 4 3)",
	"COMPOUNDCURVE(CIRCULARSTRING(0 0,2 0, 2 1, 2 3, 4 3),(4 3, 4 5, 1 4, 0 0))",
	"MULTICURVE((0 0, 5 5),CIRCULARSTRING(4 0, 4 4, 8 4))",
	"CURVEPOLYGON(COMPOUNDCURVE(CIRCULARSTRING(0 0,2 0, 2 1, 2 3, 4 3),(4 3, 4 5, 1 4, 0 0)), LINESTRING (0.1 0.1, 0.3 0.1, 0.3 0.3, 0.1 0.1) )",
	"MULTISURFACE(CURVEPOLYGON(CIRCULARSTRING(0 0, 4 0, 4 4, 0 4, 0 0),(1 1, 3 3, 3 1, 1 1)),((10 10, 14 12, 11 10, 10 10),(11 11, 11.5 11, 11 11.5, 11 11)))",
	"POLYHEDRALSURFACE( ((0 0 0, 0 0 1, 0 1 1, 0 1 0, 0 0 0)), ((0 0 0, 0 1 0, 1 1 0, 1 0 0, 0 0 0)), ((0 0 0, 1 0 0, 1 0 1, 0 0 1, 0 0 0)), ((1 1 0, 1 1 1, 1 0 1, 1 0 0, 1 1 0)), ((0 1 0, 0 1 1, 1 1 1, 1 1 0, 0 1 0)), ((0 0 1, 1 0 1, 1 1 1, 0 1 1, 0 0 1)) )",
	"TIN(((80 130,50 160,80 70,80 130)),((50 160,10 190,10 70,50 160)), ((80 70,50 160,10 70,80 70)),((120 160,120 190,50 160,120 160)), ((120 190,10 190,50 160,120 190)))"
};

static uint32_t
count_points_using_iterator(LWGEOM* g)
{
	POINT4D p;
	uint32_t count = 0;
	LWPOINTITERATOR* it = lwpointiterator_create(g);

	while (lwpointiterator_has_next(it))
	{
		CU_ASSERT_TRUE(lwpointiterator_next(it, &p));
		count++;
	}

	lwpointiterator_destroy(it);

	return count;
}

static void
test_point_count(void)
{
	char* types_visited = lwalloc(NUMTYPES * sizeof(char));
	memset(types_visited, LW_FALSE, NUMTYPES * sizeof(char));

	uint32_t i;
	for (i = 0; i < sizeof(inputs)/sizeof(char*); i++)
	{
		LWGEOM* input = lwgeom_from_wkt(inputs[i], LW_PARSER_CHECK_NONE);
		types_visited[lwgeom_get_type(input)] = LW_TRUE;

		uint32_t itercount = count_points_using_iterator(input);

		CU_ASSERT_EQUAL(lwgeom_count_vertices(input), itercount);

		lwgeom_free(input);
	}

	/* Assert that every valid LWGEOM type has been tested */
	for (i = 1; i < NUMTYPES; i++)
	{
		CU_ASSERT_TRUE(types_visited[i]);
	}

	lwfree(types_visited);
}

static void
test_cannot_modify_read_only(void)
{
	LWGEOM* input = lwgeom_from_wkt(inputs[0], LW_PARSER_CHECK_NONE);
	LWPOINTITERATOR* it = lwpointiterator_create(input);

	POINT4D p;
	p.x = 3.2;
	p.y = 4.8;

	CU_ASSERT_EQUAL(LW_FAILURE, lwpointiterator_modify_next(it, &p));

	lwgeom_free(input);
	lwpointiterator_destroy(it);
}

static void
test_modification(void)
{
	uint32_t i;
	uint32_t j = 0;

	for (i = 0; i < sizeof(inputs)/sizeof(char*); i++)
	{
		LWGEOM* input = lwgeom_from_wkt(inputs[i], LW_PARSER_CHECK_NONE);
		LWPOINTITERATOR* it1 = lwpointiterator_create_rw(input);
		LWPOINTITERATOR* it2 = lwpointiterator_create(input);;

		while (lwpointiterator_has_next(it1))
		{
			/* Make up a coordinate, assign it to the next spot in it1,
			 * read it from it2 to verify that it was assigned correctly. */
			POINT4D p1, p2;
			p1.x = sqrt(j++);
			p1.y = sqrt(j++);
			p1.z = sqrt(j++);
			p1.m = sqrt(j++);

			CU_ASSERT_TRUE(lwpointiterator_modify_next(it1, &p1));
			CU_ASSERT_TRUE(lwpointiterator_next(it2, &p2));

			CU_ASSERT_EQUAL(p1.x, p2.x);
			CU_ASSERT_EQUAL(p1.y, p2.y);

			if (lwgeom_has_z(input))
				CU_ASSERT_EQUAL(p1.z, p2.z);

			if (lwgeom_has_m(input))
				CU_ASSERT_EQUAL(p1.m, p2.m);
		}

		lwgeom_free(input);

		lwpointiterator_destroy(it1);
		lwpointiterator_destroy(it2);
	}
}

static void
test_no_memory_leaked_when_iterator_is_partially_used(void)
{
	LWGEOM* g = lwgeom_from_wkt("GEOMETRYCOLLECTION (POINT (3 7), GEOMETRYCOLLECTION(LINESTRING (2 8, 4 3), POLYGON EMPTY, MULTIPOINT ((2 8), (17 3), EMPTY)))", LW_PARSER_CHECK_NONE);

	LWPOINTITERATOR* it = lwpointiterator_create(g);
	lwpointiterator_next(it, NULL);
	lwpointiterator_next(it, NULL);

	lwpointiterator_destroy(it);
	lwgeom_free(g);
}

static void
test_mixed_rw_access(void)
{
	uint32_t i = 0;
	LWGEOM* g = lwgeom_from_wkt("GEOMETRYCOLLECTION (POINT (3 7), GEOMETRYCOLLECTION(LINESTRING (2 8, 4 3), POLYGON EMPTY, MULTIPOINT ((2 8), (17 3), EMPTY)))", LW_PARSER_CHECK_NONE);
	LWPOINTITERATOR* it1 = lwpointiterator_create_rw(g);
	LWPOINTITERATOR* it2 = lwpointiterator_create(g);

	/* Flip the coordinates of the 3rd point */
	while(lwpointiterator_has_next(it1))
	{
		if (i == 2)
		{
			POINT4D p;
			double tmp;

			lwpointiterator_peek(it1, &p);
			tmp = p.x;
			p.x = p.y;
			p.y = tmp;

			lwpointiterator_modify_next(it1, &p);
		}
		else
		{
			lwpointiterator_next(it1, NULL);
		}
		i++;
	}
	CU_ASSERT_EQUAL(5, i); /* Every point was visited */
	lwpointiterator_destroy(it1);

	/* Verify that the points are as expected */
	POINT2D points[] =
	{
		{ .x = 3, .y = 7 },
		{ .x = 2, .y = 8 },
		{ .x = 3, .y = 4 },
		{ .x = 2, .y = 8 },
		{ .x = 17, .y = 3}
	};

	for (i = 0; lwpointiterator_has_next(it2); i++)
	{
		POINT4D p;

		lwpointiterator_next(it2, &p);

		CU_ASSERT_EQUAL(p.x, points[i].x);
		CU_ASSERT_EQUAL(p.y, points[i].y);
	}

	lwpointiterator_destroy(it2);
	lwgeom_free(g);
}

static void
test_ordering(void)
{
	uint32_t i = 0;
	LWGEOM* g = lwgeom_from_wkt("GEOMETRYCOLLECTION (POLYGON ((0 0, 0 10, 10 10, 0 10, 0 0), (1 1, 1 2, 2 2, 2 1, 1 1)), MULTIPOINT((4 4), (3 3)))", LW_PARSER_CHECK_NONE);

	POINT2D points[] = { {.x = 0,  .y = 0},
		{.x = 0,  .y = 10},
		{.x = 10, .y = 10},
		{.x = 0,  .y = 10},
		{.x = 0,  .y = 0},
		{.x = 1,  .y = 1},
		{.x = 1,  .y = 2},
		{.x = 2,  .y = 2},
		{.x = 2,  .y = 1},
		{.x = 1,  .y = 1},
		{.x = 4,  .y = 4},
		{.x = 3,  .y = 3}
	};

	LWPOINTITERATOR* it = lwpointiterator_create(g);
	POINT4D p;

	for (i = 0; lwpointiterator_has_next(it); i++)
	{
		CU_ASSERT_EQUAL(LW_SUCCESS, lwpointiterator_next(it, &p));
		CU_ASSERT_EQUAL(p.x, points[i].x);
		CU_ASSERT_EQUAL(p.y, points[i].y);
	}

	lwpointiterator_destroy(it);
	lwgeom_free(g);
}

/*
** Used by test harness to register the tests in this file.
*/
void iterator_suite_setup(void);
void iterator_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("iterator", NULL, NULL);
	PG_ADD_TEST(suite, test_point_count);
	PG_ADD_TEST(suite, test_ordering);
	PG_ADD_TEST(suite, test_modification);
	PG_ADD_TEST(suite, test_mixed_rw_access);
	PG_ADD_TEST(suite, test_cannot_modify_read_only);
	PG_ADD_TEST(suite, test_no_memory_leaked_when_iterator_is_partially_used);
}
