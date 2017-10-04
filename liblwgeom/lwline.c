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
 * Copyright (C) 2012 Sandro Santilli <strk@kbt.io>
 * Copyright (C) 2001-2006 Refractions Research Inc.
 *
 **********************************************************************/


/* basic LWLINE functions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "liblwgeom_internal.h"
#include "lwgeom_log.h"



/*
 * Construct a new LWLINE.  points will *NOT* be copied
 * use SRID=SRID_UNKNOWN for unknown SRID (will have 8bit type's S = 0)
 */
LWLINE *
lwline_construct(int srid, GBOX *bbox, POINTARRAY *points)
{
	LWLINE *result;
	result = (LWLINE*) lwalloc(sizeof(LWLINE));

	LWDEBUG(2, "lwline_construct called.");

	result->type = LINETYPE;

	result->flags = points->flags;
	FLAGS_SET_BBOX(result->flags, bbox?1:0);

	LWDEBUGF(3, "lwline_construct type=%d", result->type);

	result->srid = srid;
	result->points = points;
	result->bbox = bbox;

	return result;
}

LWLINE *
lwline_construct_empty(int srid, char hasz, char hasm)
{
	LWLINE *result = lwalloc(sizeof(LWLINE));
	result->type = LINETYPE;
	result->flags = gflags(hasz,hasm,0);
	result->srid = srid;
	result->points = ptarray_construct_empty(hasz, hasm, 1);
	result->bbox = NULL;
	return result;
}


void lwline_free (LWLINE  *line)
{
	if ( ! line ) return;

	if ( line->bbox )
		lwfree(line->bbox);
	if ( line->points )
		ptarray_free(line->points);
	lwfree(line);
}


void printLWLINE(LWLINE *line)
{
	lwnotice("LWLINE {");
	lwnotice("    ndims = %i", (int)FLAGS_NDIMS(line->flags));
	lwnotice("    srid = %i", (int)line->srid);
	printPA(line->points);
	lwnotice("}");
}

/* @brief Clone LWLINE object. Serialized point lists are not copied.
 *
 * @see ptarray_clone
 */
LWLINE *
lwline_clone(const LWLINE *g)
{
	LWLINE *ret = lwalloc(sizeof(LWLINE));

	LWDEBUGF(2, "lwline_clone called with %p", g);

	memcpy(ret, g, sizeof(LWLINE));

	ret->points = ptarray_clone(g->points);

	if ( g->bbox ) ret->bbox = gbox_copy(g->bbox);
	return ret;
}

/* Deep clone LWLINE object. POINTARRAY *is* copied. */
LWLINE *
lwline_clone_deep(const LWLINE *g)
{
	LWLINE *ret = lwalloc(sizeof(LWLINE));

	LWDEBUGF(2, "lwline_clone_deep called with %p", g);
	memcpy(ret, g, sizeof(LWLINE));

	if ( g->bbox ) ret->bbox = gbox_copy(g->bbox);
	if ( g->points ) ret->points = ptarray_clone_deep(g->points);
	FLAGS_SET_READONLY(ret->flags,0);

	return ret;
}


void
lwline_release(LWLINE *lwline)
{
	lwgeom_release(lwline_as_lwgeom(lwline));
}

void
lwline_reverse(LWLINE *line)
{
	if ( lwline_is_empty(line) ) return;
	ptarray_reverse(line->points);
}

LWLINE *
lwline_segmentize2d(LWLINE *line, double dist)
{
	POINTARRAY *segmentized = ptarray_segmentize2d(line->points, dist);
	if ( ! segmentized ) return NULL;
	return lwline_construct(line->srid, NULL, segmentized);
}

/* check coordinate equality  */
char
lwline_same(const LWLINE *l1, const LWLINE *l2)
{
	return ptarray_same(l1->points, l2->points);
}

/*
 * Construct a LWLINE from an array of point and line geometries
 * LWLINE dimensions are large enough to host all input dimensions.
 */
