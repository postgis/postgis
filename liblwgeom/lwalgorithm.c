/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2008 Paul Ramsey
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "lwalgorithm.h"



/**
** lw_segment_side()
**
** Return < 0.0 if point Q is left of segment P
** Return > 0.0 if point Q is right of segment P
** Return = 0.0 if point Q in on segment P
*/
double lw_segment_side(const POINT2D *p1, const POINT2D *p2, const POINT2D *q)
{
	double side = ( (q->x - p1->x) * (p2->y - p1->y) - (p2->x - p1->x) * (q->y - p1->y) );
	if ( FP_IS_ZERO(side) )
		return 0.0;
	else
		return side;
}


int lw_segment_envelope_intersects(const POINT2D *p1, const POINT2D *p2, const POINT2D *q1, const POINT2D *q2)
{
	double minq=LW_MIN(q1->x,q2->x);
	double maxq=LW_MAX(q1->x,q2->x);
	double minp=LW_MIN(p1->x,p2->x);
	double maxp=LW_MAX(p1->x,p2->x);

	if (FP_GT(minp,maxq) || FP_LT(maxp,minq))
		return LW_FALSE;

	minq=LW_MIN(q1->y,q2->y);
	maxq=LW_MAX(q1->y,q2->y);
	minp=LW_MIN(p1->y,p2->y);
	maxp=LW_MAX(p1->y,p2->y);

	if (FP_GT(minp,maxq) || FP_LT(maxp,minq))
		return LW_FALSE;

	return LW_TRUE;
}

/**
** @brief returns the kind of #CG_SEGMENT_INTERSECTION_TYPE  behavior of lineseg 1 (constructed from p1 and p2) and lineseg 2 (constructed from q1 and q2)
**	@param p1 start point of first straight linesegment
**	@param p2 end point of first straight linesegment
**	@param q1 start point of second line segment
**	@param q2 end point of second line segment
**	@return a #CG_SEGMENT_INTERSECTION_TYPE
** 	Returns one of
**		SEG_ERROR = -1,
**		SEG_NO_INTERSECTION = 0,
**		SEG_COLINEAR = 1,
**		SEG_CROSS_LEFT = 2,
**		SEG_CROSS_RIGHT = 3,
*/
int lw_segment_intersects(const POINT2D *p1, const POINT2D *p2, const POINT2D *q1, const POINT2D *q2)
{

	double pq1, pq2, qp1, qp2;

	/* No envelope interaction => we are done. */
	if (!lw_segment_envelope_intersects(p1, p2, q1, p2))
	{
		return SEG_NO_INTERSECTION;
	}

	/* Are the start and end points of q on the same side of p? */
	pq1=lw_segment_side(p1,p2,q1);
	pq2=lw_segment_side(p1,p2,q2);
	if ((pq1>0 && pq2>0) || (pq1<0 && pq2<0))
	{
		return SEG_NO_INTERSECTION;
	}

	/* Are the start and end points of p on the same side of q? */
	qp1=lw_segment_side(q1,q2,p1);
	qp2=lw_segment_side(q1,q2,p2);
	if ( (qp1 > 0.0 && qp2 > 0.0) || (qp1 < 0.0 && qp2 < 0.0) )
	{
		return SEG_NO_INTERSECTION;
	}

	/* Nobody is on one side or another? Must be colinear. */
	if ( pq1 == 0.0 && pq2 == 0.0 && qp1 == 0.0 && qp2 == 0.0 )
	{
		return SEG_COLINEAR;
	}

	/*
	** When one end-point touches, the sidedness is determined by the
	** location of the other end-point. Only touches by the first point
	** will be considered "real" to avoid double counting.
	*/
	LWDEBUGF(4, "pq1=%.15g pq2=%.15g", pq1, pq2);
	LWDEBUGF(4, "qp1=%.15g qp2=%.15g", qp1, qp2);

	/* Second point of p or q touches, it's not a crossing. */
	if ( pq2 == 0.0 || qp2 == 0.0 )
	{
		return SEG_NO_INTERSECTION;
	}

	/* First point of p touches, it's a "crossing". */
	if ( pq1 == 0.0 )
	{
		if ( FP_GT(pq2,0.0) )
			return SEG_CROSS_RIGHT;
		else
			return SEG_CROSS_LEFT;
	}

	/* First point of q touches, it's a crossing. */
	if ( qp1 == 0.0 )
	{
		if ( FP_LT(pq1,pq2) )
			return SEG_CROSS_RIGHT;
		else
			return SEG_CROSS_LEFT;
	}

	/* The segments cross, what direction is the crossing? */
	if ( FP_LT(pq1,pq2) )
		return SEG_CROSS_RIGHT;
	else
		return SEG_CROSS_LEFT;

	/* This should never happen! */
	return SEG_ERROR;
}

