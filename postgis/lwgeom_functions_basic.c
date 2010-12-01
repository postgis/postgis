/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2006 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "postgres.h"
#include "fmgr.h"
#include "utils/elog.h"
#include "utils/array.h"
#include "utils/geo_decls.h"

#include "liblwgeom_internal.h"
#include "libtgeom.h"
#include "lwalgorithm.h"
#include "lwgeom_pg.h"
#include "profile.h"

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

Datum LWGEOM_mem_size(PG_FUNCTION_ARGS);
Datum LWGEOM_summary(PG_FUNCTION_ARGS);
Datum LWGEOM_npoints(PG_FUNCTION_ARGS);
Datum LWGEOM_nrings(PG_FUNCTION_ARGS);
Datum LWGEOM_area_polygon(PG_FUNCTION_ARGS);
Datum LWGEOM_dwithin(PG_FUNCTION_ARGS);
Datum LWGEOM_dfullywithin(PG_FUNCTION_ARGS);
Datum postgis_uses_stats(PG_FUNCTION_ARGS);
Datum postgis_autocache_bbox(PG_FUNCTION_ARGS);
Datum postgis_scripts_released(PG_FUNCTION_ARGS);
Datum postgis_version(PG_FUNCTION_ARGS);
Datum postgis_lib_version(PG_FUNCTION_ARGS);
Datum postgis_libxml_version(PG_FUNCTION_ARGS);
Datum postgis_lib_build_date(PG_FUNCTION_ARGS);
Datum LWGEOM_length2d_linestring(PG_FUNCTION_ARGS);
Datum LWGEOM_length_linestring(PG_FUNCTION_ARGS);
Datum LWGEOM_perimeter2d_poly(PG_FUNCTION_ARGS);
Datum LWGEOM_perimeter_poly(PG_FUNCTION_ARGS);

Datum LWGEOM_maxdistance2d_linestring(PG_FUNCTION_ARGS);
Datum LWGEOM_mindistance2d(PG_FUNCTION_ARGS);
Datum LWGEOM_closestpoint(PG_FUNCTION_ARGS);
Datum LWGEOM_shortestline2d(PG_FUNCTION_ARGS);
Datum LWGEOM_longestline2d(PG_FUNCTION_ARGS);


Datum LWGEOM_maxdistance3d(PG_FUNCTION_ARGS);
Datum LWGEOM_mindistance3d(PG_FUNCTION_ARGS);
Datum LWGEOM_closestpoint3d(PG_FUNCTION_ARGS);
Datum LWGEOM_shortestline3d(PG_FUNCTION_ARGS);
Datum LWGEOM_longestline3d(PG_FUNCTION_ARGS);

Datum LWGEOM_dwithin3d(PG_FUNCTION_ARGS);
Datum LWGEOM_dfullywithin3d(PG_FUNCTION_ARGS);

Datum LWGEOM_inside_circle_point(PG_FUNCTION_ARGS);
Datum LWGEOM_collect(PG_FUNCTION_ARGS);
Datum LWGEOM_accum(PG_FUNCTION_ARGS);
Datum LWGEOM_collect_garray(PG_FUNCTION_ARGS);
Datum LWGEOM_expand(PG_FUNCTION_ARGS);
Datum LWGEOM_to_BOX(PG_FUNCTION_ARGS);
Datum LWGEOM_envelope(PG_FUNCTION_ARGS);
Datum LWGEOM_isempty(PG_FUNCTION_ARGS);
Datum LWGEOM_segmentize2d(PG_FUNCTION_ARGS);
Datum LWGEOM_reverse(PG_FUNCTION_ARGS);
Datum LWGEOM_force_clockwise_poly(PG_FUNCTION_ARGS);
Datum LWGEOM_noop(PG_FUNCTION_ARGS);
Datum LWGEOM_zmflag(PG_FUNCTION_ARGS);
Datum LWGEOM_hasz(PG_FUNCTION_ARGS);
Datum LWGEOM_hasm(PG_FUNCTION_ARGS);
Datum LWGEOM_ndims(PG_FUNCTION_ARGS);
Datum LWGEOM_makepoint(PG_FUNCTION_ARGS);
Datum LWGEOM_makepoint3dm(PG_FUNCTION_ARGS);
Datum LWGEOM_makeline_garray(PG_FUNCTION_ARGS);
Datum LWGEOM_makeline(PG_FUNCTION_ARGS);
Datum LWGEOM_makepoly(PG_FUNCTION_ARGS);
Datum LWGEOM_line_from_mpoint(PG_FUNCTION_ARGS);
Datum LWGEOM_addpoint(PG_FUNCTION_ARGS);
Datum LWGEOM_removepoint(PG_FUNCTION_ARGS);
Datum LWGEOM_setpoint_linestring(PG_FUNCTION_ARGS);
Datum LWGEOM_asEWKT(PG_FUNCTION_ARGS);
Datum LWGEOM_hasBBOX(PG_FUNCTION_ARGS);
Datum LWGEOM_azimuth(PG_FUNCTION_ARGS);
Datum LWGEOM_affine(PG_FUNCTION_ARGS);
Datum LWGEOM_longitude_shift(PG_FUNCTION_ARGS);
Datum optimistic_overlap(PG_FUNCTION_ARGS);
Datum ST_GeoHash(PG_FUNCTION_ARGS);
Datum ST_MakeEnvelope(PG_FUNCTION_ARGS);
Datum ST_CollectionExtract(PG_FUNCTION_ARGS);
Datum ST_IsCollection(PG_FUNCTION_ARGS);


/*------------------------------------------------------------------*/

/** find the size of geometry */
PG_FUNCTION_INFO_V1(LWGEOM_mem_size);
Datum LWGEOM_mem_size(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	size_t size = VARSIZE(geom);
	size_t computed_size = lwgeom_size(SERIALIZED_FORM(geom));
	computed_size += VARHDRSZ; /* varlena size */
	if ( size != computed_size )
	{
		elog(NOTICE, "varlena size (%lu) != computed size+4 (%lu)",
		     (unsigned long)size,
		     (unsigned long)computed_size);
	}

	PG_FREE_IF_COPY(geom,0);
	PG_RETURN_INT32(size);
}

/** get summary info on a GEOMETRY */
PG_FUNCTION_INFO_V1(LWGEOM_summary);
Datum LWGEOM_summary(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	char *result;
	text *mytext;
	LWGEOM *lwgeom;

	lwgeom = lwgeom_deserialize(SERIALIZED_FORM(geom));

	result = lwgeom_summary(lwgeom, 0);

	/* create a text obj to return */
	mytext = (text *) lwalloc(VARHDRSZ  + strlen(result) + 1);
	SET_VARSIZE(mytext, VARHDRSZ + strlen(result) + 1);
	VARDATA(mytext)[0] = '\n';
	memcpy(VARDATA(mytext)+1, result, strlen(result) );

	lwfree(result);
	PG_FREE_IF_COPY(geom,0);

	PG_RETURN_POINTER(mytext);
}

