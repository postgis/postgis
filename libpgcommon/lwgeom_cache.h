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

#ifndef LWGEOM_CACHE_H_
#define LWGEOM_CACHE_H_ 1

#include "postgres.h"
#include "fmgr.h"

#include "liblwgeom.h"
#include "lwgeodetic_tree.h"
#include "lwgeom_pg.h"


#define PROJ_CACHE_ENTRY 0
#define PREP_CACHE_ENTRY 1
#define RTREE_CACHE_ENTRY 2
#define CIRC_CACHE_ENTRY 3
#define RECT_CACHE_ENTRY 4
#define TOAST_CACHE_ENTRY 5
#define SRSDESC_CACHE_ENTRY 6
#define SRID_CACHE_ENTRY 7

#define NUM_CACHE_ENTRIES 8

/* Returns the MemoryContext used to store the caches */
MemoryContext PostgisCacheContext(FunctionCallInfo fcinfo);

typedef struct {
	uint32_t count; /* For PgSQL use only, use VAR* macros to manipulate. */
	GSERIALIZED *geom;
} SHARED_GSERIALIZED;

/*
* A generic GeomCache just needs space for the cache type,
* the cache keys (GSERIALIZED geometries), the key sizes,
* and the argument number the cached index/tree is going
* to refer to.
*/
typedef struct {
	int                         type;
	SHARED_GSERIALIZED shared_geom1[1];
	SHARED_GSERIALIZED shared_geom2[1];
	size_t                      geom1_size;
	size_t                      geom2_size;
	LWGEOM*                     lwgeom1;
	LWGEOM*                     lwgeom2;
	int32                       argnum;
} GeomCache;

/*
* Other specific geometry cache types are the
* RTreeGeomCache - lwgeom_rtree.h
* PrepGeomCache - lwgeom_geos_prepared.h
*/

/*
* Proj4 caching has it's own mechanism, but is
* integrated into the generic caching system because
* some geography functions make cached SRID lookup
* calls and also CircTree accelerated calls, so
* there needs to be a management object on
* fcinfo->flinfo->fn_extra to avoid collisions.
*/

/* An entry in the PROJ SRS cache */
typedef struct struct_PROJSRSCacheItem
{
	int32_t srid_from;
	int32_t srid_to;
	uint64_t hits;
	LWPROJ *projection;
}
PROJSRSCacheItem;

/* PROJ 4 lookup transaction cache methods */
#define PROJ_CACHE_ITEMS 128

/*
* The proj4 cache holds a fixed number of reprojection
* entries. In normal usage we don't expect it to have
* many entries, so we always linearly scan the list.
*/
typedef struct struct_PROJPortalCache
{
	int type;
	PROJSRSCacheItem PROJSRSCache[PROJ_CACHE_ITEMS];
	uint32_t PROJSRSCacheCount;
	MemoryContext PROJSRSCacheContext;
}
PROJPortalCache;

/**
* Generic signature for functions to manage a geometry
* cache structure.
*/
typedef struct
{
	int entry_number; /* What kind of structure is this? */
	int (*GeomIndexBuilder)(const LWGEOM* lwgeom, GeomCache* cache); /* Build an index/tree and add it to your cache */
	int (*GeomIndexFreer)(GeomCache* cache); /* Free the index/tree in your cache */
	GeomCache* (*GeomCacheAllocator)(void); /* Allocate the kind of cache object you use (GeomCache+some extra space) */
} GeomCacheMethods;

/*
* Cache retrieval functions
*/
PROJPortalCache *GetPROJSRSCache(FunctionCallInfo fcinfo);
GeomCache *GetGeomCache(FunctionCallInfo fcinfo,
			const GeomCacheMethods *cache_methods,
			const GSERIALIZED *g1,
			const GSERIALIZED *g2);

/******************************************************************************/

#define ToastCacheSize 2

typedef struct
{
	Oid valueid;
	Oid toastrelid;
	SHARED_GSERIALIZED shared_geom[1];
} ToastCacheArgument;

typedef struct
{
	int type;
	ToastCacheArgument arg[ToastCacheSize];
} ToastCache;

GSERIALIZED* ToastCacheGetGeometry(FunctionCallInfo fcinfo, uint32_t argnum);

/******************************************************************************/

#define SRSDescCacheSize 1
typedef struct {
	int32_t srid;
	bool short_mode;
	char *srs;
} SRSDescCacheArgument;

typedef struct {
	int type;
	SRSDescCacheArgument arg[SRSDescCacheSize];
} SRSDescCache;

const char *GetSRSCacheBySRID(FunctionCallInfo fcinfo, int32_t srid, bool short_crs);

/******************************************************************************/

#define SRIDCacheSize 1
typedef struct {
	char *srs;
	int32_t srid;
} SRIDCacheArgument;

typedef struct {
	int type;
	SRIDCacheArgument arg[SRIDCacheSize];
} SRIDCache;

int32_t GetSRIDCacheBySRS(FunctionCallInfo fcinfo, const char *srs);

#endif /* LWGEOM_CACHE_H_ */
