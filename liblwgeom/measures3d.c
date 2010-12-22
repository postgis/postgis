
/**********************************************************************
 * $Id: measures.c 5439 2010-03-16 03:13:33Z pramsey $
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
#include <stdlib.h>

#include "measures3d.h"


/*------------------------------------------------------------------------------------------------------------
Initializing functions
The functions starting the distance-calculation processses
--------------------------------------------------------------------------------------------------------------*/

/**
Function initializing 3dshortestline and 3dlongestline calculations.
*/
LWGEOM *
lw_dist3d_distanceline(LWGEOM *lw1, LWGEOM *lw2,int srid,int mode)
{
	double x1,x2,y1,y2, z1, z2;
	double initdistance = ( mode == DIST_MIN ? MAXFLOAT : -1.0);
	DISTPTS3D thedl;
	LWPOINT *lwpoints[2];
	LWGEOM *result;

	thedl.mode = mode;
	thedl.distance = initdistance;
	thedl.tolerance = 0.0;

	LWDEBUG(2, "lw_dist3d_distanceline is called");
	if (!lw_dist3d_recursive(lw1, lw2, &thedl))
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
		z1=thedl.p1.z;
		x2=thedl.p2.x;
		y2=thedl.p2.y;
		z2=thedl.p2.z;


		lwpoints[0] = lwpoint_make3dz(srid, x1, y1, z1);
		lwpoints[1] = lwpoint_make3dz(srid, x2, y2, z2);

		result = (LWGEOM *)lwline_from_lwpointarray(srid, 2, lwpoints);
	}

	return result;
}

/**
Function initializing 3dclosestpoint calculations.
*/
LWGEOM *
lw_dist3d_distancepoint(LWGEOM *lw1, LWGEOM *lw2,int srid,int mode)
{
	double x,y,z;
	DISTPTS3D thedl;
	double initdistance = MAXFLOAT;
	LWGEOM *result;

	thedl.mode = mode;
	thedl.distance= initdistance;
	thedl.tolerance = 0;

	LWDEBUG(2, "lw_dist3d_distancepoint is called");

	if (!lw_dist3d_recursive(lw1, lw2, &thedl))
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
		z=thedl.p1.z;
		result = (LWGEOM *)lwpoint_make3dz(srid, x, y, z);
	}

	return result;
}


/**
Function initialazing 3d max distance calculation
*/
double
lwgeom_maxdistance3d(LWGEOM *lw1, LWGEOM *lw2)
{
	LWDEBUG(2, "lwgeom_maxdistance3d is called");

	return lwgeom_maxdistance3d_tolerance( lw1, lw2, 0.0 );
}

/**
Function handling 3d max distance calculations and dfyllywithin calculations.
The difference is just the tolerance.
*/
double
lwgeom_maxdistance3d_tolerance(LWGEOM *lw1, LWGEOM *lw2, double tolerance)
{
	/*double thedist;*/
	DISTPTS3D thedl;
	LWDEBUG(2, "lwgeom_maxdistance3d_tolerance is called");
	thedl.mode = DIST_MAX;
	thedl.distance= -1;
	thedl.tolerance = tolerance;
	if (lw_dist3d_recursive(lw1, lw2, &thedl))
	{
		return thedl.distance;
	}
	/*should never get here. all cases ought to be error handled earlier*/
	lwerror("Some unspecified error.");
	return -1;
}

/**
	Function initialazing 3d min distance calculation
*/
double
lwgeom_mindistance3d(LWGEOM *lw1, LWGEOM *lw2)
{
	LWDEBUG(2, "lwgeom_mindistance3d is called");
	return lwgeom_mindistance3d_tolerance( lw1, lw2, 0.0 );
}

