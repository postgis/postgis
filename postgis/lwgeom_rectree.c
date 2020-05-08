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
 * Copyright (C) 2018 Paul Ramsey
 *
 **********************************************************************/


#include "postgres.h"
#include "funcapi.h"
#include "fmgr.h"
#include "liblwgeom.h"
#include "liblwgeom_internal.h"  /* For FP comparators. */
#include "lwgeom_pg.h"
#include "lwtree.h"
#include "lwgeom_cache.h"


/* Prototypes */
Datum ST_DistanceRectTree(PG_FUNCTION_ARGS);
Datum ST_DistanceRectTreeCached(PG_FUNCTION_ARGS);


/**********************************************************************
* RectTree Caching support
**********************************************************************/

/*
* Specific tree types include all the generic slots and
* their own slots for their trees. We put the implementation
* for the RectTreeGeomCache here because we can't shove
* the PgSQL specific bits of the code (fcinfo) back into
* liblwgeom/lwtree.c, where most of the rect_tree logic lives.
*/
typedef struct {
	GeomCache           gcache;
	RECT_NODE           *index;
} RectTreeGeomCache;


/**
* Builder, freeer and public accessor for cached RECT_NODE trees
*/
static int
RectTreeBuilder(const LWGEOM *lwgeom, GeomCache *cache)
{
	RectTreeGeomCache *rect_cache = (RectTreeGeomCache*)cache;
	RECT_NODE *tree = rect_tree_from_lwgeom(lwgeom);

	if ( rect_cache->index )
	{
		rect_tree_free(rect_cache->index);
		rect_cache->index = 0;
	}
	if ( ! tree )
		return LW_FAILURE;

	rect_cache->index = tree;
	return LW_SUCCESS;
}

static int
RectTreeFreer(GeomCache *cache)
{
	RectTreeGeomCache *rect_cache = (RectTreeGeomCache*)cache;
	if ( rect_cache->index )
	{
		rect_tree_free(rect_cache->index);
		rect_cache->index = 0;
		rect_cache->gcache.argnum = 0;
	}
	return LW_SUCCESS;
}

static GeomCache *
RectTreeAllocator(void)
{
	RectTreeGeomCache *cache = palloc(sizeof(RectTreeGeomCache));
	memset(cache, 0, sizeof(RectTreeGeomCache));
	return (GeomCache*)cache;
}

static GeomCacheMethods RectTreeCacheMethods =
{
	RECT_CACHE_ENTRY,
	RectTreeBuilder,
	RectTreeFreer,
	RectTreeAllocator
};

static RectTreeGeomCache *
GetRectTreeGeomCache(FunctionCallInfo fcinfo, SHARED_GSERIALIZED *g1, SHARED_GSERIALIZED *g2)
{
	return (RectTreeGeomCache*)GetGeomCache(fcinfo, &RectTreeCacheMethods, g1, g2);
}


/**********************************************************************
* ST_DistanceRectTree
**********************************************************************/

PG_FUNCTION_INFO_V1(ST_DistanceRectTree);
Datum ST_DistanceRectTree(PG_FUNCTION_ARGS)
{
	GSERIALIZED *g1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *g2 = PG_GETARG_GSERIALIZED_P(1);
	LWGEOM *lwg1, *lwg2;
	RECT_NODE *n1, *n2;

	/* Return NULL on empty arguments. */
	if (gserialized_is_empty(g1) || gserialized_is_empty(g2))
	{
		PG_FREE_IF_COPY(g1, 0);
		PG_FREE_IF_COPY(g2, 1);
		PG_RETURN_NULL();
	}

	lwg1 = lwgeom_from_gserialized(g1);
	lwg2 = lwgeom_from_gserialized(g2);

	/* Two points? Get outa here... */
	if (lwg1->type == POINTTYPE && lwg2->type == POINTTYPE)
		PG_RETURN_FLOAT8(lwgeom_mindistance2d(lwg1, lwg2));


	n1 = rect_tree_from_lwgeom(lwg1);
	n2 = rect_tree_from_lwgeom(lwg2);
	PG_RETURN_FLOAT8(rect_tree_distance_tree(n1, n2, 0.0));
}

PG_FUNCTION_INFO_V1(ST_DistanceRectTreeCached);
Datum ST_DistanceRectTreeCached(PG_FUNCTION_ARGS)
{
	RectTreeGeomCache *tree_cache = NULL;
	SHARED_GSERIALIZED *shared_geom1 = ToastCacheGetGeometry(fcinfo, 0);
	SHARED_GSERIALIZED *shared_geom2 = ToastCacheGetGeometry(fcinfo, 1);
	const GSERIALIZED *g1 = shared_gserialized_get(shared_geom1);
	const GSERIALIZED *g2 = shared_gserialized_get(shared_geom2);

	/* Return NULL on empty arguments. */
	if (gserialized_is_empty(g1) || gserialized_is_empty(g2))
	{
		PG_RETURN_NULL();
	}

	/* Two points? Get outa here... */
	if (gserialized_get_type(g1) == POINTTYPE && gserialized_get_type(g2) == POINTTYPE)
	{
		LWGEOM *lwg1 = lwgeom_from_gserialized(g1);
		LWGEOM *lwg2 = lwgeom_from_gserialized(g2);
		PG_RETURN_FLOAT8(lwgeom_mindistance2d(lwg1, lwg2));
	}

	/* Fetch/build our cache, if appropriate, etc... */
	tree_cache = GetRectTreeGeomCache(fcinfo, shared_geom1, shared_geom2);

	if (tree_cache && tree_cache->gcache.argnum)
	{
		RECT_NODE *n;
		RECT_NODE *n_cached = tree_cache->index;;
		if (tree_cache->gcache.argnum == 1)
		{
			LWGEOM *lwg2 = lwgeom_from_gserialized(g2);
			n = rect_tree_from_lwgeom(lwg2);
		}
		else if (tree_cache->gcache.argnum == 2)
		{
			LWGEOM *lwg1 = lwgeom_from_gserialized(g1);
			n = rect_tree_from_lwgeom(lwg1);
		}
		else
		{
			elog(ERROR, "reached unreachable block in %s", __func__);
		}
		PG_RETURN_FLOAT8(rect_tree_distance_tree(n, n_cached, 0.0));
	}
	else
	{
		LWGEOM *lwg1 = lwgeom_from_gserialized(g1);
		LWGEOM *lwg2 = lwgeom_from_gserialized(g2);
		PG_RETURN_FLOAT8(lwgeom_mindistance2d(lwg1, lwg2));
	}

	PG_RETURN_NULL();
}
