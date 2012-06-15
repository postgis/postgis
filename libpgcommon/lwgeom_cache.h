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

#ifndef LWGEOM_CACHE_H_
#define LWGEOM_CACHE_H_ 1

#include "postgres.h"
#include "fmgr.h"

#include "liblwgeom_internal.h"
#include "lwgeodetic.h";
#include "lwgeodetic_tree.h"

#include "lwgeom_pg.h"


#define PROJ_CACHE_ENTRY 0
#define PREP_CACHE_ENTRY 1
#define RTREE_CACHE_ENTRY 2
#define CIRC_CACHE_ENTRY 3
#define RECT_CACHE_ENTRY 4
#define NUM_CACHE_ENTRIES 5


/* 
* For the geometry-with-tree case, we need space for
* the serialized geometries and their sizes, so we can
* test for cache hits/misses. The argnum tells us which
* argument the tree is built for.
*/
typedef struct {
	int            type;
	GSERIALIZED*   geom1;
	GSERIALIZED*   geom2;
	size_t         geom1_size;
	size_t         geom2_size;
	int32          argnum;
	MemoryContext  context_statement;
	MemoryContext  context_callback;
	void*          index;
} GeomCache;


/* An entry in the PROJ4 SRS cache */
typedef struct struct_PROJ4SRSCacheItem
{
	int srid;
	projPJ projection;
	MemoryContext projection_mcxt;
}
PROJ4SRSCacheItem;

/* PROJ 4 lookup transaction cache methods */
#define PROJ4_CACHE_ITEMS	8

/*
* The proj4 cache holds a fixed number of reprojection
* entries. In normal usage we don't expect it to have
* many entries, so we always linearly scan the list.
*/
typedef struct struct_PROJ4PortalCache
{
	int type;
	PROJ4SRSCacheItem PROJ4SRSCache[PROJ4_CACHE_ITEMS];
	int PROJ4SRSCacheCount;
	MemoryContext PROJ4SRSCacheContext;
}
PROJ4PortalCache;

/*
* Generic signature for function to take a serialized 
* geometry and return a tree structure for fast edge
* access.
*/
typedef int (*GeomIndexBuilder)(const LWGEOM* lwgeom, GeomCache* cache);
typedef int (*GeomIndexFreer)(GeomCache* cache);

/* 
* Cache retrieval functions
*/
PROJ4PortalCache* GetPROJ4SRSCache(FunctionCallInfoData *fcinfo);
GeomCache*        GetGeomCache(FunctionCallInfoData *fcinfo, int cache_entry);
CIRC_NODE*        GetCircTree(FunctionCallInfoData* fcinfo, GSERIALIZED* g1, GSERIALIZED* g2, int* argnum_of_cache);

/* 
* Given candidate geometries, a builer function and an entry type, cache and/or return an
* appropriate tree.
*/
void* GetGeomIndex(FunctionCallInfoData* fcinfo, int cache_entry, GeomIndexBuilder index_build, GeomIndexFreer tree_free, const GSERIALIZED* g1, const GSERIALIZED* g2, int* argnum);


#endif /* LWGEOM_CACHE_H_ */