LWLINE *
lwline_from_lwgeom_array(int srid, uint32_t ngeoms, LWGEOM **geoms)
{
	int i;
	int hasz = LW_FALSE;
	int hasm = LW_FALSE;
	POINTARRAY *pa;
	LWLINE *line;
	POINT4D pt;
	LWPOINTITERATOR* it;

	/*
	 * Find output dimensions, check integrity
	 */
	for (i=0; i<ngeoms; i++)
	{
		if ( FLAGS_GET_Z(geoms[i]->flags) ) hasz = LW_TRUE;
		if ( FLAGS_GET_M(geoms[i]->flags) ) hasm = LW_TRUE;
		if ( hasz && hasm ) break; /* Nothing more to learn! */
	}

	/*
	 * ngeoms should be a guess about how many points we have in input.
	 * It's an underestimate for lines and multipoints */
	pa = ptarray_construct_empty(hasz, hasm, ngeoms);

	for ( i=0; i < ngeoms; i++ )
	{
		LWGEOM *g = geoms[i];

		if ( lwgeom_is_empty(g) ) continue;

		if ( g->type == POINTTYPE )
		{
			lwpoint_getPoint4d_p((LWPOINT*)g, &pt);
			ptarray_append_point(pa, &pt, LW_TRUE);
		}
		else if ( g->type == LINETYPE )
		{
			/*
			 * Append the new line points, de-duplicating against the previous points.
			 * Duplicated points internal to the linestring are untouched.
			 */
			ptarray_append_ptarray(pa, ((LWLINE*)g)->points, -1);
		}
		else if ( g->type == MULTIPOINTTYPE )
		{
			it = lwpointiterator_create(g);
			while(lwpointiterator_next(it, &pt))
			{
				ptarray_append_point(pa, &pt, LW_TRUE);
			}
			lwpointiterator_destroy(it);
		}
		else
		{
			ptarray_free(pa);
			lwerror("lwline_from_ptarray: invalid input type: %s", lwtype_name(g->type));
			return NULL;
		}
	}

	if ( pa->npoints > 0 )
		line = lwline_construct(srid, NULL, pa);
	else  {
		/* Is this really any different from the above ? */
		ptarray_free(pa);
		line = lwline_construct_empty(srid, hasz, hasm);
	}

	return line;
}

/*
 * Construct a LWLINE from an array of LWPOINTs
 * LWLINE dimensions are large enough to host all input dimensions.
 */
LWLINE *
lwline_from_ptarray(int srid, uint32_t npoints, LWPOINT **points)
{
 	int i;
	int hasz = LW_FALSE;
	int hasm = LW_FALSE;
	POINTARRAY *pa;
	LWLINE *line;
	POINT4D pt;

	/*
	 * Find output dimensions, check integrity
	 */
	for (i=0; i<npoints; i++)
	{
		if ( points[i]->type != POINTTYPE )
		{
			lwerror("lwline_from_ptarray: invalid input type: %s", lwtype_name(points[i]->type));
			return NULL;
		}
		if ( FLAGS_GET_Z(points[i]->flags) ) hasz = LW_TRUE;
		if ( FLAGS_GET_M(points[i]->flags) ) hasm = LW_TRUE;
		if ( hasz && hasm ) break; /* Nothing more to learn! */
	}

	pa = ptarray_construct_empty(hasz, hasm, npoints);

	for ( i=0; i < npoints; i++ )
	{
		if ( ! lwpoint_is_empty(points[i]) )
		{
			lwpoint_getPoint4d_p(points[i], &pt);
			ptarray_append_point(pa, &pt, LW_TRUE);
		}
	}

	if ( pa->npoints > 0 )
		line = lwline_construct(srid, NULL, pa);
	else
		line = lwline_construct_empty(srid, hasz, hasm);

	return line;
}

/*
 * Construct a LWLINE from a LWMPOINT
 */
