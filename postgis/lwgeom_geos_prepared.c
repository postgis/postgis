/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 *
 * Copyright (C) 2008 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright (C) 2007 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "lwgeom_geos_prepared.h"

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

/* Memory context cache function prototypes */
static void PreparedCacheInit(MemoryContext context);
static void PreparedCacheReset(MemoryContext context);
static void PreparedCacheDelete(MemoryContext context);
static bool PreparedCacheIsEmpty(MemoryContext context);
static void PreparedCacheStats(MemoryContext context, int level);
#ifdef MEMORY_CONTEXT_CHECKING
static void PreparedCacheCheck(MemoryContext context);
#endif

/* Memory context definition must match the current version of PostgreSQL */
static MemoryContextMethods PreparedCacheContextMethods =
{
	NULL,
	NULL,
	NULL,
	PreparedCacheInit,
	PreparedCacheReset,
	PreparedCacheDelete,
	NULL,
	PreparedCacheIsEmpty,
	PreparedCacheStats
#ifdef MEMORY_CONTEXT_CHECKING
	, PreparedCacheCheck
#endif
};

static void
PreparedCacheInit(MemoryContext context)
{
	/*
	 * Do nothing as the cache is initialised when the transform()
	 * function is first called
	 */
}

static void
PreparedCacheDelete(MemoryContext context)
{
	PrepGeomHashEntry* pghe;

	/* Lookup the hash entry pointer in the global hash table so we can free it */
	pghe = GetPrepGeomHashEntry(context);

	if (!pghe)
		elog(ERROR, "PreparedCacheDelete: Trying to delete non-existant hash entry object with MemoryContext key (%p)", (void *)context);

	POSTGIS_DEBUGF(3, "deleting geom object (%p) and prepared geom object (%p) with MemoryContext key (%p)", pghe->geom, pghe->prepared_geom, context);

	/* Free them */
	if ( pghe->prepared_geom )
		GEOSPreparedGeom_destroy( pghe->prepared_geom );
	if ( pghe->geom )
		GEOSGeom_destroy( (GEOSGeometry *)pghe->geom );

	/* Remove the hash entry as it is no longer needed */
	DeletePrepGeomHashEntry(context);
}

static void
PreparedCacheReset(MemoryContext context)
{
	/*
	 * Do nothing, but we must supply a function since this call is mandatory according to tgl
	 * (see postgis-devel archives July 2007)
	 */
}

static bool
PreparedCacheIsEmpty(MemoryContext context)
{
	/*
	 * Always return false since this call is mandatory according to tgl
	 * (see postgis-devel archives July 2007)
	 */
	return FALSE;
}

static void
PreparedCacheStats(MemoryContext context, int level)
{
	/*
	 * Simple stats display function - we must supply a function since this call is mandatory according to tgl
	 * (see postgis-devel archives July 2007)
	 */

	fprintf(stderr, "%s: Prepared context\n", context->name);
}

#ifdef MEMORY_CONTEXT_CHECKING
static void
PreparedCacheCheck(MemoryContext context)
{
	/*
	 * Do nothing - stub required for when PostgreSQL is compiled
	 * with MEMORY_CONTEXT_CHECKING defined
	 */
}
#endif

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

	he->prepared_geom = NULL;
	he->geom = NULL;

	if (!he)
		elog(ERROR, "DeletePrepGeomHashEntry: There was an error removing the geometry object from this MemoryContext (%p)", (void *)mcxt);
}

