/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright (C) 2015 Sandro Santilli <strk@keybit.net>
 * Copyright (C) 2011 Paul Ramsey
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "liblwgeom_internal.h"
#include "lwgeom_log.h"
#include "measures3d.h"

static int
segment_locate_along(const POINT4D *p1, const POINT4D *p2, double m, double offset, POINT4D *pn)
{
	double m1 = p1->m;
	double m2 = p2->m;
	double mprop;

	/* M is out of range, no new point generated. */
	if ( (m < FP_MIN(m1,m2)) || (m > FP_MAX(m1,m2)) )
	{
		return LW_FALSE;
	}

	if( m1 == m2 )
	{
		/* Degenerate case: same M on both points.
		   If they are the same point we just return one of them. */
		if ( p4d_same(p1,p2) )
		{
			*pn = *p1;
			return LW_TRUE;
		}
		/* If the points are different we can out.
		   Correct behavior is probably an mprop of 0.5? */
		lwerror("Zero measure-length line encountered!");
		return LW_FALSE;
	}

	/* M is in range, new point to be generated. */
	mprop = (m - m1) / (m2 - m1);
	pn->x = p1->x + (p2->x - p1->x) * mprop;
	pn->y = p1->y + (p2->y - p1->y) * mprop;
	pn->z = p1->z + (p2->z - p1->z) * mprop;
	pn->m = m;

	/* Offset to the left or right, if necessary. */
	if ( offset != 0.0 )
	{
		double theta = atan2(p2->y - p1->y, p2->x - p1->x);
		pn->x -= sin(theta) * offset;
		pn->y += cos(theta) * offset;
	}

	return LW_TRUE;
}


static POINTARRAY*
ptarray_locate_along(const POINTARRAY *pa, double m, double offset)
{
	int i;
	POINT4D p1, p2, pn;
	POINTARRAY *dpa = NULL;

	/* Can't do anything with degenerate point arrays */
	if ( ! pa || pa->npoints < 2 ) return NULL;

	/* Walk through each segment in the point array */
	for ( i = 1; i < pa->npoints; i++ )
	{
		getPoint4d_p(pa, i-1, &p1);
		getPoint4d_p(pa, i, &p2);

		/* No derived point? Move to next segment. */
		if ( segment_locate_along(&p1, &p2, m, offset, &pn) == LW_FALSE )
			continue;

		/* No pointarray, make a fresh one */
		if ( dpa == NULL )
			dpa = ptarray_construct_empty(ptarray_has_z(pa), ptarray_has_m(pa), 8);

		/* Add our new point to the array */
		ptarray_append_point(dpa, &pn, 0);
	}

	return dpa;
}

static LWMPOINT*
lwline_locate_along(const LWLINE *lwline, double m, double offset)
{
	POINTARRAY *opa = NULL;
	LWMPOINT *mp = NULL;
	LWGEOM *lwg = lwline_as_lwgeom(lwline);
	int hasz, hasm, srid;

	/* Return degenerates upwards */
	if ( ! lwline ) return NULL;

	/* Create empty return shell */
	srid = lwgeom_get_srid(lwg);
	hasz = lwgeom_has_z(lwg);
	hasm = lwgeom_has_m(lwg);

	if ( hasm )
	{
		/* Find points along */
		opa = ptarray_locate_along(lwline->points, m, offset);
	}
	else
	{
		LWLINE *lwline_measured = lwline_measured_from_lwline(lwline, 0.0, 1.0);
		opa = ptarray_locate_along(lwline_measured->points, m, offset);
		lwline_free(lwline_measured);
	}

	/* Return NULL as EMPTY */
	if ( ! opa )
		return lwmpoint_construct_empty(srid, hasz, hasm);

	/* Convert pointarray into a multipoint */
	mp = lwmpoint_construct(srid, opa);
	ptarray_free(opa);
	return mp;
}

static LWMPOINT*
lwmline_locate_along(const LWMLINE *lwmline, double m, double offset)
{
	LWMPOINT *lwmpoint = NULL;
	LWGEOM *lwg = lwmline_as_lwgeom(lwmline);
	int i, j;

	/* Return degenerates upwards */
	if ( (!lwmline) || (lwmline->ngeoms < 1) ) return NULL;

	/* Construct return */
	lwmpoint = lwmpoint_construct_empty(lwgeom_get_srid(lwg), lwgeom_has_z(lwg), lwgeom_has_m(lwg));

	/* Locate along each sub-line */
	for ( i = 0; i < lwmline->ngeoms; i++ )
	{
		LWMPOINT *along = lwline_locate_along(lwmline->geoms[i], m, offset);
		if ( along )
		{
			if ( ! lwgeom_is_empty((LWGEOM*)along) )
			{
				for ( j = 0; j < along->ngeoms; j++ )
				{
					lwmpoint_add_lwpoint(lwmpoint, along->geoms[j]);
				}
			}
			/* Free the containing geometry, but leave the sub-geometries around */
			along->ngeoms = 0;
			lwmpoint_free(along);
		}
	}
	return lwmpoint;
}

static LWMPOINT*
lwpoint_locate_along(const LWPOINT *lwpoint, double m, double offset)
{
	double point_m = lwpoint_get_m(lwpoint);
	LWGEOM *lwg = lwpoint_as_lwgeom(lwpoint);
	LWMPOINT *r = lwmpoint_construct_empty(lwgeom_get_srid(lwg), lwgeom_has_z(lwg), lwgeom_has_m(lwg));
	if ( FP_EQUALS(m, point_m) )
	{
		lwmpoint_add_lwpoint(r, lwpoint_clone(lwpoint));
	}
	return r;
}

