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
 * Copyright (C) 2012 Sandro Santilli <strk@kbt.io>
 * Copyright (C) 2008 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright (C) 2007 Refractions Research Inc.
 *
 **********************************************************************/


#include <assert.h>

#include "../postgis_config.h"
#include "lwgeom_geos_prepared.h"
#include "lwgeom_cache.h"

/***********************************************************************
**
**  PreparedGeometry implementations that cache intermediate indexed versions
**  of geometry in a special MemoryContext for re-used by future function
**  invocations.
**
**  By creating a memory context to hold the GEOS PreparedGeometry and Geometry
**  and making it a child of the fmgr memory context, we can get the memory held
**  by the GEOS objects released when the memory context delete callback is
**  invoked by the parent context.
**
**  Working parts:
**
**  PrepGeomCache, the actual struct that holds the keys we compare
**  to determine if our cache is stale, and references to the GEOS
**  objects used in computations.
**
**  PrepGeomHash, a global hash table that uses a MemoryContext as
**  key and returns a structure holding references to the GEOS
**  objects used in computations.
**
**  PreparedCacheContextMethods, a set of callback functions that
**  get hooked into a MemoryContext that is in turn used as a
**  key in the PrepGeomHash.
**
**  All this is to allow us to clean up external malloc'ed objects
**  (the GEOS Geometry and PreparedGeometry) before the structure
**  that references them (PrepGeomCache) is pfree'd by PgSQL. The
**  methods in the PreparedCacheContext are called just before the
**  function context is freed, allowing us to look up the references
**  in the PrepGeomHash and free them before the function context
**  is freed.
**
**/

/*
** Backend prepared hash table
**
** The memory context call-backs use a MemoryContext as the parameter
** so we need to map that over to actual references to GEOS objects to
** delete.
**
** This hash table stores a key/value pair of MemoryContext/Geom* objects.
*/
static HTAB* PrepGeomHash = NULL;

#define PREPARED_BACKEND_HASH_SIZE	32

typedef struct
{
	MemoryContext context;
	const GEOSPreparedGeometry* prepared_geom;
	const GEOSGeometry* geom;
}
PrepGeomHashEntry;

/* Memory context hash table function prototypes */
uint32 mcxt_ptr_hasha(const void *key, Size keysize);
static void CreatePrepGeomHash(void);
static void AddPrepGeomHashEntry(PrepGeomHashEntry pghe);
static PrepGeomHashEntry *GetPrepGeomHashEntry(MemoryContext mcxt);
static void DeletePrepGeomHashEntry(MemoryContext mcxt);


static void
PreparedCacheDelete(void *ptr)
{
	MemoryContext context = (MemoryContext)ptr;

	PrepGeomHashEntry* pghe;

	/* Lookup the hash entry pointer in the global hash table so we can free it */
	pghe = GetPrepGeomHashEntry(context);

	if (!pghe)
		elog(ERROR, "PreparedCacheDelete: Trying to delete non-existent hash entry object with MemoryContext key (%p)", (void *)context);

	POSTGIS_DEBUGF(3, "deleting geom object (%p) and prepared geom object (%p) with MemoryContext key (%p)", pghe->geom, pghe->prepared_geom, context);

	/* Free them */
	if ( pghe->prepared_geom )
		GEOSPreparedGeom_destroy( pghe->prepared_geom );
	if ( pghe->geom )
		GEOSGeom_destroy( (GEOSGeometry *)pghe->geom );

	/* Remove the hash entry as it is no longer needed */
	DeletePrepGeomHashEntry(context);
}


/* TODO: put this in common are for both transform and prepared
** mcxt_ptr_hash
** Build a key from a pointer and a size value.
*/
uint32
mcxt_ptr_hasha(const void *key, Size keysize)
{
	uint32 hashval;

	hashval = DatumGetUInt32(hash_any(key, keysize));

	return hashval;
}

