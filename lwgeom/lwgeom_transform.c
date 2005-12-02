/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2003 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 * 
 **********************************************************************/

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "postgres.h"
#include "fmgr.h"

#include "liblwgeom.h"
#include "lwgeom_pg.h"

Datum transform(PG_FUNCTION_ARGS);
Datum transform_geom(PG_FUNCTION_ARGS);
Datum postgis_proj_version(PG_FUNCTION_ARGS);

// if USE_PROJECTION undefined, we get a do-nothing transform() function
#ifndef USE_PROJ

PG_FUNCTION_INFO_V1(transform);
Datum transform(PG_FUNCTION_ARGS)
{

	elog(ERROR,"PostGIS transform() called, but support not compiled in.  Modify your makefile to add proj support, remake and re-install");
	PG_RETURN_NULL();
}

PG_FUNCTION_INFO_V1(transform_geom);
Datum transform_geom(PG_FUNCTION_ARGS)
{

	elog(ERROR,"PostGIS transform_geom() called, but support not compiled in.  Modify your makefile to add proj support, remake and re-install");
	PG_RETURN_NULL();
}

PG_FUNCTION_INFO_V1(postgis_proj_version);
Datum postgis_proj_version(PG_FUNCTION_ARGS)
{
	PG_RETURN_NULL();
}

#else // defined USE_PROJ 

#include "projects.h"
#include "utils/memutils.h"
#include "executor/spi.h"
#include "access/hash.h"
#include "utils/hsearch.h"

PJ *make_project(char *str1);
void to_rad(POINT2D *pt);
void to_dec(POINT2D *pt);
int pj_transform_nodatum(PJ *srcdefn, PJ *dstdefn, long point_count, int point_offset, double *x, double *y, double *z );
int transform_point(POINT2D *pt, PJ *srcdefn, PJ *dstdefn);
int lwgeom_transform_recursive(char *geom, PJ *inpj, PJ *outpj);


// PROJ4 SRS Cache debugging (uncomment to debug)
//#define PROJ4_CACHE_DEBUG 1

// PROJ 4 lookup transaction cache methods
#define PROJ4_CACHE_ITEMS	8

// PROJ 4 backend hash table initial hash size
// (since 16 is the default portal hash table size, and we would typically have 2 entries per portal
// then we shall use a default size of 32)
#define PROJ4_BACKEND_HASH_SIZE	32		


// An entry in the PROJ4 SRS cache
typedef struct struct_PROJ4SRSCacheItem
{
	int srid;
	PJ *projection;	
	MemoryContext projection_mcxt;
} PROJ4SRSCacheItem;

// The portal cache: it's contents and cache context
typedef struct struct_PROJ4PortalCache
{
	PROJ4SRSCacheItem PROJ4SRSCache[PROJ4_CACHE_ITEMS];
	int PROJ4SRSCacheCount;
	MemoryContext PROJ4SRSCacheContext;
} PROJ4PortalCache;


// Backend PJ hash table
//
// This hash table stores a key/value pair of MemoryContext/PJ * objects. Whenever we create
// a PJ * object using pj_init(), we create a separate MemoryContext as a child context of the
// current executor context. The MemoryContext/PJ * object is stored in this hash table so
// that when PROJ4SRSCacheDelete() is called during query cleanup, we can lookup the PJ * object
// based upon the MemoryContext parameter and hence pj_free() it.

static HTAB *PJHash = NULL;

typedef struct struct_PJHashEntry
{
	MemoryContext ProjectionContext;
	PJ *projection;
} PJHashEntry;


// PJ Hash API
#if USE_VERSION == 72
long mcxt_ptr_hash(void *key, int keysize);
#endif
#if USE_VERSION == 73
uint32 mcxt_ptr_hash(void *key, int keysize);
#endif
#if USE_VERSION > 73
uint32 mcxt_ptr_hash(const void *key, Size keysize);
#endif

static HTAB *CreatePJHash(void);
static void AddPJHashEntry(MemoryContext mcxt, PJ *projection);
static PJ *GetPJHashEntry(MemoryContext mcxt);
static void DeletePJHashEntry(MemoryContext mcxt);

// Cache API
bool IsInPROJ4SRSCache(PROJ4PortalCache *PROJ4Cache, int srid);
PJ *GetProjectionFromPROJ4SRSCache(PROJ4PortalCache *PROJ4Cache, int srid);
void AddToPROJ4SRSCache(PROJ4PortalCache *PROJ4Cache, int srid, int other_srid);
void DeleteFromPROJ4SRSCache(PROJ4PortalCache *PROJ4Cache, int srid);

