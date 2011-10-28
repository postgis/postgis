/**********************************************************************
 * $Id: geography_estimate.c 4211 2009-06-25 03:32:43Z robe $
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2006 Refractions Research Inc.
 * Copyright 2009 Mark Cave-Ayland
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/


#include "postgres.h"
#include "commands/vacuum.h"
#include "nodes/relation.h"
#include "parser/parsetree.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"

#include "liblwgeom.h"
#include "lwgeom_pg.h"

/* Prototypes */
Datum geography_gist_selectivity(PG_FUNCTION_ARGS);
Datum geography_gist_join_selectivity(PG_FUNCTION_ARGS);
Datum geography_analyze(PG_FUNCTION_ARGS);


/**
* Place holder selectivity calculations to make the index work in
* the absence of real selectivity calculations.
*/

#define DEFAULT_GEOGRAPHY_SEL 0.000005

/**
 * 	Assign a number to the postgis statistics kind
 *
 * 	tgl suggested:
 *
 * 	1-100:	reserved for assignment by the core Postgres project
 * 	100-199: reserved for assignment by PostGIS
 * 	200-9999: reserved for other globally-known stats kinds
 * 	10000-32767: reserved for private site-local use
 *
 */
#define STATISTIC_KIND_GEOGRAPHY 101

/*
 * Define this if you want to use standard deviation based
 * histogram extent computation. If you do, you can also
 * tweak the deviation factor used in computation with
 * SDFACTOR.
 */
#define USE_STANDARD_DEVIATION 1
#define SDFACTOR 3.25


/* Information about the dimensions stored in the sample */
struct dimensions
{
	char axis;
	double min;
	double max;
};

/* Main geography statistics structure, stored in pg_statistics */
typedef struct GEOG_STATS_T
{
	/* Dimensionality of this column */
	float4 dims;

	/* x . y . z = total boxes in grid */
	float4 unitsx;
	float4 unitsy;
	float4 unitsz;

	/* average feature coverage of not-null features:
	   this is either an area for the case of a 2D column
	   or a volume in the case of a 3D column */
	float4 avgFeatureCoverage;

	/*
	 * average number of histogram cells
	 * covered by the sample not-null features
	 */
	float4 avgFeatureCells;

	/* histogram extent */
	float4 xmin, ymin, zmin, xmax, ymax, zmax;

	/* No. of geometries within this column (approx) */
	float4 totalrows;

	/*
	 * variable length # of floats for histogram
	 */
	float4 value[1];
}
GEOG_STATS;


/**
 * This function returns an estimate of the selectivity
 * of a search_box looking at data in the GEOG_STATS
 * structure.
 * */
