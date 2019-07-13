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
 * Copyright (C) 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright (C) 2009 David Skea <David.Skea@gov.bc.ca>
 *
 **********************************************************************/


#include "liblwgeom_internal.h"
#include "lwgeodetic.h"
#include "lwgeom_log.h"
#include <geodesic.h>

/**
* Initialize spheroid object based on major and minor axis
*/
void spheroid_init(SPHEROID *s, double a, double b)
{
	s->a = a;
	s->b = b;
	s->f = (a - b) / a;
	s->e_sq = (a*a - b*b)/(a*a);
	s->radius = (2.0 * a + b ) / 3.0;
}

/**
* Computes the shortest distance along the surface of the spheroid
* between two points, using the inverse geodesic problem from
* GeographicLib (Karney 2013).
*
* @param a - location of first point
* @param b - location of second point
* @param s - spheroid to calculate on
* @return spheroidal distance between a and b in spheroid units
*/
double spheroid_distance(const GEOGRAPHIC_POINT *a, const GEOGRAPHIC_POINT *b, const SPHEROID *spheroid)
{
	struct geod_geodesic gd;
	geod_init(&gd, spheroid->a, spheroid->f);
	double lat1 = a->lat * 180.0 / M_PI;
	double lon1 = a->lon * 180.0 / M_PI;
	double lat2 = b->lat * 180.0 / M_PI;
	double lon2 = b->lon * 180.0 / M_PI;
	double s12 = 0.0; /* return distance */
	geod_inverse(&gd, lat1, lon1, lat2, lon2, &s12, 0, 0);
	return s12;
}

/**
* Computes the forward azimuth of the geodesic joining two points on
* the spheroid, using the inverse geodesic problem (Karney 2013).
*
* @param r - location of first point
* @param s - location of second point
* @return azimuth of line joining r to s (but not reverse)
*/
double spheroid_direction(const GEOGRAPHIC_POINT *a, const GEOGRAPHIC_POINT *b, const SPHEROID *spheroid)
{
	struct geod_geodesic gd;
	geod_init(&gd, spheroid->a, spheroid->f);
	double lat1 = a->lat * 180.0 / M_PI;
	double lon1 = a->lon * 180.0 / M_PI;
	double lat2 = b->lat * 180.0 / M_PI;
	double lon2 = b->lon * 180.0 / M_PI;
	double azi1; /* return azimuth */
	geod_inverse(&gd, lat1, lon1, lat2, lon2, 0, &azi1, 0);
	return azi1 * M_PI / 180.0;
}

/**
* Given a location, an azimuth and a distance, computes the location of
* the projected point. Using the direct geodesic problem from
* GeographicLib (Karney 2013).
*
* @param r - location of first point
* @param distance - distance in meters
* @param azimuth - azimuth in radians
* @return g - location of projected point
*/
int spheroid_project(const GEOGRAPHIC_POINT *r, const SPHEROID *spheroid, double distance, double azimuth, GEOGRAPHIC_POINT *g)
{
	struct geod_geodesic gd;
	geod_init(&gd, spheroid->a, spheroid->f);
	double lat1 = r->lat * 180.0 / M_PI;
	double lon1 = r->lon * 180.0 / M_PI;
	double lat2, lon2; /* return projected position */
	geod_direct(&gd, lat1, lon1, azimuth * 180.0 / M_PI, distance, &lat2, &lon2, 0);
	g->lat = lat2 * M_PI / 180.0;
	g->lon = lon2 * M_PI / 180.0;
	return LW_SUCCESS;
}


static double ptarray_area_spheroid(const POINTARRAY *pa, const SPHEROID *spheroid)
{
	/* Return zero on non-sensical inputs */
	if ( ! pa || pa->npoints < 4 )
		return 0.0;

	struct geod_geodesic gd;
	geod_init(&gd, spheroid->a, spheroid->f);
	struct geod_polygon poly;
	geod_polygon_init(&poly, 0);
	uint32_t i;
	double area; /* returned polygon area */
	POINT2D p; /* long/lat units are degrees */

	/* Pass points from point array; don't close the linearring */
	for ( i = 0; i < pa->npoints - 1; i++ )
	{
		getPoint2d_p(pa, i, &p);
		geod_polygon_addpoint(&gd, &poly, p.y, p.x);
		LWDEBUGF(4, "geod_polygon_addpoint %d: %.12g %.12g", i, p.y, p.x);
	}
	i = geod_polygon_compute(&gd, &poly, 0, 1, &area, 0);
	if ( i != pa->npoints - 1 )
	{
		lwerror("ptarray_area_spheroid: different number of points %d vs %d",
				i, pa->npoints - 1);
	}
	LWDEBUGF(4, "geod_polygon_compute area: %.12g", area);
	return fabs(area);
}

/**
* Calculate the area of an LWGEOM. Anything except POLYGON, MULTIPOLYGON
* and GEOMETRYCOLLECTION return zero immediately. Multi's recurse, polygons
* calculate external ring area and subtract internal ring area. A GBOX is
* required to check relationship to equator an outside point.
* WARNING: Does NOT WORK for polygons over equator or pole.
*/
double lwgeom_area_spheroid(const LWGEOM *lwgeom, const SPHEROID *spheroid)
{
	int type;

	assert(lwgeom);

	/* No area in nothing */
	if ( lwgeom_is_empty(lwgeom) )
		return 0.0;

	/* Read the geometry type number */
	type = lwgeom->type;

	/* Anything but polygons and collections returns zero */
	if ( ! ( type == POLYGONTYPE || type == MULTIPOLYGONTYPE || type == COLLECTIONTYPE ) )
		return 0.0;

	/* Actually calculate area */
	if ( type == POLYGONTYPE )
	{
		LWPOLY *poly = (LWPOLY*)lwgeom;
		uint32_t i;
		double area = 0.0;

		/* Just in case there's no rings */
		if ( poly->nrings < 1 )
			return 0.0;

		/* First, the area of the outer ring */
		area += ptarray_area_spheroid(poly->rings[0], spheroid);

		/* Subtract areas of inner rings */
		for ( i = 1; i < poly->nrings; i++ )
		{
			area -=  ptarray_area_spheroid(poly->rings[i], spheroid);
		}
		return area;
	}

	/* Recurse into sub-geometries to get area */
	if ( type == MULTIPOLYGONTYPE || type == COLLECTIONTYPE )
	{
		LWCOLLECTION *col = (LWCOLLECTION*)lwgeom;
		uint32_t i;
		double area = 0.0;

		for ( i = 0; i < col->ngeoms; i++ )
		{
			area += lwgeom_area_spheroid(col->geoms[i], spheroid);
		}
		return area;
	}

	/* Shouldn't get here. */
	return 0.0;
}