static LWMPOINT*
lwmpoint_locate_along(const LWMPOINT *lwin, double m, double offset)
{
	LWGEOM *lwg = lwmpoint_as_lwgeom(lwin);
	LWMPOINT *lwout = NULL;
	int i;

	/* Construct return */
	lwout = lwmpoint_construct_empty(lwgeom_get_srid(lwg), lwgeom_has_z(lwg), lwgeom_has_m(lwg));

	for ( i = 0; i < lwin->ngeoms; i++ )
	{
		double point_m = lwpoint_get_m(lwin->geoms[i]);
		if ( FP_EQUALS(m, point_m) )
		{
			lwmpoint_add_lwpoint(lwout, lwpoint_clone(lwin->geoms[i]));
		}
	}

	return lwout;
}

LWGEOM*
lwgeom_locate_along(const LWGEOM *lwin, double m, double offset)
{
	if ( ! lwin ) return NULL;

	if ( ! lwgeom_has_m(lwin) )
		lwerror("Input geometry does not have a measure dimension");

	switch (lwin->type)
	{
	case POINTTYPE:
		return (LWGEOM*)lwpoint_locate_along((LWPOINT*)lwin, m, offset);
	case MULTIPOINTTYPE:
		return (LWGEOM*)lwmpoint_locate_along((LWMPOINT*)lwin, m, offset);
	case LINETYPE:
		return (LWGEOM*)lwline_locate_along((LWLINE*)lwin, m, offset);
	case MULTILINETYPE:
		return (LWGEOM*)lwmline_locate_along((LWMLINE*)lwin, m, offset);
	/* Only line types supported right now */
	/* TO DO: CurveString, CompoundCurve, MultiCurve */
	/* TO DO: Point, MultiPoint */
	default:
		lwerror("Only linear geometries are supported, %s provided.",lwtype_name(lwin->type));
		return NULL;
	}
	return NULL;
}

/**
* Given a POINT4D and an ordinate number, return
* the value of the ordinate.
* @param p input point
* @param ordinate number (1=x, 2=y, 3=z, 4=m)
* @return d value at that ordinate
*/
double lwpoint_get_ordinate(const POINT4D *p, char ordinate)
{
	if ( ! p )
	{
		lwerror("Null input geometry.");
		return 0.0;
	}

	if ( ! ( ordinate == 'X' || ordinate == 'Y' || ordinate == 'Z' || ordinate == 'M' ) )
	{
		lwerror("Cannot extract %c ordinate.", ordinate);
		return 0.0;
	}

	if ( ordinate == 'X' )
		return p->x;
	if ( ordinate == 'Y' )
		return p->y;
	if ( ordinate == 'Z' )
		return p->z;
	if ( ordinate == 'M' )
		return p->m;

	/* X */
	return p->x;

}

/**
* Given a point, ordinate number and value, set that ordinate on the
* point.
*/
void lwpoint_set_ordinate(POINT4D *p, char ordinate, double value)
{
	if ( ! p )
	{
		lwerror("Null input geometry.");
		return;
	}

	if ( ! ( ordinate == 'X' || ordinate == 'Y' || ordinate == 'Z' || ordinate == 'M' ) )
	{
		lwerror("Cannot set %c ordinate.", ordinate);
		return;
	}

	LWDEBUGF(4, "    setting ordinate %c to %g", ordinate, value);

	switch ( ordinate )
	{
	case 'X':
		p->x = value;
		return;
	case 'Y':
		p->y = value;
		return;
	case 'Z':
		p->z = value;
		return;
	case 'M':
		p->m = value;
		return;
	}
}

/**
* Given two points, a dimensionality, an ordinate, and an interpolation value
* generate a new point that is proportionally between the input points,
* using the values in the provided dimension as the scaling factors.
*/
int point_interpolate(const POINT4D *p1, const POINT4D *p2, POINT4D *p, int hasz, int hasm, char ordinate, double interpolation_value)
{
	static char* dims = "XYZM";
	double p1_value = lwpoint_get_ordinate(p1, ordinate);
	double p2_value = lwpoint_get_ordinate(p2, ordinate);
	double proportion;
	int i = 0;

	if ( ! ( ordinate == 'X' || ordinate == 'Y' || ordinate == 'Z' || ordinate == 'M' ) )
	{
		lwerror("Cannot set %c ordinate.", ordinate);
		return 0;
	}

	if ( FP_MIN(p1_value, p2_value) > interpolation_value ||
	        FP_MAX(p1_value, p2_value) < interpolation_value )
	{
		lwerror("Cannot interpolate to a value (%g) not between the input points (%g, %g).", interpolation_value, p1_value, p2_value);
		return 0;
	}

	proportion = fabs((interpolation_value - p1_value) / (p2_value - p1_value));

	for ( i = 0; i < 4; i++ )
	{
		double newordinate = 0.0;
		if ( dims[i] == 'Z' && ! hasz ) continue;
		if ( dims[i] == 'M' && ! hasm ) continue;
		p1_value = lwpoint_get_ordinate(p1, dims[i]);
		p2_value = lwpoint_get_ordinate(p2, dims[i]);
		newordinate = p1_value + proportion * (p2_value - p1_value);
		lwpoint_set_ordinate(p, dims[i], newordinate);
		LWDEBUGF(4, "   clip ordinate(%c) p1_value(%g) p2_value(%g) proportion(%g) newordinate(%g) ", dims[i], p1_value, p2_value, proportion, newordinate );
	}

	return 1;
}


/**
* Clip an input POINT between two values, on any ordinate input.
*/
LWCOLLECTION*
lwpoint_clip_to_ordinate_range(const LWPOINT *point, char ordinate, double from, double to)
{
	LWCOLLECTION *lwgeom_out = NULL;
	char hasz, hasm;
	POINT4D p4d;
	double ordinate_value;

	/* Nothing to do with NULL */
	if ( ! point )
		lwerror("Null input geometry.");

	/* Ensure 'from' is less than 'to'. */
	if ( to < from )
	{
		double t = from;
		from = to;
		to = t;
	}

	/* Read Z/M info */
	hasz = lwgeom_has_z(lwpoint_as_lwgeom(point));
	hasm = lwgeom_has_m(lwpoint_as_lwgeom(point));

	/* Prepare return object */
	lwgeom_out = lwcollection_construct_empty(MULTIPOINTTYPE, point->srid, hasz, hasm);

	/* Test if ordinate is in range */
	lwpoint_getPoint4d_p(point, &p4d);
	ordinate_value = lwpoint_get_ordinate(&p4d, ordinate);
	if ( from <= ordinate_value && to >= ordinate_value )
	{
		LWPOINT *lwp = lwpoint_clone(point);
		lwcollection_add_lwgeom(lwgeom_out, lwpoint_as_lwgeom(lwp));
	}

	/* Set the bbox, if necessary */
	if ( lwgeom_out->bbox )
	{
		lwgeom_drop_bbox((LWGEOM*)lwgeom_out);
		lwgeom_add_bbox((LWGEOM*)lwgeom_out);
	}

	return lwgeom_out;
}