static float8
estimate_selectivity(GBOX *box, GEOG_STATS *geogstats)
{
	int x, y, z;
	int x_idx_min, x_idx_max, y_idx_min, y_idx_max, z_idx_min, z_idx_max;
	double intersect_x, intersect_y, intersect_z;
	double AOI = 1.0;
	double cell_coverage = 1.0;
	double sizex, sizey, sizez;	/* dimensions of histogram */
	int unitsx, unitsy, unitsz; /* histogram grid size */
	double value;
	float overlapping_cells;
	float avg_feat_cells;
	double gain;
	float8 selectivity;


	/*
	 * Search box completely miss histogram extent
	 */
	if ( box->xmax < geogstats->xmin || box->xmin > geogstats->xmax ||
	        box->ymax < geogstats->ymin || box->ymin > geogstats->ymax ||
	        box->zmax < geogstats->zmin || box->zmin > geogstats->zmax)
	{
		POSTGIS_DEBUG(3, " search_box does not overlaps histogram, returning 0");

		return 0.0;
	}

	/*
	 * Search box completely contains histogram extent
	 */
	if ( box->xmax >= geogstats->xmax && box->xmin <= geogstats->xmin &&
	        box->ymax >= geogstats->ymax && box->ymin <= geogstats->ymin &&
	        box->zmax >= geogstats->zmax && box->zmin <= geogstats->zmin)
	{
		POSTGIS_DEBUG(3, " search_box contains histogram, returning 1");

		return 1.0;
	}

	sizex = geogstats->xmax - geogstats->xmin;
	sizey = geogstats->ymax - geogstats->ymin;
	sizez = geogstats->zmax - geogstats->zmin;

	unitsx = geogstats->unitsx;
	unitsy = geogstats->unitsy;
	unitsz = geogstats->unitsz;

	POSTGIS_DEBUGF(3, " histogram has %d unitsx, %d unitsy, %d unitsz", unitsx, unitsy, unitsz);
	POSTGIS_DEBUGF(3, " histogram geosize is %fx%fx%f", sizex, sizey, sizez);

	/* Work out the coverage depending upon the number of dimensions */
	switch ((int)geogstats->dims)
	{
	case 0:
	case 1:
		/* Non-existent or multiple single points */
		cell_coverage = 1;
		break;

	case 2:
		/* Work in areas for 2 dimensions: we have to work slightly
		   harder here to work out which 2 dimensions are valid */

		if (sizez == 0)
			/* X and Y */
			cell_coverage = (sizex * sizey) / (unitsx * unitsy);
		else if (sizey == 0)
			/* X and Z */
			cell_coverage = (sizex * sizez) / (unitsx * unitsz);
		else if (sizex == 0)
			/* Y and Z */
			cell_coverage = (sizey * sizez) / (unitsy * unitsz);
		break;

	case 3:
		/* Work in volumes for 3 dimensions */
		cell_coverage = (sizex * sizey * sizey) / (unitsx * unitsy * unitsz);
		break;
	}

	value = 0;

	/* Find first overlapping x unit */
	x_idx_min = (box->xmin-geogstats->xmin) / sizex * unitsx;
	if (x_idx_min < 0)
	{
		POSTGIS_DEBUGF(3, " search_box overlaps %d x units outside (low) of histogram grid", -x_idx_min);

		/* should increment the value somehow */
		x_idx_min = 0;
	}
	if (x_idx_min >= unitsx)
	{
		POSTGIS_DEBUGF(3, " search_box overlaps %d x units outside (high) of histogram grid", x_idx_min-unitsx+1);

		/* should increment the value somehow */
		x_idx_min = unitsx-1;
	}

	/* Find first overlapping y unit */
	y_idx_min = (box->ymin-geogstats->ymin) / sizey * unitsy;
	if (y_idx_min <0)
	{
		POSTGIS_DEBUGF(3, " search_box overlaps %d y units outside (low) of histogram grid", -y_idx_min);

		/* should increment the value somehow */
		y_idx_min = 0;
	}
	if (y_idx_min >= unitsy)
	{
		POSTGIS_DEBUGF(3, " search_box overlaps %d y units outside (high) of histogram grid", y_idx_min-unitsy+1);

		/* should increment the value somehow */
		y_idx_min = unitsy-1;
	}

	/* Find first overlapping z unit */
	z_idx_min = (box->zmin-geogstats->zmin) / sizez * unitsz;
	if (z_idx_min <0)
	{
		POSTGIS_DEBUGF(3, " search_box overlaps %d z units outside (low) of histogram grid", -z_idx_min);

		/* should increment the value somehow */
		z_idx_min = 0;
	}
	if (z_idx_min >= unitsz)
	{
		POSTGIS_DEBUGF(3, " search_box overlaps %d z units outside (high) of histogram grid", z_idx_min-unitsz+1);

		/* should increment the value somehow */
		z_idx_min = unitsz-1;
	}

	/* Find last overlapping x unit */
	x_idx_max = (box->xmax-geogstats->xmin) / sizex * unitsx;
	if (x_idx_max <0)
	{
		/* should increment the value somehow */
		x_idx_max = 0;
	}
	if (x_idx_max >= unitsx )
	{
		/* should increment the value somehow */
		x_idx_max = unitsx-1;
	}

	/* Find last overlapping y unit */
	y_idx_max = (box->ymax-geogstats->ymin) / sizey * unitsy;
	if (y_idx_max <0)
	{
		/* should increment the value somehow */
		y_idx_max = 0;
	}
	if (y_idx_max >= unitsy)
	{
		/* should increment the value somehow */
		y_idx_max = unitsy-1;
	}

	/* Find last overlapping z unit */
	z_idx_max = (box->zmax-geogstats->zmin) / sizez * unitsz;
	if (z_idx_max <0)
	{
		/* should increment the value somehow */
		z_idx_max = 0;
	}
	if (z_idx_max >= unitsz)
	{
		/* should increment the value somehow */
		z_idx_max = unitsz-1;
	}

	/*
	 * the {x,y,z}_idx_{min,max}
	 * define the grid squares that the box intersects
	 */
	for (z=z_idx_min; z<=z_idx_max; z++)
	{
		for (y=y_idx_min; y<=y_idx_max; y++)
		{
			for (x=x_idx_min; x<=x_idx_max; x++)
			{
				double val;
				double gain;

				val = geogstats->value[x + y * unitsx + z * unitsx * unitsy];

				/*
				* Of the cell value we get
				* only the overlap fraction.
				*/

				intersect_x = Min(box->xmax, geogstats->xmin + (x+1) * sizex / unitsx) - Max(box->xmin, geogstats->xmin + x * sizex / unitsx);
				intersect_y = Min(box->ymax, geogstats->ymin + (y+1) * sizey / unitsy) - Max(box->ymin, geogstats->ymin + y * sizey / unitsy);
				intersect_z = Min(box->zmax, geogstats->zmin + (z+1) * sizez / unitsz) - Max(box->zmin, geogstats->zmin + z * sizez / unitsz);


				switch ((int)geogstats->dims)
				{
				case 0:
					AOI = 1;
				case 1:
					/* Working in 1 dimension: work out the value we need
					   for AOI */
					if (sizex == 0 && sizey == 0)
						AOI = intersect_z;
					else if (sizex == 0 && sizez == 0)
						AOI = intersect_y;
					else if (sizey == 0 && sizez == 0)
						AOI = intersect_x;
					break;

				case 2:
					/* Working in 2 dimensions: work out which 2 values we
					   need for AOI */
					if (sizex == 0)
						AOI = intersect_y * intersect_z;
					else if (sizey == 0)
						AOI = intersect_x * intersect_z;
					else if (sizez == 0)
						AOI = intersect_x * intersect_y;
					break;

				case 3:
					/* Working in 3 dimensions: use all 3 values */
					AOI = intersect_x * intersect_y * intersect_z;
					break;
				}

				gain = AOI/cell_coverage;

				POSTGIS_DEBUGF(4, " [%d,%d,%d] cell val %.15f",
				               x, y, z, val);
				POSTGIS_DEBUGF(4, " [%d,%d,%d] AOI %.15f",
				               x, y, z, AOI);
				POSTGIS_DEBUGF(4, " [%d,%d,%d] gain %.15f",
				               x, y, z, gain);

				val *= gain;

				POSTGIS_DEBUGF(4, " [%d,%d,%d] adding %.15f to value",
				               x, y, z, val);

				value += val;
			}
		}
	}


	/*
	 * If the search_box is a point, it will
	 * overlap a single cell and thus get
	 * it's value, which is the fraction of
	 * samples (we can presume of row set also)
	 * which bumped to that cell.
	 *
	 * If the table features are points, each
	 * of them will overlap a single histogram cell.
	 * Our search_box value would then be correctly
	 * computed as the sum of the bumped cells values.
	 *
	 * If both our search_box AND the sample features
	 * overlap more then a single histogram cell we
	 * need to consider the fact that our sum computation
	 * will have many duplicated included. E.g. each
	 * single sample feature would have contributed to
	 * raise the search_box value by as many times as
	 * many cells in the histogram are commonly overlapped
	 * by both searc_box and feature. We should then
	 * divide our value by the number of cells in the virtual
	 * 'intersection' between average feature cell occupation
	 * and occupation of the search_box. This is as
	 * fuzzy as you understand it :)
	 *
	 * Consistency check: whenever the number of cells is
	 * one of whichever part (search_box_occupation,
	 * avg_feature_occupation) the 'intersection' must be 1.
	 * If sounds that our 'intersaction' is actually the
	 * minimun number between search_box_occupation and
	 * avg_feat_occupation.
	 *
	 */
	overlapping_cells = (x_idx_max-x_idx_min+1) * (y_idx_max-y_idx_min+1) * (z_idx_max-z_idx_min+1);
	avg_feat_cells = geogstats->avgFeatureCells;

	POSTGIS_DEBUGF(3, " search_box overlaps %f cells", overlapping_cells);
	POSTGIS_DEBUGF(3, " avg feat overlaps %f cells", avg_feat_cells);

	if ( ! overlapping_cells )
	{
		POSTGIS_DEBUG(3, " no overlapping cells, returning 0.0");

		return 0.0;
	}

	gain = 1 / Min(overlapping_cells, avg_feat_cells);
	selectivity = value * gain;

	POSTGIS_DEBUGF(3, " SUM(ov_histo_cells)=%f", value);
	POSTGIS_DEBUGF(3, " gain=%f", gain);
	POSTGIS_DEBUGF(3, " selectivity=%f", selectivity);

	/* prevent rounding overflows */
	if (selectivity > 1.0) selectivity = 1.0;
	else if (selectivity < 0) selectivity = 0.0;

	return selectivity;
}


/**
 * This function should return an estimation of the number of
 * rows returned by a query involving an overlap check
 * ( it's the restrict function for the && operator )
 *
 * It can make use (if available) of the statistics collected
 * by the geometry analyzer function.
 *
 * Note that the good work is done by estimate_selectivity() above.
 * This function just tries to find the search_box, loads the statistics
 * and invoke the work-horse.
 *
 */
