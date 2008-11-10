/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2008 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 * 
 **********************************************************************/

#include "../postgis_config.h"

#include "postgres.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "utils/hsearch.h"
#include "utils/memutils.h"
#include "access/hash.h"

/* Workaround for GEOS 2.2 compatibility: old geos_c.h does not contain
   header guards to protect from multiple inclusion */
#ifndef GEOS_C_INCLUDED
#define GEOS_C_INCLUDED
#include "geos_c.h"
#endif

#include "lwgeom_pg.h"
#include "liblwgeom.h"
#include "lwgeom_geos.h"

/*
** GEOS prepared geometry is only available from GEOS 3.1 onwards
*/
#if POSTGIS_GEOS_VERSION >= 31
#define PREPARED_GEOM 
#endif

/* 
** Cache structure. We use PG_LWGEOM as keys so no transformations
** are needed before we memcmp them with other keys. We store the 
** size to avoid having to calculate the size every time.
** The argnum gives the number of function arguments we are caching. 
** Intersects requires that both arguments be checked for cacheability, 
** while Contains only requires that the containing argument be checked. 
** Both the Geometry and the PreparedGeometry have to be cached, 
** because the PreparedGeometry contains a reference to the geometry.
*/
#ifdef PREPARED_GEOM
typedef struct
{
	char                          type;
	PG_LWGEOM*                    pg_geom1;
	PG_LWGEOM*                    pg_geom2;
	size_t                        pg_geom1_size;
	size_t                        pg_geom2_size;
	int32                         argnum;
	const GEOSPreparedGeometry*   prepared_geom;
	GEOSGeometry*                 geom;
	MemoryContext                 context;
} PrepGeomCache;

/* 
** Get the current cache, given the input geometries.
** Function will create cache if none exists, and prepare geometries in
** cache if necessary, or pull an existing cache if possible.
**
** If you are only caching one argument (e.g., in contains) supply 0 as the
** value for pg_geom2.
*/
PrepGeomCache *GetPrepGeomCache(FunctionCallInfoData *fcinfo, PG_LWGEOM *pg_geom1, PG_LWGEOM *pg_geom2);


#endif /* PREPARED_GEOM */