/**
* Clip an input MULTIPOINT between two values, on any ordinate input.
*/
LWCOLLECTION*
lwmpoint_clip_to_ordinate_range(const LWMPOINT *mpoint, char ordinate, double from, double to)
{
	LWCOLLECTION *lwgeom_out = NULL;
	char hasz, hasm;
	int i;

	/* Nothing to do with NULL */
	if ( ! mpoint )
		lwerror("Null input geometry.");

	/* Ensure 'from' is less than 'to'. */
	if ( to < from )
	{
		double t = from;
		from = to;
		to = t;
	}

	/* Read Z/M info */
	hasz = lwgeom_has_z(lwmpoint_as_lwgeom(mpoint));
	hasm = lwgeom_has_m(lwmpoint_as_lwgeom(mpoint));

	/* Prepare return object */
	lwgeom_out = lwcollection_construct_empty(MULTIPOINTTYPE, mpoint->srid, hasz, hasm);

	/* For each point, is its ordinate value between from and to? */
	for ( i = 0; i < mpoint->ngeoms; i ++ )
	{
		POINT4D p4d;
		double ordinate_value;

		lwpoint_getPoint4d_p(mpoint->geoms[i], &p4d);
		ordinate_value = lwpoint_get_ordinate(&p4d, ordinate);

		if ( from <= ordinate_value && to >= ordinate_value )
		{
			LWPOINT *lwp = lwpoint_clone(mpoint->geoms[i]);
			lwcollection_add_lwgeom(lwgeom_out, lwpoint_as_lwgeom(lwp));
		}
	}

	/* Set the bbox, if necessary */
	if ( lwgeom_out->bbox )
	{
		lwgeom_drop_bbox((LWGEOM*)lwgeom_out);
		lwgeom_add_bbox((LWGEOM*)lwgeom_out);
	}

	return lwgeom_out;
}

/**
* Clip an input MULTILINESTRING between two values, on any ordinate input.
*/
LWCOLLECTION*
lwmline_clip_to_ordinate_range(const LWMLINE *mline, char ordinate, double from, double to)
{
	LWCOLLECTION *lwgeom_out = NULL;

	if ( ! mline )
	{
		lwerror("Null input geometry.");
		return NULL;
	}

	if ( mline->ngeoms == 1)
	{
		lwgeom_out = lwline_clip_to_ordinate_range(mline->geoms[0], ordinate, from, to);
	}
	else
	{
		LWCOLLECTION *col;
		char hasz = lwgeom_has_z(lwmline_as_lwgeom(mline));
		char hasm = lwgeom_has_m(lwmline_as_lwgeom(mline));
		int i, j;
		char homogeneous = 1;
		size_t geoms_size = 0;
		lwgeom_out = lwcollection_construct_empty(MULTILINETYPE, mline->srid, hasz, hasm);
		FLAGS_SET_Z(lwgeom_out->flags, hasz);
		FLAGS_SET_M(lwgeom_out->flags, hasm);
		for ( i = 0; i < mline->ngeoms; i ++ )
		{
			col = lwline_clip_to_ordinate_range(mline->geoms[i], ordinate, from, to);
			if ( col )
			{
				/* Something was left after the clip. */
				if ( lwgeom_out->ngeoms + col->ngeoms > geoms_size )
				{
					geoms_size += 16;
					if ( lwgeom_out->geoms )
					{
						lwgeom_out->geoms = lwrealloc(lwgeom_out->geoms, geoms_size * sizeof(LWGEOM*));
					}
					else
					{
						lwgeom_out->geoms = lwalloc(geoms_size * sizeof(LWGEOM*));
					}
				}
				for ( j = 0; j < col->ngeoms; j++ )
				{
					lwgeom_out->geoms[lwgeom_out->ngeoms] = col->geoms[j];
					lwgeom_out->ngeoms++;
				}
				if ( col->type != mline->type )
				{
					homogeneous = 0;
				}
				/* Shallow free the struct, leaving the geoms behind. */
				if ( col->bbox ) lwfree(col->bbox);
				lwfree(col->geoms);
				lwfree(col);
			}
		}
		if ( lwgeom_out->bbox )
		{
			lwgeom_drop_bbox((LWGEOM*)lwgeom_out);
			lwgeom_add_bbox((LWGEOM*)lwgeom_out);
		}

		if ( ! homogeneous )
		{
			lwgeom_out->type = COLLECTIONTYPE;
		}
	}

	if ( ! lwgeom_out || lwgeom_out->ngeoms == 0 ) /* Nothing left after clip. */
	{
		return NULL;
	}

	return lwgeom_out;

}


