/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2006 Refractions Research Inc.
 * Copyright 2010 Nicklas Avén
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <string.h>
#include <stdlib.h>

#include "measures.h"
#include "lwgeom_log.h"


/*------------------------------------------------------------------------------------------------------------
Initializing functions
The functions starting the distance-calculation processses
--------------------------------------------------------------------------------------------------------------*/

/**
Function initializing shortestline and longestline calculations.
*/
LWGEOM *
lw_dist2d_distanceline(LWGEOM *lw1, LWGEOM *lw2,int srid,int mode)
{
	double x1,x2,y1,y2;

	double initdistance = ( mode == DIST_MIN ? MAXFLOAT : -1.0);
	DISTPTS thedl;
	LWPOINT *lwpoints[2];
	LWGEOM *result;

	thedl.mode = mode;
	thedl.distance = initdistance;
	thedl.tolerance = 0.0;

	LWDEBUG(2, "lw_dist2d_distanceline is called");

	if (!lw_dist2d_comp( lw1,lw2,&thedl))
	{
		/*should never get here. all cases ought to be error handled earlier*/
		lwerror("Some unspecified error.");
		result = (LWGEOM *)lwcollection_construct_empty(COLLECTIONTYPE, srid, 0, 0);
	}

	/*if thedl.distance is unchanged there where only empty geometries input*/
	if (thedl.distance == initdistance)
	{
		LWDEBUG(3, "didn't find geometries to measure between, returning null");
		result = (LWGEOM *)lwcollection_construct_empty(COLLECTIONTYPE, srid, 0, 0);
	}
	else
	{
		x1=thedl.p1.x;
		y1=thedl.p1.y;
		x2=thedl.p2.x;
		y2=thedl.p2.y;

		lwpoints[0] = lwpoint_make2d(srid, x1, y1);
		lwpoints[1] = lwpoint_make2d(srid, x2, y2);

		result = (LWGEOM *)lwline_from_ptarray(srid, 2, lwpoints);
	}
	return result;
}

/**
Function initializing closestpoint calculations.
*/
LWGEOM *
lw_dist2d_distancepoint(LWGEOM *lw1, LWGEOM *lw2,int srid,int mode)
{
	double x,y;
	DISTPTS thedl;
	double initdistance = MAXFLOAT;
	LWGEOM *result;

	thedl.mode = mode;
	thedl.distance= initdistance;
	thedl.tolerance = 0;

	LWDEBUG(2, "lw_dist2d_distancepoint is called");

	if (!lw_dist2d_comp( lw1,lw2,&thedl))
	{
		/*should never get here. all cases ought to be error handled earlier*/
		lwerror("Some unspecified error.");
		result = (LWGEOM *)lwcollection_construct_empty(COLLECTIONTYPE, srid, 0, 0);
	}
	if (thedl.distance == initdistance)
	{
		LWDEBUG(3, "didn't find geometries to measure between, returning null");
		result = (LWGEOM *)lwcollection_construct_empty(COLLECTIONTYPE, srid, 0, 0);
	}
	else
	{
		x=thedl.p1.x;
		y=thedl.p1.y;
		result = (LWGEOM *)lwpoint_make2d(srid, x, y);
	}
	return result;
}


/**
Function initialazing max distance calculation
*/
double
lwgeom_maxdistance2d(LWGEOM *lw1, LWGEOM *lw2)
{
	LWDEBUG(2, "lwgeom_maxdistance2d is called");

	return lwgeom_maxdistance2d_tolerance( lw1, lw2, 0.0 );
}

/**
Function handling max distance calculations and dfyllywithin calculations.
The difference is just the tolerance.
*/
double
lwgeom_maxdistance2d_tolerance(LWGEOM *lw1, LWGEOM *lw2, double tolerance)
{
	/*double thedist;*/
	DISTPTS thedl;
	LWDEBUG(2, "lwgeom_maxdistance2d_tolerance is called");
	thedl.mode = DIST_MAX;
	thedl.distance= -1;
	thedl.tolerance = tolerance;
	if (lw_dist2d_comp( lw1,lw2,&thedl))
	{
		return thedl.distance;
	}
	/*should never get here. all cases ought to be error handled earlier*/
	lwerror("Some unspecified error.");
	return -1;
}

/**
	Function initialazing min distance calculation
*/
double
lwgeom_mindistance2d(LWGEOM *lw1, LWGEOM *lw2)
{
	LWDEBUG(2, "lwgeom_mindistance2d is called");
	return lwgeom_mindistance2d_tolerance( lw1, lw2, 0.0 );
}

/**
	Function handling min distance calculations and dwithin calculations.
	The difference is just the tolerance.
*/
double
lwgeom_mindistance2d_tolerance(LWGEOM *lw1, LWGEOM *lw2, double tolerance)
{
	DISTPTS thedl;
	LWDEBUG(2, "lwgeom_mindistance2d_tolerance is called");
	thedl.mode = DIST_MIN;
	thedl.distance= MAXFLOAT;
	thedl.tolerance = tolerance;
	if (lw_dist2d_comp( lw1,lw2,&thedl))
	{
		return thedl.distance;
	}
	/*should never get here. all cases ought to be error handled earlier*/
	lwerror("Some unspecified error.");
	return MAXFLOAT;
}


/*------------------------------------------------------------------------------------------------------------
End of Initializing functions
--------------------------------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------------------
Preprocessing functions
Functions preparing geometries for distance-calculations
--------------------------------------------------------------------------------------------------------------*/

/**
	This function just deserializes geometries
	Bboxes is not checked here since it is the subgeometries
	bboxes we will use anyway.
*/
int
lw_dist2d_comp(LWGEOM *lw1, LWGEOM *lw2, DISTPTS *dl)
{
	LWDEBUG(2, "lw_dist2d_comp is called");

	return lw_dist2d_recursive(lw1, lw2, dl);
}

