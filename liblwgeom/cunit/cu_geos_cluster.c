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

static void perform_clustering_test(char** wkt_inputs, uint32_t num_inputs, char** wkt_outputs, uint32_t num_outputs) {
	size_t i, j;

	GEOSGeometry** geos_results;
	uint32_t num_clusters;

	GEOSGeometry** geos_inputs = lwalloc(num_inputs * sizeof(GEOSGeometry*));
	LWGEOM** expected_outputs = lwalloc(num_outputs * sizeof(LWGEOM*));

	for (i = 0; i < num_inputs; i++) {
		geos_inputs[i] = LWGEOM2GEOS(lwgeom_from_wkt(wkt_inputs[i], LW_PARSER_CHECK_NONE), 0);
	}

	for (i = 0; i < num_outputs; i++) {
		expected_outputs[i] = lwgeom_from_wkt(wkt_outputs[i], LW_PARSER_CHECK_NONE);
	}

	cluster_intersecting(geos_inputs, num_inputs, &geos_results, &num_clusters);

	CU_ASSERT_EQUAL(num_outputs, num_clusters);

	for(i = 0; i < num_clusters; i++) {
		LWGEOM* result = GEOS2LWGEOM(geos_results[i], 0);

		char found_equal = 0;
		for (j = 0; j < num_outputs; j++) {
			if (lwgeom_same(result, expected_outputs[j])) {
				found_equal = 1;
				break;
			}
		}

		CU_ASSERT_TRUE(found_equal);
	}
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
		
	perform_clustering_test(wkt_inputs_a, 3, expected_outputs_a, 2);
	perform_clustering_test(wkt_inputs_b, 3, expected_outputs_b, 2);
	perform_clustering_test(wkt_inputs_c, 3, expected_outputs_c, 2);
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
		
	perform_clustering_test(wkt_inputs, 6, expected_outputs, 1);
}

CU_TestInfo geos_cluster_tests[] =
{
	PG_TEST(basic_test),
	PG_TEST(nonsequential_test),
	CU_TEST_INFO_NULL
};

CU_SuiteInfo geos_cluster_suite = {"Clustering",  init_geos_cluster_suite,  NULL, geos_cluster_tests};

