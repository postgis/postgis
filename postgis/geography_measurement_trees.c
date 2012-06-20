#include "geography_measurement_trees.h"


/*
* Specific tree types include all the generic slots and 
* their own slots for their trees. We put the implementation
* for the CircTreeGeomCache here because we can't shove
* the PgSQL specific bits of the code (fcinfo) back into
* liblwgeom, where most of the circtree logic lives.
*/
typedef struct {
	int                         type;       // <GeomCache>
	GSERIALIZED*                geom1;      // 
	GSERIALIZED*                geom2;      // 
	size_t                      geom1_size; // 
	size_t                      geom2_size; // 
	int32                       argnum;     // </GeomCache>
	CIRC_NODE*                  index;
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
		circ_cache->argnum = 0;
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

static CircTreeGeomCache*
GetCircTreeGeomCache(FunctionCallInfoData* fcinfo, const GSERIALIZED* g1, const GSERIALIZED* g2)
{
	return (CircTreeGeomCache*)GetGeomCache(fcinfo, &CircTreeCacheMethods, g1, g2);
}

static int
CircTreePIP(const CircTreeGeomCache* tree_cache, const GSERIALIZED* g, const LWGEOM* lwgeom)
{
	int tree_type = gserialized_get_type(g);
	GBOX gbox;
	GEOGRAPHIC_POINT gp;
	POINT3D gp3;
	POINT4D pt;
	
	if ( tree_type == POLYGONTYPE || tree_type == MULTIPOLYGONTYPE )
	{
		/* Need a gbox to calculate an outside point */
		if ( LW_FAILURE == gserialized_get_gbox_p(g, &gbox) )
		{
			LWGEOM* lwgeom_cached = lwgeom_from_gserialized(g);
			lwgeom_calculate_gbox_geodetic(lwgeom_cached, &gbox);
			lwgeom_free(lwgeom_cached);
		}
		
		/* Need one point from the candidate geometry */
		if ( LW_FAILURE == lwgeom_startpoint(lwgeom, &pt) )
		{
			lwerror("CircTreePIP unable to generate start point for lwgeom %p", lwgeom);
			return LW_FALSE;
		}
	
		/* Flip the candidate point into geographics */
		geographic_point_init(pt.x, pt.y, &gp);
		geog2cart(&gp, &gp3);
		
		/* If the candidate isn't in the tree box, it's not in the tree area */
		if ( ! gbox_contains_point3d(&gbox, &gp3) )
		{
			return LW_FALSE;
		}
		/* The candidate point is in the box, so it *might* be inside the tree */
		else
		{
			POINT2D pt_outside; /* latlon */
			POINT2D pt_inside;
			pt_inside.x = pt.x; pt_inside.y = pt.y;
			/* Calculate a definitive outside point */
			gbox_pt_outside(&gbox, &pt_outside);
			/* Test the candidate point for strict containment */
			return circ_tree_contains_point(tree_cache->index, &pt_inside, &pt_outside, NULL);
		}
		
	}
	else
	{
		return LW_FALSE;
	}		
}

int
geography_distance_cache(FunctionCallInfoData* fcinfo, const GSERIALIZED* g1, const GSERIALIZED* g2, const SPHEROID* s, double* distance)
{
	CircTreeGeomCache* tree_cache = NULL;
	
	Assert(distance);
	
	/* Two points? Get outa here... */
	if ( gserialized_get_type(g1) == POINT && gserialized_get_type(g2) == POINT )
		return LW_FAILURE;

	/* Fetch/build our cache, if appropriate, etc... */
	tree_cache = GetCircTreeGeomCache(fcinfo, g1, g2);
	
	if ( tree_cache && tree_cache->argnum && tree_cache->index ) 
	{
		CIRC_NODE* circ_tree = NULL;
		const GSERIALIZED* g = NULL;
		LWGEOM* lwgeom = NULL;

		/* We need to dynamically build a tree for the uncached side of the function call */
		if ( tree_cache->argnum == 1 )
		{
			lwgeom = lwgeom_from_gserialized(g2);
			g = g1;
		}
		else if ( tree_cache->argnum == 2 )
		{
			lwgeom = lwgeom_from_gserialized(g1);
			g = g2;
		}
		else
			lwerror("geography_distance_cache failed! This will never happen!");

		if ( LW_TRUE == CircTreePIP(tree_cache, g, lwgeom) )
		{
			*distance = 0.0;
			lwgeom_free(lwgeom);
			return LW_SUCCESS;
		}
		
		/* We do tree/tree distance, so turn the candidate geometry into a tree */
		circ_tree = lwgeom_calculate_circ_tree(lwgeom);
		/* Calculate tree/tree distance */
		*distance = circ_tree_distance_tree(tree_cache->index, circ_tree, s, FP_TOLERANCE);
		circ_tree_free(circ_tree);
		lwgeom_free(lwgeom);	
		return LW_SUCCESS;
	}
	else
	{
		return LW_FAILURE;
	}
}

int
geography_dwithin_cache(FunctionCallInfoData* fcinfo, const GSERIALIZED* g1, const GSERIALIZED* g2, const SPHEROID* s, double tolerance, int* dwithin)
{
	CircTreeGeomCache* tree_cache = NULL;
	double distance;
		
	Assert(dwithin);
	
	/* Two points? Get outa here... */
	if ( gserialized_get_type(g1) == POINT && gserialized_get_type(g2) == POINT )
		return LW_FAILURE;
	
	/* Fetch/build our cache, if appropriate, etc... */
	tree_cache = GetCircTreeGeomCache(fcinfo, g1, g2);
	
	if ( tree_cache && tree_cache->argnum && tree_cache->index ) 
	{
		CIRC_NODE* circ_tree = NULL;
		const GSERIALIZED* g = NULL;
		LWGEOM* lwgeom = NULL;

		/* We need to dynamically build a tree for the uncached side of the function call */
		if ( tree_cache->argnum == 1 )
		{
			lwgeom = lwgeom_from_gserialized(g2);
			g = g1;
		}
		else if ( tree_cache->argnum == 2 )
		{
			lwgeom = lwgeom_from_gserialized(g1);
			g = g2;
		}
		else
			lwerror("geography_dwithin_cache failed! This will never happen!");

		if ( LW_TRUE == CircTreePIP(tree_cache, g, lwgeom) )
		{
			*dwithin = LW_TRUE;
			lwgeom_free(lwgeom);
			return LW_SUCCESS;
		}
		
		/* We do tree/tree distance, so turn the candidate geometry into a tree */
		circ_tree = lwgeom_calculate_circ_tree(lwgeom);
		/* Calculate tree/tree distance */
		distance = circ_tree_distance_tree(tree_cache->index, circ_tree, s, tolerance);
		*dwithin = (distance <= tolerance ? LW_TRUE : LW_FALSE);
		circ_tree_free(circ_tree);
		lwgeom_free(lwgeom);	
		return LW_SUCCESS;
	}
	else
	{
		return LW_FAILURE;
	}

}
