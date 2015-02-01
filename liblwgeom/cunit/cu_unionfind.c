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

#include "../lwunionfind.h"
#include "cu_tester.h"

static void test_unionfind_create(void)
{
	UNIONFIND *uf = UF_create(10);

	int expected_initial_ids[] =   { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	int expected_initial_sizes[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

	CU_ASSERT_EQUAL(10, uf->N);
	CU_ASSERT_EQUAL(10, uf->num_clusters);
    CU_ASSERT_EQUAL(0, memcmp(uf->clusters, expected_initial_ids, 10*sizeof(int)));
    CU_ASSERT_EQUAL(0, memcmp(uf->cluster_sizes, expected_initial_sizes, 10*sizeof(int)));
}

static void test_unionfind_union(void)
{
	UNIONFIND *uf = UF_create(10);

	UF_union(uf, 0, 7); /* both have size = 1, so 7 becomes 0 */
	UF_union(uf, 3, 2); /* both have size = 1, so 3 becomes 2 */
	UF_union(uf, 8, 7); /* add 8 (smaller) to 0-7 (larger)    */
	UF_union(uf, 1, 2); /* add 1 (smaller) to 2-3 (larger)    */
	
	int expected_final_ids[] =   { 0, 2, 2, 2, 4, 5, 6, 0, 0, 9 };
	int expected_final_sizes[] = { 3, 0, 3, 0, 1, 1, 1, 0, 0, 1 };

	CU_ASSERT_EQUAL(10, uf->N);
	CU_ASSERT_EQUAL(6, uf->num_clusters);
    CU_ASSERT_EQUAL(0, memcmp(uf->clusters, expected_final_ids, 10*sizeof(int)));
    CU_ASSERT_EQUAL(0, memcmp(uf->cluster_sizes, expected_final_sizes, 10*sizeof(int)));
}

static void test_unionfind_ordered_by_cluster(void)
{
	UNIONFIND *uf = UF_create(10);
	int final_clusters[] = { 0, 2, 2, 2, 4, 5, 6, 0, 0, 2 };
   	int final_sizes[]    = { 3, 0, 4, 0, 1, 1, 1, 0, 0, 0 };
	uf->clusters = final_clusters;
	uf->cluster_sizes = final_sizes;
	uf->num_clusters = 5;

	int* actual_ids_by_cluster = UF_ordered_by_cluster(uf);
	int expected_ids_by_cluster[] = { 0, 7, 8, 1, 2, 3, 9, 4, 5, 6 };
	
	CU_ASSERT_EQUAL(0, memcmp(actual_ids_by_cluster, expected_ids_by_cluster, 10 * sizeof(int)));
}

CU_TestInfo unionfind_tests[] =
{
	PG_TEST(test_unionfind_create),
	PG_TEST(test_unionfind_union),
	PG_TEST(test_unionfind_ordered_by_cluster),
	CU_TEST_INFO_NULL
};

CU_SuiteInfo unionfind_suite = {"Clustering Union-Find",  NULL,  NULL, unionfind_tests};

