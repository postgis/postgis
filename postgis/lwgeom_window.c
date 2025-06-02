/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
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
 * Copyright 2016 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright 2016 Daniel Baston <dbaston@gmail.com>
 *
 **********************************************************************/

#include "../postgis_config.h"

/* PostgreSQL */
#include "postgres.h"
#include "funcapi.h"
#include "windowapi.h"
#include "utils/builtins.h"

/* PostGIS */
#include "liblwgeom.h"
#include "lwunionfind.h" /* TODO move into liblwgeom.h ? */
#include "lwgeom_geos.h"
#include "lwgeom_log.h"
#include "lwgeom_pg.h"


typedef struct {
	bool	isdone;
	bool	isnull;
	int		result[1];
	/* variable length */
} kmeans_context;

typedef struct
{
	uint32_t cluster_id;
	char is_null;        /* NULL may result from a NULL geometry input, or it may be used by
							algorithms such as DBSCAN that do not assign all inputs to a
							cluster. */
} cluster_entry;

typedef struct
{
	char is_error;
	cluster_entry clusters[1];
} cluster_context;

static cluster_context*
fetch_cluster_context(WindowObject win_obj, uint32_t ngeoms)
{
	size_t context_sz = sizeof(cluster_context) + (ngeoms * sizeof(cluster_entry));
	cluster_context* context = WinGetPartitionLocalMemory(win_obj, context_sz);
	return context;
}

static LWGEOM*
read_lwgeom_from_partition(WindowObject win_obj, uint32_t i, bool* is_null)
{
	GSERIALIZED* g;
	Datum arg = WinGetFuncArgInPartition(win_obj, 0, i, WINDOW_SEEK_HEAD, false, is_null, NULL);

	if (*is_null) {
		/* So that the indexes in our clustering input array can match our partition positions,
		 * toss an empty point into the clustering inputs, as a pass-through.
		 * NOTE: this will cause gaps in the output cluster id sequence.
		 * */
		return lwpoint_as_lwgeom(lwpoint_construct_empty(0, 0, 0));
	}

	g = (GSERIALIZED*) PG_DETOAST_DATUM_COPY(arg);
	return lwgeom_from_gserialized(g);
}

static GEOSGeometry*
read_geos_from_partition(WindowObject win_obj, uint32_t i, bool* is_null)
{
	GSERIALIZED* g;
	LWGEOM* lwg;
	GEOSGeometry* gg;
	Datum arg = WinGetFuncArgInPartition(win_obj, 0, i, WINDOW_SEEK_HEAD, false, is_null, NULL);

	if (*is_null) {
		/* So that the indexes in our clustering input array can match our partition positions,
		 * toss an empty point into the clustering inputs, as a pass-through.
		 * NOTE: this will cause gaps in the output cluster id sequence.
		 * */
		lwg = lwpoint_as_lwgeom(lwpoint_construct_empty(0, 0, 0));
		gg = LWGEOM2GEOS(lwg, LW_FALSE);
		lwgeom_free(lwg);
		return gg;
	}

	g = (GSERIALIZED*) PG_DETOAST_DATUM_COPY(arg);
	lwg = lwgeom_from_gserialized(g);
	gg = LWGEOM2GEOS(lwg, LW_TRUE);
	lwgeom_free(lwg);
	if (!gg) {
		*is_null = LW_TRUE;
	}
	return gg;
}

