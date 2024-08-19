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
 * Copyright 2009-2014 Sandro Santilli <strk@kbt.io>
 * Copyright 2008 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright 2001-2003 Refractions Research Inc.
 *
 **********************************************************************/


#include "../postgis_config.h"

/* PostgreSQL */
#include "postgres.h"
#include "funcapi.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/numeric.h"
#include "access/htup_details.h"

/* PostGIS */
#include "lwgeom_geos.h"
#include "liblwgeom.h"
#include "liblwgeom_internal.h"
#include "lwgeom_itree.h"
#include "lwgeom_geos_prepared.h"


/*
** Prototypes for SQL-bound functions
*/
Datum relate_full(PG_FUNCTION_ARGS);
Datum relate_pattern(PG_FUNCTION_ARGS);

Datum disjoint(PG_FUNCTION_ARGS);
Datum ST_Intersects(PG_FUNCTION_ARGS);
Datum crosses(PG_FUNCTION_ARGS);
Datum touches(PG_FUNCTION_ARGS);
Datum overlaps(PG_FUNCTION_ARGS);
Datum ST_Equals(PG_FUNCTION_ARGS);

Datum within(PG_FUNCTION_ARGS);
Datum contains(PG_FUNCTION_ARGS);
Datum containsproperly(PG_FUNCTION_ARGS);
Datum coveredby(PG_FUNCTION_ARGS);
Datum covers(PG_FUNCTION_ARGS);


/*
** Prototypes end
*/

static char
is_poly(const GSERIALIZED* g)
{
    int type = gserialized_get_type(g);
    return type == POLYGONTYPE || type == MULTIPOLYGONTYPE;
}

static char
is_point(const GSERIALIZED* g)
{
	int type = gserialized_get_type(g);
	return type == POINTTYPE || type == MULTIPOINTTYPE;
}

// 2-sided
// intersects
// disjoint
// overlaps
// touches
// crosses
// equals

// 1-sided
// contains
// within
// covers
// coveredby
// containsproperly

typedef char (*funcGeosPredicate)(const GEOSGeometry *g1, const GEOSGeometry *g2);
typedef char (*funcGeosPreparedPredicate)(const GEOSPreparedGeometry *pg1, const GEOSGeometry *g2);
typedef int  (*funcBboxInteract)(const GBOX *b1, const GBOX *b2);
typedef bool (*funcITreeCompare)(const IntervalTree *itree, const LWGEOM *lwpt);


static inline bool
predicatePipCacheable(int cacheSide, const GSERIALIZED *geom1, const GSERIALIZED *geom2)
{
	bool is_point1 = is_point(geom1);
	bool is_point2 = is_point(geom2);
	bool is_poly1 = is_poly(geom1);
	bool is_poly2 = is_poly(geom2);

	if (cacheSide == 0 && (is_poly1 && is_point2) || (is_poly2 && is_point1))
		return true;
	else if (cacheSide == 1 && is_poly1 && is_point2)
		return true;
	else if (cacheSide == 2 && is_poly2 && is_point1)
		return true;
	else
		return false;
}


