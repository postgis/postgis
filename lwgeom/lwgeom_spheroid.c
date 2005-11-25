/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2003 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "postgres.h"


#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "access/gist.h"
#include "access/itup.h"

#include "fmgr.h"
#include "utils/elog.h"

#include "liblwgeom.h"
#include "lwgeom_pg.h"

#define SHOW_DIGS_DOUBLE 15
#define MAX_DIGS_DOUBLE (SHOW_DIGS_DOUBLE + 6 + 1 + 3 +1)

//	distance from -126 49  to -126 49.011096139863 in 'SPHEROID["GRS_1980",6378137,298.257222101]' is 1234.000


// PG-exposed 
Datum ellipsoid_in(PG_FUNCTION_ARGS);
Datum ellipsoid_out(PG_FUNCTION_ARGS);
Datum LWGEOM_length2d_ellipsoid_linestring(PG_FUNCTION_ARGS);
Datum LWGEOM_length_ellipsoid_linestring(PG_FUNCTION_ARGS);
Datum LWGEOM_distance_ellipsoid_point(PG_FUNCTION_ARGS);
Datum LWGEOM_distance_sphere(PG_FUNCTION_ARGS);

// internal
double distance_sphere_method(double lat1, double long1,double lat2,double long2, SPHEROID *sphere);
double distance_ellipse_calculation(double lat1, double long1, double lat2, double long2, SPHEROID *sphere);
double	distance_ellipse(double lat1, double long1, double lat2, double long2, SPHEROID *sphere);
double deltaLongitude(double azimuth, double sigma, double tsm,SPHEROID *sphere);
double mu2(double azimuth,SPHEROID *sphere);
double bigA(double u2);
double bigB(double u2);
double lwgeom_pointarray_length2d_ellipse(POINTARRAY *pts, SPHEROID *sphere);
double lwgeom_pointarray_length_ellipse(POINTARRAY *pts, SPHEROID *sphere);


//use the WKT definition of an ellipsoid
// ie. SPHEROID["name",A,rf] or SPHEROID("name",A,rf)
//	  SPHEROID["GRS_1980",6378137,298.257222101]
// wkt says you can use "(" or "["
PG_FUNCTION_INFO_V1(ellipsoid_in);
Datum ellipsoid_in(PG_FUNCTION_ARGS)
{
	char *str = PG_GETARG_CSTRING(0);
	SPHEROID *sphere = (SPHEROID *) palloc(sizeof(SPHEROID));
	int nitems;
	double rf;

	memset(sphere,0, sizeof(SPHEROID));

	if (strstr(str,"SPHEROID") !=  str )
	{
		 elog(ERROR,"SPHEROID parser - doesnt start with SPHEROID");
		 pfree(sphere);
		 PG_RETURN_NULL();
	}

	nitems = sscanf(str,"SPHEROID[\"%19[^\"]\",%lf,%lf]",
		sphere->name, &sphere->a, &rf);

	if ( nitems==0)
		nitems = sscanf(str,"SPHEROID(\"%19[^\"]\",%lf,%lf)",
			sphere->name, &sphere->a, &rf);

	if (nitems != 3)
	{
		 elog(ERROR,"SPHEROID parser - couldnt parse the spheroid");
		 pfree(sphere);
		 PG_RETURN_NULL();
	}

	sphere->f = 1.0/rf;
	sphere->b = sphere->a - (1.0/rf)*sphere->a;
	sphere->e_sq = ((sphere->a*sphere->a) - (sphere->b*sphere->b)) /
		(sphere->a*sphere->a);
	sphere->e = sqrt(sphere->e_sq);

	PG_RETURN_POINTER(sphere);

}

PG_FUNCTION_INFO_V1(ellipsoid_out);
Datum ellipsoid_out(PG_FUNCTION_ARGS)
{
	SPHEROID *sphere = (SPHEROID *) PG_GETARG_POINTER(0);
	char *result;

	result = palloc(MAX_DIGS_DOUBLE + MAX_DIGS_DOUBLE + 20 + 9 + 2);

	sprintf(result,"SPHEROID(\"%s\",%.15g,%.15g)",
		sphere->name, sphere->a, 1.0/sphere->f);

	PG_RETURN_CSTRING(result);
}

//support function for distance calc
	//code is taken from David Skea
	//Geographic Data BC, Province of British Columbia, Canada.
	// Thanks to GDBC and David Skea for allowing this to be
	// put in PostGIS.