extern Datum ST_ClusterDBSCAN(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_ClusterDBSCAN);
Datum ST_ClusterDBSCAN(PG_FUNCTION_ARGS)
{
	WindowObject win_obj = PG_WINDOW_OBJECT();
	uint32_t row = WinGetCurrentPosition(win_obj);
	uint32_t ngeoms = WinGetPartitionRowCount(win_obj);
	cluster_context* context = fetch_cluster_context(win_obj, ngeoms);

	if (row == 0) /* beginning of the partition; do all of the work now */
	{
		uint32_t i;
		uint32_t* result_ids;
		LWGEOM** geoms;
		char* is_in_cluster = NULL;
		UNIONFIND* uf;
		bool tolerance_is_null;
		bool minpoints_is_null;
		Datum tolerance_datum = WinGetFuncArgCurrent(win_obj, 1, &tolerance_is_null);
		Datum minpoints_datum = WinGetFuncArgCurrent(win_obj, 2, &minpoints_is_null);
		double tolerance = DatumGetFloat8(tolerance_datum);
		int minpoints = DatumGetInt32(minpoints_datum);

		context->is_error = LW_TRUE; /* until proven otherwise */

		/* Validate input parameters */
		if (tolerance_is_null || tolerance < 0)
		{
			lwpgerror("Tolerance must be a positive number, got %g", tolerance);
			PG_RETURN_NULL();
		}
		if (minpoints_is_null || minpoints < 0)
		{
			lwpgerror("Minpoints must be a positive number, got %d", minpoints);
		}

		initGEOS(lwpgnotice, lwgeom_geos_error);
		geoms = lwalloc(ngeoms * sizeof(LWGEOM*));
		uf = UF_create(ngeoms);
		for (i = 0; i < ngeoms; i++)
		{
			bool geom_is_null;
			geoms[i] = read_lwgeom_from_partition(win_obj, i, &geom_is_null);
			context->clusters[i].is_null = geom_is_null;

			if (!geoms[i]) {
				/* TODO release memory ? */
				lwpgerror("Error reading geometry.");
				PG_RETURN_NULL();
			}
		}

		if (union_dbscan(geoms, ngeoms, uf, tolerance, minpoints, minpoints > 1 ? &is_in_cluster : NULL) == LW_SUCCESS)
			context->is_error = LW_FALSE;

		for (i = 0; i < ngeoms; i++)
		{
			lwgeom_free(geoms[i]);
		}
		lwfree(geoms);

		if (context->is_error)
		{
			UF_destroy(uf);
			if (is_in_cluster)
				lwfree(is_in_cluster);
			lwpgerror("Error during clustering");
			PG_RETURN_NULL();
		}

		result_ids = UF_get_collapsed_cluster_ids(uf, is_in_cluster);
		for (i = 0; i < ngeoms; i++)
		{
			if (minpoints > 1 && !is_in_cluster[i])
			{
				context->clusters[i].is_null = LW_TRUE;
			}
			else
			{
				context->clusters[i].cluster_id = result_ids[i];
			}
		}

		lwfree(result_ids);
		UF_destroy(uf);
	}

	if (context->clusters[row].is_null)
		PG_RETURN_NULL();

	PG_RETURN_INT32(context->clusters[row].cluster_id);
}

extern Datum ST_ClusterWithinWin(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_ClusterWithinWin);
Datum ST_ClusterWithinWin(PG_FUNCTION_ARGS)
{
	WindowObject win_obj = PG_WINDOW_OBJECT();
	uint32_t row = WinGetCurrentPosition(win_obj);
	uint32_t ngeoms = WinGetPartitionRowCount(win_obj);
	cluster_context* context = fetch_cluster_context(win_obj, ngeoms);

	if (row == 0) /* beginning of the partition; do all of the work now */
	{
		uint32_t i;
		uint32_t* result_ids;
		LWGEOM** geoms;
		UNIONFIND* uf;
		bool tolerance_is_null;
		double tolerance = DatumGetFloat8(WinGetFuncArgCurrent(win_obj, 1, &tolerance_is_null));

		/* Validate input parameters */
		if (tolerance_is_null || tolerance < 0)
		{
			lwpgerror("Tolerance must be a positive number, got %g", tolerance);
			PG_RETURN_NULL();
		}

		context->is_error = LW_TRUE; /* until proven otherwise */

		geoms = lwalloc(ngeoms * sizeof(LWGEOM*));
		uf = UF_create(ngeoms);
		for (i = 0; i < ngeoms; i++)
		{
			bool geom_is_null;
			geoms[i] = read_lwgeom_from_partition(win_obj, i, &geom_is_null);
			context->clusters[i].is_null = geom_is_null;

			if (!geoms[i])
			{
				lwpgerror("Error reading geometry.");
				PG_RETURN_NULL();
			}
		}

		initGEOS(lwpgnotice, lwgeom_geos_error);

		if (union_dbscan(geoms, ngeoms, uf, tolerance, 1, NULL) == LW_SUCCESS)
			context->is_error = LW_FALSE;

		for (i = 0; i < ngeoms; i++)
		{
			lwgeom_free(geoms[i]);
		}
		lwfree(geoms);

		if (context->is_error)
		{
			UF_destroy(uf);
			lwpgerror("Error during clustering");
			PG_RETURN_NULL();
		}

		result_ids = UF_get_collapsed_cluster_ids(uf, NULL);
		for (i = 0; i < ngeoms; i++)
		{
			context->clusters[i].cluster_id = result_ids[i];
		}

		lwfree(result_ids);
		UF_destroy(uf);
	}

	if (context->clusters[row].is_null)
		PG_RETURN_NULL();

	PG_RETURN_INT32(context->clusters[row].cluster_id);
}

