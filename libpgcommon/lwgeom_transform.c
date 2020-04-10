/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright (C) 2001-2003 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/


/* PostgreSQL headers */
#include "postgres.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "utils/memutils.h"
#include "executor/spi.h"
#include "access/hash.h"
#include "utils/hsearch.h"

/* PostGIS headers */
#include "../postgis_config.h"
#include "liblwgeom.h"
#include "lwgeom_pg.h"
#include "lwgeom_cache.h"
#include "lwgeom_transform.h"

/* C headers */
#include <float.h>
#include <string.h>
#include <stdio.h>



/*
 * PROJ 4 backend hash table initial hash size
 * (since 16 is the default portal hash table size, and we would
 * typically have 2 entries per portal
 * then we shall use a default size of 256)
 */
#define PROJ_BACKEND_HASH_SIZE 256


/**
 * Utility structure to get many potential string representations
 * from spatial_ref_sys query.
 */
typedef struct {
	char* authtext;
	char* srtext;
	char* proj4text;
} PjStrs;

/* Internal Cache API */
static LWPROJ *AddToPROJSRSCache(PROJPortalCache *PROJCache, int32_t srid_from, int32_t srid_to);
static void DeleteFromPROJSRSCache(PROJPortalCache *PROJCache, uint32_t position);

static void
PROJSRSDestroyPJ(void *projection)
{
	LWPROJ *pj = (LWPROJ *)projection;
#if POSTGIS_PROJ_VERSION < 60
/* Ape the Proj 6+ API for versions < 6 */
	if (pj->pj_from)
	{
		pj_free(pj->pj_from);
		pj->pj_from = NULL;
	}
	if (pj->pj_to)
	{
		pj_free(pj->pj_to);
		pj->pj_to = NULL;
	}
#else
	if (pj->pj)
	{
		proj_destroy(pj->pj);
		pj->pj = NULL;
	}
#endif
}

#if POSTGIS_PGSQL_VERSION < 96
/**
 * Backend PROJ hash table
 *
 * This hash table stores a key/value pair of MemoryContext/PJ objects.
 * Whenever we create a PJ object using pj_init(), we create a separate
 * MemoryContext as a child context of the current executor context.
 * The MemoryContext/PJ object is stored in this hash table so
 * that when PROJSRSCacheDelete() is called during query cleanup, we can
 * lookup the PJ object based upon the MemoryContext parameter and hence
 * pj_free() it.
 */
static HTAB *PJHash = NULL;

typedef struct struct_PJHashEntry
{
	MemoryContext ProjectionContext;
	LWPROJ *projection;
} PJHashEntry;

/* PJ Hash API */
uint32 mcxt_ptr_hash(const void *key, Size keysize);

static HTAB *CreatePJHash(void);
static void DeletePJHashEntry(MemoryContext mcxt);
static LWPROJ *GetPJHashEntry(MemoryContext mcxt);
static void AddPJHashEntry(MemoryContext mcxt, LWPROJ *projection);

static void
PROJSRSCacheDelete(MemoryContext context)
{
	/* Lookup the PJ pointer in the global hash table so we can free it */
	LWPROJ *projection = GetPJHashEntry(context);

	if (!projection)
		elog(ERROR, "PROJSRSCacheDelete: Trying to delete non-existant projection object with MemoryContext key (%p)", (void *)context);

	POSTGIS_DEBUGF(3, "deleting projection object (%p) with MemoryContext key (%p)", projection, context);
	/* Free it */
	PROJSRSDestroyPJ(projection);

	/* Remove the hash entry as it is no longer needed */
	DeletePJHashEntry(context);
}

static void
PROJSRSCacheInit(MemoryContext context)
{
	/*
	 * Do nothing as the cache is initialised when the transform()
	 * function is first called
	 */
}

static void
PROJSRSCacheReset(MemoryContext context)
{
	/*
	 * Do nothing, but we must supply a function since this call is mandatory according to tgl
	 * (see postgis-devel archives July 2007)
	 */
}

