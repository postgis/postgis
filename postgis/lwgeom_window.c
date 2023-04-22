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
			lwpgerror("Tolerance must be a positive number", tolerance);
			PG_RETURN_NULL();
		}
		if (minpoints_is_null || minpoints < 0)
		{
			lwpgerror("Minpoints must be a positive number", minpoints);
		}

		initGEOS(lwnotice, lwgeom_geos_error);
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
			lwpgerror("Tolerance must be a positive number", tolerance);
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

		/* What is K? If it's NULL or invalid, we can't procede */
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


/********************************************************************************/


typedef struct {
	bool	isdone;
	bool	isnull;
	LWCOLLECTION *geom;
	int64   idx[0];
	/* variable length */
} coverage_context;


static coverage_context *
fetch_coverage_context(WindowObject winobj, int64 rowcount)
{
	size_t ctx_size = sizeof(coverage_context) + rowcount * sizeof(int64);
	coverage_context *ctx = (coverage_context*) WinGetPartitionLocalMemory(
		winobj, ctx_size);
	return ctx;
}

static void destroy_geoms(GEOSGeometry **geoms, uint32 ngeoms)
{
	if (!geoms) return;
	if (!ngeoms) return;
	for (uint32 i = 0; i < ngeoms; i++)
	{
		if(geoms[i])
			GEOSGeom_destroy(geoms[i]);
	}
}

static GEOSGeometry*
read_partition_into_collection (
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
		Datum d;

		d = WinGetFuncArgInPartition(winobj, 0, i,
				WINDOW_SEEK_HEAD, false, &isnull, &isout);

		/* Note NULL inputs and move on */
		if (isnull)
		{
			context->idx[i] = -1;
			continue;
		}

		gser = (GSERIALIZED*)PG_DETOAST_DATUM(d);
		gtype = gserialized_get_type(gser);

		/* Note non-polygonal inputs */
		if (gtype != MULTIPOLYGONTYPE && gtype != POLYGONTYPE)
		{
			context->idx[i] = -1;
			continue;
		}

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

	geos = GEOSGeom_createCollection(
		GEOS_GEOMETRYCOLLECTION,
		geoms, ngeoms);

	if (!geos)
	{
		destroy_geoms(geoms, ngeoms);
		return NULL;
	}

	pfree(geoms);
	return geos;
}


enum {
	COVERAGE_SIMPLIFY = 0,
	COVERAGE_ISVALID = 1
};

static Datum run_coverage_function(PG_FUNCTION_ARGS, int mode)
{
	WindowObject winobj = PG_WINDOW_OBJECT();
	int64 curpos = WinGetCurrentPosition(winobj);
	int64 rowcount = WinGetPartitionRowCount(winobj);
	coverage_context *context = fetch_coverage_context(winobj, rowcount);
	GSERIALIZED* result;
	MemoryContext uppercontext = fcinfo->flinfo->fn_mcxt;
	MemoryContext oldcontext;

	if (!context->isdone)
	{
		bool isnull;
		double tolerance = 0.0;
		uint32 preserveBoundary = 0;
		GEOSGeometry *simplified;
		GEOSGeometry *input;
		Datum d;

		if (!fcinfo->flinfo)
			elog(ERROR, "%s: Could not find upper context", __func__);

		if (rowcount == 0)
		{
			context->isdone = true;
			context->isnull = true;
			PG_RETURN_NULL();
		}

		/* Figure out the largest tolerance we are called with */
		d = WinGetFuncArgCurrent(winobj, 1, &isnull);
		if (!isnull)
			tolerance = DatumGetFloat8(d);

		/* If at any time we are asked to preserve boundary, do that */
		if (mode == COVERAGE_SIMPLIFY)
		{
			d = WinGetFuncArgCurrent(winobj, 2, &isnull);
			if (!isnull)
				preserveBoundary = DatumGetInt32(d);
		}

		initGEOS(lwnotice, lwgeom_geos_error);

		input = read_partition_into_collection(winobj, context);
		if (!input)
			HANDLE_GEOS_ERROR("Failed to create collection");

		if (mode == COVERAGE_SIMPLIFY)
		{
			simplified = GEOSCoverageSimplifyVW(input, tolerance, preserveBoundary);
		}
		else if (mode == COVERAGE_ISVALID)
		{
			GEOSCoverageIsValid(input, tolerance, &simplified);
		}
		else
		{
			elog(ERROR, "never get here");
		}

		GEOSGeom_destroy(input);

		if (!simplified)
		{
			HANDLE_GEOS_ERROR("Failed to simplify collection");
		}

		oldcontext = MemoryContextSwitchTo(uppercontext);
		context->geom = (LWCOLLECTION *) GEOS2LWGEOM(simplified, GEOSHasZ(simplified));
		MemoryContextSwitchTo(oldcontext);
		GEOSGeom_destroy(simplified);

		context->isdone = true;
	}

	/* Bomb out of any errors */
	if (context->isnull)
		PG_RETURN_NULL();

	/* Propogate the null entries */
	if (context->idx[curpos] < 0)
		PG_RETURN_NULL();

	oldcontext = MemoryContextSwitchTo(uppercontext);
	result = geometry_serialize(lwcollection_getsubgeom(context->geom, context->idx[curpos]));
	MemoryContextSwitchTo(oldcontext);

	/* When at the end of the partition, release the */
	/* simplified collection we have been reading. */
	if (curpos == rowcount - 1)
		lwcollection_free(context->geom);

	PG_RETURN_POINTER(result);
}

