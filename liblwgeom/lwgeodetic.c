/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * Copyright 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "lwgeodetic.h"

/**
* If this is true, use the slow algorithm.
*/
int gbox_geocentric_slow = LW_FALSE;


void edge_deg2rad(GEOGRAPHIC_EDGE *e)
{
	(e->start).lat = deg2rad((e->start).lat);
	(e->end).lat = deg2rad((e->end).lat);
	(e->start).lon = deg2rad((e->start).lon);
	(e->end).lon = deg2rad((e->end).lon);
}

void edge_rad2deg(GEOGRAPHIC_EDGE *e)
{
	(e->start).lat = rad2deg((e->start).lat);
	(e->end).lat = rad2deg((e->end).lat);
	(e->start).lon = rad2deg((e->start).lon);
	(e->end).lon = rad2deg((e->end).lon);
}

void point_deg2rad(GEOGRAPHIC_POINT *p)
{
	p->lat = deg2rad(p->lat);
	p->lon = deg2rad(p->lon);
}
void point_rad2deg(GEOGRAPHIC_POINT *p)
{
	p->lat = rad2deg(p->lat);
	p->lon = rad2deg(p->lon);
}

/**
* Check to see if this geocentric gbox is wrapped around the north pole.
* Only makes sense if this gbox originated from a polygon, as it's assuming
* the box is generated from external edges and there's an "interior" which
* contains the pole.
*/
static int gbox_contains_north_pole(GBOX gbox)
{
    if( gbox.zmin > 0.0 &&
        gbox.xmin < 0.0 && gbox.xmax > 0.0 &&
        gbox.ymin < 0.0 && gbox.ymax > 0.0 )
    {
        return LW_TRUE;
    }
    return LW_FALSE;
}

/**
* Check to see if this geocentric gbox is wrapped around the south pole.
* Only makes sense if this gbox originated from a polygon, as it's assuming
* the box is generated from external edges and there's an "interior" which
* contains the pole.
*/
static int gbox_contains_south_pole(GBOX gbox)
{
    if( gbox.zmax < 0.0 &&
        gbox.xmin < 0.0 && gbox.xmax > 0.0 &&
        gbox.ymin < 0.0 && gbox.ymax > 0.0 )
    {
        return LW_TRUE;
    }
    return LW_FALSE;
}


/**
* Convert spherical coordinates to cartesion coordinates on unit sphere
*/
static void inline geog2cart(GEOGRAPHIC_POINT g, POINT3D *p)
{
	p->x = cos(g.lat) * cos(g.lon);
	p->y = cos(g.lat) * sin(g.lon);
	p->z = sin(g.lat);
}

/**
* Convert cartesion coordinates to spherical coordinates on unit sphere
*/
static void inline cart2geog(POINT3D p, GEOGRAPHIC_POINT *g)
{
	g->lon = atan2(p.y, p.x);
	g->lat = asin(p.z);
}

/** 
* Calculate the dot product of two unit vectors
*/
static double inline dot_product(POINT3D p1, POINT3D p2)
{
	return (p1.x*p2.x) + (p1.y*p2.y) + (p1.z*p2.z);
}

/**
* Calculate the cross product of two vectors
*/
static void inline cross_product(POINT3D a, POINT3D b, POINT3D *n)
{
    n->x = a.y * b.z - a.z * b.y;
	n->y = a.z * b.x - a.x * b.z;
	n->z = a.x * b.y - a.y * b.x;
	return;
}

/**
* Calculate the sum of two vectors
*/
static void inline vector_sum(POINT3D a, POINT3D b, POINT3D *n)
{
	n->x = a.x + b.x;
	n->y = a.y + b.y;
	n->z = a.z + b.z;
	return;
}

/**
* Calculate the difference of two vectors
*/
static void inline vector_difference(POINT3D a, POINT3D b, POINT3D *n)
{
	n->x = a.x - b.x;
	n->y = a.y - b.y;
	n->z = a.z - b.z;
	return;
}


/**
* Normalize to a unit vector.
*/
static void inline normalize(POINT3D *p)
{
	double d = sqrt(p->x*p->x + p->y*p->y + p->z*p->z);
	if(FP_IS_ZERO(d)) 
	{
		p->x = p->y = p->z = 0.0;
		return;
	}
	p->x = p->x / d;
	p->y = p->y / d;
	p->z = p->z / d;
    return;
}

static void inline unit_normal(POINT3D a, POINT3D b, POINT3D *n)
{
	cross_product(a, b, n);
    normalize(n);
	return;
}

