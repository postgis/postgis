
/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2003 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of hte GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************
 * $Log$
 * Revision 1.10  2004/02/05 20:21:14  dblasby
 * Added 'curvature method' for cases where the original algorithm breaks down.
 *
 * Revision 1.9  2004/02/04 02:53:20  dblasby
 * applied patricia tozer's patch (distance function was taking acos of something
 * just slightly outside [-1,1]).
 *
 * Revision 1.8  2003/12/04 18:58:35  dblasby
 * changed david skae to skea
 *
 * Revision 1.7  2003/07/01 18:30:55  pramsey
 * Added CVS revision headers.
 *
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
#include "access/rtree.h"


#include "fmgr.h"


#include "postgis.h"
#include "utils/elog.h"

#define SHOW_DIGS_DOUBLE 15
#define MAX_DIGS_DOUBLE (SHOW_DIGS_DOUBLE + 6 + 1 + 3 +1)


//	distance from -126 49  to -126 49.011096139863 in 'SPHEROID["GRS_1980",6378137,298.257222101]' is 1234.000


double distance_sphere_method(double lat1, double long1,double lat2,double long2, SPHEROID *sphere);


//use the WKT definition of an ellipsoid
// ie. SPHEROID["name",A,rf] or SPHEROID("name",A,rf)
//	  SPHEROID["GRS_1980",6378137,298.257222101]
// wkt says you can use "(" or "["

PG_FUNCTION_INFO_V1(ellipsoid_in);
Datum ellipsoid_in(PG_FUNCTION_ARGS)
{
	char	   	   *str = PG_GETARG_CSTRING(0);
	SPHEROID	   *sphere = (SPHEROID *) palloc(sizeof(SPHEROID));
	int		   nitems;
	double	   rf;


	memset(sphere,0, sizeof(SPHEROID));

	if (strstr(str,"SPHEROID") !=  str )
	{
		 elog(ERROR,"SPHEROID parser - doesnt start with SPHEROID");
		 pfree(sphere);
		 PG_RETURN_NULL();
	}

	nitems = sscanf(str,"SPHEROID[\"%19[^\"]\",%lf,%lf]",sphere->name,&sphere->a,&rf);

	if ( nitems==0)
		nitems = sscanf(str,"SPHEROID(\"%19[^\"]\",%lf,%lf)",sphere->name,&sphere->a,&rf);

	if (nitems != 3)
	{
		 elog(ERROR,"SPHEROID parser - couldnt parse the spheroid");
		 pfree(sphere);
		 PG_RETURN_NULL();
	}

	sphere->f = 1.0/rf;
	sphere->b = sphere->a - (1.0/rf)*sphere->a;
	sphere->e_sq = ((sphere->a*sphere->a) - (sphere->b*sphere->b) )/ (sphere->a*sphere->a);
	sphere->e = sqrt(sphere->e_sq);

	PG_RETURN_POINTER(sphere);

}