PG_FUNCTION_INFO_V1(postgis_version);
Datum postgis_version(PG_FUNCTION_ARGS)
{
	char *ver = POSTGIS_VERSION;
	text *result;
	result = lwalloc(VARHDRSZ  + strlen(ver));
	SET_VARSIZE(result, VARHDRSZ + strlen(ver));
	memcpy(VARDATA(result), ver, strlen(ver));
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(postgis_lib_version);
Datum postgis_lib_version(PG_FUNCTION_ARGS)
{
	char *ver = POSTGIS_LIB_VERSION;
	text *result;
	result = lwalloc(VARHDRSZ  + strlen(ver));
	SET_VARSIZE(result, VARHDRSZ + strlen(ver));
	memcpy(VARDATA(result), ver, strlen(ver));
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(postgis_lib_build_date);
Datum postgis_lib_build_date(PG_FUNCTION_ARGS)
{
	char *ver = POSTGIS_BUILD_DATE;
	text *result;
	result = lwalloc(VARHDRSZ  + strlen(ver));
	SET_VARSIZE(result, VARHDRSZ + strlen(ver));
	memcpy(VARDATA(result), ver, strlen(ver));
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(postgis_scripts_released);
Datum postgis_scripts_released(PG_FUNCTION_ARGS)
{
	char *ver = POSTGIS_SCRIPTS_VERSION;
	text *result;
	result = lwalloc(VARHDRSZ  + strlen(ver));
	SET_VARSIZE(result, VARHDRSZ + strlen(ver));
	memcpy(VARDATA(result), ver, strlen(ver));
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(postgis_uses_stats);
Datum postgis_uses_stats(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(TRUE);
}

PG_FUNCTION_INFO_V1(postgis_autocache_bbox);
Datum postgis_autocache_bbox(PG_FUNCTION_ARGS)
{
#ifdef POSTGIS_AUTOCACHE_BBOX
	PG_RETURN_BOOL(TRUE);
#else
	PG_RETURN_BOOL(FALSE);
#endif
}


PG_FUNCTION_INFO_V1(postgis_libxml_version);
Datum postgis_libxml_version(PG_FUNCTION_ARGS)
{
	char *ver = POSTGIS_LIBXML2_VERSION;
	text *result;
	result = lwalloc(VARHDRSZ  + strlen(ver));
	SET_VARSIZE(result, VARHDRSZ + strlen(ver));
	memcpy(VARDATA(result), ver, strlen(ver));
	PG_RETURN_POINTER(result);
}

/** number of points in an object */
PG_FUNCTION_INFO_V1(LWGEOM_npoints);
Datum LWGEOM_npoints(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *lwgeom = pglwgeom_deserialize(geom);
	int npoints = 0;

	npoints = lwgeom_count_vertices(lwgeom);
	lwgeom_release(lwgeom);

	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_INT32(npoints);
}

/** number of rings in an object */
PG_FUNCTION_INFO_V1(LWGEOM_nrings);
Datum LWGEOM_nrings(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *lwgeom = pglwgeom_deserialize(geom);
	int nrings = 0;

	nrings = lwgeom_count_rings(lwgeom);
	lwgeom_release(lwgeom);

	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_INT32(nrings);
}

/**
 * @brief Calculate the area of all the subobj in a polygon
 * 		area(point) = 0
 * 		area (line) = 0
 * 		area(polygon) = find its 2d area
 */
PG_FUNCTION_INFO_V1(LWGEOM_area_polygon);
Datum LWGEOM_area_polygon(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *lwgeom = pglwgeom_deserialize(geom);
	double area = 0.0;

	POSTGIS_DEBUG(2, "in LWGEOM_area_polygon");

	area = lwgeom_area(lwgeom);

	lwgeom_free(lwgeom);	
	PG_FREE_IF_COPY(geom, 0);
	
	PG_RETURN_FLOAT8(area);
}

/**
 * @brief find the "length of a geometry"
 *  	length2d(point) = 0
 *  	length2d(line) = length of line
 *  	length2d(polygon) = 0  -- could make sense to return sum(ring perimeter)
 *  	uses euclidian 2d length (even if input is 3d)
 */
PG_FUNCTION_INFO_V1(LWGEOM_length2d_linestring);
Datum LWGEOM_length2d_linestring(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *lwgeom = pglwgeom_deserialize(geom);
	double dist = lwgeom_length_2d(lwgeom);
	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_FLOAT8(dist);
}

/**
 * @brief find the "length of a geometry"
 *  	length(point) = 0
 *  	length(line) = length of line
 *  	length(polygon) = 0  -- could make sense to return sum(ring perimeter)
 *  	uses euclidian 3d/2d length depending on input dimensions.
 */
PG_FUNCTION_INFO_V1(LWGEOM_length_linestring);
Datum LWGEOM_length_linestring(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *lwgeom = pglwgeom_deserialize(geom);
	double dist = lwgeom_length(lwgeom);
	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_FLOAT8(dist);
}

/**
 *  @brief find the "perimeter of a geometry"
 *  	perimeter(point) = 0
 *  	perimeter(line) = 0
 *  	perimeter(polygon) = sum of ring perimeters
 *  	uses euclidian 3d/2d computation depending on input dimension.
 */
PG_FUNCTION_INFO_V1(LWGEOM_perimeter_poly);
Datum LWGEOM_perimeter_poly(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *lwgeom = pglwgeom_deserialize(geom);
	double perimeter = 0.0;
	
	perimeter = lwgeom_perimeter(lwgeom);
	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_FLOAT8(perimeter);
}

/**
 *  @brief find the "perimeter of a geometry"
 *  	perimeter(point) = 0
 *  	perimeter(line) = 0
 *  	perimeter(polygon) = sum of ring perimeters
 *  	uses euclidian 2d computation even if input is 3d
 */
PG_FUNCTION_INFO_V1(LWGEOM_perimeter2d_poly);
Datum LWGEOM_perimeter2d_poly(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	LWGEOM *lwgeom = pglwgeom_deserialize(geom);
	double perimeter = 0.0;
	
	perimeter = lwgeom_perimeter_2d(lwgeom);
	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_FLOAT8(perimeter);
}


/* transform input geometry to 2d if not 2d already */
PG_FUNCTION_INFO_V1(LWGEOM_force_2d);
Datum LWGEOM_force_2d(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *pg_geom_in = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *pg_geom_out;
	LWGEOM *lwg_in, *lwg_out;

	/* already 2d */
	if ( lwgeom_ndims(pg_geom_in->type) == 2 ) PG_RETURN_POINTER(pg_geom_in);

	lwg_in = pglwgeom_deserialize(pg_geom_in);
	lwg_out = lwgeom_force_2d(lwg_in);
	pg_geom_out = pglwgeom_serialize(lwg_out);
	lwgeom_free(lwg_out);
	lwgeom_free(lwg_in);

	PG_FREE_IF_COPY(pg_geom_in, 0);
	PG_RETURN_POINTER(pg_geom_out);
}

/* transform input geometry to 3dz if not 3dz already */
PG_FUNCTION_INFO_V1(LWGEOM_force_3dz);
Datum LWGEOM_force_3dz(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *pg_geom_in = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *pg_geom_out;
	LWGEOM *lwg_in, *lwg_out;

	/* already 3d */
	if ( lwgeom_ndims(pg_geom_in->type) == 3 && TYPE_HASZ(pg_geom_in->type) ) 
		PG_RETURN_POINTER(pg_geom_in);

	lwg_in = pglwgeom_deserialize(pg_geom_in);
	lwg_out = lwgeom_force_3dz(lwg_in);
	pg_geom_out = pglwgeom_serialize(lwg_out);
	lwgeom_free(lwg_out);
	lwgeom_free(lwg_in);

	PG_FREE_IF_COPY(pg_geom_in, 0);
	PG_RETURN_POINTER(pg_geom_out);
}

/** transform input geometry to 3dm if not 3dm already */
PG_FUNCTION_INFO_V1(LWGEOM_force_3dm);
Datum LWGEOM_force_3dm(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *pg_geom_in = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *pg_geom_out;
	LWGEOM *lwg_in, *lwg_out;

	/* already 3d */
	if ( lwgeom_ndims(pg_geom_in->type) == 3 && TYPE_HASM(pg_geom_in->type) ) 
		PG_RETURN_POINTER(pg_geom_in);

	lwg_in = pglwgeom_deserialize(pg_geom_in);
	lwg_out = lwgeom_force_3dm(lwg_in);
	pg_geom_out = pglwgeom_serialize(lwg_out);
	lwgeom_free(lwg_out);
	lwgeom_free(lwg_in);

	PG_FREE_IF_COPY(pg_geom_in, 0);
	PG_RETURN_POINTER(pg_geom_out);
}

/* transform input geometry to 4d if not 4d already */
PG_FUNCTION_INFO_V1(LWGEOM_force_4d);
Datum LWGEOM_force_4d(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *pg_geom_in = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *pg_geom_out;
	LWGEOM *lwg_in, *lwg_out;

	/* already 4d */
	if ( lwgeom_ndims(pg_geom_in->type) == 4 ) 
		PG_RETURN_POINTER(pg_geom_in);

	lwg_in = pglwgeom_deserialize(pg_geom_in);
	lwg_out = lwgeom_force_4d(lwg_in);
	pg_geom_out = pglwgeom_serialize(lwg_out);
	lwgeom_free(lwg_out);
	lwgeom_free(lwg_in);

	PG_FREE_IF_COPY(pg_geom_in, 0);
	PG_RETURN_POINTER(pg_geom_out);
}

/** transform input geometry to a collection type */
PG_FUNCTION_INFO_V1(LWGEOM_force_collection);
Datum LWGEOM_force_collection(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *result;
	LWGEOM *lwgeoms[1];
	LWGEOM *lwgeom;
	int srid;
	GBOX *bbox;

	POSTGIS_DEBUG(2, "LWGEOM_force_collection called");

	/*
	 * This funx is a no-op only if a bbox cache is already present
	 * in input. If bbox cache is not there we'll need to handle
	 * automatic bbox addition FOR_COMPLEX_GEOMS.
	 */
	if ( TYPE_GETTYPE(geom->type) == COLLECTIONTYPE &&
	        TYPE_HASBBOX(geom->type) )
	{
		PG_RETURN_POINTER(geom);
	}

	/* deserialize into lwgeoms[0] */
	lwgeom = lwgeom_deserialize(SERIALIZED_FORM(geom));

	/* alread a multi*, just make it a collection */
	if ( lwtype_is_collection(lwgeom->type) )
	{
		lwgeom->type = COLLECTIONTYPE;
	}

	/* single geom, make it a collection */
	else
	{
		srid = lwgeom->srid;
		/* We transfer bbox ownership from input to output */
		bbox = lwgeom->bbox;
		lwgeom->srid = -1;
		lwgeom->bbox = NULL;
		lwgeoms[0] = lwgeom;
		lwgeom = (LWGEOM *)lwcollection_construct(COLLECTIONTYPE,
		         srid, bbox, 1,
		         lwgeoms);
	}

	result = pglwgeom_serialize(lwgeom);
	lwgeom_release(lwgeom);

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result);
}

/** transform input geometry to a multi* type */
PG_FUNCTION_INFO_V1(LWGEOM_force_multi);
Datum LWGEOM_force_multi(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *result;
	LWGEOM *lwgeom;
	LWGEOM *ogeom;

	POSTGIS_DEBUG(2, "LWGEOM_force_multi called");

	/*
	** This funx is a no-op only if a bbox cache is already present
	** in input. If bbox cache is not there we'll need to handle
	** automatic bbox addition FOR_COMPLEX_GEOMS.
	*/
	if ( lwtype_is_collection(TYPE_GETTYPE(geom->type)) && 
	     TYPE_HASBBOX(geom->type) )
	{
		PG_RETURN_POINTER(geom);
	}


	/* deserialize into lwgeoms[0] */
	lwgeom = lwgeom_deserialize(SERIALIZED_FORM(geom));
	ogeom = lwgeom_as_multi(lwgeom);

	result = pglwgeom_serialize(ogeom);

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result);
}

/**
Returns the point in first input geometry that is closest to the second input geometry
*/

PG_FUNCTION_INFO_V1(LWGEOM_closestpoint);
Datum LWGEOM_closestpoint(PG_FUNCTION_ARGS)
{
	int srid;
	LWGEOM *geom1;
	LWGEOM *geom2;
	LWGEOM *point;

	geom1 = lwgeom_deserialize(SERIALIZED_FORM((PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0))));
	geom2 = lwgeom_deserialize(SERIALIZED_FORM((PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1))));

	if (geom1->srid != geom2->srid)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}

	srid = geom1->srid;

	point = lw_dist2d_distancepoint(geom1, geom2, srid, DIST_MIN);
	if (lwgeom_is_empty(point))
	{
		PG_RETURN_NULL();
	}
	PG_RETURN_POINTER(pglwgeom_serialize(point));
}

/**
Returns the shortest line between two geometries
*/
PG_FUNCTION_INFO_V1(LWGEOM_shortestline2d);
Datum LWGEOM_shortestline2d(PG_FUNCTION_ARGS)
{
	int srid;
	LWGEOM *geom1;
	LWGEOM *geom2;
	LWGEOM *theline;

	geom1 = lwgeom_deserialize(SERIALIZED_FORM((PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0))));
	geom2 = lwgeom_deserialize(SERIALIZED_FORM((PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1))));

	if (geom1->srid != geom2->srid)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}

	srid = geom1->srid;

	theline = lw_dist2d_distanceline(geom1, geom2, srid, DIST_MIN);
	if (lwgeom_is_empty(theline))
	{
		PG_RETURN_NULL();
	}
	PG_RETURN_POINTER(pglwgeom_serialize(theline));
}

/**
Returns the longest line between two geometries
*/
PG_FUNCTION_INFO_V1(LWGEOM_longestline2d);
Datum LWGEOM_longestline2d(PG_FUNCTION_ARGS)
{
	int srid;
	LWGEOM *geom1;
	LWGEOM *geom2;
	LWGEOM *theline;

	geom1 = lwgeom_deserialize(SERIALIZED_FORM((PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0))));
	geom2 = lwgeom_deserialize(SERIALIZED_FORM((PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1))));

	if (geom1->srid != geom2->srid)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}

	srid = geom1->srid;

	theline = lw_dist2d_distanceline(geom1, geom2, srid, DIST_MAX);
	if (lwgeom_is_empty(theline))
	{
		PG_RETURN_NULL();
	}
	PG_RETURN_POINTER(pglwgeom_serialize(theline));
}
/**
 Minimum 2d distance between objects in geom1 and geom2.
 */
PG_FUNCTION_INFO_V1(LWGEOM_mindistance2d);
Datum LWGEOM_mindistance2d(PG_FUNCTION_ARGS)
{
	LWGEOM *geom1;
	LWGEOM *geom2;
	double mindist;

	PROFSTART(PROF_QRUN);

	geom1 = lwgeom_deserialize(SERIALIZED_FORM((PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0))));
	geom2 = lwgeom_deserialize(SERIALIZED_FORM((PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1))));

	if (geom1->srid != geom2->srid)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}

	mindist = lwgeom_mindistance2d(geom1, geom2);

	PROFSTOP(PROF_QRUN);
	PROFREPORT("dist",geom1, geom2, NULL);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);
	/*if called with empty geometries the ingoing mindistance is untouched, and makes us return NULL*/
	if (mindist<MAXFLOAT)
	{
		PG_RETURN_FLOAT8(mindist);
	}
	PG_RETURN_NULL();
}