static Datum
GeosPredicate(
	FunctionCallInfo fcinfo,  // PG_FUNCTION_ARGS
	const char *funcName,     // SQL function call name
	int cacheSide,            // 0 for both (intersects), 1 for contains, 2 for within
	bool emptyTrue,           // true if an empty geometry implies true result (disjoint only)
	funcGeosPredicate geosPredicate,
	funcGeosPreparedPredicate geosPreparedPredicate,
	funcBboxInteract bboxInteract,
	funcITreeCompare itreeCompare)
{
	SHARED_GSERIALIZED *shared_geom1 = ToastCacheGetGeometry(fcinfo, 0);
	SHARED_GSERIALIZED *shared_geom2 = ToastCacheGetGeometry(fcinfo, 1);
	const GSERIALIZED *geom1 = shared_gserialized_get(shared_geom1);
	const GSERIALIZED *geom2 = shared_gserialized_get(shared_geom2);
	int result;
	GBOX box1, box2;
	PrepGeomCache *prep_cache;

	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	/* A.Predicate(Empty) == FALSE, except for Disjoint */
	if ( gserialized_is_empty(geom1) || gserialized_is_empty(geom2) )
		PG_RETURN_BOOL(emptyTrue);

	/*
	 * short-circuit 1: if geom2 bounding box does not overlap
	 * geom1 bounding box we can return FALSE.
	 */
	if ( gserialized_get_gbox_p(geom1, &box1) &&
	     gserialized_get_gbox_p(geom2, &box2) )
	{
		if ( ! bboxInteract(&box1, &box2) )
			PG_RETURN_BOOL(false);
	}

	/*
	 * short-circuit 2: if the geoms are a point and a polygon,
	 * call the itree_pip_intersects function.
	 */
	if ( predicatePipCacheable(cacheSide, geom1, geom2) )
	{
		bool is_poly1 = is_poly(geom1);
		SHARED_GSERIALIZED *shared_gply = is_poly1 ? shared_geom1 : shared_geom2;
		SHARED_GSERIALIZED *shared_gpnt = is_poly1 ? shared_geom2 : shared_geom1;
		const GSERIALIZED *gpnt = shared_gserialized_get(shared_gpnt);
		LWGEOM *lwpt = lwgeom_from_gserialized(gpnt);
		IntervalTree *itree = GetIntervalTree(fcinfo, shared_gply);
		bool result = funcITreeCompare(itree, lwpt);
		lwgeom_free(lwpt);
		PG_RETURN_BOOL(result);
	}

	initGEOS(lwpgnotice, lwgeom_geos_error);
	prep_cache = GetPrepGeomCache(fcinfo, shared_geom1, shared_geom2);

	if ( prep_cache && prep_cache->prepared_geom )
	{
		GEOSGeometry *g;
		if ( prep_cache->gcache.argnum == 1 )
			g = POSTGIS2GEOS(geom2);
		else
			g = POSTGIS2GEOS(geom1);

		if (!g) HANDLE_GEOS_ERROR("Geometry could not be converted to GEOS");
		result = geosPreparedPredicate( prep_cache->prepared_geom, g);
		GEOSGeom_destroy(g);
	}
	else
	{
		GEOSGeometry *g1, *g2;
		g1 = POSTGIS2GEOS(geom1);
		if (!g1) HANDLE_GEOS_ERROR("First argument geometry could not be converted to GEOS");
		g2 = POSTGIS2GEOS(geom2);
		if (!g2)
		{
			GEOSGeom_destroy(g1);
			HANDLE_GEOS_ERROR("Second argument geometry could not be converted to GEOS");
		}
		result = geosPredicate(g1, g2);
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
	}

	if (result == 2) HANDLE_GEOS_ERROR(funcName);

	PG_RETURN_BOOL(result);
}

// 2-sided
// intersects
// disjoint
// overlaps
// touches
// crosses
// equals

// 1-sided
// contains
// within
// covers
// coveredby
// containsproperly

PG_FUNCTION_INFO_V1(overlaps);
Datum overlaps(PG_FUNCTION_ARGS)
{
	return GeosPredicate(
		fcinfo,              // FunctionCallInfo
		"ST_Overlaps",       // funcName
		0,                   // cacheSide
		false,               // emptyTrue
	    GEOSOverlaps,        // geosPredicate
		GEOSPreparedOverlaps // geosPreparedPredicate
		gbox_overlaps_2d,    // bboxInteract
		NULL                 // itreeCompare
		);
}


PG_FUNCTION_INFO_V1(contains);
Datum contains(PG_FUNCTION_ARGS)
{
	return GeosPredicate(
		fcinfo,              // FunctionCallInfo
		"ST_Contains",       // funcName
		1,                   // cacheSide
		false,               // emptyTrue
	    GEOSContains,        // geosPredicate
		GEOSPreparedContains // geosPreparedPredicate
		gbox_contains_2d,    // bboxInteract
		itree_pip_contains   // itreeCompare
		);
}


PG_FUNCTION_INFO_V1(within);
Datum within(PG_FUNCTION_ARGS)
{
	return GeosPredicate(
		fcinfo,              // FunctionCallInfo
		"ST_Within",         // funcName
		1,                   // cacheSide
		false,               // emptyTrue
	    GEOSWithin,          // geosPredicate
		GEOSPreparedWithin   // geosPreparedPredicate
		gbox_within_2d,      // bboxInteract
		itree_pip_contains   // itreeCompare
		);
}



