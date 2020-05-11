/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright (C) 2012 Sandro Santilli <strk@kbt.io>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "postgres.h"

#include "catalog/pg_type.h" /* for CSTRINGOID */
#include "executor/spi.h"
#include "fmgr.h"
#include "utils/memutils.h"

#include "../postgis_config.h"

/* Include for VARATT_EXTERNAL_GET_POINTER */
#if POSTGIS_PGSQL_VERSION < 130
#include "access/tuptoaster.h"
#else
#include "access/detoast.h"
#endif

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
MemoryContext
PostgisCacheContext(FunctionCallInfo fcinfo)
{
	if (!fcinfo->flinfo)
		elog(ERROR, "%s: Could not find upper context", __func__);
	return fcinfo->flinfo->fn_mcxt;
}

/**
* Get the generic collection off the statement, allocate a
* new one if we don't have one already.
*/
static GenericCacheCollection *
GetGenericCacheCollection(FunctionCallInfo fcinfo)
{
	GenericCacheCollection *internal_cache;
	if (!fcinfo->flinfo)
		elog(ERROR, "%s: Could not find upper context", __func__);

	internal_cache = fcinfo->flinfo->fn_extra;

	if (!internal_cache)
	{
		internal_cache = MemoryContextAllocZero(PostgisCacheContext(fcinfo), sizeof(GenericCacheCollection));
		fcinfo->flinfo->fn_extra = internal_cache;
	}
	return internal_cache;
}


/**
* Get the Proj entry from the generic cache if one exists.
* If it doesn't exist, make a new empty one and return it.
*/
PROJPortalCache *
GetPROJSRSCache(FunctionCallInfo fcinfo)
{
	GenericCacheCollection* generic_cache = GetGenericCacheCollection(fcinfo);
	PROJPortalCache* cache = (PROJPortalCache*)(generic_cache->entry[PROJ_CACHE_ENTRY]);

	if ( ! cache )
	{
		/* Allocate in the upper context */
		cache = MemoryContextAlloc(PostgisCacheContext(fcinfo), sizeof(PROJPortalCache));

		if (cache)
		{
			POSTGIS_DEBUGF(3,
				       "Allocating PROJCache for portal with transform() MemoryContext %p",
				       PostgisCacheContext(fcinfo));
			memset(cache->PROJSRSCache, 0, sizeof(PROJSRSCacheItem) * PROJ_CACHE_ITEMS);
			cache->type = PROJ_CACHE_ENTRY;
			cache->PROJSRSCacheCount = 0;

			/* Store the pointer in GenericCache */
			generic_cache->entry[PROJ_CACHE_ENTRY] = (GenericCache*)cache;
		}
	}
	return cache;
}

/**
* Get an appropriate (based on the entry type number)
* GeomCache entry from the generic cache if one exists.
* Returns a cache pointer if there is a cache hit and we have an
* index built and ready to use. Returns NULL otherwise.
*/
GeomCache *
GetGeomCache(FunctionCallInfo fcinfo,
	     const GeomCacheMethods *cache_methods,
	     SHARED_GSERIALIZED *g1,
	     SHARED_GSERIALIZED *g2)
{
	GeomCache* cache;
	int cache_hit = 0;
	MemoryContext old_context;
	const GSERIALIZED *geom;
	GenericCacheCollection* generic_cache = GetGenericCacheCollection(fcinfo);
	uint32_t entry_number = cache_methods->entry_number;

	Assert(entry_number < NUM_CACHE_ENTRIES);

	cache = (GeomCache*)(generic_cache->entry[entry_number]);
	if ( ! cache )
	{
		old_context = MemoryContextSwitchTo(PostgisCacheContext(fcinfo));
		/* Allocate in the upper context */
		cache = cache_methods->GeomCacheAllocator();
		MemoryContextSwitchTo(old_context);
		/* Store the pointer in GenericCache */
		cache->type = entry_number;
		generic_cache->entry[entry_number] = (GenericCache*)cache;
	}

	/* Cache hit on the first argument */
	if (g1 && cache->geom1 && cache->argnum != 2 && shared_gserialized_equal(g1, cache->geom1))
	{
		cache_hit = 1;
		geom = shared_gserialized_get(cache->geom1);
	}
	/* Cache hit on second argument */
	else if (g2 && cache->geom2 && cache->argnum != 1 && shared_gserialized_equal(g2, cache->geom2))
	{
		cache_hit = 2;
		geom = shared_gserialized_get(cache->geom2);
	}
	/* No cache hit. If we have a tree, free it. */
	else
	{
		cache_hit = 0;
		if ( cache->argnum )
		{
			cache_methods->GeomIndexFreer(cache);
			cache->argnum = 0;
		}
	}

	/* Cache hit, but no tree built yet, build it! */
	if ( cache_hit && ! cache->argnum )
	{
		int rv;
		LWGEOM *lwgeom;

		/* Save the tree and supporting geometry in the cache */
		/* memory context */
		old_context = MemoryContextSwitchTo(PostgisCacheContext(fcinfo));
		lwgeom = lwgeom_from_gserialized(geom);
		cache->argnum = 0;

		/* Can't build a tree on a NULL or empty */
		if ((!lwgeom) || lwgeom_is_empty(lwgeom))
		{
			MemoryContextSwitchTo(old_context);
			return NULL;
		}
		rv = cache_methods->GeomIndexBuilder(lwgeom, cache);
		MemoryContextSwitchTo(old_context);

		/* Something went awry in the tree build phase */
		if ( ! rv )
			return NULL;

		/* Only set an argnum if everything completely successfully */
		cache->argnum = cache_hit;
	}

	/* We have a hit and a calculated tree, we're done */
	if ( cache_hit && cache->argnum )
		return cache;

	/* Argument one didn't match, so copy the new value in. */
	if ( g1 && cache_hit != 1 )
	{
		if (cache->geom1)
			shared_gserialized_unref(fcinfo, cache->geom1);
		cache->geom1 = shared_gserialized_ref(fcinfo, g1);
	}

	/* Argument two didn't match, so copy the new value in. */
	if ( g2 && cache_hit != 2 )
	{
		if (cache->geom2)
			shared_gserialized_unref(fcinfo, cache->geom2);
		cache->geom2 = shared_gserialized_ref(fcinfo, g2);
	}

	return NULL;
}