/**
* Computes the cross product of two vectors using their lat, lng representations.
* Good even for small distances between p and q.
*/
void robust_cross_product(GEOGRAPHIC_POINT p, GEOGRAPHIC_POINT q, POINT3D *a)
{
	double lon_qpp = (q.lon + p.lon) / -2.0;
	double lon_qmp = (q.lon - p.lon) / 2.0;
	a->x = sin(p.lat-q.lat) * sin(lon_qpp) * cos(lon_qmp) -
	       sin(p.lat+q.lat) * cos(lon_qpp) * sin(lon_qmp);
	a->y = sin(p.lat-q.lat) * cos(lon_qpp) * cos(lon_qmp) +
	       sin(p.lat+q.lat) * sin(lon_qpp) * sin(lon_qmp);
	a->z = cos(p.lat) * cos(q.lat) * sin(q.lon-p.lon);
    normalize(a);
}

void x_to_z(POINT3D *p)
{
    double tmp = p->z;
    p->z = p->x;
    p->x = tmp;
}

void y_to_z(POINT3D *p)
{
    double tmp = p->z;
    p->z = p->y;
    p->y = tmp;
}

/**
* Returns true if the point p is on the great circle plane.
* Forms the scalar triple product of A,B,p and if the volume of the
* resulting parallelepiped is near zero the point p is on the
* great circle plane.
*/
int edge_point_on_plane(GEOGRAPHIC_EDGE e, GEOGRAPHIC_POINT p)
{
	POINT3D normal, pt;
	double w;
	robust_cross_product(e.start, e.end, &normal);
	geog2cart(p, &pt);
	w = dot_product(normal, pt);
	if( FP_IS_ZERO(w) )
	{
		LWDEBUG(4, "point is on plane");
		return LW_TRUE;
	}
	LWDEBUG(4, "point is not on plane");
	return LW_FALSE;
}

/**
* Returns true if the point p is inside the cone defined by the 
* two ends of the edge e.
(*/
int edge_point_in_cone(GEOGRAPHIC_EDGE e, GEOGRAPHIC_POINT p)
{
	POINT3D vcp, vs, ve, vp;
	double vs_dot_vcp, vp_dot_vcp, ve_dot_vcp;
	geog2cart(e.start, &vs);
	geog2cart(e.end, &ve);
	geog2cart(p, &vp);
	vector_sum(vs, ve, &vcp);
	normalize(&vcp);
	vs_dot_vcp = dot_product(vs, vcp);
	ve_dot_vcp = dot_product(ve, vcp);
	vp_dot_vcp = dot_product(vp, vcp);

	if( vp_dot_vcp > ve_dot_vcp && vp_dot_vcp > ve_dot_vcp )
	{
		LWDEBUG(4, "point is in cone");
		return LW_TRUE;
	}
	LWDEBUG(4, "point is not in cone");
	return LW_FALSE;
}

/**
* Returns true if the point p is inside both the cones defined by the 
* two ends of the edge e.
(*/
int edge_point_in_cones(GEOGRAPHIC_EDGE e, GEOGRAPHIC_POINT p)
{
	POINT3D vp, vs, ve;
	double vs_dot_ve, vp_dot_ve, vp_dot_vs;
	geog2cart(e.start, &vs);
	geog2cart(e.end, &ve);
	geog2cart(p, &vp);
	vs_dot_ve = dot_product(vs, ve);
	vp_dot_ve = dot_product(vs, vp);
	vp_dot_vs = dot_product(ve, vp);
	LWDEBUGF(4, "vs_dot_ve = %.9g",vs_dot_ve);
	LWDEBUGF(4, "vp_dot_ve = %.9g",vp_dot_ve);
	LWDEBUGF(4, "vp_dot_vs = %.9g",vp_dot_vs);
	if( vp_dot_vs > vs_dot_ve && vp_dot_ve > vs_dot_ve )
	{
		LWDEBUG(4, "point is in both cones");
		return LW_TRUE;
	}
	LWDEBUG(4, "point is not in cones");
	return LW_FALSE;
}

/**
* True if the longitude of p is within the range of the longitude of the ends of e
*/
int edge_contains_longitude(GEOGRAPHIC_EDGE e, GEOGRAPHIC_POINT p)
{
    LWDEBUGF(4, "e.start == GPOINT(%.6g %.6g) ", e.start.lat, e.start.lon);
    LWDEBUGF(4, "e.end == GPOINT(%.6g %.6g) ", e.end.lat, e.end.lon);
    LWDEBUGF(4, "p == GPOINT(%.6g %.6g) ", p.lat, p.lon);
	if( e.start.lon < p.lon && p.lon < e.end.lon )
	{
	    LWDEBUG(4, "returning TRUE");
		return LW_TRUE;
	}
	if( e.end.lon < p.lon && p.lon < e.start.lon )
	{
	    LWDEBUG(4, "returning TRUE");
		return LW_TRUE;
	}
    LWDEBUG(4, "returning FALSE");
	return LW_FALSE;
}


