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

/*
** lw_segment_side()
**
** Return < 0.0 if point Q is left of segment P
** Return > 0.0 if point Q is right of segment P
** Return = 0.0 if point Q in on segment P
*/
double lw_segment_side(POINT2D *p1, POINT2D *p2, POINT2D *q)
{
	return ( (q->x - p1->x) * (p2->y - p1->y) - (p2->x - p1->x) * (q->y - p1->y) );
}

int lw_segment_envelope_intersects(POINT2D *p1, POINT2D *p2, POINT2D *q1, POINT2D *q2) 
{
	double minq=LW_MIN(q1->x,q2->x);
	double maxq=LW_MAX(q1->x,q2->x);
	double minp=LW_MIN(p1->x,p2->x);
	double maxp=LW_MAX(p1->x,p2->x);

	if(minp>maxq || maxp<minq) 
		return LW_FALSE;

	minq=LW_MIN(q1->y,q2->y);
	maxq=LW_MAX(q1->y,q2->y);
	minp=LW_MIN(p1->y,p2->y);
	maxp=LW_MAX(p1->y,p2->y);

	if(minp>maxq || maxp<minq) 
		return LW_FALSE;

	return LW_TRUE;
}

/*
** lw_segment_intersects()
** 
** Returns one of
**	SEG_ERROR = -1,
**	SEG_NO_INTERSECTION = 0, 
**	SEG_COLINEAR = 1, 
**	SEG_CROSS_LEFT = 2, 
**	SEG_CROSS_RIGHT = 3, 
**	SEG_TOUCH_LEFT = 4, 
**	SEG_TOUCH_RIGHT = 5
*/
int lw_segment_intersects(POINT2D *p1, POINT2D *p2, POINT2D *q1, POINT2D *q2) 
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
	if ((qp1>0 && qp2>0) || (qp1<0 && qp2<0)) 
	{
		return SEG_NO_INTERSECTION;
	}

	/* Nobody is on one side or another? Must be colinear. */
	if (pq1 == 0.0 && pq2 == 0.0 && qp1 == 0.0 && qp2 == 0.0) 
	{
		return SEG_COLINEAR;
	}

	/* 
	** When one end-point touches, the sidedness is determined by the 
	** location of the other end-point.
	*/
	if ( pq2 == 0.0 ) 
	{
		if ( pq1 < 0.0 ) 
			return SEG_TOUCH_LEFT;
		else 
			return SEG_TOUCH_RIGHT;
	}
	if ( pq1 == 0.0 ) 
	{
		if ( pq2 < 0.0 ) 
			return SEG_TOUCH_LEFT;
		else 
			return SEG_TOUCH_RIGHT;
	}

	/* The segments cross, what direction is the crossing? */
	if ( pq1 < pq2 ) 
		return SEG_CROSS_RIGHT;
	else 
		return SEG_CROSS_LEFT;
	
	/* This should never happen! */
	return SEG_ERROR;
		
}

