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
 * Copyright (C) 2024 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 **********************************************************************/

/* PostgreSQL */
#include "postgres.h"
#include "funcapi.h"
#include "fmgr.h"

/* Liblwgeom */
#include "liblwgeom.h"
#include "liblwgeom_internal.h"  /* For FP comparators. */
#include "intervaltree.h"

/* PostGIS */
#include "lwgeom_pg.h"
#include "lwgeom_cache.h"
#include "lwgeom_itree.h"


/* Prototypes */
Datum ST_IntersectsIntervalTree(PG_FUNCTION_ARGS);


/**********************************************************************
* IntervalTree Caching support
**********************************************************************/

/*
 * Specific cache types include all the generic GeomCache slots and
 * their own slots for their index trees. .
 */
typedef struct {
	GeomCache           gcache;
	IntervalTree        *itree;
} IntervalTreeGeomCache;

/**
* Builder, freeer and public accessor for cached IntervalTrees
*/
static int
IntervalTreeBuilder(const LWGEOM *lwgeom, GeomCache *geomcache)
{
	IntervalTreeGeomCache *cache = (IntervalTreeGeomCache*)geomcache;
	IntervalTree *itree = itree_from_lwgeom(lwgeom);

	if (cache->itree)
	{
		itree_free(cache->itree);
		cache->itree = 0;
	}
	if (!itree)
		return LW_FAILURE;

	cache->itree = itree;
	return LW_SUCCESS;
}

static int
IntervalTreeFreer(GeomCache *geomcache)
{
	IntervalTreeGeomCache *cache = (IntervalTreeGeomCache*)geomcache;
	if (cache->itree)
	{
		itree_free(cache->itree);
		cache->itree = 0;
		cache->gcache.argnum = 0;
	}
	return LW_SUCCESS;
}

static GeomCache *
IntervalTreeAllocator(void)
{
	IntervalTreeGeomCache *cache = lwalloc(sizeof(IntervalTreeGeomCache));
	memset(cache, 0, sizeof(IntervalTreeGeomCache));
	return (GeomCache*)cache;
}

static GeomCacheMethods IntervalTreeCacheMethods =
{
	ITREE_CACHE_ENTRY,
	IntervalTreeBuilder,
	IntervalTreeFreer,
	IntervalTreeAllocator
};

/*
 * Always return an IntervalTree, even if we don't have one
 * cached already so that the caller can use it to do a
 * index assisted p-i-p every time.
 */
IntervalTree *
GetIntervalTree(FunctionCallInfo fcinfo, SHARED_GSERIALIZED *g1)
{
	const GSERIALIZED *poly1;
	LWGEOM *lwpoly1;

	IntervalTree *itree = NULL;
	IntervalTreeGeomCache *cache = (IntervalTreeGeomCache*)GetGeomCache(fcinfo, &IntervalTreeCacheMethods, g1, NULL);

	/* Found a cached tree */
	if (cache && cache->itree)
		return cache->itree;

	/* Build one on the fly */
	poly1 = shared_gserialized_get(g1);
	lwpoly1 = lwgeom_from_gserialized(poly1);
	itree = itree_from_lwgeom(lwpoly1);
	lwgeom_free(lwpoly1);
	return itree;
}

/*
 * A point must be fully inside (not on boundary) of
 * a polygon to be contained. A multipoint must have
 * at least one fully contained member and no members
 * outside the polygon to be contained.
 */
bool itree_pip_contains(const IntervalTree *itree, const LWGEOM *lwpoints)
{
	if (lwgeom_get_type(lwpoints) == POINTTYPE)
	{
		return itree_point_in_multipolygon(itree, lwgeom_as_lwpoint(lwpoints)) == ITREE_INSIDE;
	}
	else if (lwgeom_get_type(lwpoints) == MULTIPOINTTYPE)
	{
		bool found_completely_inside = false;
		LWMPOINT *mpoint = lwgeom_as_lwmpoint(lwpoints);
		for (uint32_t i = 0; i < mpoint->ngeoms; i++)
		{
			IntervalTreeResult pip_result;
			const LWPOINT* pt = mpoint->geoms[i];

			if (lwpoint_is_empty(pt))
				continue;
			/*
			 * We need to find at least one point that's completely inside the
			 * polygons (pip_result == 1).  As long as we have one point that's
			 * completely inside, we can have as many as we want on the boundary
			 * itself. (pip_result == 0)
			 */
			pip_result = itree_point_in_multipolygon(itree, pt);

			if (pip_result == ITREE_INSIDE)
				found_completely_inside = true;

			if (pip_result == ITREE_OUTSIDE)
				return false;
		}
		return found_completely_inside;
	}
	else
	{
		elog(ERROR, "%s got a non-point input", __func__);
		return false;
	}
}