static bool
PROJSRSCacheIsEmpty(MemoryContext context)
{
	/*
	 * Always return false since this call is mandatory according to tgl
	 * (see postgis-devel archives July 2007)
	 */
	return LW_FALSE;
}

static void
PROJSRSCacheStats(MemoryContext context, int level)
{
	/*
	 * Simple stats display function - we must supply a function since this call is mandatory according to tgl
	 * (see postgis-devel archives July 2007)
	 */

	fprintf(stderr, "%s: PROJ context\n", context->name);
}

#ifdef MEMORY_CONTEXT_CHECKING
static void
PROJSRSCacheCheck(MemoryContext context)
{
	/*
	 * Do nothing - stub required for when PostgreSQL is compiled
	 * with MEMORY_CONTEXT_CHECKING defined
	 */
}
#endif

/* Memory context definition must match the current version of PostgreSQL */
static MemoryContextMethods PROJSRSCacheContextMethods =
{
	NULL,
	NULL,
	NULL,
	PROJSRSCacheInit,
	PROJSRSCacheReset,
	PROJSRSCacheDelete,
	NULL,
	PROJSRSCacheIsEmpty,
	PROJSRSCacheStats
#ifdef MEMORY_CONTEXT_CHECKING
	,PROJSRSCacheCheck
#endif
};


/*
 * PROJ PJ Hash Table functions
 */


/**
 * A version of tag_hash - we specify this here as the implementation
 * has changed over the years....
 */

uint32 mcxt_ptr_hash(const void *key, Size keysize)
{
	uint32 hashval;

	hashval = DatumGetUInt32(hash_any(key, keysize));

	return hashval;
}


static HTAB *CreatePJHash(void)
{
	HASHCTL ctl;

	ctl.keysize = sizeof(MemoryContext);
	ctl.entrysize = sizeof(PJHashEntry);
	ctl.hash = mcxt_ptr_hash;

	return hash_create("PostGIS PROJ Backend MemoryContext Hash", PROJ_BACKEND_HASH_SIZE, &ctl, (HASH_ELEM | HASH_FUNCTION));
}

static void
AddPJHashEntry(MemoryContext mcxt, LWPROJ *projection)
{
	bool found;
	void **key;
	PJHashEntry *he;

	/* The hash key is the MemoryContext pointer */
	key = (void *)&mcxt;

	he = (PJHashEntry*) hash_search(PJHash, key, HASH_ENTER, &found);
	if (!found)
	{
		/* Insert the entry into the new hash element */
		he->ProjectionContext = mcxt;
		he->projection = projection;
	}
	else
	{
		elog(ERROR, "AddPJHashEntry: PROJ projection object already exists for this MemoryContext (%p)",
		     (void *)mcxt);
	}
}

static LWPROJ *
GetPJHashEntry(MemoryContext mcxt)
{
	void **key;
	PJHashEntry *he;

	/* The hash key is the MemoryContext pointer */
	key = (void *)&mcxt;

	/* Return the projection object from the hash */
	he = (PJHashEntry *) hash_search(PJHash, key, HASH_FIND, NULL);

	return he ? he->projection : NULL;
}


static void DeletePJHashEntry(MemoryContext mcxt)
{
	void **key;
	PJHashEntry *he;

	/* The hash key is the MemoryContext pointer */
	key = (void *)&mcxt;

	/* Delete the projection object from the hash */
	he = (PJHashEntry *) hash_search(PJHash, key, HASH_REMOVE, NULL);

	if (!he)
		elog(ERROR, "DeletePJHashEntry: There was an error removing the PROJ projection object from this MemoryContext (%p)", (void *)mcxt);
	else
		he->projection = NULL;
}

#endif /* POSTGIS_PGSQL_VERSION < 96 */

/*****************************************************************************
 * Per-cache management functions
 */

static LWPROJ *
GetProjectionFromPROJCache(PROJPortalCache *cache, int32_t srid_from, int32_t srid_to)
{
	uint32_t i;
	for (i = 0; i < cache->PROJSRSCacheCount; i++)
	{
		if (cache->PROJSRSCache[i].srid_from == srid_from &&
		    cache->PROJSRSCache[i].srid_to == srid_to)
		{
			cache->PROJSRSCache[i].hits++;
			return cache->PROJSRSCache[i].projection;
		}
	}

	return NULL;
}