PG_FUNCTION_INFO_V1(geography_gist_selectivity);
Datum geography_gist_selectivity(PG_FUNCTION_ARGS)
{
	PlannerInfo *root = (PlannerInfo *) PG_GETARG_POINTER(0);

	/* Oid operator = PG_GETARG_OID(1); */
	List *args = (List *) PG_GETARG_POINTER(2);
	/* int varRelid = PG_GETARG_INT32(3); */
	Oid relid;
	HeapTuple stats_tuple;
	GEOG_STATS *geogstats;
	/*
	 * This is to avoid casting the corresponding
	 * "type-punned" pointer, which would break
	 * "strict-aliasing rules".
	 */
	GEOG_STATS **gsptr=&geogstats;
	int geogstats_nvalues = 0;
	Node *other;
	Var *self;
	GSERIALIZED *serialized;
	LWGEOM *geometry;
	GBOX search_box;
	float8 selectivity = 0;

	POSTGIS_DEBUG(2, "geography_gist_selectivity called");

	/* Fail if not a binary opclause (probably shouldn't happen) */
	if (list_length(args) != 2)
	{
		POSTGIS_DEBUG(3, "geography_gist_selectivity: not a binary opclause");

		PG_RETURN_FLOAT8(DEFAULT_GEOGRAPHY_SEL);
	}

	/*
	 * This selectivity function is invoked by a clause of the form <arg> && <arg>
	 *
	 * In typical usage, one argument will be a column reference, while the other will
	 * be a geography constant; set self to point to the column argument and other
	 * to point to the constant argument.
	 */
	other = (Node *) linitial(args);
	if ( ! IsA(other, Const) )
	{
		self = (Var *)other;
		other = (Node *) lsecond(args);
	}
	else
	{
		self = (Var *) lsecond(args);
	}

	if ( ! IsA(other, Const) )
	{
		POSTGIS_DEBUG(3, " no constant arguments - returning default selectivity");

		PG_RETURN_FLOAT8(DEFAULT_GEOGRAPHY_SEL);
	}

	/*
	 * We are working on two constants..
	 * TODO: check if expression is true,
	 *       returned set would be either
	 *       the whole or none.
	 */
	if ( ! IsA(self, Var) )
	{
		POSTGIS_DEBUG(3, " no variable argument ? - returning default selectivity");

		PG_RETURN_FLOAT8(DEFAULT_GEOGRAPHY_SEL);
	}

	/*
	 * Convert the constant to a GBOX
	 */
	serialized = (GSERIALIZED *)PG_DETOAST_DATUM( ((Const*)other)->constvalue );
	geometry = lwgeom_from_gserialized(serialized);

	/* Convert coordinates to 3D geodesic */
	FLAGS_SET_GEODETIC(search_box.flags, 1);
	if (!lwgeom_calculate_gbox_geodetic(geometry, &search_box))
	{
		POSTGIS_DEBUG(3, " search box cannot be calculated");

		PG_RETURN_FLOAT8(0.0);
	}

	POSTGIS_DEBUGF(4, " requested search box is : %.15g %.15g %.15g, %.15g %.15g %.15g",
	               search_box.xmin, search_box.ymin, search_box.zmin,
	               search_box.xmax, search_box.ymax, search_box.zmax);

	/*
	 * Get pg_statistic row
	 */
	relid = getrelid(self->varno, root->parse->rtable);

	stats_tuple = SearchSysCache(STATRELATT, ObjectIdGetDatum(relid), Int16GetDatum(self->varattno), 0, 0);
	if ( ! stats_tuple )
	{
		POSTGIS_DEBUG(3, " No statistics, returning default estimate");

		PG_RETURN_FLOAT8(DEFAULT_GEOGRAPHY_SEL);
	}


	if ( ! get_attstatsslot(stats_tuple, 0, 0, STATISTIC_KIND_GEOGRAPHY, InvalidOid, NULL, NULL,
#if POSTGIS_PGSQL_VERSION >= 85
	                        NULL,
#endif
	                        (float4 **)gsptr, &geogstats_nvalues) )
	{
		POSTGIS_DEBUG(3, " STATISTIC_KIND_GEOGRAPHY stats not found - returning default geography selectivity");

		ReleaseSysCache(stats_tuple);
		PG_RETURN_FLOAT8(DEFAULT_GEOGRAPHY_SEL);
	}

	POSTGIS_DEBUGF(4, " %d read from stats", geogstats_nvalues);

	POSTGIS_DEBUGF(4, " histo: xmin,ymin,zmin: %f,%f,%f", geogstats->xmin, geogstats->ymin, geogstats->zmin);
	POSTGIS_DEBUGF(4, " histo: xmax,ymax: %f,%f,%f", geogstats->xmax, geogstats->ymax, geogstats->zmax);
	POSTGIS_DEBUGF(4, " histo: unitsx: %f", geogstats->unitsx);
	POSTGIS_DEBUGF(4, " histo: unitsy: %f", geogstats->unitsy);
	POSTGIS_DEBUGF(4, " histo: unitsz: %f", geogstats->unitsz);
	POSTGIS_DEBUGF(4, " histo: avgFeatureCoverage: %f", geogstats->avgFeatureCoverage);
	POSTGIS_DEBUGF(4, " histo: avgFeatureCells: %f", geogstats->avgFeatureCells);

	/*
	 * Do the estimation
	 */
	selectivity = estimate_selectivity(&search_box, geogstats);

	POSTGIS_DEBUGF(3, " returning computed value: %f", selectivity);

	free_attstatsslot(0, NULL, 0, (float *)geogstats, geogstats_nvalues);
	ReleaseSysCache(stats_tuple);
	PG_RETURN_FLOAT8(selectivity);
}


