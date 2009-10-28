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
Datum geography_length_sphere(PG_FUNCTION_ARGS);
Datum geography_expand(PG_FUNCTION_ARGS);
Datum geography_point_outside(PG_FUNCTION_ARGS);
Datum geography_covers(PG_FUNCTION_ARGS);
Datum geography_bestsrid(PG_FUNCTION_ARGS);

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

/*
** geography_length_sphere(GSERIALIZED *g) 
** returns double length in meters
*/
PG_FUNCTION_INFO_V1(geography_length_sphere);
Datum geography_length_sphere(PG_FUNCTION_ARGS)
{
	LWGEOM *lwgeom = NULL;
	GSERIALIZED *g = NULL;
	double length;
	
	/* Get our geometry object loaded into memory. */
	g = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
		
	lwgeom = lwgeom_from_gserialized(g);
	
	/* Calculate the length */
	length = lwgeom_length_sphere(lwgeom);

	/* Something went wrong... */
	if( length < 0.0 )
	{
		elog(ERROR, "geography_length_sphere returned length < 0.0");
		PG_RETURN_NULL();
	}

	/* Currently normalizing with a fixed WGS84 radius, in future this
	   should be the average radius of the SRID in play */
	length = length * WGS84_RADIUS;

	/* Clean up, but not all the way to the point arrays */
	lwgeom_release(lwgeom);
	
	PG_RETURN_FLOAT8(length);
}

/*
** geography_point_outside(GSERIALIZED *g) 
** returns point outside the object
*/
PG_FUNCTION_INFO_V1(geography_point_outside);
Datum geography_point_outside(PG_FUNCTION_ARGS)
{
	GBOX gbox;
	GSERIALIZED *g = NULL;
	GSERIALIZED *g_out = NULL;
	size_t g_out_size;
	LWPOINT *lwpoint = NULL;
	POINT2D pt;
	
	/* Get our geometry object loaded into memory. */
	g = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	
	/* We need the bounding box to get an outside point for area algorithm */
	if( ! gbox_from_gserialized(g, &gbox) )
	{
		elog(ERROR, "Error in gbox_from_gserialized calculation.");
		PG_RETURN_NULL();
	}
	
	/* Get an exterior point, based on this gbox */
	gbox_pt_outside(gbox, &pt);
	
	lwpoint = make_lwpoint2d(4326, pt.x, pt.y);
	
	g_out = gserialized_from_lwgeom((LWGEOM*)lwpoint, 1, &g_out_size);
	SET_VARSIZE(g_out, g_out_size);
	
	PG_RETURN_POINTER(g_out);

}

/*
** geography_covers(GSERIALIZED *g, GSERIALIZED *g) returns boolean
** Only works for (multi)points and (multi)polygons currently. 
** Attempts a simple point-in-polygon test on the polygon and point. 
** Current algorithm does not distinguish between points on edge 
** and points within.
*/
PG_FUNCTION_INFO_V1(geography_covers);
Datum geography_covers(PG_FUNCTION_ARGS)
{
	LWGEOM *lwgeom1 = NULL;
	LWGEOM *lwgeom2 = NULL;
	GBOX gbox1;
	GBOX gbox2;
	GSERIALIZED *g1 = NULL;
	GSERIALIZED *g2 = NULL;
	int type1, type2;
	int result = LW_FALSE;
	
	/* Get our geometry objects loaded into memory. */
	g1 = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	g2 = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	
	type1 = gserialized_get_type(g1);
	type2 = gserialized_get_type(g2);

	/* Right now we only handle points and polygons */
	if( ! ( (type1 == POLYGONTYPE || type1 == MULTIPOLYGONTYPE || type1 == COLLECTIONTYPE) &&
	        (type2 == POINTTYPE || type2 == MULTIPOINTTYPE || type2 == COLLECTIONTYPE) ) )
	{
		elog(ERROR, "geography_covers: only POLYGON and POINT types are currently supported");
		PG_RETURN_NULL();
	}
	
	/* We need the bounding boxes in case of polygon calculations,
	   which requires them to generate a stab-line to test point-in-polygon. */
	if( ! gbox_from_gserialized(g1, &gbox1) ||
	    ! gbox_from_gserialized(g2, &gbox2) )
	{
		elog(ERROR, "geography_covers: error in gbox_from_gserialized calculation.");
		PG_RETURN_NULL();
	}
	
	/* Construct our working geometries */
	lwgeom1 = lwgeom_from_gserialized(g1);
	lwgeom2 = lwgeom_from_gserialized(g2);
	
	/* Calculate answer */
	result = lwgeom_covers_lwgeom_sphere(lwgeom1, lwgeom2, gbox1, gbox2);

	/* Clean up, but not all the way to the point arrays */
	lwgeom_release(lwgeom1);
	lwgeom_release(lwgeom2);
	
	PG_RETURN_BOOL(result);
}