LWLINE *
lwline_from_lwmpoint(int srid, const LWMPOINT *mpoint)
{
	uint32_t i;
	POINTARRAY *pa = NULL;
	LWGEOM *lwgeom = (LWGEOM*)mpoint;
	POINT4D pt;

	char hasz = lwgeom_has_z(lwgeom);
	char hasm = lwgeom_has_m(lwgeom);
	uint32_t npoints = mpoint->ngeoms;

	if ( lwgeom_is_empty(lwgeom) )
	{
		return lwline_construct_empty(srid, hasz, hasm);
	}

	pa = ptarray_construct(hasz, hasm, npoints);

	for (i=0; i < npoints; i++)
	{
		getPoint4d_p(mpoint->geoms[i]->point, 0, &pt);
		ptarray_set_point4d(pa, i, &pt);
	}

	LWDEBUGF(3, "lwline_from_lwmpoint: constructed pointarray for %d points", mpoint->ngeoms);

	return lwline_construct(srid, NULL, pa);
}

/**
* Returns freshly allocated #LWPOINT that corresponds to the index where.
* Returns NULL if the geometry is empty or the index invalid.
*/
LWPOINT*
lwline_get_lwpoint(const LWLINE *line, int where)
{
	POINT4D pt;
	LWPOINT *lwpoint;
	POINTARRAY *pa;

	if ( lwline_is_empty(line) || where < 0 || where >= line->points->npoints )
		return NULL;

	pa = ptarray_construct_empty(FLAGS_GET_Z(line->flags), FLAGS_GET_M(line->flags), 1);
	pt = getPoint4d(line->points, where);
	ptarray_append_point(pa, &pt, LW_TRUE);
	lwpoint = lwpoint_construct(line->srid, NULL, pa);
	return lwpoint;
}


int
lwline_add_lwpoint(LWLINE *line, LWPOINT *point, int where)
{
	POINT4D pt;
	getPoint4d_p(point->point, 0, &pt);

	if ( ptarray_insert_point(line->points, &pt, where) != LW_SUCCESS )
		return LW_FAILURE;

	/* Update the bounding box */
	if ( line->bbox )
	{
		lwgeom_drop_bbox(lwline_as_lwgeom(line));
		lwgeom_add_bbox(lwline_as_lwgeom(line));
	}

	return LW_SUCCESS;
}



LWLINE *
lwline_removepoint(LWLINE *line, uint32_t index)
{
	POINTARRAY *newpa;
	LWLINE *ret;

	newpa = ptarray_removePoint(line->points, index);

	ret = lwline_construct(line->srid, NULL, newpa);
	lwgeom_add_bbox((LWGEOM *) ret);

	return ret;
}

/*
 * Note: input will be changed, make sure you have permissions for this.
 */
void
lwline_setPoint4d(LWLINE *line, uint32_t index, POINT4D *newpoint)
{
	ptarray_set_point4d(line->points, index, newpoint);
	/* Update the box, if there is one to update */
	if ( line->bbox )
	{
		lwgeom_drop_bbox((LWGEOM*)line);
		lwgeom_add_bbox((LWGEOM*)line);
	}
}

/**
* Re-write the measure ordinate (or add one, if it isn't already there) interpolating
* the measure between the supplied start and end values.
*/
LWLINE*
lwline_measured_from_lwline(const LWLINE *lwline, double m_start, double m_end)
{
	int i = 0;
	int hasm = 0, hasz = 0;
	int npoints = 0;
	double length = 0.0;
	double length_so_far = 0.0;
	double m_range = m_end - m_start;
	double m;
	POINTARRAY *pa = NULL;
	POINT3DZ p1, p2;

	if ( lwline->type != LINETYPE )
	{
		lwerror("lwline_construct_from_lwline: only line types supported");
		return NULL;
	}

	hasz = FLAGS_GET_Z(lwline->flags);
	hasm = 1;

	/* Null points or npoints == 0 will result in empty return geometry */
	if ( lwline->points )
	{
		npoints = lwline->points->npoints;
		length = ptarray_length_2d(lwline->points);
		getPoint3dz_p(lwline->points, 0, &p1);
	}

	pa = ptarray_construct(hasz, hasm, npoints);

	for ( i = 0; i < npoints; i++ )
	{
		POINT4D q;
		POINT2D a, b;
		getPoint3dz_p(lwline->points, i, &p2);
		a.x = p1.x;
		a.y = p1.y;
		b.x = p2.x;
		b.y = p2.y;
		length_so_far += distance2d_pt_pt(&a, &b);
		if ( length > 0.0 )
			m = m_start + m_range * length_so_far / length;
		/* #3172, support (valid) zero-length inputs */
		else if ( length == 0.0 && npoints > 1 )
			m = m_start + m_range * i / (npoints-1);
		else
			m = 0.0;
		q.x = p2.x;
		q.y = p2.y;
		q.z = p2.z;
		q.m = m;
		ptarray_set_point4d(pa, i, &q);
		p1 = p2;
	}

	return lwline_construct(lwline->srid, NULL, pa);
}