/**
	Function handling 3d min distance calculations and dwithin calculations.
	The difference is just the tolerance.
*/
double
lwgeom_mindistance3d_tolerance(LWGEOM *lw1, LWGEOM *lw2, double tolerance)
{
	DISTPTS3D thedl;
	LWDEBUG(2, "lwgeom_mindistance3d_tolerance is called");
	thedl.mode = DIST_MIN;
	thedl.distance= MAXFLOAT;
	thedl.tolerance = tolerance;
	if (lw_dist3d_recursive(lw1, lw2, &thedl))
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
This is a recursive function delivering every possible combinatin of subgeometries
*/
int lw_dist3d_recursive(const LWGEOM *lwg1,const LWGEOM *lwg2, DISTPTS3D *dl)
{
	int i, j;
	int n1=1;
	int n2=1;
	LWGEOM *g1 = NULL;
	LWGEOM *g2 = NULL;
	LWCOLLECTION *c1 = NULL;
	LWCOLLECTION *c2 = NULL;

	LWDEBUGF(2, "lw_dist3d_recursive is called with type1=%d, type2=%d", TYPE_GETTYPE(lwg1->type), TYPE_GETTYPE(lwg2->type));

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
			if (!lw_dist3d_recursive(g1, lwg2, dl)) return LW_FALSE;
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
				if (!lw_dist3d_recursive(g1, g2, dl)) return LW_FALSE;
				continue;
			}


			/*If one of geometries is empty, return. True here only means continue searching. False would have stoped the process*/
			if (lwgeom_is_empty(g1)||lwgeom_is_empty(g2)) return LW_TRUE;


			if (!lw_dist3d_distribute_bruteforce(g1, g2, dl)) return LW_FALSE;
			if (dl->distance<=dl->tolerance && dl->mode == DIST_MIN) return LW_TRUE; /*just a check if  the answer is already given*/
		}
	}
	return LW_TRUE;
}