/**
Returns boolean describing if
mininimum 2d distance between objects in
geom1 and geom2 is shorter than tolerance
*/
PG_FUNCTION_INFO_V1(LWGEOM_dwithin);
Datum LWGEOM_dwithin(PG_FUNCTION_ARGS)
{
	LWGEOM *geom1;
	LWGEOM *geom2;
	double mindist, tolerance;

	PROFSTART(PROF_QRUN);

	geom1 = lwgeom_deserialize(SERIALIZED_FORM((PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0))));
	geom2 = lwgeom_deserialize(SERIALIZED_FORM((PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1))));
	tolerance = PG_GETARG_FLOAT8(2);

	if ( tolerance < 0 )
	{
		elog(ERROR,"Tolerance cannot be less than zero\n");
		PG_RETURN_NULL();
	}

	if (geom1->srid != geom2->srid)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}

	mindist = lwgeom_mindistance2d_tolerance(geom1,geom2,tolerance);

	PROFSTOP(PROF_QRUN);
	PROFREPORT("dist",geom1, geom2, NULL);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);
	/*empty geometries cases should be right handled since return from underlying
	 functions should be MAXFLOAT which causes false as answer*/
	PG_RETURN_BOOL(tolerance >= mindist);
}

/**
Returns boolean describing if
maximum 2d distance between objects in
geom1 and geom2 is shorter than tolerance
*/
PG_FUNCTION_INFO_V1(LWGEOM_dfullywithin);
Datum LWGEOM_dfullywithin(PG_FUNCTION_ARGS)
{
	LWGEOM *geom1;
	LWGEOM *geom2;
	double maxdist, tolerance;

	PROFSTART(PROF_QRUN);

	geom1 = lwgeom_deserialize(SERIALIZED_FORM((PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0))));
	geom2 = lwgeom_deserialize(SERIALIZED_FORM((PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1))));
	tolerance = PG_GETARG_FLOAT8(2);

	if ( tolerance < 0 )
	{
		elog(ERROR,"Tolerance cannot be less than zero\n");
		PG_RETURN_NULL();
	}

	if (geom1->srid != geom2->srid)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}
	maxdist = lwgeom_maxdistance2d_tolerance(geom1, geom2, tolerance);
	PROFSTOP(PROF_QRUN);
	PROFREPORT("dist",geom1, geom2, NULL);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);
	/*If function is feed with empty geometries we should return false*/
	if (maxdist>-1)
	{
		PG_RETURN_BOOL(tolerance >= maxdist);
	}
	PG_RETURN_BOOL(LW_FALSE);
}

/**
 Maximum 2d distance between objects in geom1 and geom2.
 */
PG_FUNCTION_INFO_V1(LWGEOM_maxdistance2d_linestring);
Datum LWGEOM_maxdistance2d_linestring(PG_FUNCTION_ARGS)
{
	LWGEOM *geom1;
	LWGEOM *geom2;
	double maxdist;

	PROFSTART(PROF_QRUN);

	geom1 = lwgeom_deserialize(SERIALIZED_FORM((PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0))));
	geom2 = lwgeom_deserialize(SERIALIZED_FORM((PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1))));

	if (geom1->srid != geom2->srid)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}


	maxdist = lwgeom_maxdistance2d(geom1, geom2);

	PROFSTOP(PROF_QRUN);
	PROFREPORT("maxdist",geom1, geom2, NULL);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);
	/*if called with empty geometries the ingoing mindistance is untouched, and makes us return NULL*/
	if (maxdist>-1)
	{
		PG_RETURN_FLOAT8(maxdist);
	}
	PG_RETURN_NULL();
}

/**
Returns the point in first input geometry that is closest to the second input geometry in 3D
*/

PG_FUNCTION_INFO_V1(LWGEOM_closestpoint3d);
Datum LWGEOM_closestpoint3d(PG_FUNCTION_ARGS)
{
	int srid;
	LWGEOM *geom1;
	LWGEOM *geom2;
	LWGEOM *point;

	geom1 = lwgeom_deserialize(SERIALIZED_FORM((PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0))));
	geom2 = lwgeom_deserialize(SERIALIZED_FORM((PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1))));

	if (geom1->srid != geom2->srid)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}

	srid = geom1->srid;

	point = lw_dist3d_distancepoint(geom1, geom2, srid, DIST_MIN);

	if (lwgeom_is_empty(point))
	{
		PG_RETURN_NULL();
	}
	PG_RETURN_POINTER(pglwgeom_serialize(point));
}

/**
Returns the shortest line between two geometries in 3D
*/
PG_FUNCTION_INFO_V1(LWGEOM_shortestline3d);
Datum LWGEOM_shortestline3d(PG_FUNCTION_ARGS)
{
	int srid;
	LWGEOM *geom1;
	LWGEOM *geom2;
	LWGEOM *theline;

	geom1 = lwgeom_deserialize(SERIALIZED_FORM((PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0))));
	geom2 = lwgeom_deserialize(SERIALIZED_FORM((PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1))));

	if (geom1->srid != geom2->srid)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}

	srid = geom1->srid;

	theline = lw_dist3d_distanceline(geom1, geom2, srid, DIST_MIN);
	if (lwgeom_is_empty(theline))
	{
		PG_RETURN_NULL();
	}
	PG_RETURN_POINTER(pglwgeom_serialize(theline));
}

/**
Returns the longest line between two geometries in 3D
*/
PG_FUNCTION_INFO_V1(LWGEOM_longestline3d);
Datum LWGEOM_longestline3d(PG_FUNCTION_ARGS)
{
	int srid;
	LWGEOM *geom1;
	LWGEOM *geom2;
	LWGEOM *theline;

	geom1 = lwgeom_deserialize(SERIALIZED_FORM((PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0))));
	geom2 = lwgeom_deserialize(SERIALIZED_FORM((PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1))));

	if (geom1->srid != geom2->srid)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}

	srid = geom1->srid;

	theline = lw_dist3d_distanceline(geom1, geom2, srid, DIST_MAX);
	if (lwgeom_is_empty(theline))
	{
		PG_RETURN_NULL();
	}
	PG_RETURN_POINTER(pglwgeom_serialize(theline));
}
/**
 Minimum 2d distance between objects in geom1 and geom2 in 3D
 */
PG_FUNCTION_INFO_V1(LWGEOM_mindistance3d);
Datum LWGEOM_mindistance3d(PG_FUNCTION_ARGS)
{
	LWGEOM *geom1;
	LWGEOM *geom2;
	double mindist;

	PROFSTART(PROF_QRUN);

	geom1 = lwgeom_deserialize(SERIALIZED_FORM((PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0))));
	geom2 = lwgeom_deserialize(SERIALIZED_FORM((PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1))));

	if (geom1->srid != geom2->srid)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}

	mindist = lwgeom_mindistance3d(geom1, geom2);

	PROFSTOP(PROF_QRUN);
	PROFREPORT("dist",geom1, geom2, NULL);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);
	/*if called with empty geometries the ingoing mindistance is untouched, and makes us return NULL*/
	if (mindist<MAXFLOAT)
	{
		PG_RETURN_FLOAT8(mindist);
	}
	PG_RETURN_NULL();
}

/**
Returns boolean describing if
mininimum 3d distance between objects in
geom1 and geom2 is shorter than tolerance
*/
PG_FUNCTION_INFO_V1(LWGEOM_dwithin3d);
Datum LWGEOM_dwithin3d(PG_FUNCTION_ARGS)
{
	LWGEOM *geom1;
	LWGEOM *geom2;
	double mindist, tolerance;

	PROFSTART(PROF_QRUN);

	geom1 = lwgeom_deserialize(SERIALIZED_FORM((PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0))));
	geom2 = lwgeom_deserialize(SERIALIZED_FORM((PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1))));
	tolerance = PG_GETARG_FLOAT8(2);

	if ( tolerance < 0 )
	{
		elog(ERROR,"Tolerance cannot be less than zero\n");
		PG_RETURN_NULL();
	}

	if (geom1->srid != geom2->srid)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}

	mindist = lwgeom_mindistance3d_tolerance(geom1,geom2,tolerance);

	PROFSTOP(PROF_QRUN);
	PROFREPORT("dist",geom1, geom2, NULL);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);
	/*empty geometries cases should be right handled since return from underlying
	 functions should be MAXFLOAT which causes false as answer*/
	PG_RETURN_BOOL(tolerance >= mindist);
}

/**
Returns boolean describing if
maximum 3d distance between objects in
geom1 and geom2 is shorter than tolerance
*/
PG_FUNCTION_INFO_V1(LWGEOM_dfullywithin3d);
Datum LWGEOM_dfullywithin3d(PG_FUNCTION_ARGS)
{
	LWGEOM *geom1;
	LWGEOM *geom2;
	double maxdist, tolerance;

	PROFSTART(PROF_QRUN);

	geom1 = lwgeom_deserialize(SERIALIZED_FORM((PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0))));
	geom2 = lwgeom_deserialize(SERIALIZED_FORM((PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1))));
	tolerance = PG_GETARG_FLOAT8(2);

	if ( tolerance < 0 )
	{
		elog(ERROR,"Tolerance cannot be less than zero\n");
		PG_RETURN_NULL();
	}

	if (geom1->srid != geom2->srid)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}
	maxdist = lwgeom_maxdistance3d_tolerance(geom1, geom2, tolerance);
	PROFSTOP(PROF_QRUN);
	PROFREPORT("dist",geom1, geom2, NULL);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);
	/*If function is feed with empty geometries we should return false*/
	if (maxdist>-1)
	{
		PG_RETURN_BOOL(tolerance >= maxdist);
	}
	PG_RETURN_BOOL(LW_FALSE);
}

/**
 Maximum 3d distance between objects in geom1 and geom2.
 */
PG_FUNCTION_INFO_V1(LWGEOM_maxdistance3d);
Datum LWGEOM_maxdistance3d(PG_FUNCTION_ARGS)
{
	LWGEOM *geom1;
	LWGEOM *geom2;
	double maxdist;

	PROFSTART(PROF_QRUN);

	geom1 = lwgeom_deserialize(SERIALIZED_FORM((PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0))));
	geom2 = lwgeom_deserialize(SERIALIZED_FORM((PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1))));

	if (geom1->srid != geom2->srid)
	{
		elog(ERROR,"Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}


	maxdist = lwgeom_maxdistance3d(geom1, geom2);

	PROFSTOP(PROF_QRUN);
	PROFREPORT("maxdist",geom1, geom2, NULL);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);
	/*if called with empty geometries the ingoing mindistance is untouched, and makes us return NULL*/
	if (maxdist>-1)
	{
		PG_RETURN_FLOAT8(maxdist);
	}
	PG_RETURN_NULL();
}


PG_FUNCTION_INFO_V1(LWGEOM_longitude_shift);
Datum LWGEOM_longitude_shift(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	LWGEOM *lwgeom;
	PG_LWGEOM *ret;

	POSTGIS_DEBUG(2, "LWGEOM_longitude_shift called.");

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));
	lwgeom = pglwgeom_deserialize(geom);

	/* Drop bbox, will be recomputed */
	lwgeom_drop_bbox(lwgeom);

	/* Modify geometry */
	lwgeom_longitude_shift(lwgeom);

	/* Construct PG_LWGEOM */
	ret = pglwgeom_serialize(lwgeom);

	/* Release deserialized geometry */
	lwgeom_release(lwgeom);

	/* Release detoasted geometry */
	pfree(geom);

	PG_RETURN_POINTER(ret);
}