LWGEOM*
lwline_remove_repeated_points(const LWLINE *lwline, double tolerance)
{
	POINTARRAY* npts = ptarray_remove_repeated_points_minpoints(lwline->points, tolerance, 2);

	LWDEBUGF(3, "%s: npts %p", __func__, npts);

	return (LWGEOM*)lwline_construct(lwline->srid,
	                                 lwline->bbox ? gbox_copy(lwline->bbox) : 0,
	                                 npts);
}

int
lwline_is_closed(const LWLINE *line)
{
	if (FLAGS_GET_Z(line->flags))
		return ptarray_is_closed_3d(line->points);

	return ptarray_is_closed_2d(line->points);
}

int
lwline_is_trajectory(const LWLINE *line)
{
  POINT3DM p;
  int i, n;
  double m = -1 * FLT_MAX;

  if ( ! FLAGS_GET_M(line->flags) ) {
    lwnotice("Line does not have M dimension");
    return LW_FALSE;
  }

  n = line->points->npoints;
  if ( n < 2 ) return LW_TRUE; /* empty or single-point are "good" */

  for (i=0; i<n; ++i) {
    getPoint3dm_p(line->points, i, &p);
    if ( p.m <= m ) {
      lwnotice("Measure of vertex %d (%g) not bigger than measure of vertex %d (%g)",
        i, p.m, i-1, m);
      return LW_FALSE;
    }
    m = p.m;
  }

  return LW_TRUE;
}


LWLINE*
lwline_force_dims(const LWLINE *line, int hasz, int hasm)
{
	POINTARRAY *pdims = NULL;
	LWLINE *lineout;

	/* Return 2D empty */
	if( lwline_is_empty(line) )
	{
		lineout = lwline_construct_empty(line->srid, hasz, hasm);
	}
	else
	{
		pdims = ptarray_force_dims(line->points, hasz, hasm);
		lineout = lwline_construct(line->srid, NULL, pdims);
	}
	lineout->type = line->type;
	return lineout;
}

int lwline_is_empty(const LWLINE *line)
{
	if ( !line->points || line->points->npoints < 1 )
		return LW_TRUE;
	return LW_FALSE;
}


int lwline_count_vertices(LWLINE *line)
{
	assert(line);
	if ( ! line->points )
		return 0;
	return line->points->npoints;
}

LWLINE* lwline_simplify(const LWLINE *iline, double dist, int preserve_collapsed)
{
	static const int minvertices = 2; /* TODO: allow setting this */
	LWLINE *oline;
	POINTARRAY *pa;

	LWDEBUG(2, "function called");

	/* Skip empty case */
	if( lwline_is_empty(iline) )
		return NULL;

	pa = ptarray_simplify(iline->points, dist, minvertices);
	if ( ! pa ) return NULL;

	/* Make sure single-point collapses have two points */
	if ( pa->npoints == 1 )
	{
		/* Make sure single-point collapses have two points */
		if ( preserve_collapsed )
		{
			POINT4D pt;
			getPoint4d_p(pa, 0, &pt);
			ptarray_append_point(pa, &pt, LW_TRUE);
		}
		/* Return null for collapse */
		else
		{
			ptarray_free(pa);
			return NULL;
		}
	}

	oline = lwline_construct(iline->srid, NULL, pa);
	oline->type = iline->type;
	return oline;
}