/**
This is a recursive function delivering every possible combinatin of subgeometries
*/
int lw_dist2d_recursive(const LWGEOM *lwg1, const LWGEOM *lwg2, DISTPTS *dl)
{
	int i, j;
	int n1=1;
	int n2=1;
	LWGEOM *g1 = NULL;
	LWGEOM *g2 = NULL;
	LWCOLLECTION *c1 = NULL;
	LWCOLLECTION *c2 = NULL;

	LWDEBUGF(2, "lw_dist2d_comp is called with type1=%d, type2=%d", lwg1->type, lwg2->type);

	if (lwgeom_is_collection(lwg1))
	{
		LWDEBUG(3, "First geometry is collection");
		c1 = lwgeom_as_lwcollection(lwg1);
		n1 = c1->ngeoms;
	}
	if (lwgeom_is_collection(lwg2))
	{
		LWDEBUG(3, "Second geometry is collection");
		c2 = lwgeom_as_lwcollection(lwg2);
		n2 = c2->ngeoms;
	}

	for ( i = 0; i < n1; i++ )
	{

		if (lwgeom_is_collection(lwg1))
		{
			g1 = c1->geoms[i];
		}
		else
		{
			g1 = (LWGEOM*)lwg1;
		}

		if (lwgeom_is_empty(g1)) return LW_TRUE;

		if (lwgeom_is_collection(g1))
		{
			LWDEBUG(3, "Found collection inside first geometry collection, recursing");
			if (!lw_dist2d_recursive(g1, lwg2, dl)) return LW_FALSE;
			continue;
		}
		for ( j = 0; j < n2; j++ )
		{
			if (lwgeom_is_collection(lwg2))
			{
				g2 = c2->geoms[j];
			}
			else
			{
				g2 = (LWGEOM*)lwg2;
			}
			if (lwgeom_is_collection(g2))
			{
				LWDEBUG(3, "Found collection inside second geometry collection, recursing");
				if (!lw_dist2d_recursive(g1, g2, dl)) return LW_FALSE;
				continue;
			}

			if ( ! g1->bbox )
			{
				lwgeom_add_bbox(g1);
			}
			if ( ! g2->bbox )
			{
				lwgeom_add_bbox(g2);
			}

			/*If one of geometries is empty, return. True here only means continue searching. False would have stoped the process*/
			if (lwgeom_is_empty(g1)||lwgeom_is_empty(g2)) return LW_TRUE;

			if ((dl->mode==DIST_MAX)||(g1->type==POINTTYPE) ||(g2->type==POINTTYPE)||lw_dist2d_check_overlap(g1, g2))
			{
				if (!lw_dist2d_distribute_bruteforce(g1, g2, dl)) return LW_FALSE;
				if (dl->distance<=dl->tolerance && dl->mode == DIST_MIN) return LW_TRUE; /*just a check if  the answer is already given*/
			}
			else
			{
				if (!lw_dist2d_distribute_fast(g1, g2, dl)) return LW_FALSE;
			}
		}
	}
	return LW_TRUE;
}



/**

This function distributes the "old-type" brut-force tasks depending on type
*/
int
lw_dist2d_distribute_bruteforce(LWGEOM *lwg1, LWGEOM *lwg2, DISTPTS *dl)
{

	int	t1 = lwg1->type;
	int	t2 = lwg2->type;

	LWDEBUGF(2, "lw_dist2d_distribute_bruteforce is called with typ1=%d, type2=%d", lwg1->type, lwg2->type);

	if  ( t1 == POINTTYPE )
	{
		if  ( t2 == POINTTYPE )
		{
			dl->twisted=1;
			return lw_dist2d_point_point((LWPOINT *)lwg1, (LWPOINT *)lwg2, dl);
		}
		else if  ( t2 == LINETYPE )
		{
			dl->twisted=1;
			return lw_dist2d_point_line((LWPOINT *)lwg1, (LWLINE *)lwg2, dl);
		}
		else if  ( t2 == POLYGONTYPE )
		{
			dl->twisted=1;
			return lw_dist2d_point_poly((LWPOINT *)lwg1, (LWPOLY *)lwg2,dl);
		}
		else
		{
			lwerror("Unsupported geometry type: %s", lwtype_name(t2));
			return LW_FALSE;
		}
	}
	else if ( t1 == LINETYPE )
	{
		if ( t2 == POINTTYPE )
		{
			dl->twisted=(-1);
			return lw_dist2d_point_line((LWPOINT *)lwg2,(LWLINE *)lwg1,dl);
		}
		else if ( t2 == LINETYPE )
		{
			dl->twisted=1;
			/*lwnotice("start line");*/
			return lw_dist2d_line_line((LWLINE *)lwg1,(LWLINE *)lwg2,dl);
		}
		else if ( t2 == POLYGONTYPE )
		{
			dl->twisted=1;
			return lw_dist2d_line_poly((LWLINE *)lwg1,(LWPOLY *)lwg2,dl);
		}
		else
		{
			lwerror("Unsupported geometry type: %s", lwtype_name(t2));
			return LW_FALSE;
		}
	}
	else if ( t1 == POLYGONTYPE )
	{
		if ( t2 == POLYGONTYPE )
		{
			dl->twisted=(1);
			return lw_dist2d_poly_poly((LWPOLY *)lwg1,(LWPOLY *)lwg2,dl);
		}
		else if ( t2 == POINTTYPE )
		{
			dl->twisted=(-1);
			return lw_dist2d_point_poly((LWPOINT *)lwg2,  (LWPOLY *)lwg1, dl);
		}
		else if ( t2 == LINETYPE )
		{
			dl->twisted=(-1);
			return lw_dist2d_line_poly((LWLINE *)lwg2,(LWPOLY *)lwg1,dl);
		}
		else
		{
			lwerror("Unsupported geometry type: %s", lwtype_name(t2));
			return LW_FALSE;
		}
	}
	else
	{
		lwerror("Unsupported geometry type: %s", lwtype_name(t1));
		return LW_FALSE;
	}
	/*You shouldn't being able to get here*/
	lwerror("unspecified error in function lw_dist2d_distribute_bruteforce");
	return LW_FALSE;
}

