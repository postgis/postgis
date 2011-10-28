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

#ifndef LWGEOM_GEOS_PREPARED_H_
#define LWGEOM_GEOS_PREPARED_H_ 1

#include "postgres.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "utils/hsearch.h"
#include "utils/memutils.h"
#include "access/hash.h"

#include "../postgis_config.h"

#include "lwgeom_pg.h"
#include "liblwgeom.h"
#include "lwgeom_geos.h"

/*
** Cache structure. We use GSERIALIZED as keys so no transformations
** are needed before we memcmp them with other keys. We store the
** size to avoid having to calculate the size every time.
** The argnum gives the number of function arguments we are caching.
** Intersects requires that both arguments be checked for cacheability,
** while Contains only requires that the containing argument be checked.
** Both the Geometry and the PreparedGeometry have to be cached,
** because the PreparedGeometry contains a reference to the geometry.
*/
typedef struct
{
	char                          type;
	GSERIALIZED                     *pg_geom1;
	GSERIALIZED                     *pg_geom2;
	size_t                        pg_geom1_size;
	size_t                        pg_geom2_size;
	int32                         argnum;
	const GEOSPreparedGeometry    *prepared_geom;
	const GEOSGeometry                  *geom;
	MemoryContext                 context;
}
PrepGeomCache;

/*
** Get the current cache, given the input geometries.
** Function will create cache if none exists, and prepare geometries in
** cache if necessary, or pull an existing cache if possible.
**
** If you are only caching one argument (e.g., in contains) supply 0 as the
** value for pg_geom2.
*/
PrepGeomCache *GetPrepGeomCache(FunctionCallInfoData *fcinfo, GSERIALIZED *pg_geom1, GSERIALIZED *pg_geom2);

#endif /* LWGEOM_GEOS_PREPARED_H_ 1 */