static char*
SPI_pstrdup(const char* str)
{
	char* ostr = NULL;
	if (str)
	{
		ostr = SPI_palloc(strlen(str)+1);
		strcpy(ostr, str);
	}
	return ostr;
}

static PjStrs
GetProjStringsSPI(int32_t srid)
{
	const int maxprojlen = 512;
	const int spibufferlen = 512;
	int spi_result;
	char proj_spi_buffer[spibufferlen];
	PjStrs strs;
	memset(&strs, 0, sizeof(strs));

	/* Connect */
	spi_result = SPI_connect();
	if (spi_result != SPI_OK_CONNECT)
	{
		elog(ERROR, "Could not connect to database using SPI");
	}

	static char *proj_str_tmpl =
	    "SELECT proj4text, auth_name, auth_srid, srtext "
	    "FROM %s "
	    "WHERE srid = %d "
	    "LIMIT 1";
	snprintf(proj_spi_buffer, spibufferlen, proj_str_tmpl, postgis_spatial_ref_sys(), srid);

	/* Execute the query, noting the readonly status of this SQL */
	spi_result = SPI_execute(proj_spi_buffer, true, 1);

	/* Read back the PROJ text */
	if (spi_result == SPI_OK_SELECT && SPI_processed > 0)
	{
		/* Select the first (and only tuple) */
		TupleDesc tupdesc = SPI_tuptable->tupdesc;
		SPITupleTable *tuptable = SPI_tuptable;
		HeapTuple tuple = tuptable->vals[0];
		/* Always return the proj4text */
		char* proj4text = SPI_getvalue(tuple, tupdesc, 1);
		if (proj4text && strlen(proj4text))
			strs.proj4text = SPI_pstrdup(proj4text);

		/* For Proj >= 6 prefer "AUTHNAME:AUTHSRID" to proj strings */
		/* as proj_create_crs_to_crs() will give us more consistent */
		/* results with authority numbers than with proj strings */
		char* authname = SPI_getvalue(tuple, tupdesc, 2);
		char* authsrid = SPI_getvalue(tuple, tupdesc, 3);
		if (authname && authsrid &&
		    strlen(authname) && strlen(authsrid))
		{
			char tmp[maxprojlen];
			snprintf(tmp, maxprojlen, "%s:%s", authname, authsrid);
			strs.authtext = SPI_pstrdup(tmp);
		}

		/* Proj6+ can parse srtext, so return that too */
		char* srtext = SPI_getvalue(tuple, tupdesc, 4);
		if (srtext && strlen(srtext))
			strs.srtext = SPI_pstrdup(srtext);
	}
	else
	{
		elog(ERROR, "Cannot find SRID (%d) in spatial_ref_sys", srid);
	}

	spi_result = SPI_finish();
	if (spi_result != SPI_OK_FINISH)
	{
		elog(ERROR, "Could not disconnect from database using SPI");
	}

	return strs;
}


/**
 *  Given an SRID, return the proj4 text.
 *  If the integer is one of the "well known" projections we support
 *  (WGS84 UTM N/S, Polar Stereographic N/S - see SRID_* macros),
 *  return the proj4text for those.
 */