/**
* Take in a LINESTRING and return a MULTILINESTRING of those portions of the
* LINESTRING between the from/to range for the specified ordinate (XYZM)
*/
LWCOLLECTION*
lwline_clip_to_ordinate_range(const LWLINE *line, char ordinate, double from, double to)
{

	POINTARRAY *pa_in = NULL;
	LWCOLLECTION *lwgeom_out = NULL;
	POINTARRAY *dp = NULL;
	int i;
	int added_last_point = 0;
	POINT4D *p = NULL, *q = NULL, *r = NULL;
	double ordinate_value_p = 0.0, ordinate_value_q = 0.0;
	char hasz = lwgeom_has_z(lwline_as_lwgeom(line));
	char hasm = lwgeom_has_m(lwline_as_lwgeom(line));
	char dims = FLAGS_NDIMS(line->flags);

	/* Null input, nothing we can do. */
	if ( ! line )
	{
		lwerror("Null input geometry.");
		return NULL;
	}

	/* Ensure 'from' is less than 'to'. */
	if ( to < from )
	{
		double t = from;
		from = to;
		to = t;
	}

	LWDEBUGF(4, "from = %g, to = %g, ordinate = %c", from, to, ordinate);
	LWDEBUGF(4, "%s", lwgeom_to_ewkt((LWGEOM*)line));

	/* Asking for an ordinate we don't have. Error. */
	if ( (ordinate == 'Z' && ! hasz) || (ordinate == 'M' && ! hasm) )
	{
		lwerror("Cannot clip on ordinate %d in a %d-d geometry.", ordinate, dims);
		return NULL;
	}

	/* Prepare our working point objects. */
	p = lwalloc(sizeof(POINT4D));
	q = lwalloc(sizeof(POINT4D));
	r = lwalloc(sizeof(POINT4D));

	/* Construct a collection to hold our outputs. */
	lwgeom_out = lwcollection_construct_empty(MULTILINETYPE, line->srid, hasz, hasm);

	/* Get our input point array */
	pa_in = line->points;

	for ( i = 0; i < pa_in->npoints; i++ )
	{
		LWDEBUGF(4, "Point #%d", i);
		LWDEBUGF(4, "added_last_point %d", added_last_point);
		if ( i > 0 )
		{
			*q = *p;
			ordinate_value_q = ordinate_value_p;
		}
		getPoint4d_p(pa_in, i, p);
		ordinate_value_p = lwpoint_get_ordinate(p, ordinate);
		LWDEBUGF(4, " ordinate_value_p %g (current)", ordinate_value_p);
		LWDEBUGF(4, " ordinate_value_q %g (previous)", ordinate_value_q);

		/* Is this point inside the ordinate range? Yes. */
		if ( ordinate_value_p >= from && ordinate_value_p <= to )
		{
			LWDEBUGF(4, " inside ordinate range (%g, %g)", from, to);

			if ( ! added_last_point )
			{
				LWDEBUG(4,"  new ptarray required");
				/* We didn't add the previous point, so this is a new segment.
				*  Make a new point array. */
				dp = ptarray_construct_empty(hasz, hasm, 32);

				/* We're transiting into the range so add an interpolated
				*  point at the range boundary.
				*  If we're on a boundary and crossing from the far side,
				*  we also need an interpolated point. */
				if ( i > 0 && ( /* Don't try to interpolate if this is the first point */
				            ( ordinate_value_p > from && ordinate_value_p < to ) || /* Inside */
				            ( ordinate_value_p == from && ordinate_value_q > to ) || /* Hopping from above */
				            ( ordinate_value_p == to && ordinate_value_q < from ) ) ) /* Hopping from below */
				{
					double interpolation_value;
					(ordinate_value_q > to) ? (interpolation_value = to) : (interpolation_value = from);
					point_interpolate(q, p, r, hasz, hasm, ordinate, interpolation_value);
					ptarray_append_point(dp, r, LW_FALSE);
					LWDEBUGF(4, "[0] interpolating between (%g, %g) with interpolation point (%g)", ordinate_value_q, ordinate_value_p, interpolation_value);
				}
			}
			/* Add the current vertex to the point array. */
			ptarray_append_point(dp, p, LW_FALSE);
			if ( ordinate_value_p == from || ordinate_value_p == to )
			{
				added_last_point = 2; /* Added on boundary. */
			}
			else
			{
				added_last_point = 1; /* Added inside range. */
			}
		}
		/* Is this point inside the ordinate range? No. */
		else
		{
			LWDEBUGF(4, "  added_last_point (%d)", added_last_point);
			if ( added_last_point == 1 )
			{
				/* We're transiting out of the range, so add an interpolated point
				*  to the point array at the range boundary. */
				double interpolation_value;
				(ordinate_value_p > to) ? (interpolation_value = to) : (interpolation_value = from);
				point_interpolate(q, p, r, hasz, hasm, ordinate, interpolation_value);
				ptarray_append_point(dp, r, LW_FALSE);
				LWDEBUGF(4, " [1] interpolating between (%g, %g) with interpolation point (%g)", ordinate_value_q, ordinate_value_p, interpolation_value);
			}
			else if ( added_last_point == 2 )
			{
				/* We're out and the last point was on the boundary.
				*  If the last point was the near boundary, nothing to do.
				*  If it was the far boundary, we need an interpolated point. */
				if ( from != to && (
				            (ordinate_value_q == from && ordinate_value_p > from) ||
				            (ordinate_value_q == to && ordinate_value_p < to) ) )
				{
					double interpolation_value;
					(ordinate_value_p > to) ? (interpolation_value = to) : (interpolation_value = from);
					point_interpolate(q, p, r, hasz, hasm, ordinate, interpolation_value);
					ptarray_append_point(dp, r, LW_FALSE);
					LWDEBUGF(4, " [2] interpolating between (%g, %g) with interpolation point (%g)", ordinate_value_q, ordinate_value_p, interpolation_value);
				}
			}
			else if ( i && ordinate_value_q < from && ordinate_value_p > to )
			{
				/* We just hopped over the whole range, from bottom to top,
				*  so we need to add *two* interpolated points! */
				dp = ptarray_construct(hasz, hasm, 2);
				/* Interpolate lower point. */
				point_interpolate(p, q, r, hasz, hasm, ordinate, from);
				ptarray_set_point4d(dp, 0, r);
				/* Interpolate upper point. */
				point_interpolate(p, q, r, hasz, hasm, ordinate, to);
				ptarray_set_point4d(dp, 1, r);
			}
			else if ( i && ordinate_value_q > to && ordinate_value_p < from )
			{
				/* We just hopped over the whole range, from top to bottom,
				*  so we need to add *two* interpolated points! */
				dp = ptarray_construct(hasz, hasm, 2);
				/* Interpolate upper point. */
				point_interpolate(p, q, r, hasz, hasm, ordinate, to);
				ptarray_set_point4d(dp, 0, r);
				/* Interpolate lower point. */
				point_interpolate(p, q, r, hasz, hasm, ordinate, from);
				ptarray_set_point4d(dp, 1, r);
			}
			/* We have an extant point-array, save it out to a multi-line. */
			if ( dp )
			{
				LWDEBUG(4, "saving pointarray to multi-line (1)");

				/* Only one point, so we have to make an lwpoint to hold this
				*  and set the overall output type to a generic collection. */
				if ( dp->npoints == 1 )
				{
					LWPOINT *opoint = lwpoint_construct(line->srid, NULL, dp);
					lwgeom_out->type = COLLECTIONTYPE;
					lwgeom_out = lwcollection_add_lwgeom(lwgeom_out, lwpoint_as_lwgeom(opoint));

				}
				else
				{
					LWLINE *oline = lwline_construct(line->srid, NULL, dp);
					lwgeom_out = lwcollection_add_lwgeom(lwgeom_out, lwline_as_lwgeom(oline));
				}

				/* Pointarray is now owned by lwgeom_out, so drop reference to it */
				dp = NULL;
			}
			added_last_point = 0;

		}
	}

	/* Still some points left to be saved out. */
	if ( dp && dp->npoints > 0 )
	{
		LWDEBUG(4, "saving pointarray to multi-line (2)");
		LWDEBUGF(4, "dp->npoints == %d", dp->npoints);
		LWDEBUGF(4, "lwgeom_out->ngeoms == %d", lwgeom_out->ngeoms);

		if ( dp->npoints == 1 )
		{
			LWPOINT *opoint = lwpoint_construct(line->srid, NULL, dp);
			lwgeom_out->type = COLLECTIONTYPE;
			lwgeom_out = lwcollection_add_lwgeom(lwgeom_out, lwpoint_as_lwgeom(opoint));
		}
		else
		{
			LWLINE *oline = lwline_construct(line->srid, NULL, dp);
			lwgeom_out = lwcollection_add_lwgeom(lwgeom_out, lwline_as_lwgeom(oline));
		}

		/* Pointarray is now owned by lwgeom_out, so drop reference to it */
		dp = NULL;
	}

	lwfree(p);
	lwfree(q);
	lwfree(r);

	if ( lwgeom_out->bbox && lwgeom_out->ngeoms > 0 )
	{
		lwgeom_drop_bbox((LWGEOM*)lwgeom_out);
		lwgeom_add_bbox((LWGEOM*)lwgeom_out);
	}

	return lwgeom_out;

}

