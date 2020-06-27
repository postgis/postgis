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

extern Datum ST_ClusterDBSCAN(PG_FUNCTION_ARGS);
extern Datum ST_ClusterKMeans(PG_FUNCTION_ARGS);

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
} dbscan_cluster_result;

typedef struct
{
	char is_error;
	dbscan_cluster_result cluster_assignments[1];
} dbscan_context;

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

PG_FUNCTION_INFO_V1(ST_ClusterDBSCAN);
Datum ST_ClusterDBSCAN(PG_FUNCTION_ARGS)
{
	WindowObject win_obj = PG_WINDOW_OBJECT();
	uint32_t row = WinGetCurrentPosition(win_obj);
	uint32_t ngeoms = WinGetPartitionRowCount(win_obj);
	dbscan_context* context = WinGetPartitionLocalMemory(win_obj, sizeof(dbscan_context) + ngeoms * sizeof(dbscan_cluster_result));

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
			geoms[i] = read_lwgeom_from_partition(win_obj, i, (bool*)&(context->cluster_assignments[i].is_null));

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
				context->cluster_assignments[i].is_null = LW_TRUE;
			}
			else
			{
				context->cluster_assignments[i].cluster_id = result_ids[i];
			}
		}

		lwfree(result_ids);
		UF_destroy(uf);
	}

	if (context->cluster_assignments[row].is_null)
		PG_RETURN_NULL();

	PG_RETURN_INT32(context->cluster_assignments[row].cluster_id);
}

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
		LWGEOM    **geoms;
		int       *r;

		/* What is K? If it's NULL or invalid, we can't procede */
		k = DatumGetInt32(WinGetFuncArgCurrent(winobj, 1, &isnull));
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

		/* Error out if N < K */
		if (N<k)
		{
			lwpgerror("K (%d) must be smaller than the number of rows in the group (%d)", k, N);
		}

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
		r = lwgeom_cluster_kmeans((const LWGEOM **)geoms, N, k);

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
