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
 * Copyright (C) 2011 Sandro Santilli <strk@kbt.io>
 *
 **********************************************************************/


#include "lwgeom_geos.h"
#include "liblwgeom_internal.h"

#include <string.h>
#include <assert.h>

static int
lwgeom_ngeoms(const LWGEOM* n)
{
	const LWCOLLECTION* c = lwgeom_as_lwcollection(n);
	if ( c ) return c->ngeoms;
	else return 1;
}

static const LWGEOM*
lwgeom_subgeom(const LWGEOM* g, int n)
{
	const LWCOLLECTION* c = lwgeom_as_lwcollection(g);
	if ( c ) return lwcollection_getsubgeom((LWCOLLECTION*)c, n);
	else return g;
}


static void
lwgeom_collect_endpoints(const LWGEOM* lwg, LWMPOINT* col)
{
	int i, n;
	LWLINE* l;

	switch (lwg->type)
	{
		case MULTILINETYPE:
			for ( i = 0,
			        n = lwgeom_ngeoms(lwg);
			      i < n; ++i )
			{
				lwgeom_collect_endpoints(
					lwgeom_subgeom(lwg, i),
					col);
			}
			break;
		case LINETYPE:
			l = (LWLINE*)lwg;
			col = lwmpoint_add_lwpoint(col,
				lwline_get_lwpoint(l, 0));
			col = lwmpoint_add_lwpoint(col,
				lwline_get_lwpoint(l, l->points->npoints-1));
			break;
		default:
			lwerror("lwgeom_collect_endpoints: invalid type %s",
				lwtype_name(lwg->type));
			break;
	}
}

static LWMPOINT*
lwgeom_extract_endpoints(const LWGEOM* lwg)
{
	LWMPOINT* col = lwmpoint_construct_empty(SRID_UNKNOWN,
	                              FLAGS_GET_Z(lwg->flags),
	                              FLAGS_GET_M(lwg->flags));
	lwgeom_collect_endpoints(lwg, col);

	return col;
}

/* exported */
extern LWGEOM* lwgeom_node(const LWGEOM* lwgeom_in);
LWGEOM*
lwgeom_node(const LWGEOM* lwgeom_in)
{
	GEOSGeometry *g1, *gn, *gm;
	LWGEOM *lines;

	if ( lwgeom_dimension(lwgeom_in) != 1 ) {
		lwerror("Noding geometries of dimension != 1 is unsupported");
		return NULL;
	}

	initGEOS(lwgeom_geos_error, lwgeom_geos_error);
	g1 = LWGEOM2GEOS(lwgeom_in, 1);
	if ( ! g1 ) {
		lwerror("LWGEOM2GEOS: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	gn = GEOSNode(g1);
	GEOSGeom_destroy(g1);
	if ( ! gn ) {
		lwerror("GEOSNode: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	/* Linemerge (in case of overlaps) */
	gm = GEOSLineMerge(gn);
	GEOSGeom_destroy(gn);
	if ( ! gm ) {
		lwerror("GEOSLineMerge: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	lines = GEOS2LWGEOM(gm, FLAGS_GET_Z(lwgeom_in->flags));
	GEOSGeom_destroy(gm);
	if ( ! lines ) {
		lwerror("Error during GEOS2LWGEOM");
		return NULL;
	}

	lines->srid = lwgeom_in->srid;
	return (LWGEOM*)lines;
}

