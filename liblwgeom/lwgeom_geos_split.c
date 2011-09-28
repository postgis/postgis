/**********************************************************************
 * $Id: lwgeom_geos.c 5258 2010-02-17 21:02:49Z strk $
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 *
 * Copyright 2009-2010 Sandro Santilli <strk@keybit.net>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************
 *
 * Split polygon by line, line by line, line by point.
 * Returns at most components as a collection.
 * First element of the collection is always the part which
 * remains after the cut, while the second element is the
 * part which has been cut out. We arbitrarely take the part
 * on the *right* of cut lines as the part which has been cut out.
 * For a line cut by a point the part which remains is the one
 * from start of the line to the cut point.
 *
 *
 * Author: Sandro Santilli <strk@keybit.net>
 *
 * Work done for Faunalia (http://www.faunalia.it) with fundings
 * from Regione Toscana - Sistema Informativo per il Governo
 * del Territorio e dell'Ambiente (RT-SIGTA).
 *
 * Thanks to the PostGIS community for sharing poly/line ideas [1]
 *
 * [1] http://trac.osgeo.org/postgis/wiki/UsersWikiSplitPolygonWithLineString
 *
 *
 **********************************************************************/

#include "lwgeom_geos.h"
#include "liblwgeom_internal.h"

#include <string.h>
#include <assert.h>

static LWGEOM* lwline_split_by_line(const LWLINE* lwgeom_in, const LWLINE* blade_in);
static LWGEOM* lwline_split_by_point(const LWLINE* lwgeom_in, const LWPOINT* blade_in);
static LWGEOM* lwline_split(const LWLINE* lwgeom_in, const LWGEOM* blade_in);
static LWGEOM* lwpoly_split_by_line(const LWPOLY* lwgeom_in, const LWLINE* blade_in);
static LWGEOM* lwcollection_split(const LWCOLLECTION* lwcoll_in, const LWGEOM* blade_in);
static LWGEOM* lwpoly_split(const LWPOLY* lwpoly_in, const LWGEOM* blade_in);