/**
* JOIN selectivity in the GiST && operator
* for all PG versions
*/
PG_FUNCTION_INFO_V1(geography_gist_join_selectivity);
Datum geography_gist_join_selectivity(PG_FUNCTION_ARGS)
{
	PlannerInfo *root = (PlannerInfo *) PG_GETARG_POINTER(0);

	/* Oid operator = PG_GETARG_OID(1); */
	List *args = (List *) PG_GETARG_POINTER(2);
	JoinType jointype = (JoinType) PG_GETARG_INT16(3);

	Node *arg1, *arg2;
	Var *var1, *var2;
	Oid relid1, relid2;

	HeapTuple stats1_tuple, stats2_tuple;
	GEOG_STATS *geogstats1, *geogstats2;
	/*
	* These are to avoid casting the corresponding
	* "type-punned" pointers, which would break
	* "strict-aliasing rules".
	*/
	GEOG_STATS **gs1ptr=&geogstats1, **gs2ptr=&geogstats2;
	int geogstats1_nvalues = 0, geogstats2_nvalues = 0;
	float8 selectivity1 = 0.0, selectivity2 = 0.0;
	float4 num1_tuples = 0.0, num2_tuples = 0.0;
	float4 total_tuples = 0.0, rows_returned = 0.0;
	GBOX search_box;


	/**
	* Join selectivity algorithm. To calculation the selectivity we
	* calculate the intersection of the two column sample extents,
	* sum the results, and then multiply by two since for each
	* geometry in col 1 that intersects a geometry in col 2, the same
	* will also be true.
	*/

	POSTGIS_DEBUGF(3, "geography_gist_join_selectivity called with jointype %d", jointype);

	/*
	* We'll only respond to an inner join/unknown context join
	*/
	if (jointype != JOIN_INNER)
	{
		elog(NOTICE, "geography_gist_join_selectivity called with incorrect join type");
		PG_RETURN_FLOAT8(DEFAULT_GEOGRAPHY_SEL);
	}

	/*
	* Determine the oids of the geometry columns we are working with
	*/
	arg1 = (Node *) linitial(args);
	arg2 = (Node *) lsecond(args);

	if (!IsA(arg1, Var) || !IsA(arg2, Var))
	{
		elog(DEBUG1, "geography_gist_join_selectivity called with arguments that are not column references");
		PG_RETURN_FLOAT8(DEFAULT_GEOGRAPHY_SEL);
	}

	var1 = (Var *)arg1;
	var2 = (Var *)arg2;

	relid1 = getrelid(var1->varno, root->parse->rtable);
	relid2 = getrelid(var2->varno, root->parse->rtable);

	POSTGIS_DEBUGF(3, "Working with relations oids: %d %d", relid1, relid2);

	/* Read the stats tuple from the first column */
	stats1_tuple = SearchSysCache(STATRELATT, ObjectIdGetDatum(relid1), Int16GetDatum(var1->varattno), 0, 0);
	if ( ! stats1_tuple )
	{
		POSTGIS_DEBUG(3, " No statistics, returning default geometry join selectivity");

		PG_RETURN_FLOAT8(DEFAULT_GEOGRAPHY_SEL);
	}

	if ( ! get_attstatsslot(stats1_tuple, 0, 0, STATISTIC_KIND_GEOGRAPHY, InvalidOid, NULL, NULL,
#if POSTGIS_PGSQL_VERSION >= 85
	                        NULL,
#endif
	                        (float4 **)gs1ptr, &geogstats1_nvalues) )
	{
		POSTGIS_DEBUG(3, " STATISTIC_KIND_GEOGRAPHY stats not found - returning default geometry join selectivity");

		ReleaseSysCache(stats1_tuple);
		PG_RETURN_FLOAT8(DEFAULT_GEOGRAPHY_SEL);
	}


	/* Read the stats tuple from the second column */
	stats2_tuple = SearchSysCache(STATRELATT, ObjectIdGetDatum(relid2), Int16GetDatum(var2->varattno), 0, 0);
	if ( ! stats2_tuple )
	{
		POSTGIS_DEBUG(3, " No statistics, returning default geometry join selectivity");

		free_attstatsslot(0, NULL, 0, (float *)geogstats1, geogstats1_nvalues);
		ReleaseSysCache(stats1_tuple);
		PG_RETURN_FLOAT8(DEFAULT_GEOGRAPHY_SEL);
	}

	if ( ! get_attstatsslot(stats2_tuple, 0, 0, STATISTIC_KIND_GEOGRAPHY, InvalidOid, NULL, NULL,
#if POSTGIS_PGSQL_VERSION >= 85
	                        NULL,
#endif
	                        (float4 **)gs2ptr, &geogstats2_nvalues) )
	{
		POSTGIS_DEBUG(3, " STATISTIC_KIND_GEOGRAPHY stats not found - returning default geometry join selectivity");

		free_attstatsslot(0, NULL, 0, (float *)geogstats1, geogstats1_nvalues);
		ReleaseSysCache(stats2_tuple);
		ReleaseSysCache(stats1_tuple);
		PG_RETURN_FLOAT8(DEFAULT_GEOGRAPHY_SEL);
	}


	/**
	* Setup the search box - this is the intersection of the two column
	* extents.
	*/
	search_box.xmin = Max(geogstats1->xmin, geogstats2->xmin);
	search_box.ymin = Max(geogstats1->ymin, geogstats2->ymin);
	search_box.zmin = Max(geogstats1->zmin, geogstats2->zmin);
	search_box.xmax = Min(geogstats1->xmax, geogstats2->xmax);
	search_box.ymax = Min(geogstats1->ymax, geogstats2->ymax);
	search_box.zmax = Min(geogstats1->zmax, geogstats2->zmax);

	/* If the extents of the two columns don't intersect, return zero */
	if (search_box.xmin > search_box.xmax || search_box.ymin > search_box.ymax ||
	        search_box.zmin > search_box.zmax)
		PG_RETURN_FLOAT8(0.0);

	POSTGIS_DEBUGF(3, " -- geomstats1 box: %.15g %.15g %.15g, %.15g %.15g %.15g", geogstats1->xmin, geogstats1->ymin, geogstats1->zmin, geogstats1->xmax, geogstats1->ymax, geogstats1->zmax);
	POSTGIS_DEBUGF(3, " -- geomstats2 box: %.15g %.15g %.15g, %.15g %.15g %.15g", geogstats2->xmin, geogstats2->ymin, geogstats2->zmin, geogstats2->xmax, geogstats2->ymax, geogstats2->zmax);
	POSTGIS_DEBUGF(3, " -- calculated intersection box is : %.15g %.15g %.15g, %.15g %.15g %.15g", search_box.xmin, search_box.ymin, search_box.zmin, search_box.xmax, search_box.ymax, search_box.zmax);


	/* Do the selectivity */
	selectivity1 = estimate_selectivity(&search_box, geogstats1);
	selectivity2 = estimate_selectivity(&search_box, geogstats2);

	POSTGIS_DEBUGF(3, "selectivity1: %.15g   selectivity2: %.15g", selectivity1, selectivity2);

	/*
	* OK, so before we calculate the join selectivity we also need to
	* know the number of tuples in each of the columns since
	* estimate_selectivity returns the number of estimated tuples
	* divided by the total number of tuples.
	*/
	num1_tuples = geogstats1->totalrows;
	num2_tuples = geogstats2->totalrows;

	/* Free the statistic tuples */
	free_attstatsslot(0, NULL, 0, (float *)geogstats1, geogstats1_nvalues);
	ReleaseSysCache(stats1_tuple);

	free_attstatsslot(0, NULL, 0, (float *)geogstats2, geogstats2_nvalues);
	ReleaseSysCache(stats2_tuple);

	/*
	* Finally calculate the estimate of the number of rows returned
	*
	*    = 2 * (nrows from col1 + nrows from col2) /
	*	total nrows in col1 x total nrows in col2
	*
	* The factor of 2 accounts for the fact that for each tuple in
	* col 1 matching col 2,
	* there will be another match in col 2 matching col 1
	*/
	total_tuples = num1_tuples * num2_tuples;
	rows_returned = 2 * ((num1_tuples * selectivity1) + (num2_tuples * selectivity2));

	POSTGIS_DEBUGF(3, "Rows from rel1: %f", num1_tuples * selectivity1);
	POSTGIS_DEBUGF(3, "Rows from rel2: %f", num2_tuples * selectivity2);
	POSTGIS_DEBUGF(3, "Estimated rows returned: %f", rows_returned);

	/*
	* One (or both) tuple count is zero...
	* We return default selectivity estimate.
	* We could probably attempt at an estimate
	* w/out looking at tables tuple count, with
	* a function of selectivity1, selectivity2.
	*/
	if ( ! total_tuples )
	{
		POSTGIS_DEBUG(3, "Total tuples == 0, returning default join selectivity");

		PG_RETURN_FLOAT8(DEFAULT_GEOGRAPHY_SEL);
	}

	if ( rows_returned > total_tuples )
		PG_RETURN_FLOAT8(1.0);

	PG_RETURN_FLOAT8(rows_returned / total_tuples);
}


