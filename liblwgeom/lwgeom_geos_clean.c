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
 * Copyright 2009-2010 Sandro Santilli <strk@kbt.io>
 *
 **********************************************************************/

#include "../postgis_config.h"

#include "liblwgeom.h"
#include "lwgeom_geos.h"
#include "liblwgeom_internal.h"
#include "lwgeom_log.h"
#include "optionlist.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

LWGEOM* lwcollection_make_geos_friendly(LWCOLLECTION* g);
LWGEOM* lwline_make_geos_friendly(LWLINE* line);
LWGEOM* lwpoly_make_geos_friendly(LWPOLY* poly);
POINTARRAY* ring_make_geos_friendly(POINTARRAY* ring);

static void
ptarray_strip_nan_coords_in_place(POINTARRAY *pa)
{
	uint32_t i, j = 0;
	POINT4D *p, *np;
	int ndims = FLAGS_NDIMS(pa->flags);
	for ( i = 0; i < pa->npoints; i++ )
	{
		int isnan = 0;
		p = (POINT4D *)(getPoint_internal(pa, i));
		if ( isnan(p->x) || isnan(p->y) ) isnan = 1;
		else if (ndims > 2 && isnan(p->z) ) isnan = 1;
		else if (ndims > 3 && isnan(p->m) ) isnan = 1;
		if ( isnan ) continue;

		np = (POINT4D *)(getPoint_internal(pa, j++));
		if ( np != p ) {
			np->x = p->x;
			np->y = p->y;
			if (ndims > 2)
				np->z = p->z;
			if (ndims > 3)
				np->m = p->m;
		}
	}
	pa->npoints = j;
}



/*
 * Ensure the geometry is "structurally" valid
 * (enough for GEOS to accept it)
 * May return the input untouched (if already valid).
 * May return geometries of lower dimension (on collapses)
 */
static LWGEOM*
lwgeom_make_geos_friendly(LWGEOM* geom)
{
	LWDEBUGF(2, "lwgeom_make_geos_friendly enter (type %d)", geom->type);
	switch (geom->type)
	{
	case POINTTYPE:
		ptarray_strip_nan_coords_in_place(((LWPOINT*)geom)->point);
		return geom;

	case LINETYPE:
		/* lines need at least 2 points */
		return lwline_make_geos_friendly((LWLINE*)geom);
		break;

	case POLYGONTYPE:
		/* polygons need all rings closed and with npoints > 3 */
		return lwpoly_make_geos_friendly((LWPOLY*)geom);
		break;

	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COLLECTIONTYPE:
	case MULTIPOINTTYPE:
		return lwcollection_make_geos_friendly((LWCOLLECTION*)geom);
		break;

	case CIRCSTRINGTYPE:
	case COMPOUNDTYPE:
	case CURVEPOLYTYPE:
	case MULTISURFACETYPE:
	case MULTICURVETYPE:
	default:
		lwerror("lwgeom_make_geos_friendly: unsupported input geometry type: %s (%d)",
			lwtype_name(geom->type),
			geom->type);
		break;
	}
	return 0;
}

/*
 * Close the point array, if not already closed in 2d.
 * Returns the input if already closed in 2d, or a newly
 * constructed POINTARRAY.
 * TODO: move in ptarray.c
 */
static POINTARRAY*
ptarray_close2d(POINTARRAY* ring)
{
	POINTARRAY* newring;

	/* close the ring if not already closed (2d only) */
	if (!ptarray_is_closed_2d(ring))
	{
		/* close it up */
		newring = ptarray_addPoint(ring, getPoint_internal(ring, 0), FLAGS_NDIMS(ring->flags), ring->npoints);
		ring = newring;
	}
	return ring;
}

/* May return the same input or a new one (never zero) */
POINTARRAY*
ring_make_geos_friendly(POINTARRAY* ring)
{
	POINTARRAY* closedring;
	POINTARRAY* ring_in = ring;

	ptarray_strip_nan_coords_in_place(ring_in);

	/* close the ring if not already closed (2d only) */
	closedring = ptarray_close2d(ring);
	if (closedring != ring) ring = closedring;

	/* return 0 for collapsed ring (after closeup) */

	while (ring->npoints < 4)
	{
		POINTARRAY* oring = ring;
		LWDEBUGF(4, "ring has %d points, adding another", ring->npoints);
		/* let's add another... */
		ring = ptarray_addPoint(ring, getPoint_internal(ring, 0), FLAGS_NDIMS(ring->flags), ring->npoints);
		if (oring != ring_in) ptarray_free(oring);
	}

	return ring;
}