extern Datum ST_CoverageSimplify(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_CoverageSimplify);
Datum ST_CoverageSimplify(PG_FUNCTION_ARGS)
{
	PG_RETURN_DATUM(run_coverage_function(fcinfo, COVERAGE_SIMPLIFY));
}

extern Datum ST_CoverageIsValid(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_CoverageIsValid);
Datum ST_CoverageIsValid(PG_FUNCTION_ARGS)
{
	PG_RETURN_DATUM(run_coverage_function(fcinfo, COVERAGE_ISVALID));
}




#if 0
CREATE TABLE p (geom geometry, id integer);

INSERT INTO p VALUES (
'POLYGON(( 0 0,10 0,10.1 5,10 10,0 10,0 0))',
1
);
INSERT INTO p VALUES (
'POLYGON((10 0,20 0,20 10,10 10,10.1 5,10 0))',
1
);

INSERT INTO p VALUES (
'POLYGON(( 0 0,10 0,10.1 5,10 10,0 10,0 0))',
2
);
INSERT INTO p VALUES (
'POLYGON((10 0,20 0,20 10,10 10,10.1 5,10 0))',
2
);
INSERT INTO p VALUES (null, 2);

SELECT id, ST_AsText(GEOM), ST_AsText(ST_CoverageSimplify(geom, 1, 0) over (partition by id)) from p;
SELECT id, ST_AsText(GEOM), ST_AsText(ST_CoverageSimplify(geom, 1, 0) over ()) from p;

DROP TABLE q;
CREATE TABLE q (id integer, geom geometry);
INSERT INTO q VALUES (1, null);
INSERT INTO q VALUES (1, null);

INSERT INTO q VALUES (2,
'POLYGON(( 0 0,10 0,10.1 5,10 10,0 10,0 0))'
);
INSERT INTO q VALUES (2,
'POLYGON((10 0,20 0,20 10,10 10,10.1 5,10 0))'
);
INSERT INTO q VALUES (2,
'POLYGON EMPTY'
);
SELECT id, ST_AsText(GEOM), ST_AsText(ST_CoverageSimplify(geom, 1, 0) over ()) from q;


#endif


// extern GEOSGeometry GEOS_DLL *GEOSCoverageUnion(
//	 const GEOSGeometry *g);

// extern int GEOSCoverageIsValid(
//     const GEOSGeometry* input,
//     double gapWidth,
//     GEOSGeometry** invalidEdges);

// extern GEOSGeometry GEOS_DLL * GEOSCoverageSimplifyVW(
//     const GEOSGeometry* input,
//     double tolerance,
//     int preserveBoundary);


