/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright 2011 Sandro Santilli <strk@keybit.net>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

/* Workaround for GEOS 2.2 compatibility: old geos_c.h does not contain
   header guards to protect from multiple inclusion */
#ifndef GEOS_C_INCLUDED
#define GEOS_C_INCLUDED
#include "geos_c.h"
#endif

#include "liblwgeom.h"


/*
** Public prototypes for GEOS utility functions.
*/
LWGEOM *GEOS2LWGEOM(const GEOSGeometry *geom, char want3d);
GEOSGeometry * LWGEOM2GEOS(const LWGEOM *g, int autofix);
GEOSGeometry * GBOX2GEOS(const GBOX *g);
GEOSGeometry * LWGEOM_GEOS_buildArea(const GEOSGeometry* geom_in);

int cluster_intersecting(GEOSGeometry** geoms, uint32_t num_geoms, GEOSGeometry*** clusterGeoms, uint32_t* num_clusters);
int cluster_within_distance(LWGEOM** geoms, uint32_t num_geoms, double tolerance, LWGEOM*** clusterGeoms, uint32_t* num_clusters);

POINTARRAY *ptarray_from_GEOSCoordSeq(const GEOSCoordSequence *cs, char want3d);


extern char lwgeom_geos_errmsg[];
extern void lwgeom_geos_error(const char *fmt, ...);

