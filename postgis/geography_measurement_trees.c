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
CircTreePIP(const CIRC_NODE* tree1, const GSERIALIZED* g1, const LWGEOM* lwgeom2)
{
	int tree1_type = gserialized_get_type(g1);
	GBOX gbox1;
	GEOGRAPHIC_POINT in_gpoint;
	POINT3D in_point3d;
	POINT4D in_point;

	POSTGIS_DEBUGF(3, "tree1_type=%d, lwgeom2->type=%d", tree1_type, lwgeom2->type);

	/* If the tree'ed argument is a polygon, do the P-i-P using the tree-based P-i-P */
	if ( tree1_type == POLYGONTYPE || tree1_type == MULTIPOLYGONTYPE )
	{
		POSTGIS_DEBUG(3, "tree is a polygon, using tree PiP");
		/* Need a gbox to calculate an outside point */
		if ( LW_FAILURE == gserialized_get_gbox_p(g1, &gbox1) )
		{
			LWGEOM* lwgeom1 = lwgeom_from_gserialized(g1);
			POSTGIS_DEBUG(3, "unable to read gbox from gserialized, calculating from scratch");
			lwgeom_calculate_gbox_geodetic(lwgeom1, &gbox1);
			lwgeom_free(lwgeom1);
		}
		
		/* Need one point from the candidate geometry */
		if ( LW_FAILURE == lwgeom_startpoint(lwgeom2, &in_point) )
		{
			lwerror("CircTreePIP unable to generate start point for lwgeom %p", lwgeom2);
			POSTGIS_DEBUG(3, "unable to generate in_point, CircTreePIP returning FALSE");
			return LW_FALSE;
		}
	
		/* Flip the candidate point into geographics */
		geographic_point_init(in_point.x, in_point.y, &in_gpoint);
		geog2cart(&in_gpoint, &in_point3d);
		
		/* If the candidate isn't in the tree box, it's not in the tree area */
		if ( ! gbox_contains_point3d(&gbox1, &in_point3d) )
		{
			POSTGIS_DEBUG(3, "in_point3d is not inside the tree gbox, CircTreePIP returning FALSE");
			return LW_FALSE;
		}
		/* The candidate point is in the box, so it *might* be inside the tree */
		else
		{
			POINT2D pt2d_outside; /* latlon */
			POINT2D pt2d_inside;
			pt2d_inside.x = in_point.x; 
			pt2d_inside.y = in_point.y;
			/* Calculate a definitive outside point */
			gbox_pt_outside(&gbox1, &pt2d_outside);
			POSTGIS_DEBUGF(3, "p2d_inside=POINT(%g %g) p2d_outside=POINT(%g %g)", pt2d_inside.x, pt2d_inside.y, pt2d_outside.x, pt2d_outside.y);
			/* Test the candidate point for strict containment */
			POSTGIS_DEBUG(3, "calling circ_tree_contains_point for PiP test");
			return circ_tree_contains_point(tree1, &pt2d_inside, &pt2d_outside, NULL);
		}
	}
	/* If the un-tree'd argument is a polygon and the tree'd argument isn't, we need to do a */
	/* standard P-i-P on the un-tree'd side. */
	else if ( lwgeom2->type == POLYGONTYPE || lwgeom2->type == MULTIPOLYGONTYPE )
	{
		int result;
		LWGEOM* lwgeom1 = lwgeom_from_gserialized(g1);
		LWGEOM* lwpoint;
		POINT4D p4d;
		POSTGIS_DEBUG(3, "tree1 not polygonal, but lwgeom2 is, calculating using lwgeom_covers_lwgeom_sphere");
		
		if ( LW_FAILURE == lwgeom_startpoint(lwgeom1, &p4d) )
		{
			lwgeom_free(lwgeom1);
			lwerror("CircTreePIP unable to get lwgeom_startpoint");
			return LW_FALSE;
		}  
		lwpoint = lwpoint_as_lwgeom(lwpoint_make(lwgeom_get_srid(lwgeom1), lwgeom_has_z(lwgeom1), lwgeom_has_m(lwgeom1), &p4d));
		result = lwgeom_covers_lwgeom_sphere(lwgeom2, lwpoint);
		lwgeom_free(lwgeom1);
		lwgeom_free(lwpoint);
		return result;
	}
	else
	{
		POSTGIS_DEBUG(3, "neither tree1 nor lwgeom2 polygonal, so CircTreePIP returning FALSE");
		return LW_FALSE;
	}		
}

int
geography_distance_cache(FunctionCallInfoData* fcinfo, const GSERIALIZED* g1, const GSERIALIZED* g2, const SPHEROID* s, double* distance)
{
	CircTreeGeomCache* tree_cache = NULL;
	
	Assert(distance);
	
	/* Two points? Get outa here... */
	if ( (gserialized_get_type(g1) == POINTTYPE) && (gserialized_get_type(g2) == POINTTYPE) )
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

		if ( LW_TRUE == CircTreePIP(tree_cache->index, g, lwgeom) )
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
	if ( (gserialized_get_type(g1) == POINTTYPE) && (gserialized_get_type(g2) == POINTTYPE) )
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

		if ( LW_TRUE == CircTreePIP(tree_cache->index, g, lwgeom) )
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


int
geography_tree_distance(const GSERIALIZED* g1, const GSERIALIZED* g2, const SPHEROID* s, double tolerance, double* distance)
{
	CIRC_NODE* circ_tree1 = NULL;
	CIRC_NODE* circ_tree2 = NULL;
	LWGEOM* lwgeom1 = NULL;
	LWGEOM* lwgeom2 = NULL;
	
	lwgeom1 = lwgeom_from_gserialized(g1);
	lwgeom2 = lwgeom_from_gserialized(g2);
	circ_tree1 = lwgeom_calculate_circ_tree(lwgeom1);
	circ_tree2 = lwgeom_calculate_circ_tree(lwgeom2);
	
	if ( CircTreePIP(circ_tree1, g1, lwgeom2) || CircTreePIP(circ_tree2, g2, lwgeom1) )
	{
		*distance = 0.0;
	}
	else 
	{
		/* Calculate tree/tree distance */
		*distance = circ_tree_distance_tree(circ_tree1, circ_tree2, s, tolerance);
	}
	
	circ_tree_free(circ_tree1);
	circ_tree_free(circ_tree2);
	lwgeom_free(lwgeom1);
	lwgeom_free(lwgeom2);
	return LW_SUCCESS;
}