LWCOLLECTION*
lwgeom_clip_to_ordinate_range(const LWGEOM *lwin, char ordinate, double from, double to, double offset)
{
	LWCOLLECTION *out_col;
	LWCOLLECTION *out_offset;
	int i;

	if ( ! lwin )
		lwerror("lwgeom_clip_to_ordinate_range: null input geometry!");

	switch ( lwin->type )
	{
	case LINETYPE:
		out_col = lwline_clip_to_ordinate_range((LWLINE*)lwin, ordinate, from, to);
		break;
	case MULTILINETYPE:
		out_col = lwmline_clip_to_ordinate_range((LWMLINE*)lwin, ordinate, from, to);
		break;
	case MULTIPOINTTYPE:
		out_col = lwmpoint_clip_to_ordinate_range((LWMPOINT*)lwin, ordinate, from, to);
		break;
	case POINTTYPE:
		out_col = lwpoint_clip_to_ordinate_range((LWPOINT*)lwin, ordinate, from, to);
		break;
	default:
		lwerror("This function does not accept %s geometries.", lwtype_name(lwin->type));
		return NULL;;
	}

	/* Stop if result is NULL */
	if ( out_col == NULL )
		lwerror("lwgeom_clip_to_ordinate_range clipping routine returned NULL");

	/* Return if we aren't going to offset the result */
	if ( FP_EQUALS(offset, 0.0) || lwgeom_is_empty(lwcollection_as_lwgeom(out_col)) )
		return out_col;

	/* Construct a collection to hold our outputs. */
	/* Things get ugly: GEOS offset drops Z's and M's so we have to drop ours */
	out_offset = lwcollection_construct_empty(MULTILINETYPE, lwin->srid, 0, 0);

	/* Try and offset the linear portions of the return value */
	for ( i = 0; i < out_col->ngeoms; i++ )
	{
		int type = out_col->geoms[i]->type;
		if ( type == POINTTYPE )
		{
			lwnotice("lwgeom_clip_to_ordinate_range cannot offset a clipped point");
			continue;
		}
		else if ( type == LINETYPE )
		{
			/* lwgeom_offsetcurve(line, offset, quadsegs, joinstyle (round), mitrelimit) */
			LWGEOM *lwoff = lwgeom_offsetcurve(lwgeom_as_lwline(out_col->geoms[i]), offset, 8, 1, 5.0);
			if ( ! lwoff )
			{
				lwerror("lwgeom_offsetcurve returned null");
			}
			lwcollection_add_lwgeom(out_offset, lwoff);
		}
		else
		{
			lwerror("lwgeom_clip_to_ordinate_range found an unexpected type (%s) in the offset routine",lwtype_name(type));
		}
	}

	return out_offset;
}

LWCOLLECTION*
lwgeom_locate_between(const LWGEOM *lwin, double from, double to, double offset)
{
	if ( ! lwgeom_has_m(lwin) )
		lwerror("Input geometry does not have a measure dimension");

	return lwgeom_clip_to_ordinate_range(lwin, 'M', from, to, offset);
}