static PjStrs
GetProjStrings(int32_t srid)
{
	const int maxprojlen = 512;
	PjStrs strs;
	memset(&strs, 0, sizeof(strs));

	/* SRIDs in SPATIAL_REF_SYS */
	if ( srid < SRID_RESERVE_OFFSET )
	{
		return GetProjStringsSPI(srid);
	}
	/* Automagic SRIDs */
	else
	{
		strs.proj4text = palloc(maxprojlen);
		int id = srid;
		/* UTM North */
		if ( id >= SRID_NORTH_UTM_START && id <= SRID_NORTH_UTM_END )
		{
			snprintf(strs.proj4text, maxprojlen, "+proj=utm +zone=%d +ellps=WGS84 +datum=WGS84 +units=m +no_defs", id - SRID_NORTH_UTM_START + 1);
		}
		/* UTM South */
		else if ( id >= SRID_SOUTH_UTM_START && id <= SRID_SOUTH_UTM_END )
		{
			snprintf(strs.proj4text, maxprojlen, "+proj=utm +zone=%d +south +ellps=WGS84 +datum=WGS84 +units=m +no_defs", id - SRID_SOUTH_UTM_START + 1);
		}
		/* Lambert zones (about 30x30, larger in higher latitudes) */
		/* There are three latitude zones, divided at -90,-60,-30,0,30,60,90. */
		/* In yzones 2,3 (equator) zones, the longitudinal zones are divided every 30 degrees (12 of them) */
		/* In yzones 1,4 (temperate) zones, the longitudinal zones are every 45 degrees (8 of them) */
		/* In yzones 0,5 (polar) zones, the longitudinal zones are ever 90 degrees (4 of them) */
		else if ( id >= SRID_LAEA_START && id <= SRID_LAEA_END )
		{
			int zone = id - SRID_LAEA_START;
			int xzone = zone % 20;
			int yzone = zone / 20;
			double lat_0 = 30.0 * (yzone - 3) + 15.0;
			double lon_0 = 0.0;

			/* The number of xzones is variable depending on yzone */
			if  ( yzone == 2 || yzone == 3 )
				lon_0 = 30.0 * (xzone - 6) + 15.0;
			else if ( yzone == 1 || yzone == 4 )
				lon_0 = 45.0 * (xzone - 4) + 22.5;
			else if ( yzone == 0 || yzone == 5 )
				lon_0 = 90.0 * (xzone - 2) + 45.0;
			else
				lwerror("Unknown yzone encountered!");

			snprintf(strs.proj4text, maxprojlen, "+proj=laea +ellps=WGS84 +datum=WGS84 +lat_0=%g +lon_0=%g +units=m +no_defs", lat_0, lon_0);
		}
		/* Lambert Azimuthal Equal Area South Pole */
		else if ( id == SRID_SOUTH_LAMBERT )
		{
			strncpy(strs.proj4text, "+proj=laea +lat_0=-90 +lon_0=0 +x_0=0 +y_0=0 +ellps=WGS84 +datum=WGS84 +units=m +no_defs", maxprojlen );
		}
		/* Polar Sterographic South */
		else if ( id == SRID_SOUTH_STEREO )
		{
			strncpy(strs.proj4text, "+proj=stere +lat_0=-90 +lat_ts=-71 +lon_0=0 +k=1 +x_0=0 +y_0=0 +ellps=WGS84 +datum=WGS84 +units=m +no_defs", maxprojlen);
		}
		/* Lambert Azimuthal Equal Area North Pole */
		else if ( id == SRID_NORTH_LAMBERT )
		{
			strncpy(strs.proj4text, "+proj=laea +lat_0=90 +lon_0=-40 +x_0=0 +y_0=0 +ellps=WGS84 +datum=WGS84 +units=m +no_defs", maxprojlen );
		}
		/* Polar Stereographic North */
		else if ( id == SRID_NORTH_STEREO )
		{
			strncpy(strs.proj4text, "+proj=stere +lat_0=90 +lat_ts=71 +lon_0=0 +k=1 +x_0=0 +y_0=0 +ellps=WGS84 +datum=WGS84 +units=m +no_defs", maxprojlen );
		}
		/* World Mercator */
		else if ( id == SRID_WORLD_MERCATOR )
		{
			strncpy(strs.proj4text, "+proj=merc +lon_0=0 +k=1 +x_0=0 +y_0=0 +ellps=WGS84 +datum=WGS84 +units=m +no_defs", maxprojlen );
		}
		else
		{
			elog(ERROR, "Invalid reserved SRID (%d)", srid);
			return strs;
		}

		POSTGIS_DEBUGF(3, "returning on SRID=%d: %s", srid, strs.proj4text);
		return strs;
	}
}

