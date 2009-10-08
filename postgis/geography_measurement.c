/**********************************************************************
 * $Id: geography_inout.c 4535 2009-09-28 18:16:21Z colivier $
 *
 * PostGIS - Spatial Types for PostgreSQL
 * Copyright 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/
 
#include "postgres.h"

#include "../postgis_config.h"

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "libgeom.h"         /* For standard geometry types. */
#include "lwgeom_pg.h"       /* For debugging macros. */
#include "geography.h"	     /* For utility functions. */

Datum geography_distance_sphere(PG_FUNCTION_ARGS);
Datum geography_area_sphere(PG_FUNCTION_ARGS);
Datum geography_expand(PG_FUNCTION_ARGS);


/*
** geography_distance_sphere(GSERIALIZED *g1, GSERIALIZED *g2, double tolerance) 
** returns double distance in meters
*/
PG_FUNCTION_INFO_V1(geography_distance_sphere);
Datum geography_distance_sphere(PG_FUNCTION_ARGS)
{
	LWGEOM *lwgeom1 = NULL;
	LWGEOM *lwgeom2 = NULL;
	GBOX gbox1;
	GBOX gbox2;
	GSERIALIZED *g1 = NULL;
	GSERIALIZED *g2 = NULL;
	double tolerance;
	double distance;
	
	/* Get our geometry objects loaded into memory. */
	g1 = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	g2 = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	
	/* We need the bounding boxes in case of polygon calculations,
	   which requires them to generate a stab-line to test point-in-polygon. */
	if( ! gbox_from_gserialized(g1, &gbox1) ||
	    ! gbox_from_gserialized(g2, &gbox2) )
	{
		elog(ERROR, "Error in gbox_from_gserialized calculation.");
		PG_RETURN_NULL();
	}
	
	lwgeom1 = lwgeom_from_gserialized(g1);
	lwgeom2 = lwgeom_from_gserialized(g2);
	
	/* Read our tolerance value. */
	tolerance = PG_GETARG_FLOAT8(2);
	tolerance = tolerance / WGS84_RADIUS;

	/* Calculate the distance */
	distance = lwgeom_distance_sphere(lwgeom1, lwgeom2, gbox1, gbox2, tolerance);

	/* Something went wrong... should already be eloged */
	if( distance < 0.0 )
	{
		elog(ERROR, "lwgeom_distance_sphere returned < 0.0");
		PG_RETURN_NULL();
	}
	
	/* Currently normalizing with a fixed WGS84 radius, in future this
	   should be the average radius of the SRID in play */
	distance = distance * WGS84_RADIUS;

	/* Clean up, but not all the way to the point arrays */
	lwgeom_release(lwgeom1);
	lwgeom_release(lwgeom2);
	
	PG_RETURN_FLOAT8(distance);

}


/*
** geography_expand(GSERIALIZED *g) returns *GSERIALIZED
**
** warning, this tricky little function does not expand the 
** geometry at all, just re-writes bounding box value to be 
** a bit bigger. only useful when passing the result along to
** an index operator (&&)
*/
PG_FUNCTION_INFO_V1(geography_expand);
Datum geography_expand(PG_FUNCTION_ARGS)
{
	GIDX *gidx = gidx_new(3);
	GSERIALIZED *g = NULL;
	GSERIALIZED *g_out = NULL;
	double distance;
	float fdistance;
	int i;
	
	/* Get our bounding box out of the geography */
	geography_datum_gidx(PG_GETARG_DATUM(0), gidx);

	/* Get a pointer to the geography */
	g = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* Read our distance value and normalize to unit-sphere. */
	distance = PG_GETARG_FLOAT8(1) / WGS84_RADIUS;
	fdistance = (float)distance;

	for( i = 0; i < 3; i++ ) 
	{
		GIDX_SET_MIN(gidx, i, GIDX_GET_MIN(gidx, i) - fdistance);
		GIDX_SET_MAX(gidx, i, GIDX_GET_MAX(gidx, i) + fdistance);
	}
	
	g_out = gidx_insert_into_gserialized(g, gidx);
	pfree(gidx);

	if( g_out == NULL )
		PG_RETURN_NULL();
	
	PG_RETURN_POINTER(g_out);

}

/*
** geography_area_sphere(GSERIALIZED *g) 
** returns double area in meters square
*/
PG_FUNCTION_INFO_V1(geography_area_sphere);
Datum geography_area_sphere(PG_FUNCTION_ARGS)
{
	LWGEOM *lwgeom = NULL;
	GBOX gbox;
	GSERIALIZED *g = NULL;
	double area;
	
	/* Get our geometry object loaded into memory. */
	g = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	
	/* We need the bounding box to get an outside point for area algorithm */
	if( ! gbox_from_gserialized(g, &gbox) )
	{
		elog(ERROR, "Error in gbox_from_gserialized calculation.");
		PG_RETURN_NULL();
	}
	
	lwgeom = lwgeom_from_gserialized(g);
	
	/* Calculate the area */
	area = lwgeom_area_sphere(lwgeom, gbox);

	/* Something went wrong... */
	if( area < 0.0 )
	{
		elog(ERROR, "lwgeom_area_sphere returned area < 0.0");
		PG_RETURN_NULL();
	}

	/* Currently normalizing with a fixed WGS84 radius, in future this
	   should be the average radius of the SRID in play */
	area = area * WGS84_RADIUS * WGS84_RADIUS;

	/* Clean up, but not all the way to the point arrays */
	lwgeom_release(lwgeom);
	
	PG_RETURN_FLOAT8(area);

}