// Memory context cache functions
static void PROJ4SRSCacheInit(MemoryContext context);
static void PROJ4SRSCacheDelete(MemoryContext context);
#ifdef MEMORY_CONTEXT_CHECKING
static void PROJ4SRSCacheCheck(MemoryContext context);
#endif


// Memory context definition must match the current version of PostgreSQL
#if USE_VERSION == 72
static MemoryContextMethods PROJ4SRSCacheContextMethods = {
	NULL,
	NULL,
 	NULL,
 	PROJ4SRSCacheInit,
 	NULL,
 	PROJ4SRSCacheDelete
#ifdef MEMORY_CONTEXT_CHECKING
 	,PROJ4SRSCacheCheck
#endif
};
#endif

#if USE_VERSION == 73 || USE_VERSION == 74
static MemoryContextMethods PROJ4SRSCacheContextMethods = {
	NULL,
	NULL,
	NULL,
	PROJ4SRSCacheInit,
	NULL,
	PROJ4SRSCacheDelete,
	NULL,
	NULL
#ifdef MEMORY_CONTEXT_CHECKING
 	,PROJ4SRSCacheCheck
#endif
};
#endif
 
#if USE_VERSION >= 80
static MemoryContextMethods PROJ4SRSCacheContextMethods = {
	NULL,
	NULL,
	NULL,
	PROJ4SRSCacheInit,
	NULL,
	PROJ4SRSCacheDelete,
	NULL,
	NULL,
	NULL
#ifdef MEMORY_CONTEXT_CHECKING
	,PROJ4SRSCacheCheck
#endif
};
#endif


static void
PROJ4SRSCacheInit(MemoryContext context)
{
	// Do nothing as the cache is initialised when the transform() function is first called
}

static void
PROJ4SRSCacheDelete(MemoryContext context)
{
	PJ *projection;

	// Lookup the PJ pointer in the global hash table so we can free it
	projection = GetPJHashEntry(context);

	if (!projection)
		elog(ERROR, "PROJ4SRSCacheDelete: Trying to delete non-existant projection object with MemoryContext key (%p)", context);

#if PROJ4_CACHE_DEBUG
	elog(NOTICE, "PROJ4SRSCacheDelete: deleting projection object (%p) with MemoryContext key (%p)", projection, context);
#endif

	// Free it
	pj_free(projection);	

	// Remove the hash entry as it is no longer needed
	DeletePJHashEntry(context);
}

#ifdef MEMORY_CONTEXT_CHECKING
static void
PROJ4SRSCacheCheck(MemoryContext context)
{
	// Do nothing - stub required for when PostgreSQL is compiled with MEMORY_CONTEXT_CHECKING defined
}
#endif


//
// PROJ4 PJ Hash Table functions
//


// A version of tag_hash - we specify this here as the implementation has changed over
// the years....

#if USE_VERSION == 72
long mcxt_ptr_hash(void *key, int keysize)
#endif
#if USE_VERSION == 73
uint32 mcxt_ptr_hash(void *key, int keysize)
#endif
#if USE_VERSION > 73
uint32 mcxt_ptr_hash(const void *key, Size keysize)
#endif
{
	uint32 hashval;

	//hashval = DatumGetUInt32(hash_any((const unsigned char *) key, (int) keysize));
	hashval = DatumGetUInt32(hash_any(key, keysize));

	return hashval;
}


static HTAB *CreatePJHash(void)
{
	HASHCTL ctl;

	ctl.keysize = sizeof(MemoryContext);
	ctl.entrysize = sizeof(PJHashEntry);
	ctl.hash = mcxt_ptr_hash;

	return hash_create("PROJ4 Backend PJ MemoryContext Hash", PROJ4_BACKEND_HASH_SIZE, &ctl, (HASH_ELEM | HASH_FUNCTION));
}

static void AddPJHashEntry(MemoryContext mcxt, PJ *projection)
{
	bool found;
	void **key;
	PJHashEntry *he;

	// The hash key is the MemoryContext pointer
	key = (void *)&mcxt;
	
	he = (PJHashEntry *) hash_search(PJHash, key, HASH_ENTER, &found);
	if (!found)
	{
		// Insert the entry into the new hash element
		he->ProjectionContext = mcxt;
		he->projection = projection;
	}
	else
	{
		elog(ERROR, "AddPJHashEntry: PROJ4 projection object already exists for this MemoryContext (%p)", mcxt);
	}
}

