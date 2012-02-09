/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 *
 * Copyright 2008 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#ifndef LWGEOM_GEOS_H_
#define LWGEOM_GEOS_H_ 1

#include "../liblwgeom/lwgeom_geos.h" /* for GEOSGeom */
#include "liblwgeom.h" /* for GSERIALIZED */

/*
** Public prototypes for GEOS utility functions.
*/

GSERIALIZED *GEOS2POSTGIS(GEOSGeom geom, char want3d);
GEOSGeometry * POSTGIS2GEOS(GSERIALIZED *g);


void errorIfGeometryCollection(GSERIALIZED *g1, GSERIALIZED *g2);

#endif /* LWGEOM_GEOS_H_ 1 */
