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
* For testing geodetic bounding box, we have a magic global variable.
* When this is true (when the cunit tests set it), use the slow, but
* guaranteed correct, algorithm. Otherwise use the regular one.
*/
int gbox_geocentric_slow = LW_FALSE;

/**
* Convert a longitude to the range of -PI,PI
*/
static double longitude_radians_normalize(double lon)
{
	if( lon == -1.0 * M_PI )
		return M_PI;
	if( lon == -2.0 * M_PI )
		return 0.0;

	if( lon > 2.0 * M_PI )
		lon = remainder(lon, 2.0 * M_PI);

	if( lon < -2.0 * M_PI )
		lon = remainder(lon, -2.0 * M_PI);
		
	if( lon > M_PI )
		lon = -2.0 * M_PI + lon;

	if( lon < -1.0 * M_PI )
		lon = 2.0 * M_PI + lon;
	
	return lon;
}

/**
* Convert a latitude to the range of -PI/2,PI/2
*/
static double latitude_radians_normalize(double lat)
{

	if( lat > 2.0 * M_PI )
		lat = remainder(lat, 2.0 * M_PI);

	if( lat < -2.0 * M_PI )
		lat = remainder(lat, -2.0 * M_PI);
		
	if( lat > M_PI )
		lat = M_PI - lat;

	if( lat < -1.0 * M_PI )
		lat = -1.0 * M_PI - lat;

	if( lat > M_PI_2 )
		lat = M_PI - lat;

	if( lat < -1.0 * M_PI_2 )
		lat = -1.0 * M_PI - lat;
	
	return lat;
}

/**
* Convert a longitude to the range of -180,180
*/
static double longitude_degrees_normalize(double lon)
{
	if( lon == -180.0 )
		return 180.0;
	if( lon == -360.0 )
		return 0.0;

	if( lon > 360.0 )
		lon = remainder(lon, 360.0);

	if( lon < -360.0 )
		lon = remainder(lon, -360.0);
		
	if( lon > 180.0 )
		lon = -360.0 + lon;

	if( lon < -180.0 )
		lon = 360 + lon;
	
	return lon;
}

/**
* Convert a latitude to the range of -90,90
*/
static double latitude_degrees_normalize(double lat)
{

	if( lat > 360.0 )
		lat = remainder(lat, 360.0);

	if( lat < -360.0 )
		lat = remainder(lat, -360.0);
		
	if( lat > 180.0 )
		lat = 180.0 - lat;

	if( lat < -180.0 )
		lat = -180.0 - lat;

	if( lat > 90.0 )
		lat = 180.0 - lat;

	if( lat < -90.0 )
		lat = -180.0 - lat;
	
	return lat;
}

/**
* Convert an edge from degrees to radians. 
*/
void edge_deg2rad(GEOGRAPHIC_EDGE *e)
{
	(e->start).lat = deg2rad((e->start).lat);
	(e->end).lat = deg2rad((e->end).lat);
	(e->start).lon = deg2rad((e->start).lon);
	(e->end).lon = deg2rad((e->end).lon);
}

/**
* Convert an edge from radians to degrees.
*/
void edge_rad2deg(GEOGRAPHIC_EDGE *e)
{
	(e->start).lat = rad2deg((e->start).lat);
	(e->end).lat = rad2deg((e->end).lat);
	(e->start).lon = rad2deg((e->start).lon);
	(e->end).lon = rad2deg((e->end).lon);
}

/** 
* Convert a point from degrees to radians.
*/
void point_deg2rad(GEOGRAPHIC_POINT *p)
{
	p->lat = latitude_radians_normalize(deg2rad(p->lat));
	p->lon = longitude_radians_normalize(deg2rad(p->lon));
}

/** 
* Convert a point from radians to degrees.
*/
void point_rad2deg(GEOGRAPHIC_POINT *p)
{
	p->lat = rad2deg(p->lat);
	p->lon = rad2deg(p->lon);
}

static int geographic_point_equals(GEOGRAPHIC_POINT g1, GEOGRAPHIC_POINT g2)
{
	return FP_EQUALS(g1.lat, g2.lat) && FP_EQUALS(g1.lon, g2.lon);
}

void geographic_point_init(double lon, double lat, GEOGRAPHIC_POINT *g)
{
	g->lat = latitude_radians_normalize(deg2rad(lat));
	g->lon = longitude_radians_normalize(deg2rad(lon));	
}

/**
* Check to see if this geocentric gbox is wrapped around a pole.
* Only makes sense if this gbox originated from a polygon, as it's assuming
* the box is generated from external edges and there's an "interior" which
* contains the pole.
*
* WARNING: There might be degenerate cases that contain multiple poles but are not
* caught, this is not a 100% function.
*/
static int gbox_check_poles(GBOX *gbox)
{
	/* Z axis */
	if ( gbox->xmin < 0.0 && gbox->xmax > 0.0 &&
	        gbox->ymin < 0.0 && gbox->ymax > 0.0 )
	{
		if ( (gbox->zmin+gbox->zmin)/2 > 0.0 )
			gbox->zmax = 1.0;
		else
			gbox->zmin = -1.0;
		return LW_TRUE;
	}

	/* Y axis */
	if ( gbox->xmin < 0.0 && gbox->xmax > 0.0 &&
	        gbox->zmin < 0.0 && gbox->zmax > 0.0 )
	{
		if ( (gbox->ymin+gbox->ymin)/2 > 0.0 )
			gbox->ymax = 1.0;
		else
			gbox->ymin = -1.0;
		return LW_TRUE;
	}

	/* X axis */
	if ( gbox->ymin < 0.0 && gbox->ymax > 0.0 &&
	        gbox->zmin < 0.0 && gbox->zmax > 0.0 )
	{
		if ( (gbox->xmin+gbox->xmin)/2 > 0.0 )
			gbox->xmax = 1.0;
		else
			gbox->xmin = -1.0;
		return LW_TRUE;
	}

	return LW_FALSE;
}


/**
* Convert spherical coordinates to cartesion coordinates on unit sphere
*/
void geog2cart(GEOGRAPHIC_POINT g, POINT3D *p)
{
	p->x = cos(g.lat) * cos(g.lon);
	p->y = cos(g.lat) * sin(g.lon);
	p->z = sin(g.lat);
}

/**
* Convert cartesion coordinates to spherical coordinates on unit sphere
*/
void cart2geog(POINT3D p, GEOGRAPHIC_POINT *g)
{
	g->lon = atan2(p.y, p.x);
	g->lat = asin(p.z);
}

/**
* Calculate the dot product of two unit vectors 
* (-1 == opposite, 0 == orthogonal, 1 == identical)
*/
static double dot_product(POINT3D p1, POINT3D p2)
{
	return (p1.x*p2.x) + (p1.y*p2.y) + (p1.z*p2.z);
}

