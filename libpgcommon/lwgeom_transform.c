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


/**
* Global variable to hold cached information about what
* schema functions are installed in. Currently used by
* SetSpatialRefSysSchema and GetProjStringsSPI
*/
static char *spatialRefSysSchema = NULL;


/*
 * PROJ 4 backend hash table initial hash size
 * (since 16 is the default portal hash table size, and we would
 * typically have 2 entries per portal
 * then we shall use a default size of 32)
 */
#define PROJ_BACKEND_HASH_SIZE	32


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

/**
 * Utility structure to get many potential string representations
 * from spatial_ref_sys query.
 */
typedef struct {
	char* epsgtext;
	char* srtext;
	char* proj4text;
} PjStrs;


typedef struct struct_PJHashEntry
{
	MemoryContext ProjectionContext;
	PJ* projection;
}
PJHashEntry;


/* PJ Hash API */
uint32 mcxt_ptr_hash(const void *key, Size keysize);

static HTAB *CreatePJHash(void);
static void DeletePJHashEntry(MemoryContext mcxt);
static PJ* GetPJHashEntry(MemoryContext mcxt);
static void AddPJHashEntry(MemoryContext mcxt, PJ* projection);

/* Internal Cache API */
/* static PROJPortalCache *GetPROJSRSCache(FunctionCallInfo fcinfo) ; */
static bool IsInPROJSRSCache(PROJPortalCache *PROJCache, int32_t srid_from, int32_t srid_to);
static void AddToPROJSRSCache(PROJPortalCache *PROJCache, int32_t srid_from, int32_t srid_to);
static void DeleteFromPROJSRSCache(PROJPortalCache *PROJCache, int32_t srid_from, int32_t srid_to);

/*
* Given a function call context, figure out what namespace the
* function is being called from, and copy that into a global
* for use by GetProjStringsSPI
*/
static void
SetSpatialRefSysSchema(FunctionCallInfo fcinfo)
{
	char *nsp_name;

	/* Schema info is already cached, we're done here */
	if (spatialRefSysSchema) return;

	/* For some reason we have a hobbled fcinfo/flinfo */
	if (!fcinfo || !fcinfo->flinfo) return;

	nsp_name = get_namespace_name(get_func_namespace(fcinfo->flinfo->fn_oid));
	/* early exit if we cannot lookup nsp_name, cf #4067 */
	if (!nsp_name) return;

	elog(DEBUG4, "%s located %s in namespace %s", __func__, get_func_name(fcinfo->flinfo->fn_oid), nsp_name);
	spatialRefSysSchema = MemoryContextStrdup(CacheMemoryContext, nsp_name);
	return;
}

static void
PROJSRSDestroyPJ(PJ* pj)
{
#if POSTGIS_PROJ_VERSION < 60
/* Ape the Proj 6+ API for versions < 6 */
	if (pj->pj_from)
		pj_free(pj->pj_from);
	if (pj->pj_to)
		pj_free(pj->pj_to);
	free(pj);
#else
	proj_destroy(pj);
#endif
}

#if 0
static const char *
PJErrStr()
{
	const char *pj_errstr = pj_strerrno(*pj_get_errno_ref());
	if (!pj_errstr)
		return "";
	return pj_errstr;
}
#endif