double deltaLongitude(double azimuth, double sigma, double tsm,SPHEROID *sphere)
{
	// compute the expansion C
	double das,C;
	double ctsm,DL;

	das = cos(azimuth)*cos(azimuth);
	C = sphere->f/16.0 * das * (4.0 + sphere->f * (4.0 - 3.0 * das));
	// compute the difference in longitude

	ctsm = cos(tsm);
	DL = ctsm + C * cos(sigma) * (-1.0 + 2.0 * ctsm*ctsm);
	DL = sigma + C * sin(sigma) * DL;
	return (1.0 - C) * sphere->f * sin(azimuth) * DL;
}


//support function for distance calc
	//code is taken from David Skea
	//Geographic Data BC, Province of British Columbia, Canada.
	// Thanks to GDBC and David Skea for allowing this to be
	// put in PostGIS.
double mu2(double azimuth,SPHEROID *sphere)
{
	double    e2;

	e2 = sqrt(sphere->a*sphere->a-sphere->b*sphere->b)/sphere->b;
	return cos(azimuth)*cos(azimuth) * e2*e2;
}


//support function for distance calc
	//code is taken from David Skea
	//Geographic Data BC, Province of British Columbia, Canada.
	// Thanks to GDBC and David Skea for allowing this to be
	// put in PostGIS.
double bigA(double u2)
{
	return 1.0 + u2/256.0 * (64.0 + u2 * (-12.0 + 5.0 * u2));
}


//support function for distance calc
	//code is taken from David Skea
	//Geographic Data BC, Province of British Columbia, Canada.
	// Thanks to GDBC and David Skea for allowing this to be
	// put in PostGIS.
double bigB(double u2)
{
	return u2/512.0 * (128.0 + u2 * (-64.0 + 37.0 * u2));
}



double	distance_ellipse(double lat1, double long1,
					double lat2, double long2,
					SPHEROID *sphere)
{
		double result;

	    if ( (lat1==lat2) && (long1 == long2) )
		{
			return 0.0; // same point, therefore zero distance
		}

		result = distance_ellipse_calculation(lat1,long1,lat2,long2,sphere);
//		result2 =  distance_sphere_method(lat1, long1,lat2,long2, sphere);

//elog(NOTICE,"delta = %lf, skae says: %.15lf,2 circle says: %.15lf",(result2-result),result,result2);
//elog(NOTICE,"2 circle says: %.15lf",result2);

		if (result != result)  // NaN check (x==x for all x except NaN by IEEE definition)
		{
			result =  distance_sphere_method(lat1, long1,lat2,long2, sphere);
		}

		return result;
}

//given 2 lat/longs and ellipse, find the distance
// note original r = 1st, s=2nd location
double	distance_ellipse_calculation(double lat1, double long1,
					double lat2, double long2,
					SPHEROID *sphere)
{
	//code is taken from David Skea
	//Geographic Data BC, Province of British Columbia, Canada.
	// Thanks to GDBC and David Skea for allowing this to be
	// put in PostGIS.

	double L1,L2,sinU1,sinU2,cosU1,cosU2;
	double dl,dl1,dl2,dl3,cosdl1,sindl1;
	double cosSigma,sigma,azimuthEQ,tsm;
	double u2,A,B;
	double dsigma;

	double TEMP;

	int iterations;


	L1 = atan((1.0 - sphere->f ) * tan( lat1) );
	L2 = atan((1.0 - sphere->f ) * tan( lat2) );
	sinU1 = sin(L1);
	sinU2 = sin(L2);
	cosU1 = cos(L1);
	cosU2 = cos(L2);

	dl = long2- long1;
	dl1 = dl;
	cosdl1 = cos(dl);
	sindl1 = sin(dl);
	iterations = 0;
	do {
		cosSigma = sinU1 * sinU2 + cosU1 * cosU2 * cosdl1;
		sigma = acos(cosSigma);
		azimuthEQ = asin((cosU1 * cosU2 * sindl1)/sin(sigma));

			// patch from patrica tozer to handle minor mathematical stability problem
		TEMP = cosSigma - (2.0 * sinU1 * sinU2)/(cos(azimuthEQ)*cos(azimuthEQ));
		if(TEMP > 1)
		{
			   TEMP = 1;
		}
		else if(TEMP < -1)
		{
			   TEMP = -1;
		}
        tsm = acos(TEMP);


		//tsm = acos(cosSigma - (2.0 * sinU1 * sinU2)/(cos(azimuthEQ)*cos(azimuthEQ)));
		dl2 = deltaLongitude(azimuthEQ, sigma, tsm,sphere);
		dl3 = dl1 - (dl + dl2);
		dl1 = dl + dl2;
		cosdl1 = cos(dl1);
		sindl1 = sin(dl1);
		iterations++;
	} while ( (iterations<999) && (fabs(dl3) > 1.0e-032));

	   // compute expansions A and B
	u2 = mu2(azimuthEQ,sphere);
	A = bigA(u2);
	B = bigB(u2);

	// compute length of geodesic
	dsigma = B * sin(sigma) * (cos(tsm) + (B*cosSigma*(-1.0 + 2.0 * (cos(tsm)*cos(tsm))))/4.0);
	return sphere->b * (A * (sigma - dsigma));
}