/*
 * If any point in the point/multipoint is outside
 * the polygon, then the polygon does not cover the point/multipoint.
 */
bool itree_pip_covers(const IntervalTree *itree, const LWGEOM *lwpoints)
{
	if (lwgeom_get_type(lwpoints) == POINTTYPE)
	{
		return itree_point_in_multipolygon(itree, lwgeom_as_lwpoint(lwpoints)) != ITREE_OUTSIDE;
	}
	else if (lwgeom_get_type(lwpoints) == MULTIPOINTTYPE)
	{
		LWMPOINT* mpoint = lwgeom_as_lwmpoint(lwpoints);
		for (uint32_t i = 0; i < mpoint->ngeoms; i++)
		{
			const LWPOINT *pt = mpoint->geoms[i];

			if (lwpoint_is_empty(pt))
				continue;

			if (itree_point_in_multipolygon(itree, pt) == ITREE_OUTSIDE)
				return false;
		}
		return true;
	}
	else
	{
		elog(ERROR, "%s got a non-point input", __func__);
		return false;
	}
}

/*
 * A.intersects(B) implies if any member of the point/multipoint
 * is not outside, then they intersect.
 */
bool itree_pip_intersects(const IntervalTree *itree, const LWGEOM *lwpoints)
{
	if (lwgeom_get_type(lwpoints) == POINTTYPE)
	{
		return itree_point_in_multipolygon(itree, lwgeom_as_lwpoint(lwpoints)) != ITREE_OUTSIDE;
	}
	else if (lwgeom_get_type(lwpoints) == MULTIPOINTTYPE)
	{
		LWMPOINT* mpoint = lwgeom_as_lwmpoint(lwpoints);
		for (uint32_t i = 0; i < mpoint->ngeoms; i++)
		{
			const LWPOINT *pt = mpoint->geoms[i];

			if (lwpoint_is_empty(pt))
				continue;

			if (itree_point_in_multipolygon(itree, pt) != ITREE_OUTSIDE)
				return true;
		}
		return false;
	}
	else
	{
		elog(ERROR, "%s got a non-point input", __func__);
		return false;
	}
}


/**********************************************************************
* ST_IntersectsIntervalTree
**********************************************************************/

PG_FUNCTION_INFO_V1(ST_IntersectsIntervalTree);
Datum ST_IntersectsIntervalTree(PG_FUNCTION_ARGS)
{
	GSERIALIZED *g1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *g2 = PG_GETARG_GSERIALIZED_P(1);
	LWGEOM *lwg1, *lwg2;
	LWPOINT *lwpt;
	IntervalTree *itree = NULL;
	bool isPoly1, isPoly2, isPt1, isPt2;

	/* Return false on empty arguments. */
	if (gserialized_is_empty(g1) || gserialized_is_empty(g2))
	{
		PG_FREE_IF_COPY(g1, 0);
		PG_FREE_IF_COPY(g2, 1);
		PG_RETURN_BOOL(false);
	}

	lwg1 = lwgeom_from_gserialized(g1);
	lwg2 = lwgeom_from_gserialized(g2);

	isPoly1 = lwg1->type == POLYGONTYPE || lwg1->type == MULTIPOLYGONTYPE;
	isPoly2 = lwg2->type == POLYGONTYPE || lwg2->type == MULTIPOLYGONTYPE;
	isPt1 = lwg1->type == POINTTYPE;
	isPt2 = lwg2->type == POINTTYPE;

	/* Two points? Get outa here... */
	if (isPoly1 && isPt2)
	{
		itree = itree_from_lwgeom(lwg1);
		lwpt = lwgeom_as_lwpoint(lwg2);
	}
	else if (isPoly2 && isPt1)
	{
		itree = itree_from_lwgeom(lwg2);
		lwpt = lwgeom_as_lwpoint(lwg1);
	}

	if (!itree)
		elog(ERROR, "arguments to %s must a point and a polygon", __func__);

	PG_RETURN_BOOL(itree_point_in_multipolygon(itree, lwpt) != ITREE_OUTSIDE);
}