/**
* Given two points on a unit sphere, calculate their distance apart. 
*/
double sphere_distance(GEOGRAPHIC_POINT s, GEOGRAPHIC_POINT e)
{
	double latS = s.lat;
	double latE = e.lat;
	double dlng = e.lon - s.lon;
	double a1 = pow(cos(latE) * sin(dlng), 2.0);
	double a2 = pow(cos(latS) * sin(latE) - sin(latS) * cos(latE) * cos(dlng), 2.0);
	double a = sqrt(a1 + a2);
	double b = sin(latS) * sin(latE) + cos(latS) * cos(latE) * cos(dlng);
	return atan2(a, b);
}

/**
* Given two points on a unit sphere, calculate the direction from s to e. 
*/
double sphere_direction(GEOGRAPHIC_POINT s, GEOGRAPHIC_POINT e)
{
	double latS = s.lat;
	double latE = e.lon;
	double dlng = e.lat - s.lon;
	double heading = atan2(sin(dlng) * cos(latE),
		             cos(latS) * sin(latE) -
	                 sin(latS) * cos(latE) * cos(dlng)) / M_PI; 
	return heading;
}

/**
* Returns true if the point p is on the minor edge defined by the
* end points of e.
*/
int edge_contains_point(GEOGRAPHIC_EDGE e, GEOGRAPHIC_POINT p)
{
#if 0
/*	Old implementation */
	if( edge_point_on_plane(e, p) )
	{
		LWDEBUG(4, "point is on plane");
		if( edge_contains_longitude(e, p) )
		{
			LWDEBUG(4, "point is on edge");
			return LW_TRUE;
		}
	}
	return LW_FALSE;
#endif
	if ( edge_point_in_cone(e, p) && edge_point_on_plane(e, p) )
	{
		LWDEBUG(4, "point is on edge");
		return LW_TRUE;
	}
	LWDEBUG(4, "point is not on edge");
	return LW_FALSE;
}

/**
* Used in great circle to compute the pole of the great circle.
*/
double z_to_latitude(double z)
{
	double sign = signum(z);
	double tlat = acos(z);
    LWDEBUGF(4, "inputs: sign(%.8g) tlat(%.8g)", sign, tlat);
	if(fabs(tlat) > M_PI_2 )
	{
		tlat = sign * (M_PI - fabs(tlat));
	} 
	else 
	{
		tlat = sign * tlat;
	}
    LWDEBUGF(4, "output: tlat(%.8g)", tlat);
	return tlat;
}

/**
* Computes the pole of the great circle disk which is the intersection of
* the great circle with the line of maximum/minimum gradiant that lies on
* the great circle plane.
*/
int clairaut_cartesian(POINT3D start, POINT3D end, int top, GEOGRAPHIC_POINT *g)
{
	POINT3D t1, t2;
	GEOGRAPHIC_POINT vN1, vN2;
    LWDEBUG(4,"entering function");
	unit_normal(start, end, &t1);
	unit_normal(end, start, &t2);
    LWDEBUGF(4, "t1 == POINT(%.8g %.8g %.8g)", t1.x, t1.y, t1.z);
    LWDEBUGF(4, "t2 == POINT(%.8g %.8g %.8g)", t2.x, t2.y, t2.z);
	cart2geog(t1, &vN1);
	cart2geog(t2, &vN2);
	if( top )
	{
		g->lat = z_to_latitude(t1.z);
		g->lon = vN2.lon;
	}
	else
	{
		g->lat = z_to_latitude(t2.z);
		g->lon = vN1.lon;
	}
	return G_SUCCESS;
}

/**
* Computes the pole of the great circle disk which is the intersection of
* the great circle with the line of maximum/minimum gradiant that lies on
* the great circle plane.
*/
int clairaut_geographic(GEOGRAPHIC_POINT start, GEOGRAPHIC_POINT end, int top, GEOGRAPHIC_POINT *g)
{
	POINT3D t1, t2;
	GEOGRAPHIC_POINT vN1, vN2;
    LWDEBUG(4,"entering function");
    robust_cross_product(start, end, &t1);
    robust_cross_product(end, start, &t2);
    LWDEBUGF(4, "t1 == POINT(%.8g %.8g %.8g)", t1.x, t1.y, t1.z);
    LWDEBUGF(4, "t2 == POINT(%.8g %.8g %.8g)", t2.x, t2.y, t2.z);
    cart2geog(t1, &vN1);
    cart2geog(t2, &vN2);
        LWDEBUGF(4, "vN1 == GPOINT(%.6g %.6g) ", vN1.lat, vN1.lon);
        LWDEBUGF(4, "vN2 == GPOINT(%.6g %.6g) ", vN2.lat, vN2.lon);
    if( top )
    {
        g->lat = z_to_latitude(t1.z);
        g->lon = vN2.lon;
    }
    else
    {
        g->lat = z_to_latitude(t2.z);
        g->lon = vN1.lon;
    }
    return G_SUCCESS;
}