PG_FUNCTION_INFO_V1(containsproperly);
Datum containsproperly(PG_FUNCTION_ARGS)
{
	return GeosPredicate(
		fcinfo,                // FunctionCallInfo
		"ST_ContainsProperly", // funcName
		1,                     // cacheSide
		false,                 // emptyTrue
	    GEOSContainsProperly,  // geosPredicate
		GEOSPreparedContainsProperly // geosPreparedPredicate
		gbox_contains_2d,      // bboxInteract
		NULL                   // itreeCompare
		);
}


/*
 * Described at
 * http://lin-ear-th-inking.blogspot.com/2007/06/subtleties-of-ogc-covers-spatial.html
 */
PG_FUNCTION_INFO_V1(covers);
Datum covers(PG_FUNCTION_ARGS)
{
	SHARED_GSERIALIZED *shared_geom1 = ToastCacheGetGeometry(fcinfo, 0);
	SHARED_GSERIALIZED *shared_geom2 = ToastCacheGetGeometry(fcinfo, 1);
	const GSERIALIZED *geom1 = shared_gserialized_get(shared_geom1);
	const GSERIALIZED *geom2 = shared_gserialized_get(shared_geom2);
	int result;
	GBOX box1, box2;
	PrepGeomCache *prep_cache;

	POSTGIS_DEBUGF(3, "Covers: type1: %d, type2: %d", gserialized_get_type(geom1), gserialized_get_type(geom2));

	/* A.Covers(Empty) == FALSE */
	if ( gserialized_is_empty(geom1) || gserialized_is_empty(geom2) )
		PG_RETURN_BOOL(false);

	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	/*
	 * short-circuit 1: if geom2 bounding box is not completely inside
	 * geom1 bounding box we can return FALSE.
	 */
	if ( gserialized_get_gbox_p(geom1, &box1) &&
	     gserialized_get_gbox_p(geom2, &box2) )
	{
		if ( ! gbox_contains_2d(&box1, &box2) )
		{
			PG_RETURN_BOOL(false);
		}
	}
	/*
	 * short-circuit 2: if geom2 is a point and geom1 is a polygon
	 * call the point-in-polygon function.
	 */
	if (is_poly(geom1) && is_point(geom2))
	{
		const GSERIALIZED *gpoint = shared_gserialized_get(shared_geom2);
		LWGEOM *lwpt = lwgeom_from_gserialized(gpoint);
		IntervalTree *itree = GetIntervalTree(fcinfo, shared_geom1);
		bool result = itree_pip_covers(itree, lwpt);
		lwgeom_free(lwpt);
		PG_RETURN_BOOL(result);
	}

	initGEOS(lwpgnotice, lwgeom_geos_error);

	prep_cache = GetPrepGeomCache(fcinfo, shared_geom1, 0);

	if ( prep_cache && prep_cache->prepared_geom && prep_cache->gcache.argnum == 1 )
	{
		GEOSGeometry *g1 = POSTGIS2GEOS(geom2);
		if (!g1) HANDLE_GEOS_ERROR("First argument geometry could not be converted to GEOS");
		result = GEOSPreparedCovers( prep_cache->prepared_geom, g1);
		GEOSGeom_destroy(g1);
	}
	else
	{
		GEOSGeometry *g1;
		GEOSGeometry *g2;

		g1 = POSTGIS2GEOS(geom1);
		if (!g1) HANDLE_GEOS_ERROR("First argument geometry could not be converted to GEOS");
		g2 = POSTGIS2GEOS(geom2);
		if (!g2)
		{
			GEOSGeom_destroy(g1);
			HANDLE_GEOS_ERROR("Second argument geometry could not be converted to GEOS");
		}
		result = GEOSRelatePattern( g1, g2, "******FF*" );
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
	}

	if (result == 2) HANDLE_GEOS_ERROR("GEOSCovers");

	PG_RETURN_BOOL(result);
}


/**
* ST_Within(A, B) => ST_Contains(B, A) so we just delegate this calculation to the
* Contains implementation.
PG_FUNCTION_INFO_V1(within);
Datum within(PG_FUNCTION_ARGS)
*/

