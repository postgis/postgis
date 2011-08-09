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

#include "liblwgeom.h"         /* For standard geometry types. */
#include "liblwgeom_internal.h"         /* For FP comparators. */
#include "lwgeom_pg.h"       /* For debugging macros. */
#include "geography.h"	     /* For utility functions. */

Datum geography_distance(PG_FUNCTION_ARGS);
Datum geography_dwithin(PG_FUNCTION_ARGS);
Datum geography_area(PG_FUNCTION_ARGS);
Datum geography_length(PG_FUNCTION_ARGS);
Datum geography_expand(PG_FUNCTION_ARGS);
Datum geography_point_outside(PG_FUNCTION_ARGS);
Datum geography_covers(PG_FUNCTION_ARGS);
Datum geography_bestsrid(PG_FUNCTION_ARGS);
Datum geography_perimeter(PG_FUNCTION_ARGS);

/*
** geography_distance(GSERIALIZED *g1, GSERIALIZED *g2, double tolerance, boolean use_spheroid)
** returns double distance in meters
*/
PG_FUNCTION_INFO_V1(geography_distance);
Datum geography_distance(PG_FUNCTION_ARGS)
{
	LWGEOM *lwgeom1 = NULL;
	LWGEOM *lwgeom2 = NULL;
	GSERIALIZED *g1 = NULL;
	GSERIALIZED *g2 = NULL;
	double distance;
	double tolerance;
	bool use_spheroid;
	SPHEROID s;

	/* Get our geometry objects loaded into memory. */
	g1 = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	g2 = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	/* Read our tolerance value. */
	tolerance = PG_GETARG_FLOAT8(2);

	/* Read our calculation type. */
	use_spheroid = PG_GETARG_BOOL(3);

	/* Initialize spheroid */
	spheroid_init(&s, WGS84_MAJOR_AXIS, WGS84_MINOR_AXIS);

	/* Set to sphere if requested */
	if ( ! use_spheroid )
		s.a = s.b = s.radius;

	lwgeom1 = lwgeom_from_gserialized(g1);
	lwgeom2 = lwgeom_from_gserialized(g2);

	/* Return NULL on empty arguments. */
	if ( lwgeom_is_empty(lwgeom1) || lwgeom_is_empty(lwgeom2) )
	{
		PG_FREE_IF_COPY(g1, 0);
		PG_FREE_IF_COPY(g2, 1);
		PG_RETURN_NULL();
	}

	distance = lwgeom_distance_spheroid(lwgeom1, lwgeom2, &s, FP_TOLERANCE);

	/* Something went wrong, negative return... should already be eloged, return NULL */
	if ( distance < 0.0 )
	{
		PG_FREE_IF_COPY(g1, 0);
		PG_FREE_IF_COPY(g2, 1);
		PG_RETURN_NULL();
	}

	/* Clean up, but not all the way to the point arrays */
	lwgeom_release(lwgeom1);
	lwgeom_release(lwgeom2);

	PG_FREE_IF_COPY(g1, 0);
	PG_FREE_IF_COPY(g2, 1);
	PG_RETURN_FLOAT8(distance);

}

