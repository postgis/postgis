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
#include "lwgeom_accum.h"


/* Prototypes for SQL-bound functions */
Datum relate_full(PG_FUNCTION_ARGS);
Datum relate_pattern(PG_FUNCTION_ARGS);
Datum disjoint(PG_FUNCTION_ARGS);
Datum touches(PG_FUNCTION_ARGS);
Datum ST_Intersects(PG_FUNCTION_ARGS);
Datum crosses(PG_FUNCTION_ARGS);
Datum contains(PG_FUNCTION_ARGS);
Datum within(PG_FUNCTION_ARGS);
Datum containsproperly(PG_FUNCTION_ARGS);
Datum covers(PG_FUNCTION_ARGS);
Datum overlaps(PG_FUNCTION_ARGS);
Datum coveredby(PG_FUNCTION_ARGS);
Datum ST_Equals(PG_FUNCTION_ARGS);


/*
 * Utility to quickly check for polygonal geometries
 */
static inline char
is_poly(const GSERIALIZED* g)
{
    int type = gserialized_get_type(g);
    return type == POLYGONTYPE || type == MULTIPOLYGONTYPE;
}

/*
 * Utility to quickly check for point geometries
 */
static inline char
is_point(const GSERIALIZED* g)
{
	int type = gserialized_get_type(g);
	return type == POINTTYPE || type == MULTIPOINTTYPE;
}







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
	 * Short-circuit 1: if geom2 bounding box does not overlap
	 * geom1 bounding box we can return FALSE.
	 */
	if ( gserialized_get_gbox_p(geom1, &box1) &&
	     gserialized_get_gbox_p(geom2, &box2) )
	{
		if ( gbox_overlaps_2d(&box1, &box2) == LW_FALSE )
			PG_RETURN_BOOL(false);
	}

	/*
	 * Short-circuit 2: if the geoms are a point and a polygon,
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
		GEOSGeometry *g = prep_cache->gcache.argnum == 1
			? POSTGIS2GEOS(geom2)
			: POSTGIS2GEOS(geom1);
		if (!g) HANDLE_GEOS_ERROR("Geometry could not be converted to GEOS");
		result = GEOSPreparedIntersects(prep_cache->prepared_geom, g);
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
		result = GEOSIntersects(g1, g2);
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
	}

	if (result == 2) HANDLE_GEOS_ERROR("GEOSIntersects");

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(ST_Equals);
Datum ST_Equals(PG_FUNCTION_ARGS)
{
	SHARED_GSERIALIZED *shared_geom1 = ToastCacheGetGeometry(fcinfo, 0);
	SHARED_GSERIALIZED *shared_geom2 = ToastCacheGetGeometry(fcinfo, 1);
	const GSERIALIZED *geom1 = shared_gserialized_get(shared_geom1);
	const GSERIALIZED *geom2 = shared_gserialized_get(shared_geom2);
	char result;
	GBOX box1, box2;
	GEOSGeometry *g1, *g2;

	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	/* Empty == Empty */
	if ( gserialized_is_empty(geom1) && gserialized_is_empty(geom2) )
		PG_RETURN_BOOL(true);

	/*
	 * Short-circuit: If geom1 and geom2 do not have the same bounding box
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
	 * Short-circuit: if geom1 and geom2 are binary-equivalent, we can return
	 * TRUE.  This is much faster than doing the comparison using GEOS.
	 */
	if (VARSIZE(geom1) == VARSIZE(geom2) && !memcmp(geom1, geom2, VARSIZE(geom1))) {
	    PG_RETURN_BOOL(true);
	}

	initGEOS(lwpgnotice, lwgeom_geos_error);

	g1 = POSTGIS2GEOS(geom1);
	if (!g1) HANDLE_GEOS_ERROR("First argument geometry could not be converted to GEOS");
	g2 = POSTGIS2GEOS(geom2);
	if (!g2)
	{
		GEOSGeom_destroy(g1);
		HANDLE_GEOS_ERROR("Second argument geometry could not be converted to GEOS");
	}
	result = GEOSEquals(g1, g2);
	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (result == 2) HANDLE_GEOS_ERROR("GEOSEquals");

	PG_RETURN_BOOL(result);
}