/*
** geography_bestsrid(GSERIALIZED *g, GSERIALIZED *g) returns int
** Utility function. Returns negative SRID numbers that match to the
** numbers handled in code by the transform(lwgeom, srid) function.
** UTM, polar stereographic and mercator as fallback. To be used
** in wrapping existing geometry functions in SQL to provide access
** to them in the geography module.
*/
PG_FUNCTION_INFO_V1(geography_bestsrid);
Datum geography_bestsrid(PG_FUNCTION_ARGS)
{
	LWGEOM *lwgeom1 = NULL;
	LWGEOM *lwgeom2 = NULL;
	GBOX gbox1;
	GBOX gbox2;
	GSERIALIZED *g1 = NULL;
	GSERIALIZED *g2 = NULL;
	int type1, type2;
	int empty1 = LW_FALSE;
	int empty2 = LW_FALSE;
	
	Datum d1 = PG_GETARG_DATUM(0);
	Datum d2 = PG_GETARG_DATUM(1);
	
	/* Get our geometry objects loaded into memory. */
	g1 = (GSERIALIZED*)PG_DETOAST_DATUM(d1);
	/* Synchronize our box types */
	gbox1.flags = g1->flags;
	/* Read our types */
	type1 = gserialized_get_type(g1);
	/* Construct our working geometries */
	lwgeom1 = lwgeom_from_gserialized(g1);
	/* Calculate if the geometry is empty. */
	empty1 = lwgeom_is_empty(lwgeom1);
	/* Calculate a naive cartesian bounds for the objects */
	if( ! empty1 && lwgeom_calculate_gbox(lwgeom1, &gbox1) == G_FAILURE )
		elog(ERROR, "Error in geography_bestsrid calling lwgeom_calculate_gbox(lwgeom1, &gbox1)");
	
	/* If we have a unique second argument, fill in all the necessarily variables. */
	if( d1 != d2 )
	{
		g2 = (GSERIALIZED*)PG_DETOAST_DATUM(d2);
		type2 = gserialized_get_type(g2);
		gbox2.flags = g2->flags;
		lwgeom2 = lwgeom_from_gserialized(g2);
		empty2 = lwgeom_is_empty(lwgeom2);
		if( ! empty2 && lwgeom_calculate_gbox(lwgeom2, &gbox2) == G_FAILURE )
			elog(ERROR, "Error in geography_bestsrid calling lwgeom_calculate_gbox(lwgeom2, &gbox2)");
	}
	/* 
	** If no unique second argument, copying the box for the first 
	** argument will give us the right answer for all subsequent tests.
	*/
	else
	{
		gbox2 = gbox1;
	}
	
	/* Both empty? We don't have an answer. */
	if ( empty1 && empty2 )
		PG_RETURN_NULL(); 

	/* One empty? We can use the other argument values as infill. */
	if ( empty1 )
		gbox1 = gbox2; 

	if ( empty2 )
		gbox2 = gbox1;

	
	/* Are these data arctic? Polar stereographic north. */
	if( gbox1.ymin > 60.0 && gbox2.ymin > 60.0 )
	{
		PG_RETURN_INT32(-3995);
	}

	/* Are these data antarctic? Polar stereographic south. */
	if( gbox1.ymin < -60.0 && gbox2.ymin < -60.0 )
	{
		PG_RETURN_INT32(-3995);
	}
	
	/*
	** Can we fit these data into one UTM zone? We will assume we can push things as
	** far as a half zone past a zone boundary. Note we have no handling for the 
	** date line in here.
	*/
	if( fabs(FP_MAX(gbox1.xmax, gbox2.xmax) - FP_MIN(gbox1.xmin, gbox2.xmin)) < 6.0 )
	{
		/* Cheap hack to pick a zone. Average of the box center points. */
		double dzone = (gbox1.xmin + gbox1.xmax + gbox2.xmin + gbox2.xmax) / 4.0;
		int zone = floor(1.0 + ((dzone + 180.0) / 6.0));
	
		/* Are these data below the equator? UTM South. */
		if( gbox1.ymax < 0.0 && gbox2.ymax < 0.0 )
		{
			PG_RETURN_INT32( -32700 - zone );
		}
		/* Are these data above the equator? UTM North. */
		else
		{
			PG_RETURN_INT32( -32600 - zone );
		}
	}
	
	/*
	** Running out of options... fall-back to Mercator and hope for the best.
	*/
	PG_RETURN_INT32(-3395);
		
}