/**

We have to check for overlapping bboxes
*/
int
lw_dist2d_check_overlap(LWGEOM *lwg1,LWGEOM *lwg2)
{
	LWDEBUG(2, "lw_dist2d_check_overlap is called");
	if ( ! lwg1->bbox )
		lwgeom_calculate_gbox(lwg1, lwg1->bbox);
	if ( ! lwg2->bbox )
		lwgeom_calculate_gbox(lwg2, lwg2->bbox);

	/*Check if the geometries intersect.
	*/
	if ((lwg1->bbox->xmax<lwg2->bbox->xmin||lwg1->bbox->xmin>lwg2->bbox->xmax||lwg1->bbox->ymax<lwg2->bbox->ymin||lwg1->bbox->ymin>lwg2->bbox->ymax))
	{
		LWDEBUG(3, "geometries bboxes did not overlap");
		return LW_FALSE;
	}
	LWDEBUG(3, "geometries bboxes overlap");
	return LW_TRUE;
}

/**

Here the geometries are distributed for the new faster distance-calculations
*/
int
lw_dist2d_distribute_fast(LWGEOM *lwg1, LWGEOM *lwg2, DISTPTS *dl)
{
	POINTARRAY *pa1, *pa2;
	int	type1 = lwg1->type;
	int	type2 = lwg2->type;

	LWDEBUGF(2, "lw_dist2d_distribute_fast is called with typ1=%d, type2=%d", lwg1->type, lwg2->type);

	switch (type1)
	{
	case LINETYPE:
		pa1 = ((LWLINE *)lwg1)->points;
		break;
	case POLYGONTYPE:
		pa1 = ((LWPOLY *)lwg1)->rings[0];
		break;
	default:
		lwerror("Unsupported geometry1 type: %s", lwtype_name(type1));
		return LW_FALSE;
	}
	switch (type2)
	{
	case LINETYPE:
		pa2 = ((LWLINE *)lwg2)->points;
		break;
	case POLYGONTYPE:
		pa2 = ((LWPOLY *)lwg2)->rings[0];
		break;
	default:
		lwerror("Unsupported geometry2 type: %s", lwtype_name(type1));
		return LW_FALSE;
	}
	dl->twisted=1;
	return lw_dist2d_fast_ptarray_ptarray(pa1, pa2, dl, lwg1->bbox, lwg2->bbox);
}

/*------------------------------------------------------------------------------------------------------------
End of Preprocessing functions
--------------------------------------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------------------------------------
Brute force functions
The old way of calculating distances, now used for:
1)	distances to points (because there shouldn't be anything to gain by the new way of doing it)
2)	distances when subgeometries geometries bboxes overlaps
--------------------------------------------------------------------------------------------------------------*/

/**

point to point calculation
*/
int
lw_dist2d_point_point(LWPOINT *point1, LWPOINT *point2, DISTPTS *dl)
{
	POINT2D p1;
	POINT2D p2;

	getPoint2d_p(point1->point, 0, &p1);
	getPoint2d_p(point2->point, 0, &p2);

	return lw_dist2d_pt_pt(&p1, &p2,dl);
}
/**

point to line calculation
*/
int
lw_dist2d_point_line(LWPOINT *point, LWLINE *line, DISTPTS *dl)
{
	POINT2D p;
	POINTARRAY *pa = line->points;
	LWDEBUG(2, "lw_dist2d_point_line is called");
	getPoint2d_p(point->point, 0, &p);
	return lw_dist2d_pt_ptarray(&p, pa, dl);
}
/**
 * 1. see if pt in outer boundary. if no, then treat the outer ring like a line
 * 2. if in the boundary, test to see if its in a hole.
 *    if so, then return dist to hole, else return 0 (point in polygon)
 */
int
lw_dist2d_point_poly(LWPOINT *point, LWPOLY *poly, DISTPTS *dl)
{
	POINT2D p;
	int i;

	LWDEBUG(2, "lw_dist2d_point_poly called");

	getPoint2d_p(point->point, 0, &p);

	if (dl->mode == DIST_MAX)
	{
		LWDEBUG(3, "looking for maxdistance");
		return lw_dist2d_pt_ptarray(&p, poly->rings[0], dl);
	}
	/* Return distance to outer ring if not inside it */
	if ( ! pt_in_ring_2d(&p, poly->rings[0]) )
	{
		LWDEBUG(3, "first point not inside outer-ring");
		return lw_dist2d_pt_ptarray(&p, poly->rings[0], dl);
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
			LWDEBUG(3, " inside an hole");
			return lw_dist2d_pt_ptarray(&p, poly->rings[i], dl);
		}
	}

	LWDEBUG(3, " inside the polygon");
	if (dl->mode == DIST_MIN)
	{
		dl->distance=0.0;
		dl->p1.x=p.x;
		dl->p1.y=p.y;
		dl->p2.x=p.x;
		dl->p2.y=p.y;
	}
	return LW_TRUE; /* Is inside the polygon */
}
/**

line to line calculation
*/
int
lw_dist2d_line_line(LWLINE *line1, LWLINE *line2, DISTPTS *dl)
{
	POINTARRAY *pa1 = line1->points;
	POINTARRAY *pa2 = line2->points;
	LWDEBUG(2, "lw_dist2d_line_line is called");
	return lw_dist2d_ptarray_ptarray(pa1, pa2, dl);
}
/**

line to polygon calculation
*/
int
lw_dist2d_line_poly(LWLINE *line, LWPOLY *poly, DISTPTS *dl)
{
	LWDEBUG(2, "lw_dist2d_line_poly is called");
	return lw_dist2d_ptarray_poly(line->points, poly, dl);
}

