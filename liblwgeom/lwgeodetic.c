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
 * Copyright 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright 2009 David Skea <David.Skea@gov.bc.ca>
 *
 **********************************************************************/


#include "liblwgeom_internal.h"
#include "lwgeodetic.h"
#include "lwgeom_log.h"

/**
* For testing geodetic bounding box, we have a magic global variable.
* When this is true (when the cunit tests set it), use the slow, but
* guaranteed correct, algorithm. Otherwise use the regular one.
*/
int gbox_geocentric_slow = LW_FALSE;

/**
* Utility function for ptarray_contains_point_sphere()
*/
static int
point3d_equals(const POINT3D *p1, const POINT3D *p2)
{
	return FP_EQUALS(p1->x, p2->x) && FP_EQUALS(p1->y, p2->y) && FP_EQUALS(p1->z, p2->z);
}

/**
* Convert a longitude to the range of -PI,PI
*/
double longitude_radians_normalize(double lon)
{
	if ( lon == -1.0 * M_PI )
		return M_PI;
	if ( lon == -2.0 * M_PI )
		return 0.0;

	if ( lon > 2.0 * M_PI )
		lon = remainder(lon, 2.0 * M_PI);

	if ( lon < -2.0 * M_PI )
		lon = remainder(lon, -2.0 * M_PI);

	if ( lon > M_PI )
		lon = -2.0 * M_PI + lon;

	if ( lon < -1.0 * M_PI )
		lon = 2.0 * M_PI + lon;

	if ( lon == -2.0 * M_PI )
		lon *= -1.0;

	return lon;
}

/**
* Convert a latitude to the range of -PI/2,PI/2
*/
double latitude_radians_normalize(double lat)
{

	if ( lat > 2.0 * M_PI )
		lat = remainder(lat, 2.0 * M_PI);

	if ( lat < -2.0 * M_PI )
		lat = remainder(lat, -2.0 * M_PI);

	if ( lat > M_PI )
		lat = M_PI - lat;

	if ( lat < -1.0 * M_PI )
		lat = -1.0 * M_PI - lat;

	if ( lat > M_PI_2 )
		lat = M_PI - lat;

	if ( lat < -1.0 * M_PI_2 )
		lat = -1.0 * M_PI - lat;

	return lat;
}

/**
* Convert a longitude to the range of -180,180
* @param lon longitude in degrees
*/
double longitude_degrees_normalize(double lon)
{
	if ( lon > 360.0 )
		lon = remainder(lon, 360.0);

	if ( lon < -360.0 )
		lon = remainder(lon, -360.0);

	if ( lon > 180.0 )
		lon = -360.0 + lon;

	if ( lon < -180.0 )
		lon = 360 + lon;

	if ( lon == -180.0 )
		return 180.0;

	if ( lon == -360.0 )
		return 0.0;

	return lon;
}

/**
* Convert a latitude to the range of -90,90
* @param lat latitude in degrees
*/
double latitude_degrees_normalize(double lat)
{

	if ( lat > 360.0 )
		lat = remainder(lat, 360.0);

	if ( lat < -360.0 )
		lat = remainder(lat, -360.0);

	if ( lat > 180.0 )
		lat = 180.0 - lat;

	if ( lat < -180.0 )
		lat = -180.0 - lat;

	if ( lat > 90.0 )
		lat = 180.0 - lat;

	if ( lat < -90.0 )
		lat = -180.0 - lat;

	return lat;
}

/**
* Shift a point around by a number of radians
*/
void point_shift(GEOGRAPHIC_POINT *p, double shift)
{
	double lon = p->lon + shift;
	if ( lon > M_PI )
		p->lon = -1.0 * M_PI + (lon - M_PI);
	else
		p->lon = lon;
	return;
}

int geographic_point_equals(const GEOGRAPHIC_POINT *g1, const GEOGRAPHIC_POINT *g2)
{
	return FP_EQUALS(g1->lat, g2->lat) && FP_EQUALS(g1->lon, g2->lon);
}

/**
* Initialize a geographic point
* @param lon longitude in degrees
* @param lat latitude in degrees
*/
void geographic_point_init(double lon, double lat, GEOGRAPHIC_POINT *g)
{
	g->lat = latitude_radians_normalize(deg2rad(lat));
	g->lon = longitude_radians_normalize(deg2rad(lon));
}

/** Returns the angular height (latitudinal span) of the box in radians */
double
gbox_angular_height(const GBOX* gbox)
{
	double d[6];
	int i;
	double zmin = FLT_MAX;
	double zmax = -1 * FLT_MAX;
	POINT3D pt;

	/* Take a copy of the box corners so we can treat them as a list */
	/* Elements are xmin, xmax, ymin, ymax, zmin, zmax */
	memcpy(d, &(gbox->xmin), 6*sizeof(double));

	/* Generate all 8 corner vectors of the box */
	for ( i = 0; i < 8; i++ )
	{
		pt.x = d[i / 4];
		pt.y = d[2 + (i % 4) / 2];
		pt.z = d[4 + (i % 2)];
		normalize(&pt);
		if ( pt.z < zmin ) zmin = pt.z;
		if ( pt.z > zmax ) zmax = pt.z;
	}
	return asin(zmax) - asin(zmin);
}

/** Returns the angular width (longitudinal span) of the box in radians */
double
gbox_angular_width(const GBOX* gbox)
{
	double d[6];
	int i, j;
	POINT3D pt[3];
	double maxangle;
	double magnitude;

	/* Take a copy of the box corners so we can treat them as a list */
	/* Elements are xmin, xmax, ymin, ymax, zmin, zmax */
	memcpy(d, &(gbox->xmin), 6*sizeof(double));

	/* Start with the bottom corner */
	pt[0].x = gbox->xmin;
	pt[0].y = gbox->ymin;
	magnitude = sqrt(pt[0].x*pt[0].x + pt[0].y*pt[0].y);
	pt[0].x /= magnitude;
	pt[0].y /= magnitude;

	/* Generate all 8 corner vectors of the box */
	/* Find the vector furthest from our seed vector */
	for ( j = 0; j < 2; j++ )
	{
		maxangle = -1 * FLT_MAX;
		for ( i = 0; i < 4; i++ )
		{
			double angle, dotprod;
			POINT3D pt_n;

			pt_n.x = d[i / 2];
			pt_n.y = d[2 + (i % 2)];
			magnitude = sqrt(pt_n.x*pt_n.x + pt_n.y*pt_n.y);
			pt_n.x /= magnitude;
			pt_n.y /= magnitude;
			pt_n.z = 0.0;

			dotprod = pt_n.x*pt[j].x + pt_n.y*pt[j].y;
			angle = acos(dotprod > 1.0 ? 1.0 : dotprod);
			if ( angle > maxangle )
			{
				pt[j+1] = pt_n;
				maxangle = angle;
			}
		}
	}

	/* Return the distance between the two furthest vectors */
	return maxangle;
}

/** Computes the average(ish) center of the box and returns success. */
int
gbox_centroid(const GBOX* gbox, POINT2D* out)
{
	double d[6];
	GEOGRAPHIC_POINT g;
	POINT3D pt;
	int i;

	/* Take a copy of the box corners so we can treat them as a list */
	/* Elements are xmin, xmax, ymin, ymax, zmin, zmax */
	memcpy(d, &(gbox->xmin), 6*sizeof(double));

	/* Zero out our return vector */
	pt.x = pt.y = pt.z = 0.0;

	for ( i = 0; i < 8; i++ )
	{
		POINT3D pt_n;

		pt_n.x = d[i / 4];
		pt_n.y = d[2 + ((i % 4) / 2)];
		pt_n.z = d[4 + (i % 2)];
		normalize(&pt_n);

		pt.x += pt_n.x;
		pt.y += pt_n.y;
		pt.z += pt_n.z;
	}

	pt.x /= 8.0;
	pt.y /= 8.0;
	pt.z /= 8.0;
	normalize(&pt);

	cart2geog(&pt, &g);
	out->x = longitude_degrees_normalize(rad2deg(g.lon));
	out->y = latitude_degrees_normalize(rad2deg(g.lat));

	return LW_SUCCESS;
}

/**
* Check to see if this geocentric gbox is wrapped around a pole.
* Only makes sense if this gbox originated from a polygon, as it's assuming
* the box is generated from external edges and there's an "interior" which
* contains the pole.
*
* This function is overdetermined, for very large polygons it might add an
* unwarranted pole. STILL NEEDS WORK!
*/
static int gbox_check_poles(GBOX *gbox)
{
	int rv = LW_FALSE;
#if POSTGIS_DEBUG_LEVEL >= 4
	char *gbox_str = gbox_to_string(gbox);
	LWDEBUG(4, "checking poles");
	LWDEBUGF(4, "gbox %s", gbox_str);
	lwfree(gbox_str);
#endif
	/* Z axis */
	if (gbox->xmin < 0.0 && gbox->xmax > 0.0 &&
	    gbox->ymin < 0.0 && gbox->ymax > 0.0)
	{
		/* Extrema lean positive */
		if ((gbox->zmin > 0.0) && (gbox->zmax > 0.0))
		{
			LWDEBUG(4, "enclosed positive z axis");
			gbox->zmax = 1.0;
		}
		/* Extrema lean negative */
		else if ((gbox->zmin < 0.0) && (gbox->zmax < 0.0))
		{
			LWDEBUG(4, "enclosed negative z axis");
			gbox->zmin = -1.0;
		}
		/* Extrema both sides! */
		else
		{
			LWDEBUG(4, "enclosed both z axes");
			gbox->zmin = -1.0;
			gbox->zmax = 1.0;
		}
		rv = LW_TRUE;
	}

	/* Y axis */
	if (gbox->xmin < 0.0 && gbox->xmax > 0.0 &&
	    gbox->zmin < 0.0 && gbox->zmax > 0.0)
	{
		if ((gbox->ymin > 0.0) && (gbox->ymax > 0.0))
		{
			LWDEBUG(4, "enclosed positive y axis");
			gbox->ymax = 1.0;
		}
		else if ((gbox->ymin < 0.0) && (gbox->ymax < 0.0))
		{
			LWDEBUG(4, "enclosed negative y axis");
			gbox->ymin = -1.0;
		}
		else
		{
			LWDEBUG(4, "enclosed both y axes");
			gbox->ymax = 1.0;
			gbox->ymin = -1.0;
		}
		rv = LW_TRUE;
	}

	/* X axis */
	if (gbox->ymin < 0.0 && gbox->ymax > 0.0 &&
	    gbox->zmin < 0.0 && gbox->zmax > 0.0)
	{
		if ((gbox->xmin > 0.0) && (gbox->xmax > 0.0))
		{
			LWDEBUG(4, "enclosed positive x axis");
			gbox->xmax = 1.0;
		}
		else if ((gbox->xmin < 0.0) && (gbox->xmax < 0.0))
		{
			LWDEBUG(4, "enclosed negative x axis");
			gbox->xmin = -1.0;
		}
		else
		{
			LWDEBUG(4, "enclosed both x axes");
			gbox->xmax = 1.0;
			gbox->xmin = -1.0;
		}

		rv = LW_TRUE;
	}

	return rv;
}

/**
* Convert spherical coordinates to cartesian coordinates on unit sphere
*/
void geog2cart(const GEOGRAPHIC_POINT *g, POINT3D *p)
{
	p->x = cos(g->lat) * cos(g->lon);
	p->y = cos(g->lat) * sin(g->lon);
	p->z = sin(g->lat);
}

/**
* Convert cartesian coordinates on unit sphere to spherical coordinates
*/
void cart2geog(const POINT3D *p, GEOGRAPHIC_POINT *g)
{
	g->lon = atan2(p->y, p->x);
	g->lat = asin(p->z);
}

/**
* Convert lon/lat coordinates to cartesian coordinates on unit sphere
*/
void ll2cart(const POINT2D *g, POINT3D *p)
{
	double x_rad = M_PI * g->x / 180.0;
	double y_rad = M_PI * g->y / 180.0;
	double cos_y_rad = cos(y_rad);
	p->x = cos_y_rad * cos(x_rad);
	p->y = cos_y_rad * sin(x_rad);
	p->z = sin(y_rad);
}

/**
* Convert cartesian coordinates on unit sphere to lon/lat coordinates
static void cart2ll(const POINT3D *p, POINT2D *g)
{
	g->x = longitude_degrees_normalize(180.0 * atan2(p->y, p->x) / M_PI);
	g->y = latitude_degrees_normalize(180.0 * asin(p->z) / M_PI);
}
*/

/**
* Calculate the dot product of two unit vectors
* (-1 == opposite, 0 == orthogonal, 1 == identical)
*/
static double dot_product(const POINT3D *p1, const POINT3D *p2)
{
	return (p1->x*p2->x) + (p1->y*p2->y) + (p1->z*p2->z);
}

/**
* Calculate the cross product of two vectors
*/
static void cross_product(const POINT3D *a, const POINT3D *b, POINT3D *n)
{
	n->x = a->y * b->z - a->z * b->y;
	n->y = a->z * b->x - a->x * b->z;
	n->z = a->x * b->y - a->y * b->x;
	return;
}

/**
* Calculate the sum of two vectors
*/
void vector_sum(const POINT3D *a, const POINT3D *b, POINT3D *n)
{
	n->x = a->x + b->x;
	n->y = a->y + b->y;
	n->z = a->z + b->z;
	return;
}

/**
* Calculate the difference of two vectors
*/
static void vector_difference(const POINT3D *a, const POINT3D *b, POINT3D *n)
{
	n->x = a->x - b->x;
	n->y = a->y - b->y;
	n->z = a->z - b->z;
	return;
}

/**
* Scale a vector out by a factor
*/
void vector_scale(POINT3D *n, double scale)
{
	n->x *= scale;
	n->y *= scale;
	n->z *= scale;
	return;
}

/*
* static inline double vector_magnitude(const POINT3D* v)
* {
*	return sqrt(v->x*v->x + v->y*v->y + v->z*v->z);
* }
*/

/**
* Angle between two unit vectors
*/
double vector_angle(const POINT3D* v1, const POINT3D* v2)
{
	POINT3D v3, normal;
	double angle, x, y;

	cross_product(v1, v2, &normal);
	normalize(&normal);
	cross_product(&normal, v1, &v3);

	x = dot_product(v1, v2);
	y = dot_product(v2, &v3);

	angle = atan2(y, x);
	return angle;
}

/**
* Normalize to a unit vector.
*/
static void normalize2d(POINT2D *p)
{
	double d = sqrt(p->x*p->x + p->y*p->y);
	if (FP_IS_ZERO(d))
	{
		p->x = p->y = 0.0;
		return;
	}
	p->x = p->x / d;
	p->y = p->y / d;
	return;
}