/**
* Returns true if an intersection can be calculated, and places it in *g. 
* Returns false otherwise.
*/
int edge_intersection(GEOGRAPHIC_EDGE e1, GEOGRAPHIC_EDGE e2, GEOGRAPHIC_POINT *g)
{
	POINT3D ea, eb, v;
	robust_cross_product(e1.start, e1.end, &ea);
	robust_cross_product(e2.start, e2.end, &eb);
	cross_product(ea, eb, &v);
	g->lat = atan2(v.z, sqrt(v.x * v.x + v.y * v.y));
	g->lon = atan2(v.y, v.x);
	if( edge_contains_point(e1, *g) && edge_contains_point(e2, *g) )
	{
		return LW_TRUE;
	}
	else
	{
		g->lat = -1.0 * g->lat;
		g->lon = g->lon + M_PI;
		if( g->lon > M_PI )
		{
			g->lon = -1.0 * (2.0 * M_PI - g->lon);
		}
		if( edge_contains_point(e1, *g) && edge_contains_point(e2, *g) )
		{
			return LW_TRUE;
		}		
	}
	return LW_FALSE;	  
}

/**
* Given a starting location r, a distance and an azimuth
* to the new point, compute the location of the projected point on the unit sphere.
*/
int sphere_project(GEOGRAPHIC_POINT r, double distance, double azimuth, GEOGRAPHIC_POINT *n) 
{
	double d = distance;
	double lat1 = r.lat;
	double a = cos(lat1) * cos(d) - sin(lat1) * sin(d) * cos(azimuth);
	double b = signum(d) * sin(azimuth);
	n->lat = asin(sin(lat1) * cos(d) +
	         cos(lat1) * sin(d) * cos(azimuth));
	n->lon = atan(b/a) + r.lon;
	return G_SUCCESS;
}


int edge_calculate_gbox_slow(GEOGRAPHIC_EDGE e, GBOX *gbox)
{
	int steps = 10000;
	int i;
	double dx, dy, dz;
	double distance = sphere_distance(e.start, e.end);
	POINT3D pn, p, start, end;

	/* Edge is zero length, just return the naive box */
	if( FP_IS_ZERO(distance) )
	{
        LWDEBUG(4, "edge is zero length. returning");
		geog2cart(e.start, &start);
		geog2cart(e.end, &end);
		gbox->xmin = FP_MIN(start.x, end.x);
		gbox->ymin = FP_MIN(start.y, end.y);
		gbox->zmin = FP_MIN(start.z, end.z);
		gbox->xmax = FP_MAX(start.x, end.x);
		gbox->ymax = FP_MAX(start.y, end.y);
		gbox->zmax = FP_MAX(start.z, end.z);
		return G_SUCCESS;
	}

	/* Edge is antipodal (one point on each side of the globe), 
	   set the box to contain the whole world and return */
	if( FP_EQUALS(distance, M_PI) )
	{
        LWDEBUG(4, "edge is antipodal. setting to maximum size box, and returning");
		gbox->xmin = gbox->ymin = gbox->zmin = -1.0;
		gbox->xmax = gbox->ymax = gbox->zmax = 1.0;
		return G_SUCCESS;
	}
	
	geog2cart(e.start, &start);
	geog2cart(e.end, &end);
	dx = (end.x - start.x)/steps;
	dy = (end.y - start.y)/steps;
	dz = (end.z - start.z)/steps;
	p = start;
	gbox->xmin = gbox->xmax = p.x;
	gbox->ymin = gbox->ymax = p.y;
	gbox->zmin = gbox->zmax = p.z;
	for( i = 0; i < steps; i++ )
	{
		p.x += dx;
		p.y += dy;
		p.z += dz;
		pn = p;
		normalize(&pn);
		gbox_merge_point3d(pn, gbox);
	}
	return G_SUCCESS;
}