static PJ *GetPJHashEntry(MemoryContext mcxt)
{
	void **key;
	PJHashEntry *he;

	// The hash key is the MemoryContext pointer
	key = (void *)&mcxt;

	// Return the projection object from the hash
	he = (PJHashEntry *) hash_search(PJHash, key, HASH_FIND, NULL);

	return he->projection;
}


static void DeletePJHashEntry(MemoryContext mcxt)
{
	void **key;
	PJHashEntry *he;	

	// The hash key is the MemoryContext pointer
	key = (void *)&mcxt;

	// Delete the projection object from the hash
	he = (PJHashEntry *) hash_search(PJHash, key, HASH_REMOVE, NULL);

	he->projection = NULL;

	if (!he)
		elog(ERROR, "DeletePJHashEntry: There was an error removing the PROJ4 projection object from this MemoryContext (%p)", mcxt);
}


//
// Per-cache management functions
//

bool IsInPROJ4SRSCache(PROJ4PortalCache *PROJ4Cache, int srid)
{
	//
	// Return true/false depending upon whether the item is in the SRS cache
	//
	
	int i;

	for (i = 0; i < PROJ4_CACHE_ITEMS; i++)
	{
		if (PROJ4Cache->PROJ4SRSCache[i].srid == srid)
			return true;
	}

	// Otherwise not found
	return false;
}


PJ *GetProjectionFromPROJ4SRSCache(PROJ4PortalCache *PROJ4Cache, int srid)
{
	//
	// Return the projection object from the cache (we should already have checked it exists
	// using IsInPROJ4SRSCache first)
	//

	int i;

	for (i = 0; i < PROJ4_CACHE_ITEMS; i++)
	{
		if (PROJ4Cache->PROJ4SRSCache[i].srid == srid)
			return PROJ4Cache->PROJ4SRSCache[i].projection;
	}

	return NULL;
}