/**
* Calculates the unit normal to two vectors, trying to avoid
* problems with over-narrow or over-wide cases.
*/
void unit_normal(const POINT3D *P1, const POINT3D *P2, POINT3D *normal)
{
	double p_dot = dot_product(P1, P2);
	POINT3D P3;

	/* If edge is really large, calculate a narrower equivalent angle A1/A3. */
	if ( p_dot < 0 )
	{
		vector_sum(P1, P2, &P3);
		normalize(&P3);
	}
	/* If edge is narrow, calculate a wider equivalent angle A1/A3. */
	else if ( p_dot > 0.95 )
	{
		vector_difference(P2, P1, &P3);
		normalize(&P3);
	}
	/* Just keep the current angle in A1/A3. */
	else
	{
		P3 = *P2;
	}

	/* Normals to the A-plane and B-plane */
	cross_product(P1, &P3, normal);
	normalize(normal);
}

/**
* Rotates v1 through an angle (in radians) within the plane defined by v1/v2, returns
* the rotated vector in n.
*/
void vector_rotate(const POINT3D* v1, const POINT3D* v2, double angle, POINT3D* n)
{
	POINT3D u;
	double cos_a = cos(angle);
	double sin_a = sin(angle);
	double uxuy, uyuz, uxuz;
	double ux2, uy2, uz2;
	double rxx, rxy, rxz, ryx, ryy, ryz, rzx, rzy, rzz;

	/* Need a unit vector normal to rotate around */
	unit_normal(v1, v2, &u);

	uxuy = u.x * u.y;
	uxuz = u.x * u.z;
	uyuz = u.y * u.z;

	ux2 = u.x * u.x;
	uy2 = u.y * u.y;
	uz2 = u.z * u.z;

	rxx = cos_a + ux2 * (1 - cos_a);
	rxy = uxuy * (1 - cos_a) - u.z * sin_a;
	rxz = uxuz * (1 - cos_a) + u.y * sin_a;

	ryx = uxuy * (1 - cos_a) + u.z * sin_a;
	ryy = cos_a + uy2 * (1 - cos_a);
	ryz = uyuz * (1 - cos_a) - u.x * sin_a;

	rzx = uxuz * (1 - cos_a) - u.y * sin_a;
	rzy = uyuz * (1 - cos_a) + u.x * sin_a;
	rzz = cos_a + uz2 * (1 - cos_a);

	n->x = rxx * v1->x + rxy * v1->y + rxz * v1->z;
	n->y = ryx * v1->x + ryy * v1->y + ryz * v1->z;
	n->z = rzx * v1->x + rzy * v1->y + rzz * v1->z;

	normalize(n);
}

/**
* Normalize to a unit vector.
*/
void normalize(POINT3D *p)
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


/**
* Computes the cross product of two vectors using their lat, lng representations.
* Good even for small distances between p and q.
*/
void robust_cross_product(const GEOGRAPHIC_POINT *p, const GEOGRAPHIC_POINT *q, POINT3D *a)
{
	double lon_qpp = (q->lon + p->lon) / -2.0;
	double lon_qmp = (q->lon - p->lon) / 2.0;
	double sin_p_lat_minus_q_lat = sin(p->lat-q->lat);
	double sin_p_lat_plus_q_lat = sin(p->lat+q->lat);
	double sin_lon_qpp = sin(lon_qpp);
	double sin_lon_qmp = sin(lon_qmp);
	double cos_lon_qpp = cos(lon_qpp);
	double cos_lon_qmp = cos(lon_qmp);
	a->x = sin_p_lat_minus_q_lat * sin_lon_qpp * cos_lon_qmp -
	       sin_p_lat_plus_q_lat * cos_lon_qpp * sin_lon_qmp;
	a->y = sin_p_lat_minus_q_lat * cos_lon_qpp * cos_lon_qmp +
	       sin_p_lat_plus_q_lat * sin_lon_qpp * sin_lon_qmp;
	a->z = cos(p->lat) * cos(q->lat) * sin(q->lon-p->lon);
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


int crosses_dateline(const GEOGRAPHIC_POINT *s, const GEOGRAPHIC_POINT *e)
{
	double sign_s = SIGNUM(s->lon);
	double sign_e = SIGNUM(e->lon);
	double ss = fabs(s->lon);
	double ee = fabs(e->lon);
	if ( sign_s == sign_e )
	{
		return LW_FALSE;
	}
	else
	{
		double dl = ss + ee;
		if ( dl < M_PI )
			return LW_FALSE;
		else if ( FP_EQUALS(dl, M_PI) )
			return LW_FALSE;
		else
			return LW_TRUE;
	}
}

/**
* Returns -1 if the point is to the left of the plane formed
* by the edge, 1 if the point is to the right, and 0 if the
* point is on the plane.
*/
static int
edge_point_side(const GEOGRAPHIC_EDGE *e, const GEOGRAPHIC_POINT *p)
{
	POINT3D normal, pt;
	double w;
	/* Normal to the plane defined by e */
	robust_cross_product(&(e->start), &(e->end), &normal);
	normalize(&normal);
	geog2cart(p, &pt);
	/* We expect the dot product of with normal with any vector in the plane to be zero */
	w = dot_product(&normal, &pt);
	LWDEBUGF(4,"dot product %.9g",w);
	if ( FP_IS_ZERO(w) )
	{
		LWDEBUG(4, "point is on plane (dot product is zero)");
		return 0;
	}

	if ( w < 0 )
		return -1;
	else
		return 1;
}

/**
* Returns true if the point p is on the great circle plane.
* Forms the scalar triple product of A,B,p and if the volume of the
* resulting parallelepiped is near zero the point p is on the
* great circle plane.
*/
int edge_point_on_plane(const GEOGRAPHIC_EDGE *e, const GEOGRAPHIC_POINT *p)
{
	int side = edge_point_side(e, p);
	if ( side == 0 )
		return LW_TRUE;

	return LW_FALSE;
}

/**
* Returns true if the point p is inside the cone defined by the
* two ends of the edge e.
*/
int edge_point_in_cone(const GEOGRAPHIC_EDGE *e, const GEOGRAPHIC_POINT *p)
{
	POINT3D vcp, vs, ve, vp;
	double vs_dot_vcp, vp_dot_vcp;
	geog2cart(&(e->start), &vs);
	geog2cart(&(e->end), &ve);
	/* Antipodal case, everything is inside. */
	if ( vs.x == -1.0 * ve.x && vs.y == -1.0 * ve.y && vs.z == -1.0 * ve.z )
		return LW_TRUE;
	geog2cart(p, &vp);
	/* The normalized sum bisects the angle between start and end. */
	vector_sum(&vs, &ve, &vcp);
	normalize(&vcp);
	/* The projection of start onto the center defines the minimum similarity */
	vs_dot_vcp = dot_product(&vs, &vcp);
	LWDEBUGF(4,"vs_dot_vcp %.19g",vs_dot_vcp);
	/* The projection of candidate p onto the center */
	vp_dot_vcp = dot_product(&vp, &vcp);
	LWDEBUGF(4,"vp_dot_vcp %.19g",vp_dot_vcp);
	/* If p is more similar than start then p is inside the cone */
	LWDEBUGF(4,"fabs(vp_dot_vcp - vs_dot_vcp) %.39g",fabs(vp_dot_vcp - vs_dot_vcp));

	/*
	** We want to test that vp_dot_vcp is >= vs_dot_vcp but there are
	** numerical stability issues for values that are very very nearly
	** equal. Unfortunately there are also values of vp_dot_vcp that are legitimately
	** very close to but still less than vs_dot_vcp which we also need to catch.
	** The tolerance of 10-17 seems to do the trick on 32-bit and 64-bit architectures,
	** for the test cases here.
	** However, tuning the tolerance value feels like a dangerous hack.
	** Fundamentally, the problem is that this test is so sensitive.
	*/

	/* 1.1102230246251565404236316680908203125e-16 */

	if ( vp_dot_vcp > vs_dot_vcp || fabs(vp_dot_vcp - vs_dot_vcp) < 2e-16 )
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
int edge_contains_coplanar_point(const GEOGRAPHIC_EDGE *e, const GEOGRAPHIC_POINT *p)
{
	GEOGRAPHIC_EDGE g;
	GEOGRAPHIC_POINT q;
	double slon = fabs((e->start).lon) + fabs((e->end).lon);
	double dlon = fabs(fabs((e->start).lon) - fabs((e->end).lon));
	double slat = (e->start).lat + (e->end).lat;

	LWDEBUGF(4, "e.start == GPOINT(%.6g %.6g) ", (e->start).lat, (e->start).lon);
	LWDEBUGF(4, "e.end == GPOINT(%.6g %.6g) ", (e->end).lat, (e->end).lon);
	LWDEBUGF(4, "p == GPOINT(%.6g %.6g) ", p->lat, p->lon);

	/* Copy values into working registers */
	g = *e;
	q = *p;

	/* Vertical plane, we need to do this calculation in latitude */
	if ( FP_EQUALS( g.start.lon, g.end.lon ) )
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
	if ( FP_EQUALS( slon, M_PI ) && ( SIGNUM(g.start.lon) != SIGNUM(g.end.lon) || FP_EQUALS(dlon, M_PI) ) )
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
			if ( q.lat > FP_MIN(g.start.lat, g.end.lat) )
				return LW_TRUE;
			else
				return LW_FALSE;
		}
		else
			/* Over south pole, test based on north pole */
		{
			LWDEBUG(4, "over the south pole...");
			if ( q.lat < FP_MAX(g.start.lat, g.end.lat) )
				return LW_TRUE;
			else
				return LW_FALSE;
		}
	}

	/* Dateline crossing, flip everything to the opposite hemisphere */
	else if ( slon > M_PI && ( SIGNUM(g.start.lon) != SIGNUM(g.end.lon) ) )
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
double sphere_distance(const GEOGRAPHIC_POINT *s, const GEOGRAPHIC_POINT *e)
{
	double d_lon, cos_d_lon, cos_lat_e, sin_lat_e, cos_lat_s, sin_lat_s;
	double a1, a2, a, b;

	if (FP_EQUALS(s->lat, e->lat) && FP_EQUALS(s->lon, e->lon)) return 0.0;
	d_lon = e->lon - s->lon;
	cos_d_lon = cos(d_lon);
	cos_lat_e = cos(e->lat);
	sin_lat_e = sin(e->lat);
	cos_lat_s = cos(s->lat);
	sin_lat_s = sin(s->lat);

	a1 = POW2(cos_lat_e * sin(d_lon));
	a2 = POW2(cos_lat_s * sin_lat_e - sin_lat_s * cos_lat_e * cos_d_lon);
	a = sqrt(a1 + a2);
	b = sin_lat_s * sin_lat_e + cos_lat_s * cos_lat_e * cos_d_lon;
	return atan2(a, b);
}

/**
* Given two unit vectors, calculate their distance apart in radians.
*/
double sphere_distance_cartesian(const POINT3D *s, const POINT3D *e)
{
	return acos(FP_MIN(1.0, dot_product(s, e)));
}

/**
* Given two points on a unit sphere, calculate the direction from s to e.
*/
double sphere_direction(const GEOGRAPHIC_POINT *s, const GEOGRAPHIC_POINT *e, double d)
{
	double heading = 0.0;
	double f;

	/* Starting from the poles? Special case. */
	if ( FP_IS_ZERO(cos(s->lat)) )
		return (s->lat > 0.0) ? M_PI : 0.0;

	f = (sin(e->lat) - sin(s->lat) * cos(d)) / (sin(d) * cos(s->lat));
	if ( FP_EQUALS(f, 1.0) )
		heading = 0.0;
	else if ( FP_EQUALS(f, -1.0) )
		heading = M_PI;
	else if ( fabs(f) > 1.0 )
	{
		LWDEBUGF(4, "f = %g", f);
		heading = acos(f);
	}
	else
		heading = acos(f);

	if ( sin(e->lon - s->lon) < 0.0 )
		heading = -1 * heading;

	return heading;
}

#if 0 /* unused */
/**
* Computes the spherical excess of a spherical triangle defined by
* the three vertices A, B, C. Computes on the unit sphere (i.e., divides
* edge lengths by the radius, even if the radius is 1.0). The excess is
* signed based on the sign of the delta longitude of A and B.
*
* @param a The first triangle vertex.
* @param b The second triangle vertex.
* @param c The last triangle vertex.
* @return the signed spherical excess.
*/
static double sphere_excess(const GEOGRAPHIC_POINT *a, const GEOGRAPHIC_POINT *b, const GEOGRAPHIC_POINT *c)
{
	double a_dist = sphere_distance(b, c);
	double b_dist = sphere_distance(c, a);
	double c_dist = sphere_distance(a, b);
	double hca = sphere_direction(c, a, b_dist);
	double hcb = sphere_direction(c, b, a_dist);
	double sign = SIGNUM(hcb-hca);
	double ss = (a_dist + b_dist + c_dist) / 2.0;
	double E = tan(ss/2.0)*tan((ss-a_dist)/2.0)*tan((ss-b_dist)/2.0)*tan((ss-c_dist)/2.0);
	return 4.0 * atan(sqrt(fabs(E))) * sign;
}
#endif


/**
* Returns true if the point p is on the minor edge defined by the
* end points of e.
*/
int edge_contains_point(const GEOGRAPHIC_EDGE *e, const GEOGRAPHIC_POINT *p)
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
	double sign = SIGNUM(z);
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
* the great circle with the line of maximum/minimum gradient that lies on
* the great circle plane.
*/
int clairaut_cartesian(const POINT3D *start, const POINT3D *end, GEOGRAPHIC_POINT *g_top, GEOGRAPHIC_POINT *g_bottom)
{
	POINT3D t1, t2;
	GEOGRAPHIC_POINT vN1, vN2;
	LWDEBUG(4,"entering function");
	unit_normal(start, end, &t1);
	unit_normal(end, start, &t2);
	LWDEBUGF(4, "unit normal t1 == POINT(%.8g %.8g %.8g)", t1.x, t1.y, t1.z);
	LWDEBUGF(4, "unit normal t2 == POINT(%.8g %.8g %.8g)", t2.x, t2.y, t2.z);
	cart2geog(&t1, &vN1);
	cart2geog(&t2, &vN2);
	g_top->lat = z_to_latitude(t1.z,LW_TRUE);
	g_top->lon = vN2.lon;
	g_bottom->lat = z_to_latitude(t2.z,LW_FALSE);
	g_bottom->lon = vN1.lon;
	LWDEBUGF(4, "clairaut top == GPOINT(%.6g %.6g)", g_top->lat, g_top->lon);
	LWDEBUGF(4, "clairaut bottom == GPOINT(%.6g %.6g)", g_bottom->lat, g_bottom->lon);
	return LW_SUCCESS;
}

/**
* Computes the pole of the great circle disk which is the intersection of
* the great circle with the line of maximum/minimum gradient that lies on
* the great circle plane.
*/
int clairaut_geographic(const GEOGRAPHIC_POINT *start, const GEOGRAPHIC_POINT *end, GEOGRAPHIC_POINT *g_top, GEOGRAPHIC_POINT *g_bottom)
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
	cart2geog(&t1, &vN1);
	cart2geog(&t2, &vN2);
	g_top->lat = z_to_latitude(t1.z,LW_TRUE);
	g_top->lon = vN2.lon;
	g_bottom->lat = z_to_latitude(t2.z,LW_FALSE);
	g_bottom->lon = vN1.lon;
	LWDEBUGF(4, "clairaut top == GPOINT(%.6g %.6g)", g_top->lat, g_top->lon);
	LWDEBUGF(4, "clairaut bottom == GPOINT(%.6g %.6g)", g_bottom->lat, g_bottom->lon);
	return LW_SUCCESS;
}