static int
pjstrs_has_entry(const PjStrs *strs)
{
	if ((strs->proj4text && strlen(strs->proj4text)) ||
		(strs->authtext && strlen(strs->authtext)) ||
		(strs->srtext && strlen(strs->srtext)))
		return 1;
	else
		return 0;
}

static void
pjstrs_pfree(PjStrs *strs)
{
	if (strs->proj4text)
		pfree(strs->proj4text);
	if (strs->authtext)
		pfree(strs->authtext);
	if (strs->srtext)
		pfree(strs->srtext);
}

#if POSTGIS_PROJ_VERSION >= 60
static char*
pgstrs_get_entry(const PjStrs *strs, int n)
{
	switch (n)
	{
		case 0:
			return strs->authtext;
		case 1:
			return strs->srtext;
		case 2:
			return strs->proj4text;
		default:
			return NULL;
	}
}
#endif

#if POSTGIS_PROJ_VERSION < 60
/*
* Utility function for GML reader that still
* needs proj4text access
*/
char *
GetProj4String(int32_t srid)
{
	PjStrs strs;
	char *proj4str;
	memset(&strs, 0, sizeof(strs));
	strs = GetProjStringsSPI(srid);
	proj4str = pstrdup(strs.proj4text);
	pjstrs_pfree(&strs);
	return proj4str;
}
#endif

/**
 * Add an entry to the local PROJ SRS cache. If we need to wrap around then
 * we must make sure the entry we choose to delete does not contain other_srid
 * which is the definition for the other half of the transformation.
 */
