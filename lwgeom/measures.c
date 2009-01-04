/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2006 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 * 
 **********************************************************************/

#include <math.h>
#include <string.h>

#include "liblwgeom.h"

/*#define PGIS_DEBUG 1*/

/*
 * pt_in_ring_2d(): crossing number test for a point in a polygon
 *      input:   p = a point,
 *               pa = vertex points of a ring V[n+1] with V[n]=V[0]
 *      returns: 0 = outside, 1 = inside
 *
 *	Our polygons have first and last point the same,
 *
 */
int pt_in_ring_2d(POINT2D *p, POINTARRAY *ring)
{
	int cn = 0;    /* the crossing number counter */
	int i;
	POINT2D v1, v2;

#if INTEGRITY_CHECKS
	POINT2D first, last;

	getPoint2d_p(ring, 0, &first);
	getPoint2d_p(ring, ring->npoints-1, &last);
	if ( memcmp(&first, &last, sizeof(POINT2D)) )
	{
		lwerror("pt_in_ring_2d: V[n] != V[0] (%g %g != %g %g)",
			first.x, first.y, last.x, last.y);
			
	}
#endif

#ifdef PGIS_DEBUG
	lwnotice("pt_in_ring_2d called with point: %g %g", p->x, p->y);
	/* printPA(ring); */
#endif

	/* loop through all edges of the polygon */
	getPoint2d_p(ring, 0, &v1);
    	for (i=0; i<ring->npoints-1; i++)
	{   
		double vt;
		getPoint2d_p(ring, i+1, &v2);

		/* edge from vertex i to vertex i+1 */
       		if
		(
			/* an upward crossing */
			((v1.y <= p->y) && (v2.y > p->y))
			/* a downward crossing */
       		 	|| ((v1.y > p->y) && (v2.y <= p->y))
		)
	 	{

			vt = (double)(p->y - v1.y) / (v2.y - v1.y);

			/* P.x <intersect */
			if (p->x < v1.x + vt * (v2.x - v1.x))
			{
				/* a valid crossing of y=p.y right of p.x */
           	     		++cn;
			}
		}
		v1 = v2;
	}
#ifdef PGIS_DEBUG
	lwnotice("pt_in_ring_2d returning %d", cn&1);
#endif
	return (cn&1);    /* 0 if even (out), and 1 if odd (in) */
}

double distance2d_pt_pt(POINT2D *p1, POINT2D *p2)
{
	double hside = p2->x - p1->x;
	double vside = p2->y - p1->y;

	return sqrt ( hside*hside + vside*vside );

	/* the above is more readable
	   return sqrt(
	  	(p2->x-p1->x) * (p2->x-p1->x) + (p2->y-p1->y) * (p2->y-p1->y)
		);  */
}

/*distance2d from p to line A->B */
double distance2d_pt_seg(POINT2D *p, POINT2D *A, POINT2D *B)
{
	double	r,s;

	/*if start==end, then use pt distance */
	if (  ( A->x == B->x) && (A->y == B->y) )
		return distance2d_pt_pt(p,A);

	/*
	 * otherwise, we use comp.graphics.algorithms
	 * Frequently Asked Questions method
	 *
	 *  (1)     	      AC dot AB
         *         r = ---------
         *               ||AB||^2
	 *	r has the following meaning:
	 *	r=0 P = A
	 *	r=1 P = B
	 *	r<0 P is on the backward extension of AB
	 *	r>1 P is on the forward extension of AB
	 *	0<r<1 P is interior to AB
	 */

	r = ( (p->x-A->x) * (B->x-A->x) + (p->y-A->y) * (B->y-A->y) )/( (B->x-A->x)*(B->x-A->x) +(B->y-A->y)*(B->y-A->y) );

	if (r<0) return distance2d_pt_pt(p,A);
	if (r>1) return distance2d_pt_pt(p,B);


	/*
	 * (2)
	 *	     (Ay-Cy)(Bx-Ax)-(Ax-Cx)(By-Ay)
	 *	s = -----------------------------
	 *	             	L^2
	 *
	 *	Then the distance from C to P = |s|*L.
	 *
	 */

	s = ( (A->y-p->y)*(B->x-A->x)- (A->x-p->x)*(B->y-A->y) ) /
		( (B->x-A->x)*(B->x-A->x) +(B->y-A->y)*(B->y-A->y) );

	return LW_ABS(s) * sqrt(
		(B->x-A->x)*(B->x-A->x) + (B->y-A->y)*(B->y-A->y)
		);
}