/**
** @brief lwline_crossing_direction: returns the kind of #CG_LINE_CROSS_TYPE behavior  of 2 linestrings
** @param l1 first line string
** @param l2 second line string
** @return a #CG_LINE_CROSS_TYPE
**   LINE_NO_CROSS = 0
**   LINE_CROSS_LEFT = -1
**   LINE_CROSS_RIGHT = 1
**   LINE_MULTICROSS_END_LEFT = -2
**   LINE_MULTICROSS_END_RIGHT = 2
**   LINE_MULTICROSS_END_SAME_FIRST_LEFT = -3
**   LINE_MULTICROSS_END_SAME_FIRST_RIGHT = 3
**
*/
int lwline_crossing_direction(LWLINE *l1, LWLINE *l2)
{
	int i = 0, j = 0, rv = 0;
	POINT2D p1, p2, q1, q2;
	POINTARRAY *pa1 = NULL, *pa2 = NULL;
	int cross_left = 0;
	int cross_right = 0;
	int first_cross = 0;
	int this_cross = 0;

	pa1 = (POINTARRAY*)l1->points;
	pa2 = (POINTARRAY*)l2->points;

	/* One-point lines can't intersect (and shouldn't exist). */
	if ( pa1->npoints < 2 || pa2->npoints < 2 )
		return LINE_NO_CROSS;

	LWDEBUGF(4, "l1 = %s", lwgeom_to_ewkt((LWGEOM*)l1,0));
	LWDEBUGF(4, "l2 = %s", lwgeom_to_ewkt((LWGEOM*)l2,0));

	/* Initialize first point of q */
	rv = getPoint2d_p(pa2, 0, &q1);

	for ( i = 1; i < pa2->npoints; i++ )
	{

		/* Update second point of q to next value */
		rv = getPoint2d_p(pa2, i, &q2);

		/* Initialize first point of p */
		rv = getPoint2d_p(pa1, 0, &p1);

		for ( j = 1; j < pa1->npoints; j++ )
		{

			/* Update second point of p to next value */
			rv = getPoint2d_p(pa1, j, &p2);

			this_cross = lw_segment_intersects(&p1, &p2, &q1, &q2);

			LWDEBUGF(4, "i=%d, j=%d (%.8g %.8g, %.8g %.8g)", this_cross, i, j, p1.x, p1.y, p2.x, p2.y);

			if ( this_cross == SEG_CROSS_LEFT )
			{
				LWDEBUG(4,"this_cross == SEG_CROSS_LEFT");
				cross_left++;
				if ( ! first_cross )
					first_cross = SEG_CROSS_LEFT;
			}

			if ( this_cross == SEG_CROSS_RIGHT )
			{
				LWDEBUG(4,"this_cross == SEG_CROSS_RIGHT");
				cross_right++;
				if ( ! first_cross )
					first_cross = SEG_CROSS_LEFT;
			}

			/*
			** Crossing at a co-linearity can be turned handled by extending
			** segment to next vertext and seeing if the end points straddle
			** the co-linear segment.
			*/
			if ( this_cross == SEG_COLINEAR )
			{
				LWDEBUG(4,"this_cross == SEG_COLINEAR");
				/* TODO: Add logic here and in segment_intersects()
				continue;
				*/
			}

			LWDEBUG(4,"this_cross == SEG_NO_INTERSECTION");

			/* Turn second point of p into first point */
			p1 = p2;

		}

		/* Turn second point of q into first point */
		q1 = q2;

	}

	LWDEBUGF(4, "first_cross=%d, cross_left=%d, cross_right=%d", first_cross, cross_left, cross_right);

	if ( !cross_left && !cross_right )
		return LINE_NO_CROSS;

	if ( !cross_left && cross_right == 1 )
		return LINE_CROSS_RIGHT;

	if ( !cross_right && cross_left == 1 )
		return LINE_CROSS_LEFT;

	if ( cross_left - cross_right == 1 )
		return LINE_MULTICROSS_END_LEFT;

	if ( cross_left - cross_right == -1 )
		return LINE_MULTICROSS_END_RIGHT;

	if ( cross_left - cross_right == 0 && first_cross == SEG_CROSS_LEFT )
		return LINE_MULTICROSS_END_SAME_FIRST_LEFT;

	if ( cross_left - cross_right == 0 && first_cross == SEG_CROSS_RIGHT )
		return LINE_MULTICROSS_END_SAME_FIRST_RIGHT;

	return LINE_NO_CROSS;

}



