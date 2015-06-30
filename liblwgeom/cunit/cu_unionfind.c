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

	CU_ASSERT_EQUAL(10, uf->N);
	CU_ASSERT_EQUAL(10, uf->num_clusters);
	CU_ASSERT_EQUAL(0, memcmp(uf->clusters, expected_initial_ids, 10*sizeof(uint32_t)));
	CU_ASSERT_EQUAL(0, memcmp(uf->cluster_sizes, expected_initial_sizes, 10*sizeof(uint32_t)));

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

	CU_ASSERT_EQUAL(10, uf->N);
	CU_ASSERT_EQUAL(6, uf->num_clusters);
	CU_ASSERT_EQUAL(0, memcmp(uf->clusters, expected_final_ids, 10*sizeof(uint32_t)));
	CU_ASSERT_EQUAL(0, memcmp(uf->cluster_sizes, expected_final_sizes, 10*sizeof(uint32_t)));

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

void unionfind_suite_setup(void);
void unionfind_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("Clustering Union-Find", NULL, NULL);
	PG_ADD_TEST(suite, test_unionfind_create);
	PG_ADD_TEST(suite, test_unionfind_union);
	PG_ADD_TEST(suite, test_unionfind_ordered_by_cluster);
}