/**
* Returns true if an intersection can be calculated, and places it in *g.
* Returns false otherwise.
*/
int edge_intersection(const GEOGRAPHIC_EDGE *e1, const GEOGRAPHIC_EDGE *e2, GEOGRAPHIC_POINT *g)
{
	POINT3D ea, eb, v;
	LWDEBUGF(4, "e1 start(%.20g %.20g) end(%.20g %.20g)", e1->start.lat, e1->start.lon, e1->end.lat, e1->end.lon);
	LWDEBUGF(4, "e2 start(%.20g %.20g) end(%.20g %.20g)", e2->start.lat, e2->start.lon, e2->end.lat, e2->end.lon);

	LWDEBUGF(4, "e1 start(%.20g %.20g) end(%.20g %.20g)", rad2deg(e1->start.lon), rad2deg(e1->start.lat), rad2deg(e1->end.lon), rad2deg(e1->end.lat));
	LWDEBUGF(4, "e2 start(%.20g %.20g) end(%.20g %.20g)", rad2deg(e2->start.lon), rad2deg(e2->start.lat), rad2deg(e2->end.lon), rad2deg(e2->end.lat));

	if ( geographic_point_equals(&(e1->start), &(e2->start)) )
	{
		*g = e1->start;
		return LW_TRUE;
	}
	if ( geographic_point_equals(&(e1->end), &(e2->end)) )
	{
		*g = e1->end;
		return LW_TRUE;
	}
	if ( geographic_point_equals(&(e1->end), &(e2->start)) )
	{
		*g = e1->end;
		return LW_TRUE;
	}
	if ( geographic_point_equals(&(e1->start), &(e2->end)) )
	{
		*g = e1->start;
		return LW_TRUE;
	}

	robust_cross_product(&(e1->start), &(e1->end), &ea);
	normalize(&ea);
	robust_cross_product(&(e2->start), &(e2->end), &eb);
	normalize(&eb);
	LWDEBUGF(4, "e1 cross product == POINT(%.12g %.12g %.12g)", ea.x, ea.y, ea.z);
	LWDEBUGF(4, "e2 cross product == POINT(%.12g %.12g %.12g)", eb.x, eb.y, eb.z);
	LWDEBUGF(4, "fabs(dot_product(ea, eb)) == %.14g", fabs(dot_product(&ea, &eb)));
	if ( FP_EQUALS(fabs(dot_product(&ea, &eb)), 1.0) )
	{
		LWDEBUGF(4, "parallel edges found! dot_product = %.12g", dot_product(&ea, &eb));
		/* Parallel (maybe equal) edges! */
		/* Hack alert, only returning ONE end of the edge right now, most do better later. */
		/* Hack alert #2, returning a value of 2 to indicate a co-linear crossing event. */
		if ( edge_contains_point(e1, &(e2->start)) )
		{
			*g = e2->start;
			return 2;
		}
		if ( edge_contains_point(e1, &(e2->end)) )
		{
			*g = e2->end;
			return 2;
		}
		if ( edge_contains_point(e2, &(e1->start)) )
		{
			*g = e1->start;
			return 2;
		}
		if ( edge_contains_point(e2, &(e1->end)) )
		{
			*g = e1->end;
			return 2;
		}
	}
	unit_normal(&ea, &eb, &v);
	LWDEBUGF(4, "v == POINT(%.12g %.12g %.12g)", v.x, v.y, v.z);
	g->lat = atan2(v.z, sqrt(v.x * v.x + v.y * v.y));
	g->lon = atan2(v.y, v.x);
	LWDEBUGF(4, "g == GPOINT(%.12g %.12g)", g->lat, g->lon);
	LWDEBUGF(4, "g == POINT(%.12g %.12g)", rad2deg(g->lon), rad2deg(g->lat));
	if ( edge_contains_point(e1, g) && edge_contains_point(e2, g) )
	{
		return LW_TRUE;
	}
	else
	{
		LWDEBUG(4, "flipping point to other side of sphere");
		g->lat = -1.0 * g->lat;
		g->lon = g->lon + M_PI;
		if ( g->lon > M_PI )
		{
			g->lon = -1.0 * (2.0 * M_PI - g->lon);
		}
		if ( edge_contains_point(e1, g) && edge_contains_point(e2, g) )
		{
			return LW_TRUE;
		}
	}
	return LW_FALSE;
}

double edge_distance_to_point(const GEOGRAPHIC_EDGE *e, const GEOGRAPHIC_POINT *gp, GEOGRAPHIC_POINT *closest)
{
	double d1 = 1000000000.0, d2, d3, d_nearest;
	POINT3D n, p, k;
	GEOGRAPHIC_POINT gk, g_nearest;

	/* Zero length edge, */
	if ( geographic_point_equals(&(e->start), &(e->end)) )
	{
		if (closest)
			*closest = e->start;

		return sphere_distance(&(e->start), gp);
	}

	robust_cross_product(&(e->start), &(e->end), &n);
	normalize(&n);
	geog2cart(gp, &p);
	vector_scale(&n, dot_product(&p, &n));
	vector_difference(&p, &n, &k);
	normalize(&k);
	cart2geog(&k, &gk);
	if ( edge_point_in_cone(e, &gk) )
	{
		d1 = sphere_distance(gp, &gk);
	}
	d2 = sphere_distance(gp, &(e->start));
	d3 = sphere_distance(gp, &(e->end));

	d_nearest = d1;
	g_nearest = gk;

	if ( d2 < d_nearest )
	{
		d_nearest = d2;
		g_nearest = e->start;
	}
	if ( d3 < d_nearest )
	{
		d_nearest = d3;
		g_nearest = e->end;
	}
	if (closest)
		*closest = g_nearest;

	return d_nearest;
}

/**
* Calculate the distance between two edges.
* IMPORTANT: this test does not check for edge intersection!!! (distance == 0)
* You have to check for intersection before calling this function.
*/
double edge_distance_to_edge(const GEOGRAPHIC_EDGE *e1, const GEOGRAPHIC_EDGE *e2, GEOGRAPHIC_POINT *closest1, GEOGRAPHIC_POINT *closest2)
{
	double d;
	GEOGRAPHIC_POINT gcp1s, gcp1e, gcp2s, gcp2e, c1, c2;
	double d1s = edge_distance_to_point(e1, &(e2->start), &gcp1s);
	double d1e = edge_distance_to_point(e1, &(e2->end), &gcp1e);
	double d2s = edge_distance_to_point(e2, &(e1->start), &gcp2s);
	double d2e = edge_distance_to_point(e2, &(e1->end), &gcp2e);

	d = d1s;
	c1 = gcp1s;
	c2 = e2->start;

	if ( d1e < d )
	{
		d = d1e;
		c1 = gcp1e;
		c2 = e2->end;
	}

	if ( d2s < d )
	{
		d = d2s;
		c1 = e1->start;
		c2 = gcp2s;
	}

	if ( d2e < d )
	{
		d = d2e;
		c1 = e1->end;
		c2 = gcp2e;
	}

	if ( closest1 ) *closest1 = c1;
	if ( closest2 ) *closest2 = c2;

	return d;
}


/**
* Given a starting location r, a distance and an azimuth
* to the new point, compute the location of the projected point on the unit sphere.
*/
int sphere_project(const GEOGRAPHIC_POINT *r, double distance, double azimuth, GEOGRAPHIC_POINT *n)
{
	double d = distance;
	double lat1 = r->lat;
	double lon1 = r->lon;
	double lat2, lon2;

	lat2 = asin(sin(lat1)*cos(d) + cos(lat1)*sin(d)*cos(azimuth));

	/* If we're going straight up or straight down, we don't need to calculate the longitude */
	/* TODO: this isn't quite true, what if we're going over the pole? */
	if ( FP_EQUALS(azimuth, M_PI) || FP_EQUALS(azimuth, 0.0) )
	{
		lon2 = r->lon;
	}
	else
	{
		lon2 = lon1 + atan2(sin(azimuth)*sin(d)*cos(lat1), cos(d)-sin(lat1)*sin(lat2));
	}

	if ( isnan(lat2) || isnan(lon2) )
		return LW_FAILURE;

	n->lat = lat2;
	n->lon = lon2;

	return LW_SUCCESS;
}


int edge_calculate_gbox_slow(const GEOGRAPHIC_EDGE *e, GBOX *gbox)
{
	int steps = 1000000;
	int i;
	double dx, dy, dz;
	double distance = sphere_distance(&(e->start), &(e->end));
	POINT3D pn, p, start, end;

	/* Edge is zero length, just return the naive box */
	if ( FP_IS_ZERO(distance) )
	{
		LWDEBUG(4, "edge is zero length. returning");
		geog2cart(&(e->start), &start);
		geog2cart(&(e->end), &end);
		gbox_init_point3d(&start, gbox);
		gbox_merge_point3d(&end, gbox);
		return LW_SUCCESS;
	}

	/* Edge is antipodal (one point on each side of the globe),
	   set the box to contain the whole world and return */
	if ( FP_EQUALS(distance, M_PI) )
	{
		LWDEBUG(4, "edge is antipodal. setting to maximum size box, and returning");
		gbox->xmin = gbox->ymin = gbox->zmin = -1.0;
		gbox->xmax = gbox->ymax = gbox->zmax = 1.0;
		return LW_SUCCESS;
	}

	/* Walk along the chord between start and end incrementally,
	   normalizing at each step. */
	geog2cart(&(e->start), &start);
	geog2cart(&(e->end), &end);
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
		gbox_merge_point3d(&pn, gbox);
	}
	return LW_SUCCESS;
}

/**
* The magic function, given an edge in spherical coordinates, calculate a
* 3D bounding box that fully contains it, taking into account the curvature
* of the sphere on which it is inscribed.
*
* Any arc on the sphere defines a plane that bisects the sphere. In this plane,
* the arc is a portion of a unit circle.
* Projecting the end points of the axes (1,0,0), (-1,0,0) etc, into the plane
* and normalizing yields potential extrema points. Those points on the
* side of the plane-dividing line formed by the end points that is opposite
* the origin of the plane are extrema and should be added to the bounding box.
*/
int edge_calculate_gbox(const POINT3D *A1, const POINT3D *A2, GBOX *gbox)
{
	POINT2D R1, R2, RX, O;
	POINT3D AN, A3;
	POINT3D X[6];
	int i, o_side;

	/* Initialize the box with the edge end points */
	gbox_init_point3d(A1, gbox);
	gbox_merge_point3d(A2, gbox);

	/* Zero length edge, just return! */
	if ( p3d_same(A1, A2) )
		return LW_SUCCESS;

	/* Error out on antipodal edge */
	if ( FP_EQUALS(A1->x, -1*A2->x) && FP_EQUALS(A1->y, -1*A2->y) && FP_EQUALS(A1->z, -1*A2->z) )
	{
		lwerror("Antipodal (180 degrees long) edge detected!");
		return LW_FAILURE;
	}

	/* Create A3, a vector in the plane of A1/A2, orthogonal to A1  */
	unit_normal(A1, A2, &AN);
	unit_normal(&AN, A1, &A3);

	/* Project A1 and A2 into the 2-space formed by the plane A1/A3 */
	R1.x = 1.0;
	R1.y = 0.0;
	R2.x = dot_product(A2, A1);
	R2.y = dot_product(A2, &A3);

	/* Initialize our 3-space axis points (x+, x-, y+, y-, z+, z-) */
	memset(X, 0, sizeof(POINT3D) * 6);
	X[0].x = X[2].y = X[4].z =  1.0;
	X[1].x = X[3].y = X[5].z = -1.0;

	/* Initialize a 2-space origin point. */
	O.x = O.y = 0.0;
	/* What side of the line joining R1/R2 is O? */
	o_side = lw_segment_side(&R1, &R2, &O);

	/* Add any extrema! */
	for ( i = 0; i < 6; i++ )
	{
		/* Convert 3-space axis points to 2-space unit vectors */
		RX.x = dot_product(&(X[i]), A1);
		RX.y = dot_product(&(X[i]), &A3);
		normalize2d(&RX);

		/* Any axis end on the side of R1/R2 opposite the origin */
		/* is an extreme point in the arc, so we add the 3-space */
		/* version of the point on R1/R2 to the gbox */
		if ( lw_segment_side(&R1, &R2, &RX) != o_side )
		{
			POINT3D Xn;
			Xn.x = RX.x * A1->x + RX.y * A3.x;
			Xn.y = RX.x * A1->y + RX.y * A3.y;
			Xn.z = RX.x * A1->z + RX.y * A3.z;

			gbox_merge_point3d(&Xn, gbox);
		}
	}

	return LW_SUCCESS;
}

