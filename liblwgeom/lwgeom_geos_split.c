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
 * Copyright 2011-2015 Sandro Santilli <strk@kbt.io>
 *
 **********************************************************************/

#include "../postgis_config.h"
/*#define POSTGIS_DEBUG_LEVEL 4*/
#include "lwgeom_geos.h"
#include "liblwgeom_internal.h"

#include <string.h>
#include <assert.h>

static LWGEOM* lwline_split_by_line(const LWLINE* lwgeom_in, const LWGEOM* blade_in);
static LWGEOM* lwline_split_by_point(const LWLINE* lwgeom_in, const LWPOINT* blade_in);
static LWGEOM* lwline_split_by_mpoint(const LWLINE* lwgeom_in, const LWMPOINT* blade_in);
static LWGEOM* lwline_split(const LWLINE* lwgeom_in, const LWGEOM* blade_in);
static LWGEOM* lwpoly_split_by_line(const LWPOLY* lwgeom_in, const LWGEOM* blade_in);
static LWGEOM* lwcollection_split(const LWCOLLECTION* lwcoll_in, const LWGEOM* blade_in);
static LWGEOM* lwpoly_split(const LWPOLY* lwpoly_in, const LWGEOM* blade_in);

/* Initializes and uses GEOS internally */
static LWGEOM*
lwline_split_by_line(const LWLINE* lwline_in, const LWGEOM* blade_in)
{
	LWGEOM** components;
	LWGEOM* diff;
	LWCOLLECTION* out;
	GEOSGeometry* gdiff; /* difference */
	GEOSGeometry* g1;
	GEOSGeometry* g2;
	int ret;

	/* ASSERT blade_in is LINE or MULTILINE */
	assert (blade_in->type == LINETYPE ||
	        blade_in->type == MULTILINETYPE ||
	        blade_in->type == POLYGONTYPE ||
	        blade_in->type == MULTIPOLYGONTYPE );

	/* Possible outcomes:
	 *
	 *  1. The lines do not cross or overlap
	 *      -> Return a collection with single element
	 *  2. The lines cross
	 *      -> Return a collection of all elements resulting from the split
	 */

	initGEOS(lwgeom_geos_error, lwgeom_geos_error);

	g1 = LWGEOM2GEOS((LWGEOM*)lwline_in, 0);
	if ( ! g1 )
	{
		lwerror("LWGEOM2GEOS: %s", lwgeom_geos_errmsg);
		return NULL;
	}
	g2 = LWGEOM2GEOS(blade_in, 0);
	if ( ! g2 )
	{
		GEOSGeom_destroy(g1);
		lwerror("LWGEOM2GEOS: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	/* If blade is a polygon, pick its boundary */
	if ( blade_in->type == POLYGONTYPE || blade_in->type == MULTIPOLYGONTYPE )
	{
		gdiff = GEOSBoundary(g2);
		GEOSGeom_destroy(g2);
		if ( ! gdiff )
		{
			GEOSGeom_destroy(g1);
			lwerror("GEOSBoundary: %s", lwgeom_geos_errmsg);
			return NULL;
		}
		g2 = gdiff; gdiff = NULL;
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

	out = lwgeom_as_lwcollection(diff);
	if ( ! out )
	{
		components = lwalloc(sizeof(LWGEOM*)*1);
		components[0] = diff;
		out = lwcollection_construct(COLLECTIONTYPE, lwline_in->srid,
		                             NULL, 1, components);
	}
	else
	{
	  /* Set SRID */
		lwgeom_set_srid((LWGEOM*)out, lwline_in->srid);
	  /* Force collection type */
	  out->type = COLLECTIONTYPE;
	}


	return (LWGEOM*)out;
}

static LWGEOM*
lwline_split_by_point(const LWLINE* lwline_in, const LWPOINT* blade_in)
{
	LWMLINE* out;

	out = lwmline_construct_empty(lwline_in->srid,
		FLAGS_GET_Z(lwline_in->flags),
		FLAGS_GET_M(lwline_in->flags));
	if ( lwline_split_by_point_to(lwline_in, blade_in, out) < 2 )
	{
		lwmline_add_lwline(out, lwline_clone_deep(lwline_in));
	}

	/* Turn multiline into collection */
	out->type = COLLECTIONTYPE;

	return (LWGEOM*)out;
}

static LWGEOM*
lwline_split_by_mpoint(const LWLINE* lwline_in, const LWMPOINT* mp)
{
  LWMLINE* out;
  uint32_t i, j;

  out = lwmline_construct_empty(lwline_in->srid,
          FLAGS_GET_Z(lwline_in->flags),
          FLAGS_GET_M(lwline_in->flags));
  lwmline_add_lwline(out, lwline_clone_deep(lwline_in));

  for (i=0; i<mp->ngeoms; ++i)
  {
    for (j=0; j<out->ngeoms; ++j)
    {
      lwline_in = out->geoms[j];
      LWPOINT *blade_in = mp->geoms[i];
      int ret = lwline_split_by_point_to(lwline_in, blade_in, out);
      if ( 2 == ret )
      {
        /* the point splits this line,
         * 2 splits were added to collection.
         * We'll move the latest added into
         * the slot of the current one.
         */
        lwline_free(out->geoms[j]);
        out->geoms[j] = out->geoms[--out->ngeoms];
      }
    }
  }

  /* Turn multiline into collection */
  out->type = COLLECTIONTYPE;

  return (LWGEOM*)out;
}

int
lwline_split_by_point_to(const LWLINE* lwline_in, const LWPOINT* blade_in,
                         LWMLINE* v)
{
	double mindist_sqr = -1;
	POINT4D pt, pt_projected;
	POINT4D p1, p2;
	POINTARRAY *ipa = lwline_in->points;
	POINTARRAY* pa1;
	POINTARRAY* pa2;
	uint32_t i, nsegs, seg = UINT32_MAX;

	/* Possible outcomes:
	 *
	 *  1. The point is not on the line or on the boundary
	 *      -> Leave collection untouched, return 0
	 *  2. The point is on the boundary
	 *      -> Leave collection untouched, return 1
	 *  3. The point is in the line
	 *      -> Push 2 elements on the collection:
	 *         o start_point - cut_point
	 *         o cut_point - last_point
	 *      -> Return 2
	 */

	getPoint4d_p(blade_in->point, 0, &pt);

	/* Find closest segment */
	getPoint4d_p(ipa, 0, &p1);
	nsegs = ipa->npoints - 1;
	for ( i = 0; i < nsegs; i++ )
	{
		getPoint4d_p(ipa, i+1, &p2);
		double dist_sqr = distance2d_sqr_pt_seg((POINT2D *)&pt, (POINT2D *)&p1, (POINT2D *)&p2);
		LWDEBUGF(4, "Distance (squared) of point %g %g to segment %g %g, %g %g: %g",
				 pt.x, pt.y,
				 p1.x, p1.y,
				 p2.x, p2.y,
				 dist_sqr);
		if (i == 0 || dist_sqr < mindist_sqr)
		{
			mindist_sqr = dist_sqr;
			seg=i;
			if (mindist_sqr == 0.0)
				break; /* can't be closer than ON line */
		}
		p1 = p2;
	}

	LWDEBUGF(3, "Closest segment: %d", seg);
	LWDEBUGF(3, "mindist: %g", mindist_sqr);

	/* No intersection */
	if (mindist_sqr > 0)
		return 0;

	/* empty or single-point line, intersection on boundary */
	if ( seg == UINT32_MAX ) return 1;

	/*
	 * We need to project the
	 * point on the closest segment,
	 * to interpolate Z and M if needed
	 */
	getPoint4d_p(ipa, seg, &p1);
	getPoint4d_p(ipa, seg+1, &p2);
	closest_point_on_segment(&pt, &p1, &p2, &pt_projected);
	/* But X and Y we want the ones of the input point,
	 * as on some architectures the interpolation math moves the
	 * coordinates (see #3422)
	 */
	pt_projected.x = pt.x;
	pt_projected.y = pt.y;

	LWDEBUGF(3, "Projected point:(%g %g), seg:%d, p1:(%g %g), p2:(%g %g)", pt_projected.x, pt_projected.y, seg, p1.x, p1.y, p2.x, p2.y);

	/* When closest point == an endpoint, this is a boundary intersection */
	if ( ( (seg == nsegs-1) && p4d_same(&pt_projected, &p2) ) ||
	     ( (seg == 0)       && p4d_same(&pt_projected, &p1) ) )
	{
		return 1;
	}

	/* This is an internal intersection, let's build the two new pointarrays */

	pa1 = ptarray_construct_empty(FLAGS_GET_Z(ipa->flags), FLAGS_GET_M(ipa->flags), seg+2);
	/* TODO: replace with a memcpy ? */
	for (i=0; i<=seg; ++i)
	{
		getPoint4d_p(ipa, i, &p1);
		ptarray_append_point(pa1, &p1, LW_FALSE);
	}
	ptarray_append_point(pa1, &pt_projected, LW_FALSE);

	pa2 = ptarray_construct_empty(FLAGS_GET_Z(ipa->flags), FLAGS_GET_M(ipa->flags), ipa->npoints-seg);
	ptarray_append_point(pa2, &pt_projected, LW_FALSE);
	/* TODO: replace with a memcpy (if so need to check for duplicated point) ? */
	for (i=seg+1; i<ipa->npoints; ++i)
	{
		getPoint4d_p(ipa, i, &p1);
		ptarray_append_point(pa2, &p1, LW_FALSE);
	}

	/* NOTE: I've seen empty pointarrays with loc != 0 and loc != 1 */
	if ( pa1->npoints == 0 || pa2->npoints == 0 ) {
		ptarray_free(pa1);
		ptarray_free(pa2);
		/* Intersection is on the boundary */
		return 1;
	}

	lwmline_add_lwline(v, lwline_construct(SRID_UNKNOWN, NULL, pa1));
	lwmline_add_lwline(v, lwline_construct(SRID_UNKNOWN, NULL, pa2));
	return 2;
}

static LWGEOM*
lwline_split(const LWLINE* lwline_in, const LWGEOM* blade_in)
{
	switch (blade_in->type)
	{
	case POINTTYPE:
		return lwline_split_by_point(lwline_in, (LWPOINT*)blade_in);
	case MULTIPOINTTYPE:
		return lwline_split_by_mpoint(lwline_in, (LWMPOINT*)blade_in);

	case LINETYPE:
	case MULTILINETYPE:
	case POLYGONTYPE:
	case MULTIPOLYGONTYPE:
		return lwline_split_by_line(lwline_in, blade_in);

	default:
		lwerror("Splitting a Line by a %s is unsupported",
		        lwtype_name(blade_in->type));
		return NULL;
	}
	return NULL;
}

/* Initializes and uses GEOS internally */
static LWGEOM*
lwpoly_split_by_line(const LWPOLY* lwpoly_in, const LWGEOM* blade_in)
{
	LWCOLLECTION* out;
	GEOSGeometry* g1;
	GEOSGeometry* g2;
	GEOSGeometry* g1_bounds;
	GEOSGeometry* polygons;
	const GEOSGeometry *vgeoms[1];
	int i,n;
	int hasZ = FLAGS_GET_Z(lwpoly_in->flags);


	/* Possible outcomes:
	 *
	 *  1. The line does not split the polygon
	 *      -> Return a collection with single element
	 *  2. The line does split the polygon
	 *      -> Return a collection of all elements resulting from the split
	 */

	initGEOS(lwgeom_geos_error, lwgeom_geos_error);

	g1 = LWGEOM2GEOS((LWGEOM*)lwpoly_in, 0);
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

	g2 = LWGEOM2GEOS(blade_in, 0);
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
	if ( GEOSGeomTypeId(polygons) != COLLECTIONTYPE )
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		GEOSGeom_destroy(g1_bounds);
		GEOSGeom_destroy((GEOSGeometry*)vgeoms[0]);
		GEOSGeom_destroy(polygons);
		lwerror("%s [%s] Unexpected return from GEOSpolygonize", __FILE__, __LINE__);
		return 0;
	}
#endif

	/* We should now have all polygons, just skip
	 * the ones which are in holes of the original
	 * geometries and return the rest in a collection
	 */
	n = GEOSGetNumGeometries(polygons);
	out = lwcollection_construct_empty(COLLECTIONTYPE, lwpoly_in->srid,
				     hasZ, 0);
	/* Allocate space for all polys */
	out->geoms = lwrealloc(out->geoms, sizeof(LWGEOM*)*n);
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

		out->geoms[out->ngeoms++] = GEOS2LWGEOM(p, hasZ);
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
		lwfree(col->geoms);
		lwfree(col);
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
	case MULTILINETYPE:
	case LINETYPE:
		return lwpoly_split_by_line(lwpoly_in, blade_in);
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