/*
** geography_dwithin(GSERIALIZED *g1, GSERIALIZED *g2, double tolerance, boolean use_spheroid)
** returns double distance in meters
*/
PG_FUNCTION_INFO_V1(geography_dwithin);
Datum geography_dwithin(PG_FUNCTION_ARGS)
{
	LWGEOM *lwgeom1 = NULL;
	LWGEOM *lwgeom2 = NULL;
	GSERIALIZED *g1 = NULL;
	GSERIALIZED *g2 = NULL;
	double tolerance;
	double distance;
	bool use_spheroid;
	SPHEROID s;

	/* Get our geometry objects loaded into memory. */
	g1 = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	g2 = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	/* Read our tolerance value. */
	tolerance = PG_GETARG_FLOAT8(2);

	/* Read our calculation type. */
	use_spheroid = PG_GETARG_BOOL(3);

	/* Initialize spheroid */
	spheroid_init(&s, WGS84_MAJOR_AXIS, WGS84_MINOR_AXIS);

	/* Set to sphere if requested */
	if ( ! use_spheroid )
		s.a = s.b = s.radius;

	lwgeom1 = lwgeom_from_gserialized(g1);
	lwgeom2 = lwgeom_from_gserialized(g2);

	/* Return FALSE on empty arguments. */
	if ( lwgeom_is_empty(lwgeom1) || lwgeom_is_empty(lwgeom2) )
	{
		PG_RETURN_BOOL(FALSE);
	}

	distance = lwgeom_distance_spheroid(lwgeom1, lwgeom2, &s, tolerance);

	/* Something went wrong... should already be eloged, return FALSE */
	if ( distance < 0.0 )
	{
		elog(ERROR, "lwgeom_distance_spheroid returned negative!");
		PG_RETURN_BOOL(FALSE);
	}

	/* Clean up, but not all the way to the point arrays */
	lwgeom_release(lwgeom1);
	lwgeom_release(lwgeom2);

	PG_FREE_IF_COPY(g1, 0);
	PG_FREE_IF_COPY(g2, 1);
	PG_RETURN_BOOL(distance < tolerance);
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
	GSERIALIZED *g = NULL;
	GSERIALIZED *g_out = NULL;
	double distance;

	/* Get a wholly-owned pointer to the geography */
	g = (GSERIALIZED*)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));

	/* Read our distance value and normalize to unit-sphere. */
	distance = PG_GETARG_FLOAT8(1) / WGS84_RADIUS;

	/* Try the expansion */
	g_out = gserialized_expand(g, distance);

	/* If the expansion fails, the return our input */
	if ( g_out == NULL )
	{
		PG_RETURN_POINTER(g);
	}
	
	if ( g_out != g )
	{
		pfree(g);
	}

	PG_RETURN_POINTER(g_out);
}

/*
** geography_area(GSERIALIZED *g)
** returns double area in meters square
*/
PG_FUNCTION_INFO_V1(geography_area);
Datum geography_area(PG_FUNCTION_ARGS)
{
	LWGEOM *lwgeom = NULL;
	GSERIALIZED *g = NULL;
	GBOX gbox;
	double area;
	bool use_spheroid = LW_TRUE;
	SPHEROID s;

	/* Get our geometry object loaded into memory. */
	g = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* Read our calculation type */
	use_spheroid = PG_GETARG_BOOL(1);

	/* Initialize spheroid */
	spheroid_init(&s, WGS84_MAJOR_AXIS, WGS84_MINOR_AXIS);

	lwgeom = lwgeom_from_gserialized(g);

	/* EMPTY things have no area */
	if ( lwgeom_is_empty(lwgeom) )
	{
		lwgeom_release(lwgeom);
		PG_RETURN_FLOAT8(0.0);
	}
	
	if ( lwgeom->bbox )
		gbox = *(lwgeom->bbox);
	else
		lwgeom_calculate_gbox_geodetic(lwgeom, &gbox);

	/* Test for cases that are currently not handled by spheroid code */
	if ( use_spheroid )
	{
		/* We can't circle the poles right now */
		if ( FP_GTEQ(gbox.zmax,1.0) || FP_LTEQ(gbox.zmin,-1.0) )
			use_spheroid = LW_FALSE;
		/* We can't cross the equator right now */
		if ( gbox.zmax > 0.0 && gbox.zmin < 0.0 )
			use_spheroid = LW_FALSE;
	}

	/* User requests spherical calculation, turn our spheroid into a sphere */
	if ( ! use_spheroid )
		s.a = s.b = s.radius;

	/* Calculate the area */
	if ( use_spheroid )
		area = lwgeom_area_spheroid(lwgeom, &s);
	else
		area = lwgeom_area_sphere(lwgeom, &s);

	/* Something went wrong... */
	if ( area < 0.0 )
	{
		elog(ERROR, "lwgeom_area_spher(oid) returned area < 0.0");
		PG_RETURN_NULL();
	}

	/* Clean up, but not all the way to the point arrays */
	lwgeom_release(lwgeom);

	PG_FREE_IF_COPY(g, 0);
	PG_RETURN_FLOAT8(area);

}

