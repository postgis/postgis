/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright 2015 Daniel Baston
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <stdlib.h>

#include "CUnit/Basic.h"

#include "../lwgeom_log.h"
#include "../lwgeom_geos.h"
#include "cu_tester.h"

static void assert_all_results_found(LWGEOM** results, size_t num_outputs, LWGEOM** expected, size_t num_expected_outputs)
{
	size_t i, j;

	char found_equal = 0;
	for (i = 0; i < num_outputs; i++)
	{
		for (j = 0; j < num_expected_outputs; j++)
		{
			if (lwgeom_same(results[i], expected[j]))
			{
				found_equal = 1;
				break;
			}
		}

		CU_ASSERT_TRUE(found_equal);
	}
}

static GEOSGeometry** LWGEOMARRAY2GEOS(LWGEOM** lw_array, size_t num_geoms)
{
	size_t i;
	GEOSGeometry** geos_geoms = lwalloc(num_geoms * sizeof(GEOSGeometry*));

	for (i = 0; i < num_geoms; i++)
	{
		geos_geoms[i] = LWGEOM2GEOS(lw_array[i], 0);
	}

	return geos_geoms;
}

static LWGEOM** GEOSARRAY2LWGEOM(GEOSGeometry** geos_array, size_t num_geoms)
{
	size_t i;
	LWGEOM** lw_geoms = lwalloc(num_geoms * sizeof(LWGEOM*));

	for (i = 0; i < num_geoms; i++)
	{
		lw_geoms[i] = GEOS2LWGEOM(geos_array[i], 0);
	}

	return lw_geoms;
}

static LWGEOM** WKTARRAY2LWGEOM(char** wkt_array, size_t num_geoms)
{
	size_t i;

	LWGEOM** lw_geoms = lwalloc(num_geoms * sizeof(LWGEOM*));

	for (i = 0; i < num_geoms; i++)
	{
		lw_geoms[i] = lwgeom_from_wkt(wkt_array[i], LW_PARSER_CHECK_NONE);
	}

	return lw_geoms;
}

static void perform_cluster_intersecting_test(char** wkt_inputs, uint32_t num_inputs, char** wkt_outputs, uint32_t num_outputs)
{
	GEOSGeometry** geos_results;
	LWGEOM** lw_results;
	uint32_t num_clusters;

	LWGEOM** expected_outputs = WKTARRAY2LWGEOM(wkt_outputs, num_outputs);
	LWGEOM** lw_inputs = WKTARRAY2LWGEOM(wkt_inputs, num_inputs);
	GEOSGeometry** geos_inputs = LWGEOMARRAY2GEOS(lw_inputs, num_inputs);

	cluster_intersecting(geos_inputs, num_inputs, &geos_results, &num_clusters);
	CU_ASSERT_EQUAL(num_outputs, num_clusters);

	lw_results = GEOSARRAY2LWGEOM(geos_results, num_clusters);

	assert_all_results_found(lw_results, num_clusters, expected_outputs, num_outputs);

	/* Cleanup */
	uint32_t i;
	for(i = 0; i < num_clusters; i++)
	{
		GEOSGeom_destroy(geos_results[i]);
	}
	lwfree(geos_inputs);
	lwfree(geos_results);

	for(i = 0; i < num_outputs; i++)
	{
		lwgeom_free(expected_outputs[i]);
		lwgeom_free(lw_results[i]);
	}
	lwfree(expected_outputs);
	lwfree(lw_results);

	for(i = 0; i < num_inputs; i++)
	{
		lwgeom_free(lw_inputs[i]);
	}
	lwfree(lw_inputs);
}