/******************************************************************************/

inline static ToastCache*
ToastCacheGet(FunctionCallInfo fcinfo)
{
	const uint32_t entry_number = TOAST_CACHE_ENTRY;
	GenericCacheCollection* generic_cache = GetGenericCacheCollection(fcinfo);
	ToastCache* cache = (ToastCache*)(generic_cache->entry[entry_number]);
	if (!cache)
	{
		cache = MemoryContextAllocZero(PostgisCacheContext(fcinfo), sizeof(ToastCache));
		cache->type = entry_number;
		generic_cache->entry[entry_number] = (GenericCache*)cache;
	}
	return cache;
}

SHARED_GSERIALIZED *
ToastCacheGetGeometry(FunctionCallInfo fcinfo, uint32_t argnum)
{
	Assert(argnum < ToastCacheSize);
	ToastCache* cache = ToastCacheGet(fcinfo);
	ToastCacheArgument* arg = &(cache->arg[argnum]);

	Datum datum = PG_GETARG_DATUM(argnum);
	struct varlena *attr = (struct varlena *) DatumGetPointer(datum);

	/*
	* In general, you have to detoast the whole object to
	* determine if it's different from the cached object, but
	* for toasted objects, the va_valueid and va_toastrelid are
	* a unique key. Only objects toasted to disk have this
	* property, but fortunately those are also the objects
	* that are costliest to de-TOAST, which is why this
	* cache is a performance win.
	* https://www.postgresql.org/message-id/8196.1585870220@sss.pgh.pa.us
	*/
	if (!VARATT_IS_EXTERNAL_ONDISK(attr))
		return shared_gserialized_new_nocache(datum);

	/* Retrieve the unique keys for this object */
	struct varatt_external ve;
	VARATT_EXTERNAL_GET_POINTER(ve, attr);
	Oid valueid = ve.va_valueid;
	Oid toastrelid = ve.va_toastrelid;

	/* We've seen this object before? */
	if (arg->valueid == valueid && arg->toastrelid == toastrelid)
	{
		return arg->geom;
	}
	/* New object, clear our old copies and see if it */
	/* shows up a second time before taking a copy */
	else
	{
		if (arg->geom)
			shared_gserialized_unref(fcinfo, arg->geom);
		arg->valueid = valueid;
		arg->toastrelid = toastrelid;
		arg->geom = shared_gserialized_new_cached(fcinfo, datum);
		return arg->geom;
	}
}

/*
 * Retrieve an SRS from a given SRID
 * Require valid spatial_ref_sys table entry
 *
 * Could return SRS as short one (i.e EPSG:4326)
 * or as long one: (i.e urn:ogc:def:crs:EPSG::4326)
 */
static char *
getSRSbySRID(FunctionCallInfo fcinfo, int32_t srid, bool short_crs)
{
	static const uint16_t max_query_size = 512;
	char query[512];
	char *srs, *srscopy;
	int size, err;
	postgis_initialize_cache(fcinfo);

	if (SPI_OK_CONNECT != SPI_connect())
	{
		elog(NOTICE, "%s: could not connect to SPI manager", __func__);
		SPI_finish();
		return NULL;
	}

	if (short_crs)
		snprintf(query,
			 max_query_size,
			 "SELECT auth_name||':'||auth_srid \
		        FROM %s WHERE srid='%d'",
			 postgis_spatial_ref_sys(),
			 srid);
	else
		snprintf(query,
			 max_query_size,
			 "SELECT 'urn:ogc:def:crs:'||auth_name||'::'||auth_srid \
		        FROM %s WHERE srid='%d'",
			 postgis_spatial_ref_sys(),
			 srid);

	err = SPI_exec(query, 1);
	if (err < 0)
	{
		elog(NOTICE, "%s: error executing query %d", __func__, err);
		SPI_finish();
		return NULL;
	}

	/* no entry in spatial_ref_sys */
	if (SPI_processed <= 0)
	{
		SPI_finish();
		return NULL;
	}

	/* get result  */
	srs = SPI_getvalue(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1);

	/* NULL result */
	if (!srs)
	{
		SPI_finish();
		return NULL;
	}

	size = strlen(srs) + 1;

	srscopy = MemoryContextAllocZero(PostgisCacheContext(fcinfo), size);
	memcpy(srscopy, srs, size);

	/* disconnect from SPI */
	SPI_finish();

	return srscopy;
}