/* Make sure all rings are closed and have > 3 points.
 * May return the input untouched.
 */
LWGEOM*
lwpoly_make_geos_friendly(LWPOLY* poly)
{
	LWGEOM* ret;
	POINTARRAY** new_rings;
	uint32_t i;

	/* If the polygon has no rings there's nothing to do */
	if (!poly->nrings) return (LWGEOM*)poly;

	/* Allocate enough pointers for all rings */
	new_rings = lwalloc(sizeof(POINTARRAY*) * poly->nrings);

	/* All rings must be closed and have > 3 points */
	for (i = 0; i < poly->nrings; i++)
	{
		POINTARRAY* ring_in = poly->rings[i];
		POINTARRAY* ring_out = ring_make_geos_friendly(ring_in);

		if (ring_in != ring_out)
		{
			LWDEBUGF(
			    3, "lwpoly_make_geos_friendly: ring %d cleaned, now has %d points", i, ring_out->npoints);
			ptarray_free(ring_in);
		}
		else
			LWDEBUGF(3, "lwpoly_make_geos_friendly: ring %d untouched", i);

		assert(ring_out);
		new_rings[i] = ring_out;
	}

	lwfree(poly->rings);
	poly->rings = new_rings;
	ret = (LWGEOM*)poly;

	return ret;
}

/* Need NO or >1 points. Duplicate first if only one. */
LWGEOM*
lwline_make_geos_friendly(LWLINE* line)
{
	LWGEOM* ret;

	ptarray_strip_nan_coords_in_place(line->points);

	if (line->points->npoints == 1) /* 0 is fine, 2 is fine */
	{
#if 1
		/* Duplicate point */
		line->points = ptarray_addPoint(line->points,
						getPoint_internal(line->points, 0),
						FLAGS_NDIMS(line->points->flags),
						line->points->npoints);
		ret = (LWGEOM*)line;
#else
		/* Turn into a point */
		ret = (LWGEOM*)lwpoint_construct(line->srid, 0, line->points);
#endif
		return ret;
	}
	else
	{
		return (LWGEOM*)line;
		/* return lwline_clone(line); */
	}
}

LWGEOM*
lwcollection_make_geos_friendly(LWCOLLECTION* g)
{
	LWGEOM** new_geoms;
	uint32_t i, new_ngeoms = 0;
	LWCOLLECTION* ret;

	if ( ! g->ngeoms ) {
		LWDEBUG(3, "lwcollection_make_geos_friendly: returning input untouched");
		return lwcollection_as_lwgeom(g);
	}

	/* enough space for all components */
	new_geoms = lwalloc(sizeof(LWGEOM*) * g->ngeoms);

	ret = lwalloc(sizeof(LWCOLLECTION));
	memcpy(ret, g, sizeof(LWCOLLECTION));
	ret->maxgeoms = g->ngeoms;

	for (i = 0; i < g->ngeoms; i++)
	{
		LWGEOM* newg = lwgeom_make_geos_friendly(g->geoms[i]);
		if (!newg) continue;
		if ( newg != g->geoms[i] ) {
			new_geoms[new_ngeoms++] = newg;
		} else {
			new_geoms[new_ngeoms++] = lwgeom_clone(newg);
		}
	}

	ret->bbox = NULL; /* recompute later... */

	ret->ngeoms = new_ngeoms;
	if (new_ngeoms)
		ret->geoms = new_geoms;
	else
	{
		free(new_geoms);
		ret->geoms = NULL;
		ret->maxgeoms = 0;
	}

	return (LWGEOM*)ret;
}

/* Exported. Uses GEOS internally */
LWGEOM*
lwgeom_make_valid(LWGEOM* lwgeom_in)
{
	return lwgeom_make_valid_params(lwgeom_in, NULL);
}