/**

Function handling polygon to polygon calculation
1	if we are looking for maxdistance, just check the outer rings.
2	check if poly1 has first point outside poly2 and vice versa, if so, just check outer rings
3	check if first point of poly2 is in a hole of poly1. If so check outer ring of poly2 against that hole of poly1
4	check if first point of poly1 is in a hole of poly2. If so check outer ring of poly1 against that hole of poly2
5	If we have come all the way here we know that the first point of one of them is inside the other ones outer ring and not in holes so we check wich one is inside.
 */
int
lw_dist2d_poly_poly(LWPOLY *poly1, LWPOLY *poly2, DISTPTS *dl)
{

	POINT2D pt;
	int i;

	LWDEBUG(2, "lw_dist2d_poly_poly called");

	/*1	if we are looking for maxdistance, just check the outer rings.*/
	if (dl->mode == DIST_MAX)
	{
		return lw_dist2d_ptarray_ptarray(poly1->rings[0],	poly2->rings[0],dl);
	}


	/* 2	check if poly1 has first point outside poly2 and vice versa, if so, just check outer rings
	here it would be possible to handle the information about wich one is inside wich one and only search for the smaller ones in the bigger ones holes.*/
	getPoint2d_p(poly1->rings[0], 0, &pt);
	if ( !pt_in_ring_2d(&pt, poly2->rings[0]))
	{
		getPoint2d_p(poly2->rings[0], 0, &pt);
		if (!pt_in_ring_2d(&pt, poly1->rings[0]))
		{
			return lw_dist2d_ptarray_ptarray(poly1->rings[0],	poly2->rings[0],dl);
		}
	}

	/*3	check if first point of poly2 is in a hole of poly1. If so check outer ring of poly2 against that hole of poly1*/
	getPoint2d_p(poly2->rings[0], 0, &pt);
	for (i=1; i<poly1->nrings; i++)
	{
		/* Inside a hole */
		if ( pt_in_ring_2d(&pt, poly1->rings[i]) )
		{
			return lw_dist2d_ptarray_ptarray(poly1->rings[i],	poly2->rings[0],dl);
		}
	}

	/*4	check if first point of poly1 is in a hole of poly2. If so check outer ring of poly1 against that hole of poly2*/
	getPoint2d_p(poly1->rings[0], 0, &pt);
	for (i=1; i<poly2->nrings; i++)
	{
		/* Inside a hole */
		if ( pt_in_ring_2d(&pt, poly2->rings[i]) )
		{
			return lw_dist2d_ptarray_ptarray(poly1->rings[0],	poly2->rings[i],dl);
		}
	}


	/*5	If we have come all the way here we know that the first point of one of them is inside the other ones outer ring and not in holes so we check wich one is inside.*/
	getPoint2d_p(poly1->rings[0], 0, &pt);
	if ( pt_in_ring_2d(&pt, poly2->rings[0]))
	{
		dl->distance=0.0;
		dl->p1.x=pt.x;
		dl->p1.y=pt.y;
		dl->p2.x=pt.x;
		dl->p2.y=pt.y;
		return LW_TRUE;
	}

	getPoint2d_p(poly2->rings[0], 0, &pt);
	if (pt_in_ring_2d(&pt, poly1->rings[0]))
	{
		dl->distance=0.0;
		dl->p1.x=pt.x;
		dl->p1.y=pt.y;
		dl->p2.x=pt.x;
		dl->p2.y=pt.y;
		return LW_TRUE;
	}


	lwerror("Unspecified error in function lw_dist2d_poly_poly");
	return LW_FALSE;
}

/**

 * search all the segments of pointarray to see which one is closest to p1
 * Returns minimum distance between point and pointarray
 */
int
lw_dist2d_pt_ptarray(POINT2D *p, POINTARRAY *pa,DISTPTS *dl)
{
	int t;
	POINT2D	start, end;
	int twist = dl->twisted;

	LWDEBUG(2, "lw_dist2d_pt_ptarray is called");

	getPoint2d_p(pa, 0, &start);

	if ( !lw_dist2d_pt_pt(p, &start, dl) ) return LW_FALSE;

	for (t=1; t<pa->npoints; t++)
	{
		dl->twisted=twist;
		getPoint2d_p(pa, t, &end);
		if (!lw_dist2d_pt_seg(p, &start, &end,dl)) return LW_FALSE;

		if (dl->distance<=dl->tolerance && dl->mode == DIST_MIN) return LW_TRUE; /*just a check if  the answer is already given*/
		start = end;
	}

	return LW_TRUE;
}

/**
 * Brute force.
 * Test line-ring distance against each ring.
 * If there's an intersection (distance==0) then return 0 (crosses boundary).
 * Otherwise, test to see if any point is inside outer rings of polygon,
 * but not in inner rings.
 * If so, return 0  (line inside polygon),
 * otherwise return min distance to a ring (could be outside
 * polygon or inside a hole)
 */

int
lw_dist2d_ptarray_poly(POINTARRAY *pa, LWPOLY *poly, DISTPTS *dl)
{
	POINT2D pt;
	int i;

	LWDEBUGF(2, "lw_dist2d_ptarray_poly called (%d rings)", poly->nrings);

	getPoint2d_p(pa, 0, &pt);
	if ( !pt_in_ring_2d(&pt, poly->rings[0]))
	{
		return lw_dist2d_ptarray_ptarray(pa,poly->rings[0],dl);
	}

	for (i=1; i<poly->nrings; i++)
	{
		if (!lw_dist2d_ptarray_ptarray(pa, poly->rings[i], dl)) return LW_FALSE;

		LWDEBUGF(3, " distance from ring %d: %f, mindist: %f",
		         i, dl->distance, dl->tolerance);
		if (dl->distance<=dl->tolerance && dl->mode == DIST_MIN) return LW_TRUE; /*just a check if  the answer is already given*/
	}

	/*
	 * No intersection, have to check if a point is
	 * inside polygon
	 */
	getPoint2d_p(pa, 0, &pt);

	/*
	 * Outside outer ring, so min distance to a ring
	 * is the actual min distance

	if ( ! pt_in_ring_2d(&pt, poly->rings[0]) )
	{
		return ;
	} */

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
			return LW_TRUE;
		}
	}
	if (dl->mode == DIST_MIN)
	{
		dl->distance=0.0;
		dl->p1.x=pt.x;
		dl->p1.y=pt.y;
		dl->p2.x=pt.x;
		dl->p2.y=pt.y;
	}
	return LW_TRUE; /* Not in hole, so inside polygon */
}



