#include <math.h>
#include <string.h>

#include "liblwgeom.h"

//#define DEBUG

// pt_in_ring_2d(): crossing number test for a point in a polygon
//      input:   p = a point,
//               pa = vertex points of a ring V[n+1] with V[n]=V[0]
//      returns: 0 = outside, 1 = inside
//
//	Our polygons have first and last point the same,
//
int pt_in_ring_2d(POINT2D *p, POINTARRAY *ring)
{
	int cn = 0;    // the crossing number counter
	int i;
	POINT2D *v1, *v2;

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

#ifdef DEBUG
	lwnotice("pt_in_ring_2d called with point: %g %g", p->x, p->y);
	printPA(ring);
#endif

	// loop through all edges of the polygon
	v1 = (POINT2D *)getPoint(ring, 0);
    	for (i=0; i<ring->npoints-1; i++)
	{   
		double vt;
		v2 = (POINT2D *)getPoint(ring, i+1);

		// edge from vertex i to vertex i+1
       		if
		(
			// an upward crossing
			((v1->y <= p->y) && (v2->y > p->y))
			// a downward crossing
       		 	|| ((v1->y > p->y) && (v2->y <= p->y))
		)
	 	{

			vt = (double)(p->y - v1->y) / (v2->y - v1->y);

			// P.x <intersect
			if (p->x < v1->x + vt * (v2->x - v1->x))
			{
				// a valid crossing of y=p.y right of p.x
           	     		++cn;
			}
		}
		v1 = v2;
	}
#ifdef DEBUG
	lwnotice("pt_in_ring_2d returning %d", cn&1);
#endif
	return (cn&1);    // 0 if even (out), and 1 if odd (in)
}

double distance2d_pt_pt(POINT2D *p1, POINT2D *p2)
{
	return sqrt(
		(p2->x-p1->x) * (p2->x-p1->x) + (p2->y-p1->y) * (p2->y-p1->y)
		); 
}

//distance2d from p to line A->B
double distance2d_pt_seg(POINT2D *p, POINT2D *A, POINT2D *B)
{
	double	r,s;

	//if start==end, then use pt distance
	if (  ( A->x == B->x) && (A->y == B->y) )
		return distance2d_pt_pt(p,A);

	//otherwise, we use comp.graphics.algorithms Frequently Asked Questions method

	/*(1)     	      AC dot AB
                   r = ---------
                         ||AB||^2
		r has the following meaning:
		r=0 P = A
		r=1 P = B
		r<0 P is on the backward extension of AB
		r>1 P is on the forward extension of AB
		0<r<1 P is interior to AB
	*/

	r = ( (p->x-A->x) * (B->x-A->x) + (p->y-A->y) * (B->y-A->y) )/( (B->x-A->x)*(B->x-A->x) +(B->y-A->y)*(B->y-A->y) );

	if (r<0) return distance2d_pt_pt(p,A);
	if (r>1) return distance2d_pt_pt(p,B);


	/*(2)
		     (Ay-Cy)(Bx-Ax)-(Ax-Cx)(By-Ay)
		s = -----------------------------
		             	L^2

		Then the distance from C to P = |s|*L.

	*/

	s = ( (A->y-p->y)*(B->x-A->x)- (A->x-p->x)*(B->y-A->y) ) /
		( (B->x-A->x)*(B->x-A->x) +(B->y-A->y)*(B->y-A->y) );

	return LW_ABS(s) * sqrt(
		(B->x-A->x)*(B->x-A->x) + (B->y-A->y)*(B->y-A->y)
		);
}

// find the minimum 2d distance from AB to CD
double distance2d_seg_seg(POINT2D *A, POINT2D *B, POINT2D *C, POINT2D *D)
{

	double	s_top, s_bot,s;
	double	r_top, r_bot,r;

#ifdef DEBUG
	lwnotice("distance2d_seg_seg [%g,%g]->[%g,%g] by [%g,%g]->[%g,%g]",
		A->x,A->y,B->x,B->y, C->x,C->y, D->x, D->y);
#endif


	//A and B are the same point
	if (  ( A->x == B->x) && (A->y == B->y) )
		return distance2d_pt_seg(A,C,D);

		//U and V are the same point

	if (  ( C->x == D->x) && (C->y == D->y) )
		return distance2d_pt_seg(D,A,B);

	// AB and CD are line segments
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
		//no intersection
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
		return -0; //intersection exists

}