/**
* The magic function, given an edge in spherical coordinates, calculate a
* 3D bounding box that fully contains it, taking into account the curvature
* of the sphere on which it is inscribed. Note special case testing
* for edges over poles and fully around the globe. An edge is assumed
* to follow the shortest great circle route between the end points.
*/
int edge_calculate_gbox(GEOGRAPHIC_EDGE e, GBOX *gbox)
{
	double deltaLongitude;
	double distance = sphere_distance(e.start, e.end);
	int flipped_longitude = LW_FALSE;
    int gimbal_lock = LW_FALSE;
	POINT3D p, start, end, startXZ, endXZ, startYZ, endYZ, nT1, nT2;
	GEOGRAPHIC_EDGE g;
	GEOGRAPHIC_POINT vT1, vT2;

	/* We're testing, do this the slow way. */
	if(gbox_geocentric_slow) 
	{
		//printf("\n--- using SLOW calc! ---\n");
		return edge_calculate_gbox_slow(e, gbox);
	}
	//printf("\n=== using FAST calc! ===\n");

	/* Initialize our working copy of the edge */
	g = e;

    LWDEBUG(4, "entered function");
	LWDEBUGF(4, "edge length: %.8g", distance);
    LWDEBUGF(4, "edge values: (%.6g %.6g, %.6g %.6g)", g.start.lon, g.start.lat, g.end.lon, g.end.lat);


	/* Edge is zero length, just return the naive box */
	if( FP_IS_ZERO(distance) )
	{
        LWDEBUG(4, "edge is zero length. returning");
		geog2cart(g.start, &start);
		geog2cart(g.end, &end);
		gbox->xmin = FP_MIN(start.x, end.x);
		gbox->ymin = FP_MIN(start.y, end.y);
		gbox->zmin = FP_MIN(start.z, end.z);
		gbox->xmax = FP_MAX(start.x, end.x);
		gbox->ymax = FP_MAX(start.y, end.y);
		gbox->zmax = FP_MAX(start.z, end.z);
		return G_SUCCESS;
	}

	/* Edge is antipodal (one point on each side of the globe), 
	   set the box to contain the whole world and return */
	if( FP_EQUALS(distance, M_PI) )
	{
        LWDEBUG(4, "edge is antipodal. setting to maximum size box, and returning");
		gbox->xmin = gbox->ymin = gbox->zmin = -1.0;
		gbox->xmax = gbox->ymax = gbox->zmax = 1.0;
		return G_SUCCESS;
	}

 	/* Calculate the difference in longitude between the two points. */
	if( signum(g.start.lon) == signum(g.end.lon) )
	{
		deltaLongitude = fabs(fabs(g.start.lon) - fabs(g.end.lon));
        LWDEBUG(4, "edge does not cross dateline (start.lon same sign as end.long)");
	}
	else
	{
		double dl = fabs(g.start.lon) + fabs(g.end.lon);
		/* Less then a hemisphere apart */
		if( dl < M_PI )
		{
			deltaLongitude = dl;
            LWDEBUG(4, "edge does not cross dateline");
		}
		/* Exactly a hemisphere apart */
		else if ( FP_EQUALS( dl, M_PI ) )
		{
			deltaLongitude = M_PI;
            LWDEBUG(4, "edge points are 180d apart");
		}
		/* More than a hemisphere apart, return the other half of the sphere 
		   and note that we are crossing the dateline */
		else
		{
			flipped_longitude = LW_TRUE;
			deltaLongitude = dl - M_PI;
            LWDEBUG(4, "edge crosses dateline");
		}
	}	
    LWDEBUGF(4, "longitude delta is %g", deltaLongitude);
	
	/* If we are crossing the dateline, flip the calculation to the other
	   side of the globe. We'll flip our output box back at the end of the
	   calculation. */
	if ( flipped_longitude )
	{
        LWDEBUG(4, "reversing longitudes");
		if ( g.start.lon > 0.0 ) 
			g.start.lon -= M_PI;
		else 
			g.start.lon += M_PI;
		if ( g.end.lon > 0.0 ) 
			g.end.lon -= M_PI;
		else 
			g.end.lon += M_PI;
	}
    LWDEBUGF(4, "edge values: (%.6g %.6g, %.6g %.6g)", g.start.lon, g.start.lat, g.end.lon, g.end.lat);

	/* Initialize box with the start and end points of the edge. */
	geog2cart(g.start, &start);
	geog2cart(g.end, &end);
	gbox->xmin = FP_MIN(start.x, end.x);
	gbox->ymin = FP_MIN(start.y, end.y);
	gbox->zmin = FP_MIN(start.z, end.z);
	gbox->xmax = FP_MAX(start.x, end.x);
	gbox->ymax = FP_MAX(start.y, end.y);
	gbox->zmax = FP_MAX(start.z, end.z);
    LWDEBUGF(4, "initialized gbox: %s", gbox_to_string(gbox));
	
	/* Check for pole crossings. */
	if( FP_EQUALS(deltaLongitude, M_PI) ) 
	{
		/* Crosses the north pole, adjust box to contain pole */
		if( (g.start.lat + g.end.lat) > 0.0 )
		{
            LWDEBUG(4, "edge crosses north pole");
			gbox->zmax = 1.0;
		} 
		/* Crosses the south pole, adjust box to contain pole */
		else 
		{
            LWDEBUG(4, "edge crosses south pole");
			gbox->zmin = -1.0;
		}
	}
	/* How about maximal latitudes in this great circle. Are any
	   of them contained within this arc? */
	else
	{
        LWDEBUG(4, "not a pole crossing, calculating clairaut points");
		clairaut_cartesian(start, end, LW_TRUE, &vT1);
		clairaut_cartesian(start, end, LW_FALSE, &vT2);
        LWDEBUGF(4, "vT1 == GPOINT(%.6g %.6g) ", vT1.lat, vT1.lon);
        LWDEBUGF(4, "vT2 == GPOINT(%.6g %.6g) ", vT2.lat, vT2.lon);
		if( edge_contains_point(g, vT1) )
		{
			geog2cart(vT1, &p);
            LWDEBUGF(4, "p == POINT(%.8g %.8g %.8g)", p.x, p.y, p.z);
			gbox_merge_point3d(p, gbox);
            LWDEBUG(4, "edge contained vT1");
            LWDEBUGF(4, "gbox: %s", gbox_to_string(gbox));
		}
		else if ( edge_contains_point(g, vT2) )
		{
			geog2cart(vT2, &p);
            LWDEBUGF(4, "p == POINT(%.8g %.8g %.8g)", p.x, p.y, p.z);
			gbox_merge_point3d(p, gbox);
            LWDEBUG(4, "edge contained vT2");
            LWDEBUGF(4, "gbox: %s", gbox_to_string(gbox));
		}
	}

    /* Flip the X axis to Z and check for maximal latitudes again. */
    LWDEBUG(4, "flipping x to z and calculating clairaut points");
    startXZ = start;
    endXZ = end;
    x_to_z(&startXZ);
    x_to_z(&endXZ);
	clairaut_cartesian(startXZ, endXZ, LW_TRUE, &vT1);
	clairaut_cartesian(startXZ, endXZ, LW_FALSE, &vT2);
    gimbal_lock = LW_FALSE;
    LWDEBUGF(4, "vT1 == GPOINT(%.6g %.6g) ", vT1.lat, vT1.lon);
    LWDEBUGF(4, "vT2 == GPOINT(%.6g %.6g) ", vT2.lat, vT2.lon);
    if ( FP_IS_ZERO(vT1.lat) ) 
    {
        gimbal_lock = LW_TRUE;
    }
    geog2cart(vT1, &nT1);
    geog2cart(vT2, &nT2);
    x_to_z(&nT1);
    x_to_z(&nT2);
    cart2geog(nT1, &vT1);
    cart2geog(nT2, &vT2);
    LWDEBUGF(4, "vT1 == GPOINT(%.6g %.6g) ", vT1.lat, vT1.lon);
    LWDEBUGF(4, "vT2 == GPOINT(%.6g %.6g) ", vT2.lat, vT2.lon);
    if( gimbal_lock )
    {
        LWDEBUG(4, "gimbal lock");
        vT1.lon = M_PI;
        vT2.lon = -1.0 * M_PI;
    }
    if( edge_contains_point(g, vT1) )
    {
		geog2cart(vT1, &p);
        LWDEBUGF(4, "p == POINT(%.8g %.8g %.8g)", p.x, p.y, p.z);
		gbox_merge_point3d(p, gbox);
        LWDEBUG(4, "edge contained vT1");
        LWDEBUGF(4, "gbox: %s", gbox_to_string(gbox));
    }
	else if ( edge_contains_point(g, vT2) )
	{
		geog2cart(vT2, &p);
        LWDEBUGF(4, "p == POINT(%.8g %.8g %.8g)", p.x, p.y, p.z);
		gbox_merge_point3d(p, gbox);
        LWDEBUG(4, "edge contained vT2");
        LWDEBUGF(4, "gbox: %s", gbox_to_string(gbox));
	}
    
    /* Flip the Y axis to Z and check for maximal latitudes again. */
    LWDEBUG(4, "flipping y to z and calculating clairaut points");
    startYZ = start;
    endYZ = end;
    y_to_z(&startYZ);
    y_to_z(&endYZ);
	clairaut_cartesian(startYZ, endYZ, LW_TRUE, &vT1);
	clairaut_cartesian(startYZ, endYZ, LW_FALSE, &vT2);
    gimbal_lock = LW_FALSE;
    if ( FP_IS_ZERO(vT1.lat) ) 
    {
        gimbal_lock = LW_TRUE;
    }
    geog2cart(vT1, &nT1);
    geog2cart(vT2, &nT2);
    y_to_z(&nT1);
    y_to_z(&nT2);
    cart2geog(nT1, &vT1);
    cart2geog(nT2, &vT2);
    LWDEBUGF(4, "vT1 == GPOINT(%.6g %.6g) ", vT1.lat, vT1.lon);
    LWDEBUGF(4, "vT2 == GPOINT(%.6g %.6g) ", vT2.lat, vT2.lon);
    if( gimbal_lock )
    {
        LWDEBUG(4, "gimbal lock");
        vT1.lon = M_PI;
        vT2.lon = -1.0 * M_PI;
    }
    if( edge_contains_point(g, vT1) )
    {
		geog2cart(vT1, &p);
        LWDEBUGF(4, "p == POINT(%.8g %.8g %.8g)", p.x, p.y, p.z);
		gbox_merge_point3d(p, gbox);
        LWDEBUG(4, "edge contained vT1");
        LWDEBUGF(4, "gbox: %s", gbox_to_string(gbox));
    }
	else if ( edge_contains_point(g, vT2) )
	{
		geog2cart(vT2, &p);
        LWDEBUGF(4, "p == POINT(%.8g %.8g %.8g)", p.x, p.y, p.z);
		gbox_merge_point3d(p, gbox);
        LWDEBUG(4, "edge contained vT2");
        LWDEBUGF(4, "gbox: %s", gbox_to_string(gbox));
	}

    /* Our cartesian gbox is complete! 
       If we flipped our longitudes, now we have to flip our cartesian box. */
    if ( flipped_longitude )  
    {
        double tmp;
        LWDEBUG(4, "flipping cartesian box back");
        LWDEBUGF(4, "gbox before: %s", gbox_to_string(gbox));
        tmp = gbox->xmax;
        gbox->xmax = -1.0 * gbox->xmin;
        gbox->xmin = -1.0 * tmp;
        tmp = gbox->ymax;
        gbox->ymax = -1.0 * gbox->ymin;
        gbox->ymin = -1.0 * tmp;
        LWDEBUGF(4, "gbox after: %s", gbox_to_string(gbox));
    }
    
    LWDEBUGF(4, "final gbox: %s", gbox_to_string(gbox));
	return G_SUCCESS;
}


