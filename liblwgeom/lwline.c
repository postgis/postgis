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
lwline_construct(int32_t srid, GBOX *bbox, POINTARRAY *points)
{
	LWLINE *result = (LWLINE *)lwalloc(sizeof(LWLINE));
	result->type = LINETYPE;
	result->flags = points->flags;
	FLAGS_SET_BBOX(result->flags, bbox?1:0);
	result->srid = srid;
	result->points = points;
	result->bbox = bbox;
	return result;
}

LWLINE *
lwline_construct_empty(int32_t srid, char hasz, char hasm)
{
	LWLINE *result = lwalloc(sizeof(LWLINE));
	result->type = LINETYPE;
	result->flags = lwflags(hasz,hasm,0);
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


LWLINE *
lwline_segmentize2d(const LWLINE *line, double dist)
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
lwline_from_lwgeom_array(int32_t srid, uint32_t ngeoms, LWGEOM **geoms)
{
	uint32_t i;
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
lwline_from_ptarray(int32_t srid, uint32_t npoints, LWPOINT **points)
{
 	uint32_t i;
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
lwline_from_lwmpoint(int32_t srid, const LWMPOINT *mpoint)
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
lwline_get_lwpoint(const LWLINE *line, uint32_t where)
{
	POINT4D pt;
	LWPOINT *lwpoint;
	POINTARRAY *pa;

	if ( lwline_is_empty(line) || where >= line->points->npoints )
		return NULL;

	pa = ptarray_construct_empty(FLAGS_GET_Z(line->flags), FLAGS_GET_M(line->flags), 1);
	pt = getPoint4d(line->points, where);
	ptarray_append_point(pa, &pt, LW_TRUE);
	lwpoint = lwpoint_construct(line->srid, NULL, pa);
	return lwpoint;
}


int
lwline_add_lwpoint(LWLINE *line, LWPOINT *point, uint32_t where)
{
	POINT4D pt;
	getPoint4d_p(point->point, 0, &pt);

	if ( ptarray_insert_point(line->points, &pt, where) != LW_SUCCESS )
		return LW_FAILURE;

	/* Update the bounding box */
	if ( line->bbox )
	{
		lwgeom_refresh_bbox((LWGEOM*)line);
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
		lwgeom_refresh_bbox((LWGEOM*)line);
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
	return lwgeom_remove_repeated_points((LWGEOM*)lwline, tolerance);
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
	if (!FLAGS_GET_M(line->flags))
	{
		lwnotice("Line does not have M dimension");
		return LW_FALSE;
	}

	uint32_t n = line->points->npoints;

	if (n < 2)
		return LW_TRUE; /* empty or single-point are "good" */

	double m = -1 * FLT_MAX;
	for (uint32_t i = 0; i < n; ++i)
	{
		POINT3DM p;
		if (!getPoint3dm_p(line->points, i, &p))
			return LW_FALSE;
		if (p.m <= m)
		{
			lwnotice(
			    "Measure of vertex %d (%g) not bigger than measure of vertex %d (%g)", i, p.m, i - 1, m);
			return LW_FALSE;
		}
		m = p.m;
	}

	return LW_TRUE;
}

LWLINE*
lwline_force_dims(const LWLINE *line, int hasz, int hasm, double zval, double mval)
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
		pdims = ptarray_force_dims(line->points, hasz, hasm, zval, mval);
		lineout = lwline_construct(line->srid, NULL, pdims);
	}
	lineout->type = line->type;
	return lineout;
}

uint32_t lwline_count_vertices(LWLINE *line)
{
	assert(line);
	if ( ! line->points )
		return 0;
	return line->points->npoints;
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

extern LWPOINT *
lwline_interpolate_point_3d(const LWLINE *line, double distance)
{
	double length, slength, tlength;
	POINTARRAY *ipa;
	POINT4D pt;
	int nsegs, i;
	LWGEOM *geom = lwline_as_lwgeom(line);
	int has_z = lwgeom_has_z(geom);
	int has_m = lwgeom_has_m(geom);
	ipa = line->points;

	/* Empty.InterpolatePoint == Point Empty */
	if (lwline_is_empty(line))
	{
		return lwpoint_construct_empty(line->srid, has_z, has_m);
	}

	/* If distance is one of the two extremes, return the point on that
	 * end rather than doing any expensive computations
	 */
	if (distance == 0.0 || distance == 1.0)
	{
		if (distance == 0.0)
			getPoint4d_p(ipa, 0, &pt);
		else
			getPoint4d_p(ipa, ipa->npoints - 1, &pt);

		return lwpoint_make(line->srid, has_z, has_m, &pt);
	}

	/* Interpolate a point on the line */
	nsegs = ipa->npoints - 1;
	length = ptarray_length(ipa);
	tlength = 0;
	for (i = 0; i < nsegs; i++)
	{
		POINT4D p1, p2;
		POINT4D *p1ptr = &p1, *p2ptr = &p2; /* don't break
						     * strict-aliasing rules
						     */

		getPoint4d_p(ipa, i, &p1);
		getPoint4d_p(ipa, i + 1, &p2);

		/* Find the relative length of this segment */
		slength = distance3d_pt_pt((POINT3D *)p1ptr, (POINT3D *)p2ptr) / length;

		/* If our target distance is before the total length we've seen
		 * so far. create a new point some distance down the current
		 * segment.
		 */
		if (distance < tlength + slength)
		{
			double dseg = (distance - tlength) / slength;
			interpolate_point4d(&p1, &p2, &pt, dseg);
			return lwpoint_make(line->srid, has_z, has_m, &pt);
		}
		tlength += slength;
	}

	/* Return the last point on the line. This shouldn't happen, but
	 * could if there's some floating point rounding errors. */
	getPoint4d_p(ipa, ipa->npoints - 1, &pt);
	return lwpoint_make(line->srid, has_z, has_m, &pt);
}
