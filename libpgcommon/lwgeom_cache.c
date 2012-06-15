/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 *
 * Copyright (C) 2012 Sandro Santilli <strk@keybit.net>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "postgres.h"
#include "fmgr.h"

#include "../postgis_config.h"
#include "lwgeom_cache.h"

/* 
* Generic statement caching infrastructure. We cache 
* the following kinds of objects:
* 
*   geometries-with-trees
*      PreparedGeometry, RTree, CIRC_TREE, RECT_TREE
*   srids-with-projections
*      projPJ
* 
* Each GenericCache* has a type, and after that
* some data. Similar to generic LWGEOM*. Test that
* the type number is what you expect before casting
* and de-referencing struct members.
*/
typedef struct {
	int type;
	char data[1];
} GenericCache;

/* 
* Although there are only two basic kinds of 
* cache entries, the actual trees stored in the
* geometries-with-trees pattern are quite diverse,
* and they might be used in combination, so we have 
* one slot for each tree type as well as a slot for
* projections.
*/
typedef struct {
	GenericCache* entry[NUM_CACHE_ENTRIES];
} GenericCacheCollection;

/**
* Utility function to read the upper memory context off a function call 
* info data.
*/
static MemoryContext 
FIContext(FunctionCallInfoData* fcinfo)
{
	return fcinfo->flinfo->fn_mcxt;
}

/**
* Get the generic collection off the statement, allocate a 
* new one if we don't have one already.
*/ 
static GenericCacheCollection* 
GetGenericCacheCollection(FunctionCallInfoData* fcinfo)
{
	GenericCacheCollection* cache = fcinfo->flinfo->fn_extra;

	if ( ! cache ) 
	{
		cache = MemoryContextAlloc(FIContext(fcinfo), sizeof(GenericCacheCollection));
		memset(cache, 0, sizeof(GenericCacheCollection));
		fcinfo->flinfo->fn_extra = cache;
	}
	return cache;		
}
	

/**
* Get the Proj4 entry from the generic cache if one exists.
* If it doesn't exist, make a new empty one and return it.
*/
PROJ4PortalCache*
GetPROJ4SRSCache(FunctionCallInfoData* fcinfo)
{
	GenericCacheCollection* generic_cache = GetGenericCacheCollection(fcinfo);
	PROJ4PortalCache* cache = (PROJ4PortalCache*)(generic_cache->entry[PROJ_CACHE_ENTRY]);
	
	if ( ! cache )
	{
		/* Allocate in the upper context */
		cache = MemoryContextAlloc(FIContext(fcinfo), sizeof(PROJ4PortalCache));

		if (cache)
		{
			int i;

			POSTGIS_DEBUGF(3, "Allocating PROJ4Cache for portal with transform() MemoryContext %p", FIContext(fcinfo));
			/* Put in any required defaults */
			for (i = 0; i < PROJ4_CACHE_ITEMS; i++)
			{
				cache->PROJ4SRSCache[i].srid = SRID_UNKNOWN;
				cache->PROJ4SRSCache[i].projection = NULL;
				cache->PROJ4SRSCache[i].projection_mcxt = NULL;
			}
			cache->type = PROJ_CACHE_ENTRY;
			cache->PROJ4SRSCacheCount = 0;
			cache->PROJ4SRSCacheContext = FIContext(fcinfo);

			/* Store the pointer in GenericCache */
			generic_cache->entry[PROJ_CACHE_ENTRY] = (GenericCache*)cache;
		}
	}
	return cache;
}

/**
* Get an appropriate (based on the entry type number) 
* GeomCache entry from the generic cache if one exists.
* If it doesn't exist, make a new empty one and return it.
*/
GeomCache*            
GetGeomCache(FunctionCallInfoData* fcinfo, int cache_entry)
{
	GeomCache* cache;
	GenericCacheCollection* generic_cache = GetGenericCacheCollection(fcinfo);
	
	if ( (cache_entry) < 0 || (cache_entry >= NUM_CACHE_ENTRIES) )
		return NULL;
	
	cache = (GeomCache*)(generic_cache->entry[cache_entry]);
	
	if ( ! cache )
	{
		/* Allocate in the upper context */
		cache = MemoryContextAlloc(FIContext(fcinfo), sizeof(GeomCache));
		/* Zero everything out */
		memset(cache, 0, sizeof(GeomCache));
		/* Set the cache type */
		cache->type = cache_entry;
		cache->context_statement = FIContext(fcinfo);

		/* Store the pointer in GenericCache */
		generic_cache->entry[cache_entry] = (GenericCache*)cache;
		
	}

	/* The cache object type should always equal the entry type */
	if ( cache->type != cache_entry )
	{
		lwerror("cache type does not equal expected entry type");
		return NULL;
	}
		
	return cache;
}