double lwline_length(const LWLINE *line)
{
	if ( lwline_is_empty(line) )
		return 0.0;
	return ptarray_length(line->points);
}

double lwline_length_2d(const LWLINE *line)
{
	if ( lwline_is_empty(line) )
		return 0.0;
	return ptarray_length_2d(line->points);
}



LWLINE* lwline_grid(const LWLINE *line, const gridspec *grid)
{
	LWLINE *oline;
	POINTARRAY *opa;

	opa = ptarray_grid(line->points, grid);

	/* Skip line3d with less then 2 points */
	if ( opa->npoints < 2 ) return NULL;

	/* TODO: grid bounding box... */
	oline = lwline_construct(line->srid, NULL, opa);

	return oline;
}

POINTARRAY* lwline_interpolate_points(const LWLINE *line, double length_fraction, char repeat) {
	POINT4D pt;
	uint32_t i;
	uint32_t points_to_interpolate;
	uint32_t points_found = 0;
	double length;
	double length_fraction_increment = length_fraction;
	double length_fraction_consumed = 0;
	char has_z = (char) lwgeom_has_z(lwline_as_lwgeom(line));
	char has_m = (char) lwgeom_has_m(lwline_as_lwgeom(line));
	const POINTARRAY* ipa = line->points;
	POINTARRAY* opa;

	/* Empty.InterpolatePoint == Point Empty */
	if ( lwline_is_empty(line) )
	{
		return ptarray_construct_empty(has_z, has_m, 0);
	}

	/* If distance is one of the two extremes, return the point on that
	 * end rather than doing any computations
	 */
	if ( length_fraction == 0.0 || length_fraction == 1.0 )
	{
		if ( length_fraction == 0.0 )
			getPoint4d_p(ipa, 0, &pt);
		else
			getPoint4d_p(ipa, ipa->npoints-1, &pt);

		opa = ptarray_construct(has_z, has_m, 1);
		ptarray_set_point4d(opa, 0, &pt);

		return opa;
	}

	/* Interpolate points along the line */
	length = ptarray_length_2d(ipa);
	points_to_interpolate = repeat ? (uint32_t) floor(1 / length_fraction) : 1;
	opa = ptarray_construct(has_z, has_m, points_to_interpolate);

	const POINT2D* p1 = getPoint2d_cp(ipa, 0);
	for ( i = 0; i < ipa->npoints - 1 && points_found < points_to_interpolate; i++ )
	{
		const POINT2D* p2 = getPoint2d_cp(ipa, i+1);
		double segment_length_frac = distance2d_pt_pt(p1, p2) / length;

		/* If our target distance is before the total length we've seen
		 * so far. create a new point some distance down the current
		 * segment.
		 */
		while ( length_fraction < length_fraction_consumed + segment_length_frac && points_found < points_to_interpolate )
		{
			POINT4D p1_4d = getPoint4d(ipa, i);
			POINT4D p2_4d = getPoint4d(ipa, i+1);

			double segment_fraction = (length_fraction - length_fraction_consumed) / segment_length_frac;
			interpolate_point4d(&p1_4d, &p2_4d, &pt, segment_fraction);
			ptarray_set_point4d(opa, points_found++, &pt);
			length_fraction += length_fraction_increment;
		}

		length_fraction_consumed += segment_length_frac;

		p1 = p2;
	}

	/* Return the last point on the line. This shouldn't happen, but
	 * could if there's some floating point rounding errors. */
	if (points_found < points_to_interpolate) {
		getPoint4d_p(ipa, ipa->npoints - 1, &pt);
		ptarray_set_point4d(opa, points_found, &pt);
	}

    return opa;
}