/*
 * Computed 2d/3d length of a POINTARRAY depending on input dimensions.
 * Uses ellipsoidal math to find the distance.
 */
double lwgeom_pointarray_length_ellipse(POINTARRAY *pts, SPHEROID *sphere)
{
	double dist = 0.0;
	int i;

	//elog(NOTICE, "lwgeom_pointarray_length_ellipse called");

	if ( pts->npoints < 2 ) return 0.0;

	// compute 2d length if 3d is not available
	if ( TYPE_NDIMS(pts->dims) < 3 )
	{
		return lwgeom_pointarray_length2d_ellipse(pts, sphere);
	}

	for (i=0; i<pts->npoints-1;i++)
	{
		POINT3DZ frm;
		POINT3DZ to;
		double distellips;

		getPoint3dz_p(pts, i, &frm);
		getPoint3dz_p(pts, i+1, &to);

		distellips = distance_ellipse(
			frm.y*M_PI/180.0, frm.x*M_PI/180.0,
			to.y*M_PI/180.0, to.x*M_PI/180.0, sphere);
		dist += sqrt(distellips*distellips + (frm.z-to.z)*(frm.z-to.z));
	}
	return dist;
}

/*
 * Computed 2d length of a POINTARRAY regardless of input dimensions
 * Uses ellipsoidal math to find the distance.
 */
double
lwgeom_pointarray_length2d_ellipse(POINTARRAY *pts, SPHEROID *sphere)
{
	double dist = 0.0;
	int i;
	POINT2D frm;
	POINT2D to;

	//elog(NOTICE, "lwgeom_pointarray_length2d_ellipse called");

	if ( pts->npoints < 2 ) return 0.0;
	for (i=0; i<pts->npoints-1;i++)
	{
		getPoint2d_p(pts, i, &frm);
		getPoint2d_p(pts, i+1, &to);
		dist += distance_ellipse(frm.y*M_PI/180.0,
			frm.x*M_PI/180.0, to.y*M_PI/180.0,
			to.x*M_PI/180.0, sphere);
	}
	return dist;
}

//find the "length of a geometry"
// length2d_spheroid(point, sphere) = 0
// length2d_spheroid(line, sphere) = length of line
// length2d_spheroid(polygon, sphere) = 0 
//	-- could make sense to return sum(ring perimeter)
// uses ellipsoidal math to find the distance
//// x's are longitude, and y's are latitude - both in decimal degrees
PG_FUNCTION_INFO_V1(LWGEOM_length2d_ellipsoid_linestring);
Datum LWGEOM_length2d_ellipsoid_linestring(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	SPHEROID *sphere = (SPHEROID *) PG_GETARG_POINTER(1);
	LWGEOM_INSPECTED *inspected = lwgeom_inspect(SERIALIZED_FORM(geom));
	LWLINE *line;
	double dist = 0.0;
	int i;

//elog(NOTICE, "in LWGEOM_length2d_ellipsoid_linestring");

	for (i=0; i<inspected->ngeometries; i++)
	{
		line = lwgeom_getline_inspected(inspected, i);
		if ( line == NULL ) continue;
		dist += lwgeom_pointarray_length2d_ellipse(line->points,
			sphere);
//elog(NOTICE, " LWGEOM_length2d_ellipsoid_linestring found a line (%f)", dist);
	}

	pfree_inspected(inspected);

	PG_RETURN_FLOAT8(dist);
}