/*
* When we have a globe-covering gbox but we still want an outside
* point, we do this Very Bad Hack, which is look at the first two points
* in the ring and then nudge a point to the left of that arc.
* There is an assumption of convexity built in there, as well as that
* the shape doesn't have a sharp reversal in it. It's ugly, but
* it fixes some common cases (large selection polygons) that users
* are generating. At some point all of geodetic needs a clean-room
* rewrite.
* There is also an assumption of CCW exterior ring, which is how the
* GeoJSON spec defined geographic ring orientation.
*/
static int lwpoly_pt_outside_hack(const LWPOLY *poly, POINT2D *pt_outside)
{
	GEOGRAPHIC_POINT g1, g2, gSum;
	POINT4D p1, p2;
	POINT3D q1, q2, qMid, qCross, qSum;
	POINTARRAY *pa;
	if (lwgeom_is_empty((LWGEOM*)poly))
		return LW_FAILURE;
	if (poly->nrings < 1)
		return LW_FAILURE;
	pa = poly->rings[0];
	if (pa->npoints < 2)
		return LW_FAILURE;

	/* First two points of ring */
	getPoint4d_p(pa, 0, &p1);
	getPoint4d_p(pa, 1, &p2);
	/* Convert to XYZ unit vectors */
	geographic_point_init(p1.x, p1.y, &g1);
	geographic_point_init(p2.x, p2.y, &g2);
	geog2cart(&g1, &q1);
	geog2cart(&g2, &q2);
	/* Mid-point of first two points */
	vector_sum(&q1, &q2, &qMid);
	normalize(&qMid);
	/* Cross product of first two points (perpendicular) */
	cross_product(&q1, &q2, &qCross);
	normalize(&qCross);
	/* Invert it to put it outside, and scale down */
	vector_scale(&qCross, -0.2);
	/* Project midpoint to the right */
	vector_sum(&qMid, &qCross, &qSum);
	normalize(&qSum);
	/* Convert back to lon/lat */
	cart2geog(&qSum, &gSum);
	pt_outside->x = rad2deg(gSum.lon);
	pt_outside->y = rad2deg(gSum.lat);
	return LW_SUCCESS;
}

int lwpoly_pt_outside(const LWPOLY *poly, POINT2D *pt_outside)
{
	int rv;
	/* Make sure we have boxes */
	if ( poly->bbox )
	{
		rv = gbox_pt_outside(poly->bbox, pt_outside);
	}
	else
	{
		GBOX gbox;
		lwgeom_calculate_gbox_geodetic((LWGEOM*)poly, &gbox);
		rv = gbox_pt_outside(&gbox, pt_outside);
	}

	if (rv == LW_FALSE)
		return lwpoly_pt_outside_hack(poly, pt_outside);

	return rv;
}

/**
* Given a unit geocentric gbox, return a lon/lat (degrees) coordinate point point that is
* guaranteed to be outside the box (and therefore anything it contains).
*/
int gbox_pt_outside(const GBOX *gbox, POINT2D *pt_outside)
{
	double grow = M_PI / 180.0 / 60.0; /* one arc-minute */
	int i;
	GBOX ge;
	POINT3D corners[8];
	POINT3D pt;
	GEOGRAPHIC_POINT g;

	while ( grow < M_PI )
	{
		/* Assign our box and expand it slightly. */
		ge = *gbox;
		if ( ge.xmin > -1 ) ge.xmin -= grow;
		if ( ge.ymin > -1 ) ge.ymin -= grow;
		if ( ge.zmin > -1 ) ge.zmin -= grow;
		if ( ge.xmax < 1 )  ge.xmax += grow;
		if ( ge.ymax < 1 )  ge.ymax += grow;
		if ( ge.zmax < 1 )  ge.zmax += grow;

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

		LWDEBUG(4, "trying to use a box corner point...");
		for ( i = 0; i < 8; i++ )
		{
			normalize(&(corners[i]));
			LWDEBUGF(4, "testing corner %d: POINT(%.8g %.8g %.8g)", i, corners[i].x, corners[i].y, corners[i].z);
			if ( ! gbox_contains_point3d(gbox, &(corners[i])) )
			{
				LWDEBUGF(4, "corner %d is outside our gbox", i);
				pt = corners[i];
				normalize(&pt);
				cart2geog(&pt, &g);
				pt_outside->x = rad2deg(g.lon);
				pt_outside->y = rad2deg(g.lat);
				LWDEBUGF(4, "returning POINT(%.8g %.8g) as outside point", pt_outside->x, pt_outside->y);
				return LW_SUCCESS;
			}
		}

		/* Try a wider growth to push the corners outside the original box. */
		grow *= 2.0;
	}

	/* This should never happen! */
	// lwerror("BOOM! Could not generate outside point!");
	return LW_FAILURE;
}


static int ptarray_segmentize_sphere_edge_recursive (
	const POINT3D *p1, const POINT3D *p2, /* 3-space points we are interpolating between */
	const POINT4D *v1, const POINT4D *v2, /* real values and z/m values */
	double d, double max_seg_length, /* current segment length and segment limit */
	POINTARRAY *pa) /* write out results here */
{
	GEOGRAPHIC_POINT g;
	/* Reached the terminal leaf in recursion. Add */
	/* the left-most point to the pointarray here */
	/* We recurse down the left side first, so outputs should */
	/* end up added to the array in order this way */
	if (d <= max_seg_length)
	{
		POINT4D p;
		cart2geog(p1, &g);
		p.x = v1->x;
		p.y = v1->y;
		p.z = v1->z;
		p.m = v1->m;
		return ptarray_append_point(pa, &p, LW_FALSE);
	}
	/* Find the mid-point and recurse on the left and then the right */
	else
	{
		/* Calculate mid-point */
		POINT3D mid;
		mid.x = (p1->x + p2->x) / 2.0;
		mid.y = (p1->y + p2->y) / 2.0;
		mid.z = (p1->z + p2->z) / 2.0;
		normalize(&mid);

		/* Calculate z/m mid-values */
		POINT4D midv;
		cart2geog(&mid, &g);
		midv.x = rad2deg(g.lon);
		midv.y = rad2deg(g.lat);
		midv.z = (v1->z + v2->z) / 2.0;
		midv.m = (v1->m + v2->m) / 2.0;
		/* Recurse on the left first */
		ptarray_segmentize_sphere_edge_recursive(p1, &mid, v1, &midv, d/2.0, max_seg_length, pa);
		ptarray_segmentize_sphere_edge_recursive(&mid, p2, &midv, v2, d/2.0, max_seg_length, pa);
		return LW_SUCCESS;
	}
}

/**
* Create a new point array with no segment longer than the input segment length (expressed in radians!)
* @param pa_in - input point array pointer
* @param max_seg_length - maximum output segment length in radians
*/
static POINTARRAY*
ptarray_segmentize_sphere(const POINTARRAY *pa_in, double max_seg_length)
{
	POINTARRAY *pa_out;
	int hasz = ptarray_has_z(pa_in);
	int hasm = ptarray_has_m(pa_in);
	POINT4D p1, p2;
	POINT3D q1, q2;
	GEOGRAPHIC_POINT g1, g2;
	uint32_t i;

	/* Just crap out on crazy input */
	if ( ! pa_in )
		lwerror("%s: null input pointarray", __func__);
	if ( max_seg_length <= 0.0 )
		lwerror("%s: maximum segment length must be positive", __func__);

	/* Empty starting array */
	pa_out = ptarray_construct_empty(hasz, hasm, pa_in->npoints);

	/* Simple loop per edge */
	for (i = 1; i < pa_in->npoints; i++)
	{
		getPoint4d_p(pa_in, i-1, &p1);
		getPoint4d_p(pa_in, i, &p2);
		geographic_point_init(p1.x, p1.y, &g1);
		geographic_point_init(p2.x, p2.y, &g2);

		/* Skip duplicate points (except in case of 2-point lines!) */
		if ((pa_in->npoints > 2) && p4d_same(&p1, &p2))
			continue;

		/* How long is this edge? */
		double d = sphere_distance(&g1, &g2);

		if (d > max_seg_length)
		{
			geog2cart(&g1, &q1);
			geog2cart(&g2, &q2);
			/* 3-d end points, XYZM end point, current edge size, min edge size */
			ptarray_segmentize_sphere_edge_recursive(&q1, &q2, &p1, &p2, d, max_seg_length, pa_out);
		}
		/* If we don't segmentize, we need to add first point manually */
		else
		{
			ptarray_append_point(pa_out, &p1, LW_TRUE);
		}
	}
	/* Always add the last point */
	ptarray_append_point(pa_out, &p2, LW_TRUE);
	return pa_out;
}

/**
* Create a new, densified geometry where no segment is longer than max_seg_length.
* Input geometry is not altered, output geometry must be freed by caller.
* @param lwg_in = input geometry
* @param max_seg_length = maximum segment length in radians
*/
LWGEOM*
lwgeom_segmentize_sphere(const LWGEOM *lwg_in, double max_seg_length)
{
	POINTARRAY *pa_out;
	LWLINE *lwline;
	LWPOLY *lwpoly_in, *lwpoly_out;
	LWCOLLECTION *lwcol_in, *lwcol_out;
	uint32_t i;

	/* Reflect NULL */
	if ( ! lwg_in )
		return NULL;

	/* Clone empty */
	if ( lwgeom_is_empty(lwg_in) )
		return lwgeom_clone(lwg_in);

	switch (lwg_in->type)
	{
	case MULTIPOINTTYPE:
	case POINTTYPE:
		return lwgeom_clone_deep(lwg_in);
		break;
	case LINETYPE:
		lwline = lwgeom_as_lwline(lwg_in);
		pa_out = ptarray_segmentize_sphere(lwline->points, max_seg_length);
		return lwline_as_lwgeom(lwline_construct(lwg_in->srid, NULL, pa_out));
		break;
	case POLYGONTYPE:
		lwpoly_in = lwgeom_as_lwpoly(lwg_in);
		lwpoly_out = lwpoly_construct_empty(lwg_in->srid, lwgeom_has_z(lwg_in), lwgeom_has_m(lwg_in));
		for ( i = 0; i < lwpoly_in->nrings; i++ )
		{
			pa_out = ptarray_segmentize_sphere(lwpoly_in->rings[i], max_seg_length);
			lwpoly_add_ring(lwpoly_out, pa_out);
		}
		return lwpoly_as_lwgeom(lwpoly_out);
		break;
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COLLECTIONTYPE:
		lwcol_in = lwgeom_as_lwcollection(lwg_in);
		lwcol_out = lwcollection_construct_empty(lwg_in->type, lwg_in->srid, lwgeom_has_z(lwg_in), lwgeom_has_m(lwg_in));
		for ( i = 0; i < lwcol_in->ngeoms; i++ )
		{
			lwcollection_add_lwgeom(lwcol_out, lwgeom_segmentize_sphere(lwcol_in->geoms[i], max_seg_length));
		}
		return lwcollection_as_lwgeom(lwcol_out);
		break;
	default:
		lwerror("lwgeom_segmentize_sphere: unsupported input geometry type: %d - %s",
		        lwg_in->type, lwtype_name(lwg_in->type));
		break;
	}

	lwerror("lwgeom_segmentize_sphere got to the end of the function, should not happen");
	return NULL;
}


static double ptarray_distance_spheroid(const POINTARRAY *pa1, const POINTARRAY *pa2, const SPHEROID *s, double tolerance, int check_intersection)
{
	GEOGRAPHIC_EDGE e1, e2;
	GEOGRAPHIC_POINT g1, g2;
	GEOGRAPHIC_POINT nearest1, nearest2;
	POINT3D A1, A2, B1, B2;
	const POINT2D *p;
	double distance;
	uint32_t i, j;
	int use_sphere = (s->a == s->b ? 1 : 0);

	/* Make result really big, so that everything will be smaller than it */
	distance = FLT_MAX;

	/* Empty point arrays? Return negative */
	if ( pa1->npoints == 0 || pa2->npoints == 0 )
		return -1.0;

	/* Handle point/point case here */
	if ( pa1->npoints == 1 && pa2->npoints == 1 )
	{
		p = getPoint2d_cp(pa1, 0);
		geographic_point_init(p->x, p->y, &g1);
		p = getPoint2d_cp(pa2, 0);
		geographic_point_init(p->x, p->y, &g2);
		/* Sphere special case, axes equal */
		distance = s->radius * sphere_distance(&g1, &g2);
		if ( use_sphere )
			return distance;
		/* Below tolerance, actual distance isn't of interest */
		else if ( distance < 0.95 * tolerance )
			return distance;
		/* Close or greater than tolerance, get the real answer to be sure */
		else
			return spheroid_distance(&g1, &g2, s);
	}

	/* Handle point/line case here */
	if ( pa1->npoints == 1 || pa2->npoints == 1 )
	{
		/* Handle one/many case here */
		uint32_t i;
		const POINTARRAY *pa_one;
		const POINTARRAY *pa_many;

		if ( pa1->npoints == 1 )
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
		p = getPoint2d_cp(pa_one, 0);
		geographic_point_init(p->x, p->y, &g1);

		/* Initialize start of line */
		p = getPoint2d_cp(pa_many, 0);
		geographic_point_init(p->x, p->y, &(e1.start));

		/* Iterate through the edges in our line */
		for ( i = 1; i < pa_many->npoints; i++ )
		{
			double d;
			p = getPoint2d_cp(pa_many, i);
			geographic_point_init(p->x, p->y, &(e1.end));
			/* Get the spherical distance between point and edge */
			d = s->radius * edge_distance_to_point(&e1, &g1, &g2);
			/* New shortest distance! Record this distance / location */
			if ( d < distance )
			{
				distance = d;
				nearest2 = g2;
			}
			/* We've gotten closer than the tolerance... */
			if ( d <= tolerance )
			{
				/* Working on a sphere? The answer is correct, return */
				if ( use_sphere )
				{
					return d;
				}
				/* Far enough past the tolerance that the spheroid calculation won't change things */
				else if ( d <= tolerance * 0.95 )
				{
					return d;
				}
				/* On a spheroid and near the tolerance? Confirm that we are *actually* closer than tolerance */
				else
				{
					d = spheroid_distance(&g1, &nearest2, s);
					/* Yes, closer than tolerance, return! */
					if ( d <= tolerance )
						return d;
				}
			}
			e1.start = e1.end;
		}

		/* On sphere, return answer */
		if ( use_sphere )
			return distance;
		/* On spheroid, calculate final answer based on closest approach */
		else
			return spheroid_distance(&g1, &nearest2, s);

	}

	/* Initialize start of line 1 */
	p = getPoint2d_cp(pa1, 0);
	geographic_point_init(p->x, p->y, &(e1.start));
	geog2cart(&(e1.start), &A1);


	/* Handle line/line case */
	for ( i = 1; i < pa1->npoints; i++ )
	{
		p = getPoint2d_cp(pa1, i);
		geographic_point_init(p->x, p->y, &(e1.end));
		geog2cart(&(e1.end), &A2);

		/* Initialize start of line 2 */
		p = getPoint2d_cp(pa2, 0);
		geographic_point_init(p->x, p->y, &(e2.start));
		geog2cart(&(e2.start), &B1);

		for ( j = 1; j < pa2->npoints; j++ )
		{
			double d;

			p = getPoint2d_cp(pa2, j);
			geographic_point_init(p->x, p->y, &(e2.end));
			geog2cart(&(e2.end), &B2);

			LWDEBUGF(4, "e1.start == GPOINT(%.6g %.6g) ", e1.start.lat, e1.start.lon);
			LWDEBUGF(4, "e1.end == GPOINT(%.6g %.6g) ", e1.end.lat, e1.end.lon);
			LWDEBUGF(4, "e2.start == GPOINT(%.6g %.6g) ", e2.start.lat, e2.start.lon);
			LWDEBUGF(4, "e2.end == GPOINT(%.6g %.6g) ", e2.end.lat, e2.end.lon);

			if ( check_intersection && edge_intersects(&A1, &A2, &B1, &B2) )
			{
				LWDEBUG(4,"edge intersection! returning 0.0");
				return 0.0;
			}
			d = s->radius * edge_distance_to_edge(&e1, &e2, &g1, &g2);
			LWDEBUGF(4,"got edge_distance_to_edge %.8g", d);

			if ( d < distance )
			{
				distance = d;
				nearest1 = g1;
				nearest2 = g2;
			}
			if ( d <= tolerance )
			{
				if ( use_sphere )
				{
					return d;
				}
				else
				{
					d = spheroid_distance(&nearest1, &nearest2, s);
					if ( d <= tolerance )
						return d;
				}
			}

			/* Copy end to start to allow a new end value in next iteration */
			e2.start = e2.end;
			B1 = B2;
		}

		/* Copy end to start to allow a new end value in next iteration */
		e1.start = e1.end;
		A1 = A2;
		LW_ON_INTERRUPT(return -1.0);
	}
	LWDEBUGF(4,"finished all loops, returning %.8g", distance);

	if ( use_sphere )
		return distance;
	else
		return spheroid_distance(&nearest1, &nearest2, s);
}