/* 
* This function can only be used on LWGEOM that is built on top of
* GSERIALIZED, otherwise alignment errors will ensue.
*/
int getPoint2d_p_ro(const POINTARRAY *pa, int n, POINT2D **point)
{
	uchar *pa_ptr = NULL;
	assert(pa);
	assert(n >= 0);
	assert(n < pa->npoints);

	pa_ptr = getPoint_internal(pa, n);
	//printf( "pa_ptr[0]: %g\n", *((double*)pa_ptr));
	*point = (POINT2D*)pa_ptr;

	return G_SUCCESS;
}

int ptarray_calculate_gbox_geodetic(POINTARRAY *pa, GBOX *gbox)
{
	int i;
    int first = LW_TRUE;
    POINT2D start_pt;
    POINT2D end_pt;
    GEOGRAPHIC_EDGE edge;
    GBOX edge_gbox;

	assert(gbox);
	assert(pa);
	
    edge_gbox.flags = gbox->flags;
	
	if ( pa->npoints == 0 ) return G_FAILURE;
	
	if ( pa->npoints == 1 )
	{
	    POINT2D in_pt;
        POINT3D out_pt;
        GEOGRAPHIC_POINT gp;
        getPoint2d_p(pa, 0, &in_pt);
        gp.lon = deg2rad(in_pt.x);
        gp.lat = deg2rad(in_pt.y);
        geog2cart(gp, &out_pt);
        gbox->xmin = gbox->xmax = out_pt.x;
        gbox->ymin = gbox->ymax = out_pt.y;
        gbox->zmin = gbox->zmax = out_pt.z;
        return G_SUCCESS;
    }
	
	for( i = 1; i < pa->npoints; i++ )
	{
		getPoint2d_p(pa, i-1, &start_pt);
		getPoint2d_p(pa, i, &end_pt);
        edge.start.lon = deg2rad(start_pt.x);
        edge.start.lat = deg2rad(start_pt.y);
        edge.end.lon = deg2rad(end_pt.x);
        edge.end.lat = deg2rad(end_pt.y);

        edge_calculate_gbox(edge, &edge_gbox);

        LWDEBUGF(4, "edge_gbox: %s", gbox_to_string(&edge_gbox));
        
		/* Initialize the box */
		if( first )
		{
            gbox_duplicate(edge_gbox,gbox);
            LWDEBUGF(4, "gbox_duplicate: %s", gbox_to_string(gbox));
            first = LW_FALSE;
	    }
	    /* Expand the box where necessary */
		else
		{ 
            gbox_merge(edge_gbox, gbox);
            LWDEBUGF(4, "gbox_merge: %s", gbox_to_string(gbox));
		}
		
	}
	
	return G_SUCCESS;
	
}