//search all the segments of pointarray to see which one is closest to p1
//Returns minimum distance between point and pointarray
double distance2d_pt_ptarray(POINT2D *p, POINTARRAY *pa)
{
	double result = 0;
	int t;
	POINT2D	*start, *end;

	start = (POINT2D *)getPoint(pa, 0);

	for (t=1; t<pa->npoints; t++)
	{
		double dist;
		end = (POINT2D *)getPoint(pa, t);
		dist = distance2d_pt_seg(p, start, end);
		if (t==1) result = dist;
		else result = LW_MIN(result, dist);

		if ( result == 0 ) return 0;

		start = end;
	}

	return result;
}

// test each segment of l1 against each segment of l2.  Return min
double distance2d_ptarray_ptarray(POINTARRAY *l1, POINTARRAY *l2)
{
	double 	result = 99999999999.9;
	char result_okay = 0; //result is a valid min
	int t,u;
	POINT2D	*start,*end;
	POINT2D	*start2,*end2;

#ifdef DEBUG
	lwnotice("distance2d_ptarray_ptarray called (points: %d-%d)",
			l1->npoints, l2->npoints);
#endif

	start = (POINT2D *)getPoint(l1, 0);
	for (t=1; t<l1->npoints; t++) //for each segment in L1
	{
		end = (POINT2D *)getPoint(l1, t);

		start2 = (POINT2D *)getPoint(l2, 0);
		for (u=1; u<l2->npoints; u++) //for each segment in L2
		{
			double dist;

			end2 = (POINT2D *)getPoint(l2, u);

			dist = distance2d_seg_seg(start, end, start2, end2);
//printf("line_line; seg %i * seg %i, dist = %g\n",t,u,dist_this);

			if (result_okay)
				result = LW_MIN(result,dist);
			else
			{
				result_okay = 1;
				result = dist;
			}

#ifdef DEBUG
			lwnotice(" seg%d-seg%d dist: %f, mindist: %f",
				t, u, dist, result);
#endif

			if (result <= 0) return 0; //intersection

			start2 = end2;
		}
		start = end;
	}

	return result;
}

// true if point is in poly (and not in its holes)
int pt_in_poly_2d(POINT2D *p, LWPOLY *poly)
{
	int i;

	// Not in outer ring
	if ( ! pt_in_ring_2d(p, poly->rings[0]) ) return 0;

	// Check holes
	for (i=1; i<poly->nrings; i++)
	{
		// Inside a hole
		if ( pt_in_ring_2d(p, poly->rings[i]) ) return 0;
	}

	return 1; // In outer ring, not in holes
}

// Brute force.
// Test line-ring distance against each ring.
// If there's an intersection (distance==0) then return 0 (crosses boundary).
// Otherwise, test to see if any point is inside outer rings of polygon,
// but not in inner rings.
// If so, return 0  (line inside polygon),
// otherwise return min distance to a ring (could be outside
// polygon or inside a hole)
double distance2d_ptarray_poly(POINTARRAY *pa, LWPOLY *poly)
{
	POINT2D *pt;
	int i;
	double mindist = 0;

#ifdef DEBUG
	lwnotice("distance2d_ptarray_poly called (%d rings)", poly->nrings);
#endif

	for (i=0; i<poly->nrings; i++)
	{
		double dist = distance2d_ptarray_ptarray(pa, poly->rings[i]);
		if (i) mindist = LW_MIN(mindist, dist);
		else mindist = dist;
#ifdef DEBUG
	lwnotice(" distance from ring %d: %f, mindist: %f",
			i, dist, mindist);
#endif

		if ( mindist <= 0 ) return 0.0; // intersection
	}

	// No intersection, have to check if a point is
	// inside polygon
	pt = (POINT2D *)getPoint(pa, 0);

	// Outside outer ring, so min distance to a ring
	// is the actual min distance
	if ( ! pt_in_ring_2d(pt, poly->rings[0]) ) return mindist;

	// Its in the outer ring.
	// Have to check if its inside a hole
	for (i=1; i<poly->nrings; i++)
	{
		if ( pt_in_ring_2d(pt, poly->rings[i]) )
		{
			// Its inside a hole, then the actual
			// distance is the min ring distance
			return mindist;
		}
	}

	return 0.0; // Not in hole, so inside polygon
}

