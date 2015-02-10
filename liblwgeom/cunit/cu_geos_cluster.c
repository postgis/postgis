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

#include "CUnit/Basic.h"

#include "../lwgeom_geos.h"
#include "cu_tester.h"

static void assert_all_results_found(LWGEOM** results, size_t num_outputs, LWGEOM** expected, size_t num_expected_outputs) {
    size_t i, j;

    char found_equal = 0;
    for (i = 0; i < num_outputs; i++) {
        for (j = 0; j < num_expected_outputs; j++) {
            if (lwgeom_same(results[i], expected[j])) {
                found_equal = 1;
                break;
            }
        }

        CU_ASSERT_TRUE(found_equal);
    }
}

static GEOSGeometry** LWGEOMARRAY2GEOS(LWGEOM** lw_array, size_t num_geoms) {
    size_t i;
    GEOSGeometry** geos_geoms = lwalloc(num_geoms * sizeof(GEOSGeometry*));

    for (i = 0; i < num_geoms; i++) {
        geos_geoms[i] = LWGEOM2GEOS(lw_array[i], 0);
    }

    return geos_geoms;
}

static LWGEOM** GEOSARRAY2LWGEOM(GEOSGeometry** geos_array, size_t num_geoms) {
    size_t i;
    LWGEOM** lw_geoms = lwalloc(num_geoms * sizeof(LWGEOM*));

    for (i = 0; i < num_geoms; i++) {
        lw_geoms[i] = GEOS2LWGEOM(geos_array[i], 0);
    }

    return lw_geoms;
}

static LWGEOM** WKTARRAY2LWGEOM(char** wkt_array, size_t num_geoms) {
    size_t i;

    LWGEOM** lw_geoms = lwalloc(num_geoms * sizeof(LWGEOM*));

    for (i = 0; i < num_geoms; i++) {
        lw_geoms[i] = lwgeom_from_wkt(wkt_array[i], LW_PARSER_CHECK_NONE);
    }

    return lw_geoms;
}

static void perform_cluster_intersecting_test(char** wkt_inputs, uint32_t num_inputs, char** wkt_outputs, uint32_t num_outputs) {
	GEOSGeometry** geos_results;
    LWGEOM** lw_results;
	uint32_t num_clusters;

	LWGEOM** expected_outputs = WKTARRAY2LWGEOM(wkt_outputs, num_outputs);
    GEOSGeometry** geos_inputs = LWGEOMARRAY2GEOS(WKTARRAY2LWGEOM(wkt_inputs, num_inputs), num_inputs);

	cluster_intersecting(geos_inputs, num_inputs, &geos_results, &num_clusters);
	CU_ASSERT_EQUAL(num_outputs, num_clusters);

    lw_results = GEOSARRAY2LWGEOM(geos_results, num_clusters);

    assert_all_results_found(lw_results, num_clusters, expected_outputs, num_outputs);
}

static void perform_cluster_within_distance_test(double tolerance, char** wkt_inputs, uint32_t num_inputs, char** wkt_outputs, uint32_t num_outputs) {
    LWGEOM** lw_results;
	uint32_t num_clusters;

	LWGEOM** expected_outputs = WKTARRAY2LWGEOM(wkt_outputs, num_outputs);
    LWGEOM** lw_inputs = WKTARRAY2LWGEOM(wkt_inputs, num_inputs);

	cluster_within_distance(lw_inputs, num_inputs, tolerance, &lw_results, &num_clusters);

	CU_ASSERT_EQUAL(num_outputs, num_clusters);

    int i;
    for(i=0; i < num_clusters; i++) {
        printf("%s\n", lwgeom_to_ewkt(lw_results[i]));
    }
    assert_all_results_found(lw_results, num_clusters, expected_outputs, num_outputs);
}


static int init_geos_cluster_suite(void)
{
	initGEOS(lwnotice, lwgeom_geos_error);
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
		                       "GEOMETRYCOLLECTION(LINESTRING (5 5, 6 6))" };

	char* expected_outputs_b[] = { "GEOMETRYCOLLECTION(LINESTRING (1 1, 2 2), LINESTRING (0 0, 1 1))",
		                       "GEOMETRYCOLLECTION(LINESTRING (5 5, 6 6))" };

	char* expected_outputs_c[] = { "GEOMETRYCOLLECTION(LINESTRING (0 0, 1 1), LINESTRING (1 1, 2 2))",
		                       "GEOMETRYCOLLECTION(LINESTRING (5 5, 6 6))" };
		
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
                                      "GEOMETRYCOLLECTION(LINESTRING(5 5, 6 6))"};

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
                               "LINESTRING (3 3, 4 4)"};

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

CU_TestInfo geos_cluster_tests[] =
{
	PG_TEST(basic_test),
	PG_TEST(nonsequential_test),
	PG_TEST(basic_distance_test),
	PG_TEST(single_input_test),
	CU_TEST_INFO_NULL
};

CU_SuiteInfo geos_cluster_suite = {"Clustering",  init_geos_cluster_suite,  NULL, geos_cluster_tests};