extern Datum ST_ClusterIntersectingWin(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_ClusterIntersectingWin);
Datum ST_ClusterIntersectingWin(PG_FUNCTION_ARGS)
{
	WindowObject win_obj = PG_WINDOW_OBJECT();
	uint32_t row = WinGetCurrentPosition(win_obj);
	uint32_t ngeoms = WinGetPartitionRowCount(win_obj);
	cluster_context* context = fetch_cluster_context(win_obj, ngeoms);

	if (row == 0) /* beginning of the partition; do all of the work now */
	{
		uint32_t i;
		uint32_t* result_ids;
		GEOSGeometry** geoms = lwalloc(ngeoms * sizeof(GEOSGeometry*));
		UNIONFIND* uf = UF_create(ngeoms);

		context->is_error = LW_TRUE; /* until proven otherwise */

		initGEOS(lwpgnotice, lwgeom_geos_error);

		for (i = 0; i < ngeoms; i++)
		{
			bool geom_is_null;
			geoms[i] = read_geos_from_partition(win_obj, i, &geom_is_null);
			context->clusters[i].is_null = geom_is_null;

			if (!geoms[i])
			{
				lwpgerror("Error reading geometry.");
				PG_RETURN_NULL();
			}
		}

		if (union_intersecting_pairs(geoms, ngeoms, uf) == LW_SUCCESS)
			context->is_error = LW_FALSE;

		for (i = 0; i < ngeoms; i++)
		{
			GEOSGeom_destroy(geoms[i]);
		}
		lwfree(geoms);

		if (context->is_error)
		{
			UF_destroy(uf);
			lwpgerror("Error during clustering");
			PG_RETURN_NULL();
		}

		result_ids = UF_get_collapsed_cluster_ids(uf, NULL);
		for (i = 0; i < ngeoms; i++)
		{
			context->clusters[i].cluster_id = result_ids[i];
		}

		lwfree(result_ids);
		UF_destroy(uf);
	}

	if (context->clusters[row].is_null)
		PG_RETURN_NULL();

	PG_RETURN_INT32(context->clusters[row].cluster_id);
}


extern Datum ST_ClusterKMeans(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_ClusterKMeans);
Datum ST_ClusterKMeans(PG_FUNCTION_ARGS)
{
	WindowObject winobj = PG_WINDOW_OBJECT();
	kmeans_context *context;
	int64 curpos, rowcount;

	rowcount = WinGetPartitionRowCount(winobj);
	context = (kmeans_context *)
		WinGetPartitionLocalMemory(winobj,
			sizeof(kmeans_context) + sizeof(int) * rowcount);

	if (!context->isdone)
	{
		int       i, k, N;
		bool      isnull, isout;
		double max_radius = 0.0;
		LWGEOM    **geoms;
		int       *r;
		Datum argdatum;

		/* What is K? If it's NULL or invalid, we can't proceed */
		argdatum = WinGetFuncArgCurrent(winobj, 1, &isnull);
		k = DatumGetInt32(argdatum);
		if (isnull || k <= 0)
		{
			context->isdone = true;
			context->isnull = true;
			PG_RETURN_NULL();
		}

		/* We also need a non-zero N */
		N = (int) WinGetPartitionRowCount(winobj);
		if (N <= 0)
		{
			context->isdone = true;
			context->isnull = true;
			PG_RETURN_NULL();
		}

		/* Maximum cluster radius. 0 if not set*/
		argdatum = WinGetFuncArgCurrent(winobj, 2, &isnull);
		if (!isnull)
		{
			max_radius = DatumGetFloat8(argdatum);
			if (max_radius < 0)
				max_radius = 0.0;
		}

		/* Error out if N < K */
		if (N<k)
			lwpgerror("K (%d) must be smaller than the number of rows in the group (%d)", k, N);

		/* Read all the geometries from the partition window into a list */
		geoms = palloc(sizeof(LWGEOM*) * N);
		for (i = 0; i < N; i++)
		{
			GSERIALIZED *g;
			Datum arg = WinGetFuncArgInPartition(winobj, 0, i,
						WINDOW_SEEK_HEAD, false, &isnull, &isout);

			/* Null geometries are entered as NULL pointers */
			if (isnull)
			{
				geoms[i] = NULL;
				continue;
			}

			g = (GSERIALIZED*)PG_DETOAST_DATUM_COPY(arg);
			geoms[i] = lwgeom_from_gserialized(g);
		}

		/* Calculate k-means on the list! */
		r = lwgeom_cluster_kmeans((const LWGEOM **)geoms, N, k, max_radius);

		/* Clean up */
		for (i = 0; i < N; i++)
			if (geoms[i])
				lwgeom_free(geoms[i]);

		pfree(geoms);

		if (!r)
		{
			context->isdone = true;
			context->isnull = true;
			PG_RETURN_NULL();
		}

		/* Safe the result */
		memcpy(context->result, r, sizeof(int) * N);
		lwfree(r);
		context->isdone = true;
	}

	if (context->isnull)
		PG_RETURN_NULL();

	curpos = WinGetCurrentPosition(winobj);
	PG_RETURN_INT32(context->result[curpos]);
}


