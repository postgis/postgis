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
 * Copyright 2008 Paul Ramsey <pramsey@cleverelephant.ca>
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

#include "lwgeom_pg.h"
#include "liblwgeom.h"
#include "lwgeom_geos.h"
#include "lwgeom_cache.h"

/*
* Cache structure. We use GSERIALIZED as keys so no transformations
* are needed before we memcmp them with other keys. We store the
* size to avoid having to calculate the size every time.
* The argnum gives the number of function arguments we are caching.
* Intersects requires that both arguments be checked for cacheability,
* while Contains only requires that the containing argument be checked.
* Both the Geometry and the PreparedGeometry have to be cached,
* because the PreparedGeometry contains a reference to the geometry.
*
* Note that the first 6 entries are part of the common GeomCache
* structure and have to remain in order to allow the overall caching
* system to share code (the cache checking code is common between
* prepared geometry, circtrees, recttrees, and rtrees).
*/
typedef struct {
	GeomCache                   gcache;
	MemoryContext               context_statement;
	MemoryContext               context_callback;
	const GEOSPreparedGeometry* prepared_geom;
	const GEOSGeometry*         geom;
} PrepGeomCache;


/*
** Get the current cache, given the input geometries.
** Function will create cache if none exists, and prepare geometries in
** cache if necessary, or pull an existing cache if possible.
**
** If you are only caching one argument (e.g., in contains) supply 0 as the
** value for pg_geom2.
*/
PrepGeomCache *GetPrepGeomCache(FunctionCallInfo fcinfo, SHARED_GSERIALIZED *pg_geom1, SHARED_GSERIALIZED *pg_geom2);

#endif /* LWGEOM_GEOS_PREPARED_H_ */