//find the "length of a geometry"
// length2d_spheroid(point, sphere) = 0
// length2d_spheroid(line, sphere) = length of line
// length2d_spheroid(polygon, sphere) = 0 
//	-- could make sense to return sum(ring perimeter)
// uses ellipsoidal math to find the distance
//// x's are longitude, and y's are latitude - both in decimal degrees
PG_FUNCTION_INFO_V1(LWGEOM_length_ellipsoid_linestring);
Datum LWGEOM_length_ellipsoid_linestring(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	SPHEROID *sphere = (SPHEROID *) PG_GETARG_POINTER(1);
	LWGEOM_INSPECTED *inspected = lwgeom_inspect(SERIALIZED_FORM(geom));
	LWLINE *line;
	double dist = 0.0;
	int i;

//elog(NOTICE, "in LWGEOM_length_ellipsoid_linestring");

	for (i=0; i<inspected->ngeometries; i++)
	{
		line = lwgeom_getline_inspected(inspected, i);
		if ( line == NULL ) continue;
		dist += lwgeom_pointarray_length_ellipse(line->points,
			sphere);
//elog(NOTICE, " LWGEOM_length_ellipsoid_linestring found a line (%f)", dist);
	}

	pfree_inspected(inspected);

	PG_RETURN_FLOAT8(dist);
}

/*
 *  For some lat/long points, the above method doesnt calculate the distance very well.
 *  Typically this is for two lat/long points that are very very close together (<10cm).
 *  This gets worse closer to the equator.
 *
 *   This method works very well for very close together points, not so well if they're
 *   far away (>1km).
 *
 *  METHOD:
 *    We create two circles (with Radius R and Radius S) and use these to calculate the distance.
 *
 *    The first (R) is basically a (north-south) line of longitude.
 *    Its radius is approximated by looking at the ellipse. Near the equator R = 'a' (earth's major axis)
 *    near the pole R = 'b' (earth's minor axis).
 *
 *    The second (S) is basically a (east-west) line of lattitude.
 *    Its radius runs from 'a' (major axis) at the equator, and near 0 at the poles.
 *
 *
 *                North pole
 *                *
 *               *
 *              *\--S--
 *             *  R   +
 *            *    \  +
 *           *     A\ +
 *          * ------ \         Equator/centre of earth
 *           *
 *            *
 *             *
 *              *
 *               *
 *                *
 *                South pole
 *  (side view of earth)
 *
 *   Angle A is lat1
 *   R is the distance from the centre of the earth to the lat1/long1 point on the surface
 *   of the Earth.
 *   S is the circle-of-lattitude.  Its calculated from the right triangle defined by
 *      the angle (90-A), and the hypothenus R.
 *
 *
 *
 *   Once R and S have been calculated, the actual distance between the two points can be
 *   calculated.
 *
 *   We dissolve the vector from lat1,long1 to lat2,long2 into its X and Y components (called DeltaX,DeltaY).
 *   The actual distance that these angle-based measurements represent is taken from the two
 *   circles we just calculated; R (for deltaY) and S (for deltaX).
 *
 *    (if deltaX is 1 degrees, then that distance represents 1/360 of a circle of radius S.)
 *
 *
 *  Parts taken from PROJ4 - geodetic_to_geocentric() (for calculating Rn)
 *
 *  remember that lat1/long1/lat2/long2 are comming in a *RADIANS* not degrees.
 *
 * By Patricia Tozer and Dave Blasby
 *
 *  This is also called the "curvature method".
 */

double distance_sphere_method(double lat1, double long1,double lat2,double long2, SPHEROID *sphere)
{
			double R,S,X,Y,deltaX,deltaY;

			double 	distance 	= 0.0;
			double 	sin_lat 	= sin(lat1);
			double 	sin2_lat 	= sin_lat * sin_lat;

			double  Geocent_a 	= sphere->a;
			double  Geocent_e2 	= sphere->e_sq;

			R  	= Geocent_a / (sqrt(1.0e0 - Geocent_e2 * sin2_lat));
			S 	= R * sin(M_PI/2.0-lat1) ; // 90 - lat1, but in radians

			deltaX = long2 - long1;  //in rads
			deltaY = lat2 - lat1;    // in rads

			X = deltaX/(2.0*M_PI) * 2 * M_PI * S;  // think: a % of 2*pi*S
			Y = deltaY /(2.0*M_PI) * 2 * M_PI * R;

			distance = sqrt((X * X + Y * Y));

			return distance;
}

