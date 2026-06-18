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
 * ^copyright^
 * Copyright (C) 2026 Darafei Praliaskouski <me@komzpa.net>
 *
 **********************************************************************/

#include "geography_measurement_trees.h"


/*
* Specific tree types include all the generic slots and
* their own slots for their trees. We put the implementation
* for the CircTreeGeomCache here because we can't shove
* the PgSQL specific bits of the code (fcinfo) back into
* liblwgeom, where most of the circtree logic lives.
*/
typedef struct {
	GeomCache    gcache;
	CIRC_NODE*   index;
} CircTreeGeomCache;



/**
* Builder, freeer and public accessor for cached CIRC_NODE trees
*/
static int
CircTreeBuilder(const LWGEOM* lwgeom, GeomCache* cache)
{
	CircTreeGeomCache* circ_cache = (CircTreeGeomCache*)cache;
	CIRC_NODE* tree = lwgeom_calculate_circ_tree(lwgeom);

	if ( circ_cache->index )
	{
		circ_tree_free(circ_cache->index);
		circ_cache->index = 0;
	}
	if ( ! tree )
		return LW_FAILURE;

	circ_cache->index = tree;
	return LW_SUCCESS;
}

static int
CircTreeFreer(GeomCache* cache)
{
	CircTreeGeomCache* circ_cache = (CircTreeGeomCache*)cache;
	if ( circ_cache->index )
	{
		circ_tree_free(circ_cache->index);
		circ_cache->index = 0;
		circ_cache->gcache.argnum = 0;
	}
	return LW_SUCCESS;
}

static GeomCache*
CircTreeAllocator(void)
{
	CircTreeGeomCache* cache = palloc(sizeof(CircTreeGeomCache));
	memset(cache, 0, sizeof(CircTreeGeomCache));
	return (GeomCache*)cache;
}

static GeomCacheMethods CircTreeCacheMethods =
{
	CIRC_CACHE_ENTRY,
	CircTreeBuilder,
	CircTreeFreer,
	CircTreeAllocator
};

static double
circ_tree_distance_tree_edges(const CIRC_NODE *n1, const CIRC_NODE *n2, const SPHEROID *spheroid, double threshold)
{
	double min_dist = FLT_MAX;
	double max_dist = FLT_MAX;
	GEOGRAPHIC_POINT closest1, closest2;
	double threshold_radians = 0.95 * threshold / spheroid->radius;

	circ_tree_distance_tree_internal(n1, n2, threshold_radians, &min_dist, &max_dist, &closest1, &closest2);

	if (spheroid->a == spheroid->b)
		return spheroid->radius * sphere_distance(&closest1, &closest2);

	return spheroid_distance(&closest1, &closest2, spheroid);
}

static CircTreeGeomCache *
GetCircTreeGeomCache(FunctionCallInfo fcinfo, SHARED_GSERIALIZED *g1, SHARED_GSERIALIZED *g2)
{
	return (CircTreeGeomCache*)GetGeomCache(fcinfo, &CircTreeCacheMethods, g1, g2);
}

static int
lwgeom_covers_point2d(const LWGEOM *lwgeom, const POINT2D *pt)
{
	uint32_t i;
	const LWCOLLECTION *col;

	if (!lwgeom || lwgeom_is_empty(lwgeom))
		return LW_FALSE;

	if (lwgeom->type == POLYGONTYPE)
		return lwpoly_covers_point2d((const LWPOLY *)lwgeom, pt);

	if (!lwtype_is_collection(lwgeom->type))
		return LW_FALSE;

	col = (const LWCOLLECTION *)lwgeom;
	for (i = 0; i < col->ngeoms; i++)
	{
		if (lwgeom_covers_point2d(col->geoms[i], pt))
			return LW_TRUE;
	}

	return LW_FALSE;
}

static int
lwgeom_has_polygonal_component(const LWGEOM *lwgeom)
{
	uint32_t i;
	const LWCOLLECTION *col;

	if (!lwgeom || lwgeom_is_empty(lwgeom))
		return LW_FALSE;

	if (lwgeom->type == POLYGONTYPE)
		return LW_TRUE;

	if (!lwtype_is_collection(lwgeom->type))
		return LW_FALSE;

	col = (const LWCOLLECTION *)lwgeom;
	for (i = 0; i < col->ngeoms; i++)
	{
		if (lwgeom_has_polygonal_component(col->geoms[i]))
			return LW_TRUE;
	}

	return LW_FALSE;
}