/**
* Calculate the cross product of two vectors
*/
static void cross_product(POINT3D a, POINT3D b, POINT3D *n)
{
	n->x = a.y * b.z - a.z * b.y;
	n->y = a.z * b.x - a.x * b.z;
	n->z = a.x * b.y - a.y * b.x;
	return;
}

/**
* Calculate the sum of two vectors
*/
static void vector_sum(POINT3D a, POINT3D b, POINT3D *n)
{
	n->x = a.x + b.x;
	n->y = a.y + b.y;
	n->z = a.z + b.z;
	return;
}

/**
* Calculate the difference of two vectors
*/
static void vector_difference(POINT3D a, POINT3D b, POINT3D *n)
{
	n->x = a.x - b.x;
	n->y = a.y - b.y;
	n->z = a.z - b.z;
	return;
}

/**
* Scale a vector out by a factor
*/
static void vector_scale(POINT3D *n, double scale)
{
	n->x *= scale;
	n->y *= scale;
	n->z *= scale;
	return;
}

/**
* Normalize to a unit vector.
*/
static void normalize(POINT3D *p)
{
	double d = sqrt(p->x*p->x + p->y*p->y + p->z*p->z);
	if (FP_IS_ZERO(d))
	{
		p->x = p->y = p->z = 0.0;
		return;
	}
	p->x = p->x / d;
	p->y = p->y / d;
	p->z = p->z / d;
	return;
}

static void unit_normal(POINT3D a, POINT3D b, POINT3D *n)
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
	double sin_p_lat_minus_q_lat = sin(p.lat-q.lat);
	double sin_p_lat_plus_q_lat = sin(p.lat+q.lat);
	double sin_lon_qpp = sin(lon_qpp);
	double sin_lon_qmp = sin(lon_qmp);
	double cos_lon_qpp = cos(lon_qpp);
	double cos_lon_qmp = cos(lon_qmp);
	a->x = sin_p_lat_minus_q_lat * sin_lon_qpp * cos_lon_qmp -
	        sin_p_lat_plus_q_lat * cos_lon_qpp * sin_lon_qmp;
	a->y = sin_p_lat_minus_q_lat * cos_lon_qpp * cos_lon_qmp +
	        sin_p_lat_plus_q_lat * sin_lon_qpp * sin_lon_qmp;
	a->z = cos(p.lat) * cos(q.lat) * sin(q.lon-p.lon);
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
	/* Normal to the plane defined by e */
	robust_cross_product(e.start, e.end, &normal);
	normalize(&normal);
	geog2cart(p, &pt);
	/* We expect the dot product of with normal with any vector in the plane to be zero */
	w = dot_product(normal, pt);
	LWDEBUGF(4,"dot product %.9g",w);
	if ( FP_IS_ZERO(w) )
	{
		LWDEBUG(4, "point is on plane (dot product is zero)");
		return LW_TRUE;
	}
	LWDEBUG(4, "point is not on plane");
	return LW_FALSE;
}

/**
* Returns true if the point p is inside the cone defined by the
* two ends of the edge e.
*/
int edge_point_in_cone(GEOGRAPHIC_EDGE e, GEOGRAPHIC_POINT p)
{
	POINT3D vcp, vs, ve, vp;
	double vs_dot_vcp, vp_dot_vcp;
	geog2cart(e.start, &vs);
	geog2cart(e.end, &ve);
	/* Antipodal case, everything is inside. */
	if( vs.x == -1.0 * ve.x && vs.y == -1.0 * ve.y && vs.z == -1.0 * ve.z )
		return LW_TRUE;
	geog2cart(p, &vp);
	/* The normalized sum bisects the angle between start and end. */
	vector_sum(vs, ve, &vcp);
	normalize(&vcp);
	/* The projection of start onto the center defines the minimum similarity */
	vs_dot_vcp = dot_product(vs, vcp);
	LWDEBUGF(4,"vs_dot_vcp %.19g",vs_dot_vcp);
	/* The projection of candidate p onto the center */
	vp_dot_vcp = dot_product(vp, vcp);
	LWDEBUGF(4,"vp_dot_vcp %.19g",vp_dot_vcp);
	/* If p is more similar than start then p is inside the cone */
	if ( vp_dot_vcp >= vs_dot_vcp )
	{
		LWDEBUG(4, "point is in cone");
		return LW_TRUE;
	}
	LWDEBUG(4, "point is not in cone");
	return LW_FALSE;
}

/**
* True if the longitude of p is within the range of the longitude of the ends of e
*/
int edge_contains_coplanar_point(GEOGRAPHIC_EDGE e, GEOGRAPHIC_POINT p)
{
	GEOGRAPHIC_EDGE g;
	GEOGRAPHIC_POINT q;
	double slon = fabs(e.start.lon) + fabs(e.end.lon);
	double dlon = fabs(fabs(e.start.lon) - fabs(e.end.lon));
	double slat = e.start.lat + e.end.lat;

	LWDEBUGF(4, "e.start == GPOINT(%.6g %.6g) ", e.start.lat, e.start.lon);
	LWDEBUGF(4, "e.end == GPOINT(%.6g %.6g) ", e.end.lat, e.end.lon);
	LWDEBUGF(4, "p == GPOINT(%.6g %.6g) ", p.lat, p.lon);
	
	/* Copy values into working registers */
	g = e;
	q = p;

	/* Vertical plane, we need to do this calculation in latitude */
	if( FP_EQUALS( g.start.lon, g.end.lon ) )
	{
		LWDEBUG(4, "vertical plane, we need to do this calculation in latitude");
		/* Supposed to be co-planar... */
		if ( ! FP_EQUALS( q.lon, g.start.lon ) )
			return LW_FALSE;

		if ( ( g.start.lat <= q.lat && q.lat <= g.end.lat ) || 
		     ( g.end.lat <= q.lat && q.lat <= g.start.lat ) )
		{
			return LW_TRUE;
		}
		else
		{
			return LW_FALSE;
		}
	}

	/* Over the pole, we need normalize latitude and do this calculation in latitude */
	if ( FP_EQUALS( slon, M_PI ) && ( signum(g.start.lon) != signum(g.end.lon) || FP_EQUALS(dlon, M_PI) ) )
	{
		LWDEBUG(4, "over the pole...");
		/* Antipodal, everything (or nothing?) is inside */
		if ( FP_EQUALS( slat, 0.0 ) )
			return LW_TRUE;

		/* Point *is* the north pole */
		if ( slat > 0.0 && FP_EQUALS(q.lat, M_PI_2 ) )
			return LW_TRUE;
		
		/* Point *is* the south pole */
		if ( slat < 0.0 && FP_EQUALS(q.lat, -1.0 * M_PI_2) )
			return LW_TRUE;

		LWDEBUG(4, "coplanar?...");

		/* Supposed to be co-planar... */
		if ( ! FP_EQUALS( q.lon, g.start.lon ) )
			return LW_FALSE;

		LWDEBUG(4, "north or south?...");
				
		/* Over north pole, test based on south pole */
		if ( slat > 0.0 )
		{
			LWDEBUG(4, "over the north pole...");
			if( q.lat > FP_MIN(g.start.lat, g.end.lat) )
				return LW_TRUE;
			else 
				return LW_FALSE;
		}
		else
		/* Over south pole, test based on north pole */
		{
			LWDEBUG(4, "over the south pole...");
			if( q.lat < FP_MAX(g.start.lat, g.end.lat) )
				return LW_TRUE;
			else 
				return LW_FALSE;
		}			
	}

	/* Dateline crossing, flip everything to the opposite hemisphere */
	else if( slon > M_PI && ( signum(g.start.lon) != signum(g.end.lon) ) )
	{
		LWDEBUG(4, "crosses dateline, flip longitudes...");
		if ( g.start.lon > 0.0 )
			g.start.lon -= M_PI;
		else
			g.start.lon += M_PI;
		if ( g.end.lon > 0.0 )
			g.end.lon -= M_PI;
		else
			g.end.lon += M_PI;

		if ( q.lon > 0.0 )
			q.lon -= M_PI;
		else
			q.lon += M_PI;			
	}

	if ( ( g.start.lon <= q.lon && q.lon <= g.end.lon ) || 
	     ( g.end.lon <= q.lon && q.lon <= g.start.lon ) )
	{
		LWDEBUG(4, "true, this edge contains point");
		return LW_TRUE;
	}

	LWDEBUG(4, "false, this edge does not contain point");
	return LW_FALSE;
}