PG_FUNCTION_INFO_V1(LWGEOM_inside_circle_point);
Datum LWGEOM_inside_circle_point(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	double cx = PG_GETARG_FLOAT8(1);
	double cy = PG_GETARG_FLOAT8(2);
	double rr = PG_GETARG_FLOAT8(3);
	LWPOINT *point;
	POINT2D pt;

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	point = lwpoint_deserialize(SERIALIZED_FORM(geom));
	if ( point == NULL )
	{
		PG_FREE_IF_COPY(geom, 0);
		PG_RETURN_NULL(); /* not a point */
	}

	getPoint2d_p(point->point, 0, &pt);

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_BOOL(lwgeom_pt_inside_circle(&pt, cx, cy, rr));
}

/**
 *  @brief collect( geom, geom ) returns a geometry which contains
 *  		all the sub_objects from both of the argument geometries
 *  @return geometry is the simplest possible, based on the types
 *  	of the collected objects
 *  	ie. if all are of either X or multiX, then a multiX is returned.
 */
PG_FUNCTION_INFO_V1(LWGEOM_collect);
Datum LWGEOM_collect(PG_FUNCTION_ARGS)
{
	Pointer geom1_ptr = PG_GETARG_POINTER(0);
	Pointer geom2_ptr =  PG_GETARG_POINTER(1);
	PG_LWGEOM *pglwgeom1, *pglwgeom2, *result;
	LWGEOM *lwgeoms[2], *outlwg;
	uint32 type1, type2, outtype;
	GBOX *box=NULL;
	int srid;

	POSTGIS_DEBUG(2, "LWGEOM_collect called.");

	/* return null if both geoms are null */
	if ( (geom1_ptr == NULL) && (geom2_ptr == NULL) )
	{
		PG_RETURN_NULL();
	}

	/* return a copy of the second geom if only first geom is null */
	if (geom1_ptr == NULL)
	{
		result = (PG_LWGEOM *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(1));
		PG_RETURN_POINTER(result);
	}

	/* return a copy of the first geom if only second geom is null */
	if (geom2_ptr == NULL)
	{
		result = (PG_LWGEOM *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));
		PG_RETURN_POINTER(result);
	}


	pglwgeom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	pglwgeom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	POSTGIS_DEBUGF(3, "LWGEOM_collect(%s, %s): call", lwtype_name(TYPE_GETTYPE(pglwgeom1->type)), lwtype_name(TYPE_GETTYPE(pglwgeom2->type)));

#if 0
	if ( pglwgeom_get_srid(pglwgeom1) != pglwgeom_get_srid(pglwgeom2) )
	{
		elog(ERROR, "Operation on two GEOMETRIES with different SRIDs\n");
		PG_RETURN_NULL();
	}
#endif

	srid = pglwgeom_get_srid(pglwgeom1);
	error_if_srid_mismatch(srid, pglwgeom_get_srid(pglwgeom2));

	lwgeoms[0] = lwgeom_deserialize(SERIALIZED_FORM(pglwgeom1));
	lwgeoms[1] = lwgeom_deserialize(SERIALIZED_FORM(pglwgeom2));

	type1 = lwgeoms[0]->type;
	type2 = lwgeoms[1]->type;
	if ( type1 == type2 && type1 < 4 ) outtype = type1+3;
	else outtype = COLLECTIONTYPE;

	POSTGIS_DEBUGF(3, " outtype = %d", outtype);

	/* COMPUTE_BBOX WHEN_SIMPLE */
	if ( lwgeoms[0]->bbox && lwgeoms[1]->bbox )
	{
		int hasz=(FLAGS_GET_Z(lwgeoms[0]->flags) && FLAGS_GET_Z(lwgeoms[1]->flags));
		int hasm=(FLAGS_GET_M(lwgeoms[0]->flags) && FLAGS_GET_M(lwgeoms[1]->flags));

		box = palloc(sizeof(GBOX));
		FLAGS_SET_Z(box->flags, hasz?1:0);
		FLAGS_SET_M(box->flags, hasm?1:0);
		box->xmin = LW_MIN(lwgeoms[0]->bbox->xmin, lwgeoms[1]->bbox->xmin);
		box->ymin = LW_MIN(lwgeoms[0]->bbox->ymin, lwgeoms[1]->bbox->ymin);
		box->xmax = LW_MAX(lwgeoms[0]->bbox->xmax, lwgeoms[1]->bbox->xmax);
		box->ymax = LW_MAX(lwgeoms[0]->bbox->ymax, lwgeoms[1]->bbox->ymax);
		if (hasz)
		{
			box->zmin = LW_MIN(lwgeoms[0]->bbox->zmin, lwgeoms[1]->bbox->zmin);
			box->zmax = LW_MAX(lwgeoms[0]->bbox->zmax, lwgeoms[1]->bbox->zmax);
		}
		if (hasm)
		{
			box->mmin = LW_MIN(lwgeoms[0]->bbox->mmin, lwgeoms[1]->bbox->mmin);
			box->mmax = LW_MAX(lwgeoms[0]->bbox->mmax, lwgeoms[1]->bbox->mmax);
		}
	}

	/* Drop input geometries bbox and SRID */
	lwgeom_drop_bbox(lwgeoms[0]);
	lwgeom_drop_srid(lwgeoms[0]);
	lwgeom_drop_bbox(lwgeoms[1]);
	lwgeom_drop_srid(lwgeoms[1]);

	outlwg = (LWGEOM *)lwcollection_construct(
	             outtype, srid,
	             box, 2, lwgeoms);

	result = pglwgeom_serialize(outlwg);

	PG_FREE_IF_COPY(pglwgeom1, 0);
	PG_FREE_IF_COPY(pglwgeom2, 1);
	lwgeom_release(lwgeoms[0]);
	lwgeom_release(lwgeoms[1]);

	PG_RETURN_POINTER(result);
}

/**
 * @brief This is a geometry array constructor
 * 		for use as aggregates sfunc.
 * 		Will have as input an array of Geometry pointers and a Geometry.
 * 		Will DETOAST given geometry and put a pointer to it
 * 			in the given array. DETOASTED value is first copied
 * 		to a safe memory context to avoid premature deletion.
 */
PG_FUNCTION_INFO_V1(LWGEOM_accum);
Datum LWGEOM_accum(PG_FUNCTION_ARGS)
{
	ArrayType *array = NULL;
	int nelems;
	int lbs=1;
	size_t nbytes, oldsize;
	Datum datum;
	PG_LWGEOM *geom;
	ArrayType *result;
	Oid oid = get_fn_expr_argtype(fcinfo->flinfo, 1);

	POSTGIS_DEBUG(2, "LWGEOM_accum called");

	datum = PG_GETARG_DATUM(0);
	if ( (Pointer *)datum == NULL )
	{
		array = NULL;
		nelems = 0;

		POSTGIS_DEBUG(3, "geom_accum: NULL array");
	}
	else
	{
		array = DatumGetArrayTypePCopy(datum);
		/*array = PG_GETARG_ARRAYTYPE_P(0); */
		nelems = ArrayGetNItems(ARR_NDIM(array), ARR_DIMS(array));

		POSTGIS_DEBUGF(3, "geom_accum: array of nelems=%d", nelems);
	}

	datum = PG_GETARG_DATUM(1);
	/* Do nothing, return state array */
	if ( (Pointer *)datum == NULL )
	{
		POSTGIS_DEBUGF(3, "geom_accum: NULL geom, nelems=%d", nelems);

		if ( array == NULL ) PG_RETURN_NULL();
		PG_RETURN_ARRAYTYPE_P(array);
	}

	/* Make a DETOASTED copy of input geometry */
	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(datum);

	POSTGIS_DEBUGF(3, "geom_accum: detoasted geom: %p", (void*)geom);

	/*
	 * Might use a more optimized version instead of lwrealloc'ing
	 * at every iteration. This is not the bottleneck anyway.
	 * 		--strk(TODO);
	 */
	++nelems;
	if ( nelems == 1 || ! array )
	{
		nbytes = ARR_OVERHEAD_NONULLS(1)+INTALIGN(VARSIZE(geom));

		POSTGIS_DEBUGF(3, "geom_accum: adding %p (nelems=%d; nbytes=%d)",
		               (void*)geom, nelems, (int)nbytes);

		result = lwalloc(nbytes);
		if ( ! result )
		{
			elog(ERROR, "Out of virtual memory");
			PG_RETURN_NULL();
		}

		SET_VARSIZE(result, nbytes);
		result->ndim = 1;
		result->elemtype = oid;
		result->dataoffset = 0;

		memcpy(ARR_DIMS(result), &nelems, sizeof(int));
		memcpy(ARR_LBOUND(result), &lbs, sizeof(int));
		memcpy(ARR_DATA_PTR(result), geom, VARSIZE(geom));

		POSTGIS_DEBUGF(3, " %d bytes memcopied", VARSIZE(geom));

	}
	else
	{
		oldsize = VARSIZE(array);
		nbytes = oldsize + INTALIGN(VARSIZE(geom));

		POSTGIS_DEBUGF(3, "geom_accum: old array size: %d, adding %ld bytes (nelems=%d; nbytes=%u)", ARR_SIZE(array), INTALIGN(VARSIZE(geom)), nelems, (int)nbytes);

		result = (ArrayType *) lwrealloc(array, nbytes);
		if ( ! result )
		{
			elog(ERROR, "Out of virtual memory");
			PG_RETURN_NULL();
		}

		POSTGIS_DEBUGF(3, " %d bytes allocated for array", (int)nbytes);

		POSTGIS_DEBUGF(3, " array start  @ %p", (void*)result);
		POSTGIS_DEBUGF(3, " ARR_DATA_PTR @ %p (%d)",
		               ARR_DATA_PTR(result), (uchar *)ARR_DATA_PTR(result)-(uchar *)result);
		POSTGIS_DEBUGF(3, " next element @ %p", (uchar *)result+oldsize);

		SET_VARSIZE(result, nbytes);
		memcpy(ARR_DIMS(result), &nelems, sizeof(int));

		POSTGIS_DEBUGF(3, " writing next element starting @ %p",
		               (void*)(result+oldsize));

		memcpy((uchar *)result+oldsize, geom, VARSIZE(geom));
	}

	POSTGIS_DEBUG(3, " returning");

	PG_RETURN_ARRAYTYPE_P(result);

}

/**
 * @brief collect_garray ( GEOMETRY[] ) returns a geometry which contains
 * 		all the sub_objects from all of the geometries in given array.
 *
 * @return geometry is the simplest possible, based on the types
 * 		of the collected objects
 * 		ie. if all are of either X or multiX, then a multiX is returned
 * 		bboxonly types are treated as null geometries (no sub_objects)
 */
