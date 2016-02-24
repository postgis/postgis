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

Datum ST_ClusterDBSCAN(PG_FUNCTION_ARGS);

struct cluster_result
{
	uint32_t cluster_id;
	char is_null;        /* NULL may result from a NULL geometry input, or it may be used by 
							algorithms such as DBSCAN that do not assign all inputs to a
							cluster. */
};

typedef struct cluster_result cluster_result;

struct cluster_context
{
	char is_error;
	cluster_result cluster_assignments[1];
};


typedef struct cluster_context cluster_context;

static LWGEOM*
read_lwgeom_from_partition(WindowObject win_obj, uint32_t i, bool* is_null)
{
	GSERIALIZED* g;
	Datum arg = WinGetFuncArgInPartition(win_obj, 0, i, WINDOW_SEEK_HEAD, false, is_null, NULL);

	if (*is_null) {
		/* So that the indexes in our clustering input array can match our partition positions,
		 * 		 * toss an empty point into the clustering inputs, as a pass-through.
		 * 		 		 * NOTE: this will cause gaps in the output cluster id sequence.
		 * 		 		 		 * */
		return lwpoint_as_lwgeom(lwpoint_construct_empty(0, 0, 0));
	}

	g = (GSERIALIZED*) PG_DETOAST_DATUM(arg);
	return lwgeom_from_gserialized(g);
}

PG_FUNCTION_INFO_V1(ST_ClusterDBSCAN);
Datum ST_ClusterDBSCAN(PG_FUNCTION_ARGS)
{
	WindowObject win_obj = PG_WINDOW_OBJECT();
	uint32_t row = WinGetCurrentPosition(win_obj);
	uint32_t ngeoms = WinGetPartitionRowCount(win_obj);	
	cluster_context* context = WinGetPartitionLocalMemory(win_obj, sizeof(cluster_context) + ngeoms * sizeof(cluster_result));

	if (row == 0) /* beginning of the partition; do all of the work now */
	{
		uint32_t i;
		uint32_t* result_ids;
		LWGEOM** geoms;
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
			geoms[i] = read_lwgeom_from_partition(win_obj, i, &(context->cluster_assignments[i].is_null));

			if (!geoms[i]) {
				/* TODO release memory ? */
				lwpgerror("Error reading geometry.");
				PG_RETURN_NULL();
			}
		}

		if (union_dbscan(geoms, ngeoms, uf, tolerance, minpoints) == LW_SUCCESS)
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

		result_ids = UF_get_collapsed_cluster_ids(uf);
		for (i = 0; i < ngeoms; i++)
		{
			context->cluster_assignments[i].cluster_id = result_ids[i];
		}

		lwfree(result_ids);
		UF_destroy(uf);
	}

	if (context->cluster_assignments[row].is_null)
		PG_RETURN_NULL();

	PG_RETURN_INT32(context->cluster_assignments[row].cluster_id);
}
