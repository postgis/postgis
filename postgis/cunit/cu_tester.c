/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * This file is part of PostGIS
 *
 * PostGIS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * PostGIS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PostGIS.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 *
 * Copyright 2025 (C) Darafei Praliaskouski <me@komzpa.net>
 *
 **********************************************************************/

#include "postgres.h"

#include <CUnit/Basic.h>
#include <limits.h>
#include <string.h>

#include "../gserialized_estimate_support.h"

static ND_BOX
make_box(float minx, float miny, float minz, float minm, float maxx, float maxy, float maxz, float maxm)
{
	ND_BOX box;

	memset(&box, 0, sizeof(box));
	box.min[0] = minx;
	box.min[1] = miny;
	box.min[2] = minz;
	box.min[3] = minm;
	box.max[0] = maxx;
	box.max[1] = maxy;
	box.max[2] = maxz;
	box.max[3] = maxm;
	return box;
}

static void
histogram_budget_clamps(void)
{
	/* Zero or negative row counts disable histogram construction. */
	CU_ASSERT_EQUAL(histogram_cell_budget(0.0, 2, 100), 0);
	CU_ASSERT_EQUAL(histogram_cell_budget(-1.0, 4, 100), 0);

	/* Degenerate dimensionality cannot allocate histogram space. */
	CU_ASSERT_EQUAL(histogram_cell_budget(1000.0, 0, 100), 0);

	/* Matches the classic pow(attstattarget, ndims) path. */
	CU_ASSERT_EQUAL(histogram_cell_budget(1e6, 2, 100), 10000);
	CU_ASSERT_EQUAL(histogram_cell_budget(1e6, 3, 50), 125000);

	/* attstattarget^ndims exceeds ndims * 100000 and must be clamped. */
	CU_ASSERT_EQUAL(histogram_cell_budget(1e6, 4, 50), 400000);

	/* attstattarget<=0 is normalised to the smallest viable target. */
	CU_ASSERT_EQUAL(histogram_cell_budget(1e6, 2, 0), 1);

	/* Row clamp shrinks the grid for small relations. */
	CU_ASSERT_EQUAL(histogram_cell_budget(1.0, 2, 100), 20);

	/* Large tables now preserve the dimensional cap instead of overflowing. */
	CU_ASSERT_EQUAL(histogram_cell_budget(1.5e8, 2, 100), 10000);

	/* Regression for #5984: huge attstat targets stabilise instead of wrapping. */
	CU_ASSERT_EQUAL(histogram_cell_budget(5e6, 2, 10000), 200000);

	/* Trigger the INT_MAX guard once both other caps exceed it. */
	CU_ASSERT_EQUAL(histogram_cell_budget((double)INT_MAX, 50000, INT_MAX), INT_MAX);
}

static void
nd_stats_indexing_behaviour(void)
{
	ND_STATS stats;
	const int good_index[ND_DIMS] = {1, 2, 0, 0};
	const int bad_index[ND_DIMS] = {1, 5, 0, 0};

	memset(&stats, 0, sizeof(stats));
	stats.ndims = 3;
	stats.size[0] = 4.0f;
	stats.size[1] = 5.0f;
	stats.size[2] = 3.0f;

	/* Three-dimensional index  (x=1, y=2, z=0) collapses into 1 + 2 * 4. */
	CU_ASSERT_EQUAL(nd_stats_value_index(&stats, good_index), 1 + 2 * 4);
	/* Any request outside the histogram bounds triggers a guard. */
	CU_ASSERT_EQUAL(nd_stats_value_index(&stats, bad_index), -1);

	/* Regression for #5959: ndims higher than populated sizes still honours guards. */
	stats.ndims = 4;
	CU_ASSERT_EQUAL(nd_stats_value_index(&stats, good_index), -1);
}

static void
nd_box_ratio_cases(void)
{
	ND_BOX covering = make_box(0.0f, 0.0f, 0.0f, 0.0f, 2.0f, 2.0f, 2.0f, 0.0f);
	ND_BOX interior = make_box(0.5f, 0.5f, 0.5f, 0.0f, 1.5f, 1.5f, 1.5f, 0.0f);
	ND_BOX partial = make_box(0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.5f, 0.5f, 0.0f);
	ND_BOX target = make_box(0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f);
	ND_BOX flat = make_box(0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f);
	ND_BOX touch = make_box(2.0f, 0.0f, 0.0f, 0.0f, 3.0f, 1.0f, 1.0f, 0.0f);

	/* Full coverage should evaluate to one regardless of the extra extent. */
	CU_ASSERT_DOUBLE_EQUAL(nd_box_ratio(&covering, &interior, 3), 1.0, 1e-12);
	/* A shared octant carries one eighth of the reference volume. */
	CU_ASSERT_DOUBLE_EQUAL(nd_box_ratio(&partial, &target, 3), 0.125, 1e-12);
	/* Degenerate slabs have zero volume in three dimensions. */
	CU_ASSERT_DOUBLE_EQUAL(nd_box_ratio(&covering, &flat, 3), 0.0, 1e-12);
	/* Boxes that only touch along a face should not count as overlap. */
	CU_ASSERT_DOUBLE_EQUAL(nd_box_ratio(&covering, &touch, 3), 0.0, 1e-12);
}

int
main(void)
{
	CU_pSuite suite;
	unsigned int failures = 0;
	if (CU_initialize_registry() != CUE_SUCCESS)
		return CU_get_error();

	suite = CU_add_suite("gserialized_histogram_helpers", NULL, NULL);
	if (!suite)
		goto cleanup;

	if (!CU_add_test(suite, "histogram budget clamps", histogram_budget_clamps) ||
	    !CU_add_test(suite, "nd_stats value index guards", nd_stats_indexing_behaviour) ||
	    !CU_add_test(suite, "nd_box ratio edge cases", nd_box_ratio_cases))
	{
		goto cleanup;
	}

	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();

cleanup:
	failures = CU_get_number_of_tests_failed();
	CU_cleanup_registry();
	return failures == 0 ? CUE_SUCCESS : 1;
}
