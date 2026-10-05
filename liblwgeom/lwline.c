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
		else if ( g->type == MULTILINETYPE )
		{
			LWMLINE *mline = lwgeom_as_lwmline(g);
			for ( uint32_t j = 0; j < mline->ngeoms; j++ )
			{
				LWLINE *line = mline->geoms[j];
				if (lwline_is_empty(line)) continue;
				ptarray_append_ptarray(pa, line->points, -1);
			}
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
* Re-write the measure coordinate (or add one, if it isn't already there) interpolating
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

uint32_t lwline_count_vertices(const LWLINE *line)
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

extern LWLINE *
lwline_extend(const LWLINE *line, double distance_forward, double distance_backward)
{
	POINTARRAY *pa, *opa;
	POINT4D p00, p01, p10, p11;
	POINT4D p_start, p_end;
	uint32_t i;
	bool forward = false, backward = false;

	if (distance_forward < 0 || distance_backward < 0)
		lwerror("%s: distances must be non-negative", __func__);

	if (!line || lwline_is_empty(line) || lwline_count_vertices(line) < 2)
	{
		lwerror("%s: line must have at least two points", __func__);
	}

	pa = line->points;
	if (distance_backward > 0.0)
	{
		i = 0;
		/* Get two distinct points at start of pointarray */
		getPoint4d_p(pa, i++, &p00);
		getPoint4d_p(pa, i, &p01);
		while(p4d_same(&p00, &p01))
		{
			if (i == pa->npoints - 1)
			{
				lwerror("%s: line must have at least two distinct points", __func__);
			}
			i++;
			getPoint4d_p(pa, i, &p01);
		}
		project_pt_pt(&p01, &p00, distance_backward, &p_start);
		backward = true;
	}

	if (distance_forward > 0.0)
	{
		i = pa->npoints - 1;
		/* Get two distinct points at end of pointarray */
		getPoint4d_p(pa, i--, &p10);
		getPoint4d_p(pa, i, &p11);
		while(p4d_same(&p10, &p11))
		{
			if (i == 0)
			{
				lwerror("%s: line must have at least two distinct points", __func__);
			}
			i--;
			getPoint4d_p(pa, i, &p11);
		}
		project_pt_pt(&p11, &p10, distance_forward, &p_end);
		forward = true;
	}

	opa = ptarray_construct_empty(ptarray_has_z(pa), ptarray_has_m(pa), pa->npoints + 2);

	if (backward)
	{
		ptarray_append_point(opa, &p_start, true);
	}
	ptarray_append_ptarray(opa, pa, -1.0);
	if (forward)
	{
		ptarray_append_point(opa, &p_end, true);
	}
	return lwline_construct(line->srid, NULL, opa);
}

/*
* The ways two line segments can intersect.
* Returned by lwsegment_intersection()
*/
enum LWSEGMENT_INTERSECTION
{
	/* The segments do not intersect */
	LWSEG_INTERSECT_NONE = 0,
	/* The segments are collinear and overlap for a positive length */
	LWSEG_INTERSECT_OVERLAP,
	/* The segments cross at a point interior to both of them */
	LWSEG_INTERSECT_CROSS,
	/* The segments touch at a single point, an endpoint of one of them */
	LWSEG_INTERSECT_POINT
};

/**
* Does the point lie on the segment, endpoints included?
*
* Unlike lw_pt_on_segment(), which excludes the segment endpoints.
*
* @param A1 first point of the segment
* @param A2 second point of the segment
* @param P the point to test
*
* @return LW_TRUE if P is on the A1/A2 segment
*/
static int
lw_pt_on_segment_inclusive(const POINT2D *A1, const POINT2D *A2, const POINT2D *P)
{
	if ( lw_segment_side(A1, A2, P) != 0 )
		return LW_FALSE;

	/* The point is on the segment line, check it's within its extent */
	return (P->x >= FP_MIN(A1->x, A2->x) && P->x <= FP_MAX(A1->x, A2->x) &&
	        P->y >= FP_MIN(A1->y, A2->y) && P->y <= FP_MAX(A1->y, A2->y));
}

/**
* How do the segment (P1,P2) and the segment (Q1,Q2) intersect?
*
* Only 2D coordinates are considered. Zero length segments are supported,
* they can only intersect at their single point.
*
* @param P1 first point of the first segment
* @param P2 second point of the first segment
* @param Q1 first point of the second segment
* @param Q2 second point of the second segment
* @param ipoint storage for the intersection point, set when the returned
*               value is #LWSEG_INTERSECT_POINT
*
* @return one of the #LWSEGMENT_INTERSECTION values
*/
static int
lwsegment_intersection(const POINT2D *P1,
                       const POINT2D *P2,
                       const POINT2D *Q1,
                       const POINT2D *Q2,
                       POINT2D *ipoint)
{
	int s11, s12, s21, s22;
	int zeroP, zeroQ;
	double len2, tQ1, tQ2, tmin, tmax;

	zeroP = P2D_SAME_STRICT(P1, P2);
	zeroQ = P2D_SAME_STRICT(Q1, Q2);

	/* Zero length segments, if they intersect, do it on their single point */
	if ( zeroP || zeroQ )
	{
		if ( zeroP && zeroQ )
		{
			if ( ! P2D_SAME_STRICT(P1, Q1) )
				return LWSEG_INTERSECT_NONE;
		}
		else if ( zeroP )
		{
			if ( ! lw_pt_on_segment_inclusive(Q1, Q2, P1) )
				return LWSEG_INTERSECT_NONE;
		}
		else
		{
			if ( ! lw_pt_on_segment_inclusive(P1, P2, Q1) )
				return LWSEG_INTERSECT_NONE;
		}

		*ipoint = zeroP ? *P1 : *Q1;
		return LWSEG_INTERSECT_POINT;
	}

	/* Are both endpoints of the second segment on the same side of the first? */
	s11 = lw_segment_side(P1, P2, Q1);
	s12 = lw_segment_side(P1, P2, Q2);
	if ( (s11 > 0 && s12 > 0) || (s11 < 0 && s12 < 0) )
		return LWSEG_INTERSECT_NONE;

	/* Are both endpoints of the first segment on the same side of the second? */
	s21 = lw_segment_side(Q1, Q2, P1);
	s22 = lw_segment_side(Q1, Q2, P2);
	if ( (s21 > 0 && s22 > 0) || (s21 < 0 && s22 < 0) )
		return LWSEG_INTERSECT_NONE;

	/*
	* Both endpoints of the second segment are on the line of the first,
	* the segments are collinear and the intersection is their overlap.
	*
	* Use the projection on the (P1,P2) vector as the segment parameter,
	* the overlap is the [0,len2] interval trimmed by the [tQ1,tQ2] one.
	*/
	if ( s11 == 0 && s12 == 0 )
	{
		len2 = (P2->x - P1->x) * (P2->x - P1->x) +
		       (P2->y - P1->y) * (P2->y - P1->y);
		tQ1 = (Q1->x - P1->x) * (P2->x - P1->x) +
		      (Q1->y - P1->y) * (P2->y - P1->y);
		tQ2 = (Q2->x - P1->x) * (P2->x - P1->x) +
		      (Q2->y - P1->y) * (P2->y - P1->y);

		tmin = FP_MAX(0.0, FP_MIN(tQ1, tQ2));
		tmax = FP_MIN(len2, FP_MAX(tQ1, tQ2));

		/* No overlap at all */
		if ( tmin > tmax )
			return LWSEG_INTERSECT_NONE;

		/* The segments are collinear and share a positive length */
		if ( tmin < tmax )
			return LWSEG_INTERSECT_OVERLAP;

		/* The segments touch on a single point, always a segment endpoint */
		if ( tmin == 0.0 )
			*ipoint = *P1;
		else if ( tmin == len2 )
			*ipoint = *P2;
		else if ( tmin == tQ1 )
			*ipoint = *Q1;
		else if ( tmin == tQ2 )
			*ipoint = *Q2;
		else
		{
			/* Rounding fallback, the point is one of the four endpoints */
			double t = tmin / len2;
			ipoint->x = P1->x + t * (P2->x - P1->x);
			ipoint->y = P1->y + t * (P2->y - P1->y);
		}
		return LWSEG_INTERSECT_POINT;
	}

	/*
	* The segments are not collinear, they intersect on a single point.
	* When that point is not a vertex of either segment it is interior
	* to both of them.
	*/
	if ( lw_pt_on_segment_inclusive(P1, P2, Q1) )
	{
		*ipoint = *Q1;
		return LWSEG_INTERSECT_POINT;
	}

	if ( lw_pt_on_segment_inclusive(P1, P2, Q2) )
	{
		*ipoint = *Q2;
		return LWSEG_INTERSECT_POINT;
	}

	if ( lw_pt_on_segment_inclusive(Q1, Q2, P1) )
	{
		*ipoint = *P1;
		return LWSEG_INTERSECT_POINT;
	}

	if ( lw_pt_on_segment_inclusive(Q1, Q2, P2) )
	{
		*ipoint = *P2;
		return LWSEG_INTERSECT_POINT;
	}

	return LWSEG_INTERSECT_CROSS;
}

/**
* Is the point a boundary point of the line, that is one of its endpoints?
*
* Those are the only points of a line that may be shared with another line
* without requiring any of them to be split.
*
* @param line the line to test the point against
* @param P the point to test
*
* @return LW_TRUE if P is the first or the last point of the line
*/
static int
lwline_point_is_boundary(const LWLINE *line, const POINT2D *P)
{
	const POINT2D *start = getPoint2d_cp(line->points, 0);
	const POINT2D *end = getPoint2d_cp(line->points, line->points->npoints - 1);

	return P2D_SAME_STRICT(P, start) || P2D_SAME_STRICT(P, end);
}

/**
* Do two lines intersect anywhere but on a point shared by both of their
* boundaries?
*
* Only 2D coordinates are considered.
*
* @param line1 first line
* @param line2 second line
*
* @return 1 if there's a non-boundary 2d intersection, 0 if there's NO
*         non-boundary 2d intersection
*/
int
lwline_have_nonboundary_2d_intersection(const LWLINE *line1, const LWLINE *line2)
{
	const POINTARRAY *pa1, *pa2;
	uint32_t i1, i2;
	POINT2D ipoint;

	if ( !line1 || !line2 )
		return LW_FALSE;

	pa1 = line1->points;
	pa2 = line2->points;

	/* Lines without segments can't intersect anything */
	if ( !pa1 || !pa2 || pa1->npoints < 2 || pa2->npoints < 2 )
		return LW_FALSE;

	for (i1 = 0; i1 < pa1->npoints - 1; ++i1)
	{
		const POINT2D *p1 = getPoint2d_cp(pa1, i1);
		const POINT2D *p2 = getPoint2d_cp(pa1, i1 + 1);

		for (i2 = 0; i2 < pa2->npoints - 1; ++i2)
		{
			const POINT2D *q1 = getPoint2d_cp(pa2, i2);
			const POINT2D *q2 = getPoint2d_cp(pa2, i2 + 1);
			int inter;

			inter = lwsegment_intersection(p1, p2, q1, q2, &ipoint);

			/* Crossings and overlaps are never boundary only */
			if ( inter == LWSEG_INTERSECT_CROSS ||
			     inter == LWSEG_INTERSECT_OVERLAP )
				return LW_TRUE;

			/*
			** A touch on a point which is a boundary point of both lines
			** is not a non-boundary intersection
			*/
			if ( inter == LWSEG_INTERSECT_POINT &&
			     ( ! lwline_point_is_boundary(line1, &ipoint) ||
			       ! lwline_point_is_boundary(line2, &ipoint) ) )
				return LW_TRUE;
		}
	}

	return LW_FALSE;
}