//distance (geometry,geometry, sphere)
// -geometrys MUST be points
PG_FUNCTION_INFO_V1(LWGEOM_distance_ellipsoid_point);
Datum LWGEOM_distance_ellipsoid_point(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	SPHEROID *sphere = (SPHEROID *)PG_GETARG_POINTER(2);
	LWPOINT *point1, *point2;
	POINT2D p1, p2;

	if (pglwgeom_getSRID(geom1) != pglwgeom_getSRID(geom2))
	{
		elog(ERROR, "LWGEOM_distance_ellipsoid_point: Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}

	point1 = lwpoint_deserialize(SERIALIZED_FORM(geom1));
	if (point1 == NULL)
	{
		elog(ERROR, "LWGEOM_distance_ellipsoid_point: first arg isnt a point\n");
		PG_RETURN_NULL();
	}

	point2 = lwpoint_deserialize(SERIALIZED_FORM(geom2));
	if (point2 == NULL)
	{
		elog(ERROR, "LWGEOM_distance_ellipsoid_point: second arg isnt a point\n");
		PG_RETURN_NULL();
	}

	getPoint2d_p(point1->point, 0, &p1);
	getPoint2d_p(point2->point, 0, &p2);
	PG_RETURN_FLOAT8(distance_ellipse(p1.y*M_PI/180.0,
		p1.x*M_PI/180.0, p2.y*M_PI/180.0,
		p2.x*M_PI/180.0, sphere));

}

/*
 * This algorithm was taken from the geo_distance function of the 
 * earthdistance package contributed by Bruno Wolff III.
 * It was altered to accept GEOMETRY objects and return results in
 * meters.
 */
PG_FUNCTION_INFO_V1(LWGEOM_distance_sphere);
Datum LWGEOM_distance_sphere(PG_FUNCTION_ARGS)
{
        const double EARTH_RADIUS = 6370986.884258304;
	//const double TWO_PI = 2.0 * M_PI;
	PG_LWGEOM *geom1 = (PG_LWGEOM *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *geom2 = (PG_LWGEOM *) PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
        LWPOINT *lwpt1, *lwpt2;
	POINT2D *pt1, *pt2;

	double long1, lat1, long2, lat2;
	double longdiff;
	double sino;
        double result;

	if (pglwgeom_getSRID(geom1) != pglwgeom_getSRID(geom2))
	{
		elog(ERROR, "LWGEOM_distance_sphere Operation on two GEOMETRIES with differenc SRIDs\n");
		PG_RETURN_NULL();
	}

        lwpt1 = lwpoint_deserialize(SERIALIZED_FORM(geom1));
	if (lwpt1 == NULL)
	{
		elog(ERROR, "LWGEOM_distance_sphere first arg isnt a point\n");
		PG_RETURN_NULL();
	}

        lwpt2 = lwpoint_deserialize(SERIALIZED_FORM(geom2));
	if (lwpt2 == NULL)
	{
		elog(ERROR, "optimistic_overlap: second arg isnt a point\n");
		PG_RETURN_NULL();
	}

        pt1 = palloc(sizeof(POINT2D));
        pt2 = palloc(sizeof(POINT2D));

        lwpoint_getPoint2d_p(lwpt1, pt1);
        lwpoint_getPoint2d_p(lwpt2, pt2);

	/*
	 * Start geo_distance code.  Longitude is degrees west of
	 * Greenwich, and thus is negative from what normal things
	 * will supply the function.
	 */
        long1 = -2 * (pt1->x / 360.0) * M_PI;
        lat1 = 2 * (pt1->y / 360.0) * M_PI;

	long2 = -2 * (pt2->x / 360.0) * M_PI;
	lat2 = 2 * (pt2->y / 360.0) * M_PI;

        /* compute difference in longitudes - want < 180 degrees */
        longdiff = fabs(long1 - long2);
        if (longdiff > M_PI)
        {
                longdiff = (2 * M_PI) - longdiff;
        }

        sino = sqrt(sin(fabs(lat1 - lat2) / 2.) * sin(fabs(lat1 - lat2) / 2.) +
                cos(lat1) * cos(lat2) * sin(longdiff / 2.) * sin(longdiff / 2.));
        if (sino > 1.)
        {
                sino = 1.;
        }

        result = 2. * EARTH_RADIUS * asin(sino);
        
        pfree(pt1);
        pfree(pt2);

        PG_RETURN_FLOAT8(result);
}