/*
** lwpoint_get_ordinate(point, ordinate) => double
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
		char hasz = TYPE_HASZ(mline->type);
		char hasm = TYPE_HASM(mline->type);
		char hassrid = TYPE_HASSRID(mline->type);
		int i, j;
		char homogeneous = 1;
		size_t geoms_size = 0;
		lwgeom_out = lwcollection_construct_empty(mline->SRID, hasz, hasm);
		lwgeom_out->type = lwgeom_makeType(hasz, hasm, hassrid, MULTILINETYPE);
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
				if ( TYPE_GETTYPE(col->type) != TYPE_GETTYPE(mline->type) )
				{
					homogeneous = 0;
				}
				/* Shallow free the struct, leaving the geoms behind. */
				if ( col->bbox ) lwfree(col->bbox);
				lwfree(col);
			}
		}
		lwgeom_drop_bbox((LWGEOM*)lwgeom_out);
		lwgeom_add_bbox((LWGEOM*)lwgeom_out);
		if ( ! homogeneous )
		{
			lwgeom_out->type = lwgeom_makeType(hasz, hasm, hassrid, COLLECTIONTYPE);
		}
	}

	if ( ! lwgeom_out || lwgeom_out->ngeoms == 0 ) /* Nothing left after clip. */
	{
		return NULL;
	}

	return lwgeom_out;

}