/*
** geography_perimeter(GSERIALIZED *g)
** returns double perimeter in meters for area features
*/
PG_FUNCTION_INFO_V1(geography_perimeter);
Datum geography_perimeter(PG_FUNCTION_ARGS)
{
	LWGEOM *lwgeom = NULL;
	GSERIALIZED *g = NULL;
	double length;
	bool use_spheroid = LW_TRUE;
	SPHEROID s;
    int type;

	/* Get our geometry object loaded into memory. */
	g = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	
	/* Only return for area features. */
    type = gserialized_get_type(g);
    if ( ! (type == POLYGONTYPE || type == MULTIPOLYGONTYPE || type == COLLECTIONTYPE) )
    {
		PG_RETURN_FLOAT8(0.0);
    }
	
	lwgeom = lwgeom_from_gserialized(g);

	/* EMPTY things have no perimeter */
	if ( lwgeom_is_empty(lwgeom) )
	{
		lwgeom_free(lwgeom);
		PG_RETURN_FLOAT8(0.0);
	}

	/* Read our calculation type */
	use_spheroid = PG_GETARG_BOOL(1);

	/* Initialize spheroid */
	spheroid_init(&s, WGS84_MAJOR_AXIS, WGS84_MINOR_AXIS);

	/* User requests spherical calculation, turn our spheroid into a sphere */
	if ( ! use_spheroid )
		s.a = s.b = s.radius;

	/* Calculate the length */
	length = lwgeom_length_spheroid(lwgeom, &s);

	/* Something went wrong... */
	if ( length < 0.0 )
	{
		elog(ERROR, "lwgeom_length_spheroid returned length < 0.0");
		PG_RETURN_NULL();
	}

	/* Clean up, but not all the way to the point arrays */
	lwgeom_free(lwgeom);

	PG_FREE_IF_COPY(g, 0);
	PG_RETURN_FLOAT8(length);
}