/*
** GetPrepGeomCache
**
** Pull the current prepared geometry from the cache or make
** one if there is not one available. Only prepare geometry
** if we are seeing a key for the second time. That way rapidly
** cycling keys don't cause too much preparing.
*/
PrepGeomCache*
GetPrepGeomCache(FunctionCallInfoData *fcinfo, GSERIALIZED *pg_geom1, GSERIALIZED *pg_geom2)
{
	MemoryContext old_context;
	PrepGeomCache* cache = fcinfo->flinfo->fn_extra;
	int copy_keys = 1;
	size_t pg_geom1_size = 0;
	size_t pg_geom2_size = 0;

	/* Make sure this isn't someone else's cache object. */
	if ( cache && cache->type != 2 ) cache = NULL;

	if (!PrepGeomHash)
		CreatePrepGeomHash();

	if ( pg_geom1 )
		pg_geom1_size = VARSIZE(pg_geom1) + VARHDRSZ;

	if ( pg_geom2 )
		pg_geom2_size = VARSIZE(pg_geom2) + VARHDRSZ;

	if ( cache == NULL)
	{
		/*
		** Cache requested, but the cache isn't set up yet.
		** Set it up, but don't prepare the geometry yet.
		** That way if the next call is a cache miss we haven't
		** wasted time preparing a geometry we don't need.
		*/
		PrepGeomHashEntry pghe;

		old_context = MemoryContextSwitchTo(fcinfo->flinfo->fn_mcxt);
		cache = palloc(sizeof(PrepGeomCache));
		MemoryContextSwitchTo(old_context);

		cache->type = 2;
		cache->prepared_geom = 0;
		cache->geom = 0;
		cache->argnum = 0;
		cache->pg_geom1 = 0;
		cache->pg_geom2 = 0;
		cache->pg_geom1_size = 0;
		cache->pg_geom2_size = 0;
		cache->context = MemoryContextCreate(T_AllocSetContext, 8192,
		                                     &PreparedCacheContextMethods,
		                                     fcinfo->flinfo->fn_mcxt,
		                                     "PostGIS Prepared Geometry Context");

		POSTGIS_DEBUGF(3, "GetPrepGeomCache: creating cache: %p", cache);

		pghe.context = cache->context;
		pghe.geom = 0;
		pghe.prepared_geom = 0;
		AddPrepGeomHashEntry( pghe );

		fcinfo->flinfo->fn_extra = cache;

		POSTGIS_DEBUGF(3, "GetPrepGeomCache: adding context to hash: %p", cache);
	}
	else if ( pg_geom1 &&
	          cache->argnum != 2 &&
	          cache->pg_geom1_size == pg_geom1_size &&
	          memcmp(cache->pg_geom1, pg_geom1, pg_geom1_size) == 0)
	{
		if ( !cache->prepared_geom )
		{
			/*
			** Cache hit, but we haven't prepared our geometry yet.
			** Prepare it.
			*/
			PrepGeomHashEntry* pghe;

			cache->geom = POSTGIS2GEOS( pg_geom1 );
			cache->prepared_geom = GEOSPrepare( cache->geom );
			cache->argnum = 1;
			POSTGIS_DEBUG(3, "GetPrepGeomCache: preparing obj in argument 1");

			pghe = GetPrepGeomHashEntry(cache->context);
			pghe->geom = cache->geom;
			pghe->prepared_geom = cache->prepared_geom;
			POSTGIS_DEBUG(3, "GetPrepGeomCache: storing references to prepared obj in argument 1");
		}
		else
		{
			/*
			** Cache hit, and we're good to go. Do nothing.
			*/
			POSTGIS_DEBUG(3, "GetPrepGeomCache: cache hit, argument 1");
		}
		/* We don't need new keys until we have a cache miss */
		copy_keys = 0;
	}
	else if ( pg_geom2 &&
	          cache->argnum != 1 &&
	          cache->pg_geom2_size == pg_geom2_size &&
	          memcmp(cache->pg_geom2, pg_geom2, pg_geom2_size) == 0)
	{
		if ( !cache->prepared_geom )
		{
			/*
			** Cache hit on arg2, but we haven't prepared our geometry yet.
			** Prepare it.
			*/
			PrepGeomHashEntry* pghe;

			cache->geom = POSTGIS2GEOS( pg_geom2 );
			cache->prepared_geom = GEOSPrepare( cache->geom );
			cache->argnum = 2;
			POSTGIS_DEBUG(3, "GetPrepGeomCache: preparing obj in argument 2");

			pghe = GetPrepGeomHashEntry(cache->context);
			pghe->geom = cache->geom;
			pghe->prepared_geom = cache->prepared_geom;
			POSTGIS_DEBUG(3, "GetPrepGeomCache: storing references to prepared obj in argument 2");
		}
		else
		{
			/*
			** Cache hit, and we're good to go. Do nothing.
			*/
			POSTGIS_DEBUG(3, "GetPrepGeomCache: cache hit, argument 2");
		}
		/* We don't need new keys until we have a cache miss */
		copy_keys = 0;
	}
	else if ( cache->prepared_geom )
	{
		/*
		** No cache hits, so this must be a miss.
		** Destroy the GEOS objects, empty the cache.
		*/
		PrepGeomHashEntry* pghe;

		pghe = GetPrepGeomHashEntry(cache->context);
		pghe->geom = 0;
		pghe->prepared_geom = 0;

		POSTGIS_DEBUGF(3, "GetPrepGeomCache: cache miss, argument %d", cache->argnum);
		GEOSPreparedGeom_destroy( cache->prepared_geom );
		GEOSGeom_destroy( (GEOSGeometry *)cache->geom );

		cache->prepared_geom = 0;
		cache->geom = 0;
		cache->argnum = 0;

	}

	if ( copy_keys && pg_geom1 )
	{
		/*
		** If this is a new key (cache miss) we flip into the function
		** manager memory context and make a copy. We can't just store a pointer
		** because this copy will be pfree'd at the end of this function
		** call.
		*/
		POSTGIS_DEBUG(3, "GetPrepGeomCache: copying pg_geom1 into cache");
		old_context = MemoryContextSwitchTo(fcinfo->flinfo->fn_mcxt);
		if ( cache->pg_geom1 )
			pfree(cache->pg_geom1);
		cache->pg_geom1 = palloc(pg_geom1_size);
		MemoryContextSwitchTo(old_context);
		memcpy(cache->pg_geom1, pg_geom1, pg_geom1_size);
		cache->pg_geom1_size = pg_geom1_size;
	}
	if ( copy_keys && pg_geom2 )
	{
		POSTGIS_DEBUG(3, "GetPrepGeomCache: copying pg_geom2 into cache");
		old_context = MemoryContextSwitchTo(fcinfo->flinfo->fn_mcxt);
		if ( cache->pg_geom2 )
			pfree(cache->pg_geom2);
		cache->pg_geom2 = palloc(pg_geom2_size);
		MemoryContextSwitchTo(old_context);
		memcpy(cache->pg_geom2, pg_geom2, pg_geom2_size);
		cache->pg_geom2_size = pg_geom2_size;
	}

	return cache;

}

