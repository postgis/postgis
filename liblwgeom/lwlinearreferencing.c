/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2011 Paul Ramsey
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "liblwgeom_internal.h"
#include "lwgeom_log.h"


/**
* Given a POINT4D and an ordinate number, return
* the value of the ordinate.
* @param p input point
* @param ordinate number (1=x, 2=y, 3=z, 4=m)
* @return d value at that ordinate
*/
double lwpoint_get_ordinate(const POINT4D *p, int ordinate)
{
	if ( ! p )
	{
		lwerror("Null input geometry.");
		return 0.0;
	}

	if ( ordinate > 3 || ordinate < 0 )
	{
		lwerror("Cannot extract ordinate %d.", ordinate);
		return 0.0;
	}

	if ( ordinate == 3 )
		return p->m;
	if ( ordinate == 2 )
		return p->z;
	if ( ordinate == 1 )
		return p->y;

	return p->x;

}

/** 
* Given a point, ordinate number and value, set that ordinate on the
* point.
*/
void lwpoint_set_ordinate(POINT4D *p, int ordinate, double value)
{
	if ( ! p )
	{
		lwerror("Null input geometry.");
		return;
	}

	if ( ordinate > 3 || ordinate < 0 )
	{
		lwerror("Cannot extract ordinate %d.", ordinate);
		return;
	}

	LWDEBUGF(4, "    setting ordinate %d to %g", ordinate, value);

	switch ( ordinate )
	{
	case 3:
		p->m = value;
		return;
	case 2:
		p->z = value;
		return;
	case 1:
		p->y = value;
		return;
	case 0:
		p->x = value;
		return;
	}
}

/**
* Given two points, a dimensionality, an ordinate, and an interpolation value
* generate a new point that is proportionally between the input points,
* using the values in the provided dimension as the scaling factors.
*/
int lwpoint_interpolate(const POINT4D *p1, const POINT4D *p2, POINT4D *p, int ndims, int ordinate, double interpolation_value)
{
	double p1_value = lwpoint_get_ordinate(p1, ordinate);
	double p2_value = lwpoint_get_ordinate(p2, ordinate);
	double proportion;
	int i = 0;

	if ( ordinate < 0 || ordinate >= ndims )
	{
		lwerror("Ordinate (%d) is not within ndims (%d).", ordinate, ndims);
		return 0;
	}

	if ( FP_MIN(p1_value, p2_value) > interpolation_value ||
	        FP_MAX(p1_value, p2_value) < interpolation_value )
	{
		lwerror("Cannot interpolate to a value (%g) not between the input points (%g, %g).", interpolation_value, p1_value, p2_value);
		return 0;
	}

	proportion = fabs((interpolation_value - p1_value) / (p2_value - p1_value));

	for ( i = 0; i < ndims; i++ )
	{
		double newordinate = 0.0;
		p1_value = lwpoint_get_ordinate(p1, i);
		p2_value = lwpoint_get_ordinate(p2, i);
		newordinate = p1_value + proportion * (p2_value - p1_value);
		lwpoint_set_ordinate(p, i, newordinate);
		LWDEBUGF(4, "   clip ordinate(%d) p1_value(%g) p2_value(%g) proportion(%g) newordinate(%g) ", i, p1_value, p2_value, proportion, newordinate );
	}

	return 1;
}

/**
* Clip an input MULTILINESTRING between two values, on any ordinate input.
*/
LWCOLLECTION *lwmline_clip_to_ordinate_range(LWMLINE *mline, int ordinate, double from, double to)
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
		char hasz = FLAGS_GET_Z(mline->flags);
		char hasm = FLAGS_GET_M(mline->flags);
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
		lwgeom_drop_bbox((LWGEOM*)lwgeom_out);
		lwgeom_add_bbox((LWGEOM*)lwgeom_out);
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
LWCOLLECTION *lwline_clip_to_ordinate_range(LWLINE *line, int ordinate, double from, double to)
{

	POINTARRAY *pa_in = NULL;
	LWCOLLECTION *lwgeom_out = NULL;
	POINTARRAY *dp = NULL;
	int i, rv;
	int added_last_point = 0;
	POINT4D *p = NULL, *q = NULL, *r = NULL;
	double ordinate_value_p = 0.0, ordinate_value_q = 0.0;
	char hasz = FLAGS_GET_Z(line->flags);
	char hasm = FLAGS_GET_M(line->flags);
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

	LWDEBUGF(4, "from = %g, to = %g, ordinate = %d", from, to, ordinate);
	LWDEBUGF(4, "%s", lwgeom_to_ewkt((LWGEOM*)line));

	/* Asking for an ordinate we don't have. Error. */
	if ( ordinate >= dims )
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
		rv = getPoint4d_p(pa_in, i, p);
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
					rv = lwpoint_interpolate(q, p, r, dims, ordinate, interpolation_value);
					rv = ptarray_append_point(dp, r, LW_FALSE);
					LWDEBUGF(4, "[0] interpolating between (%g, %g) with interpolation point (%g)", ordinate_value_q, ordinate_value_p, interpolation_value);
				}
			}
			/* Add the current vertex to the point array. */
			rv = ptarray_append_point(dp, p, LW_FALSE);
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
				rv = lwpoint_interpolate(q, p, r, dims, ordinate, interpolation_value);
				rv = ptarray_append_point(dp, r, LW_FALSE);
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
					rv = lwpoint_interpolate(q, p, r, dims, ordinate, interpolation_value);
					rv = ptarray_append_point(dp, r, LW_FALSE);
					LWDEBUGF(4, " [2] interpolating between (%g, %g) with interpolation point (%g)", ordinate_value_q, ordinate_value_p, interpolation_value);
				}
			}
			else if ( i && ordinate_value_q < from && ordinate_value_p > to )
			{
				/* We just hopped over the whole range, from bottom to top,
				*  so we need to add *two* interpolated points! */
				dp = ptarray_construct(hasz, hasm, 2);
				/* Interpolate lower point. */
				rv = lwpoint_interpolate(p, q, r, dims, ordinate, from);
				ptarray_set_point4d(dp, 0, r);
				/* Interpolate upper point. */
				rv = lwpoint_interpolate(p, q, r, dims, ordinate, to);
				ptarray_set_point4d(dp, 1, r);
			}
			else if ( i && ordinate_value_q > to && ordinate_value_p < from )
			{
				/* We just hopped over the whole range, from top to bottom,
				*  so we need to add *two* interpolated points! */
				dp = ptarray_construct(hasz, hasm, 2);
				/* Interpolate upper point. */
				rv = lwpoint_interpolate(p, q, r, dims, ordinate, to);
				ptarray_set_point4d(dp, 0, r);
				/* Interpolate lower point. */
				rv = lwpoint_interpolate(p, q, r, dims, ordinate, from);
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

	if ( lwgeom_out->ngeoms > 0 )
	{
		lwgeom_drop_bbox((LWGEOM*)lwgeom_out);
		lwgeom_add_bbox((LWGEOM*)lwgeom_out);
		return lwgeom_out;
	}

	return NULL;

}