static void perform_cluster_within_distance_test(double tolerance, char** wkt_inputs, uint32_t num_inputs, char** wkt_outputs, uint32_t num_outputs)
{
	LWGEOM** lw_results;
	uint32_t num_clusters;

	LWGEOM** expected_outputs = WKTARRAY2LWGEOM(wkt_outputs, num_outputs);
	LWGEOM** lw_inputs = WKTARRAY2LWGEOM(wkt_inputs, num_inputs);

	cluster_within_distance(lw_inputs, num_inputs, tolerance, &lw_results, &num_clusters);

	CU_ASSERT_EQUAL(num_outputs, num_clusters);

	assert_all_results_found(lw_results, num_clusters, expected_outputs, num_outputs);

	/* Cleanup */
	uint32_t i;
	for(i = 0; i < num_outputs; i++)
	{
		lwgeom_free(expected_outputs[i]);
		lwgeom_free(lw_results[i]);
	}
	lwfree(lw_results);
	lwfree(expected_outputs);
	lwfree(lw_inputs);
}

static int init_geos_cluster_suite(void)
{
	initGEOS(lwnotice, lwgeom_geos_error);
	return 0;
}

static int clean_geos_cluster_suite(void)
{
	finishGEOS();
	return 0;
}

static void basic_test(void)
{
	char* a = "LINESTRING (0 0, 1 1)";
	char* b = "LINESTRING (1 1, 2 2)";
	char* c = "LINESTRING (5 5, 6 6)";

	char* wkt_inputs_a[] = {a, b, c};
	char* wkt_inputs_b[] = {b, c, a};
	char* wkt_inputs_c[] = {c, a, b};

	char* expected_outputs_a[] = { "GEOMETRYCOLLECTION(LINESTRING (0 0, 1 1), LINESTRING (1 1, 2 2))",
	                               "GEOMETRYCOLLECTION(LINESTRING (5 5, 6 6))"
	                             };

	char* expected_outputs_b[] = { "GEOMETRYCOLLECTION(LINESTRING (1 1, 2 2), LINESTRING (0 0, 1 1))",
	                               "GEOMETRYCOLLECTION(LINESTRING (5 5, 6 6))"
	                             };

	char* expected_outputs_c[] = { "GEOMETRYCOLLECTION(LINESTRING (0 0, 1 1), LINESTRING (1 1, 2 2))",
	                               "GEOMETRYCOLLECTION(LINESTRING (5 5, 6 6))"
	                             };

	perform_cluster_intersecting_test(wkt_inputs_a, 3, expected_outputs_a, 2);
	perform_cluster_intersecting_test(wkt_inputs_b, 3, expected_outputs_b, 2);
	perform_cluster_intersecting_test(wkt_inputs_c, 3, expected_outputs_c, 2);

	perform_cluster_within_distance_test(0, wkt_inputs_a, 3, expected_outputs_a, 2);
	perform_cluster_within_distance_test(0, wkt_inputs_b, 3, expected_outputs_b, 2);
	perform_cluster_within_distance_test(0, wkt_inputs_c, 3, expected_outputs_c, 2);
}

static void basic_distance_test(void)
{
	char* a = "LINESTRING (0 0, 1 1)";
	char* b = "LINESTRING (1 1, 2 2)";
	char* c = "LINESTRING (5 5, 6 6)";

	char* wkt_inputs[] = {a, b, c};

	char* expected_outputs_all[] = {"GEOMETRYCOLLECTION(LINESTRING(0 0, 1 1), LINESTRING(1 1, 2 2), LINESTRING(5 5, 6 6))"};
	char* expected_outputs_partial[] = {"GEOMETRYCOLLECTION(LINESTRING(0 0, 1 1), LINESTRING(1 1, 2 2))",
	                                    "GEOMETRYCOLLECTION(LINESTRING(5 5, 6 6))"
	                                   };

	perform_cluster_within_distance_test(0, wkt_inputs, 3, expected_outputs_partial, 2);
	perform_cluster_within_distance_test(sqrt(18) - 0.0000001, wkt_inputs, 3, expected_outputs_partial, 2);
	perform_cluster_within_distance_test(sqrt(18) + 0.0000001, wkt_inputs, 3, expected_outputs_all, 1);
}