/**
* Delegate to the spheroid function with a spherically
* parameterized spheroid.
*/
double lwgeom_area_sphere(const LWGEOM *lwgeom, const SPHEROID *spheroid)
{
	SPHEROID s;
	spheroid_init(&s, WGS84_RADIUS, WGS84_RADIUS);
	return lwgeom_area_spheroid(lwgeom, &s);
}


/**
* Calculate a projected point given a source point, a distance and a bearing.
* @param r - location of first point.
* @param spheroid - spheroid definition.
* @param distance - distance, in units of the spheroid def'n.
* @param azimuth - azimuth in radians.
* @return s - location of projected point.
*
*/
LWPOINT* lwgeom_project_spheroid(const LWPOINT *r, const SPHEROID *spheroid, double distance, double azimuth)
{
	GEOGRAPHIC_POINT geo_source, geo_dest;
	POINT4D pt_dest;
	double x, y;
	LWPOINT *lwp;
	int has_z, has_m;

	/* Normalize distance to be positive*/
	if ( distance < 0.0 ) {
		distance = -distance;
		azimuth += M_PI;
	}

	/* Normalize azimuth */
	azimuth -= 2.0 * M_PI * floor(azimuth / (2.0 * M_PI));

	/* Check the distance validity */
	if ( distance > (M_PI * spheroid->radius) )
	{
		lwerror("Distance must not be greater than %g", M_PI * spheroid->radius);
		return NULL;
	}

	/* Convert to ta geodetic point */
	x = lwpoint_get_x(r);
	y = lwpoint_get_y(r);
	has_z = lwgeom_has_z(lwpoint_as_lwgeom(r));
	has_m = lwgeom_has_m(lwpoint_as_lwgeom(r));
	geographic_point_init(x, y, &geo_source);

	/* Try the projection */
	if( spheroid_project(&geo_source, spheroid, distance, azimuth, &geo_dest) == LW_FAILURE )
	{
		LWDEBUGF(3, "Unable to project from (%g %g) with azimuth %g and distance %g", x, y, azimuth, distance);
		lwerror("Unable to project from (%g %g) with azimuth %g and distance %g", x, y, azimuth, distance);
		return NULL;
	}

	/* Build the output LWPOINT */
	pt_dest.x = rad2deg(longitude_radians_normalize(geo_dest.lon));
	pt_dest.y = rad2deg(latitude_radians_normalize(geo_dest.lat));
	pt_dest.z = has_z ? lwpoint_get_z(r) : 0.0;
	pt_dest.m = has_m ? lwpoint_get_m(r) : 0.0;
	lwp = lwpoint_make(r->srid, has_z, has_m, &pt_dest);
	lwgeom_set_geodetic(lwpoint_as_lwgeom(lwp), LW_TRUE);
	return lwp;
}

LWPOINT* lwgeom_project_spheroid_lwpoint(const LWPOINT *from, const LWPOINT *to, const SPHEROID *spheroid, double distance)
{
	double azimuth = lwgeom_azumith_spheroid(from, to, spheroid);
	LWPOINT *lwp = lwgeom_project_spheroid(to, spheroid, distance, azimuth);
	return lwp;
}


/**
* Calculate a bearing (azimuth) given a source and destination point.
https://accesd.desjardins.ca/coast* @param r - location of first point.
* @param s - location of second point.
* @param spheroid - spheroid definition.
* @return azimuth - azimuth in radians.
*
*/
double lwgeom_azumith_spheroid(const LWPOINT *r, const LWPOINT *s, const SPHEROID *spheroid)
{
	GEOGRAPHIC_POINT g1, g2;
	double x1, y1, x2, y2, az;

	/* Convert r to a geodetic point */
	x1 = lwpoint_get_x(r);
	y1 = lwpoint_get_y(r);
	geographic_point_init(x1, y1, &g1);

	/* Convert s to a geodetic point */
	x2 = lwpoint_get_x(s);
	y2 = lwpoint_get_y(s);
	geographic_point_init(x2, y2, &g2);

	/* Same point, return NaN */
	if ( FP_EQUALS(x1, x2) && FP_EQUALS(y1, y2) )
	{
		return NAN;
	}

	/* Do the direction calculation */
	az = spheroid_direction(&g1, &g2, spheroid);
	/* Ensure result is positive */
	return az < -0 ? 2*M_PI + az : az;
	// return az;
}

/**
* Calculate the distance between two LWGEOMs, using the coordinates are
* longitude and latitude. Return immediately when the calculated distance drops
* below the tolerance (useful for dwithin calculations).
* Return a negative distance for incalculable cases.
*/
double lwgeom_distance_spheroid(const LWGEOM *lwgeom1, const LWGEOM *lwgeom2, const SPHEROID *spheroid, double tolerance)
{
	uint8_t type1, type2;
	int check_intersection = LW_FALSE;
	GBOX gbox1, gbox2;

	gbox_init(&gbox1);
	gbox_init(&gbox2);

	assert(lwgeom1);
	assert(lwgeom2);

	LWDEBUGF(4, "entered function, tolerance %.8g", tolerance);

	/* What's the distance to an empty geometry? We don't know.
	   Return a negative number so the caller can catch this case. */
	if ( lwgeom_is_empty(lwgeom1) || lwgeom_is_empty(lwgeom2) )
	{
		return -1.0;
	}

	type1 = lwgeom1->type;
	type2 = lwgeom2->type;

	/* Make sure we have boxes */
	if ( FLAGS_GET_GEODETIC(lwgeom1->flags) && lwgeom1->bbox )
		gbox1 = *(lwgeom1->bbox);
	else
		lwgeom_calculate_gbox_geodetic(lwgeom1, &gbox1);

	/* Make sure we have boxes */
	if ( FLAGS_GET_GEODETIC(lwgeom2->flags) && lwgeom2->bbox )
		gbox2 = *(lwgeom2->bbox);
	else
		lwgeom_calculate_gbox_geodetic(lwgeom2, &gbox2);

	/* If the boxes aren't disjoint, we have to check for edge intersections */
	if ( gbox_overlaps(&gbox1, &gbox2) )
		check_intersection = LW_TRUE;

	/* Point/line combinations can all be handled with simple point array iterations */
	if ( ( type1 == POINTTYPE || type1 == LINETYPE ) &&
	     ( type2 == POINTTYPE || type2 == LINETYPE ) )
	{
		POINTARRAY *pa1, *pa2;

		if ( type1 == POINTTYPE )
			pa1 = ((LWPOINT*)lwgeom1)->point;
		else
			pa1 = ((LWLINE*)lwgeom1)->points;

		if ( type2 == POINTTYPE )
			pa2 = ((LWPOINT*)lwgeom2)->point;
		else
			pa2 = ((LWLINE*)lwgeom2)->points;

		return ptarray_distance_spheroid(pa1, pa2, spheroid, tolerance, check_intersection);
	}

	/* Point/Polygon cases, if point-in-poly, return zero, else return distance. */
	if ( ( type1 == POLYGONTYPE && type2 == POINTTYPE ) ||
	     ( type2 == POLYGONTYPE && type1 == POINTTYPE ) )
	{
		const POINT2D *p;
		LWPOLY *lwpoly;
		LWPOINT *lwpt;
		double distance = FLT_MAX;
		uint32_t i;

		if ( type1 == POINTTYPE )
		{
			lwpt = (LWPOINT*)lwgeom1;
			lwpoly = (LWPOLY*)lwgeom2;
		}
		else
		{
			lwpt = (LWPOINT*)lwgeom2;
			lwpoly = (LWPOLY*)lwgeom1;
		}
		p = getPoint2d_cp(lwpt->point, 0);

		/* Point in polygon implies zero distance */
		if ( lwpoly_covers_point2d(lwpoly, p) )
		{
			return 0.0;
		}

		/* Not inside, so what's the actual distance? */
		for ( i = 0; i < lwpoly->nrings; i++ )
		{
			double ring_distance = ptarray_distance_spheroid(lwpoly->rings[i], lwpt->point, spheroid, tolerance, check_intersection);
			if ( ring_distance < distance )
				distance = ring_distance;
			if ( distance <= tolerance )
				return distance;
		}
		return distance;
	}

	/* Line/polygon case, if start point-in-poly, return zero, else return distance. */
	if ( ( type1 == POLYGONTYPE && type2 == LINETYPE ) ||
	     ( type2 == POLYGONTYPE && type1 == LINETYPE ) )
	{
		const POINT2D *p;
		LWPOLY *lwpoly;
		LWLINE *lwline;
		double distance = FLT_MAX;
		uint32_t i;

		if ( type1 == LINETYPE )
		{
			lwline = (LWLINE*)lwgeom1;
			lwpoly = (LWPOLY*)lwgeom2;
		}
		else
		{
			lwline = (LWLINE*)lwgeom2;
			lwpoly = (LWPOLY*)lwgeom1;
		}
		p = getPoint2d_cp(lwline->points, 0);

		LWDEBUG(4, "checking if a point of line is in polygon");

		/* Point in polygon implies zero distance */
		if ( lwpoly_covers_point2d(lwpoly, p) )
			return 0.0;

		LWDEBUG(4, "checking ring distances");

		/* Not contained, so what's the actual distance? */
		for ( i = 0; i < lwpoly->nrings; i++ )
		{
			double ring_distance = ptarray_distance_spheroid(lwpoly->rings[i], lwline->points, spheroid, tolerance, check_intersection);
			LWDEBUGF(4, "ring[%d] ring_distance = %.8g", i, ring_distance);
			if ( ring_distance < distance )
				distance = ring_distance;
			if ( distance <= tolerance )
				return distance;
		}
		LWDEBUGF(4, "all rings checked, returning distance = %.8g", distance);
		return distance;

	}

	/* Polygon/polygon case, if start point-in-poly, return zero, else
	 * return distance. */
	if (type1 == POLYGONTYPE && type2 == POLYGONTYPE)
	{
		const POINT2D* p;
		LWPOLY* lwpoly1 = (LWPOLY*)lwgeom1;
		LWPOLY* lwpoly2 = (LWPOLY*)lwgeom2;
		double distance = FLT_MAX;
		uint32_t i, j;

		/* Point of 2 in polygon 1 implies zero distance */
		p = getPoint2d_cp(lwpoly1->rings[0], 0);
		if (lwpoly_covers_point2d(lwpoly2, p)) return 0.0;

		/* Point of 1 in polygon 2 implies zero distance */
		p = getPoint2d_cp(lwpoly2->rings[0], 0);
		if (lwpoly_covers_point2d(lwpoly1, p)) return 0.0;

		/* Not contained, so what's the actual distance? */
		for (i = 0; i < lwpoly1->nrings; i++)
		{
			for (j = 0; j < lwpoly2->nrings; j++)
			{
				double ring_distance =
				    ptarray_distance_spheroid(
					lwpoly1->rings[i],
					lwpoly2->rings[j],
					spheroid,
					tolerance,
					check_intersection);
				if (ring_distance < distance)
					distance = ring_distance;
				if (distance <= tolerance) return distance;
			}
		}
		return distance;
	}

	/* Recurse into collections */
	if ( lwtype_is_collection(type1) )
	{
		uint32_t i;
		double distance = FLT_MAX;
		LWCOLLECTION *col = (LWCOLLECTION*)lwgeom1;

		for ( i = 0; i < col->ngeoms; i++ )
		{
			double geom_distance = lwgeom_distance_spheroid(
			    col->geoms[i], lwgeom2, spheroid, tolerance);
			if ( geom_distance < distance )
				distance = geom_distance;
			if ( distance <= tolerance )
				return distance;
		}
		return distance;
	}

	/* Recurse into collections */
	if ( lwtype_is_collection(type2) )
	{
		uint32_t i;
		double distance = FLT_MAX;
		LWCOLLECTION *col = (LWCOLLECTION*)lwgeom2;

		for ( i = 0; i < col->ngeoms; i++ )
		{
			double geom_distance = lwgeom_distance_spheroid(lwgeom1, col->geoms[i], spheroid, tolerance);
			if ( geom_distance < distance )
				distance = geom_distance;
			if ( distance <= tolerance )
				return distance;
		}
		return distance;
	}


	lwerror("arguments include unsupported geometry type (%s, %s)", lwtype_name(type1), lwtype_name(type1));
	return -1.0;

}