PG_FUNCTION_INFO_V1(LWGEOM_collect_garray);
Datum LWGEOM_collect_garray(PG_FUNCTION_ARGS)
{
	Datum datum;
	ArrayType *array;
	int nelems;
	/*PG_LWGEOM **geoms; */
	PG_LWGEOM *result=NULL;
	LWGEOM **lwgeoms, *outlwg;
	uint32 outtype;
	int i, count;
	int srid=-1;
	size_t offset;
	GBOX *box=NULL;
	bits8 *bitmap;
	int bitmask;

	POSTGIS_DEBUG(2, "LWGEOM_collect_garray called.");

	/* Get input datum */
	datum = PG_GETARG_DATUM(0);

	/* Return null on null input */
	if ( (Pointer *)datum == NULL )
	{
		elog(NOTICE, "NULL input");
		PG_RETURN_NULL();
	}

	/* Get actual ArrayType */
	array = DatumGetArrayTypeP(datum);

	POSTGIS_DEBUGF(3, " array is %d-bytes in size, %ld w/out header",
	               ARR_SIZE(array), ARR_SIZE(array)-ARR_OVERHEAD_NONULLS(ARR_NDIM(array)));


	/* Get number of geometries in array */
	nelems = ArrayGetNItems(ARR_NDIM(array), ARR_DIMS(array));

	POSTGIS_DEBUGF(3, "LWGEOM_collect_garray: array has %d elements", nelems);

	/* Return null on 0-elements input array */
	if ( nelems == 0 )
	{
		elog(NOTICE, "0 elements input array");
		PG_RETURN_NULL();
	}

	/*
	 * Deserialize all geometries in array into the lwgeoms pointers
	 * array. Check input types to form output type.
	 */
	lwgeoms = palloc(sizeof(LWGEOM *)*nelems);
	count = 0;
	outtype = 0;
	offset = 0;
	bitmap = ARR_NULLBITMAP(array);
	bitmask = 1;
	for (i=0; i<nelems; i++)
	{
		/* Don't do anything for NULL values */
		if ((bitmap && (*bitmap & bitmask) != 0) || !bitmap)
		{
			PG_LWGEOM *geom = (PG_LWGEOM *)(ARR_DATA_PTR(array)+offset);
			uint32 intype = TYPE_GETTYPE(geom->type);

			offset += INTALIGN(VARSIZE(geom));

			lwgeoms[count] = lwgeom_deserialize(SERIALIZED_FORM(geom));

			POSTGIS_DEBUGF(3, "LWGEOM_collect_garray: geom %d deserialized", i);

			if ( ! count )
			{
				/* Get first geometry SRID */
				srid = lwgeoms[count]->srid;

				/* COMPUTE_BBOX WHEN_SIMPLE */
				if ( lwgeoms[count]->bbox )
				{
					box = gbox_copy(lwgeoms[count]->bbox);
				}
			}
			else
			{
				/* Check SRID homogeneity */
				if ( lwgeoms[count]->srid != srid )
				{
					elog(ERROR,
					     "Operation on mixed SRID geometries");
					PG_RETURN_NULL();
				}

				/* COMPUTE_BBOX WHEN_SIMPLE */
				if ( box )
				{
					if ( lwgeoms[count]->bbox )
					{
						box->xmin = LW_MIN(box->xmin, lwgeoms[count]->bbox->xmin);
						box->ymin = LW_MIN(box->ymin, lwgeoms[count]->bbox->ymin);
						box->xmax = LW_MAX(box->xmax, lwgeoms[count]->bbox->xmax);
						box->ymax = LW_MAX(box->ymax, lwgeoms[count]->bbox->ymax);
					}
					else
					{
						pfree(box);
						box = NULL;
					}
				}
			}

			lwgeom_drop_srid(lwgeoms[count]);
			lwgeom_drop_bbox(lwgeoms[count]);

			/* Output type not initialized */
			if ( ! outtype )
			{
				/* Input is single, make multi */
				if ( intype < 4 ) outtype = intype+3;
				/* Input is multi, make collection */
				else outtype = COLLECTIONTYPE;
			}

			/* Input type not compatible with output */
			/* make output type a collection */
			else if ( outtype != COLLECTIONTYPE && intype != outtype-3 )
			{
				outtype = COLLECTIONTYPE;
			}

			/* Advance NULL bitmap */
			if (bitmap)
			{
				bitmask <<= 1;
				if (bitmask == 0x100)
				{
					bitmap++;
					bitmask = 1;
				}
			}

			count++;
		}
	}

	POSTGIS_DEBUGF(3, "LWGEOM_collect_garray: outtype = %d", outtype);

	/* If we have been passed a complete set of NULLs then return NULL */
	if (!outtype)
	{
		PG_RETURN_NULL();
	}
	else
	{
		outlwg = (LWGEOM *)lwcollection_construct(
		             outtype, srid,
		             box, count, lwgeoms);

		result = pglwgeom_serialize(outlwg);

		PG_RETURN_POINTER(result);
	}
}

/**
 * LineFromMultiPoint ( GEOMETRY ) returns a LINE formed by
 * 		all the points in the in given multipoint.
 */
PG_FUNCTION_INFO_V1(LWGEOM_line_from_mpoint);
Datum LWGEOM_line_from_mpoint(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *ingeom, *result;
	LWLINE *lwline;
	LWMPOINT *mpoint;

	POSTGIS_DEBUG(2, "LWGEOM_line_from_mpoint called");

	/* Get input PG_LWGEOM and deserialize it */
	ingeom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	if ( TYPE_GETTYPE(ingeom->type) != MULTIPOINTTYPE )
	{
		elog(ERROR, "makeline: input must be a multipoint");
		PG_RETURN_NULL(); /* input is not a multipoint */
	}

	mpoint = lwmpoint_deserialize(SERIALIZED_FORM(ingeom));
	lwline = lwline_from_lwmpoint(mpoint->srid, mpoint);
	if ( ! lwline )
	{
		PG_FREE_IF_COPY(ingeom, 0);
		elog(ERROR, "makeline: lwline_from_lwmpoint returned NULL");
		PG_RETURN_NULL();
	}

	result = pglwgeom_serialize((LWGEOM *)lwline);

	PG_FREE_IF_COPY(ingeom, 0);
	lwgeom_release((LWGEOM *)lwline);

	PG_RETURN_POINTER(result);
}

/**
 * @brief makeline_garray ( GEOMETRY[] ) returns a LINE formed by
 * 		all the point geometries in given array.
 * 		array elements that are NOT points are discarded..
 */
PG_FUNCTION_INFO_V1(LWGEOM_makeline_garray);
Datum LWGEOM_makeline_garray(PG_FUNCTION_ARGS)
{
	Datum datum;
	ArrayType *array;
	int nelems;
	PG_LWGEOM *result=NULL;
	LWPOINT **lwpoints;
	LWGEOM *outlwg;
	uint32 npoints;
	int i;
	size_t offset;
	int srid=-1;

	bits8 *bitmap;
	int bitmask;

	POSTGIS_DEBUG(2, "LWGEOM_makeline_garray called.");

	/* Get input datum */
	datum = PG_GETARG_DATUM(0);

	/* Return null on null input */
	if ( (Pointer *)datum == NULL )
	{
		elog(NOTICE, "NULL input");
		PG_RETURN_NULL();
	}

	/* Get actual ArrayType */
	array = DatumGetArrayTypeP(datum);

	POSTGIS_DEBUG(3, "LWGEOM_makeline_garray: array detoasted");

	/* Get number of geometries in array */
	nelems = ArrayGetNItems(ARR_NDIM(array), ARR_DIMS(array));

	POSTGIS_DEBUGF(3, "LWGEOM_makeline_garray: array has %d elements", nelems);

	/* Return null on 0-elements input array */
	if ( nelems == 0 )
	{
		elog(NOTICE, "0 elements input array");
		PG_RETURN_NULL();
	}

	/*
	 * Deserialize all point geometries in array into the
	 * lwpoints pointers array.
	 * Count actual number of points.
	 */

	/* possibly more then required */
	lwpoints = palloc(sizeof(LWGEOM *)*nelems);
	npoints = 0;
	offset = 0;
	bitmap = ARR_NULLBITMAP(array);
	bitmask = 1;
	for (i=0; i<nelems; i++)
	{
		/* Don't do anything for NULL values */
		if ((bitmap && (*bitmap & bitmask) != 0) || !bitmap)
		{
			PG_LWGEOM *geom = (PG_LWGEOM *)(ARR_DATA_PTR(array)+offset);
			offset += INTALIGN(VARSIZE(geom));

			if ( TYPE_GETTYPE(geom->type) != POINTTYPE ) continue;

			lwpoints[npoints++] =
			    lwpoint_deserialize(SERIALIZED_FORM(geom));

			/* Check SRID homogeneity */
			if ( npoints == 1 )
			{
				/* Get first geometry SRID */
				srid = lwpoints[npoints-1]->srid;
			}
			else
			{
				if ( lwpoints[npoints-1]->srid != srid )
				{
					elog(ERROR,
					     "Operation on mixed SRID geometries");
					PG_RETURN_NULL();
				}
			}

			POSTGIS_DEBUGF(3, "LWGEOM_makeline_garray: element %d deserialized",
			               i);
		}

		/* Advance NULL bitmap */
		if (bitmap)
		{
			bitmask <<= 1;
			if (bitmask == 0x100)
			{
				bitmap++;
				bitmask = 1;
			}
		}
	}

	/* Return null on 0-points input array */
	if ( npoints == 0 )
	{
		elog(NOTICE, "No points in input array");
		PG_RETURN_NULL();
	}

	POSTGIS_DEBUGF(3, "LWGEOM_makeline_garray: point elements: %d", npoints);

	outlwg = (LWGEOM *)lwline_from_lwpointarray(srid, npoints, lwpoints);

	result = pglwgeom_serialize(outlwg);

	PG_RETURN_POINTER(result);
}

/**
 * makeline ( GEOMETRY, GEOMETRY ) returns a LINESTRIN segment
 * formed by the given point geometries.
 */
PG_FUNCTION_INFO_V1(LWGEOM_makeline);
Datum LWGEOM_makeline(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *pglwg1, *pglwg2;
	PG_LWGEOM *result=NULL;
	LWPOINT *lwpoints[2];
	LWLINE *outline;

	POSTGIS_DEBUG(2, "LWGEOM_makeline called.");

	/* Get input datum */
	pglwg1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	pglwg2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	if ( ! TYPE_GETTYPE(pglwg1->type) == POINTTYPE ||
	        ! TYPE_GETTYPE(pglwg2->type) == POINTTYPE )
	{
		elog(ERROR, "Input geometries must be points");
		PG_RETURN_NULL();
	}

	error_if_srid_mismatch(pglwgeom_get_srid(pglwg1), pglwgeom_get_srid(pglwg2));

	lwpoints[0] = lwpoint_deserialize(SERIALIZED_FORM(pglwg1));
	lwpoints[1] = lwpoint_deserialize(SERIALIZED_FORM(pglwg2));

	outline = lwline_from_lwpointarray(lwpoints[0]->srid, 2, lwpoints);

	result = pglwgeom_serialize((LWGEOM *)outline);

	PG_FREE_IF_COPY(pglwg1, 0);
	PG_FREE_IF_COPY(pglwg2, 1);
	lwgeom_release((LWGEOM *)lwpoints[0]);
	lwgeom_release((LWGEOM *)lwpoints[1]);

	PG_RETURN_POINTER(result);
}

/**
 * makepoly( GEOMETRY, GEOMETRY[] ) returns a POLYGON
 * 		formed by the given shell and holes geometries.
 */