void AddToPROJ4SRSCache(PROJ4PortalCache *PROJ4Cache, int srid, int other_srid)
{
	//
        // Add an entry to the local PROJ4 SRS cache. If we need to wrap around then
	// we must make sure the entry we choose to delete does not contain other_srid
	// which is the definition for the other half of the transformation.
	//        
	
	MemoryContext PJMemoryContext;
	int spi_result;
	PJ *projection = NULL;
	char *proj_str;
	char proj4_spi_buffer[256];

	// Connect
	spi_result = SPI_connect();
        if (spi_result != SPI_OK_CONNECT)
        {
                elog(ERROR, "AddToPROJ4SRSCache: Could not connect to database using SPI");
        }

        // Execute the lookup query
        snprintf(proj4_spi_buffer, 255, "SELECT proj4text FROM spatial_ref_sys WHERE srid = %d LIMIT 1", srid);
        spi_result = SPI_exec(proj4_spi_buffer, 1);

        // Read back the PROJ4 text
        if (spi_result == SPI_OK_SELECT && SPI_processed > 0)
        {
                // Select the first (and only tuple)
                TupleDesc tupdesc = SPI_tuptable->tupdesc;
                SPITupleTable *tuptable = SPI_tuptable;
                HeapTuple tuple = tuptable->vals[0];

		// Make a projection object out of it
		proj_str = palloc(strlen(SPI_getvalue(tuple, tupdesc, 1)) + 1);
		strcpy(proj_str, SPI_getvalue(tuple, tupdesc, 1));
		projection = make_project(proj_str);

		if ( (projection == NULL) || pj_errno)
		{
			//pfree(projection); // we need this for error reporting
			elog(ERROR, "AddToPROJ4SRSCache: couldn't parse proj4 string: '%s': %s", proj_str, pj_strerrno(pj_errno));
		}

		// If the cache is already full then find the first entry that doesn't contain other_srid
		// and use this as the subsequent value of PROJ4SRSCacheCount
		if (PROJ4Cache->PROJ4SRSCacheCount == PROJ4_CACHE_ITEMS)
		{
			bool found = false;
			int i;

			for (i = 0; i < PROJ4_CACHE_ITEMS; i++)
			{
				if (PROJ4Cache->PROJ4SRSCache[i].srid != other_srid && found == false)
				{
#if PROJ4_CACHE_DEBUG
					elog(NOTICE, "AddToPROJ4SRSCache: choosing to remove item from query cache with SRID %d and index %d", PROJ4Cache->PROJ4SRSCache[i].srid, i);
#endif
					DeleteFromPROJ4SRSCache(PROJ4Cache, PROJ4Cache->PROJ4SRSCache[i].srid);
					PROJ4Cache->PROJ4SRSCacheCount = i;

					found = true;
				}
			}
		}

		// Now create a memory context for this projection and store it in the backend hash
#if PROJ4_CACHE_DEBUG
		elog(NOTICE, "AddToPROJ4SRSCache: adding SRID %d with proj4text \"%s\" to query cache at index %d", srid, proj_str, PROJ4Cache->PROJ4SRSCacheCount);
#endif
		PJMemoryContext = MemoryContextCreate(T_AllocSetContext, 8192,
					&PROJ4SRSCacheContextMethods,
					PROJ4Cache->PROJ4SRSCacheContext,
					"PROJ4 PJ Memory Context");

		// Create the backend hash if it doesn't already exist
		if (!PJHash)
			PJHash = CreatePJHash();

		// Add the MemoryContext to the backend hash so we can clean up upon portal shutdown
#if PROJ4_CACHE_DEBUG
		elog(NOTICE, "AddToPROJ4SRSCache: adding projection object (%p) to hash table with MemoryContext key (%p)", projection, PJMemoryContext);
#endif
		AddPJHashEntry(PJMemoryContext, projection);
		
		PROJ4Cache->PROJ4SRSCache[PROJ4Cache->PROJ4SRSCacheCount].srid = srid;
		PROJ4Cache->PROJ4SRSCache[PROJ4Cache->PROJ4SRSCacheCount].projection = projection;
		PROJ4Cache->PROJ4SRSCache[PROJ4Cache->PROJ4SRSCacheCount].projection_mcxt = PJMemoryContext;
		PROJ4Cache->PROJ4SRSCacheCount++;

		// Free the projection string
		pfree(proj_str);
	}
        else
        {
                elog(ERROR, "AddToPROJ4SRSCache: Cannot find SRID (%d) in spatial_ref_sys", srid);
        }

        // Close the connection
        spi_result = SPI_finish();
        if (spi_result != SPI_OK_FINISH)
        {
                elog(ERROR, "AddToPROJ4SRSCache: Could not disconnect from database using SPI");
        }

}


void DeleteFromPROJ4SRSCache(PROJ4PortalCache *PROJ4Cache, int srid)
{
	//
	// Delete the SRID entry from the cache
	//

	int i;

	for (i = 0; i < PROJ4_CACHE_ITEMS; i++)
	{
		if (PROJ4Cache->PROJ4SRSCache[i].srid == srid)
		{
#if PROJ4_CACHE_DEBUG
			elog(NOTICE, "DeleteFromPROJ4SRSCache: removing query cache entry with SRID %d at index %d", srid, i);
#endif
			// Zero out the entries and free the PROJ4 handle by deleting the memory context
			MemoryContextDelete(PROJ4Cache->PROJ4SRSCache[i].projection_mcxt);
			PROJ4Cache->PROJ4SRSCache[i].projection = NULL;
			PROJ4Cache->PROJ4SRSCache[i].projection_mcxt = NULL;
			PROJ4Cache->PROJ4SRSCache[i].srid = -1;
		}
	}
}


//this is *exactly* the same as PROJ.4's pj_transform(), but it doesnt do the
// datum shift.
int
pj_transform_nodatum(PJ *srcdefn, PJ *dstdefn, long point_count,
int point_offset, double *x, double *y, double *z )
{
    long      i;
    //int       need_datum_shift;

    pj_errno = 0;

    if( point_offset == 0 )
        point_offset = 1;

    if( !srcdefn->is_latlong )
    {
        for( i = 0; i < point_count; i++ )
        {
            XY         projected_loc;
            LP	       geodetic_loc;

            projected_loc.u = x[point_offset*i];
            projected_loc.v = y[point_offset*i];

            geodetic_loc = pj_inv( projected_loc, srcdefn );
            if( pj_errno != 0 )
                return pj_errno;

            x[point_offset*i] = geodetic_loc.u;
            y[point_offset*i] = geodetic_loc.v;
        }
    }

    if( !dstdefn->is_latlong )
    {
        for( i = 0; i < point_count; i++ )
        {
            XY         projected_loc;
            LP	       geodetic_loc;

            geodetic_loc.u = x[point_offset*i];
            geodetic_loc.v = y[point_offset*i];

            projected_loc = pj_fwd( geodetic_loc, dstdefn );
            if( pj_errno != 0 )
                return pj_errno;

            x[point_offset*i] = projected_loc.u;
            y[point_offset*i] = projected_loc.v;
        }
    }

    return 0;
}