static void
#if POSTGIS_PGSQL_VERSION < 96
PROJSRSCacheDelete(MemoryContext context)
{
#else
PROJSRSCacheDelete(void *ptr)
{
	MemoryContext context = (MemoryContext)ptr;
#endif

	/* Lookup the PJ pointer in the global hash table so we can free it */
	PJ* projection = GetPJHashEntry(context);

	if (!projection)
		elog(ERROR, "PROJSRSCacheDelete: Trying to delete non-existant projection object with MemoryContext key (%p)", (void *)context);

	POSTGIS_DEBUGF(3, "deleting projection object (%p) with MemoryContext key (%p)", projection, context);
	/* Free it */
	PROJSRSDestroyPJ(projection);

	/* Remove the hash entry as it is no longer needed */
	DeletePJHashEntry(context);
}

#if POSTGIS_PGSQL_VERSION < 96

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

#endif /* POSTGIS_PGSQL_VERSION < 96 */

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

static void AddPJHashEntry(MemoryContext mcxt, PJ* projection)
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

static PJ* GetPJHashEntry(MemoryContext mcxt)
{
	void **key;
	PJHashEntry *he;

	/* The hash key is the MemoryContext pointer */
	key = (void *)&mcxt;

	/* Return the projection object from the hash */
	he = (PJHashEntry *) hash_search(PJHash, key, HASH_FIND, NULL);

	return he->projection;
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


/*****************************************************************************
 * Per-cache management functions
 */

static bool
IsInPROJSRSCache(PROJPortalCache *cache, int32_t srid_from, int32_t srid_to)
{
	/*
	 * Return true/false depending upon whether the item
	 * is in the SRS cache.
	 */
	uint32_t i;
	for (i = 0; i < PROJ_CACHE_ITEMS; i++)
	{
		if (cache->PROJSRSCache[i].srid_from == srid_from &&
		    cache->PROJSRSCache[i].srid_to == srid_to)
			return true;
	}
	/* Otherwise not found */
	return false;
}

static PJ *
GetProjectionFromPROJCache(PROJPortalCache *cache, int32_t srid_from, int32_t srid_to)
{
	uint32_t i;
	for (i = 0; i < PROJ_CACHE_ITEMS; i++)
	{
		if (cache->PROJSRSCache[i].srid_from == srid_from &&
		    cache->PROJSRSCache[i].srid_to == srid_to)
			return cache->PROJSRSCache[i].projection;
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

	/*
	* This global is allocated in CacheMemoryContext (lifespan of this backend)
	* and is set by SetSpatialRefSysSchema the first time
	* that GetPJUsingFCInfo is called.
	*/
	static char *proj_str_tmpl = "SELECT proj4text, auth_name, auth_srid, srtext "
	                             "FROM %s%sspatial_ref_sys "
	                             "WHERE srid = %d "
	                             "LIMIT 1";
	if (spatialRefSysSchema)
	{
		snprintf(proj_spi_buffer, spibufferlen, proj_str_tmpl, spatialRefSysSchema, ".", srid);
	}
	else
	{
		snprintf(proj_spi_buffer, spibufferlen, proj_str_tmpl, "", "", srid);
	}

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

		/* For Proj >= 6 prefer "EPSG:XXXX" to proj strings */
		/* as proj_create_crs_to_crs() will give us more consistent */
		/* results with EPSG numbers than with proj strings */
		char* authname = SPI_getvalue(tuple, tupdesc, 2);
		char* authsrid = SPI_getvalue(tuple, tupdesc, 3);
		if (authname && authsrid &&
		    strcmp(authname,"EPSG") == 0 &&
		    strlen(authsrid))
		{
			char tmp[maxprojlen];
			snprintf(tmp, maxprojlen, "EPSG:%s", authsrid);
			strs.epsgtext = SPI_pstrdup(tmp);
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
		(strs->epsgtext && strlen(strs->epsgtext)) ||
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
	if (strs->epsgtext)
		pfree(strs->epsgtext);
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
			return strs->epsgtext;
		case 1:
			return strs->srtext;
		case 2:
			return strs->proj4text;
		default:
			return NULL;
	}
}
#endif

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

/**
 * Add an entry to the local PROJ SRS cache. If we need to wrap around then
 * we must make sure the entry we choose to delete does not contain other_srid
 * which is the definition for the other half of the transformation.
 */
static void
AddToPROJSRSCache(PROJPortalCache *PROJCache, int32_t srid_from, int32_t srid_to)
{
	MemoryContext PJMemoryContext;

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

#if POSTGIS_PROJ_VERSION < 60
	PJ* projection = malloc(sizeof(PJ));
	pj_from_str = from_strs.proj4text;
	pj_to_str = to_strs.proj4text;
	projection->pj_from = lwproj_from_string(pj_from_str);
	projection->pj_to = lwproj_from_string(pj_to_str);

	if (!projection->pj_from)
		elog(ERROR,
		    "could not form projection from 'srid=%d' to 'srid=%d'",
		    srid_from, srid_to);

	if (!projection->pj_to)
		elog(ERROR,
		    "could not form projection from 'srid=%d' to 'srid=%d'",
		    srid_from, srid_to);
#else
	PJ* projection = NULL;
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
		projection = proj_create_crs_to_crs(NULL, pj_from_str, pj_to_str, NULL);
		if (projection && !proj_errno(projection))
			break;
	}
	if (!projection)
	{
		elog(ERROR,
		    "could not form projection from 'srid=%d' to 'srid=%d'",
		    srid_from, srid_to);
	}
#endif

	/*
	 * If the cache is already full then find the first entry
	 * that doesn't contain our srids and use this as the
	 * subsequent value of PROJSRSCacheCount
	 */
	if (PROJCache->PROJSRSCacheCount == PROJ_CACHE_ITEMS)
	{
		bool found = false;
		uint32_t i;
		for (i = 0; i < PROJ_CACHE_ITEMS; i++)
		{
			if (found == false &&
				(PROJCache->PROJSRSCache[i].srid_from != srid_from ||
				 PROJCache->PROJSRSCache[i].srid_to != srid_to))
			{
				POSTGIS_DEBUGF(3, "choosing to remove item from query cache with SRIDs %d => %d and index %d",
					PROJCache->PROJSRSCache[i].srid_from,
					PROJCache->PROJSRSCache[i].srid_to,
					i);

				DeleteFromPROJSRSCache(PROJCache,
				    PROJCache->PROJSRSCache[i].srid_from,
				    PROJCache->PROJSRSCache[i].srid_to);

				PROJCache->PROJSRSCacheCount = i;
				found = true;
			}
		}
	}

	/*
	 * Now create a memory context for this projection and
	 * store it in the backend hash
	 */
	POSTGIS_DEBUGF(3, "adding transform %d => %d aka \"%s\" => \"%s\" to query cache at index %d",
		srid_from, srid_to, pj_from_str, pj_to_str, PROJCache->PROJSRSCacheCount);

	/* Free the projection strings */
	pjstrs_pfree(&from_strs);
	pjstrs_pfree(&to_strs);

#if POSTGIS_PGSQL_VERSION < 96
	PJMemoryContext = MemoryContextCreate(T_AllocSetContext, 8192,
	                                      &PROJSRSCacheContextMethods,
	                                      PROJCache->PROJSRSCacheContext,
	                                      "PostGIS PROJ PJ Memory Context");
#else
	PJMemoryContext = AllocSetContextCreate(PROJCache->PROJSRSCacheContext,
	                                        "PostGIS PROJ PJ Memory Context",
	                                        ALLOCSET_SMALL_SIZES);

	/* PgSQL comments suggest allocating callback in the context */
	/* being managed, so that the callback object gets cleaned along with */
	/* the context */
	{
		MemoryContextCallback *callback = MemoryContextAlloc(PJMemoryContext, sizeof(MemoryContextCallback));
		callback->arg = (void*)PJMemoryContext;
		callback->func = PROJSRSCacheDelete;
		MemoryContextRegisterResetCallback(PJMemoryContext, callback);
	}
#endif

 	/* Create the backend hash if it doesn't already exist */
	if (!PJHash)
		PJHash = CreatePJHash();

	/*
	 * Add the MemoryContext to the backend hash so we can
	 * clean up upon portal shutdown
	 */
	POSTGIS_DEBUGF(3, "adding projection object (%p) to hash table with MemoryContext key (%p)", projection, PJMemoryContext);

	AddPJHashEntry(PJMemoryContext, projection);

	PROJCache->PROJSRSCache[PROJCache->PROJSRSCacheCount].srid_from = srid_from;
	PROJCache->PROJSRSCache[PROJCache->PROJSRSCacheCount].srid_to = srid_to;
	PROJCache->PROJSRSCache[PROJCache->PROJSRSCacheCount].projection = projection;
	PROJCache->PROJSRSCache[PROJCache->PROJSRSCacheCount].projection_mcxt = PJMemoryContext;
	PROJCache->PROJSRSCacheCount++;
}

static void
DeleteFromPROJSRSCache(PROJPortalCache *PROJCache, int32_t srid_from, int32_t srid_to)
{
	/*
	 * Delete the SRID entry from the cache
	 */
	uint32_t i;
	for (i = 0; i < PROJ_CACHE_ITEMS; i++)
	{
		if (PROJCache->PROJSRSCache[i].srid_from == srid_from &&
			PROJCache->PROJSRSCache[i].srid_to == srid_to)
		{
			POSTGIS_DEBUGF(3, "removing query cache entry with SRIDs %d => %d at index %d", srid_from, srid_to, i);

			/*
			 * Zero out the entries and free the PROJ handle
			 * by deleting the memory context
			 */
			MemoryContextDelete(PROJCache->PROJSRSCache[i].projection_mcxt);
			PROJCache->PROJSRSCache[i].projection = NULL;
			PROJCache->PROJSRSCache[i].projection_mcxt = NULL;
			PROJCache->PROJSRSCache[i].srid_from = SRID_UNKNOWN;
			PROJCache->PROJSRSCache[i].srid_to = SRID_UNKNOWN;
		}
	}
}


int
GetPJUsingFCInfo(FunctionCallInfo fcinfo, int32_t srid_from, int32_t srid_to, PJ **pj)
{
	PROJPortalCache *proj_cache = NULL;

	/* Look up the spatial_ref_sys schema if we haven't already */
	SetSpatialRefSysSchema(fcinfo);

	/* get or initialize the cache for this round */
	proj_cache = GetPROJSRSCache(fcinfo);
	if (!proj_cache)
		return LW_FAILURE;

	/* Add the output srid to the cache if it's not already there */
	if (!IsInPROJSRSCache(proj_cache, srid_from, srid_to))
		AddToPROJSRSCache(proj_cache, srid_from, srid_to);

	/* Get the projections */
	*pj = GetProjectionFromPROJCache(proj_cache, srid_from, srid_to);

	return LW_SUCCESS;
}

static int
proj_pj_is_latlong(const PJ* pj)
{
#if POSTGIS_PROJ_VERSION < 60
	return pj_is_latlong(pj->pj_from);
#else
	PJ_TYPE pj_type;
	PJ *pj_src_crs = proj_get_source_crs(NULL, pj);
	if (!pj_src_crs)
		elog(ERROR, "%s: proj_get_source_crs returned NULL", __func__);
	pj_type = proj_get_type(pj_src_crs);
	proj_destroy(pj_src_crs);
	return (pj_type == PJ_TYPE_GEOGRAPHIC_2D_CRS) ||
	       (pj_type == PJ_TYPE_GEOGRAPHIC_3D_CRS);
#endif
}

static int
srid_is_latlong(FunctionCallInfo fcinfo, int32_t srid)
{
	PJ* pj;
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
	PJ* pj;
#if POSTGIS_PROJ_VERSION >= 60
	double out_semi_major_metre, out_semi_minor_metre, out_inv_flattening;
	int out_is_semi_minor_computed;
	PJ *pj_ellps, *pj_crs;
#elif POSTGIS_PROJ_VERSION >= 48
	double major_axis, minor_axis, eccentricity_squared;
#endif

	if ( GetPJUsingFCInfo(fcinfo, srid, srid, &pj) == LW_FAILURE)
		return LW_FAILURE;

#if POSTGIS_PROJ_VERSION >= 60
	if (!proj_pj_is_latlong(pj))
		return LW_FAILURE;
	pj_crs = proj_get_source_crs(NULL, pj);
	if (!pj_crs)
	{
		lwerror("%s: proj_get_source_crs returned NULL", __func__);
	}
	pj_ellps = proj_get_ellipsoid(NULL, pj_crs);
	if (!pj_ellps)
	{
		proj_destroy(pj_crs);
		lwerror("%s: proj_get_ellipsoid returned NULL", __func__);
	}
	proj_ellipsoid_get_parameters(NULL, pj_ellps,
		&out_semi_major_metre, &out_semi_minor_metre,
		&out_is_semi_minor_computed, &out_inv_flattening);
	proj_destroy(pj_ellps);
	proj_destroy(pj_crs);
	spheroid_init(s, out_semi_major_metre, out_semi_minor_metre);

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