PG_FUNCTION_INFO_V1(LWGEOM_makepoly);
Datum LWGEOM_makepoly(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *pglwg1;
	ArrayType *array=NULL;
	PG_LWGEOM *result=NULL;
	const LWLINE *shell=NULL;
	const LWLINE **holes=NULL;
	LWPOLY *outpoly;
	uint32 nholes=0;
	uint32 i;
	size_t offset=0;

	POSTGIS_DEBUG(2, "LWGEOM_makepoly called.");

	/* Get input shell */
	pglwg1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	if ( ! TYPE_GETTYPE(pglwg1->type) == LINETYPE )
	{
		lwerror("Shell is not a line");
	}
	shell = lwline_deserialize(SERIALIZED_FORM(pglwg1));

	/* Get input holes if any */
	if ( PG_NARGS() > 1 )
	{
		array = PG_GETARG_ARRAYTYPE_P(1);
		nholes = ArrayGetNItems(ARR_NDIM(array), ARR_DIMS(array));
		holes = lwalloc(sizeof(LWLINE *)*nholes);
		for (i=0; i<nholes; i++)
		{
			PG_LWGEOM *g = (PG_LWGEOM *)(ARR_DATA_PTR(array)+offset);
			LWLINE *hole;
			offset += INTALIGN(VARSIZE(g));
			if ( TYPE_GETTYPE(g->type) != LINETYPE )
			{
				lwerror("Hole %d is not a line", i);
			}
			hole = lwline_deserialize(SERIALIZED_FORM(g));
			holes[i] = hole;
		}
	}

	outpoly = lwpoly_from_lwlines(shell, nholes, holes);

	POSTGIS_DEBUGF(3, "%s", lwgeom_summary((LWGEOM*)outpoly, 0));

	result = pglwgeom_serialize((LWGEOM *)outpoly);

	PG_FREE_IF_COPY(pglwg1, 0);
	lwgeom_release((LWGEOM *)shell);
	for (i=0; i<nholes; i++) lwgeom_release((LWGEOM *)holes[i]);

	PG_RETURN_POINTER(result);
}

/**
 *  makes a polygon of the expanded features bvol - 1st point = LL 3rd=UR
 *  2d only. (3d might be worth adding).
 *  create new geometry of type polygon, 1 ring, 5 points
 */
PG_FUNCTION_INFO_V1(LWGEOM_expand);
Datum LWGEOM_expand(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	double d = PG_GETARG_FLOAT8(1);
	BOX3D box3d;
	POINT4D pt;
	POINTARRAY *pa = ptarray_construct_empty(0, 0, 5);
	POINTARRAY **ppa = lwalloc(sizeof(POINTARRAY*));
	LWPOLY *poly;
	int srid;
	PG_LWGEOM *result;

	POSTGIS_DEBUG(2, "LWGEOM_expand called.");

	/* get geometry box  */
	if ( ! compute_serialized_box3d_p(SERIALIZED_FORM(geom), &box3d) )
	{
		/* must be an EMPTY geometry */
		PG_RETURN_POINTER(geom);
	}

	/* get geometry SRID */
	srid = lwgeom_getsrid(SERIALIZED_FORM(geom));

	/* expand it */
	expand_box3d(&box3d, d);

	/* Assign coordinates to POINT2D array */
	pt.x = box3d.xmin;
	pt.y = box3d.ymin;
	ptarray_append_point(pa, &pt, REPEATED_POINTS_OK);
	pt.x = box3d.xmin;
	pt.y = box3d.ymax;
	ptarray_append_point(pa, &pt, REPEATED_POINTS_OK);
	pt.x = box3d.xmax;
	pt.y = box3d.ymax;
	ptarray_append_point(pa, &pt, REPEATED_POINTS_OK);
	pt.x = box3d.xmax;
	pt.y = box3d.ymin;
	ptarray_append_point(pa, &pt, REPEATED_POINTS_OK);
	pt.x = box3d.xmin;
	pt.y = box3d.ymin;
	ptarray_append_point(pa, &pt, REPEATED_POINTS_OK);

	/* Construct point array */
	ppa[0] = pa;

	/* Construct polygon  */
	poly = lwpoly_construct(srid, NULL, 1, ppa);
	lwgeom_add_bbox((LWGEOM *)poly);

	/* Construct PG_LWGEOM  */
	result = pglwgeom_serialize(lwpoly_as_lwgeom(poly));
	lwgeom_free(lwpoly_as_lwgeom(poly));

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result);
}

/** Convert geometry to BOX (internal postgres type) */
PG_FUNCTION_INFO_V1(LWGEOM_to_BOX);
Datum LWGEOM_to_BOX(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *pg_lwgeom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	BOX3D *box3d;
	BOX *result = (BOX *)lwalloc(sizeof(BOX));
	LWGEOM *lwgeom = lwgeom_deserialize(SERIALIZED_FORM(pg_lwgeom));

	/* Calculate the BOX3D of the geometry */
	box3d = lwgeom_compute_box3d(lwgeom);
	box3d_to_box_p(box3d, result);
	lwfree(box3d);
	lwfree(lwgeom);

	PG_FREE_IF_COPY(pg_lwgeom, 0);

	PG_RETURN_POINTER(result);
}

/**
 *  makes a polygon of the features bvol - 1st point = LL 3rd=UR
 *  2d only. (3d might be worth adding).
 *  create new geometry of type polygon, 1 ring, 5 points
 */
PG_FUNCTION_INFO_V1(LWGEOM_envelope);
Datum LWGEOM_envelope(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	BOX3D box;
	int srid;
	POINTARRAY *pa;
	PG_LWGEOM *result;
	uchar *ser = NULL;


	/* get bounding box  */
	if ( ! compute_serialized_box3d_p(SERIALIZED_FORM(geom), &box) )
	{
		/* must be the EMPTY geometry */
		PG_RETURN_POINTER(geom);
	}

	/* get geometry SRID */
	srid = lwgeom_getsrid(SERIALIZED_FORM(geom));


	/*
	 * Alter envelope type so that a valid geometry is always
	 * returned depending upon the size of the geometry. The
	 * code makes the following assumptions:
	 *     - If the bounding box is a single point then return a
	 *     POINT geometry
	 *     - If the bounding box represents either a horizontal or
	 *     vertical line, return a LINESTRING geometry
	 *     - Otherwise return a POLYGON
	 */


	if (box.xmin == box.xmax &&
	        box.ymin == box.ymax)
	{
		/* Construct and serialize point */
		LWPOINT *point = make_lwpoint2d(srid, box.xmin, box.ymin);
		ser = lwpoint_serialize(point);
	}
	else if (box.xmin == box.xmax ||
	         box.ymin == box.ymax)
	{
		LWLINE *line;
		POINT2D *pts = palloc(sizeof(POINT2D)*2);

		/* Assign coordinates to POINT2D array */
		pts[0].x = box.xmin;
		pts[0].y = box.ymin;
		pts[1].x = box.xmax;
		pts[1].y = box.ymax;

		/* Construct point array */
		pa = ptarray_construct_reference_data(0, 0, 2, (uchar*)pts);

		/* Construct and serialize linestring */
		line = lwline_construct(srid, NULL, pa);
		ser = lwline_serialize(line);
	}
	else
	{
		LWPOLY *poly;
		POINT2D *pts = lwalloc(sizeof(POINT2D)*5);

		/* Assign coordinates to POINT2D array */
		pts[0].x = box.xmin;
		pts[0].y = box.ymin;
		pts[1].x = box.xmin;
		pts[1].y = box.ymax;
		pts[2].x = box.xmax;
		pts[2].y = box.ymax;
		pts[3].x = box.xmax;
		pts[3].y = box.ymin;
		pts[4].x = box.xmin;
		pts[4].y = box.ymin;

		/* Construct point array */
		pa = ptarray_construct_reference_data(0, 0, 5, (uchar*)pts);

		/* Construct polygon  */
		poly = lwpoly_construct(srid, NULL, 1, &pa);
		lwgeom_add_bbox((LWGEOM *)poly);

		/* Serialize polygon */
		ser = lwpoly_serialize(poly);
	}

	PG_FREE_IF_COPY(geom, 0);

	/* Construct PG_LWGEOM  */
	result = PG_LWGEOM_construct(ser, srid, 1);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(LWGEOM_isempty);
Datum LWGEOM_isempty(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	if ( lwgeom_getnumgeometries(SERIALIZED_FORM(geom)) == 0 )
	{
		PG_FREE_IF_COPY(geom, 0);
		PG_RETURN_BOOL(TRUE);
	}
	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_BOOL(FALSE);
}


/**
 *  @brief Returns a modified geometry so that no segment is
 *  	longer then the given distance (computed using 2d).
 *  	Every input point is kept.
 *  	Z and M values for added points (if needed) are set to 0.
 */
PG_FUNCTION_INFO_V1(LWGEOM_segmentize2d);
Datum LWGEOM_segmentize2d(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *outgeom, *ingeom;
	double dist;
	LWGEOM *inlwgeom, *outlwgeom;

	POSTGIS_DEBUG(2, "LWGEOM_segmentize2d called");

	ingeom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	dist = PG_GETARG_FLOAT8(1);

	/* Avoid deserialize/serialize steps */
	if ( (TYPE_GETTYPE(ingeom->type) == POINTTYPE) ||
	        (TYPE_GETTYPE(ingeom->type) == MULTIPOINTTYPE) )
		PG_RETURN_POINTER(ingeom);

	inlwgeom = lwgeom_deserialize(SERIALIZED_FORM(ingeom));
	outlwgeom = lwgeom_segmentize2d(inlwgeom, dist);

	/* Copy input bounding box if any */
	if ( inlwgeom->bbox )
		outlwgeom->bbox = gbox_copy(inlwgeom->bbox);

	outgeom = pglwgeom_serialize(outlwgeom);

	PG_FREE_IF_COPY(ingeom, 0);
	lwgeom_release(outlwgeom);
	lwgeom_release(inlwgeom);

	PG_RETURN_POINTER(outgeom);
}

/** Reverse vertex order of geometry */
PG_FUNCTION_INFO_V1(LWGEOM_reverse);
Datum LWGEOM_reverse(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	LWGEOM *lwgeom;

	POSTGIS_DEBUG(2, "LWGEOM_reverse called");

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));

	lwgeom = lwgeom_deserialize(SERIALIZED_FORM(geom));
	lwgeom_reverse(lwgeom);

	geom = pglwgeom_serialize(lwgeom);

	PG_RETURN_POINTER(geom);
}

/** Force polygons of the collection to obey Right-Hand-Rule */
PG_FUNCTION_INFO_V1(LWGEOM_force_clockwise_poly);
Datum LWGEOM_force_clockwise_poly(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *ingeom, *outgeom;
	LWGEOM *lwgeom;

	POSTGIS_DEBUG(2, "LWGEOM_force_clockwise_poly called");

	ingeom = (PG_LWGEOM *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));

	lwgeom = lwgeom_deserialize(SERIALIZED_FORM(ingeom));
	lwgeom_force_clockwise(lwgeom);

	outgeom = pglwgeom_serialize(lwgeom);

	PG_FREE_IF_COPY(ingeom, 0);
	lwgeom_release(lwgeom);

	PG_RETURN_POINTER(outgeom);
}

/** Test deserialize/serialize operations */
PG_FUNCTION_INFO_V1(LWGEOM_noop);
Datum LWGEOM_noop(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *in, *out;
	LWGEOM *lwgeom;

	POSTGIS_DEBUG(2, "LWGEOM_noop called");

	in = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	lwgeom = lwgeom_deserialize(SERIALIZED_FORM(in));

	POSTGIS_DEBUGF(3, "Deserialized: %s", lwgeom_summary(lwgeom, 0));

	out = pglwgeom_serialize(lwgeom);

	PG_FREE_IF_COPY(in, 0);
	lwgeom_release(lwgeom);

	PG_RETURN_POINTER(out);
}