static int
lwgeom_covers_any_startpoint2d(const LWGEOM *polygonal, const LWGEOM *points)
{
	uint32_t i;
	const LWCOLLECTION *col;
	POINT4D p4d;
	POINT2D pt2d;

	if (!points || lwgeom_is_empty(points))
		return LW_FALSE;

	if (lwtype_is_collection(points->type))
	{
		col = (const LWCOLLECTION *)points;
		for (i = 0; i < col->ngeoms; i++)
		{
			if (lwgeom_covers_any_startpoint2d(polygonal, col->geoms[i]))
				return LW_TRUE;
		}
		return LW_FALSE;
	}

	/*
	 * The distance tree can use only one representative point per primitive.
	 * For collections, recurse so a covered later member is not hidden behind
	 * an uncovered first member.
	 */
	if (lwgeom_startpoint(points, &p4d) != LW_SUCCESS)
		return LW_FALSE;

	pt2d.x = p4d.x;
	pt2d.y = p4d.y;
	return lwgeom_covers_point2d(polygonal, &pt2d);
}

static int
CircTreePIP(const CIRC_NODE *tree1, const GSERIALIZED *g1, const LWGEOM *lwgeom2)
{
	int tree1_type = gserialized_get_type(g1);
	LWGEOM *lwgeom1;
	int covers;

	POSTGIS_DEBUGF(3, "tree1_type=%d", tree1_type);

	/* If the tree'ed argument contains polygons, do the P-i-P using the geodetic predicate. */
	if (tree1_type == POLYGONTYPE || tree1_type == MULTIPOLYGONTYPE || lwtype_is_collection(tree1_type))
	{
		(void)tree1;
		POSTGIS_DEBUG(3, "tree contains polygons, using geodetic PiP");
		lwgeom1 = lwgeom_from_gserialized(g1);
		covers = lwgeom_has_polygonal_component(lwgeom1) && lwgeom_covers_any_startpoint2d(lwgeom1, lwgeom2);
		lwgeom_free(lwgeom1);
		return covers;
	}
	else
	{
		POSTGIS_DEBUG(3, "tree1 not polygonal, so CircTreePIP returning FALSE");
		return LW_FALSE;
	}
}

static int
geography_distance_cache_tolerance(FunctionCallInfo fcinfo,
				   SHARED_GSERIALIZED *shared_g1,
				   SHARED_GSERIALIZED *shared_g2,
				   const SPHEROID *s,
				   double tolerance,
				   double *distance)
{
	const GSERIALIZED *g1 = shared_gserialized_get(shared_g1);
	const GSERIALIZED *g2 = shared_gserialized_get(shared_g2);
	CircTreeGeomCache* tree_cache = NULL;

	int type1 = gserialized_get_type(g1);
	int type2 = gserialized_get_type(g2);

	Assert(distance);

	/* Two points? Get outa here... */
	if ( type1 == POINTTYPE && type2 == POINTTYPE )
		return LW_FAILURE;

	/* Fetch/build our cache, if appropriate, etc... */
	tree_cache = GetCircTreeGeomCache(fcinfo, shared_g1, shared_g2);

	/* OK, we have an index at the ready! Use it for the one tree argument and */
	/* fill in the other tree argument */
	if ( tree_cache && tree_cache->gcache.argnum && tree_cache->index )
	{
		CIRC_NODE* circtree_cached = tree_cache->index;
		CIRC_NODE* circtree = NULL;
		const GSERIALIZED* g_cached;
		const GSERIALIZED* g;
		LWGEOM* lwgeom = NULL;
		int geomtype_cached;
		int geomtype;
		LWGEOM *lwgeom_cached = NULL;

		/* We need to dynamically build a tree for the uncached side of the function call */
		if (tree_cache->gcache.argnum == 1)
		{
			g_cached = g1;
			g = g2;
			geomtype_cached = type1;
			geomtype = type2;
		}
		else if ( tree_cache->gcache.argnum == 2 )
		{
			g_cached = g2;
			g = g1;
			geomtype_cached = type2;
			geomtype = type1;
		}
		else
		{
			lwpgerror("geography_distance_cache this cannot happen!");
			return LW_FAILURE;
		}

		lwgeom = lwgeom_from_gserialized(g);
		if (geomtype_cached == POLYGONTYPE || geomtype_cached == MULTIPOLYGONTYPE ||
		    lwtype_is_collection(geomtype_cached))
		{
			if (CircTreePIP(circtree_cached, g_cached, lwgeom))
			{
				*distance = 0.0;
				lwgeom_free(lwgeom);
				return LW_SUCCESS;
			}
		}

		circtree = lwgeom_calculate_circ_tree(lwgeom);
		if (geomtype == POLYGONTYPE || geomtype == MULTIPOLYGONTYPE || lwtype_is_collection(geomtype))
		{
			lwgeom_cached = lwgeom_from_gserialized(g_cached);
			if (CircTreePIP(circtree, g, lwgeom_cached))
			{
				*distance = 0.0;
				lwgeom_free(lwgeom_cached);
				circ_tree_free(circtree);
				lwgeom_free(lwgeom);
				return LW_SUCCESS;
			}
			lwgeom_free(lwgeom_cached);
		}

		/*
		 * Public liblwgeom tree distance keeps a tree-only containment shortcut
		 * for external callers. Geography SQL reaches this point only after
		 * checking containment against the original polygonal geometry, so use
		 * edge distance here to avoid reintroducing tree-metadata false zeros.
		 */
		*distance = circ_tree_distance_tree_edges(circtree_cached, circtree, s, tolerance);
		circ_tree_free(circtree);
		lwgeom_free(lwgeom);
		return LW_SUCCESS;
	}
	else
	{
		return LW_FAILURE;
	}
}