/*
** geography_length(GSERIALIZED *g)
** returns double length in meters
*/
PG_FUNCTION_INFO_V1(geography_length);
Datum geography_length(PG_FUNCTION_ARGS)
{
	LWGEOM *lwgeom = NULL;
	GSERIALIZED *g = NULL;
	double length;
	bool use_spheroid = LW_TRUE;
	SPHEROID s;

	/* Get our geometry object loaded into memory. */
	g = (GSERIALIZED*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	lwgeom = lwgeom_from_gserialized(g);

	/* EMPTY things have no length */
	if ( lwgeom_is_empty(lwgeom) || lwgeom->type == POLYGONTYPE || lwgeom->type == MULTIPOLYGONTYPE )
	{
		lwgeom_free(lwgeom);
		PG_RETURN_FLOAT8(0.0);
	}

	/* Read our calculation type */
	use_spheroid = PG_GETARG_BOOL(1);

	/* Initialize spheroid */
	spheroid_init(&s, WGS84_MAJOR_AXIS, WGS84_MINOR_AXIS);

	/* User requests spherical calculation, turn our spheroid into a sphere */
	if ( ! use_spheroid )
		s.a = s.b = s.radius;

	/* Calculate the length */
	length = lwgeom_length_spheroid(lwgeom, &s);

	/* Something went wrong... */
	if ( length < 0.0 )
	{
		elog(ERROR, "lwgeom_length_spheroid returned length < 0.0");
		PG_RETURN_NULL();
	}

	/* Clean up */
	lwgeom_free(lwgeom);

	PG_FREE_IF_COPY(g, 0);
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
	if ( gserialized_get_gbox_p(g, &gbox) == LW_FAILURE )
	{
		LWGEOM *lwgeom = lwgeom_from_gserialized(g);
		LWDEBUGF(4,"unable to read gbox from gserialized, calculating from lwgeom (%p)", lwgeom);
		if ( lwgeom_calculate_gbox(lwgeom, &gbox) == LW_FAILURE )
		{
			LWDEBUG(4,"lwgeom_calculate_gbox returned LW_FAILURE");
			elog(ERROR, "Error in lwgeom_calculate_gbox calculation.");
			PG_RETURN_NULL();
		}
	}
	LWDEBUGF(4, "got gbox %s", gbox_to_string(&gbox));

	/* Get an exterior point, based on this gbox */
	gbox_pt_outside(&gbox, &pt);

	lwpoint = lwpoint_make2d(4326, pt.x, pt.y);

	g_out = gserialized_from_lwgeom((LWGEOM*)lwpoint, 1, &g_out_size);
	SET_VARSIZE(g_out, g_out_size);

	PG_FREE_IF_COPY(g, 0);
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
	if ( ! ( (type1 == POLYGONTYPE || type1 == MULTIPOLYGONTYPE || type1 == COLLECTIONTYPE) &&
	         (type2 == POINTTYPE || type2 == MULTIPOINTTYPE || type2 == COLLECTIONTYPE) ) )
	{
		elog(ERROR, "geography_covers: only POLYGON and POINT types are currently supported");
		PG_RETURN_NULL();
	}

	/* Construct our working geometries */
	lwgeom1 = lwgeom_from_gserialized(g1);
	lwgeom2 = lwgeom_from_gserialized(g2);

	/* EMPTY never intersects with another geometry */
	if ( lwgeom_is_empty(lwgeom1) || lwgeom_is_empty(lwgeom2) )
	{
		lwgeom_release(lwgeom1);
		lwgeom_release(lwgeom2);
		PG_FREE_IF_COPY(g1, 0);
		PG_FREE_IF_COPY(g2, 1);
		PG_RETURN_BOOL(false);
	}

	/* Calculate answer */
	result = lwgeom_covers_lwgeom_sphere(lwgeom1, lwgeom2);

	/* Clean up, but not all the way to the point arrays */
	lwgeom_release(lwgeom1);
	lwgeom_release(lwgeom2);

	PG_FREE_IF_COPY(g1, 0);
	PG_FREE_IF_COPY(g2, 1);
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
	if ( ! empty1 && lwgeom_calculate_gbox_cartesian(lwgeom1, &gbox1) == LW_FAILURE )
		elog(ERROR, "Error in geography_bestsrid calling lwgeom_calculate_gbox(lwgeom1, &gbox1)");

	LWDEBUGF(4, "calculated gbox = %s", gbox_to_string(&gbox1));

	/* If we have a unique second argument, fill in all the necessarily variables. */
	if ( d1 != d2 )
	{
		g2 = (GSERIALIZED*)PG_DETOAST_DATUM(d2);
		type2 = gserialized_get_type(g2);
		gbox2.flags = g2->flags;
		lwgeom2 = lwgeom_from_gserialized(g2);
		empty2 = lwgeom_is_empty(lwgeom2);
		if ( ! empty2 && lwgeom_calculate_gbox_cartesian(lwgeom2, &gbox2) == LW_FAILURE )
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


	/* Are these data arctic? Lambert Azimuthal Equal Area North. */
	if ( gbox1.ymin > 65.0 && gbox2.ymin > 65.0 )
	{
		PG_RETURN_INT32(-3574);
	}

	/* Are these data antarctic? Lambert Azimuthal Equal Area South. */
	if ( gbox1.ymin < -65.0 && gbox2.ymin < -65.0 )
	{
		PG_RETURN_INT32(-3409);
	}

	/*
	** Can we fit these data into one UTM zone? We will assume we can push things as
	** far as a half zone past a zone boundary. Note we have no handling for the
	** date line in here.
	*/
	if ( fabs(FP_MAX(gbox1.xmax, gbox2.xmax) - FP_MIN(gbox1.xmin, gbox2.xmin)) < 6.0 )
	{
		/* Cheap hack to pick a zone. Average of the box center points. */
		double dzone = (gbox1.xmin + gbox1.xmax + gbox2.xmin + gbox2.xmax) / 4.0;
		int zone = floor(1.0 + ((dzone + 180.0) / 6.0));

		/* Are these data below the equator? UTM South. */
		if ( gbox1.ymax < 0.0 && gbox2.ymax < 0.0 )
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


