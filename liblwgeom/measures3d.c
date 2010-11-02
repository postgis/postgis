
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
	if (!lw_dist3d_recursive((LWCOLLECTION*) lw1,(LWCOLLECTION*) lw2,&thedl))
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


		lwpoints[0] = make_lwpoint3dz(srid, x1, y1, z1);
		lwpoints[1] = make_lwpoint3dz(srid, x2, y2, z2);

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

	if (!lw_dist3d_recursive((LWCOLLECTION*) lw1,(LWCOLLECTION*) lw2,&thedl))
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
		result = (LWGEOM *)make_lwpoint3dz(srid, x, y, z);
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
	if (lw_dist3d_recursive((LWCOLLECTION*) lw1,(LWCOLLECTION*) lw2,&thedl))
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
	if (lw_dist3d_recursive((LWCOLLECTION*) lw1,(LWCOLLECTION*) lw2,&thedl))
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
int lw_dist3d_recursive(const LWCOLLECTION * lwg1,const LWCOLLECTION * lwg2,DISTPTS3D *dl)
{
	int i, j;
	int n1=1;
	int n2=1;
	LWGEOM *g1, *g2;

	LWDEBUGF(2, "lw_dist3d_recursive is called with type1=%d, type2=%d", TYPE_GETTYPE(lwg1->type), TYPE_GETTYPE(lwg2->type));

	if (lwtype_is_collection(TYPE_GETTYPE(lwg1->type)))
	{
		LWDEBUG(3, "First geometry is collection");
		n1=lwg1->ngeoms;
	}
	if (lwtype_is_collection(TYPE_GETTYPE(lwg2->type)))
	{
		LWDEBUG(3, "Second geometry is collection");
		n2=lwg2->ngeoms;
	}

	for ( i = 0; i < n1; i++ )
	{

		if (lwtype_is_collection(TYPE_GETTYPE(lwg1->type)))
		{
			g1=lwg1->geoms[i];
		}
		else
		{
			g1=(LWGEOM*)lwg1;
		}

		if (lwgeom_is_empty(g1)) return LW_TRUE;

		if (lwtype_is_collection(TYPE_GETTYPE(g1->type)))
		{
			LWDEBUG(3, "Found collection inside first geometry collection, recursing");
			if (!lw_dist3d_recursive((LWCOLLECTION*)g1, (LWCOLLECTION*)lwg2, dl)) return LW_FALSE;
			continue;
		}
		for ( j = 0; j < n2; j++ )
		{
			if (lwtype_is_collection(TYPE_GETTYPE(lwg2->type)))
			{
				g2=lwg2->geoms[j];
			}
			else
			{
				g2=(LWGEOM*)lwg2;
			}
			if (lwtype_is_collection(TYPE_GETTYPE(g2->type)))
			{
				LWDEBUG(3, "Found collection inside second geometry collection, recursing");
				if (!lw_dist3d_recursive((LWCOLLECTION*) g1, (LWCOLLECTION*)g2, dl)) return LW_FALSE;
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

	int	t1 = TYPE_GETTYPE(lwg1->type);
	int	t2 = TYPE_GETTYPE(lwg2->type);

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
			lwerror("Linestring to Linestring distance measures is not yet supported for 3d");
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


/*------------------------------------------------------------------------------------------------------------
End of Brute force functions
--------------------------------------------------------------------------------------------------------------*/



/*------------------------------------------------------------------------------------------------------------
Functions in common for Brute force and new calculation
--------------------------------------------------------------------------------------------------------------*/

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




/*------------------------------------------------------------------------------------------------------------
End of Functions in common for Brute force and new calculation
--------------------------------------------------------------------------------------------------------------*/