/**
 * This function is called by the analyze function iff
 * the geography_analyze() function give it its pointer
 * (this is always the case so far).
 * The geography_analyze() function is also responsible
 * of deciding the number of "sample" rows we will receive
 * here. It is able to give use other 'custom' data, but we
 * won't use them so far.
 *
 * Our job is to build some statistics on the sample data
 * for use by operator estimators.
 *
 * Currently we only need statistics to estimate the number of rows
 * overlapping a given extent (estimation function bound
 * to the && operator).
 *
 */

static void
compute_geography_stats(VacAttrStats *stats, AnalyzeAttrFetchFunc fetchfunc,
                        int samplerows, double totalrows)
{
	MemoryContext old_context;
	GEOG_STATS *geogstats;

	GBOX gbox;

	GBOX *sample_extent = NULL;
	GBOX **sampleboxes;
	GBOX histobox;
	int histocells;
	double sizex, sizey, sizez, edgelength;
	int unitsx = 0, unitsy = 0, unitsz = 0;
	int geog_stats_size;
	struct dimensions histodims[3];
	int ndims;

	double total_width = 0;
	int notnull_cnt = 0, examinedsamples = 0, total_count_cells=0, total_cells_coverage = 0;

#if USE_STANDARD_DEVIATION
	/* for standard deviation */
	double avgLOWx, avgLOWy, avgLOWz, avgHIGx, avgHIGy, avgHIGz;
	double sumLOWx = 0, sumLOWy = 0, sumLOWz = 0, sumHIGx = 0, sumHIGy = 0, sumHIGz = 0;
	double sdLOWx = 0, sdLOWy = 0, sdLOWz = 0, sdHIGx = 0, sdHIGy = 0, sdHIGz = 0;
	GBOX *newhistobox = NULL;
#endif

	bool isnull;
	int i;

	POSTGIS_DEBUG(2, "compute_geography_stats called");

	/*
	 * We'll build an histogram having from 40 to 400 boxesPerSide
	 * Total number of cells is determined by attribute stat
	 * target. It can go from  1600 to 160000 (stat target: 10,1000)
	 */
	histocells = 160 * stats->attr->attstattarget;

	/*
	 * Memory to store the bounding boxes from all of the sampled rows
	 */
	sampleboxes = palloc(sizeof(GBOX *) * samplerows);

	/* Mark the GBOX as being geodetic */
	FLAGS_SET_GEODETIC(gbox.flags, 1);

	/*
	 * First scan:
	 *  o find extent of the sample rows
	 *  o count null-infinite/not-null values
	 *  o compute total_width
	 *  o compute total features's box area (for avgFeatureArea)
	 *  o sum features box coordinates (for standard deviation)
	 */
	for (i = 0; i < samplerows; i++)
	{
		Datum datum;
		GSERIALIZED *serialized;
		LWGEOM *geometry;

		/* Fetch the datum and cast it into a geography */
		datum = fetchfunc(stats, i, &isnull);

		/* Skip nulls */
		if (isnull)
			continue;

		serialized = (GSERIALIZED *)PG_DETOAST_DATUM(datum);
		geometry = lwgeom_from_gserialized(serialized);

		/* Convert coordinates to 3D geodesic */
		if (!lwgeom_calculate_gbox_geodetic(geometry, &gbox))
		{
			/* Unable to obtain or calculate a bounding box */
			POSTGIS_DEBUGF(3, "skipping geometry at position %d", i);

			continue;
		}

		/*
		 * Skip infinite geoms
		 */
		if ( ! finite(gbox.xmin) || ! finite(gbox.xmax) ||
		        ! finite(gbox.ymin) || ! finite(gbox.ymax) ||
		        ! finite(gbox.zmin) || ! finite(gbox.zmax) )
		{
			POSTGIS_DEBUGF(3, " skipped infinite geometry at position %d", i);

			continue;
		}

		/*
		 * Store bounding box in array
		 */
		sampleboxes[notnull_cnt] = palloc(sizeof(GBOX));
		memcpy(sampleboxes[notnull_cnt], &gbox, sizeof(GBOX));

		/*
		 * Add to sample extent union
		 */
		if ( ! sample_extent )
		{
			sample_extent = palloc(sizeof(GBOX));
			memcpy(sample_extent, &gbox, sizeof(GBOX));
		}
		else
		{
			sample_extent->xmax = Max(sample_extent->xmax, gbox.xmax);
			sample_extent->ymax = Max(sample_extent->ymax, gbox.ymax);
			sample_extent->zmax = Max(sample_extent->zmax, gbox.zmax);
			sample_extent->xmin = Min(sample_extent->xmin, gbox.xmin);
			sample_extent->ymin = Min(sample_extent->ymin, gbox.ymin);
			sample_extent->zmin = Min(sample_extent->zmin, gbox.zmin);
		}

		/** TODO: ask if we need geom or bvol size for stawidth */
		total_width += serialized->size;

#if USE_STANDARD_DEVIATION
		/*
		 * Add bvol coordinates to sum for standard deviation
		 * computation.
		 */
		sumLOWx += gbox.xmin;
		sumLOWy += gbox.ymin;
		sumLOWz += gbox.zmin;
		sumHIGx += gbox.xmax;
		sumHIGy += gbox.ymax;
		sumHIGz += gbox.zmax;
#endif

		notnull_cnt++;

		/* give backend a chance of interrupting us */
		vacuum_delay_point();
	}

	POSTGIS_DEBUG(3, "End of 1st scan:");
	POSTGIS_DEBUGF(3, " Sample extent (min, max): (%g %g %g), (%g %g %g)", sample_extent->xmin, sample_extent->ymin,
	               sample_extent->zmin, sample_extent->xmax, sample_extent->ymax, sample_extent->zmax);
	POSTGIS_DEBUGF(3, " No. of geometries sampled: %d", samplerows);
	POSTGIS_DEBUGF(3, " No. of non-null geometries sampled: %d", notnull_cnt);

	if ( ! notnull_cnt )
	{
		elog(NOTICE, " no notnull values, invalid stats");
		stats->stats_valid = false;
		return;
	}

#if USE_STANDARD_DEVIATION

	POSTGIS_DEBUG(3, "Standard deviation filter enabled");

	/*
	 * Second scan:
	 *  o compute standard deviation
	 */
	avgLOWx = sumLOWx / notnull_cnt;
	avgLOWy = sumLOWy / notnull_cnt;
	avgLOWz = sumLOWz / notnull_cnt;
	avgHIGx = sumHIGx / notnull_cnt;
	avgHIGy = sumHIGy / notnull_cnt;
	avgHIGz = sumHIGz / notnull_cnt;

	for (i = 0; i < notnull_cnt; i++)
	{
		GBOX *box;
		box = (GBOX *)sampleboxes[i];

		sdLOWx += (box->xmin - avgLOWx) * (box->xmin - avgLOWx);
		sdLOWy += (box->ymin - avgLOWy) * (box->ymin - avgLOWy);
		sdLOWz += (box->zmin - avgLOWz) * (box->zmin - avgLOWz);
		sdHIGx += (box->xmax - avgHIGx) * (box->xmax - avgHIGx);
		sdHIGy += (box->ymax - avgHIGy) * (box->ymax - avgHIGy);
		sdHIGz += (box->zmax - avgHIGz) * (box->zmax - avgHIGz);
	}
	sdLOWx = sqrt(sdLOWx / notnull_cnt);
	sdLOWy = sqrt(sdLOWy / notnull_cnt);
	sdLOWz = sqrt(sdLOWz / notnull_cnt);
	sdHIGx = sqrt(sdHIGx / notnull_cnt);
	sdHIGy = sqrt(sdHIGy / notnull_cnt);
	sdHIGz = sqrt(sdHIGz / notnull_cnt);

	POSTGIS_DEBUG(3, " standard deviations:");
	POSTGIS_DEBUGF(3, "  LOWx - avg:%f sd:%f", avgLOWx, sdLOWx);
	POSTGIS_DEBUGF(3, "  LOWy - avg:%f sd:%f", avgLOWy, sdLOWy);
	POSTGIS_DEBUGF(3, "  LOWz - avg:%f sd:%f", avgLOWz, sdLOWz);
	POSTGIS_DEBUGF(3, "  HIGx - avg:%f sd:%f", avgHIGx, sdHIGx);
	POSTGIS_DEBUGF(3, "  HIGy - avg:%f sd:%f", avgHIGy, sdHIGy);
	POSTGIS_DEBUGF(3, "  HIGz - avg:%f sd:%f", avgHIGz, sdHIGz);

	histobox.xmin = Max((avgLOWx - SDFACTOR * sdLOWx), sample_extent->xmin);
	histobox.ymin = Max((avgLOWy - SDFACTOR * sdLOWy), sample_extent->ymin);
	histobox.zmin = Max((avgLOWz - SDFACTOR * sdLOWz), sample_extent->zmin);
	histobox.xmax = Min((avgHIGx + SDFACTOR * sdHIGx), sample_extent->xmax);
	histobox.ymax = Min((avgHIGy + SDFACTOR * sdHIGy), sample_extent->ymax);
	histobox.zmax = Min((avgHIGz + SDFACTOR * sdHIGz), sample_extent->zmax);

	POSTGIS_DEBUGF(3, " sd_extent: xmin, ymin, zmin: %f, %f, %f",
	               histobox.xmin, histobox.ymin, histobox.zmin);
	POSTGIS_DEBUGF(3, " sd_extent: xmax, ymax, zmax: %f, %f, %f",
	               histobox.xmax, histobox.ymax, histobox.zmax);

	/*
	 * Third scan:
	 *   o skip hard deviants
	 *   o compute new histogram box
	 */
	for (i = 0; i < notnull_cnt; i++)
	{
		GBOX *box;
		box = (GBOX *)sampleboxes[i];

		if ( box->xmin > histobox.xmax || box->xmax < histobox.xmin ||
		        box->ymin > histobox.ymax || box->ymax < histobox.ymin ||
		        box->zmin > histobox.zmax || box->zmax < histobox.zmin)
		{
			POSTGIS_DEBUGF(4, " feat %d is an hard deviant, skipped", i);

			sampleboxes[i] = NULL;
			continue;
		}

		if ( ! newhistobox )
		{
			newhistobox = palloc(sizeof(GBOX));
			memcpy(newhistobox, box, sizeof(GBOX));
		}
		else
		{
			if ( box->xmin < newhistobox->xmin )
				newhistobox->xmin = box->xmin;
			if ( box->ymin < newhistobox->ymin )
				newhistobox->ymin = box->ymin;
			if ( box->zmin < newhistobox->zmin )
				newhistobox->zmin = box->zmin;
			if ( box->xmax > newhistobox->xmax )
				newhistobox->xmax = box->xmax;
			if ( box->ymax > newhistobox->ymax )
				newhistobox->ymax = box->ymax;
			if ( box->zmax > newhistobox->zmax )
				newhistobox->zmax = box->zmax;
		}
	}

	/* If everything was a deviant, the new histobox is the same as the old histobox */
	if ( ! newhistobox )
	{
		newhistobox = palloc(sizeof(GBOX));
		memcpy(newhistobox, &histobox, sizeof(GBOX));
	}

	/*
	 * Set histogram extent as the intersection between
	 * standard deviation based histogram extent
	 * and computed sample extent after removal of
	 * hard deviants (there might be no hard deviants).
	 */
	if ( histobox.xmin < newhistobox->xmin )
		histobox.xmin = newhistobox->xmin;
	if ( histobox.ymin < newhistobox->ymin )
		histobox.ymin = newhistobox->ymin;
	if ( histobox.zmin < newhistobox->zmin )
		histobox.zmin = newhistobox->zmin;
	if ( histobox.xmax > newhistobox->xmax )
		histobox.xmax = newhistobox->xmax;
	if ( histobox.ymax > newhistobox->ymax )
		histobox.ymax = newhistobox->ymax;
	if ( histobox.zmax > newhistobox->zmax )
		histobox.zmax = newhistobox->zmax;

#else /* ! USE_STANDARD_DEVIATION */

	/*
	* Set histogram extent box
	*/
	histobox.xmin = sample_extent->xmin;
	histobox.ymin = sample_extent->ymin;
	histobox.zmin = sample_extent->zmin;
	histobox.xmax = sample_extent->xmax;
	histobox.ymax = sample_extent->ymax;
	histobox.zmax = sample_extent->zmax;

#endif /* USE_STANDARD_DEVIATION */


	POSTGIS_DEBUGF(3, " histogram_extent: xmin, ymin, zmin: %f, %f, %f",
	               histobox.xmin, histobox.ymin, histobox.zmin);
	POSTGIS_DEBUGF(3, " histogram_extent: xmax, ymax, zmax: %f, %f, %f",
	               histobox.xmax, histobox.ymax, histobox.zmax);

	/* Calculate the size of each dimension */
	sizex = histobox.xmax - histobox.xmin;
	sizey = histobox.ymax - histobox.ymin;
	sizez = histobox.zmax - histobox.zmin;

	/* In order to calculate a suitable aspect ratio for the histogram, we need
	   to work out how many dimensions exist within our sample data (which we
	   assume is representative of the whole data) */
	ndims = 0;
	if (sizex != 0)
	{
		histodims[ndims].axis = 'X';
		histodims[ndims].min = histobox.xmin;
		histodims[ndims].max = histobox.xmax;
		ndims++;
	}

	if (sizey != 0)
	{
		histodims[ndims].axis = 'Y';
		histodims[ndims].min = histobox.ymin;
		histodims[ndims].max = histobox.ymax;

		ndims++;
	}

	if (sizez != 0)
	{
		histodims[ndims].axis = 'Z';
		histodims[ndims].min = histobox.zmin;
		histodims[ndims].max = histobox.zmax;

		ndims++;
	}

	/* Based upon the number of dimensions, we now work out the number of units in each dimension.
	   The number of units is defined as the number of cell blocks in each dimension which make
	   up the total number of histocells; i.e. unitsx * unitsy * unitsz = histocells */

	/* Note: geodetic data is currently indexed in 3 dimensions; however code for remaining dimensions
	   is also included to allow for indexing 3D cartesian data at a later date */

	POSTGIS_DEBUGF(3, "Number of dimensions in sample set: %d", ndims);

	switch (ndims)
	{
	case 0:
		/* An empty column, or multiple points in exactly the same
		   position in space */
		unitsx = 1;
		unitsy = 1;
		unitsz = 1;
		histocells = 1;
		break;

	case 1:
		/* Sample data all lies on a single line, so set the correct
		   units variables depending upon which axis is in use */
		for (i = 0; i < ndims; i++)
		{
			if ( (histodims[i].max - histodims[i].min) != 0)
			{
				/* We've found the non-zero dimension, so set the
				   units variables accordingly */
				switch (histodims[i].axis)
				{
				case 'X':
					unitsx = histocells;
					unitsy = 1;
					unitsz = 1;
					break;

				case 'Y':
					unitsx = 1;
					unitsy = histocells;
					unitsz = 1;
					break;

				case 'Z':
					unitsx = 1;
					unitsy = 1;
					unitsz = histocells;
					break;
				}
			}
		}
		break;

	case 2:
		/* Sample data lies within 2D space: divide the total area by the total
		   number of cells, and thus work out the edge size of the unit block */
		edgelength = sqrt(
		                 Abs(histodims[0].max - histodims[0].min) *
		                 Abs(histodims[1].max - histodims[1].min) / (double)histocells
		             );

		/* The calculation is easy; the harder part is to work out which dimensions
		   we actually have to set the units variables appropriately */
		if (histodims[0].axis == 'X' && histodims[1].axis == 'Y')
		{
			/* X and Y */
			unitsx = Abs(histodims[0].max - histodims[0].min) / edgelength;
			unitsy = Abs(histodims[1].max - histodims[1].min) / edgelength;
			unitsz = 1;
		}
		else if (histodims[0].axis == 'Y' && histodims[1].axis == 'X')
		{
			/* Y and X */
			unitsx = Abs(histodims[1].max - histodims[1].min) / edgelength;
			unitsy = Abs(histodims[0].max - histodims[0].min) / edgelength;
			unitsz = 1;
		}
		else if (histodims[0].axis == 'X' && histodims[1].axis == 'Z')
		{
			/* X and Z */
			unitsx = Abs(histodims[0].max - histodims[0].min) / edgelength;
			unitsy = 1;
			unitsz = Abs(histodims[1].max - histodims[1].min) / edgelength;
		}
		else if (histodims[0].axis == 'Z' && histodims[1].axis == 'X')
		{
			/* Z and X */
			unitsx = Abs(histodims[0].max - histodims[0].min) / edgelength;
			unitsy = 1;
			unitsz = Abs(histodims[1].max - histodims[1].min) / edgelength;
		}
		else if (histodims[0].axis == 'Y' && histodims[1].axis == 'Z')
		{
			/* Y and Z */
			unitsx = 1;
			unitsy = Abs(histodims[0].max - histodims[0].min) / edgelength;
			unitsz = Abs(histodims[1].max - histodims[1].min) / edgelength;
		}
		else if (histodims[0].axis == 'Z' && histodims[1].axis == 'Y')
		{
			/* Z and X */
			unitsx = 1;
			unitsy = Abs(histodims[1].max - histodims[1].min) / edgelength;
			unitsz = Abs(histodims[0].max - histodims[0].min) / edgelength;
		}

		break;

	case 3:
		/* Sample data lies within 3D space: divide the total volume by the total
		   number of cells, and thus work out the edge size of the unit block */
		edgelength = pow(
		                 Abs(histodims[0].max - histodims[0].min) *
		                 Abs(histodims[1].max - histodims[1].min) *
		                 Abs(histodims[2].max - histodims[2].min) / (double)histocells,
		                 (double)1/3);

		/* Units are simple in 3 dimensions */
		unitsx = Abs(histodims[0].max - histodims[0].min) / edgelength;
		unitsy = Abs(histodims[1].max - histodims[1].min) / edgelength;
		unitsz = Abs(histodims[2].max - histodims[2].min) / edgelength;

		break;
	}

	POSTGIS_DEBUGF(3, " computed histogram grid size (X,Y,Z): %d x %d x %d (%d out of %d cells)", unitsx, unitsy, unitsz, unitsx * unitsy * unitsz, histocells);

	/*
	 * Create the histogram (GEOG_STATS)
	 */
	old_context = MemoryContextSwitchTo(stats->anl_context);
	geog_stats_size = sizeof(GEOG_STATS) + (histocells - 1) * sizeof(float4);
	geogstats = palloc(geog_stats_size);
	MemoryContextSwitchTo(old_context);

	geogstats->dims = ndims;
	geogstats->xmin = histobox.xmin;
	geogstats->ymin = histobox.ymin;
	geogstats->zmin = histobox.zmin;
	geogstats->xmax = histobox.xmax;
	geogstats->ymax = histobox.ymax;
	geogstats->zmax = histobox.zmax;
	geogstats->unitsx = unitsx;
	geogstats->unitsy = unitsy;
	geogstats->unitsz = unitsz;
	geogstats->totalrows = totalrows;

	/* Initialize all values to 0 */
	for (i = 0; i < histocells; i++)
		geogstats->value[i] = 0;


	/*
	 * Fourth scan:
	 *  o fill histogram values with the number of
	 *    features' bbox overlaps: a feature's bvol
	 *    can fully overlap (1) or partially overlap
	 *    (fraction of 1) an histogram cell.
	 *
	 *  o compute total cells occupation
	 *
	 */

	POSTGIS_DEBUG(3, "Beginning histogram intersection calculations");

	for (i = 0; i < notnull_cnt; i++)
	{
		GBOX *box;

		/* Note these array index variables are zero-based */
		int x_idx_min, x_idx_max, x;
		int y_idx_min, y_idx_max, y;
		int z_idx_min, z_idx_max, z;
		int numcells = 0;

		box = (GBOX *)sampleboxes[i];
		if ( ! box ) continue; /* hard deviant.. */

		/* give backend a chance of interrupting us */
		vacuum_delay_point();

		POSTGIS_DEBUGF(4, " feat %d box is %f %f %f, %f %f %f",
		               i, box->xmax, box->ymax, box->zmax, box->xmin, box->ymin, box->zmin);

		/* Find first overlapping unitsx cell */
		x_idx_min = (box->xmin - geogstats->xmin) / sizex * unitsx;
		if (x_idx_min <0) x_idx_min = 0;
		if (x_idx_min >= unitsx) x_idx_min = unitsx - 1;

		/* Find first overlapping unitsy cell */
		y_idx_min = (box->ymin - geogstats->ymin) / sizey * unitsy;
		if (y_idx_min <0) y_idx_min = 0;
		if (y_idx_min >= unitsy) y_idx_min = unitsy - 1;

		/* Find first overlapping unitsz cell */
		z_idx_min = (box->zmin - geogstats->zmin) / sizez * unitsz;
		if (z_idx_min <0) z_idx_min = 0;
		if (z_idx_min >= unitsz) z_idx_min = unitsz - 1;

		/* Find last overlapping unitsx cell */
		x_idx_max = (box->xmax - geogstats->xmin) / sizex * unitsx;
		if (x_idx_max <0) x_idx_max = 0;
		if (x_idx_max >= unitsx ) x_idx_max = unitsx - 1;

		/* Find last overlapping unitsy cell */
		y_idx_max = (box->ymax - geogstats->ymin) / sizey * unitsy;
		if (y_idx_max <0) y_idx_max = 0;
		if (y_idx_max >= unitsy) y_idx_max = unitsy - 1;

		/* Find last overlapping unitsz cell */
		z_idx_max = (box->zmax - geogstats->zmin) / sizez * unitsz;
		if (z_idx_max <0) z_idx_max = 0;
		if (z_idx_max >= unitsz) z_idx_max = unitsz - 1;

		POSTGIS_DEBUGF(4, " feat %d overlaps unitsx %d-%d, unitsy %d-%d, unitsz %d-%d",
		               i, x_idx_min, x_idx_max, y_idx_min, y_idx_max, z_idx_min, z_idx_max);

		/* Calculate the feature coverage - this of course depends upon the number of dims */
		switch (ndims)
		{
		case 1:
			total_cells_coverage++;
			break;

		case 2:
			total_cells_coverage += (box->xmax - box->xmin) * (box->ymax - box->ymin);
			break;

		case 3:
			total_cells_coverage += (box->xmax - box->xmin) * (box->ymax - box->ymin) *
			                        (box->zmax - box->zmin);
			break;
		}

		/*
		 * the {x,y,z}_idx_{min,max}
		 * define the grid squares that the box intersects
		 */

		for (z = z_idx_min; z <= z_idx_max; z++)
		{
			for (y = y_idx_min; y <= y_idx_max; y++)
			{
				for (x = x_idx_min; x <= x_idx_max; x++)
				{
					geogstats->value[x + y * unitsx + z * unitsx * unitsy] += 1;
					numcells++;
				}
			}
		}

		/*
		 * before adding to the total cells
		 * we could decide if we really
		 * want this feature to count
		 */
		total_count_cells += numcells;

		examinedsamples++;
	}

	POSTGIS_DEBUGF(3, " examined_samples: %d/%d", examinedsamples, samplerows);

	if ( ! examinedsamples )
	{
		elog(NOTICE, " no examined values, invalid stats");
		stats->stats_valid = false;

		POSTGIS_DEBUG(3, " no stats have been gathered");

		return;
	}

	/** TODO: what about null features (TODO) */
	geogstats->avgFeatureCells = (float4)total_count_cells / examinedsamples;
	geogstats->avgFeatureCoverage = total_cells_coverage / examinedsamples;

	POSTGIS_DEBUGF(3, " histo: total_boxes_cells: %d", total_count_cells);
	POSTGIS_DEBUGF(3, " histo: avgFeatureCells: %f", geogstats->avgFeatureCells);
	POSTGIS_DEBUGF(3, " histo: avgFeatureCoverage: %f", geogstats->avgFeatureCoverage);

	/*
	 * Normalize histogram
	 *
	 * We divide each histogram cell value
	 * by the number of samples examined.
	 *
	 */
	for (i = 0; i < histocells; i++)
		geogstats->value[i] /= examinedsamples;

#if POSTGIS_DEBUG_LEVEL >= 4
	/* Dump the resulting histogram for analysis */
	{
		int x, y, z;
		for (x = 0; x < unitsx; x++)
		{
			for (y = 0; y < unitsy; y++)
			{
				for (z = 0; z < unitsz; z++)
				{
					POSTGIS_DEBUGF(4, " histo[%d,%d,%d] = %.15f", x, y, z,
					               geogstats->value[x + y * unitsx + z * unitsx * unitsy]);
				}
			}
		}
	}
#endif

	/*
	 * Write the statistics data
	 */
	stats->stakind[0] = STATISTIC_KIND_GEOGRAPHY;
	stats->staop[0] = InvalidOid;
	stats->stanumbers[0] = (float4 *)geogstats;
	stats->numnumbers[0] = geog_stats_size/sizeof(float4);

	stats->stanullfrac = (float4)(samplerows - notnull_cnt)/samplerows;
	stats->stawidth = total_width/notnull_cnt;
	stats->stadistinct = -1.0;

	POSTGIS_DEBUGF(3, " out: slot 0: kind %d (STATISTIC_KIND_GEOGRAPHY)",
	               stats->stakind[0]);
	POSTGIS_DEBUGF(3, " out: slot 0: op %d (InvalidOid)", stats->staop[0]);
	POSTGIS_DEBUGF(3, " out: slot 0: numnumbers %d", stats->numnumbers[0]);
	POSTGIS_DEBUGF(3, " out: null fraction: %d/%d=%g", (samplerows - notnull_cnt), samplerows, stats->stanullfrac);
	POSTGIS_DEBUGF(3, " out: average width: %d bytes", stats->stawidth);
	POSTGIS_DEBUG(3, " out: distinct values: all (no check done)");

	stats->stats_valid = true;

}


