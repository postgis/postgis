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
* Convert spherical coordinates to cartesion coordinates on unit sphere
*/
void inline geog2cart(GEOGRAPHIC_POINT g, POINT3D *p)
{
	p->x = cos(g.lat) * cos(g.lon);
	p->y = cos(g.lat) * sin(g.lon);
	p->z = sin(g.lat);
}

/**
* Convert cartesion coordinates to spherical coordinates on unit sphere
*/
void inline cart2geog(POINT3D p, GEOGRAPHIC_POINT *g)
{
	g->lon = atan2(p.x, p.y);
	g->lat = asin(p.z);
}

/** 
* Calculate the dot product of two unit vectors
*/
double inline dot_product(POINT3D p1, POINT3D p2)
{
	return (p1.x*p2.x) + (p1.y*p2.y) + (p1.z*p2.z);
}

void inline unit_normal(POINT3D a, POINT3D b, POINT3D *n)
{
	double d;
	double x = a.y * b.z - a.z * b.y;
	double y = a.z * b.x - a.x * b.z;
	double z = a.x * b.y - a.y * b.x;
	d = sqrt(x*x + y*y + z*z);
	n->x = x/d;
	n->y = y/d;
	n->z = z/d;
	return;
}

/**
* Normalize to a unit vector.
*/
void normalize(POINT3D *p)
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
}

/**
* Computes the cross product of two unit vectors using their lat, lng representations.
* Good for small distances between p and q.
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
		return LW_FALSE;
	return LW_TRUE;
}

/**
* True if the longitude of p is within the range of the longitude of the ends of e
*/
int edge_contains_longitude(GEOGRAPHIC_EDGE e, GEOGRAPHIC_POINT p, int flipped_longitude)
{
	if( flipped_longitude )
	{
		if( p.lon > e.start.lon && p.lon > e.end.lon )
			return LW_TRUE;
		if( p.lon < e.start.lon && p.lon < e.end.lon )
			return LW_TRUE;
	}
	else
	{
		if( e.start.lon < p.lon && p.lon < e.end.lon )
			return LW_TRUE;
		if( e.end.lon < p.lon && p.lon < e.start.lon )
			return LW_TRUE;
	}
	return LW_FALSE;
}


/**
* Given two points on a unit sphere, calculate their distance apart. 
*/
double sphere_distance(GEOGRAPHIC_POINT s, GEOGRAPHIC_POINT e)
{
	double latS = s.lat;
	double latE = e.lon;
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
	                 sin(latS) * cos(latE) * cos(dlng)) / PI; 
	return heading;
}

/**
* Returns true if the point p is on the minor edge defined by the
* end points of e.
*/
int edge_contains_point(GEOGRAPHIC_EDGE e, GEOGRAPHIC_POINT p, int flipped_longitude)
{
	if( edge_point_on_plane(e, p) && edge_contains_longitude(e, p, flipped_longitude) )
		return LW_TRUE;
	return LW_FALSE;
}