int lwgeom_covers_lwgeom_sphere(const LWGEOM *lwgeom1, const LWGEOM *lwgeom2)
{
	int type1, type2;
	GBOX gbox1, gbox2;
	gbox1.flags = gbox2.flags = 0;

	assert(lwgeom1);
	assert(lwgeom2);

	type1 = lwgeom1->type;
	type2 = lwgeom2->type;

	/* dim(geom2) > dim(geom1) always returns false (because geom2 is bigger) */
	if ( (type1 == POINTTYPE && type2 == LINETYPE)
		|| (type1 == POINTTYPE && type2 == POLYGONTYPE)
		|| (type1 == LINETYPE && type2 == POLYGONTYPE) )
	{
		LWDEBUG(4, "dimension of geom2 is bigger than geom1");
		return LW_FALSE;
	}

	/* Make sure we have boxes */
	if ( lwgeom1->bbox )
		gbox1 = *(lwgeom1->bbox);
	else
		lwgeom_calculate_gbox_geodetic(lwgeom1, &gbox1);

	/* Make sure we have boxes */
	if ( lwgeom2->bbox )
		gbox2 = *(lwgeom2->bbox);
	else
		lwgeom_calculate_gbox_geodetic(lwgeom2, &gbox2);


	/* Handle the polygon/point case */
	if ( type1 == POLYGONTYPE && type2 == POINTTYPE )
	{
		POINT2D pt_to_test;
		getPoint2d_p(((LWPOINT*)lwgeom2)->point, 0, &pt_to_test);
		return lwpoly_covers_point2d((LWPOLY*)lwgeom1, &pt_to_test);
	}
	else if ( type1 == POLYGONTYPE && type2 == LINETYPE)
	{
		return lwpoly_covers_lwline((LWPOLY*)lwgeom1, (LWLINE*)lwgeom2);
	}
	else if ( type1 == POLYGONTYPE && type2 == POLYGONTYPE)
	{
		return lwpoly_covers_lwpoly((LWPOLY*)lwgeom1, (LWPOLY*)lwgeom2);
	}
	else if ( type1 == LINETYPE && type2 == POINTTYPE)
	{
		return lwline_covers_lwpoint((LWLINE*)lwgeom1, (LWPOINT*)lwgeom2);
	}
	else if ( type1 == LINETYPE && type2 == LINETYPE)
	{
		return lwline_covers_lwline((LWLINE*)lwgeom1, (LWLINE*)lwgeom2);
	}
	else if ( type1 == POINTTYPE && type2 == POINTTYPE)
	{
		return lwpoint_same((LWPOINT*)lwgeom1, (LWPOINT*)lwgeom2);
	}

	/* If any of the first argument parts covers the second argument, it's true */
	if ( lwtype_is_collection( type1 ) )
	{
		uint32_t i;
		LWCOLLECTION *col = (LWCOLLECTION*)lwgeom1;

		for ( i = 0; i < col->ngeoms; i++ )
		{
			if ( lwgeom_covers_lwgeom_sphere(col->geoms[i], lwgeom2) )
			{
				return LW_TRUE;
			}
		}
		return LW_FALSE;
	}

	/* Only if all of the second arguments are covered by the first argument is the condition true */
	if ( lwtype_is_collection( type2 ) )
	{
		uint32_t i;
		LWCOLLECTION *col = (LWCOLLECTION*)lwgeom2;

		for ( i = 0; i < col->ngeoms; i++ )
		{
			if ( ! lwgeom_covers_lwgeom_sphere(lwgeom1, col->geoms[i]) )
			{
				return LW_FALSE;
			}
		}
		return LW_TRUE;
	}

	/* Don't get here */
	lwerror("lwgeom_covers_lwgeom_sphere: reached end of function without resolution");
	return LW_FALSE;

}

/**
* Given a polygon (lon/lat decimal degrees) and point (lon/lat decimal degrees) and
* a guaranteed outside point (lon/lat decimal degrees) (calculate with gbox_pt_outside())
* return LW_TRUE if point is inside or on edge of polygon.
*/
int lwpoly_covers_point2d(const LWPOLY *poly, const POINT2D *pt_to_test)
{
	uint32_t i;
	int in_hole_count = 0;
	POINT3D p;
	GEOGRAPHIC_POINT gpt_to_test;
	POINT2D pt_outside;
	GBOX gbox;
#if POSTGIS_DEBUG_LEVEL >= 4
	char *geom_ewkt;
#endif
	gbox.flags = 0;

	/* Nulls and empties don't contain anything! */
	if ( ! poly || lwgeom_is_empty((LWGEOM*)poly) )
	{
		LWDEBUG(4,"returning false, geometry is empty or null");
		return LW_FALSE;
	}

	/* Make sure we have boxes */
	if ( poly->bbox )
		gbox = *(poly->bbox);
	else
		lwgeom_calculate_gbox_geodetic((LWGEOM*)poly, &gbox);

	/* Point not in box? Done! */
	geographic_point_init(pt_to_test->x, pt_to_test->y, &gpt_to_test);
	geog2cart(&gpt_to_test, &p);
	if ( ! gbox_contains_point3d(&gbox, &p) )
	{
		LWDEBUG(4, "the point is not in the box!");
		return LW_FALSE;
	}

	/* Calculate our outside point from the gbox */
	lwpoly_pt_outside(poly, &pt_outside);

	LWDEBUGF(4, "pt_outside POINT(%.18g %.18g)", pt_outside.x, pt_outside.y);
	LWDEBUGF(4, "pt_to_test POINT(%.18g %.18g)", pt_to_test->x, pt_to_test->y);
#if POSTGIS_DEBUG_LEVEL >= 4
	geom_ewkt = lwgeom_to_ewkt((LWGEOM*)poly);
	LWDEBUGF(4, "polygon %s", geom_ewkt);
	lwfree(geom_ewkt);
	geom_ewkt = gbox_to_string(&gbox);
	LWDEBUGF(4, "gbox %s", geom_ewkt);
	lwfree(geom_ewkt);
#endif

	/* Not in outer ring? We're done! */
	if ( ! ptarray_contains_point_sphere(poly->rings[0], &pt_outside, pt_to_test) )
	{
		LWDEBUG(4,"returning false, point is outside ring");
		return LW_FALSE;
	}

	LWDEBUGF(4, "testing %d rings", poly->nrings);

	/* But maybe point is in a hole... */
	for ( i = 1; i < poly->nrings; i++ )
	{
		LWDEBUGF(4, "ring test loop %d", i);
		/* Count up hole containment. Odd => outside boundary. */
		if ( ptarray_contains_point_sphere(poly->rings[i], &pt_outside, pt_to_test) )
			in_hole_count++;
	}

	LWDEBUGF(4, "in_hole_count == %d", in_hole_count);

	if ( in_hole_count % 2 )
	{
		LWDEBUG(4,"returning false, inner ring containment count is odd");
		return LW_FALSE;
	}

	LWDEBUG(4,"returning true, inner ring containment count is even");
	return LW_TRUE;
}

/**
 * Given a polygon1 check if all points of polygon2 are inside polygon1 and no
 * intersections of the polygon edges occur.
 * return LW_TRUE if polygon is inside or on edge of polygon.
 */
int lwpoly_covers_lwpoly(const LWPOLY *poly1, const LWPOLY *poly2)
{
	uint32_t i;

	/* Nulls and empties don't contain anything! */
	if ( ! poly1 || lwgeom_is_empty((LWGEOM*)poly1) )
	{
		LWDEBUG(4,"returning false, geometry1 is empty or null");
		return LW_FALSE;
	}

	/* Nulls and empties don't contain anything! */
	if ( ! poly2 || lwgeom_is_empty((LWGEOM*)poly2) )
	{
		LWDEBUG(4,"returning false, geometry2 is empty or null");
		return LW_FALSE;
	}

	/* check if all vertices of poly2 are inside poly1 */
	for (i = 0; i < poly2->nrings; i++)
	{
		if (LW_FALSE == lwpoly_covers_pointarray(poly1, poly2->rings[i]))
		{
			LWDEBUG(4,"returning false, geometry2 has point outside of geometry1");
			return LW_FALSE;
		}
	}

	/* check for any edge intersections, so nothing is partially outside of poly1 */
	for (i = 0; i < poly2->nrings; i++)
	{
		if (LW_TRUE == lwpoly_intersects_line(poly1, poly2->rings[i]))
		{
			LWDEBUG(4,"returning false, geometry2 is partially outside of geometry1");
			return LW_FALSE;
		}
	}

	/* no abort condition found, so the poly2 should be completely inside poly1 */
	return LW_TRUE;
}

/**
 *
 */
int lwpoly_covers_lwline(const LWPOLY *poly, const LWLINE *line)
{
   /* Nulls and empties don't contain anything! */
   if ( ! poly || lwgeom_is_empty((LWGEOM*)poly) )
   {
	   LWDEBUG(4,"returning false, geometry1 is empty or null");
	   return LW_FALSE;
   }

   /* Nulls and empties don't contain anything! */
   if ( ! line || lwgeom_is_empty((LWGEOM*)line) )
   {
	   LWDEBUG(4,"returning false, geometry2 is empty or null");
	   return LW_FALSE;
   }

   if (LW_FALSE == lwpoly_covers_pointarray(poly, line->points))
   {
	   LWDEBUG(4,"returning false, geometry2 has point outside of geometry1");
	   return LW_FALSE;
   }

   /* check for any edge intersections, so nothing is partially outside of poly1 */
   if (LW_TRUE == lwpoly_intersects_line(poly, line->points))
   {
	   LWDEBUG(4,"returning false, geometry2 is partially outside of geometry1");
	   return LW_FALSE;
   }

   /* no abort condition found, so the poly2 should be completely inside poly1 */
   return LW_TRUE;
}

/**
 * return LW_TRUE if all points are inside the polygon
 */
int lwpoly_covers_pointarray(const LWPOLY* lwpoly, const POINTARRAY* pta)
{
	uint32_t i;
	for (i = 0; i < pta->npoints; i++) {
		const POINT2D* pt_to_test = getPoint2d_cp(pta, i);

		if ( LW_FALSE == lwpoly_covers_point2d(lwpoly, pt_to_test) ) {
			LWDEBUG(4,"returning false, geometry2 has point outside of geometry1");
			return LW_FALSE;
		}
	}

	return LW_TRUE;
}

/**
 * Checks if any edges of lwpoly intersect with the line formed by the pointarray
 * return LW_TRUE if any intersection between the given polygon and the line
 */
int lwpoly_intersects_line(const LWPOLY* lwpoly, const POINTARRAY* line)
{
	uint32_t i, j, k;
	POINT3D pa1, pa2, pb1, pb2;
	for (i = 0; i < lwpoly->nrings; i++)
	{
		for (j = 0; j < lwpoly->rings[i]->npoints - 1; j++)
		{
			const POINT2D* a1 = getPoint2d_cp(lwpoly->rings[i], j);
			const POINT2D* a2 = getPoint2d_cp(lwpoly->rings[i], j+1);

			/* Set up our stab line */
			ll2cart(a1, &pa1);
			ll2cart(a2, &pa2);

			for (k = 0; k < line->npoints - 1; k++)
			{
				const POINT2D* b1 = getPoint2d_cp(line, k);
				const POINT2D* b2 = getPoint2d_cp(line, k+1);

				/* Set up our stab line */
				ll2cart(b1, &pb1);
				ll2cart(b2, &pb2);

				int inter = edge_intersects(&pa1, &pa2, &pb1, &pb2);

				/* ignore same edges */
				if (inter & PIR_INTERSECTS
					&& !(inter & PIR_B_TOUCH_RIGHT || inter & PIR_COLINEAR) )
				{
					return LW_TRUE;
				}
			}
		}
	}

	return LW_FALSE;
}

/**
 * return LW_TRUE if any of the line segments covers the point
 */
int lwline_covers_lwpoint(const LWLINE* lwline, const LWPOINT* lwpoint)
{
	uint32_t i;
	GEOGRAPHIC_POINT p;
	GEOGRAPHIC_EDGE e;

	for ( i = 0; i < lwline->points->npoints - 1; i++)
	{
		const POINT2D* a1 = getPoint2d_cp(lwline->points, i);
		const POINT2D* a2 = getPoint2d_cp(lwline->points, i+1);

		geographic_point_init(a1->x, a1->y, &(e.start));
		geographic_point_init(a2->x, a2->y, &(e.end));

		geographic_point_init(lwpoint_get_x(lwpoint), lwpoint_get_y(lwpoint), &p);

		if ( edge_contains_point(&e, &p) ) {
			return LW_TRUE;
		}
	}

	return LW_FALSE;
}

/**
 * Check if first and last point of line2 are covered by line1 and then each
 * point in between has to be one line1 in the exact same order
 * return LW_TRUE if all edge points of line2 are on line1
 */
int lwline_covers_lwline(const LWLINE* lwline1, const LWLINE* lwline2)
{
	uint32_t i, j;
	GEOGRAPHIC_EDGE e1, e2;
	GEOGRAPHIC_POINT p1, p2;
	int start = LW_FALSE;
	int changed = LW_FALSE;

	/* first point on line */
	if ( ! lwline_covers_lwpoint(lwline1, lwline_get_lwpoint(lwline2, 0)))
	{
		LWDEBUG(4,"returning false, first point of line2 is not covered by line1");
		return LW_FALSE;
	}

	/* last point on line */
	if ( ! lwline_covers_lwpoint(lwline1, lwline_get_lwpoint(lwline2, lwline2->points->npoints - 1)))
	{
		LWDEBUG(4,"returning false, last point of line2 is not covered by line1");
		return LW_FALSE;
	}

	j = 0;
	i = 0;
	while (i < lwline1->points->npoints - 1 && j < lwline2->points->npoints - 1)
	{
		changed = LW_FALSE;
		const POINT2D* a1 = getPoint2d_cp(lwline1->points, i);
		const POINT2D* a2 = getPoint2d_cp(lwline1->points, i+1);
		const POINT2D* b1 = getPoint2d_cp(lwline2->points, j);
		const POINT2D* b2 = getPoint2d_cp(lwline2->points, j+1);

		geographic_point_init(a1->x, a1->y, &(e1.start));
		geographic_point_init(a2->x, a2->y, &(e1.end));
		geographic_point_init(b1->x, b1->y, &p2);

		/* we already know, that the last point is on line1, so we're done */
		if ( j == lwline2->points->npoints - 1)
		{
			return LW_TRUE;
		}
		else if (start == LW_TRUE)
		{
			/* point is on current line1 edge, check next point in line2 */
			if ( edge_contains_point(&e1, &p2)) {
				j++;
				changed = LW_TRUE;
			}

			geographic_point_init(a1->x, a1->y, &(e2.start));
			geographic_point_init(a2->x, b2->y, &(e2.end));
			geographic_point_init(a1->x, a1->y, &p1);

			/* point is on current line2 edge, check next point in line1 */
			if ( edge_contains_point(&e2, &p1)) {
				i++;
				changed = LW_TRUE;
			}

			/* no edge progressed -> point left one line */
			if ( changed == LW_FALSE )
			{
				LWDEBUG(4,"returning false, found point not covered by both lines");
				return LW_FALSE;
			}
			else
			{
				continue;
			}
		}

		/* find first edge to cover line2 */
		if (edge_contains_point(&e1, &p2))
		{
			start = LW_TRUE;
		}

		/* next line1 edge */
		i++;
	}

	/* no uncovered point found */
	return LW_TRUE;
}