// convert decimal degress to radians
void to_rad(POINT2D *pt)
{
	pt->x *= PI/180.0;
	pt->y *= PI/180.0;
}

// convert radians to decimal degress
void to_dec(POINT2D *pt)
{
	pt->x *= 180.0/PI;
	pt->y *= 180.0/PI;
}

//given a string, make a PJ object
PJ *
make_project(char *str1)
{
	int t;
	char *params[1024];  //one for each parameter
	char *loc;
	char *str;
	PJ *result;


	if (str1 == NULL) return NULL;

	if (strlen(str1) == 0) return NULL;

	str = pstrdup(str1);

	/*
	 * first we split the string into a bunch of smaller strings,
	 * based on the " " separator
	 */

	params[0] = str; // 1st param, we'll null terminate at the " " soon

	loc = str;
	t = 1;
	while  ((loc != NULL) && (*loc != 0) )
	{
		loc = strchr(loc, ' ');
		if (loc != NULL)
		{
			*loc = 0; // null terminate
			params[t] = loc+1;
			loc++; // next char
			t++; //next param
		}
	}

	if (!(result=pj_init(t, params)))
	{
		pfree(str);
		return NULL;
	}
	pfree(str);
	return result;
}

/*
 * Transform given SERIALIZED geometry
 * from inpj projection to outpj projection
 */
int
lwgeom_transform_recursive(char *geom, PJ *inpj, PJ *outpj)
{
	LWGEOM_INSPECTED *inspected = lwgeom_inspect(geom);
	int j, i;

	for (j=0; j<inspected->ngeometries; j++)
	{
		LWLINE *line=NULL;
		LWPOINT *point=NULL;
		LWPOLY *poly=NULL;
		POINT2D p;
		char *subgeom=NULL;

		point = lwgeom_getpoint_inspected(inspected,j);
		if (point != NULL)
		{
			getPoint2d_p(point->point, 0, &p);
			transform_point(&p, inpj, outpj);
			memcpy(getPoint_internal(point->point, 0),
				&p, sizeof(POINT2D));
			lwgeom_release((LWGEOM *)point);
			continue;
		}

		line = lwgeom_getline_inspected(inspected, j);
		if (line != NULL)
		{
			POINTARRAY *pts = line->points;
			for (i=0; i<pts->npoints; i++)
			{
				getPoint2d_p(pts, i, &p);
				transform_point(&p, inpj, outpj);
				memcpy(getPoint_internal(pts, i),
					&p, sizeof(POINT2D));
			}
			lwgeom_release((LWGEOM *)line);
			continue;
		}

		poly = lwgeom_getpoly_inspected(inspected, j);
		if (poly !=NULL)
		{
			for (i=0; i<poly->nrings;i++)
			{
				int pi;
				POINTARRAY *pts = poly->rings[i];
				for (pi=0; pi<pts->npoints; pi++)
				{
					getPoint2d_p(pts, pi, &p);
					transform_point(&p, inpj, outpj);
					memcpy(getPoint_internal(pts, pi),
						&p, sizeof(POINT2D));
				}
			}
			lwgeom_release((LWGEOM *)poly);
			continue;
		}

		subgeom = lwgeom_getsubgeometry_inspected(inspected, j);
		if ( subgeom != NULL )
		{
			if (!lwgeom_transform_recursive(subgeom, inpj, outpj))
			{
				pfree_inspected(inspected);
				return 0;
			}
			continue;
		}
		else
		{
			pfree_inspected(inspected);
			lwerror("lwgeom_getsubgeometry_inspected returned NULL");
			return 0;
		}
	}

	pfree_inspected(inspected);
	return 1;
}