/********************************************************************************
 * GEOS Coverage Functions
 *
 * The GEOS "coverage" support is a set of functions for working with
 * "implicit coverages". That is, collections of polygonal geometry
 * that have perfectly shared edges. Since perfectly matched edges are
 * fast to detect, "building a coverage" on these inputs is fast, so
 * operations like edge simplification and so on can include the "build"
 * step with relatively low cost.
 *
 */

/*
 * For CoverageUnion and CoverageSimplify, clean up
 * the start of a collection if things fail in mid-stream.
 */
static void
coverage_destroy_geoms(GEOSGeometry **geoms, uint32 ngeoms)
{
	if (!geoms) return;
	if (!ngeoms) return;
	for (uint32 i = 0; i < ngeoms; i++)
	{
		if(geoms[i])
			GEOSGeom_destroy(geoms[i]);
	}
}

#if POSTGIS_GEOS_VERSION >= 31200

/*
 * For CoverageIsValid and CoverageSimplify, maintain context
 * of unioned output and the position of the resultant in that
 * output relative to the index of the input geometries.
 */
typedef struct {
	bool	isdone;
	bool	isnull;
	LWCOLLECTION *geom;
	int64   idx[0];
	/* variable length */
} coverage_context;

/*
 * For CoverageIsValid and CoverageSimplify, read the context
 * out of the WindowObject.
 */
static coverage_context *
coverage_context_fetch(WindowObject winobj, int64 rowcount)
{
	size_t ctx_size = sizeof(coverage_context) + rowcount * sizeof(int64);
	coverage_context *ctx = (coverage_context*) WinGetPartitionLocalMemory(
		winobj, ctx_size);
	return ctx;
}

/*
 * For CoverageIsValid and CoverageSimplify, read all the
 * geometries in this partition and form a GEOS geometry
 * collection out of them.
 */