/*
 * Described at:
 * http://lin-ear-th-inking.blogspot.com/2007/06/subtleties-of-ogc-covers-spatial.html
 */
PG_FUNCTION_INFO_V1(coveredby);
Datum coveredby(PG_FUNCTION_ARGS)
{
	SHARED_GSERIALIZED *shared_geom1 = ToastCacheGetGeometry(fcinfo, 0);
	SHARED_GSERIALIZED *shared_geom2 = ToastCacheGetGeometry(fcinfo, 1);
	const GSERIALIZED *geom1 = shared_gserialized_get(shared_geom1);
	const GSERIALIZED *geom2 = shared_gserialized_get(shared_geom2);
	GEOSGeometry *g1, *g2;
	int result;
	GBOX box1, box2;
	char *patt = "**F**F***";

	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	POSTGIS_DEBUGF(3, "CoveredBy: type1: %d, type2: %d", gserialized_get_type(geom1), gserialized_get_type(geom2));

	/* A.CoveredBy(Empty) == FALSE */
	if ( gserialized_is_empty(geom1) || gserialized_is_empty(geom2) )
		PG_RETURN_BOOL(false);

	/*
	 * short-circuit 1: if geom1 bounding box is not completely inside
	 * geom2 bounding box we can return FALSE.
	 */
	if ( gserialized_get_gbox_p(geom1, &box1) &&
	     gserialized_get_gbox_p(geom2, &box2) )
	{
		if ( ! gbox_contains_2d(&box2, &box1) )
		{
			PG_RETURN_BOOL(false);
		}

		POSTGIS_DEBUG(3, "bounding box short-circuit missed.");
	}
	/*
	 * short-circuit 2: if geom1 is a point and geom2 is a polygon
	 * call the point-in-polygon function.
	 */
	if (is_point(geom1) && is_poly(geom2))
	{
		const GSERIALIZED *gpoint = shared_gserialized_get(shared_geom1);
		IntervalTree *itree = GetIntervalTree(fcinfo, shared_geom2);
		LWGEOM *lwpt = lwgeom_from_gserialized(gpoint);
		bool result = itree_pip_covers(itree, lwpt);
		lwgeom_free(lwpt);
		PG_RETURN_BOOL(result);
	}

	initGEOS(lwpgnotice, lwgeom_geos_error);

	g1 = POSTGIS2GEOS(geom1);

	if (!g1)
		HANDLE_GEOS_ERROR("First argument geometry could not be converted to GEOS");

	g2 = POSTGIS2GEOS(geom2);

	if (!g2)
	{
		GEOSGeom_destroy(g1);
		HANDLE_GEOS_ERROR("Second argument geometry could not be converted to GEOS");
	}

	result = GEOSRelatePattern(g1,g2,patt);

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (result == 2) HANDLE_GEOS_ERROR("GEOSCoveredBy");

	PG_RETURN_BOOL(result);
}




PG_FUNCTION_INFO_V1(crosses);
Datum crosses(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *geom2 = PG_GETARG_GSERIALIZED_P(1);
	int result = GeosPredicate(fcinfo, geom1, geom2, "ST_Crosses", GEOSCrosses);
	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);
	PG_RETURN_BOOL(result);
}


// PG_FUNCTION_INFO_V1(crosses);
// Datum crosses(PG_FUNCTION_ARGS)
// {
// 	GSERIALIZED *geom1;
// 	GSERIALIZED *geom2;
// 	GEOSGeometry *g1, *g2;
// 	int result;
// 	GBOX box1, box2;

// 	geom1 = PG_GETARG_GSERIALIZED_P(0);
// 	geom2 = PG_GETARG_GSERIALIZED_P(1);
// 	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

// 	/* A.Crosses(Empty) == FALSE */
// 	if ( gserialized_is_empty(geom1) || gserialized_is_empty(geom2) )
// 		PG_RETURN_BOOL(false);

// 	/*
// 	 * short-circuit 1: if geom2 bounding box does not overlap
// 	 * geom1 bounding box we can return FALSE.
// 	 */
// 	if ( gserialized_get_gbox_p(geom1, &box1) &&
// 	     gserialized_get_gbox_p(geom2, &box2) )
// 	{
// 		if ( gbox_overlaps_2d(&box1, &box2) == LW_FALSE )
// 		{
// 			PG_RETURN_BOOL(false);
// 		}
// 	}

