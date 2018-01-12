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

	uint32_t expected_initial_ids[] =   { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	uint32_t expected_initial_sizes[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

	ASSERT_INT_EQUAL(uf->N, 10);
	ASSERT_INT_EQUAL(uf->num_clusters, 10);
	ASSERT_INTARRAY_EQUAL(uf->clusters, expected_initial_ids, 10);
	ASSERT_INTARRAY_EQUAL(uf->cluster_sizes, expected_initial_sizes, 10);

	UF_destroy(uf);
}

static void test_unionfind_union(void)
{
	UNIONFIND *uf = UF_create(10);

	UF_union(uf, 0, 7); /* both have size = 1, so 7 becomes 0 */
	UF_union(uf, 3, 2); /* both have size = 1, so 3 becomes 2 */
	UF_union(uf, 8, 7); /* add 8 (smaller) to 0-7 (larger)    */
	UF_union(uf, 1, 2); /* add 1 (smaller) to 2-3 (larger)    */

	uint32_t expected_final_ids[] =   { 0, 2, 2, 2, 4, 5, 6, 0, 0, 9 };
	uint32_t expected_final_sizes[] = { 3, 0, 3, 0, 1, 1, 1, 0, 0, 1 };

	ASSERT_INT_EQUAL(uf->N, 10);
	ASSERT_INT_EQUAL(uf->num_clusters, 6);
	ASSERT_INTARRAY_EQUAL(uf->clusters, expected_final_ids, 10);
	ASSERT_INTARRAY_EQUAL(uf->cluster_sizes, expected_final_sizes, 10);

	UF_destroy(uf);
}

static void test_unionfind_ordered_by_cluster(void)
{
	uint32_t final_clusters[] = { 0, 2, 2, 2, 4, 5, 6, 0, 0, 2 };
	uint32_t final_sizes[]    = { 3, 0, 4, 0, 1, 1, 1, 0, 0, 0 };

	/* Manually create UF at desired final state */
	UNIONFIND uf =
	{
		.N = 10,
		.num_clusters = 5,
		.clusters = final_clusters,
		.cluster_sizes = final_sizes
	};

	uint32_t* ids_by_cluster = UF_ordered_by_cluster(&uf);

	char encountered_cluster[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	uint32_t i;
	for (i = 0; i < uf.N; i++)
	{
		uint32_t c = final_clusters[ids_by_cluster[i]];
		if (!encountered_cluster[c])
		{
			encountered_cluster[c] = 1;
		}
		else
		{
			/* If we've seen an element of this cluster before, then the
			 * current cluster must be the same as the previous cluster. */
			uint32_t c_prev = final_clusters[ids_by_cluster[i-1]];
			CU_ASSERT_EQUAL(c, c_prev);
		}
	}
	lwfree(ids_by_cluster);
}

static void test_unionfind_path_compression(void)
{
	UNIONFIND* uf = UF_create(5);
	uint32_t i;

	uf->clusters[1] = 0;
	uf->clusters[2] = 1;
	uf->clusters[3] = 2;
	uf->clusters[4] = 3;

	/* Calling "find" on a leaf should attach all nodes between the root and the
	 * leaf directly to the root. */
	uint32_t root = UF_find(uf, 4);
	for (i = 0; i < uf->N; i++)
	{
		/* Verify that all cluster references have been updated to point
		 * directly to the root. */
		CU_ASSERT_EQUAL(root, uf->clusters[i]);
	}

	UF_destroy(uf);
}

static void test_unionfind_collapse_cluster_ids(void)
{
	UNIONFIND* uf = UF_create(10);

	uf->clusters[0] = 8;
	uf->clusters[1] = 5;
	uf->clusters[2] = 5;
	uf->clusters[3] = 5;
	uf->clusters[4] = 7;
	uf->clusters[5] = 5;
	uf->clusters[6] = 8;
	uf->clusters[7] = 7;
	uf->clusters[8] = 8;
	uf->clusters[9] = 7;

	uf->cluster_sizes[0] = 3;
	uf->cluster_sizes[1] = 4;
	uf->cluster_sizes[2] = 4;
	uf->cluster_sizes[3] = 4;
	uf->cluster_sizes[4] = 3;
	uf->cluster_sizes[5] = 4;
	uf->cluster_sizes[6] = 3;
	uf->cluster_sizes[7] = 3;
	uf->cluster_sizes[8] = 3;
	uf->cluster_sizes[9] = 3;

	/* 5 -> 0
	 * 7 -> 1
	 * 8 -> 2
	 */
	uint32_t expected_collapsed_ids[] = { 2, 0, 0, 0, 1, 0, 2, 1, 2, 1 };
	uint32_t* collapsed_ids = UF_get_collapsed_cluster_ids(uf, NULL);

	ASSERT_INTARRAY_EQUAL(collapsed_ids, expected_collapsed_ids, 10);

	lwfree(collapsed_ids);

	char is_in_cluster[] = { 0, 1, 1, 1, 0, 1, 0, 0, 0, 0 };
	uint32_t expected_collapsed_ids2[] = { 8, 0, 0, 0, 7, 0, 8, 7, 8, 7 };

	collapsed_ids = UF_get_collapsed_cluster_ids(uf, is_in_cluster);
	uint32_t i;
	for (i = 0; i < uf->N; i++)
	{
		if (is_in_cluster[i])
			ASSERT_INT_EQUAL(expected_collapsed_ids2[i], collapsed_ids[i]);
	}

	lwfree(collapsed_ids);
	UF_destroy(uf);
}

void unionfind_suite_setup(void);
void unionfind_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("clustering_unionfind", NULL, NULL);
	PG_ADD_TEST(suite, test_unionfind_create);
	PG_ADD_TEST(suite, test_unionfind_union);
	PG_ADD_TEST(suite, test_unionfind_ordered_by_cluster);
	PG_ADD_TEST(suite, test_unionfind_path_compression);
	PG_ADD_TEST(suite, test_unionfind_collapse_cluster_ids);
}