static void
CreatePrepGeomHash(void)
{
	HASHCTL ctl;

	ctl.keysize = sizeof(MemoryContext);
	ctl.entrysize = sizeof(PrepGeomHashEntry);
	ctl.hash = mcxt_ptr_hasha;

	PrepGeomHash = hash_create("PostGIS Prepared Geometry Backend MemoryContext Hash", PREPARED_BACKEND_HASH_SIZE, &ctl, (HASH_ELEM | HASH_FUNCTION));
}

static void
AddPrepGeomHashEntry(PrepGeomHashEntry pghe)
{
	bool found;
	void **key;
	PrepGeomHashEntry *he;

	/* The hash key is the MemoryContext pointer */
	key = (void *)&(pghe.context);

	he = (PrepGeomHashEntry *) hash_search(PrepGeomHash, key, HASH_ENTER, &found);
	if (!found)
	{
		/* Insert the entry into the new hash element */
		he->context = pghe.context;
		he->geom = pghe.geom;
		he->prepared_geom = pghe.prepared_geom;
	}
	else
	{
		elog(ERROR, "AddPrepGeomHashEntry: This memory context is already in use! (%p)", (void *)pghe.context);
	}
}

static PrepGeomHashEntry*
GetPrepGeomHashEntry(MemoryContext mcxt)
{
	void **key;
	PrepGeomHashEntry *he;

	/* The hash key is the MemoryContext pointer */
	key = (void *)&mcxt;

	/* Return the projection object from the hash */
	he = (PrepGeomHashEntry *) hash_search(PrepGeomHash, key, HASH_FIND, NULL);

	return he;
}


static void
DeletePrepGeomHashEntry(MemoryContext mcxt)
{
	void **key;
	PrepGeomHashEntry *he;

	/* The hash key is the MemoryContext pointer */
	key = (void *)&mcxt;

	/* Delete the projection object from the hash */
	he = (PrepGeomHashEntry *) hash_search(PrepGeomHash, key, HASH_REMOVE, NULL);

	if (!he)
	{
		elog(ERROR, "DeletePrepGeomHashEntry: There was an error removing the geometry object from this MemoryContext (%p)", (void *)mcxt);
	}

	he->prepared_geom = NULL;
	he->geom = NULL;
}

/**
* Given a generic GeomCache, and a geometry to prepare,
* prepare a PrepGeomCache and stick it into the GeomCache->index
* slot. The PrepGeomCache includes the original GEOS geometry,
* and the GEOS prepared geometry, and a pointer to the
* MemoryContext where the callback functions are registered.
*
* This function is passed into the generic GetGeomCache function
* so that it can build an appropriate indexed structure in the case
* of a cache hit when there is no indexed structure yet
* available to return.
*/
static int
PrepGeomCacheBuilder(const LWGEOM *lwgeom, GeomCache *cache)
{
	PrepGeomCache* prepcache = (PrepGeomCache*)cache;
	PrepGeomHashEntry* pghe;

	/*
	* First time through? allocate the global hash.
	*/
	if (!PrepGeomHash)
		CreatePrepGeomHash();

	/*
	* No callback entry for this statement context yet? Set it up
	*/
	if ( ! prepcache->context_callback )
	{
		PrepGeomHashEntry pghe;
		MemoryContextCallback *callback;
		prepcache->context_callback = AllocSetContextCreate(prepcache->context_statement,
	                                   "PostGIS Prepared Geometry Context",
	                                   ALLOCSET_SMALL_SIZES);

		/* PgSQL comments suggest allocating callback in the context */
		/* being managed, so that the callback object gets cleaned along with */
		/* the context */
		callback = MemoryContextAlloc(prepcache->context_callback, sizeof(MemoryContextCallback));
		callback->arg = (void*)(prepcache->context_callback);
		callback->func = PreparedCacheDelete;
		MemoryContextRegisterResetCallback(prepcache->context_callback, callback);

		pghe.context = prepcache->context_callback;
		pghe.geom = 0;
		pghe.prepared_geom = 0;
		AddPrepGeomHashEntry( pghe );
	}

	/*
	* Hum, we shouldn't be asked to build a new cache on top of
	* an existing one. Error.
	*/
	if ( prepcache->gcache.argnum || prepcache->geom || prepcache->prepared_geom )
	{
		lwpgerror("PrepGeomCacheBuilder asked to build new prepcache where one already exists.");
		return LW_FAILURE;
    }

	prepcache->geom = LWGEOM2GEOS( lwgeom , 0);
	if ( ! prepcache->geom ) return LW_FAILURE;
	prepcache->prepared_geom = GEOSPrepare( prepcache->geom );
	if ( ! prepcache->prepared_geom ) return LW_FAILURE;
	prepcache->gcache.argnum = cache->argnum;

	/*
	* In order to find the objects we need to destroy, we keep
	* extra references in a global hash object.
	*/
	pghe = GetPrepGeomHashEntry(prepcache->context_callback);
	if ( ! pghe )
	{
		lwpgerror("PrepGeomCacheBuilder failed to find hash entry for context %p", prepcache->context_callback);
		return LW_FAILURE;
	}

	pghe->geom = prepcache->geom;
	pghe->prepared_geom = prepcache->prepared_geom;

	return LW_SUCCESS;
}