PG_FUNCTION_INFO_V1(touches);
Datum touches(PG_FUNCTION_ARGS)
{
	SHARED_GSERIALIZED *shared_geom1 = ToastCacheGetGeometry(fcinfo, 0);
	SHARED_GSERIALIZED *shared_geom2 = ToastCacheGetGeometry(fcinfo, 1);
	const GSERIALIZED *geom1 = shared_gserialized_get(shared_geom1);
	const GSERIALIZED *geom2 = shared_gserialized_get(shared_geom2);
	char result;
	GBOX box1, box2;
	PrepGeomCache *prep_cache;

	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	/* A.Touches(Empty) == FALSE */
	if ( gserialized_is_empty(geom1) || gserialized_is_empty(geom2) )
		PG_RETURN_BOOL(false);

	/*
	 * Short-circuit 1: if geom2 bounding box does not overlap
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

	prep_cache = GetPrepGeomCache(fcinfo, shared_geom1, shared_geom2);
	if ( prep_cache && prep_cache->prepared_geom )
	{
		GEOSGeometry *g = prep_cache->gcache.argnum == 1
			? POSTGIS2GEOS(geom2)
			: POSTGIS2GEOS(geom1);
		if (!g) HANDLE_GEOS_ERROR("Geometry could not be converted to GEOS");
		result = GEOSPreparedTouches(prep_cache->prepared_geom, g);
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
		result = GEOSTouches(g1, g2);
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
	}

	if (result == 2) HANDLE_GEOS_ERROR("GEOSTouches");

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(disjoint);
Datum disjoint(PG_FUNCTION_ARGS)
{
	SHARED_GSERIALIZED *shared_geom1 = ToastCacheGetGeometry(fcinfo, 0);
	SHARED_GSERIALIZED *shared_geom2 = ToastCacheGetGeometry(fcinfo, 1);
	const GSERIALIZED *geom1 = shared_gserialized_get(shared_geom1);
	const GSERIALIZED *geom2 = shared_gserialized_get(shared_geom2);
	char result;
	GBOX box1, box2;
	PrepGeomCache *prep_cache;

	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	/* A.Disjoint(Empty) == TRUE */
	if ( gserialized_is_empty(geom1) || gserialized_is_empty(geom2) )
		PG_RETURN_BOOL(true);

	/*
	 * Short-circuit 1: if geom2 bounding box does not overlap
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

	prep_cache = GetPrepGeomCache(fcinfo, shared_geom1, shared_geom2);
	if ( prep_cache && prep_cache->prepared_geom )
	{
		GEOSGeometry *g = prep_cache->gcache.argnum == 1
			? POSTGIS2GEOS(geom2)
			: POSTGIS2GEOS(geom1);
		if (!g) HANDLE_GEOS_ERROR("Geometry could not be converted to GEOS");
		result = GEOSPreparedDisjoint(prep_cache->prepared_geom, g);
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
		result = GEOSDisjoint(g1, g2);
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
	}

	if (result == 2) HANDLE_GEOS_ERROR("GEOSDisjoint");

	PG_RETURN_BOOL(result);
}



/**
 * ST_Overlaps(geometry, geometry)
 */
PG_FUNCTION_INFO_V1(overlaps);
Datum overlaps(PG_FUNCTION_ARGS)
{
	SHARED_GSERIALIZED *shared_geom1 = ToastCacheGetGeometry(fcinfo, 0);
	SHARED_GSERIALIZED *shared_geom2 = ToastCacheGetGeometry(fcinfo, 1);
	const GSERIALIZED *geom1 = shared_gserialized_get(shared_geom1);
	const GSERIALIZED *geom2 = shared_gserialized_get(shared_geom2);
	char result;
	GBOX box1, box2;
	PrepGeomCache *prep_cache;

	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	/* A.Overlaps(Empty) == FALSE */
	if ( gserialized_is_empty(geom1) || gserialized_is_empty(geom2) )
		PG_RETURN_BOOL(false);

	/*
	 * Short-circuit 1: if geom2 bounding box does not overlap
	 * geom1 bounding box we can return FALSE.
	 */
	if ( gserialized_get_gbox_p(geom1, &box1) &&
	     gserialized_get_gbox_p(geom2, &box2) )
	{
		if ( ! gbox_overlaps_2d(&box1, &box2) )
		{
			PG_RETURN_BOOL(false);
		}
	}

	initGEOS(lwpgnotice, lwgeom_geos_error);

	prep_cache = GetPrepGeomCache(fcinfo, shared_geom1, shared_geom2);
	if ( prep_cache && prep_cache->prepared_geom )
	{
		GEOSGeometry *g = prep_cache->gcache.argnum == 1
			? POSTGIS2GEOS(geom2)
			: POSTGIS2GEOS(geom1);
		if (!g) HANDLE_GEOS_ERROR("Geometry could not be converted to GEOS");
		result = GEOSPreparedOverlaps(prep_cache->prepared_geom, g);
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
		result = GEOSOverlaps(g1, g2);
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
	}

	if (result == 2) HANDLE_GEOS_ERROR("GEOSOverlaps");

	PG_RETURN_BOOL(result);
}


/**
 * ST_Crosses(geometry, geometry)
 */
PG_FUNCTION_INFO_V1(crosses);
Datum crosses(PG_FUNCTION_ARGS)
{
	SHARED_GSERIALIZED *shared_geom1 = ToastCacheGetGeometry(fcinfo, 0);
	SHARED_GSERIALIZED *shared_geom2 = ToastCacheGetGeometry(fcinfo, 1);
	const GSERIALIZED *geom1 = shared_gserialized_get(shared_geom1);
	const GSERIALIZED *geom2 = shared_gserialized_get(shared_geom2);
	int result;
	GBOX box1, box2;
	PrepGeomCache *prep_cache;

	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	/* A.Crosses(Empty) == FALSE */
	if ( gserialized_is_empty(geom1) || gserialized_is_empty(geom2) )
		PG_RETURN_BOOL(false);

	/*
	 * Short-circuit 1: if geom2 bounding box does not overlap
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

	prep_cache = GetPrepGeomCache(fcinfo, shared_geom1, shared_geom2);
	if ( prep_cache && prep_cache->prepared_geom )
	{
		GEOSGeometry *g = prep_cache->gcache.argnum == 1
			? POSTGIS2GEOS(geom2)
			: POSTGIS2GEOS(geom1);
		if (!g) HANDLE_GEOS_ERROR("Geometry could not be converted to GEOS");
		result = GEOSPreparedCrosses(prep_cache->prepared_geom, g);
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
		result = GEOSCrosses(g1, g2);
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
	}

	if (result == 2) HANDLE_GEOS_ERROR("GEOSCrosses");

	PG_RETURN_BOOL(result);
}



/**
 * ST_Contains(geometry, geometry)
 */
PG_FUNCTION_INFO_V1(contains);
Datum contains(PG_FUNCTION_ARGS)
{
	SHARED_GSERIALIZED *shared_geom1 = ToastCacheGetGeometry(fcinfo, 0);
	SHARED_GSERIALIZED *shared_geom2 = ToastCacheGetGeometry(fcinfo, 1);
	const GSERIALIZED *geom1 = shared_gserialized_get(shared_geom1);
	const GSERIALIZED *geom2 = shared_gserialized_get(shared_geom2);
	int result;
	GEOSGeometry *g1, *g2;
	GBOX box1, box2;
	PrepGeomCache *prep_cache;

	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	/* A.Contains(Empty) == FALSE */
	if (gserialized_is_empty(geom1) || gserialized_is_empty(geom2))
		PG_RETURN_BOOL(false);

	POSTGIS_DEBUGF(3, "Contains: type1: %d, type2: %d", gserialized_get_type(geom1), gserialized_get_type(geom2));

	/*
	** Short-circuit 1: if geom2 bounding box is not completely inside
	** geom1 bounding box we can return FALSE.
	*/
	if (gserialized_get_gbox_p(geom1, &box1) &&
	    gserialized_get_gbox_p(geom2, &box2))
	{
		if (!gbox_contains_2d(&box1, &box2))
			PG_RETURN_BOOL(false);
	}

	/*
	** Short-circuit 2: if geom2 is a point and geom1 is a polygon
	** call the point-in-polygon function.
	*/
	if (is_poly(geom1) && is_point(geom2))
	{
		const GSERIALIZED *gpoint = shared_gserialized_get(shared_geom2);
		LWGEOM *lwpt = lwgeom_from_gserialized(gpoint);
		IntervalTree *itree = GetIntervalTree(fcinfo, shared_geom1);
		bool result = itree_pip_contains(itree, lwpt);
		lwgeom_free(lwpt);
		PG_RETURN_BOOL(result);
	}

	initGEOS(lwpgnotice, lwgeom_geos_error);

	prep_cache = GetPrepGeomCache(fcinfo, shared_geom1, NULL);
	if ( prep_cache && prep_cache->prepared_geom && prep_cache->gcache.argnum == 1 )
	{
		g1 = POSTGIS2GEOS(geom2);
		if (!g1)
			HANDLE_GEOS_ERROR("Geometry could not be converted to GEOS");

		result = GEOSPreparedContains( prep_cache->prepared_geom, g1);
		GEOSGeom_destroy(g1);
	}
	else
	{
		g1 = POSTGIS2GEOS(geom1);
		if (!g1) HANDLE_GEOS_ERROR("First argument geometry could not be converted to GEOS");
		g2 = POSTGIS2GEOS(geom2);
		if (!g2)
		{
			HANDLE_GEOS_ERROR("Second argument geometry could not be converted to GEOS");
			GEOSGeom_destroy(g1);
		}
		result = GEOSContains( g1, g2);
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
	}

	if (result == 2) HANDLE_GEOS_ERROR("GEOSContains");

	PG_RETURN_BOOL(result > 0);
}


/**
 * ST_Within(geometry, geometry)
 */
PG_FUNCTION_INFO_V1(within);
Datum within(PG_FUNCTION_ARGS)
{
	SHARED_GSERIALIZED *shared_geom1 = ToastCacheGetGeometry(fcinfo, 0);
	SHARED_GSERIALIZED *shared_geom2 = ToastCacheGetGeometry(fcinfo, 1);
	const GSERIALIZED *geom1 = shared_gserialized_get(shared_geom1);
	const GSERIALIZED *geom2 = shared_gserialized_get(shared_geom2);
	int result;
	GEOSGeometry *g1, *g2;
	GBOX box1, box2;
	PrepGeomCache *prep_cache;

	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	/* A.Contains(Empty) == FALSE */
	if (gserialized_is_empty(geom1) || gserialized_is_empty(geom2))
		PG_RETURN_BOOL(false);

	POSTGIS_DEBUGF(3, "Contains: type1: %d, type2: %d", gserialized_get_type(geom1), gserialized_get_type(geom2));

	/*
	** Short-circuit 1: if geom1 bounding box is not completely inside
	** geom2 bounding box we can return FALSE.
	*/
	if (gserialized_get_gbox_p(geom1, &box1) &&
	    gserialized_get_gbox_p(geom2, &box2))
	{
		if (!gbox_within_2d(&box1, &box2))
			PG_RETURN_BOOL(false);
	}

	/*
	** Short-circuit 2: if geom2 is a polygon and geom1 is a point
	** call the point-in-polygon function.
	*/
	if (is_poly(geom2) && is_point(geom1))
	{
		const GSERIALIZED *gpoint = shared_gserialized_get(shared_geom1);
		LWGEOM *lwpt = lwgeom_from_gserialized(gpoint);
		IntervalTree *itree = GetIntervalTree(fcinfo, shared_geom2);
		bool result = itree_pip_contains(itree, lwpt);
		lwgeom_free(lwpt);
		PG_RETURN_BOOL(result);
	}

	initGEOS(lwpgnotice, lwgeom_geos_error);

	prep_cache = GetPrepGeomCache(fcinfo, NULL, shared_geom2);
	if ( prep_cache && prep_cache->prepared_geom && prep_cache->gcache.argnum == 2 )
	{
		g1 = POSTGIS2GEOS(geom1);
		if (!g1)
			HANDLE_GEOS_ERROR("Geometry could not be converted to GEOS");

		result = GEOSPreparedContains(prep_cache->prepared_geom, g1);
		GEOSGeom_destroy(g1);
	}
	else
	{
		g1 = POSTGIS2GEOS(geom1);
		if (!g1) HANDLE_GEOS_ERROR("First argument geometry could not be converted to GEOS");
		g2 = POSTGIS2GEOS(geom2);
		if (!g2)
		{
			HANDLE_GEOS_ERROR("Second argument geometry could not be converted to GEOS");
			GEOSGeom_destroy(g1);
		}
		result = GEOSWithin(g1, g2);
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
	}

	if ( result == 2 ) HANDLE_GEOS_ERROR("GEOSWithin");

	PG_RETURN_BOOL(result > 0);
}

/**
 * ST_ContainsProperly(geometry, geometry)
 */
PG_FUNCTION_INFO_V1(containsproperly);
Datum containsproperly(PG_FUNCTION_ARGS)
{
	SHARED_GSERIALIZED *shared_geom1 = ToastCacheGetGeometry(fcinfo, 0);
	SHARED_GSERIALIZED *shared_geom2 = ToastCacheGetGeometry(fcinfo, 1);
	const GSERIALIZED *geom1 = shared_gserialized_get(shared_geom1);
	const GSERIALIZED *geom2 = shared_gserialized_get(shared_geom2);
	char result;
	GBOX box1, box2;
	PrepGeomCache *prep_cache;

	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	/* A.ContainsProperly(Empty) == FALSE */
	if ( gserialized_is_empty(geom1) || gserialized_is_empty(geom2) )
		PG_RETURN_BOOL(false);

	/*
	 * Short-circuit: if geom2 bounding box is not completely inside
	 * geom1 bounding box we can return FALSE.
	 */
	if ( gserialized_get_gbox_p(geom1, &box1) &&
	     gserialized_get_gbox_p(geom2, &box2) )
	{
		if ( ! gbox_contains_2d(&box1, &box2) )
			PG_RETURN_BOOL(false);
	}

	initGEOS(lwpgnotice, lwgeom_geos_error);

	prep_cache = GetPrepGeomCache(fcinfo, shared_geom1, 0);
	if ( prep_cache && prep_cache->prepared_geom && prep_cache->gcache.argnum == 1 )
	{
		GEOSGeometry *g = POSTGIS2GEOS(geom2);
		if (!g) HANDLE_GEOS_ERROR("First argument geometry could not be converted to GEOS");
		result = GEOSPreparedContainsProperly( prep_cache->prepared_geom, g);
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
		result = GEOSRelatePattern(g1, g2, "T**FF*FF*");
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
	}

	if (result == 2) HANDLE_GEOS_ERROR("GEOSContainProperly");

	PG_RETURN_BOOL(result);
}


/**
 * ST_Covers(geometry, geometry)
 *
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
	 * Short-circuit 1: if geom2 bounding box is not completely inside
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
	 * Short-circuit 2: if geom2 is a point and geom1 is a polygon
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
		GEOSGeometry *g1, *g2;

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
 * ST_CoveredBy(geometry, geometry)
 *
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
	int result;
	GBOX box1, box2;
	PrepGeomCache *prep_cache;

	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	POSTGIS_DEBUGF(3, "CoveredBy: type1: %d, type2: %d", gserialized_get_type(geom1), gserialized_get_type(geom2));

	/* A.CoveredBy(Empty) == FALSE */
	if ( gserialized_is_empty(geom1) || gserialized_is_empty(geom2) )
		PG_RETURN_BOOL(false);

	/*
	 * Short-circuit 1: if geom1 bounding box is not completely inside
	 * geom2 bounding box we can return FALSE.
	 */
	if ( gserialized_get_gbox_p(geom1, &box1) &&
	     gserialized_get_gbox_p(geom2, &box2) )
	{
		if ( ! gbox_within_2d(&box1, &box2) )
		{
			PG_RETURN_BOOL(false);
		}
	}
	/*
	 * Short-circuit 2: if geom1 is a point and geom2 is a polygon
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

	prep_cache = GetPrepGeomCache(fcinfo, shared_geom1, 0);
	if ( prep_cache && prep_cache->prepared_geom && prep_cache->gcache.argnum == 2)
	{
		GEOSGeometry *g1 = POSTGIS2GEOS(geom2);
		if (!g1) HANDLE_GEOS_ERROR("First argument geometry could not be converted to GEOS");
		result = GEOSPreparedCovers( prep_cache->prepared_geom, g1);
		GEOSGeom_destroy(g1);
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

		result = GEOSCoveredBy(g1, g2);
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
	}

	if (result == 2) HANDLE_GEOS_ERROR("GEOSCoveredBy");

	PG_RETURN_BOOL(result);
}


#if POSTGIS_GEOS_VERSION >= 31300
/*
 *  Flip the DE9IM matrix around the diagonal to
 *  account for flipping the order of the operands
 *  to the GEOSPreparedRelatePattern function.
 */
static void
imInvert(char *im)
{
	char t;
	t = im[1]; im[1] = im[3]; im[3] = t;
	t = im[2]; im[2] = im[6]; im[6] = t;
	t = im[5]; im[5] = im[7]; im[7] = t;
}
#endif

PG_FUNCTION_INFO_V1(relate_pattern);
Datum relate_pattern(PG_FUNCTION_ARGS)
{
	SHARED_GSERIALIZED *shared_geom1 = ToastCacheGetGeometry(fcinfo, 0);
	SHARED_GSERIALIZED *shared_geom2 = ToastCacheGetGeometry(fcinfo, 1);
	const GSERIALIZED *geom1 = shared_gserialized_get(shared_geom1);
	const GSERIALIZED *geom2 = shared_gserialized_get(shared_geom2);

	/* Ensure DE9IM pattern is no more than 9 chars */
	text *imPtr = DatumGetTextP(DirectFunctionCall2(text_left,
		PG_GETARG_DATUM(2), Int32GetDatum(9)));
	char *im = text_to_cstring(imPtr);
	char result;
	uint32_t i;
#if POSTGIS_GEOS_VERSION >= 31300
	PrepGeomCache *prep_cache;
#endif

	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	/*
	** Need to make sure 't' and 'f' are upper-case before handing to GEOS
	*/
	for (i = 0; i < strlen(im); i++)
		im[i] = toupper(im[i]);

	initGEOS(lwpgnotice, lwgeom_geos_error);

#if POSTGIS_GEOS_VERSION >= 31300
	prep_cache = GetPrepGeomCache(fcinfo, shared_geom1, shared_geom2);
	if ( prep_cache && prep_cache->prepared_geom )
	{
		GEOSGeometry *g = NULL;
		if (prep_cache->gcache.argnum == 1)
		{
			g = POSTGIS2GEOS(geom2);
		}
		else
		{
			g = POSTGIS2GEOS(geom1);
			imInvert(im);
		}

		if (!g) HANDLE_GEOS_ERROR("Geometry could not be converted to GEOS");
		result = GEOSPreparedRelatePattern(prep_cache->prepared_geom, g, im);
		GEOSGeom_destroy(g);
	}
	else
#endif
	{
		GEOSGeometry *g1, *g2;
		g1 = POSTGIS2GEOS(geom1);
		if (!g1) HANDLE_GEOS_ERROR("First argument geometry could not be converted to GEOS");
		g2 = POSTGIS2GEOS(geom2);
		if (!g2)
		{
			HANDLE_GEOS_ERROR("Second argument geometry could not be converted to GEOS");
			GEOSGeom_destroy(g1);
		}
		result = GEOSRelatePattern(g1, g2, im);
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
	}

	pfree(im);
	if (result == 2) HANDLE_GEOS_ERROR("GEOSRelatePattern");

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(relate_full);
Datum relate_full(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom1 = PG_GETARG_GSERIALIZED_P(0);
	GSERIALIZED *geom2 = PG_GETARG_GSERIALIZED_P(1);
	GEOSGeometry *g1, *g2;
	char *relate_str;
	text *result;
	int bnr = GEOSRELATE_BNR_OGC;

	/* TODO handle empty */
	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	if ( PG_NARGS() > 2 )
		bnr = PG_GETARG_INT32(2);

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

	POSTGIS_DEBUG(3, "constructed geometries ");

	POSTGIS_DEBUGF(3, "%s", GEOSGeomToWKT(g1));
	POSTGIS_DEBUGF(3, "%s", GEOSGeomToWKT(g2));

	relate_str = GEOSRelateBoundaryNodeRule(g1, g2, bnr);
	if (!relate_str) HANDLE_GEOS_ERROR("GEOSRelate");
	result = cstring_to_text(relate_str);
	GEOSFree(relate_str);
	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_TEXT_P(result);
}