double
lwgeom_interpolate_point(const LWGEOM *lwin, const LWPOINT *lwpt)
{
	POINT4D p, p_proj;
	double ret = 0.0;

	if ( ! lwin )
		lwerror("lwgeom_interpolate_point: null input geometry!");

	if ( ! lwgeom_has_m(lwin) )
		lwerror("Input geometry does not have a measure dimension");

	if ( lwgeom_is_empty(lwin) || lwpoint_is_empty(lwpt) )
		lwerror("Input geometry is empty");

	switch ( lwin->type )
	{
	case LINETYPE:
	{
		LWLINE *lwline = lwgeom_as_lwline(lwin);
		lwpoint_getPoint4d_p(lwpt, &p);
		ret = ptarray_locate_point(lwline->points, &p, NULL, &p_proj);
		ret = p_proj.m;
		break;
	}
	default:
		lwerror("This function does not accept %s geometries.", lwtype_name(lwin->type));
	}
	return ret;
}

/*
 * Time of closest point of approach
 *
 * Given two vectors (p1-p2 and q1-q2) and
 * a time range (t1-t2) return the time in which
 * a point p is closest to a point q on their
 * respective vectors, and the actual points
 *
 * Here we use algorithm from softsurfer.com
 * that can be found here
 * http://softsurfer.com/Archive/algorithm_0106/algorithm_0106.htm
 *
 * @param p0 start of first segment, will be set to actual
 *           closest point of approach on segment.
 * @param p1 end of first segment
 * @param q0 start of second segment, will be set to actual
 *           closest point of approach on segment.
 * @param q1 end of second segment
 * @param t0 start of travel time
 * @param t1 end of travel time
 *
 * @return time of closest point of approach
 *
 */
static double
segments_tcpa(POINT4D* p0, const POINT4D* p1,
              POINT4D* q0, const POINT4D* q1,
              double t0, double t1)
{
	POINT3DZ pv; /* velocity of p, aka u */
	POINT3DZ qv; /* velocity of q, aka v */
	POINT3DZ dv; /* velocity difference */
	POINT3DZ w0; /* vector between first points */

	/*
	  lwnotice("FROM %g,%g,%g,%g -- %g,%g,%g,%g",
	    p0->x, p0->y, p0->z, p0->m,
	    p1->x, p1->y, p1->z, p1->m);
	  lwnotice("  TO %g,%g,%g,%g -- %g,%g,%g,%g",
	    q0->x, q0->y, q0->z, q0->m,
	    q1->x, q1->y, q1->z, q1->m);
	*/

	/* PV aka U */
	pv.x = ( p1->x - p0->x );
	pv.y = ( p1->y - p0->y );
	pv.z = ( p1->z - p0->z );
	/*lwnotice("PV:  %g, %g, %g", pv.x, pv.y, pv.z);*/

	/* QV aka V */
	qv.x = ( q1->x - q0->x );
	qv.y = ( q1->y - q0->y );
	qv.z = ( q1->z - q0->z );
	/*lwnotice("QV:  %g, %g, %g", qv.x, qv.y, qv.z);*/

	dv.x = pv.x - qv.x;
	dv.y = pv.y - qv.y;
	dv.z = pv.z - qv.z;
	/*lwnotice("DV:  %g, %g, %g", dv.x, dv.y, dv.z);*/

	double dv2 = DOT(dv,dv);
	/*lwnotice("DOT: %g", dv2);*/

	if ( dv2 == 0.0 )
	{
		/* Distance is the same at any time, we pick the earliest */
		return t0;
	}

	/* Distance at any given time, with t0 */
	w0.x = ( p0->x - q0->x );
	w0.y = ( p0->y - q0->y );
	w0.z = ( p0->z - q0->z );

	/*lwnotice("W0:  %g, %g, %g", w0.x, w0.y, w0.z);*/

	/* Check that at distance dt w0 is distance */

	/* This is the fraction of measure difference */
	double t = -DOT(w0,dv) / dv2;
	/*lwnotice("CLOSEST TIME (fraction): %g", t);*/

	if ( t > 1.0 )
	{
		/* Getting closer as we move to the end */
		/*lwnotice("Converging");*/
		t = 1;
	}
	else if ( t < 0.0 )
	{
		/*lwnotice("Diverging");*/
		t = 0;
	}

	/* Interpolate the actual points now */

	p0->x += pv.x * t;
	p0->y += pv.y * t;
	p0->z += pv.z * t;

	q0->x += qv.x * t;
	q0->y += qv.y * t;
	q0->z += qv.z * t;

	t = t0 + (t1 - t0) * t;
	/*lwnotice("CLOSEST TIME (real): %g", t);*/

	return t;
}

static int
ptarray_collect_mvals(const POINTARRAY *pa, double tmin, double tmax, double *mvals)
{
	POINT4D pbuf;
	int i, n=0;
	for (i=0; i<pa->npoints; ++i)
	{
		getPoint4d_p(pa, i, &pbuf); /* could be optimized */
		if ( pbuf.m >= tmin && pbuf.m <= tmax )
			mvals[n++] = pbuf.m;
	}
	return n;
}

static int
compare_double(const void *pa, const void *pb)
{
	double a = *((double *)pa);
	double b = *((double *)pb);
	if ( a < b )
		return -1;
	else if ( a > b )
		return 1;
	else
		return 0;
}

/* Return number of elements in unique array */
static int
uniq(double *vals, int nvals)
{
	int i, last=0;
	for (i=1; i<nvals; ++i)
	{
		// lwnotice("(I%d):%g", i, vals[i]);
		if ( vals[i] != vals[last] )
		{
			vals[++last] = vals[i];
			// lwnotice("(O%d):%g", last, vals[last]);
		}
	}
	return last+1;
}

/*
 * Find point at a given measure
 *
 * The function assumes measures are linear so that always a single point
 * is returned for a single measure.
 *
 * @param pa the point array to perform search on
 * @param m the measure to search for
 * @param p the point to write result into
 * @param from the segment number to start from
 *
 * @return the segment number the point was found into
 *         or -1 if given measure was out of the known range.
 */
