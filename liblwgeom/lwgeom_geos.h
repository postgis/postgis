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
 * Copyright 2011 Sandro Santilli <strk@keybit.net>
 *
 **********************************************************************/


/* Workaround for GEOS 2.2 compatibility: old geos_c.h does not contain
   header guards to protect from multiple inclusion */
#ifndef GEOS_C_INCLUDED
#define GEOS_C_INCLUDED
#include "geos_c.h"
#endif

#include "liblwgeom.h"
#include "lwunionfind.h"

/*
** Public prototypes for GEOS utility functions.
*/
LWGEOM *GEOS2LWGEOM(const GEOSGeometry *geom, char want3d);
GEOSGeometry * LWGEOM2GEOS(const LWGEOM *g, int autofix);
GEOSGeometry * LWGEOM2GEOS_PREC(const LWGEOM *g, int autofix, const double* precision);
GEOSGeometry * GBOX2GEOS(const GBOX *g);
GEOSGeometry * LWGEOM_GEOS_buildArea(const GEOSGeometry* geom_in);

int cluster_intersecting(GEOSGeometry** geoms, uint32_t num_geoms, GEOSGeometry*** clusterGeoms, uint32_t* num_clusters);
int cluster_within_distance(LWGEOM** geoms, uint32_t num_geoms, double tolerance, LWGEOM*** clusterGeoms, uint32_t* num_clusters);
int cluster_dbscan(LWGEOM** geoms, uint32_t num_geoms, double eps, uint32_t min_points, LWGEOM*** clusterGeoms, uint32_t* num_clusters);
int union_dbscan(LWGEOM** geoms, uint32_t num_geoms, UNIONFIND* uf, double eps, uint32_t min_points);

POINTARRAY *ptarray_from_GEOSCoordSeq(const GEOSCoordSequence *cs, char want3d);


extern char lwgeom_geos_errmsg[];
extern void lwgeom_geos_error(const char *fmt, ...);

