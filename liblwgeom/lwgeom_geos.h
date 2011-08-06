/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
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


LWGEOM *lwgeom_intersection(LWGEOM *geom1, LWGEOM *geom2) ;
LWGEOM *lwgeom_difference(LWGEOM *geom1, LWGEOM *geom2) ;
LWGEOM *lwgeom_symdifference(LWGEOM* geom1, LWGEOM* geom2) ;
LWGEOM* lwgeom_union(LWGEOM *geom1, LWGEOM *geom2) ;
LWGEOM* lwgeom_buildarea(LWGEOM *geom) ;



/*
** Public prototypes for GEOS utility functions.
*/
LWGEOM *GEOS2LWGEOM(const GEOSGeometry *geom, char want3d);
GEOSGeometry * LWGEOM2GEOS(LWGEOM *g);
GEOSGeometry * LWGEOM_GEOS_buildArea(const GEOSGeometry* geom_in);


POINTARRAY *ptarray_from_GEOSCoordSeq(const GEOSCoordSequence *cs, char want3d);


#define LWGEOM_GEOS_ERRMSG_MAXSIZE 256
extern char lwgeom_geos_errmsg[];
extern void lwgeom_geos_error(const char *fmt, ...);