static int lwpoint_calculate_gbox_geodetic(LWPOINT *point, GBOX *gbox)
{
	assert(point);
	if( ptarray_calculate_gbox_geodetic(point->point, gbox) == G_FAILURE )
		return G_FAILURE;
	return G_SUCCESS;
}

static int lwline_calculate_gbox_geodetic(LWLINE *line, GBOX *gbox)
{
	assert(line);
	if( ptarray_calculate_gbox_geodetic(line->points, gbox) == G_FAILURE )
		return G_FAILURE;
	return G_SUCCESS;
}

static int lwpolygon_calculate_gbox_geodetic(LWPOLY *poly, GBOX *gbox)
{
	GBOX ringbox;
	int i;
	int first = LW_TRUE;
	assert(poly);
	if( poly->nrings == 0 )
		return G_FAILURE;
	ringbox.flags = gbox->flags;
	for( i = 0; i < poly->nrings; i++ )
	{
		if( ptarray_calculate_gbox_geodetic(poly->rings[i], &ringbox) == G_FAILURE )
			return G_FAILURE;
		if( first )
		{
			gbox_duplicate(ringbox, gbox);
			first = LW_FALSE;
		}
		else
		{
			gbox_merge(ringbox, gbox);
		}
	}
	
	/* If the box wraps the south pole, set zmin to the absolute min. */
	if( gbox_contains_south_pole(*gbox) )
        gbox->zmin = -1.0;
	/* If the box wraps the north pole, set zmax to the absolute max. */
	if( gbox_contains_north_pole(*gbox) )
        gbox->zmax = 1.0;
        
	return G_SUCCESS;
}