/**

test each segment of l1 against each segment of l2.
*/
int
lw_dist2d_ptarray_ptarray(POINTARRAY *l1, POINTARRAY *l2,DISTPTS *dl)
{
	int t,u;
	POINT2D	start, end;
	POINT2D	start2, end2;
	int twist = dl->twisted;

	LWDEBUGF(2, "lw_dist2d_ptarray_ptarray called (points: %d-%d)",l1->npoints, l2->npoints);

	if (dl->mode == DIST_MAX)/*If we are searching for maxdistance we go straight to point-point calculation since the maxdistance have to be between two vertexes*/
	{
		for (t=0; t<l1->npoints; t++) /*for each segment in L1 */
		{
			getPoint2d_p(l1, t, &start);
			for (u=0; u<l2->npoints; u++) /*for each segment in L2 */
			{
				getPoint2d_p(l2, u, &start2);
				lw_dist2d_pt_pt(&start,&start2,dl);
				LWDEBUGF(4, "maxdist_ptarray_ptarray; seg %i * seg %i, dist = %g\n",t,u,dl->distance);
				LWDEBUGF(3, " seg%d-seg%d dist: %f, mindist: %f",
				         t, u, dl->distance, dl->tolerance);
			}
		}
	}
	else
	{
		getPoint2d_p(l1, 0, &start);
		for (t=1; t<l1->npoints; t++) /*for each segment in L1 */
		{
			getPoint2d_p(l1, t, &end);
			getPoint2d_p(l2, 0, &start2);
			for (u=1; u<l2->npoints; u++) /*for each segment in L2 */
			{
				getPoint2d_p(l2, u, &end2);
				dl->twisted=twist;
				lw_dist2d_seg_seg(&start, &end, &start2, &end2,dl);
				LWDEBUGF(4, "mindist_ptarray_ptarray; seg %i * seg %i, dist = %g\n",t,u,dl->distance);
				LWDEBUGF(3, " seg%d-seg%d dist: %f, mindist: %f",
				         t, u, dl->distance, dl->tolerance);
				if (dl->distance<=dl->tolerance && dl->mode == DIST_MIN) return LW_TRUE; /*just a check if  the answer is already given*/
				start2 = end2;
			}
			start = end;
		}
	}
	return LW_TRUE;
}

/**

Finds the shortest distance between two segments.
This function is changed so it is not doing any comparasion of distance
but just sending every possible combination further to lw_dist2d_pt_seg
*/
int
lw_dist2d_seg_seg(POINT2D *A, POINT2D *B, POINT2D *C, POINT2D *D, DISTPTS *dl)
{
	double	s_top, s_bot,s;
	double	r_top, r_bot,r;

	LWDEBUGF(2, "lw_dist2d_seg_seg [%g,%g]->[%g,%g] by [%g,%g]->[%g,%g]",
	         A->x,A->y,B->x,B->y, C->x,C->y, D->x, D->y);

	/*A and B are the same point */
	if (  ( A->x == B->x) && (A->y == B->y) )
	{
		return lw_dist2d_pt_seg(A,C,D,dl);
	}
	/*U and V are the same point */

	if (  ( C->x == D->x) && (C->y == D->y) )
	{
		dl->twisted= ((dl->twisted) * (-1));
		return lw_dist2d_pt_seg(D,A,B,dl);
	}
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
		if ((lw_dist2d_pt_seg(A,C,D,dl)) && (lw_dist2d_pt_seg(B,C,D,dl)))
		{
			dl->twisted= ((dl->twisted) * (-1));  /*here we change the order of inputted geometrys and that we  notice by changing sign on dl->twisted*/
			return ((lw_dist2d_pt_seg(C,A,B,dl)) && (lw_dist2d_pt_seg(D,A,B,dl))); /*if all is successful we return true*/
		}
		else
		{
			return LW_FALSE; /* if any of the calls to lw_dist2d_pt_seg goes wrong we return false*/
		}
	}

	s = s_top/s_bot;
	r=  r_top/r_bot;

	if (((r<0) || (r>1) || (s<0) || (s>1)) || (dl->mode == DIST_MAX))
	{
		if ((lw_dist2d_pt_seg(A,C,D,dl)) && (lw_dist2d_pt_seg(B,C,D,dl)))
		{
			dl->twisted= ((dl->twisted) * (-1));  /*here we change the order of inputted geometrys and that we  notice by changing sign on dl->twisted*/
			return ((lw_dist2d_pt_seg(C,A,B,dl)) && (lw_dist2d_pt_seg(D,A,B,dl))); /*if all is successful we return true*/
		}
		else
		{
			return LW_FALSE; /* if any of the calls to lw_dist2d_pt_seg goes wrong we return false*/
		}
	}
	else
	{
		if (dl->mode == DIST_MIN)	/*If there is intersection we identify the intersection point and return it but only if we are looking for mindistance*/
		{
			POINT2D theP;

			if (((A->x==C->x)&&(A->y==C->y))||((A->x==D->x)&&(A->y==D->y)))
			{
				theP.x = A->x;
				theP.y = A->y;
			}
			else if (((B->x==C->x)&&(B->y==C->y))||((B->x==D->x)&&(B->y==D->y)))
			{
				theP.x = B->x;
				theP.y = B->y;
			}
			else
			{
				theP.x = A->x+r*(B->x-A->x);
				theP.y = A->y+r*(B->y-A->y);
			}
			dl->distance=0.0;
			dl->p1=theP;
			dl->p2=theP;
		}
		return LW_TRUE;

	}
	lwerror("unspecified error in function lw_dist2d_seg_seg");
	return LW_FALSE; /*If we have come here something is wrong*/
}