/**
* Used in great circle to compute the pole of the great circle.
*/
double z_to_latitude(double z)
{
	double sign = signum(z);
	double tlat = acos(z);
	if(fabs(tlat) > (PI/2.0) )
	{
		tlat = sign * (PI - fabs(tlat));
	} 
	else 
	{
		tlat = sign * tlat;
	}
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
	unit_normal(start, end, &t1);
	unit_normal(end, start, &t2);
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
    robust_cross_product(start, end, &t1);
    robust_cross_product(end, start, &t2);
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



int edge_calculate_gbox(GEOGRAPHIC_EDGE e, GBOX *gbox)
{
	double deltaLongitude;
	double distance = sphere_distance(e.start, e.end);
	int flipped_longitude = LW_FALSE;
    int gimbal_lock = LW_FALSE;
	POINT3D p, start, end, startXZ, endXZ, startYZ, endYZ, nT1B, nT2B;
	GEOGRAPHIC_EDGE g;
	GEOGRAPHIC_POINT vT1, vT2;

	/* Initialize our working copy of the edge */
	g = e;

	/* Initialize box with the start and end points of the edge. */
	geog2cart(g.start, &start);
	geog2cart(g.end, &end);
	gbox->xmin = FP_MIN(start.x, end.x);
	gbox->ymin = FP_MIN(start.y, end.y);
	gbox->zmin = FP_MIN(start.z, end.z);
	gbox->xmax = FP_MAX(start.x, end.x);
	gbox->ymax = FP_MAX(start.y, end.y);
	gbox->zmax = FP_MAX(start.z, end.z);

	/* Edge is zero length, just return the naive box */
	if( FP_IS_ZERO(distance) )
		return G_SUCCESS;

	/* Edge is antipodal (one point on each side of the globe), 
	   set the box to contain the whole world and return */
	if( FP_EQUALS(distance, PI) )
	{
		gbox->xmin = gbox->ymin = gbox->zmin = -1.0;
		gbox->xmax = gbox->ymax = gbox->zmax = 1.0;
		return G_SUCCESS;
	}

	/* Calculate the difference in longitude between the two points. */
	if( signum(g.start.lon) == signum(g.end.lon) )
	{
		deltaLongitude = fabs(g.start.lon - g.end.lon);
	}
	else
	{
		double dl = fabs(g.start.lon) + fabs(g.end.lon);
		/* Less then a hemisphere apart */
		if( dl < PI )
		{
			deltaLongitude = dl;
		}
		/* Exactly a hemisphere apart */
		else if ( FP_EQUALS( dl, PI ) )
		{
			deltaLongitude = PI;
		}
		/* More than a hemisphere apart, return the other half of the sphere 
		   and note that we are crossing the dateline */
		else
		{
			flipped_longitude = LW_TRUE;
			deltaLongitude = dl - PI;
		}
	}	
	
	/* If we are crossing the dateline, flip the calculation to the other
	   side of the globe. We'll flip our output box back at the end of the
	   calculation. */
	if ( flipped_longitude )
	{
		if ( g.start.lon > 0.0 ) 
			g.start.lon -= PI;
		else 
			g.start.lon += PI;
		if ( g.end.lon > 0.0 ) 
			g.end.lon -= PI;
		else 
			g.end.lon += PI;
	}
	
	/* Check for pole crossings. */
	if( FP_EQUALS(deltaLongitude, PI) ) 
	{
		/* Crosses the north pole, adjust box to contain pole */
		if( (g.start.lat + g.end.lat) > 0.0 )
		{
			gbox->zmax = 1.0;
		} 
		/* Crosses the south pole, adjust box to contain pole */
		else 
		{
			gbox->zmin = -1.0;
		}
	}
	/* How about maximal latitudes in this great circle. Are any
	   of them contained within this arc? */
	else
	{
		clairaut_cartesian(start, end, LW_TRUE, &vT1);
		clairaut_cartesian(start, end, LW_FALSE, &vT2);
		if( edge_contains_point(g, vT1, flipped_longitude) )
		{
			geog2cart(vT1, &p);
			gbox_merge_point3d(gbox, &p);
		}
		else if ( edge_contains_point(g, vT2, flipped_longitude) )
		{
			geog2cart(vT2, &p);
			gbox_merge_point3d(gbox, &p);
		}
	}

    /* Flip the X axis to Z and check for maximal latitudes again. */
    startXZ = start;
    x_to_z(&startXZ);
    endXZ = end;
    x_to_z(&endXZ);
	clairaut_cartesian(startXZ, endXZ, LW_TRUE, &vT1);
	clairaut_cartesian(startXZ, endXZ, LW_FALSE, &vT2);
    gimbal_lock = LW_FALSE;
    if ( FP_IS_ZERO(vT1.lat) ) 
    {
        gimbal_lock = LW_TRUE;
    }
    geog2cart(vT1, &nT1B);
    geog2cart(vT2, &nT2B);
    x_to_z(&nT1B);
    x_to_z(&nT2B);
    cart2geog(nT1B, &vT1);
    cart2geog(nT2B, &vT2);
    if( gimbal_lock )
    {
        vT1.lon = PI;
        vT2.lon = -1.0 * PI;
    }
    if( edge_contains_point(g, vT1, flipped_longitude) )
    {
			geog2cart(vT1, &p);
			gbox_merge_point3d(gbox, &p);
    }
	else if ( edge_contains_point(g, vT2, flipped_longitude) )
	{
		geog2cart(vT2, &p);
		gbox_merge_point3d(gbox, &p);
	}
    
    /* Flip the Y axis to Z and check for maximal latitudes again. */
    startYZ = start;
    y_to_z(&startYZ);
    endYZ = end;
    y_to_z(&endYZ);
	clairaut_cartesian(startYZ, endYZ, LW_TRUE, &vT1);
	clairaut_cartesian(startYZ, endYZ, LW_FALSE, &vT2);
    gimbal_lock = LW_FALSE;
    if ( FP_IS_ZERO(vT1.lat) ) 
    {
        gimbal_lock = LW_TRUE;
    }
    geog2cart(vT1, &nT1B);
    geog2cart(vT2, &nT2B);
    y_to_z(&nT1B);
    y_to_z(&nT2B);
    cart2geog(nT1B, &vT1);
    cart2geog(nT2B, &vT2);
    if( gimbal_lock )
    {
        vT1.lon = PI;
        vT2.lon = -1.0 * PI;
    }
    if( edge_contains_point(g, vT1, flipped_longitude) )
    {
			geog2cart(vT1, &p);
			gbox_merge_point3d(gbox, &p);
    }
	else if ( edge_contains_point(g, vT2, flipped_longitude) )
	{
		geog2cart(vT2, &p);
		gbox_merge_point3d(gbox, &p);
	}

    /* Our cartesian gbox is complete! 
       If we flipped our longitudes, now we have to flip our cartesian box. */
    if (flipped_longitude) 
    {
        double tmp;
        tmp = gbox->xmax;
        gbox->xmax = -1.0 * gbox->xmin;
        gbox->xmin = -1.0 * tmp;
        tmp = gbox->ymax;
        gbox->ymax = -1.0 * gbox->ymin;
        gbox->ymin = -1.0 * tmp;
    }
    
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
	//static double wgs84_radius = 6371008.7714;
	int i;
/*	int pt_spacing; */
	double *sphericalcoords;
	POINT2D *pt;

	assert(gbox);
	assert(pa);
	
/*	pt_spacing = 2;
	if( TYPE_HASZ(pa->dims) ) pt_spacing++;
	if( TYPE_HASM(pa->dims) ) pt_spacing++;
*/
	
	if ( pa->npoints == 0 ) return G_FAILURE;
	
	sphericalcoords = (double*)(pa->serialized_pointlist);
	
	for( i = 0; i < pa->npoints; i++ )
	{
		getPoint2d_p_ro(pa, i, &pt);
		/* Convert to radians */
		/* register double theta = sphericalcoords[pt_spacing*i] * PI / 180.0; */
		/* register double phi = sphericalcoords[pt_spacing*i + 1] * PI / 180; */
		register double theta = pt->x * PI / 180.0;
		register double phi = pt->y * PI / 180;
		/* Convert to geocentric */
		register double x = cos(theta);
		register double y = sin(theta);
		register double z = sin(phi);
		/* Initialize the box */
		if( i == 0 )
		{ 
			gbox->xmin = gbox->xmax = x;
			gbox->ymin = gbox->ymax = y;
			gbox->zmin = gbox->zmax = z;
			gbox->mmin = gbox->mmax = 0.0;
		}
		/* Expand the box where necessary */
		if( x < gbox->xmin ) gbox->xmin = x;
		if( y < gbox->ymin ) gbox->ymin = y;
		if( z < gbox->zmin ) gbox->zmin = z;
		if( x > gbox->xmax ) gbox->xmax = x;
		if( y > gbox->ymax ) gbox->ymax = y;
		if( z > gbox->zmax ) gbox->zmax = z;
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
			gbox_duplicate(gbox, &ringbox);
			first = LW_FALSE;
		}
		else
		{
			gbox_merge(gbox, &ringbox);
		}
	}
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
				gbox_duplicate(gbox, &subbox);
				first = LW_FALSE;
			}
			else
			{
				gbox_merge(gbox, &subbox);
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
	POINT2D *pt;

	assert(pa);

	for (t=0; t<pa->npoints; t++)
	{
		getPoint2d_p_ro(pa, t, &pt);
		//printf( "%d (%g, %g)\n", t, pt->x, pt->y);
		if ( pt->x < -180.0 || pt->y < -90.0 || pt->x > 180.0 || pt->y > 90.0 )
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