//transform( GEOMETRY, INT (output srid) )
// tmpPts - if there is a nadgrid error (-38), we re-try the transform on a copy of points.  The transformed points
//          are in an indeterminate state after the -38 error is thrown.
PG_FUNCTION_INFO_V1(transform);
Datum transform(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	PG_LWGEOM *result=NULL;
	LWGEOM *lwgeom;
	PJ *input_pj,*output_pj;
	int32 result_srid ;
	char *srl;

	PROJ4PortalCache *PROJ4Cache = NULL;


	result_srid = PG_GETARG_INT32(1);
	if (result_srid == -1)
	{
		elog(ERROR,"-1 is an invalid target SRID");
		PG_RETURN_NULL();
	}

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));
	if (pglwgeom_getSRID(geom) == -1)
	{
		pfree(geom);
		elog(ERROR,"Input geometry has unknown (-1) SRID");
		PG_RETURN_NULL();
	}

	// If we have not already created PROJ4 cache for this portal then create it
	if (fcinfo->flinfo->fn_extra == NULL)
	{
		MemoryContext old_context;
		
		old_context = MemoryContextSwitchTo(fcinfo->flinfo->fn_mcxt);
		PROJ4Cache = palloc(sizeof(PROJ4PortalCache));		
		MemoryContextSwitchTo(old_context);

		if (PROJ4Cache)
		{
			int i;

#if PROJ4_CACHE_DEBUG
			elog(NOTICE, "Allocating PROJ4Cache for portal with transform() MemoryContext %p", fcinfo->flinfo->fn_mcxt);
#endif
			// Put in any required defaults
			for (i = 0; i < PROJ4_CACHE_ITEMS; i++)
			{
				PROJ4Cache->PROJ4SRSCache[i].srid = -1;
				PROJ4Cache->PROJ4SRSCache[i].projection = NULL;
				PROJ4Cache->PROJ4SRSCache[i].projection_mcxt = NULL;
			}
			PROJ4Cache->PROJ4SRSCacheCount = 0;
			PROJ4Cache->PROJ4SRSCacheContext = fcinfo->flinfo->fn_mcxt;

			// Store the pointer in fcinfo->flinfo->fn_extra
			fcinfo->flinfo->fn_extra = PROJ4Cache;
		}
	}
	else
	{
		// Use the existing cache
		PROJ4Cache = fcinfo->flinfo->fn_extra;
	}

	// Add the output srid to the cache if it's not already there
	if (!IsInPROJ4SRSCache(PROJ4Cache, result_srid))
		AddToPROJ4SRSCache(PROJ4Cache, result_srid, pglwgeom_getSRID(geom));

	// Get the output projection
	output_pj = GetProjectionFromPROJ4SRSCache(PROJ4Cache, result_srid);

	// Add the input srid to the cache if it's not already there
	if (!IsInPROJ4SRSCache(PROJ4Cache, pglwgeom_getSRID(geom)))
		AddToPROJ4SRSCache(PROJ4Cache, pglwgeom_getSRID(geom), result_srid);
	
	// Get the input projection	
	input_pj = GetProjectionFromPROJ4SRSCache(PROJ4Cache, pglwgeom_getSRID(geom));

	
	// now we have a geometry, and input/output PJ structs.
	lwgeom_transform_recursive(SERIALIZED_FORM(geom),
		input_pj, output_pj);

	srl = SERIALIZED_FORM(geom);

	// Re-compute bbox if input had one (COMPUTE_BBOX TAINTING)
	if ( TYPE_HASBBOX(geom->type) )
	{
		lwgeom = lwgeom_deserialize(srl);
		lwgeom_dropBBOX(lwgeom);
		lwgeom->bbox = lwgeom_compute_box2d(lwgeom);
		lwgeom->SRID = result_srid;
		result = pglwgeom_serialize(lwgeom);
		lwgeom_release(lwgeom);
	}
	else
	{
		result = PG_LWGEOM_construct(srl, result_srid, 0);
	}

	pfree(geom);

	PG_RETURN_POINTER(result); // new geometry
}