static int
ptarray_locate_along_linear(const POINTARRAY *pa, double m, POINT4D *p, int from)
{
	int i = from;
	POINT4D p1, p2;

	/* Walk through each segment in the point array */
	getPoint4d_p(pa, i, &p1);
	for ( i = from+1; i < pa->npoints; i++ )
	{
		getPoint4d_p(pa, i, &p2);

		if ( segment_locate_along(&p1, &p2, m, 0, p) == LW_TRUE )
			return i-1; /* found */

		p1 = p2;
	}

	return -1; /* not found */
}

double
lwgeom_tcpa(const LWGEOM *g1, const LWGEOM *g2, double *mindist)
{
	LWLINE *l1, *l2;
	int i;
	const GBOX *gbox1, *gbox2;
	double tmin, tmax;
	double *mvals;
	int nmvals = 0;
	double mintime;
	double mindist2 = FLT_MAX; /* minimum distance, squared */

	if ( ! lwgeom_has_m(g1) || ! lwgeom_has_m(g2) )
	{
		lwerror("Both input geometries must have a measure dimension");
		return -1;
	}

	l1 = lwgeom_as_lwline(g1);
	l2 = lwgeom_as_lwline(g2);

	if ( ! l1 || ! l2 )
	{
		lwerror("Both input geometries must be linestrings");
		return -1;
	}

	if ( l1->points->npoints < 2 || l2->points->npoints < 2 )
	{
		lwerror("Both input lines must have at least 2 points");
		return -1;
	}

	/* WARNING: these ranges may be wider than real ones */
	gbox1 = lwgeom_get_bbox(g1);
	gbox2 = lwgeom_get_bbox(g2);

	assert(gbox1); /* or the npoints check above would have failed */
	assert(gbox2); /* or the npoints check above would have failed */

	/*
	 * Find overlapping M range
	 * WARNING: may be larger than the real one
	 */

	tmin = FP_MAX(gbox1->mmin, gbox2->mmin);
	tmax = FP_MIN(gbox1->mmax, gbox2->mmax);

	if ( tmax < tmin )
	{
		LWDEBUG(1, "Inputs never exist at the same time");
		return -2;
	}

	// lwnotice("Min:%g, Max:%g", tmin, tmax);

	/*
	 * Collect M values in common time range from inputs
	 */

	mvals = lwalloc( sizeof(double) *
	                 ( l1->points->npoints + l2->points->npoints ) );

	/* TODO: also clip the lines ? */
	nmvals  = ptarray_collect_mvals(l1->points, tmin, tmax, mvals);
	nmvals += ptarray_collect_mvals(l2->points, tmin, tmax, mvals + nmvals);

	/* Sort values in ascending order */
	qsort(mvals, nmvals, sizeof(double), compare_double);

	/* Remove duplicated values */
	nmvals = uniq(mvals, nmvals);

	if ( nmvals < 2 )
	{
		{
			/* there's a single time, must be that one... */
			double t0 = mvals[0];
			POINT4D p0, p1;
			LWDEBUGF(1, "Inputs only exist both at a single time (%g)", t0);
			if ( mindist )
			{
				if ( -1 == ptarray_locate_along_linear(l1->points, t0, &p0, 0) )
				{
					lwfree(mvals);
					lwerror("Could not find point with M=%g on first geom", t0);
					return -1;
				}
				if ( -1 == ptarray_locate_along_linear(l2->points, t0, &p1, 0) )
				{
					lwfree(mvals);
					lwerror("Could not find point with M=%g on second geom", t0);
					return -1;
				}
				*mindist = distance3d_pt_pt((POINT3D*)&p0, (POINT3D*)&p1);
			}
			lwfree(mvals);
			return t0;
		}
	}

	/*
	 * For each consecutive pair of measures, compute time of closest point
	 * approach and actual distance between points at that time
	 */
	mintime = tmin;
	for (i=1; i<nmvals; ++i)
	{
		double t0 = mvals[i-1];
		double t1 = mvals[i];
		double t;
		POINT4D p0, p1, q0, q1;
		int seg;
		double dist2;

		// lwnotice("T %g-%g", t0, t1);

		seg = ptarray_locate_along_linear(l1->points, t0, &p0, 0);
		if ( -1 == seg ) continue; /* possible, if GBOX is approximated */
		// lwnotice("Measure %g on segment %d of line 1: %g, %g, %g", t0, seg, p0.x, p0.y, p0.z);

		seg = ptarray_locate_along_linear(l1->points, t1, &p1, seg);
		if ( -1 == seg ) continue; /* possible, if GBOX is approximated */
		// lwnotice("Measure %g on segment %d of line 1: %g, %g, %g", t1, seg, p1.x, p1.y, p1.z);

		seg = ptarray_locate_along_linear(l2->points, t0, &q0, 0);
		if ( -1 == seg ) continue; /* possible, if GBOX is approximated */
		// lwnotice("Measure %g on segment %d of line 2: %g, %g, %g", t0, seg, q0.x, q0.y, q0.z);

		seg = ptarray_locate_along_linear(l2->points, t1, &q1, seg);
		if ( -1 == seg ) continue; /* possible, if GBOX is approximated */
		// lwnotice("Measure %g on segment %d of line 2: %g, %g, %g", t1, seg, q1.x, q1.y, q1.z);

		t = segments_tcpa(&p0, &p1, &q0, &q1, t0, t1);

		/*
		lwnotice("Closest points: %g,%g,%g and %g,%g,%g at time %g",
		p0.x, p0.y, p0.z,
		q0.x, q0.y, q0.z, t);
		*/

		dist2 = ( q0.x - p0.x ) * ( q0.x - p0.x ) +
		        ( q0.y - p0.y ) * ( q0.y - p0.y ) +
		        ( q0.z - p0.z ) * ( q0.z - p0.z );
		if ( dist2 < mindist2 )
		{
			mindist2 = dist2;
			mintime = t;
			// lwnotice("MINTIME: %g", mintime);
		}
	}

	/*
	 * Release memory
	 */

	lwfree(mvals);

	if ( mindist )
	{
		*mindist = sqrt(mindist2);
	}
	/*lwnotice("MINDIST: %g", sqrt(mindist2));*/

	return mintime;
}