int ptarray_calculate_gbox_geodetic(const POINTARRAY *pa, GBOX *gbox)
{
	uint32_t i;
	int first = LW_TRUE;
	const POINT2D *p;
	POINT3D A1, A2;
	GBOX edge_gbox;

	assert(gbox);
	assert(pa);

	gbox_init(&edge_gbox);
	edge_gbox.flags = gbox->flags;

	if ( pa->npoints == 0 ) return LW_FAILURE;

	if ( pa->npoints == 1 )
	{
		p = getPoint2d_cp(pa, 0);
		ll2cart(p, &A1);
		gbox->xmin = gbox->xmax = A1.x;
		gbox->ymin = gbox->ymax = A1.y;
		gbox->zmin = gbox->zmax = A1.z;
		return LW_SUCCESS;
	}

	p = getPoint2d_cp(pa, 0);
	ll2cart(p, &A1);

	for ( i = 1; i < pa->npoints; i++ )
	{

		p = getPoint2d_cp(pa, i);
		ll2cart(p, &A2);

		edge_calculate_gbox(&A1, &A2, &edge_gbox);

		/* Initialize the box */
		if ( first )
		{
			gbox_duplicate(&edge_gbox, gbox);
			first = LW_FALSE;
		}
		/* Expand the box where necessary */
		else
		{
			gbox_merge(&edge_gbox, gbox);
		}

		A1 = A2;
	}

	return LW_SUCCESS;
}

static int lwpoint_calculate_gbox_geodetic(const LWPOINT *point, GBOX *gbox)
{
	assert(point);
	return ptarray_calculate_gbox_geodetic(point->point, gbox);
}

static int lwline_calculate_gbox_geodetic(const LWLINE *line, GBOX *gbox)
{
	assert(line);
	return ptarray_calculate_gbox_geodetic(line->points, gbox);
}

static int lwpolygon_calculate_gbox_geodetic(const LWPOLY *poly, GBOX *gbox)
{
	GBOX ringbox;
	uint32_t i;
	int first = LW_TRUE;
	assert(poly);
	if ( poly->nrings == 0 )
		return LW_FAILURE;
	ringbox.flags = gbox->flags;
	for ( i = 0; i < poly->nrings; i++ )
	{
		if ( ptarray_calculate_gbox_geodetic(poly->rings[i], &ringbox) == LW_FAILURE )
			return LW_FAILURE;
		if ( first )
		{
			gbox_duplicate(&ringbox, gbox);
			first = LW_FALSE;
		}
		else
		{
			gbox_merge(&ringbox, gbox);
		}
	}

	/* If the box wraps a poly, push that axis to the absolute min/max as appropriate */
	gbox_check_poles(gbox);

	return LW_SUCCESS;
}

static int lwtriangle_calculate_gbox_geodetic(const LWTRIANGLE *triangle, GBOX *gbox)
{
	assert(triangle);
	return ptarray_calculate_gbox_geodetic(triangle->points, gbox);
}


static int lwcollection_calculate_gbox_geodetic(const LWCOLLECTION *coll, GBOX *gbox)
{
	GBOX subbox = {0};
	uint32_t i;
	int result = LW_FAILURE;
	int first = LW_TRUE;
	assert(coll);
	if ( coll->ngeoms == 0 )
		return LW_FAILURE;

	subbox.flags = gbox->flags;

	for ( i = 0; i < coll->ngeoms; i++ )
	{
		if ( lwgeom_calculate_gbox_geodetic((LWGEOM*)(coll->geoms[i]), &subbox) == LW_SUCCESS )
		{
			/* Keep a copy of the sub-bounding box for later */
			if ( coll->geoms[i]->bbox )
				lwfree(coll->geoms[i]->bbox);
			coll->geoms[i]->bbox = gbox_copy(&subbox);
			if ( first )
			{
				gbox_duplicate(&subbox, gbox);
				first = LW_FALSE;
			}
			else
			{
				gbox_merge(&subbox, gbox);
			}
			result = LW_SUCCESS;
		}
	}
	return result;
}

int lwgeom_calculate_gbox_geodetic(const LWGEOM *geom, GBOX *gbox)
{
	int result = LW_FAILURE;
	LWDEBUGF(4, "got type %d", geom->type);

	/* Add a geodetic flag to the incoming gbox */
	gbox->flags = lwflags(FLAGS_GET_Z(geom->flags),FLAGS_GET_M(geom->flags),1);

	switch (geom->type)
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
	case TRIANGLETYPE:
		result = lwtriangle_calculate_gbox_geodetic((LWTRIANGLE *)geom, gbox);
		break;
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case POLYHEDRALSURFACETYPE:
	case TINTYPE:
	case COLLECTIONTYPE:
		result = lwcollection_calculate_gbox_geodetic((LWCOLLECTION *)geom, gbox);
		break;
	default:
		lwerror("lwgeom_calculate_gbox_geodetic: unsupported input geometry type: %d - %s",
		        geom->type, lwtype_name(geom->type));
		break;
	}
	return result;
}



static int ptarray_check_geodetic(const POINTARRAY *pa)
{
	uint32_t t;
	POINT2D pt;

	assert(pa);

	for (t=0; t<pa->npoints; t++)
	{
		getPoint2d_p(pa, t, &pt);
		/* printf( "%d (%g, %g)\n", t, pt.x, pt.y); */
		if ( pt.x < -180.0 || pt.y < -90.0 || pt.x > 180.0 || pt.y > 90.0 )
			return LW_FALSE;
	}

	return LW_TRUE;
}

static int lwpoint_check_geodetic(const LWPOINT *point)
{
	assert(point);
	return ptarray_check_geodetic(point->point);
}

static int lwline_check_geodetic(const LWLINE *line)
{
	assert(line);
	return ptarray_check_geodetic(line->points);
}

static int lwpoly_check_geodetic(const LWPOLY *poly)
{
	uint32_t i = 0;
	assert(poly);

	for ( i = 0; i < poly->nrings; i++ )
	{
		if ( ptarray_check_geodetic(poly->rings[i]) == LW_FALSE )
			return LW_FALSE;
	}
	return LW_TRUE;
}

static int lwtriangle_check_geodetic(const LWTRIANGLE *triangle)
{
	assert(triangle);
	return ptarray_check_geodetic(triangle->points);
}


static int lwcollection_check_geodetic(const LWCOLLECTION *col)
{
	uint32_t i = 0;
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
	if ( lwgeom_is_empty(geom) )
		return LW_TRUE;

	switch (geom->type)
	{
	case POINTTYPE:
		return lwpoint_check_geodetic((LWPOINT *)geom);
	case LINETYPE:
		return lwline_check_geodetic((LWLINE *)geom);
	case POLYGONTYPE:
		return lwpoly_check_geodetic((LWPOLY *)geom);
	case TRIANGLETYPE:
		return lwtriangle_check_geodetic((LWTRIANGLE *)geom);
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case POLYHEDRALSURFACETYPE:
	case TINTYPE:
	case COLLECTIONTYPE:
		return lwcollection_check_geodetic((LWCOLLECTION *)geom);
	default:
		lwerror("lwgeom_check_geodetic: unsupported input geometry type: %d - %s",
		        geom->type, lwtype_name(geom->type));
	}
	return LW_FALSE;
}

static int ptarray_force_geodetic(POINTARRAY *pa)
{
	uint32_t t;
	int changed = LW_FALSE;
	POINT4D pt;

	assert(pa);

	for ( t=0; t < pa->npoints; t++ )
	{
		getPoint4d_p(pa, t, &pt);
		if ( pt.x < -180.0 || pt.x > 180.0 || pt.y < -90.0 || pt.y > 90.0 )
		{
			pt.x = longitude_degrees_normalize(pt.x);
			pt.y = latitude_degrees_normalize(pt.y);
			ptarray_set_point4d(pa, t, &pt);
			changed = LW_TRUE;
		}
	}
	return changed;
}

static int lwpoint_force_geodetic(LWPOINT *point)
{
	assert(point);
	return ptarray_force_geodetic(point->point);
}

static int lwline_force_geodetic(LWLINE *line)
{
	assert(line);
	return ptarray_force_geodetic(line->points);
}

static int lwpoly_force_geodetic(LWPOLY *poly)
{
	uint32_t i = 0;
	int changed = LW_FALSE;
	assert(poly);

	for ( i = 0; i < poly->nrings; i++ )
	{
		if ( ptarray_force_geodetic(poly->rings[i]) == LW_TRUE )
			changed = LW_TRUE;
	}
	return changed;
}

static int lwcollection_force_geodetic(LWCOLLECTION *col)
{
	uint32_t i = 0;
	int changed = LW_FALSE;
	assert(col);

	for ( i = 0; i < col->ngeoms; i++ )
	{
		if ( lwgeom_force_geodetic(col->geoms[i]) == LW_TRUE )
			changed = LW_TRUE;
	}
	return changed;
}

int lwgeom_force_geodetic(LWGEOM *geom)
{
	switch ( lwgeom_get_type(geom) )
	{
		case POINTTYPE:
			return lwpoint_force_geodetic((LWPOINT *)geom);
		case LINETYPE:
			return lwline_force_geodetic((LWLINE *)geom);
		case POLYGONTYPE:
			return lwpoly_force_geodetic((LWPOLY *)geom);
		case MULTIPOINTTYPE:
		case MULTILINETYPE:
		case MULTIPOLYGONTYPE:
		case COLLECTIONTYPE:
			return lwcollection_force_geodetic((LWCOLLECTION *)geom);
		default:
			lwerror("unsupported input geometry type: %d", lwgeom_get_type(geom));
	}
	return LW_FALSE;
}


double ptarray_length_spheroid(const POINTARRAY *pa, const SPHEROID *s)
{
	GEOGRAPHIC_POINT a, b;
	double za = 0.0, zb = 0.0;
	POINT4D p;
	uint32_t i;
	int hasz = LW_FALSE;
	double length = 0.0;
	double seglength = 0.0;

	/* Return zero on non-sensical inputs */
	if ( ! pa || pa->npoints < 2 )
		return 0.0;

	/* See if we have a third dimension */
	hasz = FLAGS_GET_Z(pa->flags);

	/* Initialize first point */
	getPoint4d_p(pa, 0, &p);
	geographic_point_init(p.x, p.y, &a);
	if ( hasz )
		za = p.z;

	/* Loop and sum the length for each segment */
	for ( i = 1; i < pa->npoints; i++ )
	{
		seglength = 0.0;
		getPoint4d_p(pa, i, &p);
		geographic_point_init(p.x, p.y, &b);
		if ( hasz )
			zb = p.z;

		/* Special sphere case */
		if ( s->a == s->b )
			seglength = s->radius * sphere_distance(&a, &b);
		/* Spheroid case */
		else
			seglength = spheroid_distance(&a, &b, s);

		/* Add in the vertical displacement if we're in 3D */
		if ( hasz )
			seglength = sqrt( (zb-za)*(zb-za) + seglength*seglength );

		/* Add this segment length to the total */
		length += seglength;

		/* B gets incremented in the next loop, so we save the value here */
		a = b;
		za = zb;
	}
	return length;
}

double lwgeom_length_spheroid(const LWGEOM *geom, const SPHEROID *s)
{
	int type;
	uint32_t i = 0;
	double length = 0.0;

	assert(geom);

	/* No area in nothing */
	if ( lwgeom_is_empty(geom) )
		return 0.0;

	type = geom->type;

	if ( type == POINTTYPE || type == MULTIPOINTTYPE )
		return 0.0;

	if ( type == LINETYPE )
		return ptarray_length_spheroid(((LWLINE*)geom)->points, s);

	if ( type == POLYGONTYPE )
	{
		LWPOLY *poly = (LWPOLY*)geom;
		for ( i = 0; i < poly->nrings; i++ )
		{
			length += ptarray_length_spheroid(poly->rings[i], s);
		}
		return length;
	}

	if ( type == TRIANGLETYPE )
		return ptarray_length_spheroid(((LWTRIANGLE*)geom)->points, s);

	if ( lwtype_is_collection( type ) )
	{
		LWCOLLECTION *col = (LWCOLLECTION*)geom;

		for ( i = 0; i < col->ngeoms; i++ )
		{
			length += lwgeom_length_spheroid(col->geoms[i], s);
		}
		return length;
	}

	lwerror("unsupported type passed to lwgeom_length_sphere");
	return 0.0;
}

/**
* When features are snapped or sometimes they are just this way, they are very close to
* the geodetic bounds but slightly over. This routine nudges those points, and only
* those points, back over to the bounds.
* http://trac.osgeo.org/postgis/ticket/1292
*/
static int
ptarray_nudge_geodetic(POINTARRAY *pa)
{

	uint32_t i;
	POINT4D p;
	int altered = LW_FALSE;
	int rv = LW_FALSE;
	static double tolerance = 1e-10;

	if ( ! pa )
		lwerror("ptarray_nudge_geodetic called with null input");

	for(i = 0; i < pa->npoints; i++ )
	{
		getPoint4d_p(pa, i, &p);
		if ( p.x < -180.0 && (-180.0 - p.x <= tolerance) )
		{
			p.x = -180.0;
			altered = LW_TRUE;
		}
		if ( p.x > 180.0 && (p.x - 180.0 <= tolerance) )
		{
			p.x = 180.0;
			altered = LW_TRUE;
		}
		if ( p.y < -90.0 && (-90.0 - p.y <= tolerance) )
		{
			p.y = -90.0;
			altered = LW_TRUE;
		}
		if ( p.y > 90.0 && (p.y - 90.0 <= tolerance) )
		{
			p.y = 90.0;
			altered = LW_TRUE;
		}
		if ( altered == LW_TRUE )
		{
			ptarray_set_point4d(pa, i, &p);
			altered = LW_FALSE;
			rv = LW_TRUE;
		}
	}
	return rv;
}