/**
* Given two points on a unit sphere, calculate their distance apart in radians.
*/
double sphere_distance(GEOGRAPHIC_POINT s, GEOGRAPHIC_POINT e)
{
	double d_lon = e.lon - s.lon;
	double cos_d_lon = cos(d_lon);
	double cos_lat_e = cos(e.lat);
	double sin_lat_e = sin(e.lat);
	double cos_lat_s = cos(s.lat);
	double sin_lat_s = sin(s.lat);
	
	double a1 = pow(cos_lat_e * sin(d_lon), 2.0);
	double a2 = pow(cos_lat_s * sin_lat_e - sin_lat_s * cos_lat_e * cos_d_lon, 2.0);
	double a = sqrt(a1 + a2);
	double b = sin_lat_s * sin_lat_e + cos_lat_s * cos_lat_e * cos_d_lon;
	return atan2(a, b);
}

/**
* Given two unit vectors, calculate their distance apart in radians.
*/
double sphere_distance_cartesian(POINT3D s, POINT3D e)
{
	return acos(dot_product(s, e));
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
	if ( edge_point_in_cone(e, p) && edge_point_on_plane(e, p) )
/*	if ( edge_contains_coplanar_point(e, p) && edge_point_on_plane(e, p) ) */
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
double z_to_latitude(double z, int top)
{
	double sign = signum(z);
	double tlat = acos(z);
	LWDEBUGF(4, "inputs: z(%.8g) sign(%.8g) tlat(%.8g)", z, sign, tlat);
	if (FP_IS_ZERO(z))
	{
		if (top) return M_PI_2;
		else return -1.0 * M_PI_2;
	}
	if (fabs(tlat) > M_PI_2 )
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
int clairaut_cartesian(POINT3D start, POINT3D end, GEOGRAPHIC_POINT *g_top, GEOGRAPHIC_POINT *g_bottom)
{
	POINT3D t1, t2;
	GEOGRAPHIC_POINT vN1, vN2;
	LWDEBUG(4,"entering function");
	unit_normal(start, end, &t1);
	unit_normal(end, start, &t2);
	LWDEBUGF(4, "unit normal t1 == POINT(%.8g %.8g %.8g)", t1.x, t1.y, t1.z);
	LWDEBUGF(4, "unit normal t2 == POINT(%.8g %.8g %.8g)", t2.x, t2.y, t2.z);
	cart2geog(t1, &vN1);
	cart2geog(t2, &vN2);
	g_top->lat = z_to_latitude(t1.z,LW_TRUE);
	g_top->lon = vN2.lon;
	g_bottom->lat = z_to_latitude(t2.z,LW_FALSE);
	g_bottom->lon = vN1.lon;
	LWDEBUGF(4, "clairaut top == GPOINT(%.6g %.6g)", g_top->lat, g_top->lon);
	LWDEBUGF(4, "clairaut bottom == GPOINT(%.6g %.6g)", g_bottom->lat, g_bottom->lon);
	return G_SUCCESS;
}

/**
* Computes the pole of the great circle disk which is the intersection of
* the great circle with the line of maximum/minimum gradiant that lies on
* the great circle plane.
*/
int clairaut_geographic(GEOGRAPHIC_POINT start, GEOGRAPHIC_POINT end, GEOGRAPHIC_POINT *g_top, GEOGRAPHIC_POINT *g_bottom)
{
	POINT3D t1, t2;
	GEOGRAPHIC_POINT vN1, vN2;
	LWDEBUG(4,"entering function");
	robust_cross_product(start, end, &t1);
	normalize(&t1);
	robust_cross_product(end, start, &t2);
	normalize(&t2);
	LWDEBUGF(4, "unit normal t1 == POINT(%.8g %.8g %.8g)", t1.x, t1.y, t1.z);
	LWDEBUGF(4, "unit normal t2 == POINT(%.8g %.8g %.8g)", t2.x, t2.y, t2.z);
	cart2geog(t1, &vN1);
	cart2geog(t2, &vN2);
	g_top->lat = z_to_latitude(t1.z,LW_TRUE);
	g_top->lon = vN2.lon;
	g_bottom->lat = z_to_latitude(t2.z,LW_FALSE);
	g_bottom->lon = vN1.lon;
	LWDEBUGF(4, "clairaut top == GPOINT(%.6g %.6g)", g_top->lat, g_top->lon);
	LWDEBUGF(4, "clairaut bottom == GPOINT(%.6g %.6g)", g_bottom->lat, g_bottom->lon);
	return G_SUCCESS;
}

/**
* Returns true if an intersection can be calculated, and places it in *g.
* Returns false otherwise.
*/
int edge_intersection(GEOGRAPHIC_EDGE e1, GEOGRAPHIC_EDGE e2, GEOGRAPHIC_POINT *g)
{
	POINT3D ea, eb, v;
	LWDEBUGF(4, "e1 start(%.20g %.20g) end(%.20g %.20g)", e1.start.lat, e1.start.lon, e1.end.lat, e1.end.lon);
	LWDEBUGF(4, "e2 start(%.20g %.20g) end(%.20g %.20g)", e2.start.lat, e2.start.lon, e2.end.lat, e2.end.lon);
	
	LWDEBUGF(4, "e1 start(%.20g %.20g) end(%.20g %.20g)", rad2deg(e1.start.lon), rad2deg(e1.start.lat), rad2deg(e1.end.lon), rad2deg(e1.end.lat));
	LWDEBUGF(4, "e2 start(%.20g %.20g) end(%.20g %.20g)", rad2deg(e2.start.lon), rad2deg(e2.start.lat), rad2deg(e2.end.lon), rad2deg(e2.end.lat));

	robust_cross_product(e1.start, e1.end, &ea);
	normalize(&ea);
	robust_cross_product(e2.start, e2.end, &eb);
	normalize(&eb);
	LWDEBUGF(4, "e1 cross product == POINT(%.12g %.12g %.12g)", ea.x, ea.y, ea.z);
	LWDEBUGF(4, "e2 cross product == POINT(%.12g %.12g %.12g)", eb.x, eb.y, eb.z);
	LWDEBUGF(4, "fabs(dot_product(ea, eb)) == %.14g", fabs(dot_product(ea, eb)));
	if( FP_EQUALS(fabs(dot_product(ea, eb)), 1.0) )
	{
		LWDEBUGF(4, "parallel edges found! dot_product = %.12g", dot_product(ea, eb));
		/* Parallel (maybe equal) edges! */
		/* Hack alert, only returning ONE end of the edge right now, most do better later. */
		if ( edge_contains_point(e1, e2.start) )
		{
			*g = e2.start;
			return LW_TRUE;
		}
		if ( edge_contains_point(e1, e2.end) )
		{
			*g = e2.end;
			return LW_TRUE;
		}
		if ( edge_contains_point(e2, e1.start) )
		{
			*g = e1.start;
			return LW_TRUE;
		}
		if ( edge_contains_point(e2, e1.end) )
		{
			*g = e1.end;
			return LW_TRUE;
		}
	}
	unit_normal(ea, eb, &v);
	LWDEBUGF(4, "v == POINT(%.8g %.8g %.8g)", v.x, v.y, v.z);
	g->lat = atan2(v.z, sqrt(v.x * v.x + v.y * v.y));
	g->lon = atan2(v.y, v.x);
	LWDEBUGF(4, "g == GPOINT(%.6g %.6g)", g->lat, g->lon);
	LWDEBUGF(4, "g == POINT(%.12g %.12g)", rad2deg(g->lon), rad2deg(g->lat));
	if ( edge_contains_point(e1, *g) && edge_contains_point(e2, *g) )
	{
		return LW_TRUE;
	}
	else
	{
		g->lat = -1.0 * g->lat;
		g->lon = g->lon + M_PI;
		if ( g->lon > M_PI )
		{
			g->lon = -1.0 * (2.0 * M_PI - g->lon);
		}
		if ( edge_contains_point(e1, *g) && edge_contains_point(e2, *g) )
		{
			return LW_TRUE;
		}
	}
	return LW_FALSE;
}

double edge_distance_to_point(GEOGRAPHIC_EDGE e, GEOGRAPHIC_POINT gp, GEOGRAPHIC_POINT *closest)
{
	double d1 = 1000000000.0, d2, d3, d_nearest;
	POINT3D n, p, k;
	GEOGRAPHIC_POINT gk, g_nearest;

	/* Zero length edge, */
	if( geographic_point_equals(e.start,e.end) ) 
		return sphere_distance(e.start, gp);
	
	robust_cross_product(e.start, e.end, &n);
	normalize(&n);
	geog2cart(gp, &p);
	vector_scale(&n, dot_product(p, n));
	vector_difference(p, n, &k);
	normalize(&k);
	cart2geog(k, &gk);
	if( edge_contains_point(e, gk) )
	{
		d1 = sphere_distance(gp, gk);
	}
	d2 = sphere_distance(gp, e.start);
	d3 = sphere_distance(gp, e.end);
	
	d_nearest = d1;
	g_nearest = gk;
	
	if( d2 < d_nearest ) 
	{
		d_nearest = d2;
		g_nearest = e.start;
	}
	if( d3 < d_nearest ) 
	{
		d_nearest = d3;
		g_nearest = e.end;
	}
	if(closest)
		*closest = g_nearest;
	
	return d_nearest;	
}

double edge_distance_to_edge(GEOGRAPHIC_EDGE e1, GEOGRAPHIC_EDGE e2, GEOGRAPHIC_POINT *closest1, GEOGRAPHIC_POINT *closest2)
{
	double d;
	GEOGRAPHIC_POINT gcp1s, gcp1e, gcp2s, gcp2e, c1, c2;
	double d1s = edge_distance_to_point(e1, e2.start, &gcp1s);
	double d1e = edge_distance_to_point(e1, e2.end, &gcp1e);
	double d2s = edge_distance_to_point(e2, e1.start, &gcp2s);
	double d2e = edge_distance_to_point(e2, e1.end, &gcp2e);

	d = d1s;
	c1 = gcp1s;
	c2 = e2.start;
	
	if( d1e < d ) 
	{
		d = d1e;
		c1 = gcp1e;
		c2 = e2.end;
	}

	if( d2s < d ) 
	{
		d = d2s;
		c1 = e1.start;
		c2 = gcp2s;
	}
	
	if( d2e < d ) 
	{
		d = d2e;
		c1 = e1.end;
		c2 = gcp2e;
	}
	
	if( closest1 ) *closest1 = c1;
	if( closest2 ) *closest2 = c2;
		
	return d;
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
	int steps = 1000000;
	int i;
	double dx, dy, dz;
	double distance = sphere_distance(e.start, e.end);
	POINT3D pn, p, start, end;

	/* Edge is zero length, just return the naive box */
	if ( FP_IS_ZERO(distance) )
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
	if ( FP_EQUALS(distance, M_PI) )
	{
		LWDEBUG(4, "edge is antipodal. setting to maximum size box, and returning");
		gbox->xmin = gbox->ymin = gbox->zmin = -1.0;
		gbox->xmax = gbox->ymax = gbox->zmax = 1.0;
		return G_SUCCESS;
	}

	/* Walk along the chord between start and end incrementally, 
	   normalizing at each step. */
	geog2cart(e.start, &start);
	geog2cart(e.end, &end);
	dx = (end.x - start.x)/steps;
	dy = (end.y - start.y)/steps;
	dz = (end.z - start.z)/steps;
	p = start;
	gbox->xmin = gbox->xmax = p.x;
	gbox->ymin = gbox->ymax = p.y;
	gbox->zmin = gbox->zmax = p.z;
	for ( i = 0; i < steps; i++ )
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
	if (gbox_geocentric_slow)
	{
		return edge_calculate_gbox_slow(e, gbox);
	}

	/* Initialize our working copy of the edge */
	g = e;

	LWDEBUG(4, "entered function");
	LWDEBUGF(4, "edge length: %.8g", distance);
	LWDEBUGF(4, "edge values: (%.6g %.6g, %.6g %.6g)", g.start.lon, g.start.lat, g.end.lon, g.end.lat);

	/* Edge is zero length, just return the naive box */
	if ( FP_IS_ZERO(distance) )
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
	if ( FP_EQUALS(distance, M_PI) )
	{
		LWDEBUG(4, "edge is antipodal. setting to maximum size box, and returning");
		gbox->xmin = gbox->ymin = gbox->zmin = -1.0;
		gbox->xmax = gbox->ymax = gbox->zmax = 1.0;
		return G_SUCCESS;
	}

	/* Calculate the difference in longitude between the two points. */
	if ( signum(g.start.lon) == signum(g.end.lon) )
	{
		deltaLongitude = fabs(fabs(g.start.lon) - fabs(g.end.lon));
		LWDEBUG(4, "edge does not cross dateline (start.lon same sign as end.long)");
	}
	else
	{
		double dl = fabs(g.start.lon) + fabs(g.end.lon);
		/* Less then a hemisphere apart */
		if ( dl < M_PI )
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
	if ( FP_EQUALS(deltaLongitude, M_PI) )
	{
		/* Crosses the north pole, adjust box to contain pole */
		if ( (g.start.lat + g.end.lat) > 0.0 )
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
		clairaut_cartesian(start, end, &vT1, &vT2);
		LWDEBUGF(4, "vT1 == GPOINT(%.6g %.6g) ", vT1.lat, vT1.lon);
		LWDEBUGF(4, "vT2 == GPOINT(%.6g %.6g) ", vT2.lat, vT2.lon);
		if ( edge_contains_point(g, vT1) )
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
	clairaut_cartesian(startXZ, endXZ, &vT1, &vT2);
	gimbal_lock = LW_FALSE;
	LWDEBUG(4, "vT1/vT2 before flipping back z to x");
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
	LWDEBUG(4, "vT1/vT2 after flipping back z to x");
	LWDEBUGF(4, "vT1 == GPOINT(%.6g %.6g) ", vT1.lat, vT1.lon);
	LWDEBUGF(4, "vT2 == GPOINT(%.6g %.6g) ", vT2.lat, vT2.lon);
	if ( gimbal_lock )
	{
		LWDEBUG(4, "gimbal lock");
		vT1.lon = 0.0;
		vT2.lon = M_PI;
		LWDEBUGF(4, "vT1 == GPOINT(%.6g %.6g) ", vT1.lat, vT1.lon);
		LWDEBUGF(4, "vT2 == GPOINT(%.6g %.6g) ", vT2.lat, vT2.lon);
	}
	/* For extra logging if needed
	    geog2cart(vT1, &nT1);
	    geog2cart(vT2, &nT2);
	    LWDEBUGF(4, "p1 == POINT(%.8g %.8g %.8g)", nT1.x, nT1.y, nT1.z);
	    LWDEBUGF(4, "p2 == POINT(%.8g %.8g %.8g)", nT2.x, nT2.y, nT2.z);
	*/
	if ( edge_contains_point(g, vT1) )
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
	clairaut_cartesian(startYZ, endYZ, &vT1, &vT2);
	gimbal_lock = LW_FALSE;
	LWDEBUG(4, "vT1/vT2 before flipping back z to y");
	LWDEBUGF(4, "vT1 == GPOINT(%.6g %.6g) ", vT1.lat, vT1.lon);
	LWDEBUGF(4, "vT2 == GPOINT(%.6g %.6g) ", vT2.lat, vT2.lon);
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
	LWDEBUG(4, "vT1/vT2 after flipping back z to y");
	LWDEBUGF(4, "vT1 == GPOINT(%.6g %.6g) ", vT1.lat, vT1.lon);
	LWDEBUGF(4, "vT2 == GPOINT(%.6g %.6g) ", vT2.lat, vT2.lon);
	if ( gimbal_lock )
	{
		LWDEBUG(4, "gimbal lock");
		vT1.lon = M_PI_2;
		vT2.lon = -1.0 * M_PI_2;
		LWDEBUGF(4, "vT1 == GPOINT(%.6g %.6g) ", vT1.lat, vT1.lon);
		LWDEBUGF(4, "vT2 == GPOINT(%.6g %.6g) ", vT2.lat, vT2.lon);
	}
	/* For extra logging if needed
	    geog2cart(vT1, &nT1);
	    geog2cart(vT2, &nT2);
	    LWDEBUGF(4, "p1 == POINT(%.8g %.8g %.8g)", nT1.x, nT1.y, nT1.z);
	    LWDEBUGF(4, "p2 == POINT(%.8g %.8g %.8g)", nT2.x, nT2.y, nT2.z);
	*/
	if ( edge_contains_point(g, vT1) )
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
	   If we flipped our longitudes at the start, n
	   now we have to flip our cartesian box. */
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

/**
* Given a gbox, return a cartesian unit vector to a point that is 
* guaranteed to be outside the box (and therefore anything it contains).
*/
static void gbox_pt_outside(GBOX gbox, POINT3D *pt)
{
	static double grow = M_PI / 180.0 / 60.0; /* one arc-minute */
	double d;
	int i;
	GBOX ge;
	POINT3D corners[8];
	
	/* Assign our box and expand it slightly. */
	ge = gbox;
	ge.xmin -= grow;
	ge.ymin -= grow;
	ge.zmin -= grow;
	ge.xmax += grow;
	ge.ymax += grow;
	ge.zmax += grow;
	
	/* Build our eight corner points */
	corners[0].x = ge.xmin;
	corners[0].y = ge.ymin;
	corners[0].z = ge.zmin;

	corners[1].x = ge.xmin;
	corners[1].y = ge.ymax;
	corners[1].z = ge.zmin;

	corners[2].x = ge.xmin;
	corners[2].y = ge.ymin;
	corners[2].z = ge.zmax;

	corners[3].x = ge.xmax;
	corners[3].y = ge.ymin;
	corners[3].z = ge.zmin;

	corners[4].x = ge.xmax;
	corners[4].y = ge.ymax;
	corners[4].z = ge.zmin;

	corners[5].x = ge.xmax;
	corners[5].y = ge.ymin;
	corners[5].z = ge.zmax;

	corners[6].x = ge.xmin;
	corners[6].y = ge.ymax;
	corners[6].z = ge.zmax;

	corners[7].x = ge.xmax;
	corners[7].y = ge.ymax;
	corners[7].z = ge.zmax;

	for( i = 0; i < 8; i++ )
	{
		normalize(&(corners[i]));
		if( ! gbox_contains_point3d(gbox, corners[i]) )
		{
			*pt = corners[i];
			return;
		}	
	}

	pt->x = 1.0;
	pt->y = 0.0;
	pt->z = 0.0;
	
	if((1.0 - gbox.xmax) > 0.1) 
	{
		pt->x = gbox.xmax + (1.0 - gbox.xmax) * 0.01;
		d = sqrt((1.0 - pow(pt->x, 2.0))/2.0);
		pt->y = d;
		pt->z = d;
	} 
	else if((1.0 - gbox.ymax) > 0.1) 
	{
		pt->y = gbox.ymax + (1.0 - gbox.ymax) * 0.01;
		d = sqrt((1.0 - pow(pt->y, 2.0))/2.0);
		pt->x = d;
		pt->z = d;
	} 
	else if((1.0 - gbox.zmax) > 0.1) 
	{
		pt->z = gbox.zmax + (1.0 - gbox.zmax) * 0.01;
		d = sqrt((1.0 - pow(pt->z, 2.0))/2.0);
		pt->x = d;
		pt->y = d;
	}
	normalize(pt);
	return;
}


/**
* This routine returns LW_TRUE if the point is inside the ring or on the boundary, LW_FALSE otherwise.
* The pt_outside must be guaranteed to be outside the ring (use the geography_pt_outside() function
* to derive one in postgis, or the gbox_pt_outside() function if you don't mind burning CPU cycles
* building a gbox first).
*/
int ptarray_point_in_ring(POINTARRAY *pa, POINT2D pt_outside, POINT2D pt_to_test) 
{
	GEOGRAPHIC_EDGE crossing_edge, edge;
	POINT2D p;
	int count = 0;
	int i;
	
	/* Null input, not enough points for a ring? You ain't closed! */
	if( ! pa || pa->npoints < 4 ) 
		return LW_FALSE;

	/* Set up our stab line */
	geographic_point_init(pt_to_test.x, pt_to_test.y, &(crossing_edge.start));
	geographic_point_init(pt_outside.x, pt_outside.y, &(crossing_edge.end));

	/* Walk every edge and see if the stab line hits it */
	for( i = 1; i < pa->npoints; i++ )
	{
		GEOGRAPHIC_POINT g;
		getPoint2d_p(pa, i-1, &p);
		geographic_point_init(p.x, p.y, &(edge.start));
		getPoint2d_p(pa, i, &p);
		geographic_point_init(p.x, p.y, &(edge.end));

		/* Does stab line cross, and if so, not on the first point. We except the
		   first point to avoid double counting crossings at vertices. */
		LWDEBUG(4,"testing edge crossing");
		if( edge_intersection(edge, crossing_edge, &g) )
		{
			if( ! geographic_point_equals(g, edge.start) )
			{
				LWDEBUG(4,"edge crossing found!");
				count++;
				if ( geographic_point_equals(g, edge.end) )
				{
					LWDEBUG(4,"got end point cross");
				}
			}
			else
			{
				LWDEBUG(4,"got start point cross");
			}
		}
	}
	/* An odd number of crossings implies containment! */
	if( count % 2 )
	{
		return LW_TRUE;
	}
	
	return LW_FALSE;
}


static double ptarray_distance_sphere(POINTARRAY *pa1, POINTARRAY *pa2, double tolerance)
{
	GEOGRAPHIC_EDGE e1, e2;
	GEOGRAPHIC_POINT g1, g2;
	POINT2D p;
	double distance;
	int i, j;
	
	/* Make result really big, so that everything will be smaller than it */
	distance = MAXFLOAT;
	
	/* Empty point arrays? Return negative */
	if ( pa1->npoints == 0 || pa1->npoints == 0 )
		return -1.0;
	
	/* Handle point/point case here */
	if ( pa1->npoints == 1 && pa2->npoints == 1 )
	{
		getPoint2d_p(pa1, 0, &p);
		geographic_point_init(p.x, p.y, &g1);
		getPoint2d_p(pa2, 0, &p);
		geographic_point_init(p.x, p.y, &g2);
		return sphere_distance(g1, g2);
	}

	/* Handle point/line case here */
	if ( pa1->npoints == 1 || pa2->npoints == 1 )
	{
		/* Handle one/many case here */
		int i;
		POINTARRAY *pa_one, *pa_many;
		
		if( pa1->npoints == 1 )
		{
			pa_one = pa1;
			pa_many = pa2;
		}
		else
		{
			pa_one = pa2;
			pa_many = pa1;
		}
		
		/* Initialize our point */
		getPoint2d_p(pa_one, 0, &p);
		geographic_point_init(p.x, p.y, &g1);

		/* Iterate through the edges in our line */
		for( i = 1; i < pa_many->npoints; i++ )
		{
			double d;
			getPoint2d_p(pa_many, i - 1, &p);
			geographic_point_init(p.x, p.y, &(e1.start));
			getPoint2d_p(pa_many, i, &p);
			geographic_point_init(p.x, p.y, &(e1.end));
			d = edge_distance_to_point(e1, g1, 0);
			if( d < distance )
				distance = d;
			if( d < tolerance ) 
				return distance;
		}
		return distance;			
	}
	
	/* Handle line/line case */
	for( i = 1; i < pa1->npoints; i++ )
	{
		getPoint2d_p(pa1, i - 1, &p);
		geographic_point_init(p.x, p.y, &(e1.start));
		getPoint2d_p(pa1, i, &p);
		geographic_point_init(p.x, p.y, &(e1.end));

		for( j = 1; j < pa2->npoints; j++ )
		{
			double d;
			GEOGRAPHIC_POINT g;

			getPoint2d_p(pa2, j - 1, &p);
			geographic_point_init(p.x, p.y, &(e2.start));
			getPoint2d_p(pa2, j, &p);
			geographic_point_init(p.x, p.y, &(e2.end));

			LWDEBUGF(4, "e1.start == GPOINT(%.6g %.6g) ", e1.start.lat, e1.start.lon);
			LWDEBUGF(4, "e1.end == GPOINT(%.6g %.6g) ", e1.end.lat, e1.end.lon);
			LWDEBUGF(4, "e2.start == GPOINT(%.6g %.6g) ", e2.start.lat, e2.start.lon);
			LWDEBUGF(4, "e2.end == GPOINT(%.6g %.6g) ", e2.end.lat, e2.end.lon);

			if ( edge_intersection(e1, e2, &g) )
			{
				LWDEBUG(4,"edge intersection! returning 0.0");
				return 0.0;
			}
			d = edge_distance_to_edge(e1, e2, 0, 0);
			LWDEBUGF(4,"got edge_distance_to_edge %.8g", d);
			if( d < distance )
				distance = d;
			if( d < tolerance )
				return distance;
		}
	}
	LWDEBUGF(4,"finished all loops, returning %.8g", distance);
	
	return distance;
}


/**
* Calculate the distance between two LWGEOMs, using the coordinates are 
* longitude and latitude. Return immediately when the calulated distance drops
* below the tolerance (useful for dwithin calculations).
*/
double lwgeom_distance_sphere(LWGEOM *lwgeom1, LWGEOM *lwgeom2, GBOX gbox1, GBOX gbox2, double tolerance)
{
	int type1, type2;
	
	assert(lwgeom1);
	assert(lwgeom2);
	
	LWDEBUGF(4, "entered function, tolerance %.8g", tolerance);

	/* What's the distance to an empty geometry? We don't know. */
	if( lwgeom_is_empty(lwgeom1) || lwgeom_is_empty(lwgeom2) )
	{
		return 0.0;
	}
		
	type1 = TYPE_GETTYPE(lwgeom1->type);
	type2 = TYPE_GETTYPE(lwgeom2->type);
	
	/* Point/line combinations can all be handled with simple point array iterations */
	if( ( type1 == POINTTYPE || type1 == LINETYPE ) && 
	    ( type2 == POINTTYPE || type2 == LINETYPE ) )
	{
		POINTARRAY *pa1, *pa2;
		
		if( type1 == POINTTYPE ) 
			pa1 = ((LWPOINT*)lwgeom1)->point;
		else 
			pa1 = ((LWLINE*)lwgeom1)->points;
		
		if( type2 == POINTTYPE )
			pa2 = ((LWPOINT*)lwgeom2)->point;
		else
			pa2 = ((LWLINE*)lwgeom2)->points;
		
		return ptarray_distance_sphere(pa1, pa2, tolerance);
	}
	
	/* Point/Polygon cases, if point-in-poly, return zero, else return distance. */
	if( ( type1 == POLYGONTYPE && type2 == POINTTYPE ) || 
	    ( type2 == POLYGONTYPE && type1 == POINTTYPE ) )
	{
		POINT2D p;
		LWPOLY *lwpoly;
		LWPOINT *lwpt;
		GBOX gbox;
		double distance = MAXFLOAT;
		int i;
		
		if( type1 == POINTTYPE )
		{
			lwpt = (LWPOINT*)lwgeom1;
			lwpoly = (LWPOLY*)lwgeom2;
			gbox = gbox2;
		}
		else
		{
			lwpt = (LWPOINT*)lwgeom2;
			lwpoly = (LWPOLY*)lwgeom1;
			gbox = gbox1;
		}
		getPoint2d_p(lwpt->point, 0, &p);

		/* Point in polygon implies zero distance */
		if( lwpoly_covers_point2d(lwpoly, gbox, p) )
			return 0.0;
		
		/* Not inside, so what's the actual distance? */
		for( i = 0; i < lwpoly->nrings; i++ )
		{
			double ring_distance = ptarray_distance_sphere(lwpoly->rings[i], lwpt->point, tolerance);
			if( ring_distance < distance )
				distance = ring_distance;
			if( distance < tolerance )
				return distance;
		}
		return distance;
	}

	/* Line/polygon case, if start point-in-poly, return zero, else return distance. */
	if( ( type1 == POLYGONTYPE && type2 == LINETYPE ) || 
	    ( type2 == POLYGONTYPE && type1 == LINETYPE ) )
	{
		POINT2D p;
		LWPOLY *lwpoly;
		LWLINE *lwline;
		GBOX gbox;
		double distance = MAXFLOAT;
		int i;
		
		if( type1 == LINETYPE )
		{
			lwline = (LWLINE*)lwgeom1;
			lwpoly = (LWPOLY*)lwgeom2;
			gbox = gbox2;
		}
		else
		{
			lwline = (LWLINE*)lwgeom2;
			lwpoly = (LWPOLY*)lwgeom1;
			gbox = gbox1;
		}
		getPoint2d_p(lwline->points, 0, &p);

		LWDEBUG(4, "checking if a point of line is in polygon");

		/* Point in polygon implies zero distance */
		if( lwpoly_covers_point2d(lwpoly, gbox, p) )
			return 0.0;

		LWDEBUG(4, "checking ring distances");

		/* Not contained, so what's the actual distance? */
		for( i = 0; i < lwpoly->nrings; i++ )
		{
			double ring_distance = ptarray_distance_sphere(lwpoly->rings[i], lwline->points, tolerance);
			LWDEBUGF(4, "ring[%d] ring_distance = %.8g", i, ring_distance);
			if( ring_distance < distance )
				distance = ring_distance;
			if( distance < tolerance )
				return distance;
		}
		LWDEBUGF(4, "all rings checked, returning distance = %.8g", distance);
		return distance;

	}

	/* Polygon/polygon case, if start point-in-poly, return zero, else return distance. */
	if( ( type1 == POLYGONTYPE && type2 == POLYGONTYPE ) || 
	    ( type2 == POLYGONTYPE && type1 == POLYGONTYPE ) )
	{
		POINT2D p;
		LWPOLY *lwpoly1 = (LWPOLY*)lwgeom1;
		LWPOLY *lwpoly2 = (LWPOLY*)lwgeom2;
		double distance = MAXFLOAT;
		int i, j;
		
		/* Point of 2 in polygon 1 implies zero distance */
		getPoint2d_p(lwpoly1->rings[0], 0, &p);
		if( lwpoly_covers_point2d(lwpoly2, gbox2, p) )
			return 0.0;

		/* Point of 1 in polygon 2 implies zero distance */
		getPoint2d_p(lwpoly2->rings[0], 0, &p);
		if( lwpoly_covers_point2d(lwpoly1, gbox1, p) )
			return 0.0;
		
		/* Not contained, so what's the actual distance? */
		for( i = 0; i < lwpoly1->nrings; i++ )
		{
			for( j = 0; j < lwpoly2->nrings; j++ )
			{
				double ring_distance = ptarray_distance_sphere(lwpoly1->rings[i], lwpoly2->rings[j], tolerance);
				if( ring_distance < distance )
					distance = ring_distance;
				if( distance < tolerance )
					return distance;
			}
		}
		return distance;		
	}

	/* Recurse into collections */
	if( lwgeom_contains_subgeoms(type1) )
	{
		int i;
		double distance = MAXFLOAT;
		LWCOLLECTION *col = (LWCOLLECTION*)lwgeom1;

		for( i = 0; i < col->ngeoms; i++ )
		{
			double geom_distance = lwgeom_distance_sphere(col->geoms[i], lwgeom2, gbox1, gbox2, tolerance);
			if( geom_distance < distance )
				distance = geom_distance;
			if( distance < tolerance )
				return distance;
		}
		return distance;
	}

	/* Recurse into collections */
	if( lwgeom_contains_subgeoms(type2) )
	{
		int i;
		double distance = MAXFLOAT;
		LWCOLLECTION *col = (LWCOLLECTION*)lwgeom2;

		for( i = 0; i < col->ngeoms; i++ )
		{
			double geom_distance = lwgeom_distance_sphere(lwgeom1, col->geoms[i], gbox1, gbox2, tolerance);
			if( geom_distance < distance )
				distance = geom_distance;
			if( distance < tolerance )
				return distance;
		}
		return distance;
	}


	lwerror("arguments include unsupported geometry type (%s, %s)", lwgeom_typename(type1), lwgeom_typename(type1));
	return -1.0;
	
}

/**
* Given a polygon (lon/lat decimal degrees) and point (lon/lat decimal degrees) and
* a guaranteed outside point (lon/lat decimal degrees) (calculate with gbox_pt_outside()) 
* return LW_TRUE if point is inside or on edge of polygon.
*/
int lwpoly_covers_point2d(const LWPOLY *poly, GBOX gbox, POINT2D pt_to_test)
{
	int i;
	int in_hole_count = 0;
	POINT3D p, q;
	GEOGRAPHIC_POINT g, gpt_to_test;
	POINT2D pt_outside;
	
	/* Nulls and empties don't contain anything! */
	if( ! poly || lwgeom_is_empty((LWGEOM*)poly) )
	{
		LWDEBUG(4,"returning false, geometry is empty or null");
		return LW_FALSE;
	}

	/* Point not in box? Done! */
	geographic_point_init(pt_to_test.x, pt_to_test.y, &gpt_to_test);
	geog2cart(gpt_to_test, &q);
	if( ! gbox_contains_point3d(gbox, q) )
		return LW_FALSE;
	
	/* Calculate our outside point from the gbox */
	gbox_pt_outside(gbox, &p);
	cart2geog(p, &g);
	pt_outside.x = rad2deg(g.lon);
	pt_outside.y = rad2deg(g.lat);

	LWDEBUGF(4, "pt_outside POINT(%.18g %.18g)", pt_outside.x, pt_outside.y);
	LWDEBUGF(4, "pt_to_test POINT(%.18g %.18g)", pt_to_test.x, pt_to_test.y);
	LWDEBUGF(4, "polygon %s", lwgeom_to_ewkt((LWGEOM*)poly, PARSER_CHECK_NONE));
	LWDEBUGF(4, "gbox %s", gbox_to_string(gbox));

	/* Not in outer ring? We're done! */
	if( ! ptarray_point_in_ring(poly->rings[0], pt_outside, pt_to_test) )
	{
		LWDEBUG(4,"returning false, point is outside ring");
		return LW_FALSE;
	}

	LWDEBUGF(4, "testing %d rings", poly->nrings);
	
	/* But maybe point is in a hole... */
	for( i = 1; i < poly->nrings; i++ )
	{
		LWDEBUGF(4, "ring test loop %d", i);
		/* Count up hole containment. Odd => outside boundary. */
		if( ptarray_point_in_ring(poly->rings[i], pt_outside, pt_to_test) )
			in_hole_count++;
	}

	LWDEBUGF(4, "in_hole_count == %d", in_hole_count);
	
	if( in_hole_count % 2 ) 
	{
		LWDEBUG(4,"returning false, inner ring containment count is odd");
		return LW_FALSE;
	}

	LWDEBUG(4,"returning true, inner ring containment count is even");
	return LW_TRUE;
}


/**
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
		geographic_point_init(in_pt.x, in_pt.y, &gp);
		geog2cart(gp, &out_pt);
		gbox->xmin = gbox->xmax = out_pt.x;
		gbox->ymin = gbox->ymax = out_pt.y;
		gbox->zmin = gbox->zmax = out_pt.z;
		return G_SUCCESS;
	}

	for ( i = 1; i < pa->npoints; i++ )
	{
		getPoint2d_p(pa, i-1, &start_pt);
		geographic_point_init(start_pt.x, start_pt.y, &(edge.start));

		getPoint2d_p(pa, i, &end_pt);
		geographic_point_init(end_pt.x, end_pt.y, &(edge.end));

		edge_calculate_gbox(edge, &edge_gbox);

		LWDEBUGF(4, "edge_gbox: %s", gbox_to_string(&edge_gbox));

		/* Initialize the box */
		if ( first )
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
	if ( ptarray_calculate_gbox_geodetic(point->point, gbox) == G_FAILURE )
		return G_FAILURE;
	return G_SUCCESS;
}

static int lwline_calculate_gbox_geodetic(LWLINE *line, GBOX *gbox)
{
	assert(line);
	if ( ptarray_calculate_gbox_geodetic(line->points, gbox) == G_FAILURE )
		return G_FAILURE;
	return G_SUCCESS;
}

static int lwpolygon_calculate_gbox_geodetic(LWPOLY *poly, GBOX *gbox)
{
	GBOX ringbox;
	int i;
	int first = LW_TRUE;
	assert(poly);
	if ( poly->nrings == 0 )
		return G_FAILURE;
	ringbox.flags = gbox->flags;
	for ( i = 0; i < poly->nrings; i++ )
	{
		if ( ptarray_calculate_gbox_geodetic(poly->rings[i], &ringbox) == G_FAILURE )
			return G_FAILURE;
		if ( first )
		{
			gbox_duplicate(ringbox, gbox);
			first = LW_FALSE;
		}
		else
		{
			gbox_merge(ringbox, gbox);
		}
	}

	/* If the box wraps a poly, push that axis to the absolute min/max as appropriate */
	gbox_check_poles(gbox);

	return G_SUCCESS;
}

static int lwcollection_calculate_gbox_geodetic(LWCOLLECTION *coll, GBOX *gbox)
{
	GBOX subbox;
	int i;
	int result = G_FAILURE;
	int first = LW_TRUE;
	assert(coll);
	if ( coll->ngeoms == 0 )
		return G_FAILURE;

	subbox.flags = gbox->flags;

	for ( i = 0; i < coll->ngeoms; i++ )
	{
		if ( lwgeom_calculate_gbox_geodetic((LWGEOM*)(coll->geoms[i]), &subbox) == G_FAILURE )
		{
			continue;
		}
		else
		{
			if ( first )
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

	for ( i = 0; i < poly->nrings; i++ )
	{
		if ( ptarray_check_geodetic(poly->rings[i]) == LW_FALSE )
			return LW_FALSE;
	}
	return LW_TRUE;
}

static int lwcollection_check_geodetic(LWCOLLECTION *col)
{
	int i = 0;
	assert(col);

	for ( i = 0; i < col->ngeoms; i++ )
	{
		if ( lwgeom_check_geodetic(col->geoms[i]) == LW_FALSE )
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