static LWPROJ *
AddToPROJSRSCache(PROJPortalCache *PROJCache, int32_t srid_from, int32_t srid_to)
{
#if POSTGIS_PGSQL_VERSION < 96
	MemoryContext PJMemoryContext;
#endif
	MemoryContext oldContext;

	PjStrs from_strs, to_strs;
	char *pj_from_str, *pj_to_str;

	/*
	** Turn the SRID number into a proj4 string, by reading from spatial_ref_sys
	** or instantiating a magical value from a negative srid.
	*/
	from_strs = GetProjStrings(srid_from);
	if (!pjstrs_has_entry(&from_strs))
		elog(ERROR, "got NULL for SRID (%d)", srid_from);
	to_strs = GetProjStrings(srid_to);
	if (!pjstrs_has_entry(&to_strs))
		elog(ERROR, "got NULL for SRID (%d)", srid_to);

	oldContext = MemoryContextSwitchTo(PROJCache->PROJSRSCacheContext);

#if POSTGIS_PROJ_VERSION < 60
	PJ *projection = palloc(sizeof(PJ));
	pj_from_str = from_strs.proj4text;
	pj_to_str = to_strs.proj4text;
	projection->pj_from = projpj_from_string(pj_from_str);
	projection->pj_to = projpj_from_string(pj_to_str);

	if (!projection->pj_from)
		elog(ERROR,
		    "could not form projection from 'srid=%d' to 'srid=%d'",
		    srid_from, srid_to);

	if (!projection->pj_to)
		elog(ERROR,
		    "could not form projection from 'srid=%d' to 'srid=%d'",
		    srid_from, srid_to);
#else
	PJ *projpj = NULL;
	/* Try combinations of ESPG/SRTEXT/PROJ4TEXT until we find */
	/* one that gives us a usable transform. Note that we prefer */
	/* EPSG numbers over SRTEXT and SRTEXT over PROJ4TEXT */
	/* (3 entries * 3 entries = 9 combos) */
	uint32_t i;
	for (i = 0; i < 9; i++)
	{
		pj_from_str = pgstrs_get_entry(&from_strs, i / 3);
		pj_to_str   = pgstrs_get_entry(&to_strs,   i % 3);
		if (!(pj_from_str && pj_to_str))
			continue;
		projpj = proj_create_crs_to_crs(NULL, pj_from_str, pj_to_str, NULL);
		if (projpj && !proj_errno(projpj))
			break;
	}
	if (!projpj)
	{
		elog(ERROR, "could not form projection (PJ) from 'srid=%d' to 'srid=%d'", srid_from, srid_to);
		return NULL;
	}
	LWPROJ *projection = lwproj_from_PJ(projpj, srid_from == srid_to);
	if (!projection)
	{
		elog(ERROR, "could not form projection (LWPROJ) from 'srid=%d' to 'srid=%d'", srid_from, srid_to);
		return NULL;
	}
#endif

	/* If the cache is already full then find the least used element and delete it */
	uint32_t cache_position = PROJCache->PROJSRSCacheCount;
	uint32_t hits = 1;
	if (cache_position == PROJ_CACHE_ITEMS)
	{
		cache_position = 0;
		hits = PROJCache->PROJSRSCache[0].hits;
		for (uint32_t i = 1; i < PROJ_CACHE_ITEMS; i++)
		{
			if (PROJCache->PROJSRSCache[i].hits < hits)
			{
				cache_position = i;
				hits = PROJCache->PROJSRSCache[i].hits;
			}
		}
		DeleteFromPROJSRSCache(PROJCache, cache_position);
		/* To avoid the element we are introduced now being evicted next (as
		 * it would have 1 hit, being most likely the lower one) we reuse the
		 * hits from the evicted position and add some extra buffer
		 */
		hits += 5;
	}
	else
	{
		PROJCache->PROJSRSCacheCount++;
	}

	POSTGIS_DEBUGF(3,
		       "adding transform %d => %d aka \"%s\" => \"%s\" to query cache at index %d",
		       srid_from,
		       srid_to,
		       pj_from_str,
		       pj_to_str,
		       cache_position);

	/* Free the projection strings */
	pjstrs_pfree(&from_strs);
	pjstrs_pfree(&to_strs);

#if POSTGIS_PGSQL_VERSION < 96
	/*
	 * Now create a memory context for this projection and
	 * store it in the backend hash
	 */
	PJMemoryContext = MemoryContextCreate(T_AllocSetContext, 8192,
	                                      &PROJSRSCacheContextMethods,
	                                      PROJCache->PROJSRSCacheContext,
	                                      "PostGIS PROJ PJ Memory Context");

	/* We register a new memory context to use it to delete the projection on exit */
	if (!PJHash)
		PJHash = CreatePJHash();

	AddPJHashEntry(PJMemoryContext, projection);

#else
	/* We register a new callback to delete the projection on exit */
	MemoryContextCallback *callback =
	    MemoryContextAlloc(PROJCache->PROJSRSCacheContext, sizeof(MemoryContextCallback));
	callback->func = PROJSRSDestroyPJ;
	callback->arg = (void *)projection;
	MemoryContextRegisterResetCallback(PROJCache->PROJSRSCacheContext, callback);
#endif

	PROJCache->PROJSRSCache[cache_position].srid_from = srid_from;
	PROJCache->PROJSRSCache[cache_position].srid_to = srid_to;
	PROJCache->PROJSRSCache[cache_position].projection = projection;
	PROJCache->PROJSRSCache[cache_position].hits = hits;
#if POSTGIS_PGSQL_VERSION < 96
	PROJCache->PROJSRSCache[cache_position].projection_mcxt = PJMemoryContext;
#endif

	MemoryContextSwitchTo(oldContext);
	return projection;
}

static void
DeleteFromPROJSRSCache(PROJPortalCache *PROJCache, uint32_t position)
{
	POSTGIS_DEBUGF(3,
		       "removing query cache entry with SRIDs %d => %d at index %u",
		       PROJCache->PROJSRSCache[position].srid_from,
		       PROJCache->PROJSRSCache[position].srid_to,
		       position);

#if POSTGIS_PGSQL_VERSION < 96
	/* Deleting the memory context will free the PROJ objects */
	MemoryContextDelete(PROJCache->PROJSRSCache[position].projection_mcxt);
	PROJCache->PROJSRSCache[position].projection_mcxt = NULL;
#else
	/* Call PROJSRSDestroyPJ to free the PROJ objects memory now instead of
	 * waiting for the parent memory context to exit */
	PROJSRSDestroyPJ(PROJCache->PROJSRSCache[position].projection);
#endif
	PROJCache->PROJSRSCache[position].projection = NULL;
	PROJCache->PROJSRSCache[position].srid_from = SRID_UNKNOWN;
	PROJCache->PROJSRSCache[position].srid_to = SRID_UNKNOWN;
}