static void nonsequential_test(void)
{
	char* wkt_inputs[] = { "LINESTRING (0 0, 1 1)",
	                       "LINESTRING (1 1, 2 2)",
	                       "LINESTRING (5 5, 6 6)",
	                       "LINESTRING (5 5, 4 4)",
	                       "LINESTRING (3 3, 2 2)",
	                       "LINESTRING (3 3, 4 4)"
	                     };

	char* expected_outputs[] = { "GEOMETRYCOLLECTION (LINESTRING (0 0, 1 1), LINESTRING (1 1, 2 2), LINESTRING (5 5, 6 6), LINESTRING (5 5, 4 4), LINESTRING (3 3, 2 2), LINESTRING (3 3, 4 4))" };

	perform_cluster_intersecting_test(wkt_inputs, 6, expected_outputs, 1);
	perform_cluster_within_distance_test(0, wkt_inputs, 6, expected_outputs, 1);
}

static void single_input_test(void)
{
	char* wkt_inputs[] = { "POINT (0 0)" };
	char* expected_outputs[] = { "GEOMETRYCOLLECTION (POINT (0 0))" };

	perform_cluster_intersecting_test(wkt_inputs, 1, expected_outputs, 1);
	perform_cluster_within_distance_test(1, wkt_inputs, 1, expected_outputs, 1);
}

static void empty_inputs_test(void)
{
	char* wkt_inputs[] = { "POLYGON EMPTY", "LINESTRING EMPTY"};
	char* expected_outputs[] = { "GEOMETRYCOLLECTION( LINESTRING EMPTY )", "GEOMETRYCOLLECTION( POLYGON EMPTY )" };

	perform_cluster_intersecting_test(wkt_inputs, 2, expected_outputs, 2);
	perform_cluster_within_distance_test(1, wkt_inputs, 2, expected_outputs, 2);
}

static void multipoint_test(void)
{
	/* See #3433 */
	char* wkt_inputs_mp[] = { "MULTIPOINT ((0 0), (0 1))", "POINT (0 0)"};
	char* expected_outputs_mp[] = { "GEOMETRYCOLLECTION(MULTIPOINT ((0 0), (0 1)), POINT (0 0))"};

	char* wkt_inputs_gc[] = { "GEOMETRYCOLLECTION (POINT (0 0), POINT (0 1))", "POINT (0 0)"};
	char* expected_outputs_gc[] = { "GEOMETRYCOLLECTION(GEOMETRYCOLLECTION (POINT (0 0), POINT (0 1)), POINT (0 0))"};

	char* wkt_inputs_pt[] = { "POINT (3 3)", "POINT (3 3)"};
	char* expected_outputs_pt[] = { "GEOMETRYCOLLECTION(POINT (3 3), POINT (3 3))"};

	perform_cluster_intersecting_test(wkt_inputs_mp, 2, expected_outputs_mp, 1);
	perform_cluster_intersecting_test(wkt_inputs_gc, 2, expected_outputs_gc, 1);
	perform_cluster_intersecting_test(wkt_inputs_pt, 2, expected_outputs_pt, 1);
}

struct dbscan_test_info {
	double eps;
	uint32_t min_points;
	uint32_t num_geoms;
	char** wkt_inputs;
	uint32_t* expected_ids;
	int* expected_in_cluster;
};

static void do_dbscan_test(struct dbscan_test_info test)
{
	LWGEOM** geoms = WKTARRAY2LWGEOM(test.wkt_inputs, test.num_geoms);
	UNIONFIND* uf = UF_create(test.num_geoms);
	uint32_t* ids;
	char* in_a_cluster;
	uint32_t i;

	union_dbscan(geoms, test.num_geoms, uf, test.eps, test.min_points, &in_a_cluster);
	ids = UF_get_collapsed_cluster_ids(uf, in_a_cluster);

	for (i = 0; i < test.num_geoms; i++)
	{
		ASSERT_INT_EQUAL(in_a_cluster[i], test.expected_in_cluster[i]);
		if (in_a_cluster[i])
			ASSERT_INT_EQUAL(ids[i], test.expected_ids[i]);
	}

	UF_destroy(uf);
	for (i = 0; i < test.num_geoms; i++)
	{
		lwgeom_free(geoms[i]);
	}
	lwfree(geoms);
	lwfree(in_a_cluster);
	lwfree(ids);
}