static inline SRSDescCache *
SRSDescCacheGet(FunctionCallInfo fcinfo)
{
	const uint32_t entry_number = SRSDESC_CACHE_ENTRY;
	GenericCacheCollection *generic_cache = GetGenericCacheCollection(fcinfo);
	SRSDescCache *cache = (SRSDescCache *)(generic_cache->entry[entry_number]);
	if (!cache)
	{
		cache = MemoryContextAllocZero(PostgisCacheContext(fcinfo), sizeof(SRSDescCache));
		cache->type = entry_number;
		generic_cache->entry[entry_number] = (GenericCache *)cache;
	}
	return cache;
}

const char *
GetSRSCacheBySRID(FunctionCallInfo fcinfo, int32_t srid, bool short_crs)
{
	SRSDescCache *cache = SRSDescCacheGet(fcinfo);
	SRSDescCacheArgument *arg = &(cache->arg[0]);

	if (arg->srid != srid || arg->short_mode != short_crs || !arg->srs)
	{
		arg->srid = srid;
		arg->short_mode = short_crs;
		if (arg->srs)
			pfree(arg->srs);
		arg->srs = getSRSbySRID(fcinfo, srid, short_crs);
	}
	return arg->srs;
}

/*
 * Retrieve an SRID from a given SRS
 * Require valid spatial_ref_sys table entry
 *
 */
static int32_t
getSRIDbySRS(FunctionCallInfo fcinfo, const char *srs)
{
	static const int16_t max_query_size = 512;
	char query[512];
	Oid argtypes[] = {CSTRINGOID};
	Datum values[] = {CStringGetDatum(srs)};
	int32_t srid, err;

	postgis_initialize_cache(fcinfo);
	snprintf(query,
		 max_query_size,
		 "SELECT srid "
		 "FROM %s, "
		 "regexp_matches($1::text, E'([a-z]+):([0-9]+)', 'gi') AS re "
		 "WHERE re[1] ILIKE auth_name AND int4(re[2]) = auth_srid",
		 postgis_spatial_ref_sys());

	if (!srs)
		return 0;

	if (SPI_OK_CONNECT != SPI_connect())
	{
		elog(NOTICE, "getSRIDbySRS: could not connect to SPI manager");
		return 0;
	}

	err = SPI_execute_with_args(query, 1, argtypes, values, NULL, true, 1);
	if (err < 0)
	{
		elog(NOTICE, "getSRIDbySRS: error executing query %d", err);
		SPI_finish();
		return 0;
	}

	/* no entry in spatial_ref_sys */
	if (SPI_processed <= 0)
	{
		snprintf(query,
			 max_query_size,
			 "SELECT srid "
			 "FROM %s, "
			 "regexp_matches($1::text, E'urn:ogc:def:crs:([a-z]+):.*:([0-9]+)', 'gi') AS re "
			 "WHERE re[1] ILIKE auth_name AND int4(re[2]) = auth_srid",
			 postgis_spatial_ref_sys());

		err = SPI_execute_with_args(query, 1, argtypes, values, NULL, true, 1);
		if (err < 0)
		{
			elog(NOTICE, "getSRIDbySRS: error executing query %d", err);
			SPI_finish();
			return 0;
		}

		if (SPI_processed <= 0)
		{
			SPI_finish();
			return 0;
		}
	}

	srid = atoi(SPI_getvalue(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1));
	SPI_finish();

	return srid;
}

static inline SRIDCache *
SRIDCacheGet(FunctionCallInfo fcinfo)
{
	const uint32_t entry_number = SRID_CACHE_ENTRY;
	GenericCacheCollection *generic_cache = GetGenericCacheCollection(fcinfo);
	SRIDCache *cache = (SRIDCache *)(generic_cache->entry[entry_number]);
	if (!cache)
	{
		cache = MemoryContextAllocZero(PostgisCacheContext(fcinfo), sizeof(SRIDCache));
		cache->type = entry_number;
		generic_cache->entry[entry_number] = (GenericCache *)cache;
	}
	return cache;
}

int32_t
GetSRIDCacheBySRS(FunctionCallInfo fcinfo, const char *srs)
{
	SRIDCache *cache = SRIDCacheGet(fcinfo);
	SRIDCacheArgument *arg = &(cache->arg[0]);

	if (!arg->srid || strcmp(srs, arg->srs) != 0)
	{
		size_t size = strlen(srs) + 1;
		arg->srid = getSRIDbySRS(fcinfo, srs);
		arg->srs = MemoryContextAlloc(PostgisCacheContext(fcinfo), size);
		memcpy(arg->srs, srs, size);
	}

	return arg->srid;
}