/*
** lwline_crossing_direction()
** 
** Returns one of
**   LINE_NO_CROSS = 0
**   LINE_CROSS_LEFT = -1
**   LINE_CROSS_RIGHT = 1
**   LINE_MULTICROSS_END_LEFT = -2
**   LINE_MULTICROSS_END_RIGHT = 2
**   LINE_MULTICROSS_END_SAME_FIRST_LEFT = -3
**   LINE_MULTICROSS_END_SAME_FIRST_RIGHT = 3
**   LINE_TOUCH_LEFT = -4
**   LINE_TOUCH_RIGHT = 4
**
*/
int lwline_crossing_direction(LWLINE *l1, LWLINE *l2) 
{
	
	int i = 0, j = 0, rv = 0;
	POINT2D *p1;
	POINT2D *p2;
	POINT2D *q1;
	POINT2D *q2;
	POINTARRAY *pa1;
	POINTARRAY *pa2;
	int cross_left = 0;
	int cross_right = 0;
	int first_cross = 0;
	int final_cross = 0;
	int this_cross = 0;
	int vertex_touch = -1;
	int vertex_touch_type = 0;
	
	pa1 = (POINTARRAY*)l1->points;
	pa2 = (POINTARRAY*)l2->points;
	
	p1 = lwalloc(sizeof(POINT2D));
	p2 = lwalloc(sizeof(POINT2D));
	q1 = lwalloc(sizeof(POINT2D));
	q2 = lwalloc(sizeof(POINT2D));
	
	/* One-point lines can't intersect (and shouldn't exist). */
	if( pa1->npoints < 2 || pa2->npoints < 2 ) 
		return LINE_NO_CROSS;

	LWDEBUGF(4, "lineCrossingDirection: l1 = %s", lwgeom_to_ewkt((LWGEOM*)l1,0));
	LWDEBUGF(4, "lineCrossingDirection: l2 = %s", lwgeom_to_ewkt((LWGEOM*)l2,0));

	for ( i = 1; i < pa2->npoints; i++ ) 
	{
		
		rv = getPoint2d_p(pa2, i-1, q1);
		rv = getPoint2d_p(pa2, i, q2);
		
		for ( j = 1; j < pa1->npoints; j++ ) 
		{   
			
			rv = getPoint2d_p(pa1, j-1, p1);
			rv = getPoint2d_p(pa1, j, p2);
			
			LWDEBUGF(4, "lineCrossingDirection: i=%d, j=%d", i, j);
			
			this_cross = lw_segment_intersects(p1, p2, q1, q2);
		
			if( ! first_cross && this_cross )
				first_cross = this_cross;
			if( this_cross ) 
				final_cross = this_cross;
				
			if( this_cross == SEG_CROSS_LEFT ) 
			{
				cross_left++;
				break;
			}
				
			if( this_cross == SEG_CROSS_RIGHT ) 
			{
				cross_right++;
				break;
			}

			/*
			** Crossing at a co-linearity can be turned into crossing at
			** a vertex by pulling the original touch point forward along
			** the co-linearity.
			*/
			if( this_cross == SEG_COLINEAR && vertex_touch == (i-1) ) 
			{
				vertex_touch = i;
				break;
			}
				
			/* 
			** Crossing-at-vertex will cause four segment touch interactions, two at
			** j-1 and two at j. We avoid incrementing our cross count by ignoring the
			** second pair.
			*/
			if( this_cross == SEG_TOUCH_LEFT ) 
			{
				if ( vertex_touch == (i-1) && vertex_touch_type == SEG_TOUCH_RIGHT ) {
					cross_left++;
					vertex_touch = -1;
					vertex_touch_type = 0;
				}
				else {
					vertex_touch = i;
					vertex_touch_type = this_cross;
				}
				break;
			}
			if( this_cross == SEG_TOUCH_RIGHT ) 
			{
				if ( vertex_touch == (i-1) && vertex_touch_type == SEG_TOUCH_LEFT ) {
					cross_right++;
					vertex_touch = -1;
					vertex_touch_type = 0;
				}
				else {
					vertex_touch = i;
					vertex_touch_type = this_cross;
				}
				break;
			}

			/* 
			** TODO Handle co-linear cases. 
			*/

 			LWDEBUGF(4, "lineCrossingDirection: this_cross=%d, vertex_touch=%d, vertex_touch_type=%d", this_cross, vertex_touch, vertex_touch_type);
				
		}
		
	}

	LWDEBUGF(4, "first_cross=%d, final_cross=%d, cross_left=%d, cross_right=%d", first_cross, final_cross, cross_left, cross_right);

	lwfree(p1);
	lwfree(p2);
	lwfree(q1);
	lwfree(q2);

	if( !cross_left && !cross_right ) 
		return LINE_NO_CROSS;

	if( !cross_left && cross_right == 1 )
		return LINE_CROSS_RIGHT;
		
	if( !cross_right && cross_left == 1 )
		return LINE_CROSS_LEFT;

	if( cross_left - cross_right == 1 )
		return LINE_MULTICROSS_END_LEFT;

	if( cross_left - cross_right == -1 )
		return LINE_MULTICROSS_END_RIGHT;

	if( cross_left - cross_right == 0 && first_cross == SEG_CROSS_LEFT )
		return LINE_MULTICROSS_END_SAME_FIRST_LEFT;

	if( cross_left - cross_right == 0 && first_cross == SEG_CROSS_RIGHT )
		return LINE_MULTICROSS_END_SAME_FIRST_RIGHT;

	if( cross_left - cross_right == 0 && first_cross == SEG_TOUCH_LEFT )
		return LINE_MULTICROSS_END_SAME_FIRST_RIGHT;

	if( cross_left - cross_right == 0 && first_cross == SEG_TOUCH_RIGHT )
		return LINE_MULTICROSS_END_SAME_FIRST_LEFT;
		
	return LINE_NO_CROSS;
	
}

#if 0 
/*
** lwpoint_get_ordinate(point, ordinate) => double
*/
double lwpoint_get_ordinate(POINT4D *p, int ordinate) 
{
	if( ! p ) 
	{
		lwerror("Null input geometry.");
		return 0.0;
	}
	
	if( ordinate > 3 || ordinate < 0 ) 
	{
		lwerror("Cannot extract ordinate %d.", ordinate);
		return 0.0;
	}

	if( ordinate == 3 ) 
		return p->m;
	if( ordinate == 2 ) 
		return p->z;
	if( ordinate == 1 ) 
		return p->y;

	return p->x;
	
}

/*
** lwline_clip_to_ordinate_range(line, ordinate, from, to) => lwmline
**
** Take in a LINESTRING and return a MULTILINESTRING of those portions of the 
** LINESTRING between the from/to range for the specified ordinate (XYZM)
*/
LWLINE *lwline_clip_to_ordinate_range(LWLINE *line, int ordinate, double from, double to) 
{
	
	POINTARRAY *pa_in;
	LWMLINE *mline_out;
	POINTARRAY *pa_out;
	DYNPTARRAY *dp;
	int i, rv;
	int last_point = 0;
	int nparts = 0;
	POINT4D *p, *q;
	double ordinate_value;

	
	/* Null input, nothing we can do. */
	if( ! line ) 
	{
		lwerror("Null input geometry.");
		return NULL;
	}
	
	/* Ensure 'from' is less than 'to'. */
	if( to < from ) 
	{
		double t = from;
		from = to;
		to = t;
	}
	
	int ndims = TYPE_NDIMS(line->type);
	
	/* Asking for an ordinate we don't have. */
	if( ordinate >= ndims ) 
	{
		lwerror("Cannot clip on ordinate %d in a %d-d geometry.", ordinate, ndims);
		return NULL;
	}
	
	p = lwalloc(sizeof(POINT4D));
	q = lwalloc(sizeof(POINT4D));

	pa_in = (POINTARRAY*)line->points;
	
	dp = dynptarray_create(64, ndims);

	
	for ( i = 1; i < pa_in->npoints; i++ ) 
	{
		rv = getPoint4d_p(pa_in, i, p);
		ordinate_value = lwpoint_get_ordinate(p, ordinate);
		if ( ordinate_value >= from && ordinate_value <= to )
		{
			rv =dynptarray_addPoint4d(dp, p, 1);
		} 
	}

	
}
#endif