/* Initializes and uses GEOS internally */
static LWGEOM*
lwline_split_by_line(const LWLINE* lwline_in, const LWLINE* blade_in)
{
	LWGEOM** components;
	LWGEOM* diff;
	LWCOLLECTION* out;
	GEOSGeometry* gdiff; /* difference */
	GEOSGeometry* g1;
	GEOSGeometry* g2;
	int ret;

	/* Possible outcomes:
	 *
	 *  1. The lines do not cross or overlap
	 *      -> Return a collection with single element
	 *  2. The lines cross
	 *      -> Return a collection of all elements resulting from the split
	 */

	initGEOS(lwgeom_geos_error, lwgeom_geos_error);

	g1 = LWGEOM2GEOS((LWGEOM*)lwline_in);
	if ( ! g1 )
	{
		lwerror("LWGEOM2GEOS: %s", lwgeom_geos_errmsg);
		return NULL;
	}
	g2 = LWGEOM2GEOS((LWGEOM*)blade_in);
	if ( ! g2 )
	{
		GEOSGeom_destroy(g1);
		lwerror("LWGEOM2GEOS: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	/* If interior intersecton is linear we can't split */
	ret = GEOSRelatePattern(g1, g2, "1********");
	if ( 2 == ret )
	{
		lwerror("GEOSRelatePattern: %s", lwgeom_geos_errmsg);
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		return NULL;
	}
	if ( ret )
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		lwerror("Splitter line has linear intersection with input");
		return NULL;
	}


	gdiff = GEOSDifference(g1,g2);
	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	if (gdiff == NULL)
	{
		lwerror("GEOSDifference: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	diff = GEOS2LWGEOM(gdiff, FLAGS_GET_Z(lwline_in->flags));
	GEOSGeom_destroy(gdiff);
	if (NULL == diff)
	{
		lwerror("GEOS2LWGEOM: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	if ( ! lwtype_is_collection(diff->type) )
	{
		components = lwalloc(sizeof(LWGEOM*)*1);
		components[0] = diff;
		out = lwcollection_construct(COLLECTIONTYPE, lwline_in->srid,
		                             NULL, 1, components);
	}
	else
	{
		out = lwcollection_construct(COLLECTIONTYPE, lwline_in->srid,
		                             NULL, ((LWCOLLECTION*)diff)->ngeoms,
		                             ((LWCOLLECTION*)diff)->geoms);
	}


	return (LWGEOM*)out;
}

static LWGEOM*
lwline_split_by_point(const LWLINE* lwline_in, const LWPOINT* blade_in)
{
	double loc, dist;
	POINT2D pt;
	POINTARRAY* pa1;
	POINTARRAY* pa2;
	LWGEOM** components;
	LWCOLLECTION* out;
	int ncomponents;

	/* Possible outcomes:
	 *
	 *  1. The point is not on the line or on the boundary
	 *      -> Return a collection with single element
	 *  2. The point is on the line
	 *      -> Return a collection 2 elements:
	 *         o start_point - cut_point
	 *         o cut_point - last_point
	 */

	getPoint2d_p(blade_in->point, 0, &pt);
	loc = ptarray_locate_point(lwline_in->points, &pt, &dist);

	/* lwnotice("Location: %g -- Distance: %g", loc, dist); */

	do
	{
		components = lwalloc(sizeof(LWGEOM*)*2);
		ncomponents = 1;

		if ( dist > 0 )   /* TODO: accept a tolerance ? */
		{
			/* No intersection */
			components[0] = (LWGEOM*)lwline_clone(lwline_in);
			components[0]->srid = SRID_UNKNOWN;
			break;
		}

		/* There is an intersection, let's get two substrings */
		if ( loc == 0 || loc == 1 )
		{
			/* Intersection is on the boundary or outside */
			components[0] = (LWGEOM*)lwline_clone(lwline_in);
			components[0]->srid = SRID_UNKNOWN;
			break;
		}

		pa1 = ptarray_substring(lwline_in->points, 0, loc);
		pa2 = ptarray_substring(lwline_in->points, loc, 1);

		/* TODO: check if either pa1 or pa2 are empty ? */

		components[0] = (LWGEOM*)lwline_construct(SRID_UNKNOWN, NULL, pa1);
		components[1] = (LWGEOM*)lwline_construct(SRID_UNKNOWN, NULL, pa2);
		++ncomponents;
	}
	while (0);

	out = lwcollection_construct(COLLECTIONTYPE, lwline_in->srid,
	                             NULL, ncomponents, components);

	/* That's all folks */

	return (LWGEOM*)out;
}

static LWGEOM*
lwline_split(const LWLINE* lwline_in, const LWGEOM* blade_in)
{
	switch (blade_in->type)
	{
	case POINTTYPE:
		return lwline_split_by_point(lwline_in, (LWPOINT*)blade_in);

	case LINETYPE:
		return lwline_split_by_line(lwline_in, (LWLINE*)blade_in);

	default:
		lwerror("Splitting a Line by a %s is unsupported",
		        lwtype_name(blade_in->type));
		return NULL;
	}
	return NULL;
}

/* Initializes and uses GEOS internally */
static LWGEOM*
lwpoly_split_by_line(const LWPOLY* lwpoly_in, const LWLINE* blade_in)
{
	LWCOLLECTION* out;
	GEOSGeometry* g1;
	GEOSGeometry* g2;
	GEOSGeometry* g1_bounds;
	GEOSGeometry* polygons;
	const GEOSGeometry *vgeoms[1];
	int i,n;

	/* Possible outcomes:
	 *
	 *  1. The line does not split the polygon
	 *      -> Return a collection with single element
	 *  2. The line does split the polygon
	 *      -> Return a collection of all elements resulting from the split
	 */

	initGEOS(lwgeom_geos_error, lwgeom_geos_error);

	g1 = LWGEOM2GEOS((LWGEOM*)lwpoly_in);
	if ( NULL == g1 )
	{
		lwerror("LWGEOM2GEOS: %s", lwgeom_geos_errmsg);
		return NULL;
	}
	g1_bounds = GEOSBoundary(g1);
	if ( NULL == g1_bounds )
	{
		GEOSGeom_destroy(g1);
		lwerror("GEOSBoundary: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	g2 = LWGEOM2GEOS((LWGEOM*)blade_in);
	if ( NULL == g2 )
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g1_bounds);
		lwerror("LWGEOM2GEOS: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	vgeoms[0] = GEOSUnion(g1_bounds, g2);
	if ( NULL == vgeoms[0] )
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		GEOSGeom_destroy(g1_bounds);
		lwerror("GEOSUnion: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	/* debugging..
		lwnotice("Bounds poly: %s",
		               lwgeom_to_ewkt(GEOS2LWGEOM(g1_bounds, 0)));
		lwnotice("Line: %s",
		               lwgeom_to_ewkt(GEOS2LWGEOM(g2, 0)));

		lwnotice("Noded bounds: %s",
		               lwgeom_to_ewkt(GEOS2LWGEOM(vgeoms[0], 0)));
	*/

	polygons = GEOSPolygonize(vgeoms, 1);
	if ( NULL == polygons )
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		GEOSGeom_destroy(g1_bounds);
		GEOSGeom_destroy((GEOSGeometry*)vgeoms[0]);
		lwerror("GEOSPolygonize: %s", lwgeom_geos_errmsg);
		return NULL;
	}

#if PARANOIA_LEVEL > 0
	if ( GEOSGeometryTypeId(polygons) != COLLECTIONTYPE )
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		GEOSGeom_destroy(g1_bounds);
		GEOSGeom_destroy((GEOSGeometry*)vgeoms[0]);
		GEOSGeom_destroy(polygons);
		lwerror("Unexpected return from GEOSpolygonize");
		return 0;
	}
#endif

	/* We should now have all polygons, just skip
	 * the ones which are in holes of the original
	 * geometries and return the rest in a collection
	 */
	n = GEOSGetNumGeometries(polygons);
	out = lwcollection_construct(COLLECTIONTYPE, lwpoly_in->srid,
	                             NULL, 0, NULL);
	/* Allocate space for all polys */
	out->geoms = lwalloc(sizeof(LWGEOM*)*n);
	assert(0 == out->ngeoms);
	for (i=0; i<n; ++i)
	{
		GEOSGeometry* pos; /* point on surface */
		const GEOSGeometry* p = GEOSGetGeometryN(polygons, i);
		int contains;

		pos = GEOSPointOnSurface(p);
		if ( ! pos )
		{
			GEOSGeom_destroy(g1);
			GEOSGeom_destroy(g2);
			GEOSGeom_destroy(g1_bounds);
			GEOSGeom_destroy((GEOSGeometry*)vgeoms[0]);
			GEOSGeom_destroy(polygons);
			lwerror("GEOSPointOnSurface: %s", lwgeom_geos_errmsg);
			return NULL;
		}

		contains = GEOSContains(g1, pos);
		if ( 2 == contains )
		{
			GEOSGeom_destroy(g1);
			GEOSGeom_destroy(g2);
			GEOSGeom_destroy(g1_bounds);
			GEOSGeom_destroy((GEOSGeometry*)vgeoms[0]);
			GEOSGeom_destroy(polygons);
			GEOSGeom_destroy(pos);
			lwerror("GEOSContains: %s", lwgeom_geos_errmsg);
			return NULL;
		}

		GEOSGeom_destroy(pos);

		if ( 0 == contains )
		{
			/* Original geometry doesn't contain
			 * a point in this ring, must be an hole
			 */
			continue;
		}

		out->geoms[out->ngeoms++] = GEOS2LWGEOM(p, FLAGS_GET_Z(lwpoly_in->flags));
	}

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	GEOSGeom_destroy(g1_bounds);
	GEOSGeom_destroy((GEOSGeometry*)vgeoms[0]);
	GEOSGeom_destroy(polygons);

	return (LWGEOM*)out;
}

static LWGEOM*
lwcollection_split(const LWCOLLECTION* lwcoll_in, const LWGEOM* blade_in)
{
	LWGEOM** split_vector=NULL;
	LWCOLLECTION* out;
	size_t split_vector_capacity;
	size_t split_vector_size=0;
	size_t i,j;

	split_vector_capacity=8;
	split_vector = lwalloc(split_vector_capacity * sizeof(LWGEOM*));
	if ( ! split_vector )
	{
		lwerror("Out of virtual memory");
		return NULL;
	}

	for (i=0; i<lwcoll_in->ngeoms; ++i)
	{
		LWCOLLECTION* col;
		LWGEOM* split = lwgeom_split(lwcoll_in->geoms[i], blade_in);
		/* an exception should prevent this from ever returning NULL */
		if ( ! split ) return NULL;

		col = lwgeom_as_lwcollection(split);
		/* Output, if any, will always be a collection */
		assert(col);

		/* Reallocate split_vector if needed */
		if ( split_vector_size + col->ngeoms > split_vector_capacity )
		{
			/* NOTE: we could be smarter on reallocations here */
			split_vector_capacity += col->ngeoms;
			split_vector = lwrealloc(split_vector,
			                         split_vector_capacity * sizeof(LWGEOM*));
			if ( ! split_vector )
			{
				lwerror("Out of virtual memory");
				return NULL;
			}
		}

		for (j=0; j<col->ngeoms; ++j)
		{
			col->geoms[j]->srid = SRID_UNKNOWN; /* strip srid */
			split_vector[split_vector_size++] = col->geoms[j];
		}
	}

	/* Now split_vector has split_vector_size geometries */
	out = lwcollection_construct(COLLECTIONTYPE, lwcoll_in->srid,
	                             NULL, split_vector_size, split_vector);

	return (LWGEOM*)out;
}

static LWGEOM*
lwpoly_split(const LWPOLY* lwpoly_in, const LWGEOM* blade_in)
{
	switch (blade_in->type)
	{
	case LINETYPE:
		return lwpoly_split_by_line(lwpoly_in, (LWLINE*)blade_in);
	default:
		lwerror("Splitting a Polygon by a %s is unsupported",
		        lwtype_name(blade_in->type));
		return NULL;
	}
	return NULL;
}

/* exported */
LWGEOM*
lwgeom_split(const LWGEOM* lwgeom_in, const LWGEOM* blade_in)
{
	switch (lwgeom_in->type)
	{
	case LINETYPE:
		return lwline_split((const LWLINE*)lwgeom_in, blade_in);

	case POLYGONTYPE:
		return lwpoly_split((const LWPOLY*)lwgeom_in, blade_in);

	case MULTIPOLYGONTYPE:
	case MULTILINETYPE:
	case COLLECTIONTYPE:
		return lwcollection_split((const LWCOLLECTION*)lwgeom_in, blade_in);

	default:
		lwerror("Splitting of %s geometries is unsupported",
		        lwtype_name(lwgeom_in->type));
		return NULL;
	}

}