/* Exported. Uses GEOS internally */
LWGEOM*
lwgeom_make_valid_params(LWGEOM* lwgeom_in, char* make_valid_params)
{
	int is3d;
	GEOSGeom geosgeom;
	GEOSGeometry* geosout;
	LWGEOM* lwgeom_out;

	LWDEBUG(1, "lwgeom_make_valid enter");

	is3d = FLAGS_GET_Z(lwgeom_in->flags);

	/*
	 * Step 1 : try to convert to GEOS, if impossible, clean that up first
	 *          otherwise (adding only duplicates of existing points)
	 */

	initGEOS(lwgeom_geos_error, lwgeom_geos_error);

	lwgeom_out = lwgeom_make_geos_friendly(lwgeom_in);
	if (!lwgeom_out) lwerror("Could not make a geos friendly geometry out of input");

	LWDEBUGF(4, "Input geom %p made GEOS-valid as %p", lwgeom_in, lwgeom_out);

	geosgeom = LWGEOM2GEOS(lwgeom_out, 1);
	if ( lwgeom_in != lwgeom_out ) {
		lwgeom_free(lwgeom_out);
	}
	if (!geosgeom)
	{
		lwerror("Couldn't convert POSTGIS geom to GEOS: %s", lwgeom_geos_errmsg);
		return NULL;
	}
	else
	{
		LWDEBUG(4, "geom converted to GEOS");
	}

#if POSTGIS_GEOS_VERSION < 31000
	geosout = GEOSMakeValid(geosgeom);
#else
	if (!make_valid_params) {
		geosout = GEOSMakeValid(geosgeom);
	}
	else {
		/*
		* Set up a parameters object for this
		* make valid operation before calling
		* it
		*/
		const char *value;
		char *param_list[OPTION_LIST_SIZE];
		char param_list_text[OPTION_LIST_SIZE];
		strncpy(param_list_text, make_valid_params, OPTION_LIST_SIZE-1);
		param_list_text[OPTION_LIST_SIZE-1] = '\0'; /* ensure null-termination */
		memset(param_list, 0, sizeof(param_list));
		option_list_parse(param_list_text, param_list);
		GEOSMakeValidParams *params = GEOSMakeValidParams_create();
		value = option_list_search(param_list, "method");
		if (value) {
			if (strcasecmp(value, "linework") == 0) {
				GEOSMakeValidParams_setMethod(params, GEOS_MAKE_VALID_LINEWORK);
			}
			else if (strcasecmp(value, "structure") == 0) {
				GEOSMakeValidParams_setMethod(params, GEOS_MAKE_VALID_STRUCTURE);
			}
			else
			{
				GEOSMakeValidParams_destroy(params);
				lwerror("Unsupported value for 'method', '%s'. Use 'linework' or 'structure'.", value);
			}
		}
		value = option_list_search(param_list, "keepcollapsed");
		if (value) {
			if (strcasecmp(value, "true") == 0) {
				GEOSMakeValidParams_setKeepCollapsed(params, 1);
			}
			else if (strcasecmp(value, "false") == 0) {
				GEOSMakeValidParams_setKeepCollapsed(params, 0);
			}
			else
			{
				GEOSMakeValidParams_destroy(params);
				lwerror("Unsupported value for 'keepcollapsed', '%s'. Use 'true' or 'false'", value);
			}
		}
		geosout = GEOSMakeValidWithParams(geosgeom, params);
		GEOSMakeValidParams_destroy(params);
	}
#endif
	GEOSGeom_destroy(geosgeom);
	if (!geosout) return NULL;

	lwgeom_out = GEOS2LWGEOM(geosout, is3d);
	GEOSGeom_destroy(geosout);

	if (lwgeom_is_collection(lwgeom_in) && !lwgeom_is_collection(lwgeom_out))
	{
		LWGEOM** ogeoms = lwalloc(sizeof(LWGEOM*));
		LWGEOM* ogeom;
		LWDEBUG(3, "lwgeom_make_valid: forcing multi");
		/* NOTE: this is safe because lwgeom_out is surely not lwgeom_in or
		 * otherwise we couldn't have a collection and a non-collection */
		assert(lwgeom_in != lwgeom_out);
		ogeoms[0] = lwgeom_out;
		ogeom = (LWGEOM*)lwcollection_construct(
		    MULTITYPE[lwgeom_out->type], lwgeom_out->srid, lwgeom_out->bbox, 1, ogeoms);
		lwgeom_out->bbox = NULL;
		lwgeom_out = ogeom;
	}

	lwgeom_out->srid = lwgeom_in->srid;
	return lwgeom_out;
}