int
lwgeom_cpa_within(const LWGEOM *g1, const LWGEOM *g2, double maxdist)
{
	LWLINE *l1, *l2;
	int i;
	const GBOX *gbox1, *gbox2;
	double tmin, tmax;
	double *mvals;
	int nmvals = 0;
	double maxdist2 = maxdist * maxdist;
	int within = LW_FALSE;

	if ( ! lwgeom_has_m(g1) || ! lwgeom_has_m(g2) )
	{
		lwerror("Both input geometries must have a measure dimension");
		return LW_FALSE;
	}

	l1 = lwgeom_as_lwline(g1);
	l2 = lwgeom_as_lwline(g2);

	if ( ! l1 || ! l2 )
	{
		lwerror("Both input geometries must be linestrings");
		return LW_FALSE;
	}

	if ( l1->points->npoints < 2 || l2->points->npoints < 2 )
	{
		/* TODO: return distance between these two points */
		lwerror("Both input lines must have at least 2 points");
		return LW_FALSE;
	}

	/* WARNING: these ranges may be wider than real ones */
	gbox1 = lwgeom_get_bbox(g1);
	gbox2 = lwgeom_get_bbox(g2);

	assert(gbox1); /* or the npoints check above would have failed */
	assert(gbox2); /* or the npoints check above would have failed */

	/*
	 * Find overlapping M range
	 * WARNING: may be larger than the real one
	 */

	tmin = FP_MAX(gbox1->mmin, gbox2->mmin);
	tmax = FP_MIN(gbox1->mmax, gbox2->mmax);

	if ( tmax < tmin )
	{
		LWDEBUG(1, "Inputs never exist at the same time");
		return LW_FALSE;
	}

	// lwnotice("Min:%g, Max:%g", tmin, tmax);

	/*
	 * Collect M values in common time range from inputs
	 */

	mvals = lwalloc( sizeof(double) *
	                 ( l1->points->npoints + l2->points->npoints ) );

	/* TODO: also clip the lines ? */
	nmvals  = ptarray_collect_mvals(l1->points, tmin, tmax, mvals);
	nmvals += ptarray_collect_mvals(l2->points, tmin, tmax, mvals + nmvals);

	/* Sort values in ascending order */
	qsort(mvals, nmvals, sizeof(double), compare_double);

	/* Remove duplicated values */
	nmvals = uniq(mvals, nmvals);

	if ( nmvals < 2 )
	{
		/* there's a single time, must be that one... */
		double t0 = mvals[0];
		POINT4D p0, p1;
		LWDEBUGF(1, "Inputs only exist both at a single time (%g)", t0);
		if ( -1 == ptarray_locate_along_linear(l1->points, t0, &p0, 0) )
		{
			lwnotice("Could not find point with M=%g on first geom", t0);
			return LW_FALSE;
		}
		if ( -1 == ptarray_locate_along_linear(l2->points, t0, &p1, 0) )
		{
			lwnotice("Could not find point with M=%g on second geom", t0);
			return LW_FALSE;
		}
		if ( distance3d_pt_pt((POINT3D*)&p0, (POINT3D*)&p1) <= maxdist )
			within = LW_TRUE;
		lwfree(mvals);
		return within;
	}

	/*
	 * For each consecutive pair of measures, compute time of closest point
	 * approach and actual distance between points at that time
	 */
	for (i=1; i<nmvals; ++i)
	{
		double t0 = mvals[i-1];
		double t1 = mvals[i];
#if POSTGIS_DEBUG_LEVEL >= 1
		double t;
#endif
		POINT4D p0, p1, q0, q1;
		int seg;
		double dist2;

		// lwnotice("T %g-%g", t0, t1);

		seg = ptarray_locate_along_linear(l1->points, t0, &p0, 0);
		if ( -1 == seg ) continue; /* possible, if GBOX is approximated */
		// lwnotice("Measure %g on segment %d of line 1: %g, %g, %g", t0, seg, p0.x, p0.y, p0.z);

		seg = ptarray_locate_along_linear(l1->points, t1, &p1, seg);
		if ( -1 == seg ) continue; /* possible, if GBOX is approximated */
		// lwnotice("Measure %g on segment %d of line 1: %g, %g, %g", t1, seg, p1.x, p1.y, p1.z);

		seg = ptarray_locate_along_linear(l2->points, t0, &q0, 0);
		if ( -1 == seg ) continue; /* possible, if GBOX is approximated */
		// lwnotice("Measure %g on segment %d of line 2: %g, %g, %g", t0, seg, q0.x, q0.y, q0.z);

		seg = ptarray_locate_along_linear(l2->points, t1, &q1, seg);
		if ( -1 == seg ) continue; /* possible, if GBOX is approximated */
		// lwnotice("Measure %g on segment %d of line 2: %g, %g, %g", t1, seg, q1.x, q1.y, q1.z);

#if POSTGIS_DEBUG_LEVEL >= 1
		t =
#endif
		segments_tcpa(&p0, &p1, &q0, &q1, t0, t1);

		/*
		lwnotice("Closest points: %g,%g,%g and %g,%g,%g at time %g",
		p0.x, p0.y, p0.z,
		q0.x, q0.y, q0.z, t);
		*/

		dist2 = ( q0.x - p0.x ) * ( q0.x - p0.x ) +
		        ( q0.y - p0.y ) * ( q0.y - p0.y ) +
		        ( q0.z - p0.z ) * ( q0.z - p0.z );
		if ( dist2 <= maxdist2 )
		{
			LWDEBUGF(1, "Within distance %g at time %g, breaking", sqrt(dist2), t);
			within = LW_TRUE;
			break;
		}
	}

	/*
	 * Release memory
	 */

	lwfree(mvals);

	return within;
}