/**
* Get an appropriate tree from the cache, based on the entry number
* and the geometry values. Checks for a cache, checks for cache hits, 
* returns a built tree if one exists. 
*/
void* 
GetGeomIndex(FunctionCallInfoData* fcinfo, int cache_entry, GeomIndexBuilder index_build, GeomIndexFreer index_free, const GSERIALIZED* geom1, const GSERIALIZED* geom2, int* argnum)
{
	int cache_hit = 0;
	MemoryContext old_context;
	GeomCache* cache = GetGeomCache(fcinfo, cache_entry);
	const GSERIALIZED *geom;

	/* Initialize output of argnum */
	if ( argnum )
		*argnum = cache_hit;

	/* Cache hit on the first argument */
	if ( geom1 &&
	     cache->argnum != 2 &&
	     cache->geom1_size == VARSIZE(geom1) &&
	     memcmp(cache->geom1, geom1, cache->geom1_size) == 0 )
	{
		cache_hit = 1;
		geom = geom1;

	}
	/* Cache hit on second argument */
	else if ( geom2 &&
	          cache->argnum != 1 &&
	          cache->geom2_size == VARSIZE(geom2) &&
	          memcmp(cache->geom2, geom2, cache->geom2_size) == 0 )
	{
		cache_hit = 2;
		geom = geom2;
	}
	/* No cache hit. If we have a tree, free it. */
	else
	{
		cache_hit = 0;
		if ( cache->index )
		{
			index_free(cache);
			cache->index = NULL;
		}
	}

	/* Cache hit, but no tree built yet, build it! */
	if ( cache_hit && ! cache->index )
	{
		LWGEOM *lwgeom = lwgeom_from_gserialized(geom);
		int rv;

		/* Can't build a tree on a NULL or empty */
		if ( (!lwgeom) || lwgeom_is_empty(lwgeom) )
			return NULL;

		cache->argnum = cache_hit;
		old_context = MemoryContextSwitchTo(FIContext(fcinfo));
		rv = index_build(lwgeom, cache);
		MemoryContextSwitchTo(old_context);

		/* Something went awry in the tree build phase */
		if ( ! rv )
		{
			cache->argnum = 0;
			return NULL;
		}

	}

	/* We have a hit and a calculated tree, we're done */
	if ( cache_hit && (cache_hit == cache->argnum) && cache->index )
	{
		if ( argnum )
			*argnum = cache_hit;
		return cache->index;
	}

	/* Argument one didn't match, so copy the new value in. */
	if ( geom1 && cache_hit != 1 )
	{
		if ( cache->geom1 ) pfree(cache->geom1);
		cache->geom1_size = VARSIZE(geom1);
		cache->geom1 = MemoryContextAlloc(FIContext(fcinfo), cache->geom1_size);
		memcpy(cache->geom1, geom1, cache->geom1_size);
	}
	/* Argument two didn't match, so copy the new value in. */
	if ( geom2 && cache_hit != 2 )
	{
		if ( cache->geom2 ) pfree(cache->geom2);
		cache->geom2_size = VARSIZE(geom2);
		cache->geom2 = MemoryContextAlloc(FIContext(fcinfo), cache->geom2_size);
		memcpy(cache->geom2, geom2, cache->geom2_size);
	}

	return NULL;
}



/**
* Builder, freeer and public accessor for cached CIRC_NODE trees
*/
static int
CircTreeBuilder(const LWGEOM* lwgeom, GeomCache* cache)
{
	CIRC_NODE* tree = lwgeom_calculate_circ_tree(lwgeom);
	if ( cache->index )
	{
		circ_tree_free((CIRC_NODE*)(cache->index));
		cache->index = 0;
	}
	if ( ! tree )
		return LW_FAILURE;
	
	cache->index = (void*)tree;
	return LW_SUCCESS;
}

static int
CircTreeFreer(GeomCache* cache)
{
	CIRC_NODE* tree = (CIRC_NODE*)(cache->index);
	if ( tree ) 
	{
		circ_tree_free(tree);
		cache->index = 0;
	}
	return LW_SUCCESS;
}

CIRC_NODE* 
GetCircTree(FunctionCallInfoData* fcinfo, GSERIALIZED* g1, GSERIALIZED* g2, int* argnum_of_cache)
{
	int argnum = 0;
	CIRC_NODE* tree = NULL;

	tree = (CIRC_NODE*)GetGeomIndex(fcinfo, CIRC_CACHE_ENTRY, CircTreeBuilder, CircTreeFreer, g1, g2, &argnum);

	if ( argnum_of_cache )
		*argnum_of_cache = argnum;
		
	return tree;
}