/**
* This function is passed into the generic GetGeomCache function
* in the case of a cache miss, so that it can free the particular
* indexed structure being managed.
*
* In the case of prepared geometry, we want to leave the actual
* PrepGeomCache allocated and in place, but ensure that the
* GEOS Geometry and PreparedGeometry are freed so we don't leak
* memory as we transition from cache hit to miss to hit, etc.
*/
static int
PrepGeomCacheCleaner(GeomCache *cache)
{
	PrepGeomHashEntry* pghe = 0;
	PrepGeomCache* prepcache = (PrepGeomCache*)cache;

	if ( ! prepcache )
		return LW_FAILURE;

	/*
	* Clear out the references to the soon-to-be-freed GEOS objects
	* from the callback hash entry
	*/
	pghe = GetPrepGeomHashEntry(prepcache->context_callback);
	if ( ! pghe )
	{
		lwpgerror("PrepGeomCacheCleaner failed to find hash entry for context %p", prepcache->context_callback);
		return LW_FAILURE;
	}
	pghe->geom = 0;
	pghe->prepared_geom = 0;

	/*
	* Free the GEOS objects and free the index tree
	*/
	POSTGIS_DEBUGF(3, "PrepGeomCacheFreeer: freeing %p argnum %d", prepcache, prepcache->gcache.argnum);
	GEOSPreparedGeom_destroy( prepcache->prepared_geom );
	GEOSGeom_destroy( (GEOSGeometry *)prepcache->geom );
	prepcache->gcache.argnum = 0;
	prepcache->prepared_geom = 0;
	prepcache->geom	= 0;

	return LW_SUCCESS;
}

static GeomCache*
PrepGeomCacheAllocator()
{
	PrepGeomCache* prepcache = palloc(sizeof(PrepGeomCache));
	memset(prepcache, 0, sizeof(PrepGeomCache));
	prepcache->context_statement = CurrentMemoryContext;
	prepcache->gcache.type = PREP_CACHE_ENTRY;
	return (GeomCache*)prepcache;
}

static GeomCacheMethods PrepGeomCacheMethods =
{
	PREP_CACHE_ENTRY,
	PrepGeomCacheBuilder,
	PrepGeomCacheCleaner,
	PrepGeomCacheAllocator
};


/**
* Given a couple potential geometries and a function
* call context, return a prepared structure for one
* of them, if such a structure doesn't already exist.
* If it doesn't exist, and there is a cache hit,
* ensure that the structure is built for next time.
* Most of the work is done by the GetGeomCache generic
* function, but we pass in call-backs to handle building
* and freeing the GEOS PreparedGeometry structures
* we need for this particular caching strategy.
*/
PrepGeomCache *
GetPrepGeomCache(FunctionCallInfo fcinfo, SHARED_GSERIALIZED *g1, SHARED_GSERIALIZED *g2)
{
	return (PrepGeomCache*)GetGeomCache(fcinfo, &PrepGeomCacheMethods, g1, g2);
}