int
geography_distance_cache(FunctionCallInfo fcinfo,
			 SHARED_GSERIALIZED *g1,
			 SHARED_GSERIALIZED *g2,
			 const SPHEROID *s,
			 double *distance)
{
	return geography_distance_cache_tolerance(fcinfo, g1, g2, s, FP_TOLERANCE, distance);
}

int
geography_dwithin_cache(FunctionCallInfo fcinfo,
			SHARED_GSERIALIZED *g1,
			SHARED_GSERIALIZED *g2,
			const SPHEROID *s,
			double tolerance,
			int *dwithin)
{
	double distance;
	/* Ticket #2422, difference between sphere and spheroid distance can trip up the */
	/* threshold shortcircuit (stopping a calculation before the spheroid distance is actually */
	/* below the threshold. Lower in the code line, we actually reduce the threshold a little to */
	/* avoid this. */
	/* Correct fix: propagate the spheroid information all the way to the bottom of the calculation */
	/* so the "right thing" can be done in all cases. */
	if ( LW_SUCCESS == geography_distance_cache_tolerance(fcinfo, g1, g2, s, tolerance, &distance) )
	{
		*dwithin = (distance <= (tolerance + FP_TOLERANCE) ? LW_TRUE : LW_FALSE);
		return LW_SUCCESS;
	}
	return LW_FAILURE;
}

int
geography_tree_distance(const GSERIALIZED* g1, const GSERIALIZED* g2, const SPHEROID* s, double tolerance, double* distance)
{
	CIRC_NODE* circ_tree1 = NULL;
	CIRC_NODE* circ_tree2 = NULL;
	LWGEOM* lwgeom1 = NULL;
	LWGEOM *lwgeom2 = NULL;
	lwgeom1 = lwgeom_from_gserialized(g1);
	lwgeom2 = lwgeom_from_gserialized(g2);
	circ_tree1 = lwgeom_calculate_circ_tree(lwgeom1);
	circ_tree2 = lwgeom_calculate_circ_tree(lwgeom2);

	if (lwgeom_has_polygonal_component(lwgeom1) && lwgeom_covers_any_startpoint2d(lwgeom1, lwgeom2))
	{
		*distance = 0.0;
	}
	else if (lwgeom_has_polygonal_component(lwgeom2) && lwgeom_covers_any_startpoint2d(lwgeom2, lwgeom1))
	{
		*distance = 0.0;
	}
	else
	{
		/*
		 * Containment against original geometries has already failed above; the
		 * remaining distance must come from tree edge/point pairs rather than the
		 * public tree-only containment shortcut.
		 */
		*distance = circ_tree_distance_tree_edges(circ_tree1, circ_tree2, s, tolerance);
	}

	circ_tree_free(circ_tree1);
	circ_tree_free(circ_tree2);
	lwgeom_free(lwgeom1);
	lwgeom_free(lwgeom2);
	return LW_SUCCESS;
}