static void dbscan_test(void)
{
	struct dbscan_test_info test;
	char* wkt_inputs[] = { "POINT (0 0)", "POINT (-1 0)", "POINT (-1 -0.1)", "POINT (-1 0.1)",
		                   "POINT (1 0)",
						   "POINT (2 0)", "POINT (3  0)", "POINT ( 3 -0.1)", "POINT ( 3 0.1)" };
	/* Although POINT (1 0) and POINT (2 0) are within eps distance of each other,
	 * they do not connect the two clusters because POINT (1 0) is not a core point.
	 * See #3572
	 */
	test.eps = 1.01;
	test.min_points = 5;
	uint32_t expected_ids[] =   { 0, 0, 0, 0, 0, 1, 1, 1, 1, 1 };
	int expected_in_cluster[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
	test.num_geoms = sizeof(wkt_inputs) / sizeof(char*);

	test.expected_ids = expected_ids;
	test.wkt_inputs = wkt_inputs;
	test.expected_in_cluster = expected_in_cluster;
	do_dbscan_test(test);
}

static void dbscan_test_3612a(void)
{
	struct dbscan_test_info test;
	char* wkt_inputs[] = { "POINT (1 1)" };

	test.eps = 0.0;
	test.min_points = 5;
	uint32_t expected_ids[] = { rand() };
	int expected_in_cluster[] = { 0 };
	test.num_geoms = sizeof(wkt_inputs) / sizeof(char*);

	test.expected_ids = expected_ids;
	test.expected_in_cluster = expected_in_cluster;
	test.wkt_inputs = wkt_inputs;
	do_dbscan_test(test);
}

static void dbscan_test_3612b(void)
{
	struct dbscan_test_info test;
	char* wkt_inputs[] = { "POINT (1 1)" };

	test.eps = 0.0;
	test.min_points = 1;
	uint32_t expected_ids[]   = { 0 };
	int expected_in_cluster[] = { 1 };
	test.num_geoms = sizeof(wkt_inputs) / sizeof(char*);

	test.expected_ids = expected_ids;
	test.expected_in_cluster = expected_in_cluster;
	test.wkt_inputs = wkt_inputs;
	do_dbscan_test(test);
}

static void dbscan_test_3612c(void)
{
	struct dbscan_test_info test;
	char* wkt_inputs[] = { "POLYGONM((-71.1319 42.2503 1,-71.132 42.2502 3,-71.1323 42.2504 -2,-71.1322 42.2505 1,-71.1319 42.2503 0))",
						   "POLYGONM((-71.1319 42.2512 0,-71.1318 42.2511 20,-71.1317 42.2511 -20,-71.1317 42.251 5,-71.1317 42.2509 4,-71.132 42.2511 6,-71.1319 42.2512 30))" };
	test.eps = 20.1;
	test.min_points = 5;
	uint32_t expected_ids[]   = { rand(), rand() };
	int expected_in_cluster[] = { 0, 0 };
	test.num_geoms = sizeof(wkt_inputs) / sizeof(char*);

	test.expected_ids = expected_ids;
	test.expected_in_cluster = expected_in_cluster;
	test.wkt_inputs = wkt_inputs;
	do_dbscan_test(test);
}

void geos_cluster_suite_setup(void);
void geos_cluster_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("clustering", init_geos_cluster_suite, clean_geos_cluster_suite);
	PG_ADD_TEST(suite, basic_test);
	PG_ADD_TEST(suite, nonsequential_test);
	PG_ADD_TEST(suite, basic_distance_test);
	PG_ADD_TEST(suite, single_input_test);
	PG_ADD_TEST(suite, empty_inputs_test);
	PG_ADD_TEST(suite, multipoint_test);
	PG_ADD_TEST(suite, dbscan_test);
	PG_ADD_TEST(suite, dbscan_test_3612a);
	PG_ADD_TEST(suite, dbscan_test_3612b);
	PG_ADD_TEST(suite, dbscan_test_3612c);
}