/*
** lwline_clip_to_ordinate_range(line, ordinate, from, to) => lwmline
**
** Take in a LINESTRING and return a MULTILINESTRING of those portions of the
** LINESTRING between the from/to range for the specified ordinate (XYZM)
*/
LWCOLLECTION *lwline_clip_to_ordinate_range(LWLINE *line, int ordinate, double from, double to)
{

	POINTARRAY *pa_in = NULL;
	LWCOLLECTION *lwgeom_out = NULL;
	POINTARRAY *pa_out = NULL;
	DYNPTARRAY *dp = NULL;
	int i, rv;
	int added_last_point = 0;
	POINT4D *p = NULL, *q = NULL, *r = NULL;
	double ordinate_value_p = 0.0, ordinate_value_q = 0.0;
	char hasz = TYPE_HASZ(line->type);
	char hasm = TYPE_HASM(line->type);
	char dims = TYPE_NDIMS(line->type);
	char hassrid = TYPE_HASSRID(line->type);

	LWDEBUGF(4, "hassrid = %d", hassrid);

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
	LWDEBUGF(4, "%s", lwgeom_to_ewkt((LWGEOM*)line, PARSER_CHECK_NONE));

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
	lwgeom_out = lwalloc(sizeof(LWCOLLECTION));
	lwgeom_out->type = lwgeom_makeType(hasz, hasm, hassrid, MULTILINETYPE);
	if (hassrid)
		lwgeom_out->SRID = line->SRID;
	else
		lwgeom_out->SRID = -1;
	lwgeom_out->bbox = NULL;
	lwgeom_out->ngeoms = 0;
	lwgeom_out->geoms = NULL;

	pa_in = (POINTARRAY*)line->points;

	for ( i = 0; i < pa_in->npoints; i++ )
	{
		LWDEBUGF(4, "Point #%d", i);
		if ( i > 0 )
		{
			q->x = p->x;
			q->y = p->y;
			q->z = p->z;
			q->m = p->m;
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
				if ( dp ) lwfree(dp);
				dp = dynptarray_create(64, line->type);

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
					rv = dynptarray_addPoint4d(dp, r, 0);
					LWDEBUGF(4, " interpolating between (%g, %g) with interpolation point (%g)", ordinate_value_q, ordinate_value_p, interpolation_value);
				}
			}
			/* Add the current vertex to the point array. */
			rv = dynptarray_addPoint4d(dp, p, 0);
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
				rv = dynptarray_addPoint4d(dp, r, 0);
				LWDEBUGF(4, " interpolating between (%g, %g) with interpolation point (%g)", ordinate_value_q, ordinate_value_p, interpolation_value);
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
					rv = dynptarray_addPoint4d(dp, r, 0);
					LWDEBUGF(4, " interpolating between (%g, %g) with interpolation point (%g)", ordinate_value_q, ordinate_value_p, interpolation_value);
				}
			}
			else if ( i && ordinate_value_q < from && ordinate_value_p > to )
			{
				/* We just hopped over the whole range, from bottom to top,
				*  so we need to add *two* interpolated points! */
				pa_out = ptarray_construct(hasz, hasm, 2);
				/* Interpolate lower point. */
				rv = lwpoint_interpolate(p, q, r, dims, ordinate, from);
				setPoint4d(pa_out, 0, r);
				/* Interpolate upper point. */
				rv = lwpoint_interpolate(p, q, r, dims, ordinate, to);
				setPoint4d(pa_out, 1, r);
			}
			else if ( i && ordinate_value_q > to && ordinate_value_p < from )
			{
				/* We just hopped over the whole range, from top to bottom,
				*  so we need to add *two* interpolated points! */
				pa_out = ptarray_construct(hasz, hasm, 2);
				/* Interpolate upper point. */
				rv = lwpoint_interpolate(p, q, r, dims, ordinate, to);
				setPoint4d(pa_out, 0, r);
				/* Interpolate lower point. */
				rv = lwpoint_interpolate(p, q, r, dims, ordinate, from);
				setPoint4d(pa_out, 1, r);
			}
			/* We have an extant point-array, save it out to a multi-line. */
			if ( dp || pa_out )
			{
				LWGEOM *oline;
				LWDEBUG(4, "saving pointarray to multi-line (1)");
				if ( dp )
				{
					/* Only one point, so we have to make an lwpoint to hold this
					*  and set the overall output type to a generic collection. */
					if ( dp->pa->npoints == 1 )
					{
						oline = (LWGEOM*)lwpoint_construct(line->SRID, NULL, dp->pa);
						oline->type = lwgeom_makeType(hasz, hasm, hassrid, POINTTYPE);
						lwgeom_out->type = lwgeom_makeType(hasz, hasm, hassrid, COLLECTIONTYPE);
					}
					else
					{
						oline = (LWGEOM*)lwline_construct(line->SRID, NULL, dp->pa);
						oline->type = lwgeom_makeType(hasz, hasm, hassrid, LINETYPE);
					}
				}
				else
				{
					oline = (LWGEOM*)lwline_construct(line->SRID, NULL, pa_out);
				}
				lwgeom_out->ngeoms++;
				if ( lwgeom_out->geoms ) /* We can't just realloc, since repalloc chokes on a starting null ptr. */
				{
					lwgeom_out->geoms = lwrealloc(lwgeom_out->geoms, sizeof(LWGEOM*) * lwgeom_out->ngeoms);
				}
				else
				{
					lwgeom_out->geoms = lwalloc(sizeof(LWGEOM*) * lwgeom_out->ngeoms);
				}
				lwgeom_out->geoms[lwgeom_out->ngeoms - 1] = oline;
				lwgeom_drop_bbox((LWGEOM*)lwgeom_out);
				lwgeom_add_bbox((LWGEOM*)lwgeom_out);
				if ( dp ) lwfree(dp);
				dp = NULL;
				if ( pa_out ) pa_out = NULL;
			}
			added_last_point = 0;

		}
	}

	/* Still some points left to be saved out. */
	if ( dp && dp->pa->npoints > 0 )
	{
		LWGEOM *oline;
		LWDEBUG(4, "saving pointarray to multi-line (2)");
		LWDEBUGF(4, "dp->pa->npoints == %d", dp->pa->npoints);
		LWDEBUGF(4, "lwgeom_out->ngeoms == %d", lwgeom_out->ngeoms);

		if ( dp->pa->npoints == 1 )
		{
			oline = (LWGEOM*)lwpoint_construct(line->SRID, NULL, dp->pa);
			oline->type = lwgeom_makeType(hasz, hasm, hassrid, POINTTYPE);
			lwgeom_out->type = lwgeom_makeType(hasz, hasm, hassrid, COLLECTIONTYPE);
		}
		else
		{
			oline = (LWGEOM*)lwline_construct(line->SRID, NULL, dp->pa);
			oline->type = lwgeom_makeType(hasz, hasm, hassrid, LINETYPE);
		}

		lwgeom_out->ngeoms++;
		if ( lwgeom_out->geoms ) /* We can't just realloc, since repalloc chokes on a starting null ptr. */
		{
			lwgeom_out->geoms = lwrealloc(lwgeom_out->geoms, sizeof(LWGEOM*) * lwgeom_out->ngeoms);
		}
		else
		{
			lwgeom_out->geoms = lwalloc(sizeof(LWGEOM*) * lwgeom_out->ngeoms);
		}
		lwgeom_out->geoms[lwgeom_out->ngeoms - 1] = oline;
		lwgeom_drop_bbox((LWGEOM*)lwgeom_out);
		lwgeom_add_bbox((LWGEOM*)lwgeom_out);
		if ( dp ) lwfree(dp);
		dp = NULL;
	}

	lwfree(p);
	lwfree(q);
	lwfree(r);

	if ( lwgeom_out->ngeoms > 0 )
		return lwgeom_out;

	return NULL;

}

