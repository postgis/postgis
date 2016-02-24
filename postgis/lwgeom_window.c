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
 *
 **********************************************************************/


#include "postgres.h"

#include "fmgr.h"
#include "funcapi.h"
#include "miscadmin.h"
#include "utils/builtins.h"
#include "windowapi.h"

#include "../postgis_config.h"

#include "liblwgeom.h"
#include "lwgeom_pg.h"

extern Datum ST_KMeans(PG_FUNCTION_ARGS);

typedef struct{
	bool	isdone;
	bool	isnull;
	int		result[1];
	/* variable length */
} kmeans_context;

PG_FUNCTION_INFO_V1(ST_KMeans);
Datum ST_KMeans(PG_FUNCTION_ARGS)
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
		r = lwgeom_cluster_2d_kmeans((const LWGEOM **)geoms, N, k);

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
		pfree(r);
		context->isdone = true;
	}

	if (context->isnull)
		PG_RETURN_NULL();

	curpos = WinGetCurrentPosition(winobj);
	PG_RETURN_INT32(context->result[curpos]);
}