//transform_geom( GEOMETRY, TEXT (input proj4), TEXT (output proj4), INT (output srid)
// tmpPts - if there is a nadgrid error (-38), we re-try the transform on a copy of points.  The transformed points
//          are in an indeterminate state after the -38 error is thrown.
PG_FUNCTION_INFO_V1(transform_geom);
Datum transform_geom(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	PG_LWGEOM *result=NULL;
	LWGEOM *lwgeom;
	PJ *input_pj,*output_pj;
	char *input_proj4, *output_proj4;
	text *input_proj4_text;
	text *output_proj4_text;
	int32 result_srid ;
	char *srl;

	result_srid = PG_GETARG_INT32(3);
	if (result_srid == -1)
	{
		elog(ERROR,"tranform: destination SRID = -1");
		PG_RETURN_NULL();
	}

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));
	if (pglwgeom_getSRID(geom) == -1)
	{
		pfree(geom);
		elog(ERROR,"tranform: source SRID = -1");
		PG_RETURN_NULL();
	}

	input_proj4_text  = (PG_GETARG_TEXT_P(1));
	output_proj4_text = (PG_GETARG_TEXT_P(2));

	input_proj4 = (char *)palloc(input_proj4_text->vl_len+1-4);
	memcpy(input_proj4, input_proj4_text->vl_dat,
		input_proj4_text->vl_len-4);
	input_proj4[input_proj4_text->vl_len-4] = 0; //null terminate

	output_proj4 = (char *) palloc(output_proj4_text->vl_len +1-4);
	memcpy(output_proj4, output_proj4_text->vl_dat,
		output_proj4_text->vl_len-4);
	output_proj4[output_proj4_text->vl_len-4] = 0; //null terminate

	//make input and output projection objects
	input_pj = make_project(input_proj4);
	if ( (input_pj == NULL) || pj_errno)
	{
		//pfree(input_proj4); // we need this for error reporting
		pfree(output_proj4);
		pfree(geom);
		elog(ERROR, "transform: couldn't parse proj4 input string: '%s': %s", input_proj4, pj_strerrno(pj_errno));
		PG_RETURN_NULL();
	}
	pfree(input_proj4);

	output_pj = make_project(output_proj4);
	if ((output_pj == NULL)|| pj_errno)
	{
		//pfree(output_proj4); // we need this for error reporting
		pj_free(input_pj);
		pfree(geom);
		elog(ERROR, "transform: couldn't parse proj4 output string: '%s': %s", output_proj4, pj_strerrno(pj_errno));
		PG_RETURN_NULL();
	}
	pfree(output_proj4);

	/* now we have a geometry, and input/output PJ structs. */
	lwgeom_transform_recursive(SERIALIZED_FORM(geom),
		input_pj, output_pj);

	/* clean up */
	pj_free(input_pj);
	pj_free(output_pj);

	srl = SERIALIZED_FORM(geom);

	/* Re-compute bbox if input had one (COMPUTE_BBOX TAINTING) */
	if ( TYPE_HASBBOX(geom->type) )
	{
		lwgeom = lwgeom_deserialize(srl);
		lwgeom_dropBBOX(lwgeom);
		lwgeom->bbox = lwgeom_compute_box2d(lwgeom);
		lwgeom->SRID = result_srid;
		result = pglwgeom_serialize(lwgeom);
		lwgeom_release(lwgeom);
	}
	else
	{
		result = PG_LWGEOM_construct(srl, result_srid, 0);
	}

	pfree(geom);

	PG_RETURN_POINTER(result); // new geometry
}


PG_FUNCTION_INFO_V1(postgis_proj_version);
Datum postgis_proj_version(PG_FUNCTION_ARGS)
{
	const char *ver = pj_release;
	text *result;
	result = (text *) palloc(VARHDRSZ  + strlen(ver));
	VARATT_SIZEP(result) = VARHDRSZ + strlen(ver) ;
	memcpy(VARDATA(result), ver, strlen(ver));
	PG_RETURN_POINTER(result);
}


int
transform_point(POINT2D *pt, PJ *srcpj, PJ *dstpj)
{
	if (srcpj->is_latlong) to_rad(pt);
	pj_transform(srcpj, dstpj, 1, 2, &(pt->x), &(pt->y), NULL);
	if (pj_errno)
	{
		if (pj_errno == -38)  //2nd chance
		{
			//couldnt do nadshift - do it without the datum
			pj_transform_nodatum(srcpj, dstpj, 1, 2,
				&(pt->x), &(pt->y), NULL);
		}

		if (pj_errno)
		{
			elog(ERROR,"transform: couldn't project point: %i (%s)",
				pj_errno,pj_strerrno(pj_errno));
			return 0;
		}
	}

	if (dstpj->is_latlong) to_dec(pt);
	return 1;
}


#endif