static char *base32 = "0123456789bcdefghjkmnpqrstuvwxyz";

/*
** Calculate the geohash, iterating downwards and gaining precision.
** From geohash-native.c, (c) 2008 David Troy <dave@roundhousetech.com>
** Released under the MIT License.
*/
char *geohash_point(double longitude, double latitude, int precision)
{
	int is_even=1, i=0;
	double lat[2], lon[2], mid;
	char bits[] = {16,8,4,2,1};
	int bit=0, ch=0;
	char *geohash = NULL;

	geohash = lwalloc(precision + 1);

	lat[0] = -90.0;
	lat[1] = 90.0;
	lon[0] = -180.0;
	lon[1] = 180.0;

	while (i < precision)
	{
		if (is_even)
		{
			mid = (lon[0] + lon[1]) / 2;
			if (longitude > mid)
			{
				ch |= bits[bit];
				lon[0] = mid;
			}
			else
			{
				lon[1] = mid;
			}
		}
		else
		{
			mid = (lat[0] + lat[1]) / 2;
			if (latitude > mid)
			{
				ch |= bits[bit];
				lat[0] = mid;
			}
			else
			{
				lat[1] = mid;
			}
		}

		is_even = !is_even;
		if (bit < 4)
		{
			bit++;
		}
		else
		{
			geohash[i++] = base32[ch];
			bit = 0;
			ch = 0;
		}
	}
	geohash[i] = 0;
	return geohash;
}