static GEOSGeometry*
coverage_read_partition_into_collection(
	WindowObject winobj,
	coverage_context* context)
{
	int64 rowcount = WinGetPartitionRowCount(winobj);
	GEOSGeometry* geos;
	GEOSGeometry** geoms;
	uint32 i, ngeoms = 0, gtype;

	/* Read in all the geometries in this partition */
	geoms = palloc(rowcount * sizeof(GEOSGeometry*));
	for (i = 0; i < rowcount; i++)
	{
		GSERIALIZED* gser;
		bool isnull, isout;
		bool isempty, ispolygonal;
		Datum d;

		/* Read geometry in first argument */
		d = WinGetFuncArgInPartition(winobj, 0, i,
				WINDOW_SEEK_HEAD, false, &isnull, &isout);

		/*
		 * The input to the GEOS function will be smaller than
		 * the input to this window, since we will not feed
		 * GEOS nulls or empties. So we need to maintain a
		 * map (context->idx) from the window position of the
		 * input to the GEOS position, so we can put the
		 * right result in the output stream. Things we want to
		 * skip get an index of -1.
		 */

		/* Skip NULL inputs and move on */
		if (isnull)
		{
			context->idx[i] = -1;
			continue;
		}

		gser = (GSERIALIZED*)PG_DETOAST_DATUM(d);
		gtype = gserialized_get_type(gser);
		isempty = gserialized_is_empty(gser);
		ispolygonal = (gtype == MULTIPOLYGONTYPE) || (gtype == POLYGONTYPE);

		/* Skip empty or non-polygonal inputs */
		if (isempty || !ispolygonal)
		{
			context->idx[i] = -1;
			continue;
		}

		/* Skip failed inputs */
		geos = POSTGIS2GEOS(gser);
		if (!geos)
		{
			context->idx[i] = -1;
			continue;
		}

		context->idx[i] = ngeoms;
		geoms[ngeoms] = geos;
		ngeoms = ngeoms + 1;
	}

	/*
	 * Create the GEOS input collection! The new
	 * collection takes ownership of the input GEOSGeometry
	 * objects, leaving just the geoms array, which
	 * will be cleaned up on function exit.
	 */
	geos = GEOSGeom_createCollection(
		GEOS_GEOMETRYCOLLECTION,
		geoms, ngeoms);

	/*
	 * If the creation failed, the objects will still be
	 * hanging around, so clean them up first. Should
	 * never happen, really.
	 */
	if (!geos)
	{
		coverage_destroy_geoms(geoms, ngeoms);
		return NULL;
	}

	pfree(geoms);
	return geos;
}


/*
 * The coverage_window_calculation function is just
 * the shared machinery of the two window functions
 * CoverageIsValid and CoverageSimplify which are almost
 * the same except the GEOS function they call. That
 * operating mode is controlled with this enumeration.
 */
enum {
	 COVERAGE_SIMPLIFY = 0
	,COVERAGE_ISVALID  = 1
#if POSTGIS_GEOS_VERSION >= 31400
	,COVERAGE_CLEAN    = 2
#endif
};

static char * overlapMergeStrategies[] = {
    /* Merge strategy that chooses polygon with longest common border */
    "MERGE_LONGEST_BORDER",
    /* Merge strategy that chooses polygon with maximum area */
    "MERGE_MAX_AREA",
    /* Merge strategy that chooses polygon with minimum area */
    "MERGE_MIN_AREA",
    /* Merge strategy that chooses polygon with smallest input index */
    "MERGE_MIN_INDEX"
};

static int
coverage_merge_strategy(const char *strategy)
{
	size_t stratLen = sizeof(overlapMergeStrategies) / sizeof(overlapMergeStrategies[0]);
	for (uint32_t i = 0; i < stratLen; i++)
	{
		if (strcasecmp(strategy, overlapMergeStrategies[i]) == 0)
		{
			return i;
		}
	}
	return -1;
}

/*
 * This calculation is shared by both coverage operations
 * since they have the same pattern of "consume collection,
 * return collection", and are both window functions.
 */