/**
 *  @return:
 *   0==2d
 *   1==3dm
 *   2==3dz
 *   3==4d
 */
PG_FUNCTION_INFO_V1(LWGEOM_zmflag);
Datum LWGEOM_zmflag(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *in;
	uchar type;
	int ret = 0;

	in = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	type = in->type;
	if ( TYPE_HASZ(type) ) ret += 2;
	if ( TYPE_HASM(type) ) ret += 1;
	PG_FREE_IF_COPY(in, 0);
	PG_RETURN_INT16(ret);
}

PG_FUNCTION_INFO_V1(LWGEOM_hasz);
Datum LWGEOM_hasz(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *in = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_RETURN_BOOL(TYPE_HASZ(in->type));
}

PG_FUNCTION_INFO_V1(LWGEOM_hasm);
Datum LWGEOM_hasm(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *in = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_RETURN_BOOL(TYPE_HASM(in->type));
}


PG_FUNCTION_INFO_V1(LWGEOM_hasBBOX);
Datum LWGEOM_hasBBOX(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *in;
	char res;

	in = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	res=lwgeom_hasBBOX(in->type);
	PG_FREE_IF_COPY(in, 0);

	PG_RETURN_BOOL(res);
}

/** Return: 2,3 or 4 */
PG_FUNCTION_INFO_V1(LWGEOM_ndims);
Datum LWGEOM_ndims(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *in;
	int ret;

	in = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	ret = (TYPE_NDIMS(in->type));
	PG_FREE_IF_COPY(in, 0);
	PG_RETURN_INT16(ret);
}

/** lwgeom_same(lwgeom1, lwgeom2) */
PG_FUNCTION_INFO_V1(LWGEOM_same);
Datum LWGEOM_same(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *g1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *g2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	LWGEOM *lwg1, *lwg2;
	bool result;

	if ( TYPE_GETTYPE(g1->type) != TYPE_GETTYPE(g2->type) )
	{
		PG_FREE_IF_COPY(g1, 0);
		PG_FREE_IF_COPY(g2, 1);
		PG_RETURN_BOOL(FALSE); /* different types */
	}

	if ( TYPE_GETZM(g1->type) != TYPE_GETZM(g2->type) )
	{
		PG_FREE_IF_COPY(g1, 0);
		PG_FREE_IF_COPY(g2, 1);
		PG_RETURN_BOOL(FALSE); /* different dimensions */
	}

	/* ok, deserialize. */
	lwg1 = lwgeom_deserialize(SERIALIZED_FORM(g1));
	lwg2 = lwgeom_deserialize(SERIALIZED_FORM(g2));

	/* invoke appropriate function */
	result = lwgeom_same(lwg1, lwg2);

	/* Relase memory */
	lwgeom_release(lwg1);
	lwgeom_release(lwg2);
	PG_FREE_IF_COPY(g1, 0);
	PG_FREE_IF_COPY(g2, 1);

	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(ST_MakeEnvelope);
Datum ST_MakeEnvelope(PG_FUNCTION_ARGS)
{
	LWPOLY *poly;
	PG_LWGEOM *result;
	POINTARRAY **pa;
	double *pts;
	double x1, y1, x2, y2;
	int srid;

	POSTGIS_DEBUG(2, "ST_MakeEnvelope called");

	x1 = PG_GETARG_FLOAT8(0);
	y1 = PG_GETARG_FLOAT8(1);
	x2 = PG_GETARG_FLOAT8(2);
	y2 = PG_GETARG_FLOAT8(3);
	srid = PG_GETARG_INT32(4);

	pa = (POINTARRAY**)palloc(sizeof(POINTARRAY*));
	pa[0] = ptarray_construct(0, 0, 5);
	pts = (double*)(pa[0]->serialized_pointlist);

	/* 1st point */
	pts[0] = x1;
	pts[1] = y1;

	/* 2nd point */
	pts[2] = x1;
	pts[3] = y2;

	/* 3rd point */
	pts[4] = x2;
	pts[5] = y2;

	/* 4th point */
	pts[6] = x2;
	pts[7] = y1;

	/* 5th point */
	pts[8] = x1;
	pts[9] = y1;

	poly = lwpoly_construct(srid, NULL, 1, pa);
	lwgeom_add_bbox((LWGEOM *)poly);

	result = pglwgeom_serialize((LWGEOM*)poly);
	lwpoly_free(poly);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(ST_IsCollection);
Datum ST_IsCollection(PG_FUNCTION_ARGS)
{
	PG_LWGEOM* geom;
	int type;

	/* Pull only a small amount of the tuple,
	* enough to get the type. size = header + type */
	geom = (PG_LWGEOM*)PG_DETOAST_DATUM_SLICE(PG_GETARG_DATUM(0),
	        0, VARHDRSZ + 1);

	type = lwgeom_getType(SERIALIZED_FORM(geom)[0]);
	PG_RETURN_BOOL(lwtype_is_collection(type));
}

PG_FUNCTION_INFO_V1(LWGEOM_makepoint);
Datum LWGEOM_makepoint(PG_FUNCTION_ARGS)
{
	double x,y,z,m;
	LWPOINT *point;
	PG_LWGEOM *result;

	POSTGIS_DEBUG(2, "LWGEOM_makepoint called");

	x = PG_GETARG_FLOAT8(0);
	y = PG_GETARG_FLOAT8(1);

	if ( PG_NARGS() == 2 ) point = make_lwpoint2d(-1, x, y);
	else if ( PG_NARGS() == 3 )
	{
		z = PG_GETARG_FLOAT8(2);
		point = make_lwpoint3dz(-1, x, y, z);
	}
	else if ( PG_NARGS() == 4 )
	{
		z = PG_GETARG_FLOAT8(2);
		m = PG_GETARG_FLOAT8(3);
		point = make_lwpoint4d(-1, x, y, z, m);
	}
	else
	{
		elog(ERROR, "LWGEOM_makepoint: unsupported number of args: %d",
		     PG_NARGS());
		PG_RETURN_NULL();
	}

	result = pglwgeom_serialize((LWGEOM *)point);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(LWGEOM_makepoint3dm);
Datum LWGEOM_makepoint3dm(PG_FUNCTION_ARGS)
{
	double x,y,m;
	LWPOINT *point;
	PG_LWGEOM *result;

	POSTGIS_DEBUG(2, "LWGEOM_makepoint3dm called.");

	x = PG_GETARG_FLOAT8(0);
	y = PG_GETARG_FLOAT8(1);
	m = PG_GETARG_FLOAT8(2);

	point = make_lwpoint3dm(-1, x, y, m);
	result = pglwgeom_serialize((LWGEOM *)point);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(LWGEOM_addpoint);
Datum LWGEOM_addpoint(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *pglwg1, *pglwg2, *result;
	LWPOINT *point;
	LWLINE *line;
	int where = -1;

	POSTGIS_DEBUG(2, "LWGEOM_addpoint called.");

	pglwg1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	pglwg2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	if ( PG_NARGS() > 2 )
	{
		where = PG_GETARG_INT32(2);
	}

	if ( ! TYPE_GETTYPE(pglwg1->type) == LINETYPE )
	{
		elog(ERROR, "First argument must be a LINESTRING");
		PG_RETURN_NULL();
	}

	if ( ! TYPE_GETTYPE(pglwg2->type) == POINTTYPE )
	{
		elog(ERROR, "Second argument must be a POINT");
		PG_RETURN_NULL();
	}

	line = lwline_deserialize(SERIALIZED_FORM(pglwg1));

	if ( where == -1 ) where = line->points->npoints;
	else if ( where < 0 || where > line->points->npoints )
	{
		elog(ERROR, "Invalid offset");
		PG_RETURN_NULL();
	}

	point = lwpoint_deserialize(SERIALIZED_FORM(pglwg2));

	if ( lwline_add_point(line, point, where) == LW_FAILURE )
	{
		elog(ERROR, "Point insert failed");
		PG_RETURN_NULL();
	}

	result = pglwgeom_serialize(lwline_as_lwgeom(line));

	/* Release memory */
	PG_FREE_IF_COPY(pglwg1, 0);
	PG_FREE_IF_COPY(pglwg2, 1);
	lwgeom_release((LWGEOM *)point);
	lwgeom_release((LWGEOM *)line);

	PG_RETURN_POINTER(result);

}

PG_FUNCTION_INFO_V1(LWGEOM_removepoint);
Datum LWGEOM_removepoint(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *pglwg1, *result;
	LWLINE *line, *outline;
	uint32 which;

	POSTGIS_DEBUG(2, "LWGEOM_removepoint called.");

	pglwg1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	which = PG_GETARG_INT32(1);

	if ( ! TYPE_GETTYPE(pglwg1->type) == LINETYPE )
	{
		elog(ERROR, "First argument must be a LINESTRING");
		PG_RETURN_NULL();
	}

	line = lwline_deserialize(SERIALIZED_FORM(pglwg1));

	if ( which > line->points->npoints-1 )
	{
		elog(ERROR, "Point index out of range (%d..%d)", 0, line->points->npoints-1);
		PG_RETURN_NULL();
	}

	if ( line->points->npoints < 3 )
	{
		elog(ERROR, "Can't remove points from a single segment line");
		PG_RETURN_NULL();
	}

	outline = lwline_removepoint(line, which);

	result = pglwgeom_serialize((LWGEOM *)outline);

	/* Release memory */
	PG_FREE_IF_COPY(pglwg1, 0);
	lwgeom_release((LWGEOM *)line);
	lwgeom_release((LWGEOM *)outline);

	PG_RETURN_POINTER(result);

}

PG_FUNCTION_INFO_V1(LWGEOM_setpoint_linestring);
Datum LWGEOM_setpoint_linestring(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *pglwg1, *pglwg2, *result;
	LWGEOM *lwg;
	LWLINE *line;
	LWPOINT *lwpoint;
	POINT4D newpoint;
	uint32 which;

	POSTGIS_DEBUG(2, "LWGEOM_setpoint_linestring called.");

	/* we copy input as we're going to modify it */
	pglwg1 = (PG_LWGEOM *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));

	which = PG_GETARG_INT32(1);
	pglwg2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(2));


	/* Extract a POINT4D from the point */
	lwg = pglwgeom_deserialize(pglwg2);
	lwpoint = lwgeom_as_lwpoint(lwg);
	if ( ! lwpoint )
	{
		elog(ERROR, "Third argument must be a POINT");
		PG_RETURN_NULL();
	}
	getPoint4d_p(lwpoint->point, 0, &newpoint);
	lwgeom_release((LWGEOM *)lwpoint);
	PG_FREE_IF_COPY(pglwg2, 2);

	lwg = pglwgeom_deserialize(pglwg1);
	line = lwgeom_as_lwline(lwg);
	if ( ! line )
	{
		elog(ERROR, "First argument must be a LINESTRING");
		PG_RETURN_NULL();
	}
	if ( which > line->points->npoints-1 )
	{
		elog(ERROR, "Point index out of range (%d..%d)", 0, line->points->npoints-1);
		PG_RETURN_NULL();
	}

	/*
	 * This will change pointarray of the serialized pglwg1,
	 */
	lwline_setPoint4d(line, which, &newpoint);
	result = pglwgeom_serialize((LWGEOM *)line);

	/* Release memory */
	pfree(pglwg1); /* we forced copy, POINARRAY is released now */
	lwgeom_release((LWGEOM *)line);

	PG_RETURN_POINTER(result);

}

/* convert LWGEOM to ewkt (in TEXT format) */
PG_FUNCTION_INFO_V1(LWGEOM_asEWKT);
Datum LWGEOM_asEWKT(PG_FUNCTION_ARGS)
{
	LWGEOM_UNPARSER_RESULT lwg_unparser_result;
	PG_LWGEOM *lwgeom;
	int len, result;
	char *lwgeom_result,*loc_wkt;
	/*char *semicolonLoc; */

	lwgeom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	result = serialized_lwgeom_to_ewkt(&lwg_unparser_result, SERIALIZED_FORM(lwgeom), PARSER_CHECK_ALL);
	if (result)
		PG_UNPARSER_ERROR(lwg_unparser_result);

#if 0
	semicolonLoc = strchr(lwg_unparser_result.wkb,';');

	/*loc points to start of wkt */
	if (semicolonLoc == NULL) loc_wkt = lwg_unparser_result.wkoutput;
	else loc_wkt = semicolonLoc +1;
#endif
	loc_wkt = lwg_unparser_result.wkoutput;

	len = strlen(loc_wkt);
	lwgeom_result = palloc(len + VARHDRSZ);
	SET_VARSIZE(lwgeom_result, len + VARHDRSZ);

	memcpy(VARDATA(lwgeom_result), loc_wkt, len);

	pfree(lwg_unparser_result.wkoutput);
	PG_FREE_IF_COPY(lwgeom, 0);

	PG_RETURN_POINTER(lwgeom_result);
}

/**
 * Compute the azimuth of segment defined by the two
 * given Point geometries.
 * @return NULL on exception (same point).
 * 		Return radians otherwise.
 */
PG_FUNCTION_INFO_V1(LWGEOM_azimuth);
Datum LWGEOM_azimuth(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	LWPOINT *lwpoint;
	POINT2D p1, p2;
	double result;
	int srid;

	/* Extract first point */
	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	lwpoint = lwpoint_deserialize(SERIALIZED_FORM(geom));
	if ( ! lwpoint )
	{
		PG_FREE_IF_COPY(geom, 0);
		lwerror("Argument must be POINT geometries");
		PG_RETURN_NULL();
	}
	srid = lwpoint->srid;
	if ( ! getPoint2d_p(lwpoint->point, 0, &p1) )
	{
		PG_FREE_IF_COPY(geom, 0);
		lwerror("Error extracting point");
		PG_RETURN_NULL();
	}
	lwgeom_release((LWGEOM *)lwpoint);
	PG_FREE_IF_COPY(geom, 0);

	/* Extract second point */
	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	lwpoint = lwpoint_deserialize(SERIALIZED_FORM(geom));
	if ( ! lwpoint )
	{
		PG_FREE_IF_COPY(geom, 1);
		lwerror("Argument must be POINT geometries");
		PG_RETURN_NULL();
	}
	if ( lwpoint->srid != srid )
	{
		PG_FREE_IF_COPY(geom, 1);
		lwerror("Operation on mixed SRID geometries");
		PG_RETURN_NULL();
	}
	if ( ! getPoint2d_p(lwpoint->point, 0, &p2) )
	{
		PG_FREE_IF_COPY(geom, 1);
		lwerror("Error extracting point");
		PG_RETURN_NULL();
	}
	lwgeom_release((LWGEOM *)lwpoint);
	PG_FREE_IF_COPY(geom, 1);

	/* Compute azimuth */
	if ( ! azimuth_pt_pt(&p1, &p2, &result) )
	{
		PG_RETURN_NULL();
	}

	PG_RETURN_FLOAT8(result);
}





/*
 * optimistic_overlap(Polygon P1, Multipolygon MP2, double dist)
 * returns true if P1 overlaps MP2
 *   method: bbox check -
 *   is separation < dist?  no - return false (quick)
 *                          yes  - return distance(P1,MP2) < dist
 */
PG_FUNCTION_INFO_V1(optimistic_overlap);
Datum optimistic_overlap(PG_FUNCTION_ARGS)
{

	PG_LWGEOM                  *pg_geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM                  *pg_geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	double                  dist = PG_GETARG_FLOAT8(2);
	BOX2DFLOAT4             g1_bvol;
	double                  calc_dist;

	LWGEOM                     *geom1;
	LWGEOM                     *geom2;


	/* deserialized PG_LEGEOM into their respective LWGEOM */
	geom1 = lwgeom_deserialize(SERIALIZED_FORM(pg_geom1));
	geom2 = lwgeom_deserialize(SERIALIZED_FORM(pg_geom2));

	if (geom1->srid != geom2->srid)
	{

		elog(ERROR,"optimistic_overlap:Operation on two GEOMETRIES with different SRIDs\\n");
		PG_RETURN_NULL();
	}

	if (geom1->type != POLYGONTYPE)
	{
		elog(ERROR,"optimistic_overlap: first arg isnt a polygon\n");
		PG_RETURN_NULL();
	}

	if (geom2->type != POLYGONTYPE &&  geom2->type != MULTIPOLYGONTYPE)
	{
		elog(ERROR,"optimistic_overlap: 2nd arg isnt a [multi-]polygon\n");
		PG_RETURN_NULL();
	}

	/*bbox check */

	getbox2d_p( SERIALIZED_FORM(pg_geom1), &g1_bvol );


	g1_bvol.xmin = g1_bvol.xmin - dist;
	g1_bvol.ymin = g1_bvol.ymin - dist;
	g1_bvol.xmax = g1_bvol.xmax + dist;
	g1_bvol.ymax = g1_bvol.ymax + dist;

	if (  (g1_bvol.xmin > geom2->bbox->xmax) ||
	        (g1_bvol.xmax < geom2->bbox->xmin) ||
	        (g1_bvol.ymin > geom2->bbox->ymax) ||
	        (g1_bvol.ymax < geom2->bbox->ymin)
	   )
	{
		PG_RETURN_BOOL(FALSE);  /*bbox not overlap */
	}

	/*
	 * compute distances
	 * should be a fast calc if they actually do intersect
	 */
	calc_dist =     DatumGetFloat8 ( DirectFunctionCall2(LWGEOM_mindistance2d,   PointerGetDatum( pg_geom1 ),       PointerGetDatum( pg_geom2 )));

	PG_RETURN_BOOL(calc_dist < dist);
}


/*affine transform geometry */
PG_FUNCTION_INFO_V1(LWGEOM_affine);
Datum LWGEOM_affine(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));
	LWGEOM *lwgeom = pglwgeom_deserialize(geom);
	PG_LWGEOM *ret;
	AFFINE affine;

	affine.afac =  PG_GETARG_FLOAT8(1);
	affine.bfac =  PG_GETARG_FLOAT8(2);
	affine.cfac =  PG_GETARG_FLOAT8(3);
	affine.dfac =  PG_GETARG_FLOAT8(4);
	affine.efac =  PG_GETARG_FLOAT8(5);
	affine.ffac =  PG_GETARG_FLOAT8(6);
	affine.gfac =  PG_GETARG_FLOAT8(7);
	affine.hfac =  PG_GETARG_FLOAT8(8);
	affine.ifac =  PG_GETARG_FLOAT8(9);
	affine.xoff =  PG_GETARG_FLOAT8(10);
	affine.yoff =  PG_GETARG_FLOAT8(11);
	affine.zoff =  PG_GETARG_FLOAT8(12);

	POSTGIS_DEBUG(2, "LWGEOM_affine called.");

	lwgeom_affine(lwgeom, &affine);

	/* COMPUTE_BBOX TAINTING */
	lwgeom_drop_bbox(lwgeom);
	lwgeom_add_bbox(lwgeom);
	ret = pglwgeom_serialize(lwgeom);

	/* Release memory */
	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(ret);
}

