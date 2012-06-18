#include "geography_measurement_trees.h"

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

CircTreeGeomCache*
GetCircTreeGeomCache(FunctionCallInfoData* fcinfo, const GSERIALIZED* g1, const GSERIALIZED* g2)
{
	return (CircTreeGeomCache*)GetGeomCache(fcinfo, &CircTreeCacheMethods, g1, g2);
}

