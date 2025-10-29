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
 * Internal helpers shared between the gserialized selectivity
 * implementation and the unit tests.
 *
 * Keeping the routines header-only ensures the planner code and the
 * harness evaluate the exact same floating-point flows without the
 * cross-object plumbing that previously complicated maintenance.
 * Nothing here is installed; the header is meant for
 * gserialized_estimate.c and for the dedicated CUnit suite only.
 *
 **********************************************************************
 *
 * Copyright 2012 (C) Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright 2025 (C) Darafei Praliaskouski <me@komzpa.net>
 *
 **********************************************************************/

#ifndef POSTGIS_GSERIALIZED_ESTIMATE_SUPPORT_H
#define POSTGIS_GSERIALIZED_ESTIMATE_SUPPORT_H

#include "postgres.h"

#include <limits.h>
#include <math.h>

/* The maximum number of dimensions our statistics code supports. */
#define ND_DIMS 4

/* Lightweight n-dimensional box representation for selectivity math. */
typedef struct ND_BOX_T {
	float4 min[ND_DIMS];
	float4 max[ND_DIMS];
} ND_BOX;

/* Integer counterpart used for histogram cell iteration. */
typedef struct ND_IBOX_T {
	int min[ND_DIMS];
	int max[ND_DIMS];
} ND_IBOX;

/* On-disk representation of the histogram emitted by ANALYZE. */
typedef struct ND_STATS_T {
	float4 ndims;
	float4 size[ND_DIMS];
	ND_BOX extent;
	float4 table_features;
	float4 sample_features;
	float4 not_null_features;
	float4 histogram_features;
	float4 histogram_cells;
	float4 cells_covered;
	float4 value[1];
} ND_STATS;

/*
 * Return the flattened index for the histogram coordinate expressed by
 * 'indexes'.  A negative result signals that one of the axes fell outside
 * the histogram definition.
 */
static inline int
nd_stats_value_index(const ND_STATS *stats, const int *indexes)
{
	int d;
	int accum = 1;
	int vdx = 0;

	for (d = 0; d < (int)(stats->ndims); d++)
	{
		int size = (int)(stats->size[d]);
		if (indexes[d] < 0 || indexes[d] >= size)
			return -1;
		vdx += indexes[d] * accum;
		accum *= size;
	}
	return vdx;
}

/*
 * Derive the histogram grid budget requested by PostgreSQL's ANALYZE machinery.
 * The planner caps the cell count via three heuristics that take the requested
 * attstattarget, the histogram dimensionality, and the underlying row count
 * into account.  Double precision arithmetic keeps the intermediate products in
 * range so the cap behaves consistently across build architectures.
 */
static inline int
histogram_cell_budget(double total_rows, int ndims, int attstattarget)
{
	double budget;
	double dims_cap;
	double rows_cap;
	double attstat;
	double dims;

	if (ndims <= 0)
		return 0;

	if (attstattarget <= 0)
		attstattarget = 1;

	/* Requested resolution coming from PostgreSQL's ANALYZE knob. */
	attstat = (double)attstattarget;
	dims = (double)ndims;
	budget = pow(attstat, dims);

	/* Hard ceiling that keeps the statistics collector responsive. */
	dims_cap = (double)ndims * 100000.0;
	if (budget > dims_cap)
		budget = dims_cap;

	/* Small relations do not need a histogram that dwarfs the sample. */
	if (total_rows <= 0.0)
		return 0;

	rows_cap = 10.0 * (double)ndims * total_rows;
	if (rows_cap < 0.0)
		rows_cap = 0.0;

	/* Keep intermediate computations in double precision before clamping. */
	if (rows_cap > (double)INT_MAX)
		rows_cap = (double)INT_MAX;

	if (budget > rows_cap)
		budget = rows_cap;

	if (budget >= (double)INT_MAX)
		return INT_MAX;
	if (budget <= 0.0)
		return 0;

	return (int)budget;
}

/*
 * Allocate histogram buckets along a single axis in proportion to the observed
 * density variation.  The caller passes in the global histogram target along
 * with the number of axes that exhibited variation in the sampled data and the
 * relative contribution of the current axis (edge_ratio).  Earlier versions
 * evaluated the pow() call directly in the caller, which exposed the planner to
 * NaN propagation on some amd64 builds when the ratio was denormal or negative
 * (see #5959).  Keeping the calculation in one place allows us to clamp the
 * inputs and provide a predictable fallback for problematic floating point
 * combinations.
 */
static inline int
histogram_axis_cells(int histo_cells_target, int histo_ndims, double edge_ratio)
{
	double scaled;
	double axis_cells;

	if (histo_cells_target <= 0 || histo_ndims <= 0)
		return 1;

	if (!(edge_ratio > 0.0) || !isfinite(edge_ratio))
		return 1;

	scaled = (double)histo_cells_target * (double)histo_ndims * edge_ratio;
	if (!(scaled > 0.0) || !isfinite(scaled))
		return 1;

	axis_cells = pow(scaled, 1.0 / (double)histo_ndims);
	if (!(axis_cells > 0.0) || !isfinite(axis_cells))
		return 1;

	if (axis_cells >= (double)INT_MAX)
		return INT_MAX;

	if (axis_cells <= 1.0)
		return 1;

	return (int)axis_cells;
}

/*
 * Compute the portion of 'target' covered by 'cover'.  The caller supplies the
 * dimensionality because ND_BOX always carries four slots.  Degenerate volumes
 * fold to zero, allowing the callers to detect slabs that ANALYZE sometimes
 * emits for skewed datasets.
 */
static inline double
nd_box_ratio(const ND_BOX *cover, const ND_BOX *target, int ndims)
{
	int d;
	bool fully_covered = true;
	double ivol = 1.0;
	double refvol = 1.0;

	for (d = 0; d < ndims; d++)
	{
		if (cover->max[d] <= target->min[d] || cover->min[d] >= target->max[d])
			return 0.0; /* Disjoint */

		if (cover->min[d] > target->min[d] || cover->max[d] < target->max[d])
			fully_covered = false;
	}

	if (fully_covered)
		return 1.0;

	for (d = 0; d < ndims; d++)
	{
		double width = target->max[d] - target->min[d];
		double imin = Max(cover->min[d], target->min[d]);
		double imax = Min(cover->max[d], target->max[d]);
		double iwidth = Max(0.0, imax - imin);

		refvol *= width;
		ivol *= iwidth;
	}

	if (refvol == 0.0)
		return refvol;

	return ivol / refvol;
}

#endif /* POSTGIS_GSERIALIZED_ESTIMATE_SUPPORT_H */