PG_FUNCTION_INFO_V1(ST_GeoHash);
Datum ST_GeoHash(PG_FUNCTION_ARGS)
{

	PG_LWGEOM *geom = NULL;
	int precision = 0;
	int len = 0;
	char *geohash = NULL;
	char *result = NULL;

	if ( PG_ARGISNULL(0) )
	{
		PG_RETURN_NULL();
	}

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	if ( ! PG_ARGISNULL(1) )
	{
		precision = PG_GETARG_INT32(1);
	}

	geohash = lwgeom_geohash((LWGEOM*)(pglwgeom_deserialize(geom)), precision);

	if ( ! geohash )
	{
		elog(ERROR,"ST_GeoHash: lwgeom_geohash returned NULL.\n");
		PG_RETURN_NULL();
	}

	len = strlen(geohash) + VARHDRSZ;
	result = palloc(len);
	SET_VARSIZE(result, len);
	memcpy(VARDATA(result), geohash, len-VARHDRSZ);
	pfree(geohash);
	PG_RETURN_POINTER(result);

}

PG_FUNCTION_INFO_V1(ST_CollectionExtract);
Datum ST_CollectionExtract(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *input = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *output;
	LWGEOM *lwgeom = pglwgeom_deserialize(input);
	LWCOLLECTION *lwcol = NULL;
	int type = PG_GETARG_INT32(1);

	/* Ensure the right type was input */
	if ( ! ( type == POINTTYPE || type == LINETYPE || type == POLYGONTYPE ) )
	{
		lwgeom_free(lwgeom);
		elog(ERROR, "ST_CollectionExtract: only point, linestring and polygon may be extracted");
		PG_RETURN_NULL();
	}

	/* Mirror non-collections right back */
	if ( ! lwtype_is_collection(lwgeom->type) )
	{
		output = palloc(VARSIZE(input));
		memcpy(VARDATA(output), VARDATA(input), VARSIZE(input) - VARHDRSZ);
		SET_VARSIZE(output, VARSIZE(input));
		lwgeom_free(lwgeom);
		PG_RETURN_POINTER(output);
	}

	lwcol = lwcollection_extract((LWCOLLECTION*)lwgeom, type);
	output = pglwgeom_serialize((LWGEOM*)lwcol);
	lwgeom_free(lwgeom);

	PG_RETURN_POINTER(output);
}

Datum ST_RemoveRepeatedPoints(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_RemoveRepeatedPoints);
Datum ST_RemoveRepeatedPoints(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *input = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *output;
	LWGEOM *lwgeom_in = pglwgeom_deserialize(input);
	LWGEOM *lwgeom_out;

	/* lwnotice("ST_RemoveRepeatedPoints got %p", lwgeom_in); */

	lwgeom_out = lwgeom_remove_repeated_points(lwgeom_in);
	output = pglwgeom_serialize(lwgeom_out);

	lwgeom_free(lwgeom_in);
	PG_FREE_IF_COPY(input, 0);

	PG_RETURN_POINTER(output);
}

Datum ST_FlipCoordinates(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_FlipCoordinates);
Datum ST_FlipCoordinates(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *input = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *output;
	LWGEOM *lwgeom_in = pglwgeom_deserialize(input);
	LWGEOM *lwgeom_out;

	lwgeom_out = lwgeom_flip_coordinates(lwgeom_in);
	output = pglwgeom_serialize(lwgeom_out);

	lwgeom_free(lwgeom_in);
	PG_FREE_IF_COPY(input, 0);

	PG_RETURN_POINTER(output);
}
