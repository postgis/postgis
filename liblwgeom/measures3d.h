/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * PostGIS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * PostGIS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PostGIS.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 *
 * Copyright 2011 Nicklas Avén
 * Copyright 2019 Darafei Praliaskouski
 *
 **********************************************************************/

#ifndef _MEASURES3D_H
#define _MEASURES3D_H 1
#include <float.h>
#include "measures.h"

#define DOT(u, v) ((u).x * (v).x + (u).y * (v).y + (u).z * (v).z)
#define VECTORLENGTH(v) sqrt(((v).x * (v).x) + ((v).y * (v).y) + ((v).z * (v).z))

/**

Structure used in distance-calculations
*/
typedef struct
{
	double distance; /*the distance between p1 and p2*/
	POINT3DZ p1;
	POINT3DZ p2;
	int mode; /*the direction of looking, if thedir = -1 then we look for 3dmaxdistance and if it is 1 then we look
		     for 3dmindistance*/
	int twisted; /*To preserve the order of incoming points to match the first and second point in 3dshortest and
			3dlongest line*/
	double tolerance; /*the tolerance for 3ddwithin and 3ddfullywithin*/
} DISTPTS3D;

typedef struct
{
	double x, y, z;
} VECTOR3D;

typedef struct
{
	POINT3DZ pop; /*Point On Plane*/
	VECTOR3D pv;  /*Perpendicular normal vector*/
} PLANE3D;

/*
Geometry returning functions
*/
LWGEOM *lw_dist3d_distancepoint(const LWGEOM *lw1, const LWGEOM *lw2, int32_t srid, int mode);
LWGEOM *lw_dist3d_distanceline(const LWGEOM *lw1, const LWGEOM *lw2, int32_t srid, int mode);

/*
Preprocessing functions
*/
int lw_dist3d_distribute_bruteforce(const LWGEOM *lwg1, const LWGEOM *lwg2, DISTPTS3D *dl);
int lw_dist3d_recursive(const LWGEOM *lwg1, const LWGEOM *lwg2, DISTPTS3D *dl);
int lw_dist3d_distribute_fast(const LWGEOM *lwg1, const LWGEOM *lwg2, DISTPTS3D *dl);

/*
Brute force functions
*/
int lw_dist3d_pt_ptarray(POINT3DZ *p, POINTARRAY *pa, DISTPTS3D *dl);
int lw_dist3d_point_point(LWPOINT *point1, LWPOINT *point2, DISTPTS3D *dl);
int lw_dist3d_point_line(LWPOINT *point, LWLINE *line, DISTPTS3D *dl);
int lw_dist3d_point_poly(LWPOINT *point, LWPOLY *poly, DISTPTS3D *dl);
int lw_dist3d_point_tri(LWPOINT *point, LWTRIANGLE *tri, DISTPTS3D *dl);

int lw_dist3d_line_line(LWLINE *line1, LWLINE *line2, DISTPTS3D *dl);
int lw_dist3d_line_poly(LWLINE *line, LWPOLY *poly, DISTPTS3D *dl);
int lw_dist3d_line_tri(LWLINE *line, LWTRIANGLE *tri, DISTPTS3D *dl);

int lw_dist3d_poly_poly(LWPOLY *poly1, LWPOLY *poly2, DISTPTS3D *dl);
int lw_dist3d_poly_tri(LWPOLY *poly, LWTRIANGLE *tri, DISTPTS3D *dl);

int lw_dist3d_tri_tri(LWTRIANGLE *tri1, LWTRIANGLE *tri2, DISTPTS3D *dl);

int lw_dist3d_pt_pt(POINT3DZ *p1, POINT3DZ *p2, DISTPTS3D *dl);
int lw_dist3d_pt_seg(POINT3DZ *p, POINT3DZ *A, POINT3DZ *B, DISTPTS3D *dl);
int lw_dist3d_pt_poly(POINT3DZ *p, LWPOLY *poly, PLANE3D *plane, POINT3DZ *projp, DISTPTS3D *dl);
int lw_dist3d_pt_tri(POINT3DZ *p, LWTRIANGLE *tri, PLANE3D *plane, POINT3DZ *projp, DISTPTS3D *dl);

int lw_dist3d_seg_seg(POINT3DZ *A, POINT3DZ *B, POINT3DZ *C, POINT3DZ *D, DISTPTS3D *dl);

int lw_dist3d_ptarray_ptarray(POINTARRAY *l1, POINTARRAY *l2, DISTPTS3D *dl);
int lw_dist3d_ptarray_poly(POINTARRAY *pa, LWPOLY *poly, PLANE3D *plane, DISTPTS3D *dl);
int lw_dist3d_ptarray_tri(POINTARRAY *pa, LWTRIANGLE *tri, PLANE3D *plane, DISTPTS3D *dl);

double project_point_on_plane(POINT3DZ *p, PLANE3D *pl, POINT3DZ *p0);
int define_plane(POINTARRAY *pa, PLANE3D *pl);
int pt_in_ring_3d(const POINT3DZ *p, const POINTARRAY *ring, PLANE3D *plane);

/*
Helper functions
*/

#endif /* !defined _MEASURES3D_H  */