/* find the minimum 2d distance from AB to CD */
double distance2d_seg_seg(POINT2D *A, POINT2D *B, POINT2D *C, POINT2D *D)
{

	double	s_top, s_bot,s;
	double	r_top, r_bot,r;

#ifdef PGIS_DEBUG
	lwnotice("distance2d_seg_seg [%g,%g]->[%g,%g] by [%g,%g]->[%g,%g]",
		A->x,A->y,B->x,B->y, C->x,C->y, D->x, D->y);
#endif


	/*A and B are the same point */
	if (  ( A->x == B->x) && (A->y == B->y) )
		return distance2d_pt_seg(A,C,D);

		/*U and V are the same point */

	if (  ( C->x == D->x) && (C->y == D->y) )
		return distance2d_pt_seg(D,A,B);

	/* AB and CD are line segments */
	/* from comp.graphics.algo

	Solving the above for r and s yields
				(Ay-Cy)(Dx-Cx)-(Ax-Cx)(Dy-Cy)
	           r = ----------------------------- (eqn 1)
				(Bx-Ax)(Dy-Cy)-(By-Ay)(Dx-Cx)

		 	(Ay-Cy)(Bx-Ax)-(Ax-Cx)(By-Ay)
		s = ----------------------------- (eqn 2)
			(Bx-Ax)(Dy-Cy)-(By-Ay)(Dx-Cx)
	Let P be the position vector of the intersection point, then
		P=A+r(B-A) or
		Px=Ax+r(Bx-Ax)
		Py=Ay+r(By-Ay)
	By examining the values of r & s, you can also determine some other limiting conditions:
		If 0<=r<=1 & 0<=s<=1, intersection exists
		r<0 or r>1 or s<0 or s>1 line segments do not intersect
		If the denominator in eqn 1 is zero, AB & CD are parallel
		If the numerator in eqn 1 is also zero, AB & CD are collinear.

	*/
	r_top = (A->y-C->y)*(D->x-C->x) - (A->x-C->x)*(D->y-C->y) ;
	r_bot = (B->x-A->x)*(D->y-C->y) - (B->y-A->y)*(D->x-C->x) ;

	s_top = (A->y-C->y)*(B->x-A->x) - (A->x-C->x)*(B->y-A->y);
	s_bot = (B->x-A->x)*(D->y-C->y) - (B->y-A->y)*(D->x-C->x);

	if  ( (r_bot==0) || (s_bot == 0) )
	{
		return (
			LW_MIN(distance2d_pt_seg(A,C,D),
				LW_MIN(distance2d_pt_seg(B,C,D),
					LW_MIN(distance2d_pt_seg(C,A,B),
						distance2d_pt_seg(D,A,B))
				)
			)
		);
	}
	s = s_top/s_bot;
	r=  r_top/r_bot;

	if ((r<0) || (r>1) || (s<0) || (s>1) )
	{
		/*no intersection */
		return (
			LW_MIN(distance2d_pt_seg(A,C,D),
				LW_MIN(distance2d_pt_seg(B,C,D),
					LW_MIN(distance2d_pt_seg(C,A,B),
						distance2d_pt_seg(D,A,B))
				)
			)
		);

	}
	else
		return -0; /*intersection exists */

}

/*
 * search all the segments of pointarray to see which one is closest to p1
 * Returns minimum distance between point and pointarray
 */
double distance2d_pt_ptarray(POINT2D *p, POINTARRAY *pa)
{
	double result = 0;
	int t;
	POINT2D	start, end;

	getPoint2d_p(pa, 0, &start);

	for (t=1; t<pa->npoints; t++)
	{
		double dist;
		getPoint2d_p(pa, t, &end);
		dist = distance2d_pt_seg(p, &start, &end);
		if (t==1) result = dist;
		else result = LW_MIN(result, dist);

		if ( result == 0 ) return 0;

		start = end;
	}

	return result;
}

