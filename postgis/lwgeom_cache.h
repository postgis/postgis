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

#ifndef LWGEOM_GEOS_CACHE_H_
#define LWGEOM_GEOS_CACHE_H_ 1

#include "postgres.h"
#include "fmgr.h"

#include "lwgeom_pg.h"
#include "lwgeom_rtree.h"
#include "lwgeom_geos_prepared.h"

typedef struct {
	PrepGeomCache* prep;
	RTREE_POLY_CACHE* rtree;
} GeomCache;

GeomCache* GetGeomCache(FunctionCallInfoData *fcinfo);

#endif /* LWGEOM_GEOS_CACHE_H_ 1 */