double distance2d_point_point(LWPOINT *point1, LWPOINT *point2)
{
	POINT2D *p1 = (POINT2D *)getPoint(point1->point, 0);
	POINT2D *p2 = (POINT2D *)getPoint(point2->point, 0);
	return distance2d_pt_pt(p1, p2);
}

double distance2d_point_line(LWPOINT *point, LWLINE *line)
{
	POINT2D *p = (POINT2D *)getPoint(point->point, 0);
	POINTARRAY *pa = line->points;
	return distance2d_pt_ptarray(p, pa);
}

double distance2d_line_line(LWLINE *line1, LWLINE *line2)
{
	POINTARRAY *pa1 = line1->points;
	POINTARRAY *pa2 = line2->points;
	return distance2d_ptarray_ptarray(pa1, pa2);
}

// 1. see if pt in outer boundary. if no, then treat the outer ring like a line
// 2. if in the boundary, test to see if its in a hole.
//    if so, then return dist to hole, else return 0 (point in polygon)
double distance2d_point_poly(LWPOINT *point, LWPOLY *poly)
{
	POINT2D *p = (POINT2D *)getPoint(point->point, 0);
	int i;

#ifdef DEBUG
	lwnotice("distance2d_point_poly called");
#endif

	// Return distance to outer ring if not inside it
	if ( ! pt_in_ring_2d(p, poly->rings[0]) )
	{
#ifdef DEBUG
	lwnotice(" not inside outer-ring");
#endif
		return distance2d_pt_ptarray(p, poly->rings[0]);
	}

	// Inside the outer ring.
	// Scan though each of the inner rings looking to
	// see if its inside.  If not, distance==0.
	// Otherwise, distance = pt to ring distance
	for (i=1; i<poly->nrings; i++) 
	{
		// Inside a hole. Distance = pt -> ring
		if ( pt_in_ring_2d(p, poly->rings[i]) )
		{
#ifdef DEBUG
			lwnotice(" inside an hole");
#endif
			return distance2d_pt_ptarray(p, poly->rings[i]);
		}
	}

#ifdef DEBUG
	lwnotice(" inside the polygon");
#endif
	return 0.0; // Is inside the polygon
}

// Brute force.
// Test to see if any rings intersect.
// If yes, dist=0.
// Test to see if one inside the other and if they are inside holes.
// Find min distance ring-to-ring.
double distance2d_poly_poly(LWPOLY *poly1, LWPOLY *poly2)
{
	POINT2D *pt;
	double mindist = 0;
	int i;

#ifdef DEBUG
	lwnotice("distance2d_poly_poly called");
#endif

	// if poly1 inside poly2 return 0
	pt = (POINT2D *)getPoint(poly1->rings[0], 0);
	if ( pt_in_poly_2d(pt, poly2) ) return 0.0;  

	// if poly2 inside poly1 return 0
	pt = (POINT2D *)getPoint(poly2->rings[0], 0);
	if ( pt_in_poly_2d(pt, poly1) ) return 0.0;  

#ifdef DEBUG
	lwnotice("  polys not inside each other");
#endif

	//foreach ring in Poly1
	// foreach ring in Poly2
	//   if intersect, return 0
	for (i=0; i<poly1->nrings; i++)
	{
		double dist = distance2d_ptarray_poly(poly1->rings[i], poly2);
		if (i) mindist = LW_MIN(mindist, dist);
		else mindist = dist;

#ifdef DEBUG
		lwnotice("  ring%d dist: %f, mindist: %f", i, dist, mindist);
#endif

		if ( mindist <= 0 ) return 0.0; // intersection
	}

	// otherwise return closest approach of rings (no intersection)
	return mindist;

}

double distance2d_line_poly(LWLINE *line, LWPOLY *poly)
{
	return distance2d_ptarray_poly(line->points, poly);
}


//find the 2d length of the given POINTARRAY (even if it's 3d)
double lwgeom_pointarray_length2d(POINTARRAY *pts)
{
	double dist = 0.0;
	int i;

	if ( pts->npoints < 2 ) return 0.0;
	for (i=0; i<pts->npoints-1;i++)
	{
		POINT2D *frm = (POINT2D *)getPoint(pts, i);
		POINT2D *to = (POINT2D *)getPoint(pts, i+1);
		dist += sqrt( ( (frm->x - to->x)*(frm->x - to->x) )  +
				((frm->y - to->y)*(frm->y - to->y) ) );
	}
	return dist;
}