PG_FUNCTION_INFO_V1(ellipsoid_out);
Datum ellipsoid_out(PG_FUNCTION_ARGS)
{
	SPHEROID  *sphere = (SPHEROID *) PG_GETARG_POINTER(0);
	char	*result;

	result = palloc(MAX_DIGS_DOUBLE + MAX_DIGS_DOUBLE + 20 + 9 + 2);

	sprintf(result,"SPHEROID(\"%s\",%.15g,%.15g)", sphere->name,sphere->a, 1.0/sphere->f);

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


double length2d_ellipse_linestring(LINE3D	*line, SPHEROID  	*sphere)
{

	int	i;
	POINT3D	*frm,*to;
	double	dist = 0.0;

	if (line->npoints <2)
		return 0.0;	//must have >1 point to make sense

	frm = &line->points[0];

	for (i=1; i<line->npoints;i++)
	{
		to = &line->points[i];

		dist +=  distance_ellipse(frm->y*M_PI/180.0 , frm->x*M_PI/180.0,
						to->y*M_PI/180.0 , to->x*M_PI/180.0,
						sphere);

		frm = to;
	}
	return dist;
}

double length3d_ellipse_linestring(LINE3D	*line, SPHEROID  	*sphere)
{
	int	i;
	POINT3D	*frm,*to;
	double	dist = 0.0;
	double	dist_ellipse;

	if (line->npoints <2)
		return 0.0;	//must have >1 point to make sense

	frm = &line->points[0];

	for (i=1; i<line->npoints;i++)
	{
		to = &line->points[i];

		dist_ellipse =  distance_ellipse(frm->y*M_PI/180.0 , frm->x*M_PI/180.0,
						to->y*M_PI/180.0 , to->x*M_PI/180.0,
						sphere);

		dist += sqrt(dist_ellipse*dist_ellipse + (frm->z*frm->z) );

		frm = to;
	}
	return dist;


}


// length_ellipsoid(GEOMETRY, SPHEROID)
// find the "length of a geometry"
// length2d(point) = 0
// length2d(line) = length of line
// length2d(polygon) = 0
// uses ellipsoidal math to find the distance
//// x's are longitude, and y's are latitude - both in decimal degrees

PG_FUNCTION_INFO_V1(length_ellipsoid);
Datum length_ellipsoid(PG_FUNCTION_ARGS)
{
		GEOMETRY		      *geom = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		SPHEROID    		*sphere = (SPHEROID *) PG_GETARG_POINTER(1);

	int32				*offsets1;
	char				*o1;
	int32				type1,j;
	LINE3D			*line;
	double			dist = 0.0;

	offsets1 = (int32 *) ( ((char *) &(geom->objType[0] ))+ sizeof(int32) * geom->nobjs ) ;


	//now have to do a scan of each object

	for (j=0; j< geom->nobjs; j++)		//for each object in geom1
	{
		o1 = (char *) geom +offsets1[j] ;
		type1=  geom->objType[j];
		if (type1 == LINETYPE)	//LINESTRING
		{
			line = (LINE3D *) o1;
			dist += length2d_ellipse_linestring(line,sphere);
		}
	}
	PG_RETURN_FLOAT8(dist);



}

// length3d_ellipsoid(GEOMETRY, SPHEROID)
// find the "length of a geometry"
// length3d(point) = 0
// length3d(line) = length of line
// length3d(polygon) = 0
// uses ellipsoidal math to find the distance on the XY plane, then
//  uses simple Pythagoras theorm to find the 3d distance on each segment
// x's are longitude, and y's are latitude - both in decimal degrees

PG_FUNCTION_INFO_V1(length3d_ellipsoid);
Datum length3d_ellipsoid(PG_FUNCTION_ARGS)
{
		GEOMETRY		      *geom = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		SPHEROID    		*sphere = (SPHEROID *) PG_GETARG_POINTER(1);

	int32				*offsets1;
	char				*o1;
	int32				type1,j;
	LINE3D			*line;
	double			dist = 0.0;

	offsets1 = (int32 *) ( ((char *) &(geom->objType[0] ))+ sizeof(int32) * geom->nobjs ) ;


	//now have to do a scan of each object

	for (j=0; j< geom->nobjs; j++)		//for each object in geom1
	{
		o1 = (char *) geom +offsets1[j] ;
		type1=  geom->objType[j];
		if (type1 == LINETYPE)	//LINESTRING
		{
			line = (LINE3D *) o1;
			dist += length3d_ellipse_linestring(line,sphere);
		}
	}
	PG_RETURN_FLOAT8(dist);
}

//distance (geometry,geometry, sphere)
// -geometrys MUST be points
PG_FUNCTION_INFO_V1(distance_ellipsoid);
Datum distance_ellipsoid(PG_FUNCTION_ARGS)
{
		SPHEROID    		*sphere = (SPHEROID *) PG_GETARG_POINTER(2);
		GEOMETRY		      *geom1 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		GEOMETRY		      *geom2 = (GEOMETRY *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

		POINT3D	*pt1,*pt2;
		int32				*offsets1;
		int32				*offsets2;
		char				*o;


	if (geom1->SRID != geom2->SRID)
	{
		elog(ERROR,"optimistic_overlap:Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}

	if (geom1->type != POINTTYPE)
	{
		elog(ERROR,"optimistic_overlap: first arg isnt a point\n");
		PG_RETURN_NULL();
	}
	if (geom2->type != POINTTYPE)
	{
		elog(ERROR,"optimistic_overlap: second arg isnt a point\n");
		PG_RETURN_NULL();
	}

	offsets1 = (int32 *) ( ((char *) &(geom1->objType[0] ))+ sizeof(int32) * geom1->nobjs ) ;
	offsets2 = (int32 *) ( ((char *) &(geom2->objType[0] ))+ sizeof(int32) * geom2->nobjs ) ;
	o = (char *) geom1 +offsets1[0] ;
	pt1 = (POINT3D *) o;

	o = (char *) geom2 +offsets2[0] ;
	pt2 = (POINT3D *) o;

	PG_RETURN_FLOAT8(distance_ellipse(pt1->y*M_PI/180.0 ,pt1->x*M_PI/180.0 ,
							  pt2->y*M_PI/180.0 ,pt2->x*M_PI/180.0 , sphere) );

//double	distance_ellipse(double lat1, double long1,
//					double lat2, double long2,
//					SPHEROID *sphere)


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
 */

double distance_sphere_method(double lat1, double long1,double lat2,double long2, SPHEROID *sphere)
{
			double R,S,X,Y,deltaX,deltaY;

			double 	distance 	= 0.0;
			double 	sin_lat 	= sin(lat1);
			double 	sin2_lat 	= sin_lat * sin_lat;

			double  Geocent_a 	= sphere->a;
			double  Geocent_a2 	= sphere->a * sphere->a;
			double  Geocent_b2 	= sphere->b * sphere->b;
			double  Geocent_e2 	= (Geocent_a2 - Geocent_b2) / Geocent_a2;

			R  	= Geocent_a / (sqrt(1.0e0 - Geocent_e2 * sin2_lat));
			S 	= R * sin(M_PI/2.0-lat1) ; // 90 - lat1, but in radians

			deltaX = long2 - long1;  //in rads
			deltaY = lat2 - lat1;    // in rads

			X = deltaX/(2.0*M_PI) * 2 * M_PI * S;  // think: a % of 2*pi*S
			Y = deltaY /(2.0*M_PI) * 2 * M_PI * R;

			distance = sqrt((X * X + Y * Y));

			return distance;
}