/* test each segment of l1 against each segment of l2.  Return min */
double distance2d_ptarray_ptarray(POINTARRAY *l1, POINTARRAY *l2)
{
	double 	result = 99999999999.9;
	char result_okay = 0; /*result is a valid min */
	int t,u;
	POINT2D	start, end;
	POINT2D	start2, end2;

#ifdef PGIS_DEBUG
	lwnotice("distance2d_ptarray_ptarray called (points: %d-%d)",
			l1->npoints, l2->npoints);
#endif

	getPoint2d_p(l1, 0, &start);
	for (t=1; t<l1->npoints; t++) /*for each segment in L1 */
	{
		getPoint2d_p(l1, t, &end);

		getPoint2d_p(l2, 0, &start2);
		for (u=1; u<l2->npoints; u++) /*for each segment in L2 */
		{
			double dist;

			getPoint2d_p(l2, u, &end2);

			dist = distance2d_seg_seg(&start, &end, &start2, &end2);
#if PGIS_DEBUG > 1
printf("line_line; seg %i * seg %i, dist = %g\n",t,u,dist_this);
#endif

			if (result_okay)
				result = LW_MIN(result,dist);
			else
			{
				result_okay = 1;
				result = dist;
			}

#ifdef PGIS_DEBUG
			lwnotice(" seg%d-seg%d dist: %f, mindist: %f",
				t, u, dist, result);
#endif

			if (result <= 0) return 0; /*intersection */

			start2 = end2;
		}
		start = end;
	}

	return result;
}

/* true if point is in poly (and not in its holes) */
int pt_in_poly_2d(POINT2D *p, LWPOLY *poly)
{
	int i;

	/* Not in outer ring */
	if ( ! pt_in_ring_2d(p, poly->rings[0]) ) return 0;

	/* Check holes */
	for (i=1; i<poly->nrings; i++)
	{
		/* Inside a hole */
		if ( pt_in_ring_2d(p, poly->rings[i]) ) return 0;
	}

	return 1; /* In outer ring, not in holes */
}

/*
 * Brute force.
 * Test line-ring distance against each ring.
 * If there's an intersection (distance==0) then return 0 (crosses boundary).
 * Otherwise, test to see if any point is inside outer rings of polygon,
 * but not in inner rings.
 * If so, return 0  (line inside polygon),
 * otherwise return min distance to a ring (could be outside
 * polygon or inside a hole)
 */
double distance2d_ptarray_poly(POINTARRAY *pa, LWPOLY *poly)
{
	POINT2D pt;
	int i;
	double mindist = 0;

#ifdef PGIS_DEBUG
	lwnotice("distance2d_ptarray_poly called (%d rings)", poly->nrings);
#endif

	for (i=0; i<poly->nrings; i++)
	{
		double dist = distance2d_ptarray_ptarray(pa, poly->rings[i]);
		if (i) mindist = LW_MIN(mindist, dist);
		else mindist = dist;
#ifdef PGIS_DEBUG
	lwnotice(" distance from ring %d: %f, mindist: %f",
			i, dist, mindist);
#endif

		if ( mindist <= 0 ) return 0.0; /* intersection */
	}

	/*
	 * No intersection, have to check if a point is
	 * inside polygon
	 */
	getPoint2d_p(pa, 0, &pt);

	/*
	 * Outside outer ring, so min distance to a ring
	 * is the actual min distance
	 */
	if ( ! pt_in_ring_2d(&pt, poly->rings[0]) ) return mindist;


	/*
	 * Its in the outer ring.
	 * Have to check if its inside a hole
	 */
	for (i=1; i<poly->nrings; i++)
	{
		if ( pt_in_ring_2d(&pt, poly->rings[i]) )
		{
			/*
			 * Its inside a hole, then the actual
			 * distance is the min ring distance
			 */
			return mindist;
		}
	}

	return 0.0; /* Not in hole, so inside polygon */
}

double distance2d_point_point(LWPOINT *point1, LWPOINT *point2)
{
	POINT2D p1;
	POINT2D p2;

	getPoint2d_p(point1->point, 0, &p1);
	getPoint2d_p(point2->point, 0, &p2);

	return distance2d_pt_pt(&p1, &p2);
}

double distance2d_point_line(LWPOINT *point, LWLINE *line)
{
	POINT2D p;
	POINTARRAY *pa = line->points;
	getPoint2d_p(point->point, 0, &p);
	return distance2d_pt_ptarray(&p, pa);
}

double distance2d_line_line(LWLINE *line1, LWLINE *line2)
{
	POINTARRAY *pa1 = line1->points;
	POINTARRAY *pa2 = line2->points;
	return distance2d_ptarray_ptarray(pa1, pa2);
}

/*
 * 1. see if pt in outer boundary. if no, then treat the outer ring like a line
 * 2. if in the boundary, test to see if its in a hole.
 *    if so, then return dist to hole, else return 0 (point in polygon)
 */