/**
* When features are snapped or sometimes they are just this way, they are very close to
* the geodetic bounds but slightly over. This routine nudges those points, and only
* those points, back over to the bounds.
* http://trac.osgeo.org/postgis/ticket/1292
*/
int
lwgeom_nudge_geodetic(LWGEOM *geom)
{
	int type;
	uint32_t i = 0;
	int rv = LW_FALSE;

	assert(geom);

	/* No points in nothing */
	if ( lwgeom_is_empty(geom) )
		return LW_FALSE;

	type = geom->type;

	if ( type == POINTTYPE )
		return ptarray_nudge_geodetic(((LWPOINT*)geom)->point);

	if ( type == LINETYPE )
		return ptarray_nudge_geodetic(((LWLINE*)geom)->points);

	if ( type == POLYGONTYPE )
	{
		LWPOLY *poly = (LWPOLY*)geom;
		for ( i = 0; i < poly->nrings; i++ )
		{
			int n = ptarray_nudge_geodetic(poly->rings[i]);
			rv = (rv == LW_TRUE ? rv : n);
		}
		return rv;
	}

	if ( type == TRIANGLETYPE )
		return ptarray_nudge_geodetic(((LWTRIANGLE*)geom)->points);

	if ( lwtype_is_collection( type ) )
	{
		LWCOLLECTION *col = (LWCOLLECTION*)geom;

		for ( i = 0; i < col->ngeoms; i++ )
		{
			int n = lwgeom_nudge_geodetic(col->geoms[i]);
			rv = (rv == LW_TRUE ? rv : n);
		}
		return rv;
	}

	lwerror("unsupported type (%s) passed to lwgeom_nudge_geodetic", lwtype_name(type));
	return rv;
}


/**
* Utility function for checking if P is within the cone defined by A1/A2.
*/
static int
point_in_cone(const POINT3D *A1, const POINT3D *A2, const POINT3D *P)
{
	POINT3D AC; /* Center point of A1/A2 */
	double min_similarity, similarity;

	/* Boundary case */
	if (point3d_equals(A1, P) || point3d_equals(A2, P))
		return LW_TRUE;

	/* The normalized sum bisects the angle between start and end. */
	vector_sum(A1, A2, &AC);
	normalize(&AC);

	/* The projection of start onto the center defines the minimum similarity */
	min_similarity = dot_product(A1, &AC);

	/* If the edge is sufficiently curved, use the dot product test */
	if (fabs(1.0 - min_similarity) > 1e-10)
	{
		/* The projection of candidate p onto the center */
		similarity = dot_product(P, &AC);

		/* If the projection of the candidate is larger than */
		/* the projection of the start point, the candidate */
		/* must be closer to the center than the start, so */
		/* therefore inside the cone */
		if (similarity > min_similarity)
		{
			return LW_TRUE;
		}
		else
		{
			return LW_FALSE;
		}
	}
	else
	{
		/* Where the edge is very narrow, the dot product test */
		/* fails, but we can use the almost-planar nature of the */
		/* problem space then to test if the vector from the */
		/* candidate to the start point in a different direction */
		/* to the vector from candidate to end point */
		/* If so, then candidate is between start and end */
		POINT3D PA1, PA2;
		vector_difference(P, A1, &PA1);
		vector_difference(P, A2, &PA2);
		normalize(&PA1);
		normalize(&PA2);
		if (dot_product(&PA1, &PA2) < 0.0)
		{
			return LW_TRUE;
		}
		else
		{
			return LW_FALSE;
		}
	}
	return LW_FALSE;
}



/**
* Utility function for edge_intersects(), signum with a tolerance
* in determining if the value is zero.
*/
static int
dot_product_side(const POINT3D *p, const POINT3D *q)
{
	double dp = dot_product(p, q);

	if ( FP_IS_ZERO(dp) )
		return 0;

	return dp < 0.0 ? -1 : 1;
}

/**
* Returns non-zero if edges A and B interact. The type of interaction is given in the
* return value with the bitmask elements defined above.
*/
uint32_t
edge_intersects(const POINT3D *A1, const POINT3D *A2, const POINT3D *B1, const POINT3D *B2)
{
	POINT3D AN, BN, VN;  /* Normals to plane A and plane B */
	double ab_dot;
	int a1_side, a2_side, b1_side, b2_side;
	int rv = PIR_NO_INTERACT;

	/* Normals to the A-plane and B-plane */
	unit_normal(A1, A2, &AN);
	unit_normal(B1, B2, &BN);

	/* Are A-plane and B-plane basically the same? */
	ab_dot = dot_product(&AN, &BN);

	/*
	* https://trac.osgeo.org/postgis/ticket/5765
	* Failure because the colinearity check was
	* triggering due to an overly loose equality
	* check here.
	* if ( FP_EQUALS(fabs(ab_dot), 1.0) )
	*/
	if ( 1.0 - fabs(ab_dot) <= 10e-16 )
	{
		/* Co-linear case */
		if ( point_in_cone(A1, A2, B1) || point_in_cone(A1, A2, B2) ||
		     point_in_cone(B1, B2, A1) || point_in_cone(B1, B2, A2) )
		{
			rv |= PIR_INTERSECTS;
			rv |= PIR_COLINEAR;
		}
		return rv;
	}

	/* What side of plane-A and plane-B do the end points */
	/* of A and B fall? */
	a1_side = dot_product_side(&BN, A1);
	a2_side = dot_product_side(&BN, A2);
	b1_side = dot_product_side(&AN, B1);
	b2_side = dot_product_side(&AN, B2);

	/* Both ends of A on the same side of plane B. */
	if ( a1_side == a2_side && a1_side != 0 )
	{
		/* No intersection. */
		return PIR_NO_INTERACT;
	}

	/* Both ends of B on the same side of plane A. */
	if ( b1_side == b2_side && b1_side != 0 )
	{
		/* No intersection. */
		return PIR_NO_INTERACT;
	}

	/* A straddles B and B straddles A, so... */
	if ( a1_side != a2_side && (a1_side + a2_side) == 0 &&
	     b1_side != b2_side && (b1_side + b2_side) == 0 )
	{
		/* Have to check if intersection point is inside both arcs */
		unit_normal(&AN, &BN, &VN);
		if ( point_in_cone(A1, A2, &VN) && point_in_cone(B1, B2, &VN) )
		{
			return PIR_INTERSECTS;
		}

		/* Have to check if intersection point is inside both arcs */
		vector_scale(&VN, -1);
		if ( point_in_cone(A1, A2, &VN) && point_in_cone(B1, B2, &VN) )
		{
			return PIR_INTERSECTS;
		}

		return PIR_NO_INTERACT;
	}

	/* The rest are all intersects variants... */
	rv |= PIR_INTERSECTS;

	/* A touches B */
	if ( a1_side == 0 )
	{
		/* Touches at A1, A2 is on what side? */
		rv |= (a2_side < 0 ? PIR_A_TOUCH_RIGHT : PIR_A_TOUCH_LEFT);
	}
	else if ( a2_side == 0 )
	{
		/* Touches at A2, A1 is on what side? */
		rv |= (a1_side < 0 ? PIR_A_TOUCH_RIGHT : PIR_A_TOUCH_LEFT);
	}

	/* B touches A */
	if ( b1_side == 0 )
	{
		/* Touches at B1, B2 is on what side? */
		rv |= (b2_side < 0 ? PIR_B_TOUCH_RIGHT : PIR_B_TOUCH_LEFT);
	}
	else if ( b2_side == 0 )
	{
		/* Touches at B2, B1 is on what side? */
		rv |= (b1_side < 0 ? PIR_B_TOUCH_RIGHT : PIR_B_TOUCH_LEFT);
	}

	return rv;
}

/**
* This routine returns LW_TRUE if the stabline joining the pt_outside and pt_to_test
* crosses the ring an odd number of times, or if the pt_to_test is on the ring boundary itself,
* returning LW_FALSE otherwise.
* The pt_outside *must* be guaranteed to be outside the ring (use the geography_pt_outside() function
* to derive one in postgis, or the gbox_pt_outside() function if you don't mind burning CPU cycles
* building a gbox first).
*/
int ptarray_contains_point_sphere(const POINTARRAY *pa, const POINT2D *pt_outside, const POINT2D *pt_to_test)
{
	POINT3D S1, S2; /* Stab line end points */
	POINT3D E1, E2; /* Edge end points (3-space) */
	POINT2D p; /* Edge end points (lon/lat) */
	uint32_t count = 0, i, inter;

	/* Null input, not enough points for a ring? You ain't closed! */
	if ( ! pa || pa->npoints < 4 )
		return LW_FALSE;

	/* Set up our stab line */
	ll2cart(pt_to_test, &S1);
	ll2cart(pt_outside, &S2);

	/* Initialize first point */
	getPoint2d_p(pa, 0, &p);
	ll2cart(&p, &E1);

	/* Walk every edge and see if the stab line hits it */
	for ( i = 1; i < pa->npoints; i++ )
	{
		LWDEBUGF(4, "testing edge (%d)", i);
		LWDEBUGF(4, "  start point == POINT(%.12g %.12g)", p.x, p.y);

		/* Read next point. */
		getPoint2d_p(pa, i, &p);
		ll2cart(&p, &E2);

		/* Skip over too-short edges. */
		if ( point3d_equals(&E1, &E2) )
		{
			continue;
		}

		/* Our test point is on an edge end! Point is "in ring" by our definition */
		if ( point3d_equals(&S1, &E1) )
		{
			return LW_TRUE;
		}

		/* Calculate relationship between stab line and edge */
		inter = edge_intersects(&S1, &S2, &E1, &E2);

		/* We have some kind of interaction... */
		if ( inter & PIR_INTERSECTS )
		{
			/* If the stabline is touching the edge, that implies the test point */
			/* is on the edge, so we're done, the point is in (on) the ring. */
			if ( (inter & PIR_A_TOUCH_RIGHT) || (inter & PIR_A_TOUCH_LEFT) )
			{
				return LW_TRUE;
			}

			/* It's a touching interaction, disregard all the left-side ones. */
			/* It's a co-linear intersection, ignore those. */
			if ( inter & PIR_B_TOUCH_RIGHT || inter & PIR_COLINEAR )
			{
				/* Do nothing, to avoid double counts. */
				LWDEBUGF(4,"    edge (%d) crossed, count == %d, disregarding to avoid double count", i, count);
			}
			else
			{
				/* Increment crossingn count. */
				count++;
				LWDEBUGF(4,"    edge (%d) crossed, count == %d", i, count);
			}
		}
		else
		{
			LWDEBUGF(4,"    edge (%d) did not cross", i);
		}

		/* Increment to next edge */
		E1 = E2;
	}

	LWDEBUGF(4,"final count == %d", count);

	/* An odd number of crossings implies containment! */
	if ( count % 2 )
	{
		return LW_TRUE;
	}

	return LW_FALSE;
}


/*
* Given a geodetic bounding volume, calculate a lon/lat bounding
* box that should contain the original feature that gave rise to
* the geodetic box, in plate-carre space (planar lon/lat).
*/
int gbox_geocentric_get_gbox_cartesian(const GBOX *gbox_geocentric, GBOX *gbox_planar)
{
	/* Normalized corners of the bounding volume */
	POINT3D corners[8];
	POINT3D cap_center = {0,0,0};
	double furthest_angle = 0.0;
	double cap_angle = 0.0;
	int all_longitudes = LW_FALSE;
	double lon0 = -M_PI, lon1 = M_PI;
	double lat0, lat1;
	GEOGRAPHIC_POINT cap_center_g;

	if (!gbox_geocentric || !gbox_planar)
	{
		lwerror("Null pointer passed to %s", __func__);
		return LW_FALSE;
	}

#define	CORNER_SET(ii, xx, yy, zz) { \
	corners[ii].x = gbox_geocentric->xx; \
	corners[ii].y = gbox_geocentric->yy; \
	corners[ii].z = gbox_geocentric->zz; \
	}

	/*
	* First find a "centered" vector to serve as the mid-point
	* of the input bounding volume.
	*/
	CORNER_SET(0, xmin, ymin, zmin);
	CORNER_SET(1, xmax, ymin, zmin);
	CORNER_SET(2, xmin, ymax, zmin);
	CORNER_SET(3, xmax, ymax, zmin);
	CORNER_SET(4, xmin, ymin, zmax);
	CORNER_SET(5, xmax, ymin, zmax);
	CORNER_SET(6, xmin, ymax, zmax);
	CORNER_SET(7, xmax, ymax, zmax);

	/*
	* Normalize the volume corners
	* and normalize the final vector.
	*/
	for (uint32_t i = 0; i < 8; i++)
	{
		normalize(&(corners[i]));
		cap_center.x += corners[i].x;
		cap_center.y += corners[i].y;
		cap_center.z += corners[i].z;
	}
	normalize(&cap_center);

	/*
	* Find the volume corner that is furthest from the center,
	* and calculate the angle between the center and the corner.
	* Now we have a "cap" (center and angle)
	*/
	for (uint32_t i = 0; i < 8; i++)
	{
		double angle = vector_angle(&cap_center, &(corners[i]));
		if (angle > furthest_angle)
			furthest_angle = angle;
	}
	cap_angle = furthest_angle;

	/*
	* Calculate the planar box that contains the cap.
	* If the cap contains a pole, then we include all longitudes
	*/
	cart2geog(&cap_center, &cap_center_g);

	/* Check whether cap includes the south pole */
	lat0 = cap_center_g.lat - cap_angle;
	if (lat0 <= -M_PI_2)
	{
		lat0 = -M_PI_2;
		all_longitudes = LW_TRUE;
	}

	/* Check whether cap includes the north pole */
	lat1 = cap_center_g.lat + cap_angle;
	if (lat1 >= M_PI_2)
	{
		lat1 = M_PI_2;
		all_longitudes = LW_TRUE;
	}

	if (!all_longitudes)
	{
		// Compute the range of longitudes covered by the cap.  We use the law
		// of sines for spherical triangles.  Consider the triangle ABC where
		// A is the north pole, B is the center of the cap, and C is the point
		// of tangency between the cap boundary and a line of longitude.  Then
		// C is a right angle, and letting a,b,c denote the sides opposite A,B,C,
		// we have sin(a)/sin(A) = sin(c)/sin(C), or sin(A) = sin(a)/sin(c).
		// Here "a" is the cap angle, and "c" is the colatitude (90 degrees
		// minus the latitude).  This formula also works for negative latitudes.
		//
		// The formula for sin(a) follows from the relationship h = 1 - cos(a).

		double sin_a = sin(cap_angle);
		double sin_c = cos(cap_center_g.lat);
		if (sin_a <= sin_c)
		{
			double angle_A = asin(sin_a / sin_c);
			lon0 = remainder(cap_center_g.lon - angle_A, 2 * M_PI);
			lon1 = remainder(cap_center_g.lon + angle_A, 2 * M_PI);
		}
		else
		{
			lon0 = -M_PI;
			lon1 =  M_PI;
		}
	}

	gbox_planar->xmin = rad2deg(lon0);
	gbox_planar->ymin = rad2deg(lat0);
	gbox_planar->xmax = rad2deg(lon1);
	gbox_planar->ymax = rad2deg(lat1);
	FLAGS_SET_GEODETIC(gbox_planar->flags, 0);
	FLAGS_SET_Z(gbox_planar->flags, 0);
	FLAGS_SET_M(gbox_planar->flags, 0);

	return LW_TRUE;
}