/**
 * This function will be called when the ANALYZE command is run
 * on a column of the "geography" type.
 *
 * It will need to return a stats builder function reference
 * and a "minimum" sample rows to feed it.
 * If we want analysis to be completely skipped we can return
 * FALSE and leave output vals untouched.
 *
 * What we know from this call is:
 *
 * 	o The pg_attribute row referring to the specific column.
 * 	  Could be used to get reltuples from pg_class (which
 * 	  might quite inexact though...) and use them to set an
 * 	  appropriate minimum number of sample rows to feed to
 * 	  the stats builder. The stats builder will also receive
 * 	  a more accurate "estimation" of the number or rows.
 *
 * 	o The pg_type row for the specific column.
 * 	  Could be used to set stat builder / sample rows
 * 	  based on domain type (when postgis will be implemented
 * 	  that way).
 *
 * Being this experimental we'll stick to a static stat_builder/sample_rows
 * value for now.
 *
 */

PG_FUNCTION_INFO_V1(geography_analyze);
Datum geography_analyze(PG_FUNCTION_ARGS)
{
	VacAttrStats *stats = (VacAttrStats *)PG_GETARG_POINTER(0);
	Form_pg_attribute attr = stats->attr;

	POSTGIS_DEBUG(2, "geography_analyze called");

	/* If the attstattarget column is negative, use the default value */
	/* NB: it is okay to scribble on stats->attr since it's a copy */
	if (attr->attstattarget < 0)
		attr->attstattarget = default_statistics_target;

	POSTGIS_DEBUGF(3, " attribute stat target: %d", attr->attstattarget);

	/* Setup the minimum rows and the algorithm function */
	stats->minrows = 300 * stats->attr->attstattarget;
	stats->compute_stats = compute_geography_stats;

	POSTGIS_DEBUGF(3, " minrows: %d", stats->minrows);

	/* Indicate we are done successfully */
	PG_RETURN_BOOL(true);
}