int
GetPJUsingFCInfo(FunctionCallInfo fcinfo, int32_t srid_from, int32_t srid_to, LWPROJ **pj)
{
	PROJPortalCache *proj_cache = NULL;

	/* Look up the spatial_ref_sys schema if we haven't already */
	postgis_initialize_cache(fcinfo);

	/* get or initialize the cache for this round */
	proj_cache = GetPROJSRSCache(fcinfo);
	if (!proj_cache)
		return LW_FAILURE;

	/* Add the output srid to the cache if it's not already there */
	*pj = GetProjectionFromPROJCache(proj_cache, srid_from, srid_to);
	if (*pj == NULL)
	{
		*pj = AddToPROJSRSCache(proj_cache, srid_from, srid_to);
	}

	return pj != NULL;
}

static int
proj_pj_is_latlong(const LWPROJ *pj)
{
#if POSTGIS_PROJ_VERSION < 60
	return pj_is_latlong(pj->pj_from);
#else
	return pj->source_is_latlong;
#endif
}

static int
srid_is_latlong(FunctionCallInfo fcinfo, int32_t srid)
{
	LWPROJ *pj;
	if ( GetPJUsingFCInfo(fcinfo, srid, srid, &pj) == LW_FAILURE)
		return LW_FALSE;
	return proj_pj_is_latlong(pj);
}

void
srid_check_latlong(FunctionCallInfo fcinfo, int32_t srid)
{
	if (srid == SRID_DEFAULT || srid == SRID_UNKNOWN)
		return;

	if (srid_is_latlong(fcinfo, srid))
		return;

	ereport(ERROR, (
	            errcode(ERRCODE_INVALID_PARAMETER_VALUE),
	            errmsg("Only lon/lat coordinate systems are supported in geography.")));
}

srs_precision
srid_axis_precision(FunctionCallInfo fcinfo, int32_t srid, int precision)
{
	srs_precision sp;
	sp.precision_xy = precision;
	sp.precision_z = precision;
	sp.precision_m = precision;

	if ( srid == SRID_UNKNOWN )
		return sp;

	if ( srid_is_latlong(fcinfo, srid) )
	{
		sp.precision_xy += 5;
		return sp;
	}

	return sp;
}

int
spheroid_init_from_srid(FunctionCallInfo fcinfo, int32_t srid, SPHEROID *s)
{
	LWPROJ *pj;
#if POSTGIS_PROJ_VERSION >= 48 && POSTGIS_PROJ_VERSION < 60
	double major_axis, minor_axis, eccentricity_squared;
#endif

	if ( GetPJUsingFCInfo(fcinfo, srid, srid, &pj) == LW_FAILURE)
		return LW_FAILURE;

#if POSTGIS_PROJ_VERSION >= 60
	if (!pj->source_is_latlong)
		return LW_FAILURE;
	spheroid_init(s, pj->source_semi_major_metre, pj->source_semi_minor_metre);

#elif POSTGIS_PROJ_VERSION >= 48
	if (!pj_is_latlong(pj->pj_from))
		return LW_FAILURE;
	/* For newer versions of Proj we can pull the spheroid paramaeters and initialize */
	/* using them */
	pj_get_spheroid_defn(pj->pj_from, &major_axis, &eccentricity_squared);
	minor_axis = major_axis * sqrt(1-eccentricity_squared);
	spheroid_init(s, major_axis, minor_axis);

#else
	if (!pj_is_latlong(pj->pj_from))
		return LW_FAILURE;
	/* For old versions of Proj we cannot lookup the spheroid parameters from the API */
	/* So we use the WGS84 parameters (boo!) */
	spheroid_init(s, WGS84_MAJOR_AXIS, WGS84_MINOR_AXIS);
#endif

	return LW_SUCCESS;
}