/*------------------------------------------------------------------------------------------------------------
End of Brute force functions
--------------------------------------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------------------------------------
New faster distance calculations
--------------------------------------------------------------------------------------------------------------*/

/**

The new faster calculation comparing pointarray to another pointarray
the arrays can come from both polygons and linestrings.
The naming is not good but comes from that it compares a
chosen selection of the points not all of them
*/
int
lw_dist2d_fast_ptarray_ptarray(POINTARRAY *l1, POINTARRAY *l2,DISTPTS *dl, GBOX *box1, GBOX *box2)
{
	/*here we define two lists to hold our calculated "z"-values and the order number in the geometry*/

	double k, thevalue;
	float	deltaX, deltaY, c1m, c2m;
	POINT2D	theP,c1, c2;
	float min1X, max1X, max1Y, min1Y,min2X, max2X, max2Y, min2Y;
	int t;
	int n1 = l1->npoints;
	int n2 = l2->npoints;
	
	LISTSTRUCT *list1, *list2;
	list1 = (LISTSTRUCT*)lwalloc(sizeof(LISTSTRUCT)*n1); 
	list2 = (LISTSTRUCT*)lwalloc(sizeof(LISTSTRUCT)*n2);
	
	LWDEBUG(2, "lw_dist2d_fast_ptarray_ptarray is called");

	max1X = box1->xmax;
	min1X = box1->xmin;
	max1Y = box1->ymax;
	min1Y = box1->ymin;
	max2X = box2->xmax;
	min2X = box2->xmin;
	max2Y = box2->ymax;
	min2Y = box2->ymin;
	/*we want the center of the bboxes, and calculate the slope between the centerpoints*/
	c1.x = min1X + (max1X-min1X)/2;
	c1.y = min1Y + (max1Y-min1Y)/2;
	c2.x = min2X + (max2X-min2X)/2;
	c2.y = min2Y + (max2Y-min2Y)/2;

	deltaX=(c2.x-c1.x);
	deltaY=(c2.y-c1.y);


	/*Here we calculate where the line perpendicular to the center-center line crosses the axes for each vertex
	if the center-center line is vertical the perpendicular line will be horizontal and we find it's crossing the Y-axes with z = y-kx */
	if ((deltaX*deltaX)<(deltaY*deltaY))        /*North or South*/
	{
		k = -deltaX/deltaY;
		for (t=0; t<n1; t++) /*for each segment in L1 */
		{
			getPoint2d_p(l1, t, &theP);
			thevalue = theP.y-(k*theP.x);
			list1[t].themeasure=thevalue;
			list1[t].pnr=t;

		}
		for (t=0; t<n2; t++) /*for each segment in L2*/
		{
			getPoint2d_p(l2, t, &theP);
			thevalue = theP.y-(k*theP.x);
			list2[t].themeasure=thevalue;
			list2[t].pnr=t;

		}
		c1m = c1.y-(k*c1.x);
		c2m = c2.y-(k*c2.x);
	}


	/*if the center-center line is horizontal the perpendicular line will be vertical. To eliminate problems with deviding by zero we are here mirroring the coordinate-system
	 and we find it's crossing the X-axes with z = x-(1/k)y */
	else        /*West or East*/
	{
		k = -deltaY/deltaX;
		for (t=0; t<n1; t++) /*for each segment in L1 */
		{
			getPoint2d_p(l1, t, &theP);
			thevalue = theP.x-(k*theP.y);
			list1[t].themeasure=thevalue;
			list1[t].pnr=t;
			/* lwnotice("l1 %d, measure=%f",t,thevalue ); */
		}
		for (t=0; t<n2; t++) /*for each segment in L2*/
		{
			getPoint2d_p(l2, t, &theP);

			thevalue = theP.x-(k*theP.y);
			list2[t].themeasure=thevalue;
			list2[t].pnr=t;
			/* lwnotice("l2 %d, measure=%f",t,thevalue ); */
		}
		c1m = c1.x-(k*c1.y);
		c2m = c2.x-(k*c2.y);
	}

	/*we sort our lists by the calculated values*/
	qsort(list1, n1, sizeof(LISTSTRUCT), struct_cmp_by_measure);
	qsort(list2, n2, sizeof(LISTSTRUCT), struct_cmp_by_measure);

	if (c1m < c2m)
	{
		if (!lw_dist2d_pre_seg_seg(l1,l2,list1,list2,k,dl)) 
		{
			lwfree(list1);
			lwfree(list2);
			return LW_FALSE;
		}
	}
	else
	{
		dl->twisted= ((dl->twisted) * (-1));
		if (!lw_dist2d_pre_seg_seg(l2,l1,list2,list1,k,dl)) 
		{
			lwfree(list1);
			lwfree(list2);
			return LW_FALSE;
		}
	}
	lwfree(list1);
	lwfree(list2);	
	return LW_TRUE;
}

int
struct_cmp_by_measure(const void *a, const void *b)
{
	LISTSTRUCT *ia = (LISTSTRUCT*)a;
	LISTSTRUCT *ib = (LISTSTRUCT*)b;
	return ( ia->themeasure>ib->themeasure ) ? 1 : -1;
}