/**

This function distributes the "old-type" brut-force for 3D so far the only type, tasks depending on type
*/
int
lw_dist3d_distribute_bruteforce(LWGEOM *lwg1, LWGEOM *lwg2, DISTPTS3D *dl)
{

	int	t1 = lwg1->type;
	int	t2 = lwg2->type;

	LWDEBUGF(2, "lw_dist3d_distribute_bruteforce is called with typ1=%d, type2=%d", TYPE_GETTYPE(lwg1->type), TYPE_GETTYPE(lwg2->type));

	if  ( t1 == POINTTYPE )
	{
		if  ( t2 == POINTTYPE )
		{
			dl->twisted=1;
			return lw_dist3d_point_point((LWPOINT *)lwg1, (LWPOINT *)lwg2, dl);
		}
		else if  ( t2 == LINETYPE )
		{
			dl->twisted=1;
			return lw_dist3d_point_line((LWPOINT *)lwg1, (LWLINE *)lwg2, dl);
		}
		else if  ( t2 == POLYGONTYPE )
		{
			lwerror("Polygons are not yet supported for 3d distance calculations");
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
			return lw_dist3d_point_line((LWPOINT *)lwg2,(LWLINE *)lwg1,dl);
		}
		else if ( t2 == LINETYPE )
		{
			dl->twisted=1;
			return lw_dist3d_line_line((LWLINE *)lwg1,(LWLINE *)lwg2,dl);
		}
		else if ( t2 == POLYGONTYPE )
		{
			lwerror("Polygons are not yet supported for 3d distance calculations");
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
			lwerror("Polygons are not yet supported for 3d distance calculations");
		}
		else if ( t2 == POINTTYPE )
		{
			lwerror("Polygons are not yet supported for 3d distance calculations");
		}
		else if ( t2 == LINETYPE )
		{
			lwerror("Polygons are not yet supported for 3d distance calculations");
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
	lwerror("unspecified error in function lw_dist3d_distribute_bruteforce");
	return LW_FALSE;
}



/*------------------------------------------------------------------------------------------------------------
End of Preprocessing functions
--------------------------------------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------------------------------------
Brute force functions
So far the only way to do 3D-calculations
--------------------------------------------------------------------------------------------------------------*/

/**

point to point calculation
*/
int
lw_dist3d_point_point(LWPOINT *point1, LWPOINT *point2, DISTPTS3D *dl)
{
	POINT3DZ p1;
	POINT3DZ p2;

	getPoint3dz_p(point1->point, 0, &p1);
	getPoint3dz_p(point2->point, 0, &p2);

	return lw_dist3d_pt_pt(&p1, &p2,dl);
}
/**

point to line calculation
*/
int
lw_dist3d_point_line(LWPOINT *point, LWLINE *line, DISTPTS3D *dl)
{
	POINT3DZ p;
	POINTARRAY *pa = line->points;
	LWDEBUG(2, "lw_dist3d_point_line is called");
	getPoint3dz_p(point->point, 0, &p);
	return lw_dist3d_pt_ptarray(&p, pa, dl);
}

/**

 * search all the segments of pointarray to see which one is closest to p1
 * Returns minimum distance between point and pointarray
 */
int
lw_dist3d_pt_ptarray(POINT3DZ *p, POINTARRAY *pa,DISTPTS3D *dl)
{
	int t;
	POINT3DZ	start, end;
	int twist = dl->twisted;

	LWDEBUG(2, "lw_dist3d_pt_ptarray is called");

	getPoint3dz_p(pa, 0, &start);

	for (t=1; t<pa->npoints; t++)
	{
		dl->twisted=twist;
		getPoint3dz_p(pa, t, &end);
		if (!lw_dist3d_pt_seg(p, &start, &end,dl)) return LW_FALSE;

		if (dl->distance<=dl->tolerance && dl->mode == DIST_MIN) return LW_TRUE; /*just a check if  the answer is already given*/
		start = end;
	}

	return LW_TRUE;
}


/**

This one is sending every occation to lw_dist3d_pt_pt
*/
int
lw_dist3d_pt_seg(POINT3DZ *p, POINT3DZ *A, POINT3DZ *B, DISTPTS3D *dl)
{
	POINT3DZ c;
	double	r;
	/*if start==end, then use pt distance */
	if (  ( A->x == B->x) && (A->y == B->y) && (A->z == B->z)  )
	{
		return lw_dist3d_pt_pt(p,A,dl);
	}


	r = ( (p->x-A->x) * (B->x-A->x) + (p->y-A->y) * (B->y-A->y)  + ( p->z-A->z) * (B->z-A->z) )/( (B->x-A->x)*(B->x-A->x) +(B->y-A->y)*(B->y-A->y)+(B->z-A->z)*(B->z-A->z) );

	/*This is for finding the 3Dmaxdistance.
	the maxdistance have to be between two vertexes,
	compared to mindistance which can be between
	tvo vertexes vertex.*/
	if (dl->mode == DIST_MAX)
	{
		if (r>=0.5)
		{
			return lw_dist3d_pt_pt(p,A,dl);
		}
		if (r<0.5)
		{
			return lw_dist3d_pt_pt(p,B,dl);
		}
	}

	if (r<0)	/*If the first vertex A is closest to the point p*/
	{
		return lw_dist3d_pt_pt(p,A,dl);
	}
	if (r>1)	/*If the second vertex B is closest to the point p*/
	{
		return lw_dist3d_pt_pt(p,B,dl);
	}

	/*else if the point p is closer to some point between a and b
	then we find that point and send it to lw_dist3d_pt_pt*/
	c.x=A->x + r * (B->x-A->x);
	c.y=A->y + r * (B->y-A->y);
	c.z=A->z + r * (B->z-A->z);

	return lw_dist3d_pt_pt(p,&c,dl);
}


/**

Compares incomming points and
stores the points closest to each other
or most far away from each other
depending on dl->mode (max or min)
*/
int
lw_dist3d_pt_pt(POINT3DZ *thep1, POINT3DZ *thep2,DISTPTS3D *dl)
{
	double dx = thep2->x - thep1->x;
	double dy = thep2->y - thep1->y;
	double dz = thep2->z - thep1->z;
	double dist = sqrt ( dx*dx + dy*dy + dz*dz);

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



int
lw_dist3d_line_line(LWLINE *line1, LWLINE *line2, DISTPTS3D *dl)
{
	POINTARRAY *pa1 = line1->points;
	POINTARRAY *pa2 = line2->points;
	LWDEBUG(2, "lw_dist3d_line_line is called");
	return lw_dist3d_ptarray_ptarray(pa1, pa2, dl);
}


/**

Finds all combinationes of segments between two pointarrays
*/
int
lw_dist3d_ptarray_ptarray(POINTARRAY *l1, POINTARRAY *l2,DISTPTS3D *dl)
{
	int t,u;
	POINT3DZ	start, end;
	POINT3DZ	start2, end2;
	int twist = dl->twisted;

	LWDEBUGF(2, "lw_dist3d_ptarray_ptarray called (points: %d-%d)",l1->npoints, l2->npoints);

	if (dl->mode == DIST_MAX)/*If we are searching for maxdistance we go straight to point-point calculation since the maxdistance have to be between two vertexes*/
	{
		for (t=0; t<l1->npoints; t++) /*for each segment in L1 */
		{
			getPoint3dz_p(l1, t, &start);
			for (u=0; u<l2->npoints; u++) /*for each segment in L2 */
			{
				getPoint3dz_p(l2, u, &start2);
				lw_dist3d_pt_pt(&start,&start2,dl);
				LWDEBUGF(4, "maxdist_ptarray_ptarray; seg %i * seg %i, dist = %g\n",t,u,dl->distance);
				LWDEBUGF(3, " seg%d-seg%d dist: %f, mindist: %f",
				         t, u, dl->distance, dl->tolerance);
			}
		}
	}
	else
	{
		getPoint3dz_p(l1, 0, &start);
		for (t=1; t<l1->npoints; t++) /*for each segment in L1 */
		{
			getPoint3dz_p(l1, t, &end);
			getPoint3dz_p(l2, 0, &start2);
			for (u=1; u<l2->npoints; u++) /*for each segment in L2 */
			{
				getPoint3dz_p(l2, u, &end2);
				dl->twisted=twist;
				lw_dist3d_seg_seg(&start, &end, &start2, &end2,dl);
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

Finds the two closest points on two linesegments
*/
int 
lw_dist3d_seg_seg(POINT3DZ *s1p1, POINT3DZ *s1p2, POINT3DZ *s2p1, POINT3DZ *s2p2, DISTPTS3D *dl)
{
	/*s1p1 and s1p2 are the same point */
	if (  ( s1p1->x == s1p2->x) && (s1p1->y == s1p2->y) && (s1p1->z == s1p2->y) )
	{
		return lw_dist3d_pt_seg(s1p1,s2p1,s2p2,dl);
	}
	/*s2p1 and s2p2 are the same point */
	if (  ( s2p1->x == s2p2->x) && (s2p1->y == s2p2->y) && (s2p1->z == s2p2->y) )
	{
		dl->twisted= ((dl->twisted) * (-1));
		return lw_dist3d_pt_seg(s2p1,s1p1,s1p2,dl);
	}
	
	
/*
	Here we use algorithm from softsurfer.com
	that can be found here
	http://softsurfer.com/Archive/algorithm_0106/algorithm_0106.htm
*/
	
	
	VECTOR3D v1, v2, vl;
	double s1k, s2k; /*two variables representing where on Line 1 (s1k) and where on Line 2 (s2k) a connecting line between the two lines is perpendicular to both lines*/
	POINT3DZ p1, p2;
			
	if (!get_3dvector_from_points(s1p1, s1p2, &v1))
		return LW_FALSE;	

	if (!get_3dvector_from_points(s2p1, s2p2, &v2))
		return LW_FALSE;	

	if (!get_3dvector_from_points(s2p1, s1p1, &vl))
		return LW_FALSE;	

	double    a = DOT(&v1,&v1);
	double    b = DOT(&v1,&v2);
	double    c = DOT(&v2,&v2);
	double    d = DOT(&v1,&vl);
	double    e = DOT(&v2,&vl);
	double    D = a*c - b*b; 


	if (D <0.000000001) 
	{        /* the lines are almost parallel*/
		s1k = 0.0; /*If the lines are paralell we try by using the startpoint of first segment. If that gives a projected point on the second line outside segment 2 it wil be found that s2k is >1 or <0.*/
		if(b>c)   /* use the largest denominator*/
		{
			s2k=d/b;
		}
		else
		{
			s2k =e/c;
		}
	}
	else 
	{
		s1k = (b*e - c*d) / D;
		s2k = (a*e - b*d) / D;
	}

	/* Now we check if the projected closest point on the infinite lines is outside our segments. If so the combinations with start and end points will be tested*/
	if(s1k<0.0||s1k>1.0||s2k<0.0||s2k>1.0)
	{
		if(s1k<0.0) 
		{

			if (!lw_dist3d_pt_seg(s1p1, s2p1, s2p2, dl))
			{
				return LW_FALSE;
			}
		}
		if(s1k>1.0)
		{

			if (!lw_dist3d_pt_seg(s1p2, s2p1, s2p2, dl))
			{
				return LW_FALSE;
			}
		}
		if(s2k<0.0)
		{
			dl->twisted= ((dl->twisted) * (-1));
			if (!lw_dist3d_pt_seg(s2p1, s1p1, s1p2, dl))
			{
				return LW_FALSE;
			}
		}
		if(s2k>1.0)
		{
			dl->twisted= ((dl->twisted) * (-1));
			if (!lw_dist3d_pt_seg(s2p2, s1p1, s1p2, dl))
			{
				return LW_FALSE;
			}
		}
	}
	else
	{/*Find the closest point on the edges of both segments*/
		p1.x=s1p1->x+s1k*(s1p2->x-s1p1->x);
		p1.y=s1p1->y+s1k*(s1p2->y-s1p1->y);
		p1.z=s1p1->z+s1k*(s1p2->z-s1p1->z);

		p2.x=s2p1->x+s2k*(s2p2->x-s2p1->x);
		p2.y=s2p1->y+s2k*(s2p2->y-s2p1->y);
		p2.z=s2p1->z+s2k*(s2p2->z-s2p1->z);

		if (!lw_dist3d_pt_pt(&p1,&p2,dl))/* Send the closest points to point-point calculation*/
		{
			return LW_FALSE;
		}
	}
	return LW_TRUE;
}



/*------------------------------------------------------------------------------------------------------------
End of Brute force functions
--------------------------------------------------------------------------------------------------------------*/