double distance2d_point_poly(LWPOINT *point, LWPOLY *poly)
{
	POINT2D p;
	int i;

	getPoint2d_p(point->point, 0, &p);

#ifdef PGIS_DEBUG
	lwnotice("distance2d_point_poly called");
#endif

	/* Return distance to outer ring if not inside it */
	if ( ! pt_in_ring_2d(&p, poly->rings[0]) )
	{
#ifdef PGIS_DEBUG
	lwnotice(" not inside outer-ring");
#endif
		return distance2d_pt_ptarray(&p, poly->rings[0]);
	}

	/*
	 * Inside the outer ring.
	 * Scan though each of the inner rings looking to
	 * see if its inside.  If not, distance==0.
	 * Otherwise, distance = pt to ring distance
	 */
	for (i=1; i<poly->nrings; i++) 
	{
		/* Inside a hole. Distance = pt -> ring */
		if ( pt_in_ring_2d(&p, poly->rings[i]) )
		{
#ifdef PGIS_DEBUG
			lwnotice(" inside an hole");
#endif
			return distance2d_pt_ptarray(&p, poly->rings[i]);
		}
	}

#ifdef PGIS_DEBUG
	lwnotice(" inside the polygon");
#endif
	return 0.0; /* Is inside the polygon */
}

/*
 * Brute force.
 * Test to see if any rings intersect.
 * If yes, dist=0.
 * Test to see if one inside the other and if they are inside holes.
 * Find min distance ring-to-ring.
 */
double distance2d_poly_poly(LWPOLY *poly1, LWPOLY *poly2)
{
	POINT2D pt;
	double mindist = -1;
	int i;

#ifdef PGIS_DEBUG
	lwnotice("distance2d_poly_poly called");
#endif

	/* if poly1 inside poly2 return 0 */
	getPoint2d_p(poly1->rings[0], 0, &pt);
	if ( pt_in_poly_2d(&pt, poly2) ) return 0.0;  

	/* if poly2 inside poly1 return 0 */
	getPoint2d_p(poly2->rings[0], 0, &pt);
	if ( pt_in_poly_2d(&pt, poly1) ) return 0.0;  

#ifdef PGIS_DEBUG
	lwnotice("  polys not inside each other");
#endif

	/*
	 * foreach ring in Poly1
	 * foreach ring in Poly2
	 *   if intersect, return 0
	 */
	for (i=0; i<poly1->nrings; i++)
	{
		int j;
		for (j=0; j<poly2->nrings; j++)
		{
			double d = distance2d_ptarray_ptarray(poly1->rings[i],
				poly2->rings[j]);
			if ( d <= 0 ) return 0.0;

			/* mindist is -1 when not yet set */
			if (mindist > -1) mindist = LW_MIN(mindist, d);
			else mindist = d;
#ifdef PGIS_DEBUG
		lwnotice("  ring%i-%i dist: %f, mindist: %f", i, j, d, mindist);
#endif

		}

	}

	/* otherwise return closest approach of rings (no intersection) */
	return mindist;

}

double distance2d_line_poly(LWLINE *line, LWPOLY *poly)
{
	return distance2d_ptarray_poly(line->points, poly);
}


/*find the 2d length of the given POINTARRAY (even if it's 3d) */
double lwgeom_pointarray_length2d(POINTARRAY *pts)
{
	double dist = 0.0;
	int i;
	POINT2D frm;
	POINT2D to;

	if ( pts->npoints < 2 ) return 0.0;
	for (i=0; i<pts->npoints-1;i++)
	{
		getPoint2d_p(pts, i, &frm);
		getPoint2d_p(pts, i+1, &to);
		dist += sqrt( ( (frm.x - to.x)*(frm.x - to.x) )  +
				((frm.y - to.y)*(frm.y - to.y) ) );
	}
	return dist;
}

/*
 * Find the 3d/2d length of the given POINTARRAY
 * (depending on its dimensions)
 */
double
lwgeom_pointarray_length(POINTARRAY *pts)
{
	double dist = 0.0;
	int i;
	POINT3DZ frm;
	POINT3DZ to;

	if ( pts->npoints < 2 ) return 0.0;

	/* compute 2d length if 3d is not available */
	if ( ! TYPE_HASZ(pts->dims) ) return lwgeom_pointarray_length2d(pts);

	for (i=0; i<pts->npoints-1;i++)
	{
		getPoint3dz_p(pts, i, &frm);
		getPoint3dz_p(pts, i+1, &to);
		dist += sqrt( ( (frm.x - to.x)*(frm.x - to.x) )  +
				((frm.y - to.y)*(frm.y - to.y) ) +
				((frm.z - to.z)*(frm.z - to.z) ) );
	}

	return dist;
}

