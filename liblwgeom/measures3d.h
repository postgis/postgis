
/**********************************************************************
 * $Id: measures.h 4715 2009-11-01 17:58:42Z nicklas $
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2006 Refractions Research Inc.
 * Copyright 2007-2008 Mark Cave-Ayland
 * Copyright 2008 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "liblwgeom_internal.h"


/**

Structure used in distance-calculations
*/
typedef struct
{
	double distance;	/*the distance between p1 and p2*/
	POINT3DZ p1;
	POINT3DZ p2;
	int mode;	/*the direction of looking, if thedir = -1 then we look for 3dmaxdistance and if it is 1 then we look for 3dmindistance*/
	int twisted; /*To preserve the order of incoming points to match the first and second point in 3dshortest and 3dlongest line*/
	double tolerance; /*the tolerance for 3ddwithin and 3ddfullywithin*/
} DISTPTS3D;


/*
Preprocessing functions
*/
int lw_dist3d_distribute_bruteforce(LWGEOM *lwg1, LWGEOM *lwg2, DISTPTS3D *dl);
int lw_dist3d_recursive(const LWGEOM *lwg1,const LWGEOM *lwg2, DISTPTS3D *dl);
int lw_dist3d_distribute_fast(LWGEOM *lwg1, LWGEOM *lwg2, DISTPTS3D *dl);

/*
Brute force functions
*/
int lw_dist3d_pt_ptarray(POINT3DZ *p, POINTARRAY *pa, DISTPTS3D *dl);
int lw_dist3d_point_point(LWPOINT *point1, LWPOINT *point2, DISTPTS3D *dl);
int lw_dist3d_point_line(LWPOINT *point, LWLINE *line, DISTPTS3D *dl);

/*
Functions in common for Brute force and new calculation
*/
int lw_dist3d_pt_pt(POINT3DZ *p1, POINT3DZ *p2, DISTPTS3D *dl);
int lw_dist3d_pt_seg(POINT3DZ *p, POINT3DZ *A, POINT3DZ *B, DISTPTS3D *dl);