//find the 3d/2d length of the given POINTARRAY (depending on its dimensions)
double
lwgeom_pointarray_length(POINTARRAY *pts)
{
	double dist = 0.0;
	int i;

	if ( pts->npoints < 2 ) return 0.0;

	// compute 2d length if 3d is not available
	if ( ! TYPE_HASZ(pts->dims) ) return lwgeom_pointarray_length2d(pts);

	for (i=0; i<pts->npoints-1;i++)
	{
		POINT3DZ *frm = (POINT3DZ *)getPoint(pts, i);
		POINT3DZ *to = (POINT3DZ *)getPoint(pts, i+1);
		dist += sqrt( ( (frm->x - to->x)*(frm->x - to->x) )  +
				((frm->y - to->y)*(frm->y - to->y) ) +
				((frm->z - to->z)*(frm->z - to->z) ) );
	}

	return dist;
}

//find the area of the outer ring - sum (area of inner rings)
// Could use a more numerically stable calculator...
double lwgeom_polygon_area(LWPOLY *poly)
{
	double poly_area=0.0;
	int i;

#ifdef DEBUG
	lwnotice("in lwgeom_polygon_area (%d rings)", poly->nrings);
#endif

	for (i=0; i<poly->nrings; i++)
	{
		int j;
		POINTARRAY *ring = poly->rings[i];
		double ringarea = 0.0;

//lwnotice(" rings %d has %d points", i, ring->npoints);
		for (j=0; j<ring->npoints-1; j++)
    		{
			POINT2D *p1 = (POINT2D *)getPoint(ring, j);
			POINT2D *p2 = (POINT2D *)getPoint(ring, j+1);
			ringarea += ( p1->x * p2->y ) - ( p1->y * p2->x );
		}

		ringarea  /= 2.0;
//lwnotice(" ring 1 has area %lf",ringarea);
		ringarea  = fabs(ringarea);
		if (i != 0)	//outer
			ringarea  = -1.0*ringarea ; // its a hole

		poly_area += ringarea;
	}

	return poly_area;
}

// Compute the sum of polygon rings length.
// Could use a more numerically stable calculator...
double lwgeom_polygon_perimeter(LWPOLY *poly)
{
	double result=0.0;
	int i;

//lwnotice("in lwgeom_polygon_perimeter (%d rings)", poly->nrings);

	for (i=0; i<poly->nrings; i++)
		result += lwgeom_pointarray_length(poly->rings[i]);

	return result;
}

// Compute the sum of polygon rings length (forcing 2d computation).
// Could use a more numerically stable calculator...
double lwgeom_polygon_perimeter2d(LWPOLY *poly)
{
	double result=0.0;
	int i;

//lwnotice("in lwgeom_polygon_perimeter (%d rings)", poly->nrings);

	for (i=0; i<poly->nrings; i++)
		result += lwgeom_pointarray_length2d(poly->rings[i]);

	return result;
}

double
lwgeom_mindistance2d_recursive(char *lw1, char *lw2)
{
	LWGEOM_INSPECTED *in1, *in2;
	int i, j;
	double mindist = -1;

	in1 = lwgeom_inspect(lw1);
	in2 = lwgeom_inspect(lw2);

	for (i=0; i<in1->ngeometries; i++)
	{
		char *g1 = lwgeom_getsubgeometry_inspected(in1, i);
		int t1 = lwgeom_getType(g1[0]);
		double dist=0;

		// it's a multitype... recurse
		if ( t1 >= 4 )
		{
			dist = lwgeom_mindistance2d_recursive(g1, lw2);
			if ( dist == 0 ) return 0.0; // can't be closer
			if ( mindist == -1 ) mindist = dist;
			else mindist = LW_MIN(dist, mindist);
			continue;
		}

		for (j=0; j<in2->ngeometries; j++)
		{
			char *g2 = lwgeom_getsubgeometry_inspected(in2, j);
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
			}
			else // it's a multitype... recurse
			{
				dist = lwgeom_mindistance2d_recursive(g1, g2);
			}

			if (mindist == -1 ) mindist = dist;
			else mindist = LW_MIN(dist, mindist);

#ifdef DEBUG
		lwnotice("dist %d-%d: %f - mindist: %f",
				t1, t2, dist, mindist);
#endif


			if (mindist <= 0.0) return 0.0; // can't be closer

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