static int lwcollection_calculate_gbox_geodetic(LWCOLLECTION *coll, GBOX *gbox)
{
	GBOX subbox;
	int i;
	int result = G_FAILURE;
	int first = LW_TRUE;
	assert(coll);
	if( coll->ngeoms == 0 )
		return G_FAILURE;
	
	subbox.flags = gbox->flags;
	
	for( i = 0; i < coll->ngeoms; i++ )
	{
		if( lwgeom_calculate_gbox_geodetic((LWGEOM*)(coll->geoms[i]), &subbox) == G_FAILURE )
		{
			continue;
		}
		else
		{
			if( first )
			{
				gbox_duplicate(subbox, gbox);
				first = LW_FALSE;
			}
			else
			{
				gbox_merge(subbox, gbox);
			}
			result = G_SUCCESS;
		}
	}
	return result;
}

int lwgeom_calculate_gbox_geodetic(const LWGEOM *geom, GBOX *gbox)
{
	int result = G_FAILURE;
	LWDEBUGF(4, "got type %d", TYPE_GETTYPE(geom->type));
	if ( ! FLAGS_GET_GEODETIC(gbox->flags) )
	{
		lwerror("lwgeom_get_gbox_geodetic: non-geodetic gbox provided");
	}
	switch (TYPE_GETTYPE(geom->type))
	{
		case POINTTYPE:
			result = lwpoint_calculate_gbox_geodetic((LWPOINT*)geom, gbox);
			break;
		case LINETYPE:
			result = lwline_calculate_gbox_geodetic((LWLINE *)geom, gbox);
			break;
		case POLYGONTYPE:
			result = lwpolygon_calculate_gbox_geodetic((LWPOLY *)geom, gbox);
			break;
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
			result = lwcollection_calculate_gbox_geodetic((LWCOLLECTION *)geom, gbox);
			break;
		default:
			lwerror("unsupported input geometry type: %d", TYPE_GETTYPE(geom->type));
			break;
	}
	return result;
}



static int ptarray_check_geodetic(POINTARRAY *pa)
{
	int t;
	POINT2D pt;

	assert(pa);

	for (t=0; t<pa->npoints; t++)
	{
		getPoint2d_p(pa, t, &pt);
		//printf( "%d (%g, %g)\n", t, pt.x, pt.y);
		if ( pt.x < -180.0 || pt.y < -90.0 || pt.x > 180.0 || pt.y > 90.0 )
			return LW_FALSE;
	}

	return LW_TRUE;
}

static int lwpoint_check_geodetic(LWPOINT *point)
{
	assert(point);
	return ptarray_check_geodetic(point->point);
}

static int lwline_check_geodetic(LWLINE *line)
{
	assert(line);
	return ptarray_check_geodetic(line->points);
}

static int lwpoly_check_geodetic(LWPOLY *poly)
{
	int i = 0;
	assert(poly);
	
	for( i = 0; i < poly->nrings; i++ )
	{
		if( ptarray_check_geodetic(poly->rings[i]) == LW_FALSE )
			return LW_FALSE;
	}
	return LW_TRUE;
}

static int lwcollection_check_geodetic(LWCOLLECTION *col)
{
	int i = 0;
	assert(col);
	
	for( i = 0; i < col->ngeoms; i++ )
	{
		if( lwgeom_check_geodetic(col->geoms[i]) == LW_FALSE )
			return LW_FALSE;
	}
	return LW_TRUE;
}

int lwgeom_check_geodetic(const LWGEOM *geom)
{
	switch (TYPE_GETTYPE(geom->type))
	{
		case POINTTYPE:
			return lwpoint_check_geodetic((LWPOINT *)geom);
		case LINETYPE:
			return lwline_check_geodetic((LWLINE *)geom);
		case POLYGONTYPE:
			return lwpoly_check_geodetic((LWPOLY *)geom);
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
			return lwcollection_check_geodetic((LWCOLLECTION *)geom);
		default:
			lwerror("unsupported input geometry type: %d", TYPE_GETTYPE(geom->type));
	}
	return LW_FALSE;
}