static Datum
coverage_window_calculation(PG_FUNCTION_ARGS, int mode)
{
	WindowObject winobj = PG_WINDOW_OBJECT();
	int64 curpos = WinGetCurrentPosition(winobj);
	int64 rowcount = WinGetPartitionRowCount(winobj);
	coverage_context *context = coverage_context_fetch(winobj, rowcount);
	GSERIALIZED* result;
	MemoryContext uppercontext = fcinfo->flinfo->fn_mcxt;
	MemoryContext oldcontext;
	const LWGEOM* subgeom;

	if (!context->isdone)
	{
		bool isnull;
		GEOSGeometry *output = NULL;
		GEOSGeometry *input = NULL;
		Datum d;

		if (!fcinfo->flinfo)
			elog(ERROR, "%s: Could not find upper context", __func__);

		if (rowcount == 0)
		{
			context->isdone = true;
			context->isnull = true;
			PG_RETURN_NULL();
		}

		initGEOS(lwpgnotice, lwgeom_geos_error);

		input = coverage_read_partition_into_collection(winobj, context);
		if (!input)
			HANDLE_GEOS_ERROR("Failed to create collection");

		/* Run the correct GEOS function for the calling mode */
		if (mode == COVERAGE_SIMPLIFY)
		{
			bool simplifyBoundary = true;
			double tolerance = 0.0;

			d = WinGetFuncArgCurrent(winobj, 1, &isnull);
			if (!isnull) tolerance = DatumGetFloat8(d);

			d = WinGetFuncArgCurrent(winobj, 2, &isnull);
			if (!isnull) simplifyBoundary = DatumGetFloat8(d);

			/* GEOSCoverageSimplifyVW is "preserveBoundary" so we invert simplifyBoundary */
			output = GEOSCoverageSimplifyVW(input, tolerance, !simplifyBoundary);
		}
		else if (mode == COVERAGE_ISVALID)
		{
			double tolerance = 0.0;
			d = WinGetFuncArgCurrent(winobj, 1, &isnull);
			if (!isnull) tolerance = DatumGetFloat8(d);
			GEOSCoverageIsValid(input, tolerance, &output);
		}

#if POSTGIS_GEOS_VERSION >= 31400

		else if (mode == COVERAGE_CLEAN)
		{
			double snappingDistance = 0.0;
			double gapMaximumWidth = 0.0;
			text *overlapMergeStrategyText;
			int overlapMergeStrategy;
			GEOSCoverageCleanParams *params = NULL;

			d = WinGetFuncArgCurrent(winobj, 1, &isnull);
			if (!isnull) snappingDistance = DatumGetFloat8(d);

			d = WinGetFuncArgCurrent(winobj, 2, &isnull);
			if (!isnull) gapMaximumWidth = DatumGetFloat8(d);

			d = WinGetFuncArgCurrent(winobj, 3, &isnull);
			// if (!isnull) overlapMergeStrategy = DatumGetInt32(d);
			if (!isnull)
			{
				overlapMergeStrategyText = DatumGetTextP(d);
				overlapMergeStrategy = coverage_merge_strategy(text_to_cstring(overlapMergeStrategyText));
			}
			else
			{
				overlapMergeStrategy = 0; /* Default to MERGE_LONGEST_BORDER */
			}
			if (overlapMergeStrategy < 0)
			{
				HANDLE_GEOS_ERROR("Invalid OverlapMergeStrategy");
			}

			params = GEOSCoverageCleanParams_create();
			GEOSCoverageCleanParams_setGapMaximumWidth(params, gapMaximumWidth);
			GEOSCoverageCleanParams_setSnappingDistance(params, snappingDistance);
			if (!GEOSCoverageCleanParams_setOverlapMergeStrategy(params, overlapMergeStrategy))
			{
				GEOSCoverageCleanParams_destroy(params);
				HANDLE_GEOS_ERROR("Invalid OverlapMergeStrategy");
			}

			output = GEOSCoverageCleanWithParams(input, params);
			GEOSCoverageCleanParams_destroy(params);
		}
#endif
		else
		{
			elog(ERROR, "Unknown mode, never get here");
		}

		GEOSGeom_destroy(input);

		if (!output)
		{
			HANDLE_GEOS_ERROR("Failed to process collection");
		}

		oldcontext = MemoryContextSwitchTo(uppercontext);
		context->geom = (LWCOLLECTION *) GEOS2LWGEOM(output, GEOSHasZ(output));
		MemoryContextSwitchTo(oldcontext);
		GEOSGeom_destroy(output);

		context->isdone = true;
	}

	/* Bomb out of any errors */
	if (context->isnull)
		PG_RETURN_NULL();

	/* Propagate the null entries */
	if (context->idx[curpos] < 0)
		PG_RETURN_NULL();

	/*
	 * Geometry serialization is not const-safe! (we
	 * calculate bounding boxes on demand) so we need
	 * to make sure we have a persistent context when
	 * we call the serialization, lest we create dangling
	 * pointers in the object. It's possible we could
	 * ignore them, skip the manual lwcollection_free
	 * and let the aggcontext deletion take
	 * care of the memory.
	 */
	oldcontext = MemoryContextSwitchTo(uppercontext);
	subgeom = lwcollection_getsubgeom(context->geom, context->idx[curpos]);
	if (mode == COVERAGE_ISVALID && lwgeom_is_empty(subgeom))
	{
		result = NULL;
	}
	else
	{
		result = geometry_serialize((LWGEOM*)subgeom);
	}
	MemoryContextSwitchTo(oldcontext);

	/* When at the end of the partition, release the */
	/* simplified collection we have been reading. */
	if (curpos == rowcount - 1)
		lwcollection_free(context->geom);

	if (!result) PG_RETURN_NULL();

	PG_RETURN_POINTER(result);

}
#endif /* POSTGIS_GEOS_VERSION >= 31200 */