/**
	preparation before lw_dist2d_seg_seg.
*/
int
lw_dist2d_pre_seg_seg(POINTARRAY *l1, POINTARRAY *l2,LISTSTRUCT *list1, LISTSTRUCT *list2,double k, DISTPTS *dl)
{
	POINT2D p1, p2, p3, p4, p01, p02;
	int pnr1,pnr2,pnr3,pnr4, n1, n2, i, u, r, twist;
	double maxmeasure;
	n1=	l1->npoints;
	n2 = l2->npoints;

	LWDEBUG(2, "lw_dist2d_pre_seg_seg is called");

	getPoint2d_p(l1, list1[0].pnr, &p1);
	getPoint2d_p(l2, list2[0].pnr, &p3);
	lw_dist2d_pt_pt(&p1, &p3,dl);
	maxmeasure = sqrt(dl->distance*dl->distance + (dl->distance*dl->distance*k*k));
	twist = dl->twisted; /*to keep the incomming order between iterations*/
	for (i =(n1-1); i>=0; --i)
	{
		/*we break this iteration when we have checked every
		point closer to our perpendicular "checkline" than
		our shortest found distance*/
		if (((list2[0].themeasure-list1[i].themeasure)) > maxmeasure) break;
		for (r=-1; r<=1; r +=2) /*because we are not iterating in the original pointorder we have to check the segment before and after every point*/
		{
			pnr1 = list1[i].pnr;
			getPoint2d_p(l1, pnr1, &p1);
			if (pnr1+r<0)
			{
				getPoint2d_p(l1, (n1-1), &p01);
				if (( p1.x == p01.x) && (p1.y == p01.y)) pnr2 = (n1-1);
				else pnr2 = pnr1; /* if it is a line and the last and first point is not the same we avoid the edge between start and end this way*/
			}

			else if (pnr1+r>(n1-1))
			{
				getPoint2d_p(l1, 0, &p01);
				if (( p1.x == p01.x) && (p1.y == p01.y)) pnr2 = 0;
				else pnr2 = pnr1; /* if it is a line and the last and first point is not the same we avoid the edge between start and end this way*/
			}
			else pnr2 = pnr1+r;


			getPoint2d_p(l1, pnr2, &p2);
			for (u=0; u<n2; ++u)
			{
				if (((list2[u].themeasure-list1[i].themeasure)) >= maxmeasure) break;
				pnr3 = list2[u].pnr;
				getPoint2d_p(l2, pnr3, &p3);
				if (pnr3==0)
				{
					getPoint2d_p(l2, (n2-1), &p02);
					if (( p3.x == p02.x) && (p3.y == p02.y)) pnr4 = (n2-1);
					else pnr4 = pnr3; /* if it is a line and the last and first point is not the same we avoid the edge between start and end this way*/
				}
				else pnr4 = pnr3-1;

				getPoint2d_p(l2, pnr4, &p4);
				dl->twisted=twist;
				if (!lw_dist2d_selected_seg_seg(&p1, &p2, &p3, &p4, dl)) return LW_FALSE;

				if (pnr3>=(n2-1))
				{
					getPoint2d_p(l2, 0, &p02);
					if (( p3.x == p02.x) && (p3.y == p02.y)) pnr4 = 0;
					else pnr4 = pnr3; /* if it is a line and the last and first point is not the same we avoid the edge between start and end this way*/
				}

				else pnr4 = pnr3+1;

				getPoint2d_p(l2, pnr4, &p4);
				dl->twisted=twist; /*we reset the "twist" for each iteration*/
				if (!lw_dist2d_selected_seg_seg(&p1, &p2, &p3, &p4, dl)) return LW_FALSE;

				maxmeasure = sqrt(dl->distance*dl->distance + (dl->distance*dl->distance*k*k));/*here we "translate" the found mindistance so it can be compared to our "z"-values*/
			}
		}
	}

	return LW_TRUE;
}


/**
	This is the same function as lw_dist2d_seg_seg but
	without any calculations to determine intersection since we
	already know they do not intersect
*/
int
lw_dist2d_selected_seg_seg(POINT2D *A, POINT2D *B, POINT2D *C, POINT2D *D, DISTPTS *dl)
{
	LWDEBUGF(2, "lw_dist2d_selected_seg_seg [%g,%g]->[%g,%g] by [%g,%g]->[%g,%g]",
	         A->x,A->y,B->x,B->y, C->x,C->y, D->x, D->y);

	/*A and B are the same point */
	if (  ( A->x == B->x) && (A->y == B->y) )
	{
		return lw_dist2d_pt_seg(A,C,D,dl);
	}
	/*U and V are the same point */

	if (  ( C->x == D->x) && (C->y == D->y) )
	{
		dl->twisted= ((dl->twisted) * (-1));
		return lw_dist2d_pt_seg(D,A,B,dl);
	}

	if ((lw_dist2d_pt_seg(A,C,D,dl)) && (lw_dist2d_pt_seg(B,C,D,dl)))
	{
		dl->twisted= ((dl->twisted) * (-1));  /*here we change the order of inputted geometrys and that we  notice by changing sign on dl->twisted*/
		return ((lw_dist2d_pt_seg(C,A,B,dl)) && (lw_dist2d_pt_seg(D,A,B,dl))); /*if all is successful we return true*/
	}
	else
	{
		return LW_FALSE; /* if any of the calls to lw_dist2d_pt_seg goes wrong we return false*/
	}
}

/*------------------------------------------------------------------------------------------------------------
End of New faster distance calculations
--------------------------------------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------------------------------------
Functions in common for Brute force and new calculation
--------------------------------------------------------------------------------------------------------------*/

/**
 * pt_in_ring_2d(): crossing number test for a point in a polygon
 *      input:   p = a point,
 *               pa = vertex points of a ring V[n+1] with V[n]=V[0]
 *      returns: 0 = outside, 1 = inside
 *
 *	Our polygons have first and last point the same,
 *
 */