int lwgeom_geohash_precision(BOX3D bbox, BOX3D *bounds)
{
	double minx, miny, maxx, maxy;
	double latmax, latmin, lonmax, lonmin;
	double lonwidth, latwidth;
	double latmaxadjust, lonmaxadjust, latminadjust, lonminadjust;
	int precision = 0;

	/* Get the bounding box, return error if things don't work out. */
	minx = bbox.xmin;
	miny = bbox.ymin;
	maxx = bbox.xmax;
	maxy = bbox.ymax;

	if ( minx == maxx && miny == maxy )
	{
		/* It's a point. Doubles have 51 bits of precision.
		** 2 * 51 / 5 == 20 */
		return 20;
	}

	lonmin = -180.0;
	latmin = -90.0;
	lonmax = 180.0;
	latmax = 90.0;

	/* Shrink a world bounding box until one of the edges interferes with the
	** bounds of our rectangle. */
	while ( 1 )
	{
		lonwidth = lonmax - lonmin;
		latwidth = latmax - latmin;
		latmaxadjust = lonmaxadjust = latminadjust = lonminadjust = 0.0;

		if ( minx > lonmin + lonwidth / 2.0 )
		{
			lonminadjust = lonwidth / 2.0;
		}
		else if ( maxx < lonmax - lonwidth / 2.0 )
		{
			lonmaxadjust = -1 * lonwidth / 2.0;
		}
		if ( miny > latmin + latwidth / 2.0 )
		{
			latminadjust = latwidth / 2.0;
		}
		else if (maxy < latmax - latwidth / 2.0 )
		{
			latmaxadjust = -1 * latwidth / 2.0;
		}
		/* Only adjust if adjustments are legal (we haven't crossed any edges). */
		if ( (lonminadjust || lonmaxadjust) && (latminadjust || latmaxadjust ) )
		{
			latmin += latminadjust;
			lonmin += lonminadjust;
			latmax += latmaxadjust;
			lonmax += lonmaxadjust;
			/* Each adjustment cycle corresponds to 2 bits of storage in the
			** geohash.	*/
			precision += 2;
		}
		else
		{
			break;
		}
	}

	/* Save the edges of our bounds, in case someone cares later. */
	bounds->xmin = lonmin;
	bounds->xmax = lonmax;
	bounds->ymin = latmin;
	bounds->ymax = latmax;

	/* Each geohash character (base32) can contain 5 bits of information.
	** We are returning the precision in characters, so here we divide. */
	return precision / 5;
}


/*
** Return a geohash string for the geometry. <http://geohash.org>
** Where the precision is non-positive, calculate a precision based on the
** bounds of the feature. Big features have loose precision.
** Small features have tight precision.
*/
char *lwgeom_geohash(const LWGEOM *lwgeom, int precision)
{
	BOX3D *bbox = NULL;
	BOX3D precision_bounds;
	double lat, lon;

	bbox = lwgeom_compute_box3d(lwgeom);
	if ( ! bbox ) return NULL;

	/* Return error if we are being fed something outside our working bounds */
	if ( bbox->xmin < -180 || bbox->ymin < -90 || bbox->xmax > 180 || bbox->ymax > 90 )
	{
		lwerror("Geohash requires inputs in decimal degrees.");
		lwfree(bbox);
		return NULL;
	}

	/* What is the center of our geometry bounds? We'll use that to
	** approximate location. */
	lon = bbox->xmin + (bbox->xmax - bbox->xmin) / 2;
	lat = bbox->ymin + (bbox->ymax - bbox->ymin) / 2;

	if ( precision <= 0 )
	{
		precision = lwgeom_geohash_precision(*bbox, &precision_bounds);
	}

	lwfree(bbox);

	/*
	** Return the geohash of the center, with a precision determined by the
	** extent of the bounds.
	** Possible change: return the point at the center of the precision bounds?
	*/
	return geohash_point(lon, lat, precision);
}