// 	initGEOS(lwpgnotice, lwgeom_geos_error);

// 	g1 = POSTGIS2GEOS(geom1);
// 	if (!g1)
// 		HANDLE_GEOS_ERROR("First argument geometry could not be converted to GEOS");

// 	g2 = POSTGIS2GEOS(geom2);
// 	if (!g2)
// 	{
// 		GEOSGeom_destroy(g1);
// 		HANDLE_GEOS_ERROR("Second argument geometry could not be converted to GEOS");
// 	}

// 	result = GEOSCrosses(g1,g2);

// 	GEOSGeom_destroy(g1);
// 	GEOSGeom_destroy(g2);

// 	if (result == 2) HANDLE_GEOS_ERROR("GEOSCrosses");

// 	PG_FREE_IF_COPY(geom1, 0);
// 	PG_FREE_IF_COPY(geom2, 1);

// 	PG_RETURN_BOOL(result);
// }

PG_FUNCTION_INFO_V1(ST_Intersects);
Datum ST_Intersects(PG_FUNCTION_ARGS)
{
	SHARED_GSERIALIZED *shared_geom1 = ToastCacheGetGeometry(fcinfo, 0);
	SHARED_GSERIALIZED *shared_geom2 = ToastCacheGetGeometry(fcinfo, 1);
	const GSERIALIZED *geom1 = shared_gserialized_get(shared_geom1);
	const GSERIALIZED *geom2 = shared_gserialized_get(shared_geom2);
	int result;
	GBOX box1, box2;
	PrepGeomCache *prep_cache;

	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	/* A.Intersects(Empty) == FALSE */
	if ( gserialized_is_empty(geom1) || gserialized_is_empty(geom2) )
		PG_RETURN_BOOL(false);

	/*
	 * short-circuit 1: if geom2 bounding box does not overlap
	 * geom1 bounding box we can return FALSE.
	 */
	if ( gserialized_get_gbox_p(geom1, &box1) &&
	     gserialized_get_gbox_p(geom2, &box2) )
	{
		if ( gbox_overlaps_2d(&box1, &box2) == LW_FALSE )
			PG_RETURN_BOOL(false);
	}

	/*
	 * short-circuit 2: if the geoms are a point and a polygon,
	 * call the itree_pip_intersects function.
	 */
	if ((is_point(geom1) && is_poly(geom2)) ||
	    (is_point(geom2) && is_poly(geom1)))
	{
		SHARED_GSERIALIZED *shared_gpoly = is_poly(geom1) ? shared_geom1 : shared_geom2;
		SHARED_GSERIALIZED *shared_gpoint = is_point(geom1) ? shared_geom1 : shared_geom2;
		const GSERIALIZED *gpoint = shared_gserialized_get(shared_gpoint);
		LWGEOM *lwpt = lwgeom_from_gserialized(gpoint);
		IntervalTree *itree = GetIntervalTree(fcinfo, shared_gpoly);
		bool result = itree_pip_intersects(itree, lwpt);
		lwgeom_free(lwpt);
		PG_RETURN_BOOL(result);
	}

	initGEOS(lwpgnotice, lwgeom_geos_error);
	prep_cache = GetPrepGeomCache(fcinfo, shared_geom1, shared_geom2);

	if ( prep_cache && prep_cache->prepared_geom )
	{
		if ( prep_cache->gcache.argnum == 1 )
		{
			GEOSGeometry *g = POSTGIS2GEOS(geom2);
			if (!g) HANDLE_GEOS_ERROR("Geometry could not be converted to GEOS");
			result = GEOSPreparedIntersects( prep_cache->prepared_geom, g);
			GEOSGeom_destroy(g);
		}
		else
		{
			GEOSGeometry *g = POSTGIS2GEOS(geom1);
			if (!g)
				HANDLE_GEOS_ERROR("Geometry could not be converted to GEOS");
			result = GEOSPreparedIntersects( prep_cache->prepared_geom, g);
			GEOSGeom_destroy(g);
		}
	}
	else
	{
		GEOSGeometry *g1;
		GEOSGeometry *g2;
		g1 = POSTGIS2GEOS(geom1);
		if (!g1) HANDLE_GEOS_ERROR("First argument geometry could not be converted to GEOS");
		g2 = POSTGIS2GEOS(geom2);
		if (!g2)
		{
			GEOSGeom_destroy(g1);
			HANDLE_GEOS_ERROR("Second argument geometry could not be converted to GEOS");
		}
		result = GEOSIntersects( g1, g2);
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
	}

	if (result == 2) HANDLE_GEOS_ERROR("GEOSIntersects");

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(touches);
Datum touches(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom1;
	GSERIALIZED *geom2;
	GEOSGeometry *g1, *g2;
	char result;
	GBOX box1, box2;

	geom1 = PG_GETARG_GSERIALIZED_P(0);
	geom2 = PG_GETARG_GSERIALIZED_P(1);
	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	/* A.Touches(Empty) == FALSE */
	if ( gserialized_is_empty(geom1) || gserialized_is_empty(geom2) )
		PG_RETURN_BOOL(false);

	/*
	 * short-circuit 1: if geom2 bounding box does not overlap
	 * geom1 bounding box we can return FALSE.
	 */
	if ( gserialized_get_gbox_p(geom1, &box1) &&
	     gserialized_get_gbox_p(geom2, &box2) )
	{
		if ( gbox_overlaps_2d(&box1, &box2) == LW_FALSE )
		{
			PG_RETURN_BOOL(false);
		}
	}

	initGEOS(lwpgnotice, lwgeom_geos_error);

	g1 = POSTGIS2GEOS(geom1 );
	if (!g1)
		HANDLE_GEOS_ERROR("First argument geometry could not be converted to GEOS");

	g2 = POSTGIS2GEOS(geom2 );
	if (!g2)
	{
		GEOSGeom_destroy(g1);
		HANDLE_GEOS_ERROR("Second argument geometry could not be converted to GEOS");
	}

	result = GEOSTouches(g1,g2);

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (result == 2) HANDLE_GEOS_ERROR("GEOSTouches");

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(disjoint);
Datum disjoint(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom1;
	GSERIALIZED *geom2;
	GEOSGeometry *g1, *g2;
	char result;
	GBOX box1, box2;

	geom1 = PG_GETARG_GSERIALIZED_P(0);
	geom2 = PG_GETARG_GSERIALIZED_P(1);
	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	/* A.Disjoint(Empty) == TRUE */
	if ( gserialized_is_empty(geom1) || gserialized_is_empty(geom2) )
		PG_RETURN_BOOL(true);

	/*
	 * short-circuit 1: if geom2 bounding box does not overlap
	 * geom1 bounding box we can return TRUE.
	 */
	if ( gserialized_get_gbox_p(geom1, &box1) &&
	     gserialized_get_gbox_p(geom2, &box2) )
	{
		if ( gbox_overlaps_2d(&box1, &box2) == LW_FALSE )
		{
			PG_RETURN_BOOL(true);
		}
	}

	initGEOS(lwpgnotice, lwgeom_geos_error);

	g1 = POSTGIS2GEOS(geom1);
	if (!g1)
		HANDLE_GEOS_ERROR("First argument geometry could not be converted to GEOS");

	g2 = POSTGIS2GEOS(geom2);
	if (!g2)
	{
		GEOSGeom_destroy(g1);
		HANDLE_GEOS_ERROR("Second argument geometry could not be converted to GEOS");
	}

	result = GEOSDisjoint(g1,g2);

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (result == 2) HANDLE_GEOS_ERROR("GEOSDisjoint");

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(relate_pattern);
Datum relate_pattern(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom1;
	GSERIALIZED *geom2;
	char *patt;
	char result;
	GEOSGeometry *g1, *g2;
	size_t i;

	geom1 = PG_GETARG_GSERIALIZED_P(0);
	geom2 = PG_GETARG_GSERIALIZED_P(1);
	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	/* TODO handle empty */

	initGEOS(lwpgnotice, lwgeom_geos_error);

	g1 = POSTGIS2GEOS(geom1);
	if (!g1)
		HANDLE_GEOS_ERROR("First argument geometry could not be converted to GEOS");
	g2 = POSTGIS2GEOS(geom2);
	if (!g2)
	{
		GEOSGeom_destroy(g1);
		HANDLE_GEOS_ERROR("Second argument geometry could not be converted to GEOS");
	}

	patt =  DatumGetCString(DirectFunctionCall1(textout, PG_GETARG_DATUM(2)));

	/*
	** Need to make sure 't' and 'f' are upper-case before handing to GEOS
	*/
	for ( i = 0; i < strlen(patt); i++ )
	{
		if ( patt[i] == 't' ) patt[i] = 'T';
		if ( patt[i] == 'f' ) patt[i] = 'F';
	}

	result = GEOSRelatePattern(g1,g2,patt);
	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	pfree(patt);

	if (result == 2) HANDLE_GEOS_ERROR("GEOSRelatePattern");

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}



PG_FUNCTION_INFO_V1(relate_full);
Datum relate_full(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom1;
	GSERIALIZED *geom2;
	GEOSGeometry *g1, *g2;
	char *relate_str;
	text *result;
	int bnr = GEOSRELATE_BNR_OGC;

	/* TODO handle empty */
	geom1 = PG_GETARG_GSERIALIZED_P(0);
	geom2 = PG_GETARG_GSERIALIZED_P(1);
	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	if ( PG_NARGS() > 2 )
		bnr = PG_GETARG_INT32(2);

	initGEOS(lwpgnotice, lwgeom_geos_error);

	g1 = POSTGIS2GEOS(geom1 );
	if (!g1)
		HANDLE_GEOS_ERROR("First argument geometry could not be converted to GEOS");
	g2 = POSTGIS2GEOS(geom2 );
	if (!g2)
	{
		GEOSGeom_destroy(g1);
		HANDLE_GEOS_ERROR("Second argument geometry could not be converted to GEOS");
	}

	POSTGIS_DEBUG(3, "constructed geometries ");

	POSTGIS_DEBUGF(3, "%s", GEOSGeomToWKT(g1));
	POSTGIS_DEBUGF(3, "%s", GEOSGeomToWKT(g2));

	relate_str = GEOSRelateBoundaryNodeRule(g1, g2, bnr);

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (!relate_str) HANDLE_GEOS_ERROR("GEOSRelate");

	result = cstring_to_text(relate_str);
	GEOSFree(relate_str);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_TEXT_P(result);
}


PG_FUNCTION_INFO_V1(ST_Equals);
Datum ST_Equals(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom1;
	GSERIALIZED *geom2;
	GEOSGeometry *g1, *g2;
	char result;
	GBOX box1, box2;

	geom1 = PG_GETARG_GSERIALIZED_P(0);
	geom2 = PG_GETARG_GSERIALIZED_P(1);
	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	/* Empty == Empty */
	if ( gserialized_is_empty(geom1) && gserialized_is_empty(geom2) )
		PG_RETURN_BOOL(true);

	/*
	 * short-circuit: If geom1 and geom2 do not have the same bounding box
	 * we can return FALSE.
	 */
	if ( gserialized_get_gbox_p(geom1, &box1) &&
	     gserialized_get_gbox_p(geom2, &box2) )
	{
		if ( gbox_same_2d_float(&box1, &box2) == LW_FALSE )
		{
			PG_RETURN_BOOL(false);
		}
	}

	/*
	 * short-circuit: if geom1 and geom2 are binary-equivalent, we can return
	 * TRUE.  This is much faster than doing the comparison using GEOS.
	 */
	if (VARSIZE(geom1) == VARSIZE(geom2) && !memcmp(geom1, geom2, VARSIZE(geom1))) {
	    PG_RETURN_BOOL(true);
	}

	initGEOS(lwpgnotice, lwgeom_geos_error);

	g1 = POSTGIS2GEOS(geom1);

	if (!g1)
		HANDLE_GEOS_ERROR("First argument geometry could not be converted to GEOS");

	g2 = POSTGIS2GEOS(geom2);

	if (!g2)
	{
		GEOSGeom_destroy(g1);
		HANDLE_GEOS_ERROR("Second argument geometry could not be converted to GEOS");
	}

	result = GEOSEquals(g1,g2);

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (result == 2) HANDLE_GEOS_ERROR("GEOSEquals");

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}