int
pt_in_ring_2d(const POINT2D *p, const POINTARRAY *ring)
{
	int cn = 0;    /* the crossing number counter */
	int i;
	POINT2D v1, v2;

	POINT2D first, last;

	getPoint2d_p(ring, 0, &first);
	getPoint2d_p(ring, ring->npoints-1, &last);
	if ( memcmp(&first, &last, sizeof(POINT2D)) )
	{
		lwerror("pt_in_ring_2d: V[n] != V[0] (%g %g != %g %g)",
		        first.x, first.y, last.x, last.y);
		return LW_FALSE;

	}

	LWDEBUGF(2, "pt_in_ring_2d called with point: %g %g", p->x, p->y);
	/* printPA(ring); */

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

	LWDEBUGF(3, "pt_in_ring_2d returning %d", cn&1);

	return (cn&1);    /* 0 if even (out), and 1 if odd (in) */
}


/**

lw_dist2d_comp from p to line A->B
This one is now sending every occation to lw_dist2d_pt_pt
Before it was handling occations where r was between 0 and 1 internally
and just returning the distance without identifying the points.
To get this points it was nessecary to change and it also showed to be about 10%faster.
*/
int
lw_dist2d_pt_seg(POINT2D *p, POINT2D *A, POINT2D *B, DISTPTS *dl)
{
	POINT2D c;
	double	r;
	/*if start==end, then use pt distance */
	if (  ( A->x == B->x) && (A->y == B->y) )
	{
		return lw_dist2d_pt_pt(p,A,dl);
	}
	/*
	 * otherwise, we use comp.graphics.algorithms
	 * Frequently Asked Questions method
	 *
	 *  (1)        AC dot AB
	 *         r = ---------
	 *              ||AB||^2
	 *	r has the following meaning:
	 *	r=0 P = A
	 *	r=1 P = B
	 *	r<0 P is on the backward extension of AB
	 *	r>1 P is on the forward extension of AB
	 *	0<r<1 P is interior to AB
	 */

	r = ( (p->x-A->x) * (B->x-A->x) + (p->y-A->y) * (B->y-A->y) )/( (B->x-A->x)*(B->x-A->x) +(B->y-A->y)*(B->y-A->y) );

	/*This is for finding the maxdistance.
	the maxdistance have to be between two vertexes,
	compared to mindistance which can be between
	tvo vertexes vertex.*/
	if (dl->mode == DIST_MAX)
	{
		if (r>=0.5)
		{
			return lw_dist2d_pt_pt(p,A,dl);
		}
		if (r<0.5)
		{
			return lw_dist2d_pt_pt(p,B,dl);
		}
	}

	if (r<0)	/*If p projected on the line is outside point A*/
	{
		return lw_dist2d_pt_pt(p,A,dl);
	}
	if (r>=1)	/*If p projected on the line is outside point B or on point B*/
	{
		return lw_dist2d_pt_pt(p,B,dl);
	}
	
	/*If the point p is on the segment this is a more robust way to find out that*/
	if (( ((A->y-p->y)*(B->x-A->x)==(A->x-p->x)*(B->y-A->y) ) ) && (dl->mode ==  DIST_MIN))
	{
		dl->distance = 0.0;
		dl->p1 = *p;
		dl->p2 = *p;
	}
	
	/*If the projection of point p on the segment is between A and B
	then we find that "point on segment" and send it to lw_dist2d_pt_pt*/
	c.x=A->x + r * (B->x-A->x);
	c.y=A->y + r * (B->y-A->y);

	return lw_dist2d_pt_pt(p,&c,dl);
}


/**

Compares incomming points and
stores the points closest to each other
or most far away from each other
depending on dl->mode (max or min)
*/
int
lw_dist2d_pt_pt(POINT2D *thep1, POINT2D *thep2,DISTPTS *dl)
{
	double hside = thep2->x - thep1->x;
	double vside = thep2->y - thep1->y;
	double dist = sqrt ( hside*hside + vside*vside );

	if (((dl->distance - dist)*(dl->mode))>0) /*multiplication with mode to handle mindistance (mode=1)  and maxdistance (mode = (-1)*/
	{
		dl->distance = dist;

		if (dl->twisted>0)	/*To get the points in right order. twisted is updated between 1 and (-1) every time the order is changed earlier in the chain*/
		{
			dl->p1 = *thep1;
			dl->p2 = *thep2;
		}
		else
		{
			dl->p1 = *thep2;
			dl->p2 = *thep1;
		}
	}
	return LW_TRUE;
}




/*------------------------------------------------------------------------------------------------------------
End of Functions in common for Brute force and new calculation
--------------------------------------------------------------------------------------------------------------*/


/*Mixed functions*/


/**

 true if point is in poly (and not in its holes)
 It's not used by postgis but since I don't know what else
 can be affectes in the world I don't dare removing it.
 */
int
pt_in_poly_2d(const POINT2D *p, const LWPOLY *poly)
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


/**
The old function nessecary for ptarray_segmentize2d in ptarray.c
*/
double
distance2d_pt_pt(const POINT2D *p1, const POINT2D *p2)
{
	double hside = p2->x - p1->x;
	double vside = p2->y - p1->y;

	return sqrt ( hside*hside + vside*vside );

	/* the above is more readable
	   return sqrt(
	  	(p2->x-p1->x) * (p2->x-p1->x) + (p2->y-p1->y) * (p2->y-p1->y)
		);  */
}



/**

The old function nessecary for ptarray_segmentize2d in ptarray.c
*/
double
distance2d_pt_seg(const POINT2D *p, const POINT2D *A, const POINT2D *B)
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

	return FP_ABS(s) * sqrt(
	           (B->x-A->x)*(B->x-A->x) + (B->y-A->y)*(B->y-A->y)
	       );
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

/**
 * Compute the azimuth of segment AB in radians.
 * Return 0 on exception (same point), 1 otherwise.
 */
int
azimuth_pt_pt(const POINT2D *A, const POINT2D *B, double *d)
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