extern Datum ST_CoverageSimplify(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_CoverageSimplify);
Datum ST_CoverageSimplify(PG_FUNCTION_ARGS)
{
#if POSTGIS_GEOS_VERSION < 31200

	lwpgerror("The GEOS version this PostGIS binary "
		"was compiled against (%d) doesn't support "
		"'GEOSCoverageSimplifyVW' function (3.14 or greater required)",
		POSTGIS_GEOS_VERSION);
	PG_RETURN_NULL();

#else /* POSTGIS_GEOS_VERSION >= 31200 */

	return coverage_window_calculation(fcinfo, COVERAGE_SIMPLIFY);

#endif
}


extern Datum ST_CoverageInvalidEdges(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_CoverageInvalidEdges);
Datum ST_CoverageInvalidEdges(PG_FUNCTION_ARGS)
{
#if POSTGIS_GEOS_VERSION < 31200

	lwpgerror("The GEOS version this PostGIS binary "
		"was compiled against (%d) doesn't support "
		"'GEOSCoverageIsValid' function (3.12 or greater required)",
		POSTGIS_GEOS_VERSION);
	PG_RETURN_NULL();

#else /* POSTGIS_GEOS_VERSION >= 31200 */

	return coverage_window_calculation(fcinfo, COVERAGE_ISVALID);

#endif
}

extern Datum ST_CoverageClean(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_CoverageClean);
Datum ST_CoverageClean(PG_FUNCTION_ARGS)
{
#if POSTGIS_GEOS_VERSION < 31400

	lwpgerror("The GEOS version this PostGIS binary "
		"was compiled against (%d) doesn't support "
		"'ST_CoverageClean' function (3.14 or greater required)",
		POSTGIS_GEOS_VERSION);
	PG_RETURN_NULL();

#else /* POSTGIS_GEOS_VERSION >= 31400 */

	return coverage_window_calculation(fcinfo, COVERAGE_CLEAN);

#endif
}

/**********************************************************************
 * ST_CoverageUnion(geometry[])
 *
 */

Datum ST_CoverageUnion(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_CoverageUnion);
Datum ST_CoverageUnion(PG_FUNCTION_ARGS)
{
	GSERIALIZED *result = NULL;

	Datum value;
	bool isnull;

	GEOSGeometry **geoms = NULL;
	GEOSGeometry *geos = NULL;
	GEOSGeometry *geos_result = NULL;
	uint32 ngeoms = 0;

	ArrayType *array = PG_GETARG_ARRAYTYPE_P(0);
	uint32 nelems = ArrayGetNItems(ARR_NDIM(array), ARR_DIMS(array));
	ArrayIterator iterator = array_create_iterator(array, 0, NULL);

	/* Return null on 0-elements input array */
	if (nelems == 0)
		PG_RETURN_NULL();

	/* Convert all geometries into GEOSGeometry array */
	geoms = palloc(sizeof(GEOSGeometry *) * nelems);

	initGEOS(lwpgnotice, lwgeom_geos_error);

	while (array_iterate(iterator, &value, &isnull))
	{
		GSERIALIZED *gser;
		/* Omit nulls */
		if (isnull) continue;

		/* Omit empty */
		gser = (GSERIALIZED *)DatumGetPointer(value);
		if (gserialized_is_empty(gser)) continue;

		/* Omit unconvertible */
		geos = POSTGIS2GEOS(gser);
		if (!geos) continue;

		geoms[ngeoms++] = geos;
	}
	array_free_iterator(iterator);

	if (ngeoms == 0)
		PG_RETURN_NULL();

	geos = GEOSGeom_createCollection(
		GEOS_GEOMETRYCOLLECTION,
		geoms, ngeoms);

	if (!geos)
	{
		coverage_destroy_geoms(geoms, ngeoms);
		HANDLE_GEOS_ERROR("Geometry could not be converted");
	}

	geos_result = GEOSCoverageUnion(geos);
	GEOSGeom_destroy(geos);
	if (!geos_result)
		HANDLE_GEOS_ERROR("Error computing coverage union");

	result = GEOS2POSTGIS(geos_result, LW_FALSE);
	GEOSGeom_destroy(geos_result);

	PG_RETURN_POINTER(result);
}