/*
 * This should be rewritten to make use of the curve itself.
 */
double
lwgeom_curvepolygon_area(LWCURVEPOLY *curvepoly)
{
        LWPOLY *poly = (LWPOLY *)lwgeom_segmentize((LWGEOM *)curvepoly, 32);
        return lwgeom_polygon_area(poly);
}

/*
 * Find the area of the outer ring - sum (area of inner rings).
 * Could use a more numerically stable calculator...
 */
double
lwgeom_polygon_area(LWPOLY *poly)
{
	double poly_area=0.0;
	int i;
	POINT2D p1;
	POINT2D p2;

#ifdef PGIS_DEBUG
	lwnotice("in lwgeom_polygon_area (%d rings)", poly->nrings);
#endif

	for (i=0; i<poly->nrings; i++)
	{
		int j;
		POINTARRAY *ring = poly->rings[i];
		double ringarea = 0.0;

#if PGIS_DEBUG > 1
lwnotice(" rings %d has %d points", i, ring->npoints);
#endif
		for (j=0; j<ring->npoints-1; j++)
    		{
			getPoint2d_p(ring, j, &p1);
			getPoint2d_p(ring, j+1, &p2);
			ringarea += ( p1.x * p2.y ) - ( p1.y * p2.x );
		}

		ringarea  /= 2.0;
#if PGIS_DEBUG > 1
lwnotice(" ring 1 has area %lf",ringarea);
#endif
		ringarea  = fabs(ringarea);
		if (i != 0)	/*outer */
			ringarea  = -1.0*ringarea ; /* its a hole */

		poly_area += ringarea;
	}

	return poly_area;
}

/*
 * Compute the sum of polygon rings length.
 * Could use a more numerically stable calculator...
 */
double lwgeom_polygon_perimeter(LWPOLY *poly)
{
	double result=0.0;
	int i;

#ifdef PGIS_DEBUG
lwnotice("in lwgeom_polygon_perimeter (%d rings)", poly->nrings);
#endif

	for (i=0; i<poly->nrings; i++)
		result += lwgeom_pointarray_length(poly->rings[i]);

	return result;
}

/*
 * Compute the sum of polygon rings length (forcing 2d computation).
 * Could use a more numerically stable calculator...
 */
double lwgeom_polygon_perimeter2d(LWPOLY *poly)
{
	double result=0.0;
	int i;

#ifdef PGIS_DEBUG
lwnotice("in lwgeom_polygon_perimeter (%d rings)", poly->nrings);
#endif

	for (i=0; i<poly->nrings; i++)
		result += lwgeom_pointarray_length2d(poly->rings[i]);

	return result;
}

double
lwgeom_mindistance2d_recursive(uchar *lw1, uchar *lw2)
{
  return lwgeom_mindistance2d_recursive_tolerance( lw1, lw2, 0.0 );
}

double
lwgeom_mindistance2d_recursive_tolerance(uchar *lw1, uchar *lw2, double tolerance)
{
	LWGEOM_INSPECTED *in1, *in2;
	int i, j;
	double mindist = -1;

	in1 = lwgeom_inspect(lw1);
	in2 = lwgeom_inspect(lw2);

	for (i=0; i<in1->ngeometries; i++)
	{
		uchar *g1 = lwgeom_getsubgeometry_inspected(in1, i);
		int t1 = lwgeom_getType(g1[0]);
		double dist=tolerance;

		/* it's a multitype... recurse */
		if ( lwgeom_contains_subgeoms(t1) )
		{
			dist = lwgeom_mindistance2d_recursive_tolerance(g1, lw2, tolerance);
			if ( dist <= tolerance ) return tolerance; /* can't be closer */
			if ( mindist == -1 ) mindist = dist;
			else mindist = LW_MIN(dist, mindist);
			continue;
		}

		for (j=0; j<in2->ngeometries; j++)
		{
			uchar *g2 = lwgeom_getsubgeometry_inspected(in2, j);
			int t2 = lwgeom_getType(g2[0]);

			if  ( t1 == POINTTYPE )
			{
				if  ( t2 == POINTTYPE )
				{
					dist = distance2d_point_point(
						lwpoint_deserialize(g1),
						lwpoint_deserialize(g2)
					);
				}
				else if  ( t2 == LINETYPE )
				{
					dist = distance2d_point_line(
						lwpoint_deserialize(g1),
						lwline_deserialize(g2)
					);
				}
				else if  ( t2 == POLYGONTYPE )
				{
					dist = distance2d_point_poly(
						lwpoint_deserialize(g1),
						lwpoly_deserialize(g2)
					);
				}
				else
				{
					lwerror("Unsupported geometry type: %s", lwgeom_typename(t2));
				}
			}
			else if ( t1 == LINETYPE )
			{
				if ( t2 == POINTTYPE )
				{
					dist = distance2d_point_line(
						lwpoint_deserialize(g2),
						lwline_deserialize(g1)
					);
				}
				else if ( t2 == LINETYPE )
				{
					dist = distance2d_line_line(
						lwline_deserialize(g1),
						lwline_deserialize(g2)
					);
				}
				else if ( t2 == POLYGONTYPE )
				{
					dist = distance2d_line_poly(
						lwline_deserialize(g1),
						lwpoly_deserialize(g2)
					);
				}
				else
				{
					lwerror("Unsupported geometry type: %s", lwgeom_typename(t2));
				}
			}
			else if ( t1 == POLYGONTYPE )
			{
				if ( t2 == POLYGONTYPE )
				{
					dist = distance2d_poly_poly(
						lwpoly_deserialize(g2),
						lwpoly_deserialize(g1)
					);
				}
				else if ( t2 == POINTTYPE )
				{
					dist = distance2d_point_poly(
						lwpoint_deserialize(g2),
						lwpoly_deserialize(g1)
					);
				}
				else if ( t2 == LINETYPE )
				{
					dist = distance2d_line_poly(
						lwline_deserialize(g2),
						lwpoly_deserialize(g1)
					);
				}
				else
				{
					lwerror("Unsupported geometry type: %s", lwgeom_typename(t2));
				}
			}
			else if (lwgeom_contains_subgeoms(t1)) /* it's a multitype... recurse */
			{
				dist = lwgeom_mindistance2d_recursive_tolerance(g1, g2, tolerance);
			}
			else
			{
				lwerror("Unsupported geometry type: %s", lwgeom_typename(t1));
			}

			if (mindist == -1 ) mindist = dist;
			else mindist = LW_MIN(dist, mindist);

#ifdef PGIS_DEBUG
		lwnotice("dist %d-%d: %f - mindist: %f",
				i, j, dist, mindist);
#endif


			if (mindist <= tolerance) return tolerance; /* can't be closer */

		}

	}

	if (mindist<0) mindist = 0; 

	return mindist;
}



int
lwgeom_pt_inside_circle(POINT2D *p, double cx, double cy, double rad)
{
	POINT2D center;

	center.x = cx;
	center.y = cy;

	if ( distance2d_pt_pt(p, &center) < rad ) return 1;
	else return 0;

}

/*
 * Compute the azimuth of segment AB in radians.
 * Return 0 on exception (same point), 1 otherwise.
 */
int
azimuth_pt_pt(POINT2D *A, POINT2D *B, double *d)
{
	if ( A->x == B->x )
	{
		if ( A->y < B->y ) *d=0.0;
		else if ( A->y > B->y ) *d=M_PI; 
		else return 0;
		return 1;
	}

	if ( A->y == B->y )
	{
		if ( A->x < B->x ) *d=M_PI/2; 
		else if ( A->x > B->x ) *d=M_PI+(M_PI/2);
		else return 0;
		return 1;
	}

	if ( A->x < B->x )
	{
		if ( A->y < B->y )
		{
			*d=atan(fabs(A->x - B->x) / fabs(A->y - B->y) );
		}
		else /* ( A->y > B->y )  - equality case handled above */
		{
			*d=atan(fabs(A->y - B->y) / fabs(A->x - B->x) )
				+ (M_PI/2);
		}
	}

	else /* ( A->x > B->x ) - equality case handled above */
	{
		if ( A->y > B->y )
		{
			*d=atan(fabs(A->x - B->x) / fabs(A->y - B->y) )
				+ M_PI;
		}
		else /* ( A->y < B->y )  - equality case handled above */
		{
			*d=atan(fabs(A->y - B->y) / fabs(A->x - B->x) )
				+ (M_PI+(M_PI/2));
		}
	}

	return 1;
}

