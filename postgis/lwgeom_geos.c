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
#include "lwgeom_functions_analytic.h" /* for point_in_polygon */
#include "lwgeom_geos.h"
#include "liblwgeom.h"
#include "lwgeom_rtree.h"
#include "lwgeom_geos_prepared.h"
#include "lwgeom_accum.h"


/* Return NULL on GEOS error
 *
 * Prints error message only if it was not for interruption, in which
 * case we let PostgreSQL deal with the error.
 */
#define HANDLE_GEOS_ERROR(label) \
	{ \
		if (strstr(lwgeom_geos_errmsg, "InterruptedException")) \
			ereport(ERROR, \
				(errcode(ERRCODE_QUERY_CANCELED), errmsg("canceling statement due to user request"))); \
		else \
			lwpgerror("%s: %s", (label), lwgeom_geos_errmsg); \
		PG_RETURN_NULL(); \
	}

/*
** Prototypes for SQL-bound functions
*/
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
Datum isvalid(PG_FUNCTION_ARGS);
Datum isvalidreason(PG_FUNCTION_ARGS);
Datum isvaliddetail(PG_FUNCTION_ARGS);
Datum buffer(PG_FUNCTION_ARGS);
Datum ST_Intersection(PG_FUNCTION_ARGS);
Datum convexhull(PG_FUNCTION_ARGS);
Datum topologypreservesimplify(PG_FUNCTION_ARGS);
Datum ST_Difference(PG_FUNCTION_ARGS);
Datum boundary(PG_FUNCTION_ARGS);
Datum ST_SymDifference(PG_FUNCTION_ARGS);
Datum ST_Union(PG_FUNCTION_ARGS);
Datum issimple(PG_FUNCTION_ARGS);
Datum isring(PG_FUNCTION_ARGS);
Datum pointonsurface(PG_FUNCTION_ARGS);
Datum GEOSnoop(PG_FUNCTION_ARGS);
Datum postgis_geos_version(PG_FUNCTION_ARGS);
Datum centroid(PG_FUNCTION_ARGS);
Datum polygonize_garray(PG_FUNCTION_ARGS);
Datum clusterintersecting_garray(PG_FUNCTION_ARGS);
Datum cluster_within_distance_garray(PG_FUNCTION_ARGS);
Datum linemerge(PG_FUNCTION_ARGS);
Datum coveredby(PG_FUNCTION_ARGS);
Datum hausdorffdistance(PG_FUNCTION_ARGS);
Datum hausdorffdistancedensify(PG_FUNCTION_ARGS);
Datum ST_FrechetDistance(PG_FUNCTION_ARGS);
Datum ST_UnaryUnion(PG_FUNCTION_ARGS);
Datum ST_Equals(PG_FUNCTION_ARGS);
Datum ST_BuildArea(PG_FUNCTION_ARGS);
Datum ST_DelaunayTriangles(PG_FUNCTION_ARGS);
Datum ST_MaximumInscribedCircle(PG_FUNCTION_ARGS);

Datum pgis_union_geometry_array(PG_FUNCTION_ARGS);
Datum pgis_geometry_union_finalfn(PG_FUNCTION_ARGS);

/*
** Prototypes end
*/


PG_FUNCTION_INFO_V1(postgis_geos_version);
Datum postgis_geos_version(PG_FUNCTION_ARGS)
{
	const char *ver = lwgeom_geos_version();
	text *result = cstring_to_text(ver);
	PG_RETURN_POINTER(result);
}

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

/* Utility function that checks a LWPOINT and a GSERIALIZED poly against a cache.
 * Serialized poly may be a multipart.
 */
static int
pip_short_circuit(RTREE_POLY_CACHE *poly_cache, LWPOINT *point, const GSERIALIZED *gpoly)
{
	int result;

	if ( poly_cache && poly_cache->ringIndices )
	{
        result = point_in_multipolygon_rtree(poly_cache->ringIndices, poly_cache->polyCount, poly_cache->ringCounts, point);
	}
	else
	{
		LWGEOM* poly = lwgeom_from_gserialized(gpoly);
		if ( lwgeom_get_type(poly) == POLYGONTYPE )
		{
			result = point_in_polygon(lwgeom_as_lwpoly(poly), point);
		}
		else
		{
			result = point_in_multipolygon(lwgeom_as_lwmpoly(poly), point);
		}
		lwgeom_free(poly);
	}

	return result;
}

/**
 *  @brief Compute the Hausdorff distance thanks to the corresponding GEOS function
 *  @example ST_HausdorffDistance {@link #hausdorffdistance} - SELECT ST_HausdorffDistance(
 *      'POLYGON((0 0, 0 2, 1 2, 2 2, 2 0, 0 0))'::geometry,
 *      'POLYGON((0.5 0.5, 0.5 2.5, 1.5 2.5, 2.5 2.5, 2.5 0.5, 0.5 0.5))'::geometry);
 */

PG_FUNCTION_INFO_V1(hausdorffdistance);
Datum hausdorffdistance(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom1;
	GSERIALIZED *geom2;
	GEOSGeometry *g1;
	GEOSGeometry *g2;
	double result;
	int retcode;

	geom1 = PG_GETARG_GSERIALIZED_P(0);
	geom2 = PG_GETARG_GSERIALIZED_P(1);

	if ( gserialized_is_empty(geom1) || gserialized_is_empty(geom2) )
		PG_RETURN_NULL();

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

	retcode = GEOSHausdorffDistance(g1, g2, &result);
	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (retcode == 0) HANDLE_GEOS_ERROR("GEOSHausdorffDistance");

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_FLOAT8(result);
}

/**
 *  @brief Compute the Hausdorff distance with densification thanks to the corresponding GEOS function
 *  @example hausdorffdistancedensify {@link #hausdorffdistancedensify} - SELECT ST_HausdorffDistance(
 *      'POLYGON((0 0, 0 2, 1 2, 2 2, 2 0, 0 0))'::geometry,
 *      'POLYGON((0.5 0.5, 0.5 2.5, 1.5 2.5, 2.5 2.5, 2.5 0.5, 0.5 0.5))'::geometry,
 *       0.5);
 */

PG_FUNCTION_INFO_V1(hausdorffdistancedensify);
Datum hausdorffdistancedensify(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom1;
	GSERIALIZED *geom2;
	GEOSGeometry *g1;
	GEOSGeometry *g2;
	double densifyFrac;
	double result;
	int retcode;

	geom1 = PG_GETARG_GSERIALIZED_P(0);
	geom2 = PG_GETARG_GSERIALIZED_P(1);
	densifyFrac = PG_GETARG_FLOAT8(2);

	if ( gserialized_is_empty(geom1) || gserialized_is_empty(geom2) )
		PG_RETURN_NULL();

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

	retcode = GEOSHausdorffDistanceDensify(g1, g2, densifyFrac, &result);
	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (retcode == 0) HANDLE_GEOS_ERROR("GEOSHausdorffDistanceDensify");

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_FLOAT8(result);
}

/**
 *  @brief Compute the Frechet distance with optional densification thanks to the corresponding GEOS function
 *  @example ST_FrechetDistance {@link #frechetdistance} - SELECT ST_FrechetDistance(
 *      'LINESTRING (0 0, 50 200, 100 0, 150 200, 200 0)'::geometry,
 *      'LINESTRING (0 200, 200 150, 0 100, 200 50, 0 0)'::geometry, 0.5);
 */

PG_FUNCTION_INFO_V1(ST_FrechetDistance);
Datum ST_FrechetDistance(PG_FUNCTION_ARGS)
{
#if POSTGIS_GEOS_VERSION < 37

	lwpgerror("The GEOS version this PostGIS binary "
					"was compiled against (%d) doesn't support "
					"'GEOSFechetDistance' function (3.7.0+ required)",
					POSTGIS_GEOS_VERSION);
	PG_RETURN_NULL();

#else /* POSTGIS_GEOS_VERSION >= 37 */
	GSERIALIZED *geom1;
	GSERIALIZED *geom2;
	GEOSGeometry *g1;
	GEOSGeometry *g2;
	double densifyFrac;
	double result;
	int retcode;

	geom1 = PG_GETARG_GSERIALIZED_P(0);
	geom2 = PG_GETARG_GSERIALIZED_P(1);
	densifyFrac = PG_GETARG_FLOAT8(2);

	if ( gserialized_is_empty(geom1) || gserialized_is_empty(geom2) )
		PG_RETURN_NULL();

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

	if (densifyFrac <= 0.0)
	{
		retcode = GEOSFrechetDistance(g1, g2, &result);
	}
	else
	{
		retcode = GEOSFrechetDistanceDensify(g1, g2, densifyFrac, &result);
	}

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (retcode == 0) HANDLE_GEOS_ERROR("GEOSFrechetDistance");

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_FLOAT8(result);

#endif /* POSTGIS_GEOS_VERSION >= 37 */
}


/**
 *  @brief Compute the Frechet distance with optional densification thanks to the corresponding GEOS function
 *  @example ST_FrechetDistance {@link #frechetdistance} - SELECT ST_FrechetDistance(
 *      'LINESTRING (0 0, 50 200, 100 0, 150 200, 200 0)'::geometry,
 *      'LINESTRING (0 200, 200 150, 0 100, 200 50, 0 0)'::geometry, 0.5);
 */

PG_FUNCTION_INFO_V1(ST_MaximumInscribedCircle);
Datum ST_MaximumInscribedCircle(PG_FUNCTION_ARGS)
{
#if POSTGIS_GEOS_VERSION < 39

	lwpgerror("The GEOS version this PostGIS binary "
	          "was compiled against (%d) doesn't support "
	          "'GEOSMaximumInscribedCircle' function (3.9.0+ required)",
	          POSTGIS_GEOS_VERSION);
	          PG_RETURN_NULL();

#else /* POSTGIS_GEOS_VERSION >= 39 */
	GSERIALIZED* geom;
	GSERIALIZED* center;
	GSERIALIZED* nearest;
	TupleDesc resultTupleDesc;
	HeapTuple resultTuple;
	Datum result;
	Datum result_values[3];
	bool result_is_null[3];
	double radius = 0.0;
	int32 srid = SRID_UNKNOWN;
	bool is3d;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	geom = PG_GETARG_GSERIALIZED_P(0);
	srid = gserialized_get_srid(geom);
	is3d = gserialized_has_z(geom);

    /* Empty geometry?  Return POINT EMPTY with zero radius */
	if (gserialized_is_empty(geom))
	{
		LWGEOM* lwcenter = (LWGEOM*) lwpoint_construct_empty(gserialized_get_srid(geom), LW_FALSE, LW_FALSE);
		LWGEOM* lwnearest = (LWGEOM*) lwpoint_construct_empty(gserialized_get_srid(geom), LW_FALSE, LW_FALSE);
		center = geometry_serialize(lwcenter);
		nearest = geometry_serialize(lwnearest);
		radius = 0.0;
	}
	else
	{
		GEOSGeometry *ginput, *gcircle, *gcenter, *gnearest;
		double width, height, size, tolerance;
		GBOX gbox;
		int gtype;

		if (!gserialized_get_gbox_p(geom, &gbox))
			PG_RETURN_NULL();

		width = gbox.xmax - gbox.xmin;
		height = gbox.ymax - gbox.ymin;
		size = width > height ? width : height;
		tolerance = size / 1000.0;

		initGEOS(lwpgnotice, lwgeom_geos_error);

		ginput = POSTGIS2GEOS(geom);
		if (!ginput)
			HANDLE_GEOS_ERROR("Geometry could not be converted to GEOS");

		gtype = gserialized_get_type(geom);
		if (gtype == POLYGONTYPE || gtype == MULTIPOLYGONTYPE)
		{
			gcircle = GEOSMaximumInscribedCircle(ginput, tolerance);
			if (!gcircle)
			{
				lwpgerror("Error calculating GEOSMaximumInscribedCircle.");
				GEOSGeom_destroy(ginput);
				PG_RETURN_NULL();
			}
		}
		else
		{
			gcircle = GEOSLargestEmptyCircle(ginput, NULL, tolerance);
			if (!gcircle)
			{
				lwpgerror("Error calculating GEOSLargestEmptyCircle.");
				GEOSGeom_destroy(ginput);
				PG_RETURN_NULL();
			}
		}

		gcenter = GEOSGeomGetStartPoint(gcircle);
		gnearest = GEOSGeomGetEndPoint(gcircle);
		GEOSDistance(gcenter, gnearest, &radius);
		GEOSSetSRID(gcenter, srid);
		GEOSSetSRID(gnearest, srid);

		center = GEOS2POSTGIS(gcenter, is3d);
		nearest = GEOS2POSTGIS(gnearest, is3d);
		GEOSGeom_destroy(gcenter);
		GEOSGeom_destroy(gnearest);
		GEOSGeom_destroy(gcircle);
		GEOSGeom_destroy(ginput);
	}

	get_call_result_type(fcinfo, NULL, &resultTupleDesc);
	BlessTupleDesc(resultTupleDesc);

	result_values[0] = PointerGetDatum(center);
	result_is_null[0] = false;
	result_values[1] = PointerGetDatum(nearest);
	result_is_null[1] = false;
	result_values[2] = Float8GetDatum(radius);
	result_is_null[2] = false;
	resultTuple = heap_form_tuple(resultTupleDesc, result_values, result_is_null);

	result = HeapTupleGetDatum(resultTuple);

	PG_RETURN_DATUM(result);

#endif /* POSTGIS_GEOS_VERSION >= 39 */
}


/**
 * @brief This is the final function for GeomUnion
 * 			aggregate. Will have as input an array of Geometries.
 * 			Will iteratively call GEOSUnion on the GEOS-converted
 * 			versions of them and return PGIS-converted version back.
 * 			Changing combination order *might* speed up performance.
 */
PG_FUNCTION_INFO_V1(pgis_union_geometry_array);
Datum pgis_union_geometry_array(PG_FUNCTION_ARGS)
{
	ArrayType *array;

	ArrayIterator iterator;
	Datum value;
	bool isnull;

	int is3d = LW_FALSE, gotsrid = LW_FALSE;
	int nelems = 0, geoms_size = 0, curgeom = 0, count = 0;

	GSERIALIZED *gser_out = NULL;

	GEOSGeometry *g = NULL;
	GEOSGeometry *g_union = NULL;
	GEOSGeometry **geoms = NULL;

	int32_t srid = SRID_UNKNOWN;

	int empty_type = 0;

	/* Null array, null geometry (should be empty?) */
	if ( PG_ARGISNULL(0) )
		PG_RETURN_NULL();

	array = PG_GETARG_ARRAYTYPE_P(0);
	nelems = ArrayGetNItems(ARR_NDIM(array), ARR_DIMS(array));

	/* Empty array? Null return */
	if ( nelems == 0 ) PG_RETURN_NULL();

	/* Quick scan for nulls */
	iterator = array_create_iterator(array, 0, NULL);
	while (array_iterate(iterator, &value, &isnull))
	{
		/* Skip null array items */
		if (isnull) continue;
		count++;
	}
	array_free_iterator(iterator);


	/* All-nulls? Return null */
	if ( count == 0 )
		PG_RETURN_NULL();

	/* One geom, good geom? Return it */
	if ( count == 1 && nelems == 1 )
	{
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif
		PG_RETURN_POINTER((GSERIALIZED *)(ARR_DATA_PTR(array)));
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
	}

	/* Ok, we really need GEOS now ;) */
	initGEOS(lwpgnotice, lwgeom_geos_error);

	/*
	** Collect the non-empty inputs and stuff them into a GEOS collection
	*/
	geoms_size = nelems;
	geoms = palloc(sizeof(GEOSGeometry*) * geoms_size);

	/*
	** We need to convert the array of GSERIALIZED into a GEOS collection.
	** First make an array of GEOS geometries.
	*/
	iterator = array_create_iterator(array, 0, NULL);
	while (array_iterate(iterator, &value, &isnull))
	{
		GSERIALIZED *gser_in;

		/* Skip null array items */
		if (isnull) continue;
		gser_in = (GSERIALIZED *)DatumGetPointer(value);

		/* Check for SRID mismatch in array elements */
		if ( gotsrid )
			gserialized_error_if_srid_mismatch_reference(gser_in, srid, __func__);
		else
		{
			/* Initialize SRID/dimensions info */
			srid = gserialized_get_srid(gser_in);
			is3d = gserialized_has_z(gser_in);
			gotsrid = 1;
		}

		/* Don't include empties in the union */
		if ( gserialized_is_empty(gser_in) )
		{
			int gser_type = gserialized_get_type(gser_in);
			if (gser_type > empty_type)
			{
				empty_type = gser_type;
				POSTGIS_DEBUGF(4, "empty_type = %d  gser_type = %d", empty_type, gser_type);
			}
		}
		else
		{
			g = POSTGIS2GEOS(gser_in);

			/* Uh oh! Exception thrown at construction... */
			if ( ! g )
			{
				HANDLE_GEOS_ERROR(
				    "One of the geometries in the set "
				    "could not be converted to GEOS");
			}

			/* Ensure we have enough space in our storage array */
			if ( curgeom == geoms_size )
			{
				geoms_size *= 2;
				geoms = repalloc( geoms, sizeof(GEOSGeometry*) * geoms_size );
			}

			geoms[curgeom] = g;
			curgeom++;
		}

	}
	array_free_iterator(iterator);

	/*
	** Take our GEOS geometries and turn them into a GEOS collection,
	** then pass that into cascaded union.
	*/
	if (curgeom > 0)
	{
		g = GEOSGeom_createCollection(GEOS_GEOMETRYCOLLECTION, geoms, curgeom);
		if (!g) HANDLE_GEOS_ERROR("Could not create GEOS COLLECTION from geometry array");

		g_union = GEOSUnaryUnion(g);
		GEOSGeom_destroy(g);
		if (!g_union) HANDLE_GEOS_ERROR("GEOSUnaryUnion");

		GEOSSetSRID(g_union, srid);
		gser_out = GEOS2POSTGIS(g_union, is3d);
		GEOSGeom_destroy(g_union);
	}
	/* No real geometries in our array, any empties? */
	else
	{
		/* If it was only empties, we'll return the largest type number */
		if ( empty_type > 0 )
		{
			PG_RETURN_POINTER(geometry_serialize(lwgeom_construct_empty(empty_type, srid, is3d, 0)));
		}
		/* Nothing but NULL, returns NULL */
		else
		{
			PG_RETURN_NULL();
		}
	}

	if ( ! gser_out )
	{
		/* Union returned a NULL geometry */
		PG_RETURN_NULL();
	}

	PG_RETURN_POINTER(gser_out);
}


PG_FUNCTION_INFO_V1(pgis_geometry_union_finalfn);
Datum pgis_geometry_union_finalfn(PG_FUNCTION_ARGS)
{
	CollectionBuildState *state;
	ListCell *l;
	LWGEOM **geoms;
	GSERIALIZED *gser_out;
	size_t ngeoms = 0;
	int empty_type = 0;
	bool first = true;
	int32_t srid = SRID_UNKNOWN;
	int has_z = LW_FALSE;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL(); /* returns null iff no input values */

	state = (CollectionBuildState *)PG_GETARG_POINTER(0);
	geoms = palloc(list_length(state->geoms) * sizeof(LWGEOM*));

	/* Read contents of list into an array of only non-null values */
	foreach (l, state->geoms)
	{
		LWGEOM *geom = (LWGEOM*)(lfirst(l));
		if (geom)
		{
			if (!lwgeom_is_empty(geom))
			{
				geoms[ngeoms++] = geom;
				if (first)
				{
					srid = lwgeom_get_srid(geom);
					has_z = lwgeom_has_z(geom);
					first = false;
				}
			}
			else
			{
				int type = lwgeom_get_type(geom);
				empty_type = type > empty_type ? type : empty_type;
			}
		}
	}

	/*
	** Take our array of LWGEOM* and turn it into a GEOS collection,
	** then pass that into cascaded union.
	*/
	if (ngeoms > 0)
	{
		LWCOLLECTION* col = lwcollection_construct(COLLECTIONTYPE, srid, NULL, ngeoms, geoms);
		LWGEOM *out = lwgeom_unaryunion_prec(lwcollection_as_lwgeom(col), state->gridSize);
		if ( ! out )
		{
			lwcollection_free(col);
		}
		gser_out = geometry_serialize(out);
	}
	/* No real geometries in our array, any empties? */
	else
	{
		/* If it was only empties, we'll return the largest type number */
		if (empty_type > 0)
			PG_RETURN_POINTER(
			    geometry_serialize(lwgeom_construct_empty(empty_type, srid, has_z, 0)));

		/* Nothing but NULL, returns NULL */
		else
			PG_RETURN_NULL();
	}

	if (!gser_out)
	{
		/* Union returned a NULL geometry */
		PG_RETURN_NULL();
	}

	PG_RETURN_POINTER(gser_out);
}



/**
 * @example ST_UnaryUnion {@link #geomunion} SELECT ST_UnaryUnion(
 *      'POLYGON((0 0, 10 0, 0 10, 10 10, 0 0))'
 * );
 *
 */
PG_FUNCTION_INFO_V1(ST_UnaryUnion);
Datum ST_UnaryUnion(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom1;
	GSERIALIZED *result;
	LWGEOM *lwgeom1, *lwresult ;
	double prec = -1;

	geom1 = PG_GETARG_GSERIALIZED_P(0);
	if (PG_NARGS() > 1 && ! PG_ARGISNULL(1))
		prec = PG_GETARG_FLOAT8(1);

	lwgeom1 = lwgeom_from_gserialized(geom1) ;

	lwresult = lwgeom_unaryunion_prec(lwgeom1, prec);
	result = geometry_serialize(lwresult) ;

	lwgeom_free(lwgeom1) ;
	lwgeom_free(lwresult) ;

	PG_FREE_IF_COPY(geom1, 0);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(ST_Union);
Datum ST_Union(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom1;
	GSERIALIZED *geom2;
	GSERIALIZED *result;
	LWGEOM *lwgeom1, *lwgeom2, *lwresult;
	double gridSize = -1;

	geom1 = PG_GETARG_GSERIALIZED_P(0);
	geom2 = PG_GETARG_GSERIALIZED_P(1);
	if (PG_NARGS() > 2 && ! PG_ARGISNULL(2))
		gridSize = PG_GETARG_FLOAT8(2);

	lwgeom1 = lwgeom_from_gserialized(geom1);
	lwgeom2 = lwgeom_from_gserialized(geom2);

	lwresult = lwgeom_union_prec(lwgeom1, lwgeom2, gridSize);
	result = geometry_serialize(lwresult);

	lwgeom_free(lwgeom1);
	lwgeom_free(lwgeom2);
	lwgeom_free(lwresult);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_POINTER(result);
}

/* This is retained for backward ABI compatibility
 * with PostGIS < 3.1.0 */
PG_FUNCTION_INFO_V1(symdifference);
Datum symdifference(PG_FUNCTION_ARGS)
{
  PG_RETURN_DATUM(DirectFunctionCall2(
     ST_SymDifference,
     PG_GETARG_DATUM(0), PG_GETARG_DATUM(1)
  ));
}

/**
 *  @example symdifference {@link #symdifference} - SELECT ST_SymDifference(
 *      'POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))',
 *      'POLYGON((5 5, 15 5, 15 7, 5 7, 5 5))');
 */
PG_FUNCTION_INFO_V1(ST_SymDifference);
Datum ST_SymDifference(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom1;
	GSERIALIZED *geom2;
	GSERIALIZED *result;
	LWGEOM *lwgeom1, *lwgeom2, *lwresult ;
	double prec = -1;

	geom1 = PG_GETARG_GSERIALIZED_P(0);
	geom2 = PG_GETARG_GSERIALIZED_P(1);
	if (PG_NARGS() > 2 && ! PG_ARGISNULL(2))
		prec = PG_GETARG_FLOAT8(2);

	lwgeom1 = lwgeom_from_gserialized(geom1) ;
	lwgeom2 = lwgeom_from_gserialized(geom2) ;

	lwresult = lwgeom_symdifference_prec(lwgeom1, lwgeom2, prec);
	result = geometry_serialize(lwresult) ;

	lwgeom_free(lwgeom1) ;
	lwgeom_free(lwgeom2) ;
	lwgeom_free(lwresult) ;

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_POINTER(result);
}


PG_FUNCTION_INFO_V1(boundary);
Datum boundary(PG_FUNCTION_ARGS)
{
	GSERIALIZED	*geom1;
	GEOSGeometry *g1, *g3;
	GSERIALIZED *result;
	LWGEOM *lwgeom;
	int32_t srid;

	geom1 = PG_GETARG_GSERIALIZED_P(0);

	/* Empty.Boundary() == Empty */
	if ( gserialized_is_empty(geom1) )
		PG_RETURN_POINTER(geom1);

	srid = gserialized_get_srid(geom1);

	lwgeom = lwgeom_from_gserialized(geom1);
	if ( ! lwgeom ) {
		lwpgerror("POSTGIS2GEOS: unable to deserialize input");
		PG_RETURN_NULL();
	}

	/* GEOS doesn't do triangle type, so we special case that here */
	if (lwgeom->type == TRIANGLETYPE)
	{
		lwgeom->type = LINETYPE;
		result = geometry_serialize(lwgeom);
		lwgeom_free(lwgeom);
		PG_RETURN_POINTER(result);
	}

	initGEOS(lwpgnotice, lwgeom_geos_error);

	g1 = LWGEOM2GEOS(lwgeom, 0);
	lwgeom_free(lwgeom);

	if (!g1)
		HANDLE_GEOS_ERROR("First argument geometry could not be converted to GEOS");

	g3 = GEOSBoundary(g1);

	if (!g3)
	{
		GEOSGeom_destroy(g1);
		HANDLE_GEOS_ERROR("GEOSBoundary");
	}

	POSTGIS_DEBUGF(3, "result: %s", GEOSGeomToWKT(g3));

	GEOSSetSRID(g3, srid);

	result = GEOS2POSTGIS(g3, gserialized_has_z(geom1));

	if (!result)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g3);
		elog(NOTICE,
		     "GEOS2POSTGIS threw an error (result postgis geometry "
		     "formation)!");
		PG_RETURN_NULL(); /* never get here */
	}

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g3);

	PG_FREE_IF_COPY(geom1, 0);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(convexhull);
Datum convexhull(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom1;
	GEOSGeometry *g1, *g3;
	GSERIALIZED *result;
	LWGEOM *lwout;
	int32_t srid;
	GBOX bbox;

	geom1 = PG_GETARG_GSERIALIZED_P(0);

	/* Empty.ConvexHull() == Empty */
	if ( gserialized_is_empty(geom1) )
		PG_RETURN_POINTER(geom1);

	srid = gserialized_get_srid(geom1);

	initGEOS(lwpgnotice, lwgeom_geos_error);

	g1 = POSTGIS2GEOS(geom1);

	if (!g1)
		HANDLE_GEOS_ERROR("First argument geometry could not be converted to GEOS");

	g3 = GEOSConvexHull(g1);
	GEOSGeom_destroy(g1);

	if (!g3) HANDLE_GEOS_ERROR("GEOSConvexHull");

	POSTGIS_DEBUGF(3, "result: %s", GEOSGeomToWKT(g3));

	GEOSSetSRID(g3, srid);

	lwout = GEOS2LWGEOM(g3, gserialized_has_z(geom1));
	GEOSGeom_destroy(g3);

	if (!lwout)
	{
		elog(ERROR,
		     "convexhull() failed to convert GEOS geometry to LWGEOM");
		PG_RETURN_NULL(); /* never get here */
	}

	/* Copy input bbox if any */
	if ( gserialized_get_gbox_p(geom1, &bbox) )
	{
		/* Force the box to have the same dimensionality as the lwgeom */
		bbox.flags = lwout->flags;
		lwout->bbox = gbox_copy(&bbox);
	}

	result = geometry_serialize(lwout);
	lwgeom_free(lwout);

	if (!result)
	{
		elog(ERROR,"GEOS convexhull() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /* never get here */
	}

	PG_FREE_IF_COPY(geom1, 0);
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(topologypreservesimplify);
Datum topologypreservesimplify(PG_FUNCTION_ARGS)
{
	GSERIALIZED	*geom1;
	double	tolerance;
	GEOSGeometry *g1, *g3;
	GSERIALIZED *result;
	uint32_t type;

	geom1 = PG_GETARG_GSERIALIZED_P(0);
	tolerance = PG_GETARG_FLOAT8(1);

	/* Empty.Simplify() == Empty */
	type = gserialized_get_type(geom1);
	if ( gserialized_is_empty(geom1) || type == TINTYPE || type == TRIANGLETYPE )
		PG_RETURN_POINTER(geom1);

	initGEOS(lwpgnotice, lwgeom_geos_error);

	g1 = POSTGIS2GEOS(geom1);
	if (!g1)
		HANDLE_GEOS_ERROR("First argument geometry could not be converted to GEOS");

	g3 = GEOSTopologyPreserveSimplify(g1,tolerance);
	GEOSGeom_destroy(g1);

	if (!g3) HANDLE_GEOS_ERROR("GEOSTopologyPreserveSimplify");

	POSTGIS_DEBUGF(3, "result: %s", GEOSGeomToWKT(g3));

	GEOSSetSRID(g3, gserialized_get_srid(geom1));

	result = GEOS2POSTGIS(g3, gserialized_has_z(geom1));
	GEOSGeom_destroy(g3);

	if (!result)
	{
		elog(ERROR,"GEOS topologypreservesimplify() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /* never get here */
	}

	PG_FREE_IF_COPY(geom1, 0);
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(buffer);
Datum buffer(PG_FUNCTION_ARGS)
{
	GEOSBufferParams *bufferparams;
	GEOSGeometry *g1, *g3 = NULL;
	GSERIALIZED *result;
	LWGEOM *lwg;
	int quadsegs = 8; /* the default */
	int singleside = 0; /* the default */
	enum
	{
	    ENDCAP_ROUND = 1,
	    ENDCAP_FLAT = 2,
	    ENDCAP_SQUARE = 3
	};
	enum
	{
	    JOIN_ROUND = 1,
	    JOIN_MITRE = 2,
	    JOIN_BEVEL = 3
	};
	const double DEFAULT_MITRE_LIMIT = 5.0;
	const int DEFAULT_ENDCAP_STYLE = ENDCAP_ROUND;
	const int DEFAULT_JOIN_STYLE = JOIN_ROUND;
	double mitreLimit = DEFAULT_MITRE_LIMIT;
	int endCapStyle = DEFAULT_ENDCAP_STYLE;
	int joinStyle  = DEFAULT_JOIN_STYLE;

	GSERIALIZED	*geom1 = PG_GETARG_GSERIALIZED_P(0);
	double size = PG_GETARG_FLOAT8(1);
	text *params_text;

	if (PG_NARGS() > 2)
	{
		params_text = PG_GETARG_TEXT_P(2);
	}
	else
	{
		params_text = palloc(VARHDRSZ);
		SET_VARSIZE(params_text, 0);
	}

	/* Empty.Buffer() == Empty[polygon] */
	if ( gserialized_is_empty(geom1) )
	{
		lwg = lwpoly_as_lwgeom(lwpoly_construct_empty(
		        gserialized_get_srid(geom1),
		        0, 0)); // buffer wouldn't give back z or m anyway
		PG_RETURN_POINTER(geometry_serialize(lwg));
	}

	initGEOS(lwpgnotice, lwgeom_geos_error);

	g1 = POSTGIS2GEOS(geom1);
	if (!g1)
		HANDLE_GEOS_ERROR("First argument geometry could not be converted to GEOS");


	if (VARSIZE_ANY_EXHDR(params_text) > 0)
	{
		char *param;
		char *params = text_to_cstring(params_text);

		for (param=params; ; param=NULL)
		{
			char *key, *val;
			param = strtok(param, " ");
			if (!param) break;
			POSTGIS_DEBUGF(3, "Param: %s", param);

			key = param;
			val = strchr(key, '=');
			if (!val || *(val + 1) == '\0')
			{
				lwpgerror("Missing value for buffer parameter %s", key);
				break;
			}
			*val = '\0';
			++val;

			POSTGIS_DEBUGF(3, "Param: %s : %s", key, val);

			if ( !strcmp(key, "endcap") )
			{
				/* Supported end cap styles:
				 *   "round", "flat", "square"
				 */
				if ( !strcmp(val, "round") )
				{
					endCapStyle = ENDCAP_ROUND;
				}
				else if ( !strcmp(val, "flat") ||
				          !strcmp(val, "butt")    )
				{
					endCapStyle = ENDCAP_FLAT;
				}
				else if ( !strcmp(val, "square") )
				{
					endCapStyle = ENDCAP_SQUARE;
				}
				else
				{
					lwpgerror("Invalid buffer end cap "
					        "style: %s (accept: "
					        "'round', 'flat', 'butt' "
					        "or 'square'"
					        ")", val);
					break;
				}

			}
			else if ( !strcmp(key, "join") )
			{
				if ( !strcmp(val, "round") )
				{
					joinStyle = JOIN_ROUND;
				}
				else if ( !strcmp(val, "mitre") ||
				          !strcmp(val, "miter")    )
				{
					joinStyle = JOIN_MITRE;
				}
				else if ( !strcmp(val, "bevel") )
				{
					joinStyle = JOIN_BEVEL;
				}
				else
				{
					lwpgerror("Invalid buffer end cap "
					        "style: %s (accept: "
					        "'round', 'mitre', 'miter' "
					        " or 'bevel'"
					        ")", val);
					break;
				}
			}
			else if ( !strcmp(key, "mitre_limit") ||
			          !strcmp(key, "miter_limit")    )
			{
				/* mitreLimit is a float */
				mitreLimit = atof(val);
			}
			else if ( !strcmp(key, "quad_segs") )
			{
				/* quadrant segments is an int */
				quadsegs = atoi(val);
			}
			else if ( !strcmp(key, "side") )
			{
				if ( !strcmp(val, "both") )
				{
					singleside = 0;
				}
				else if ( !strcmp(val, "left") )
				{
					singleside = 1;
				}
				else if ( !strcmp(val, "right") )
				{
					singleside = 1;
					size *= -1;
				}
				else
				{
					lwpgerror("Invalid side parameter: %s (accept: 'right', 'left', 'both')", val);
					break;
				}
			}
			else
			{
				lwpgerror(
				    "Invalid buffer parameter: %s (accept: 'endcap', 'join', 'mitre_limit', 'miter_limit', 'quad_segs' and 'side')",
				    key);
				break;
			}
		}
		pfree(params); /* was pstrduped */
	}


	POSTGIS_DEBUGF(3, "endCap:%d joinStyle:%d mitreLimit:%g",
	               endCapStyle, joinStyle, mitreLimit);

	bufferparams = GEOSBufferParams_create();
	if (bufferparams)
	{
		if (GEOSBufferParams_setEndCapStyle(bufferparams, endCapStyle) &&
			GEOSBufferParams_setJoinStyle(bufferparams, joinStyle) &&
			GEOSBufferParams_setMitreLimit(bufferparams, mitreLimit) &&
			GEOSBufferParams_setQuadrantSegments(bufferparams, quadsegs) &&
			GEOSBufferParams_setSingleSided(bufferparams, singleside))
		{
			g3 = GEOSBufferWithParams(g1, bufferparams, size);
		}
		else
		{
			lwpgerror("Error setting buffer parameters.");
		}
		GEOSBufferParams_destroy(bufferparams);
	}
	else
	{
		lwpgerror("Error setting buffer parameters.");
	}

	GEOSGeom_destroy(g1);

	if (!g3) HANDLE_GEOS_ERROR("GEOSBuffer");

	POSTGIS_DEBUGF(3, "result: %s", GEOSGeomToWKT(g3));

	GEOSSetSRID(g3, gserialized_get_srid(geom1));

	result = GEOS2POSTGIS(g3, gserialized_has_z(geom1));
	GEOSGeom_destroy(g3);

	if (!result)
	{
		elog(ERROR,"GEOS buffer() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /* never get here */
	}

	PG_FREE_IF_COPY(geom1, 0);
	PG_RETURN_POINTER(result);
}

/*
* Generate a field of random points within the area of a
* polygon or multipolygon. Throws an error for other geometry
* types.
*/
Datum ST_GeneratePoints(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_GeneratePoints);
Datum ST_GeneratePoints(PG_FUNCTION_ARGS)
{
	GSERIALIZED	*gser_input;
	GSERIALIZED *gser_result;
	LWGEOM *lwgeom_input;
	LWGEOM *lwgeom_result;
	int32 npoints;
	int32 seed = 0;

	gser_input = PG_GETARG_GSERIALIZED_P(0);
	npoints = PG_GETARG_INT32(1);

	if (npoints < 0)
		PG_RETURN_NULL();

	if (PG_NARGS() > 2 && ! PG_ARGISNULL(2))
	{
		seed = PG_GETARG_INT32(2);
		if (seed < 1)
		{
			lwpgerror("ST_GeneratePoints: seed must be greater than zero");
			PG_RETURN_NULL();
		}
	}

	/* Types get checked in the code, we'll keep things small out there */
	lwgeom_input = lwgeom_from_gserialized(gser_input);
	lwgeom_result = (LWGEOM*)lwgeom_to_points(lwgeom_input, npoints, seed);
	lwgeom_free(lwgeom_input);
	PG_FREE_IF_COPY(gser_input, 0);

	/* Return null as null */
	if (!lwgeom_result)
		PG_RETURN_NULL();

	/* Serialize and return */
	gser_result = geometry_serialize(lwgeom_result);
	lwgeom_free(lwgeom_result);
	PG_RETURN_POINTER(gser_result);
}


/*
* Compute at offset curve to a line
*/
Datum ST_OffsetCurve(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_OffsetCurve);
Datum ST_OffsetCurve(PG_FUNCTION_ARGS)
{
	GSERIALIZED	*gser_input;
	GSERIALIZED *gser_result;
	LWGEOM *lwgeom_input;
	LWGEOM *lwgeom_result;
	double size;
	int quadsegs = 8; /* the default */
	int nargs;

	enum
	{
		JOIN_ROUND = 1,
		JOIN_MITRE = 2,
		JOIN_BEVEL = 3
	};

	static const double DEFAULT_MITRE_LIMIT = 5.0;
	static const int DEFAULT_JOIN_STYLE = JOIN_ROUND;
	double mitreLimit = DEFAULT_MITRE_LIMIT;
	int joinStyle  = DEFAULT_JOIN_STYLE;
	char *param = NULL;
	char *paramstr = NULL;

	/* Read SQL arguments */
	nargs = PG_NARGS();
	gser_input = PG_GETARG_GSERIALIZED_P(0);
	size = PG_GETARG_FLOAT8(1);

	/* For distance == 0, just return the input. */
	if (size == 0) PG_RETURN_POINTER(gser_input);

	/* Read the lwgeom, check for errors */
	lwgeom_input = lwgeom_from_gserialized(gser_input);
	if ( ! lwgeom_input )
		lwpgerror("ST_OffsetCurve: lwgeom_from_gserialized returned NULL");

	/* For empty inputs, just echo them back */
	if ( lwgeom_is_empty(lwgeom_input) )
		PG_RETURN_POINTER(gser_input);

	/* Process the optional arguments */
	if ( nargs > 2 )
	{
		text *wkttext = PG_GETARG_TEXT_P(2);
		paramstr = text_to_cstring(wkttext);

		POSTGIS_DEBUGF(3, "paramstr: %s", paramstr);

		for ( param=paramstr; ; param=NULL )
		{
			char *key, *val;
			param = strtok(param, " ");
			if (!param) break;
			POSTGIS_DEBUGF(3, "Param: %s", param);

			key = param;
			val = strchr(key, '=');
			if (!val || *(val + 1) == '\0')
			{
				lwpgerror("ST_OffsetCurve: Missing value for buffer parameter %s", key);
				break;
			}
			*val = '\0';
			++val;

			POSTGIS_DEBUGF(3, "Param: %s : %s", key, val);

			if ( !strcmp(key, "join") )
			{
				if ( !strcmp(val, "round") )
				{
					joinStyle = JOIN_ROUND;
				}
				else if ( !(strcmp(val, "mitre") && strcmp(val, "miter")) )
				{
					joinStyle = JOIN_MITRE;
				}
				else if ( ! strcmp(val, "bevel") )
				{
					joinStyle = JOIN_BEVEL;
				}
				else
				{
					lwpgerror(
					    "Invalid buffer end cap style: %s (accept: 'round', 'mitre', 'miter' or 'bevel')",
					    val);
					break;
				}
			}
			else if ( !strcmp(key, "mitre_limit") ||
			          !strcmp(key, "miter_limit")    )
			{
				/* mitreLimit is a float */
				mitreLimit = atof(val);
			}
			else if ( !strcmp(key, "quad_segs") )
			{
				/* quadrant segments is an int */
				quadsegs = atoi(val);
			}
			else
			{
				lwpgerror(
				    "Invalid buffer parameter: %s (accept: 'join', 'mitre_limit', 'miter_limit and 'quad_segs')",
				    key);
				break;
			}
		}
		POSTGIS_DEBUGF(3, "joinStyle:%d mitreLimit:%g", joinStyle, mitreLimit);
		pfree(paramstr); /* alloc'ed in text_to_cstring */
	}

	lwgeom_result = lwgeom_offsetcurve(lwgeom_input, size, quadsegs, joinStyle, mitreLimit);

	if (!lwgeom_result)
		lwpgerror("ST_OffsetCurve: lwgeom_offsetcurve returned NULL");

	gser_result = geometry_serialize(lwgeom_result);
	lwgeom_free(lwgeom_input);
	lwgeom_free(lwgeom_result);
	PG_RETURN_POINTER(gser_result);
}

PG_FUNCTION_INFO_V1(ST_Intersection);
Datum ST_Intersection(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom1;
	GSERIALIZED *geom2;
	GSERIALIZED *result;
	LWGEOM *lwgeom1, *lwgeom2, *lwresult;
	double prec = -1;

	geom1 = PG_GETARG_GSERIALIZED_P(0);
	geom2 = PG_GETARG_GSERIALIZED_P(1);
	if (PG_NARGS() > 2 && ! PG_ARGISNULL(2))
		prec = PG_GETARG_FLOAT8(2);

	lwgeom1 = lwgeom_from_gserialized(geom1);
	lwgeom2 = lwgeom_from_gserialized(geom2);

	lwresult = lwgeom_intersection_prec(lwgeom1, lwgeom2, prec);
	result = geometry_serialize(lwresult);

	lwgeom_free(lwgeom1);
	lwgeom_free(lwgeom2);
	lwgeom_free(lwresult);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(ST_Difference);
Datum ST_Difference(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom1;
	GSERIALIZED *geom2;
	GSERIALIZED *result;
	LWGEOM *lwgeom1, *lwgeom2, *lwresult;
	double prec = -1;

	geom1 = PG_GETARG_GSERIALIZED_P(0);
	geom2 = PG_GETARG_GSERIALIZED_P(1);
	if (PG_NARGS() > 2 && ! PG_ARGISNULL(2))
		prec = PG_GETARG_FLOAT8(2);

	lwgeom1 = lwgeom_from_gserialized(geom1);
	lwgeom2 = lwgeom_from_gserialized(geom2);

	lwresult = lwgeom_difference_prec(lwgeom1, lwgeom2, prec);
	result = geometry_serialize(lwresult);

	lwgeom_free(lwgeom1);
	lwgeom_free(lwgeom2);
	lwgeom_free(lwresult);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_POINTER(result);
}

/**
	@example pointonsurface - {@link #pointonsurface} SELECT ST_PointOnSurface('POLYGON((0 0, 10 0, 10 10, 0 10, 0
   0))');
*/
PG_FUNCTION_INFO_V1(pointonsurface);
Datum pointonsurface(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom, *result;
	LWGEOM *lwgeom, *lwresult;

	geom = PG_GETARG_GSERIALIZED_P(0);

	lwgeom = lwgeom_from_gserialized(geom);
	lwresult = lwgeom_pointonsurface(lwgeom);
	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 0);

	if (!lwresult) PG_RETURN_NULL();

	result = geometry_serialize(lwresult);
	lwgeom_free(lwresult);
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(centroid);
Datum centroid(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom, *result;
	LWGEOM *lwgeom, *lwresult;

	geom = PG_GETARG_GSERIALIZED_P(0);

	lwgeom = lwgeom_from_gserialized(geom);
	lwresult = lwgeom_centroid(lwgeom);
	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 0);

	if (!lwresult) PG_RETURN_NULL();

	result = geometry_serialize(lwresult);
	lwgeom_free(lwresult);
	PG_RETURN_POINTER(result);
}

Datum ST_ReducePrecision(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_ReducePrecision);
Datum ST_ReducePrecision(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom, *result;
	LWGEOM *lwgeom, *lwresult;
	double gridSize = PG_GETARG_FLOAT8(1);
	geom = PG_GETARG_GSERIALIZED_P(0);

	lwgeom = lwgeom_from_gserialized(geom);
	lwresult = lwgeom_reduceprecision(lwgeom, gridSize);
	lwgeom_free(lwgeom);
	PG_FREE_IF_COPY(geom, 0);

	if (!lwresult) PG_RETURN_NULL();

	result = geometry_serialize(lwresult);
	lwgeom_free(lwresult);
	PG_RETURN_POINTER(result);
}

Datum ST_ClipByBox2d(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_ClipByBox2d);
Datum ST_ClipByBox2d(PG_FUNCTION_ARGS)
{
	static const uint32_t geom_idx = 0;
	static const uint32_t box2d_idx = 1;
	GSERIALIZED *result;
	LWGEOM *lwgeom1, *lwresult ;
	GBOX bbox1 = {0};
	GBOX *bbox2;
	uint8_t type;
	int32_t srid;
	lwflags_t flags;

	if (!gserialized_datum_get_internals_p(PG_GETARG_DATUM(geom_idx), &bbox1, &flags, &type, &srid))
	{
		/* empty clips to empty, no matter rect */
		PG_RETURN_DATUM(PG_GETARG_DATUM(geom_idx));
	}

	/* WARNING: this is really a BOX2DF, use only xmin and ymin fields */
	bbox2 = (GBOX *)PG_GETARG_POINTER(box2d_idx);
	bbox2->flags = 0;

	/* if bbox1 is covered by bbox2, return lwgeom1 */
	if (gbox_contains_2d(bbox2, &bbox1))
	{
		PG_RETURN_DATUM(PG_GETARG_DATUM(geom_idx));
	}

	/* If bbox1 outside of bbox2, return empty */
	if (!gbox_overlaps_2d(&bbox1, bbox2))
	{
		/* Get type and srid from datum */
		lwresult = lwgeom_construct_empty(type, srid, 0, 0);
		result = geometry_serialize(lwresult) ;
		lwgeom_free(lwresult) ;
		PG_RETURN_POINTER(result);
	}

	lwgeom1 = lwgeom_from_gserialized(PG_GETARG_GSERIALIZED_P(geom_idx));
	lwresult = lwgeom_clip_by_rect(lwgeom1, bbox2->xmin, bbox2->ymin,
	                               bbox2->xmax, bbox2->ymax);

	lwgeom_free(lwgeom1);

	if (!lwresult)
		PG_RETURN_NULL();

	result = geometry_serialize(lwresult) ;
	PG_RETURN_POINTER(result);
}


/*---------------------------------------------*/

PG_FUNCTION_INFO_V1(isvalid);
Datum isvalid(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom1;
	LWGEOM *lwgeom;
	char result;
	GEOSGeom g1;

	geom1 = PG_GETARG_GSERIALIZED_P(0);

	/* Empty.IsValid() == TRUE */
	if ( gserialized_is_empty(geom1) )
		PG_RETURN_BOOL(true);

	initGEOS(lwpgnotice, lwgeom_geos_error);

	lwgeom = lwgeom_from_gserialized(geom1);
	if ( ! lwgeom )
	{
		lwpgerror("unable to deserialize input");
	}
	g1 = LWGEOM2GEOS(lwgeom, 0);
	lwgeom_free(lwgeom);

	if ( ! g1 )
	{
		/* should we drop the following
		 * notice now that we have ST_isValidReason ?
		 */
		lwpgnotice("%s", lwgeom_geos_errmsg);
		PG_RETURN_BOOL(false);
	}

	result = GEOSisValid(g1);
	GEOSGeom_destroy(g1);

	if (result == 2)
	{
		elog(ERROR,"GEOS isvalid() threw an error!");
		PG_RETURN_NULL(); /*never get here */
	}

	PG_FREE_IF_COPY(geom1, 0);
	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(isvalidreason);
Datum isvalidreason(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = NULL;
	char *reason_str = NULL;
	text *result = NULL;
	const GEOSGeometry *g1 = NULL;

	geom = PG_GETARG_GSERIALIZED_P(0);

	initGEOS(lwpgnotice, lwgeom_geos_error);

	g1 = POSTGIS2GEOS(geom);
	if ( g1 )
	{
		reason_str = GEOSisValidReason(g1);
		GEOSGeom_destroy((GEOSGeometry *)g1);
		if (!reason_str) HANDLE_GEOS_ERROR("GEOSisValidReason");
		result = cstring_to_text(reason_str);
		GEOSFree(reason_str);
	}
	else
	{
		result = cstring_to_text(lwgeom_geos_errmsg);
	}

	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(isvaliddetail);
Datum isvaliddetail(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom = NULL;
	const GEOSGeometry *g1 = NULL;
	char *values[3]; /* valid bool, reason text, location geometry */
	char *geos_reason = NULL;
	char *reason = NULL;
	GEOSGeometry *geos_location = NULL;
	LWGEOM *location = NULL;
	char valid = 0;
	HeapTupleHeader result;
	TupleDesc tupdesc;
	HeapTuple tuple;
	AttInMetadata *attinmeta;
	int flags = 0;

	/*
	 * Build a tuple description for a
	 * valid_detail tuple
	 */
	get_call_result_type(fcinfo, 0, &tupdesc);
	BlessTupleDesc(tupdesc);

	/*
	 * generate attribute metadata needed later to produce
	 * tuples from raw C strings
	 */
	attinmeta = TupleDescGetAttInMetadata(tupdesc);

	geom = PG_GETARG_GSERIALIZED_P(0);
	flags = PG_GETARG_INT32(1);

	initGEOS(lwpgnotice, lwgeom_geos_error);

	g1 = POSTGIS2GEOS(geom);

	if ( g1 )
	{
		valid = GEOSisValidDetail(g1, flags, &geos_reason, &geos_location);
		GEOSGeom_destroy((GEOSGeometry *)g1);
		if ( geos_reason )
		{
			reason = pstrdup(geos_reason);
			GEOSFree(geos_reason);
		}
		if ( geos_location )
		{
			location = GEOS2LWGEOM(geos_location, GEOSHasZ(geos_location));
			GEOSGeom_destroy(geos_location);
		}

		if (valid == 2)
		{
			/* NOTE: should only happen on OOM or similar */
			lwpgerror("GEOS isvaliddetail() threw an exception!");
			PG_RETURN_NULL(); /* never gets here */
		}
	}
	else
	{
		/* TODO: check lwgeom_geos_errmsg for validity error */
		reason = pstrdup(lwgeom_geos_errmsg);
	}

	/* the boolean validity */
	values[0] =  valid ? "t" : "f";

	/* the reason */
	values[1] =  reason;

	/* the location */
	values[2] = location ? lwgeom_to_hexwkb_buffer(location, WKB_EXTENDED) : 0;

	tuple = BuildTupleFromCStrings(attinmeta, values);
	result = (HeapTupleHeader) palloc(tuple->t_len);
	memcpy(result, tuple->t_data, tuple->t_len);
	heap_freetuple(tuple);

	PG_RETURN_HEAPTUPLEHEADER(result);
}

/**
 * overlaps(GSERIALIZED g1,GSERIALIZED g2)
 * @param g1
 * @param g2
 * @return  if GEOS::g1->overlaps(g2) returns true
 * @throw an error (elog(ERROR,...)) if GEOS throws an error
 */
PG_FUNCTION_INFO_V1(overlaps);
Datum overlaps(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom1;
	GSERIALIZED *geom2;
	GEOSGeometry *g1, *g2;
	char result;
	GBOX box1, box2;

	geom1 = PG_GETARG_GSERIALIZED_P(0);
	geom2 = PG_GETARG_GSERIALIZED_P(1);
	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	/* A.Overlaps(Empty) == FALSE */
	if ( gserialized_is_empty(geom1) || gserialized_is_empty(geom2) )
		PG_RETURN_BOOL(false);

	/*
	 * short-circuit 1: if geom2 bounding box does not overlap
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

	g1 = POSTGIS2GEOS(geom1);
	if (!g1)
		HANDLE_GEOS_ERROR("First argument geometry could not be converted to GEOS");

	g2 = POSTGIS2GEOS(geom2);

	if (!g2)
	{
		GEOSGeom_destroy(g1);
		HANDLE_GEOS_ERROR("Second argument geometry could not be converted to GEOS");
	}

	result = GEOSOverlaps(g1,g2);

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	if (result == 2) HANDLE_GEOS_ERROR("GEOSOverlaps");

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}


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

	POSTGIS_DEBUG(3, "contains called.");

	/*
	** short-circuit 1: if geom2 bounding box is not completely inside
	** geom1 bounding box we can return FALSE.
	*/
	if (gserialized_get_gbox_p(geom1, &box1) &&
	    gserialized_get_gbox_p(geom2, &box2))
	{
		if (!gbox_contains_2d(&box1, &box2))
			PG_RETURN_BOOL(false);
	}

	/*
	** short-circuit 2: if geom2 is a point and geom1 is a polygon
	** call the point-in-polygon function.
	*/
	if (is_poly(geom1) && is_point(geom2))
	{
		SHARED_GSERIALIZED *shared_gpoly = is_poly(geom1) ? shared_geom1 : shared_geom2;
		SHARED_GSERIALIZED *shared_gpoint = is_point(geom1) ? shared_geom1 : shared_geom2;
		const GSERIALIZED *gpoly = shared_gserialized_get(shared_gpoly);
		const GSERIALIZED *gpoint = shared_gserialized_get(shared_gpoint);
		RTREE_POLY_CACHE *cache = GetRtreeCache(fcinfo, shared_gpoly);
		int retval;

		POSTGIS_DEBUG(3, "Point in Polygon test requested...short-circuiting.");
		if (gserialized_get_type(gpoint) == POINTTYPE)
		{
			LWGEOM* point = lwgeom_from_gserialized(gpoint);
			int pip_result = pip_short_circuit(cache, lwgeom_as_lwpoint(point), gpoly);
			lwgeom_free(point);

			retval = (pip_result == 1); /* completely inside */
		}
		else if (gserialized_get_type(gpoint) == MULTIPOINTTYPE)
		{
			LWMPOINT* mpoint = lwgeom_as_lwmpoint(lwgeom_from_gserialized(gpoint));
			uint32_t i;
			int found_completely_inside = LW_FALSE;

			retval = LW_TRUE;
			for (i = 0; i < mpoint->ngeoms; i++)
			{
				/* We need to find at least one point that's completely inside the
				 * polygons (pip_result == 1).  As long as we have one point that's
				 * completely inside, we can have as many as we want on the boundary
				 * itself. (pip_result == 0)
				 */
				int pip_result = pip_short_circuit(cache, mpoint->geoms[i], gpoly);
				if (pip_result == 1)
					found_completely_inside = LW_TRUE;

				if (pip_result == -1) /* completely outside */
				{
					retval = LW_FALSE;
					break;
				}
			}

			retval = retval && found_completely_inside;
			lwmpoint_free(mpoint);
		}
		else
		{
			/* Never get here */
			elog(ERROR,"Type isn't point or multipoint!");
			PG_RETURN_BOOL(false);
		}

		return retval > 0;
	}
	else
	{
		POSTGIS_DEBUGF(3, "Contains: type1: %d, type2: %d", gserialized_get_type(geom1), gserialized_get_type(geom2));
	}

	initGEOS(lwpgnotice, lwgeom_geos_error);

	prep_cache = GetPrepGeomCache(fcinfo, shared_geom1, NULL);

	if ( prep_cache && prep_cache->prepared_geom && prep_cache->gcache.argnum == 1 )
	{
		g1 = POSTGIS2GEOS(geom2);
		if (!g1)
			HANDLE_GEOS_ERROR("Geometry could not be converted to GEOS");

		POSTGIS_DEBUG(4, "containsPrepared: cache is live, running preparedcontains");
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
		POSTGIS_DEBUG(4, "containsPrepared: cache is not ready, running standard contains");
		result = GEOSContains( g1, g2);
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
	}

	if (result == 2) HANDLE_GEOS_ERROR("GEOSContains");

	PG_RETURN_BOOL(result > 0);
}


PG_FUNCTION_INFO_V1(within);
Datum within(PG_FUNCTION_ARGS)
{
	PG_RETURN_DATUM(CallerFInfoFunctionCall2(contains, fcinfo->flinfo, InvalidOid,
		PG_GETARG_DATUM(1), PG_GETARG_DATUM(0)));
}



PG_FUNCTION_INFO_V1(containsproperly);
Datum containsproperly(PG_FUNCTION_ARGS)
{
	SHARED_GSERIALIZED *shared_geom1 = ToastCacheGetGeometry(fcinfo, 0);
	SHARED_GSERIALIZED *shared_geom2 = ToastCacheGetGeometry(fcinfo, 1);
	const GSERIALIZED *geom1 = shared_gserialized_get(shared_geom1);
	const GSERIALIZED *geom2 = shared_gserialized_get(shared_geom2);
	char 					result;
	GBOX 			box1, box2;
	PrepGeomCache *	prep_cache;

	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	/* A.ContainsProperly(Empty) == FALSE */
	if ( gserialized_is_empty(geom1) || gserialized_is_empty(geom2) )
		PG_RETURN_BOOL(false);

	/*
	 * short-circuit: if geom2 bounding box is not completely inside
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
		GEOSGeometry *g2;
		GEOSGeometry *g1;

		g1 = POSTGIS2GEOS(geom1);
		if (!g1) HANDLE_GEOS_ERROR("First argument geometry could not be converted to GEOS");
		g2 = POSTGIS2GEOS(geom2);
		if (!g2)
		{
			GEOSGeom_destroy(g1);
			HANDLE_GEOS_ERROR("Second argument geometry could not be converted to GEOS");
		}
		result = GEOSRelatePattern( g1, g2, "T**FF*FF*" );
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
	}

	if (result == 2) HANDLE_GEOS_ERROR("GEOSContains");

	PG_RETURN_BOOL(result);
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
		SHARED_GSERIALIZED *shared_gpoly = is_poly(geom1) ? shared_geom1 : shared_geom2;
		SHARED_GSERIALIZED *shared_gpoint = is_point(geom1) ? shared_geom1 : shared_geom2;
		const GSERIALIZED *gpoly = shared_gserialized_get(shared_gpoly);
		const GSERIALIZED *gpoint = shared_gserialized_get(shared_gpoint);
		RTREE_POLY_CACHE *cache = GetRtreeCache(fcinfo, shared_gpoly);
		int retval;

		POSTGIS_DEBUG(3, "Point in Polygon test requested...short-circuiting.");
		if (gserialized_get_type(gpoint) == POINTTYPE)
		{
			LWGEOM* point = lwgeom_from_gserialized(gpoint);
			int pip_result = pip_short_circuit(cache, lwgeom_as_lwpoint(point), gpoly);
			lwgeom_free(point);

			retval = (pip_result != -1); /* not outside */
		}
		else if (gserialized_get_type(gpoint) == MULTIPOINTTYPE)
		{
			LWMPOINT* mpoint = lwgeom_as_lwmpoint(lwgeom_from_gserialized(gpoint));
			uint32_t i;

			retval = LW_TRUE;
			for (i = 0; i < mpoint->ngeoms; i++)
			{
				int pip_result = pip_short_circuit(cache, mpoint->geoms[i], gpoly);
				if (pip_result == -1)
				{
					retval = LW_FALSE;
					break;
				}
			}

			lwmpoint_free(mpoint);
		}
		else
		{
			/* Never get here */
			elog(ERROR,"Type isn't point or multipoint!");
			PG_RETURN_NULL();
		}

		PG_RETURN_BOOL(retval);
	}
	else
	{
		POSTGIS_DEBUGF(3, "Covers: type1: %d, type2: %d", gserialized_get_type(geom1), gserialized_get_type(geom2));
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
		SHARED_GSERIALIZED *shared_gpoly = is_poly(geom1) ? shared_geom1 : shared_geom2;
		SHARED_GSERIALIZED *shared_gpoint = is_point(geom1) ? shared_geom1 : shared_geom2;
		const GSERIALIZED *gpoly = shared_gserialized_get(shared_gpoly);
		const GSERIALIZED *gpoint = shared_gserialized_get(shared_gpoint);
		RTREE_POLY_CACHE *cache = GetRtreeCache(fcinfo, shared_gpoly);
		int retval;

		POSTGIS_DEBUG(3, "Point in Polygon test requested...short-circuiting.");
		if (gserialized_get_type(gpoint) == POINTTYPE)
		{
			LWGEOM* point = lwgeom_from_gserialized(gpoint);
			int pip_result = pip_short_circuit(cache, lwgeom_as_lwpoint(point), gpoly);
			lwgeom_free(point);

			retval = (pip_result != -1); /* not outside */
		}
		else if (gserialized_get_type(gpoint) == MULTIPOINTTYPE)
		{
			LWMPOINT* mpoint = lwgeom_as_lwmpoint(lwgeom_from_gserialized(gpoint));
			uint32_t i;

			retval = LW_TRUE;
			for (i = 0; i < mpoint->ngeoms; i++)
			{
				int pip_result = pip_short_circuit(cache, mpoint->geoms[i], gpoly);
				if (pip_result == -1)
				{
					retval = LW_FALSE;
					break;
				}
			}

			lwmpoint_free(mpoint);
		}
		else
		{
			/* Never get here */
			elog(ERROR,"Type isn't point or multipoint!");
			PG_RETURN_NULL();
		}

		PG_RETURN_BOOL(retval);
	}
	else
	{
		POSTGIS_DEBUGF(3, "CoveredBy: type1: %d, type2: %d", gserialized_get_type(geom1), gserialized_get_type(geom2));
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
	GSERIALIZED *geom1;
	GSERIALIZED *geom2;
	GEOSGeometry *g1, *g2;
	int result;
	GBOX box1, box2;

	geom1 = PG_GETARG_GSERIALIZED_P(0);
	geom2 = PG_GETARG_GSERIALIZED_P(1);
	gserialized_error_if_srid_mismatch(geom1, geom2, __func__);

	/* A.Crosses(Empty) == FALSE */
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

	g1 = POSTGIS2GEOS(geom1);
	if (!g1)
		HANDLE_GEOS_ERROR("First argument geometry could not be converted to GEOS");

	g2 = POSTGIS2GEOS(geom2);
	if (!g2)
	{
		GEOSGeom_destroy(g1);
		HANDLE_GEOS_ERROR("Second argument geometry could not be converted to GEOS");
	}

	result = GEOSCrosses(g1,g2);

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (result == 2) HANDLE_GEOS_ERROR("GEOSCrosses");

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
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
	 * call the point_outside_polygon function.
	 */
	if ((is_point(geom1) && is_poly(geom2)) || (is_poly(geom1) && is_point(geom2)))
	{
		SHARED_GSERIALIZED *shared_gpoly = is_poly(geom1) ? shared_geom1 : shared_geom2;
		SHARED_GSERIALIZED *shared_gpoint = is_point(geom1) ? shared_geom1 : shared_geom2;
		const GSERIALIZED *gpoly = shared_gserialized_get(shared_gpoly);
		const GSERIALIZED *gpoint = shared_gserialized_get(shared_gpoint);
		RTREE_POLY_CACHE *cache = GetRtreeCache(fcinfo, shared_gpoly);
		int retval;

		POSTGIS_DEBUG(3, "Point in Polygon test requested...short-circuiting.");
		if (gserialized_get_type(gpoint) == POINTTYPE)
		{
			LWGEOM* point = lwgeom_from_gserialized(gpoint);
			int pip_result = pip_short_circuit(cache, lwgeom_as_lwpoint(point), gpoly);
			lwgeom_free(point);

			retval = (pip_result != -1); /* not outside */
		}
		else if (gserialized_get_type(gpoint) == MULTIPOINTTYPE)
		{
			LWMPOINT* mpoint = lwgeom_as_lwmpoint(lwgeom_from_gserialized(gpoint));
			uint32_t i;

			retval = LW_FALSE;
			for (i = 0; i < mpoint->ngeoms; i++)
			{
				int pip_result = pip_short_circuit(cache, mpoint->geoms[i], gpoly);
				if (pip_result != -1) /* not outside */
				{
					retval = LW_TRUE;
					break;
				}
			}

			lwmpoint_free(mpoint);
		}
		else
		{
			/* Never get here */
			elog(ERROR,"Type isn't point or multipoint!");
			PG_RETURN_NULL();
		}

		PG_RETURN_BOOL(retval);
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

	patt =  DatumGetCString(DirectFunctionCall1(textout,
	                        PointerGetDatum(PG_GETARG_DATUM(2))));

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

PG_FUNCTION_INFO_V1(issimple);
Datum issimple(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom;
	LWGEOM *lwgeom_in;
	int result;

	POSTGIS_DEBUG(2, "issimple called");

	geom = PG_GETARG_GSERIALIZED_P(0);

	if ( gserialized_is_empty(geom) )
		PG_RETURN_BOOL(true);

	lwgeom_in = lwgeom_from_gserialized(geom);
	result = lwgeom_is_simple(lwgeom_in);
	lwgeom_free(lwgeom_in) ;
	PG_FREE_IF_COPY(geom, 0);

	if (result == -1) {
		PG_RETURN_NULL(); /*never get here */
	}

	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(isring);
Datum isring(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom;
	GEOSGeometry *g1;
	int result;

	geom = PG_GETARG_GSERIALIZED_P(0);

	/* Empty things can't close */
	if ( gserialized_is_empty(geom) )
		PG_RETURN_BOOL(false);

	initGEOS(lwpgnotice, lwgeom_geos_error);

	g1 = POSTGIS2GEOS(geom);
	if (!g1)
		HANDLE_GEOS_ERROR("First argument geometry could not be converted to GEOS");

	if ( GEOSGeomTypeId(g1) != GEOS_LINESTRING )
	{
		GEOSGeom_destroy(g1);
		elog(ERROR, "ST_IsRing() should only be called on a linear feature");
	}

	result = GEOSisRing(g1);
	GEOSGeom_destroy(g1);

	if (result == 2) HANDLE_GEOS_ERROR("GEOSisRing");

	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_BOOL(result);
}

GSERIALIZED *
GEOS2POSTGIS(GEOSGeom geom, char want3d)
{
	LWGEOM *lwgeom;
	GSERIALIZED *result;

	lwgeom = GEOS2LWGEOM(geom, want3d);
	if ( ! lwgeom )
	{
		lwpgerror("%s: GEOS2LWGEOM returned NULL", __func__);
		return NULL;
	}

	POSTGIS_DEBUGF(4, "%s: GEOS2LWGEOM returned a %s", __func__, lwgeom_summary(lwgeom, 0));

	if (lwgeom_needs_bbox(lwgeom)) lwgeom_add_bbox(lwgeom);

	result = geometry_serialize(lwgeom);
	lwgeom_free(lwgeom);

	return result;
}

/*-----=POSTGIS2GEOS= */

GEOSGeometry *
POSTGIS2GEOS(const GSERIALIZED *pglwgeom)
{
	GEOSGeometry *ret;
	LWGEOM *lwgeom = lwgeom_from_gserialized(pglwgeom);
	if ( ! lwgeom )
	{
		lwpgerror("POSTGIS2GEOS: unable to deserialize input");
		return NULL;
	}
	ret = LWGEOM2GEOS(lwgeom, 0);
	lwgeom_free(lwgeom);

	return ret;
}

uint32_t array_nelems_not_null(ArrayType* array) {
    ArrayIterator iterator;
    Datum value;
    bool isnull;
    uint32_t nelems_not_null = 0;
	iterator = array_create_iterator(array, 0, NULL);
	while(array_iterate(iterator, &value, &isnull) )
        if (!isnull)
            nelems_not_null++;

    array_free_iterator(iterator);

    return nelems_not_null;
}

/* ARRAY2LWGEOM: Converts the non-null elements of a Postgres array into a LWGEOM* array */
LWGEOM** ARRAY2LWGEOM(ArrayType* array, uint32_t nelems,  int* is3d, int* srid)
{
    ArrayIterator iterator;
    Datum value;
    bool isnull;
    bool gotsrid = false;
    uint32_t i = 0;

	LWGEOM** lw_geoms = palloc(nelems * sizeof(LWGEOM*));

    iterator = array_create_iterator(array, 0, NULL);

    while (array_iterate(iterator, &value, &isnull))
    {
	    GSERIALIZED *geom = (GSERIALIZED *)DatumGetPointer(value);

	    if (isnull)
		    continue;

	    *is3d = *is3d || gserialized_has_z(geom);

	    lw_geoms[i] = lwgeom_from_gserialized(geom);
	    if (!lw_geoms[i]) /* error in creation */
	    {
		    lwpgerror("Geometry deserializing geometry");
		    return NULL;
	    }
	    if (!gotsrid)
	    {
		    gotsrid = true;
		    *srid = gserialized_get_srid(geom);
	    }
	    else
		    gserialized_error_if_srid_mismatch_reference(geom, *srid, __func__);

	    i++;
    }

	return lw_geoms;
}

/* ARRAY2GEOS: Converts the non-null elements of a Postgres array into a GEOSGeometry* array */
GEOSGeometry** ARRAY2GEOS(ArrayType* array, uint32_t nelems, int* is3d, int* srid)
{
    ArrayIterator iterator;
    Datum value;
    bool isnull;
    bool gotsrid = false;
    uint32_t i = 0;

	GEOSGeometry** geos_geoms = palloc(nelems * sizeof(GEOSGeometry*));

    iterator = array_create_iterator(array, 0, NULL);

    while(array_iterate(iterator, &value, &isnull))
	{
        GSERIALIZED *geom = (GSERIALIZED*) DatumGetPointer(value);

        if (isnull)
            continue;

		*is3d = *is3d || gserialized_has_z(geom);

		geos_geoms[i] = POSTGIS2GEOS(geom);
		if (!geos_geoms[i])
		{
            uint32_t j;
            lwpgerror("Geometry could not be converted to GEOS");

			for (j = 0; j < i; j++) {
				GEOSGeom_destroy(geos_geoms[j]);
			}
			return NULL;
		}

		if (!gotsrid)
		{
			*srid = gserialized_get_srid(geom);
            gotsrid = true;
		}
		else if (*srid != gserialized_get_srid(geom))
		{
            uint32_t j;
            for (j = 0; j <= i; j++) {
				GEOSGeom_destroy(geos_geoms[j]);
			}
			gserialized_error_if_srid_mismatch_reference(geom, *srid, __func__);
			return NULL;
		}

        i++;
	}

    array_free_iterator(iterator);
	return geos_geoms;
}

PG_FUNCTION_INFO_V1(GEOSnoop);
Datum GEOSnoop(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom;
	GEOSGeometry *geosgeom;
	GSERIALIZED *lwgeom_result;

	initGEOS(lwpgnotice, lwgeom_geos_error);

	geom = PG_GETARG_GSERIALIZED_P(0);
	geosgeom = POSTGIS2GEOS(geom);
	if ( ! geosgeom ) PG_RETURN_NULL();

	lwgeom_result = GEOS2POSTGIS(geosgeom, gserialized_has_z(geom));
	GEOSGeom_destroy(geosgeom);

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(lwgeom_result);
}

PG_FUNCTION_INFO_V1(polygonize_garray);
Datum polygonize_garray(PG_FUNCTION_ARGS)
{
	ArrayType *array;
	int is3d = 0;
	uint32 nelems, i;
	GSERIALIZED *result;
	GEOSGeometry *geos_result;
	const GEOSGeometry **vgeoms;
	int32_t srid = SRID_UNKNOWN;
#if POSTGIS_DEBUG_LEVEL >= 3
	static int call=1;
#endif

#if POSTGIS_DEBUG_LEVEL >= 3
	call++;
#endif

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	array = PG_GETARG_ARRAYTYPE_P(0);
	nelems = array_nelems_not_null(array);

	if (nelems == 0)
		PG_RETURN_NULL();

	POSTGIS_DEBUGF(3, "polygonize_garray: number of non-null elements: %d", nelems);

	/* Ok, we really need geos now ;) */
	initGEOS(lwpgnotice, lwgeom_geos_error);

	vgeoms = (const GEOSGeometry**) ARRAY2GEOS(array, nelems, &is3d, &srid);

	POSTGIS_DEBUG(3, "polygonize_garray: invoking GEOSpolygonize");

	geos_result = GEOSPolygonize(vgeoms, nelems);

	POSTGIS_DEBUG(3, "polygonize_garray: GEOSpolygonize returned");

	for (i=0; i<nelems; ++i) GEOSGeom_destroy((GEOSGeometry *)vgeoms[i]);
	pfree(vgeoms);

	if ( ! geos_result ) PG_RETURN_NULL();

	GEOSSetSRID(geos_result, srid);
	result = GEOS2POSTGIS(geos_result, is3d);
	GEOSGeom_destroy(geos_result);
	if (!result)
	{
		elog(ERROR, "%s returned an error", __func__);
		PG_RETURN_NULL(); /*never get here */
	}

	PG_RETURN_POINTER(result);
}


PG_FUNCTION_INFO_V1(clusterintersecting_garray);
Datum clusterintersecting_garray(PG_FUNCTION_ARGS)
{
	Datum* result_array_data;
	ArrayType *array, *result;
	int is3d = 0;
	uint32 nelems, nclusters, i;
	GEOSGeometry **geos_inputs, **geos_results;
	int32_t srid = SRID_UNKNOWN;

	/* Parameters used to construct a result array */
	int16 elmlen;
	bool elmbyval;
	char elmalign;

	/* Null array, null geometry (should be empty?) */
    if (PG_ARGISNULL(0))
        PG_RETURN_NULL();

	array = PG_GETARG_ARRAYTYPE_P(0);
    nelems = array_nelems_not_null(array);

	POSTGIS_DEBUGF(3, "clusterintersecting_garray: number of non-null elements: %d", nelems);

	if ( nelems == 0 ) PG_RETURN_NULL();

    /* TODO short-circuit for one element? */

	/* Ok, we really need geos now ;) */
	initGEOS(lwpgnotice, lwgeom_geos_error);

	geos_inputs = ARRAY2GEOS(array, nelems, &is3d, &srid);
	if(!geos_inputs)
	{
		PG_RETURN_NULL();
	}

	if (cluster_intersecting(geos_inputs, nelems, &geos_results, &nclusters) != LW_SUCCESS)
	{
		elog(ERROR, "clusterintersecting: Error performing clustering");
		PG_RETURN_NULL();
	}
	pfree(geos_inputs); /* don't need to destroy items because GeometryCollections have taken ownership */

	if (!geos_results) PG_RETURN_NULL();

	result_array_data = palloc(nclusters * sizeof(Datum));
	for (i=0; i<nclusters; ++i)
	{
		result_array_data[i] = PointerGetDatum(GEOS2POSTGIS(geos_results[i], is3d));
		GEOSGeom_destroy(geos_results[i]);
	}
	lwfree(geos_results);

	get_typlenbyvalalign(array->elemtype, &elmlen, &elmbyval, &elmalign);
	result = construct_array(result_array_data, nclusters, array->elemtype, elmlen, elmbyval, elmalign);

	if (!result)
	{
		elog(ERROR, "clusterintersecting: Error constructing return-array");
		PG_RETURN_NULL();
	}

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(cluster_within_distance_garray);
Datum cluster_within_distance_garray(PG_FUNCTION_ARGS)
{
	Datum* result_array_data;
	ArrayType *array, *result;
	int is3d = 0;
	uint32 nelems, nclusters, i;
	LWGEOM** lw_inputs;
	LWGEOM** lw_results;
	double tolerance;
	int32_t srid = SRID_UNKNOWN;

	/* Parameters used to construct a result array */
	int16 elmlen;
	bool elmbyval;
	char elmalign;

	/* Null array, null geometry (should be empty?) */
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	array = PG_GETARG_ARRAYTYPE_P(0);

	tolerance = PG_GETARG_FLOAT8(1);
	if (tolerance < 0)
	{
		lwpgerror("Tolerance must be a positive number.");
		PG_RETURN_NULL();
	}

	nelems = array_nelems_not_null(array);

	POSTGIS_DEBUGF(3, "cluster_within_distance_garray: number of non-null elements: %d", nelems);

	if ( nelems == 0 ) PG_RETURN_NULL();

    /* TODO short-circuit for one element? */

	/* Ok, we really need geos now ;) */
	initGEOS(lwpgnotice, lwgeom_geos_error);

	lw_inputs = ARRAY2LWGEOM(array, nelems, &is3d, &srid);
	if (!lw_inputs)
	{
		PG_RETURN_NULL();
	}

	if (cluster_within_distance(lw_inputs, nelems, tolerance, &lw_results, &nclusters) != LW_SUCCESS)
	{
		elog(ERROR, "cluster_within: Error performing clustering");
		PG_RETURN_NULL();
	}
	pfree(lw_inputs); /* don't need to destroy items because GeometryCollections have taken ownership */

	if (!lw_results) PG_RETURN_NULL();

	result_array_data = palloc(nclusters * sizeof(Datum));
	for (i=0; i<nclusters; ++i)
	{
		result_array_data[i] = PointerGetDatum(geometry_serialize(lw_results[i]));
		lwgeom_free(lw_results[i]);
	}
	lwfree(lw_results);

	get_typlenbyvalalign(array->elemtype, &elmlen, &elmbyval, &elmalign);
	result =  construct_array(result_array_data, nclusters, array->elemtype, elmlen, elmbyval, elmalign);

	if (!result)
	{
		elog(ERROR, "clusterwithin: Error constructing return-array");
		PG_RETURN_NULL();
	}

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(linemerge);
Datum linemerge(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom1;
	GSERIALIZED *result;
	LWGEOM *lwgeom1, *lwresult ;

	geom1 = PG_GETARG_GSERIALIZED_P(0);


	lwgeom1 = lwgeom_from_gserialized(geom1) ;

	lwresult = lwgeom_linemerge(lwgeom1);
	result = geometry_serialize(lwresult) ;

	lwgeom_free(lwgeom1) ;
	lwgeom_free(lwresult) ;

	PG_FREE_IF_COPY(geom1, 0);

	PG_RETURN_POINTER(result);
}

/*
 * Take a geometry and return an areal geometry
 * (Polygon or MultiPolygon).
 * Actually a wrapper around GEOSpolygonize,
 * transforming the resulting collection into
 * a valid polygon Geometry.
 */
PG_FUNCTION_INFO_V1(ST_BuildArea);
Datum ST_BuildArea(PG_FUNCTION_ARGS)
{
	GSERIALIZED *result;
	GSERIALIZED *geom;
	LWGEOM *lwgeom_in, *lwgeom_out;

	geom = PG_GETARG_GSERIALIZED_P(0);
	lwgeom_in = lwgeom_from_gserialized(geom);

	lwgeom_out = lwgeom_buildarea(lwgeom_in);
	lwgeom_free(lwgeom_in) ;

	if ( ! lwgeom_out )
	{
		PG_FREE_IF_COPY(geom, 0);
		PG_RETURN_NULL();
	}

	result = geometry_serialize(lwgeom_out) ;
	lwgeom_free(lwgeom_out) ;

	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_POINTER(result);
}

/*
 * Take the vertices of a geometry and builds
 * Delaunay triangles around them.
 */
PG_FUNCTION_INFO_V1(ST_DelaunayTriangles);
Datum ST_DelaunayTriangles(PG_FUNCTION_ARGS)
{
	GSERIALIZED *result;
	GSERIALIZED *geom;
	LWGEOM *lwgeom_in, *lwgeom_out;
	double	tolerance = 0.0;
	int flags = 0;

	geom = PG_GETARG_GSERIALIZED_P(0);
	tolerance = PG_GETARG_FLOAT8(1);
	flags = PG_GETARG_INT32(2);

	lwgeom_in = lwgeom_from_gserialized(geom);
	lwgeom_out = lwgeom_delaunay_triangulation(lwgeom_in, tolerance, flags);
	lwgeom_free(lwgeom_in) ;

	if ( ! lwgeom_out )
	{
		PG_FREE_IF_COPY(geom, 0);
		PG_RETURN_NULL();
	}

	result = geometry_serialize(lwgeom_out) ;
	lwgeom_free(lwgeom_out) ;

	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_POINTER(result);
}

/*
 * ST_Snap
 *
 * Snap a geometry to another with a given tolerance
 */
Datum ST_Snap(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_Snap);
Datum ST_Snap(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom1, *geom2, *result;
	LWGEOM *lwgeom1, *lwgeom2, *lwresult;
	double tolerance;

	geom1 = PG_GETARG_GSERIALIZED_P(0);
	geom2 = PG_GETARG_GSERIALIZED_P(1);
	tolerance = PG_GETARG_FLOAT8(2);

	lwgeom1 = lwgeom_from_gserialized(geom1);
	lwgeom2 = lwgeom_from_gserialized(geom2);

	lwresult = lwgeom_snap(lwgeom1, lwgeom2, tolerance);
	lwgeom_free(lwgeom1);
	lwgeom_free(lwgeom2);
	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	result = geometry_serialize(lwresult);
	lwgeom_free(lwresult);

	PG_RETURN_POINTER(result);
}

/*
 * ST_Split
 *
 * Split polygon by line, line by line, line by point.
 * Returns at most components as a collection.
 * First element of the collection is always the part which
 * remains after the cut, while the second element is the
 * part which has been cut out. We arbitrarily take the part
 * on the *right* of cut lines as the part which has been cut out.
 * For a line cut by a point the part which remains is the one
 * from start of the line to the cut point.
 *
 *
 * Author: Sandro Santilli <strk@kbt.io>
 *
 * Work done for Faunalia (http://www.faunalia.it) with fundings
 * from Regione Toscana - Sistema Informativo per il Governo
 * del Territorio e dell'Ambiente (RT-SIGTA).
 *
 * Thanks to the PostGIS community for sharing poly/line ideas [1]
 *
 * [1] http://trac.osgeo.org/postgis/wiki/UsersWikiSplitPolygonWithLineString
 *
 */
Datum ST_Split(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_Split);
Datum ST_Split(PG_FUNCTION_ARGS)
{
	GSERIALIZED *in, *blade_in, *out;
	LWGEOM *lwgeom_in, *lwblade_in, *lwgeom_out;

	in = PG_GETARG_GSERIALIZED_P(0);
	blade_in = PG_GETARG_GSERIALIZED_P(1);
	gserialized_error_if_srid_mismatch(in, blade_in, __func__);

	lwgeom_in = lwgeom_from_gserialized(in);
	lwblade_in = lwgeom_from_gserialized(blade_in);

	lwgeom_out = lwgeom_split(lwgeom_in, lwblade_in);
	lwgeom_free(lwgeom_in);
	lwgeom_free(lwblade_in);

	if ( ! lwgeom_out )
	{
		PG_FREE_IF_COPY(in, 0); /* possibly referenced by lwgeom_out */
		PG_FREE_IF_COPY(blade_in, 1);
		PG_RETURN_NULL();
	}

	out = geometry_serialize(lwgeom_out);
	lwgeom_free(lwgeom_out);
	PG_FREE_IF_COPY(in, 0); /* possibly referenced by lwgeom_out */
	PG_FREE_IF_COPY(blade_in, 1);

	PG_RETURN_POINTER(out);
}

/**********************************************************************
 *
 * ST_SharedPaths
 *
 * Return the set of paths shared between two linear geometries,
 * and their direction (same or opposite).
 *
 * Developed by Sandro Santilli (strk@kbt.io) for Faunalia
 * (http://www.faunalia.it) with funding from Regione Toscana - Sistema
 * Informativo per la Gestione del Territorio e dell' Ambiente
 * [RT-SIGTA]". For the project: "Sviluppo strumenti software per il
 * trattamento di dati geografici basati su QuantumGIS e Postgis (CIG
 * 0494241492)"
 *
 **********************************************************************/
Datum ST_SharedPaths(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_SharedPaths);
Datum ST_SharedPaths(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom1, *geom2, *out;
	LWGEOM *g1, *g2, *lwgeom_out;

	geom1 = PG_GETARG_GSERIALIZED_P(0);
	geom2 = PG_GETARG_GSERIALIZED_P(1);

	g1 = lwgeom_from_gserialized(geom1);
	g2 = lwgeom_from_gserialized(geom2);

	lwgeom_out = lwgeom_sharedpaths(g1, g2);
	lwgeom_free(g1);
	lwgeom_free(g2);

	if ( ! lwgeom_out )
	{
		PG_FREE_IF_COPY(geom1, 0);
		PG_FREE_IF_COPY(geom2, 1);
		PG_RETURN_NULL();
	}

	out = geometry_serialize(lwgeom_out);
	lwgeom_free(lwgeom_out);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);
	PG_RETURN_POINTER(out);
}


/**********************************************************************
 *
 * ST_Node
 *
 * Fully node a set of lines using the least possible nodes while
 * preserving all of the input ones.
 *
 **********************************************************************/
Datum ST_Node(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_Node);
Datum ST_Node(PG_FUNCTION_ARGS)
{
	GSERIALIZED *geom1, *out;
	LWGEOM *g1, *lwgeom_out;

	geom1 = PG_GETARG_GSERIALIZED_P(0);

	g1 = lwgeom_from_gserialized(geom1);

	lwgeom_out = lwgeom_node(g1);
	lwgeom_free(g1);

	if ( ! lwgeom_out )
	{
		PG_FREE_IF_COPY(geom1, 0);
		PG_RETURN_NULL();
	}

	out = geometry_serialize(lwgeom_out);
	lwgeom_free(lwgeom_out);

	PG_FREE_IF_COPY(geom1, 0);
	PG_RETURN_POINTER(out);
}

/******************************************
 *
 * ST_Voronoi
 *
 * Returns a Voronoi diagram constructed
 * from the points of the input geometry.
 *
 ******************************************/
Datum ST_Voronoi(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_Voronoi);
Datum ST_Voronoi(PG_FUNCTION_ARGS)
{
	GSERIALIZED* input;
	GSERIALIZED* clip;
	GSERIALIZED* result;
	LWGEOM* lwgeom_input;
	LWGEOM* lwgeom_result;
	double tolerance;
	GBOX clip_envelope;
	int custom_clip_envelope;
	int return_polygons;

	/* Return NULL on NULL geometry */
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	/* Read our tolerance value */
	if (PG_ARGISNULL(2))
	{
		lwpgerror("Tolerance must be a positive number.");
		PG_RETURN_NULL();
	}

	tolerance = PG_GETARG_FLOAT8(2);

	if (tolerance < 0)
	{
		lwpgerror("Tolerance must be a positive number.");
		PG_RETURN_NULL();
	}

	/* Are we returning lines or polygons? */
	if (PG_ARGISNULL(3))
	{
		lwpgerror("return_polygons must be true or false.");
		PG_RETURN_NULL();
	}
	return_polygons = PG_GETARG_BOOL(3);

	/* Read our clipping envelope, if applicable. */
	custom_clip_envelope = !PG_ARGISNULL(1);
	if (custom_clip_envelope) {
		clip = PG_GETARG_GSERIALIZED_P(1);
		if (!gserialized_get_gbox_p(clip, &clip_envelope))
		{
			lwpgerror("Could not determine envelope of clipping geometry.");
			PG_FREE_IF_COPY(clip, 1);
			PG_RETURN_NULL();
		}
		PG_FREE_IF_COPY(clip, 1);
	}

	/* Read our input geometry */
	input = PG_GETARG_GSERIALIZED_P(0);

	lwgeom_input = lwgeom_from_gserialized(input);

	if(!lwgeom_input)
	{
		lwpgerror("Could not read input geometry.");
		PG_FREE_IF_COPY(input, 0);
		PG_RETURN_NULL();
	}

	lwgeom_result = lwgeom_voronoi_diagram(lwgeom_input, custom_clip_envelope ? &clip_envelope : NULL, tolerance, !return_polygons);
	lwgeom_free(lwgeom_input);

	if (!lwgeom_result)
	{
		lwpgerror("Error computing Voronoi diagram.");
		PG_FREE_IF_COPY(input, 0);
		PG_RETURN_NULL();
	}

	result = geometry_serialize(lwgeom_result);
	lwgeom_free(lwgeom_result);

	PG_FREE_IF_COPY(input, 0);
	PG_RETURN_POINTER(result);
}

/******************************************
 *
 * ST_MinimumClearance
 *
 * Returns the minimum clearance of a geometry.
 *
 ******************************************/
Datum ST_MinimumClearance(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_MinimumClearance);
Datum ST_MinimumClearance(PG_FUNCTION_ARGS)
{
	GSERIALIZED* input;
	GEOSGeometry* input_geos;
	int error;
	double result;

	initGEOS(lwpgnotice, lwgeom_geos_error);

	input = PG_GETARG_GSERIALIZED_P(0);
	input_geos = POSTGIS2GEOS(input);
	if (!input_geos)
		HANDLE_GEOS_ERROR("Geometry could not be converted to GEOS");

	error = GEOSMinimumClearance(input_geos, &result);
	GEOSGeom_destroy(input_geos);
	if (error) HANDLE_GEOS_ERROR("Error computing minimum clearance");

	PG_FREE_IF_COPY(input, 0);
	PG_RETURN_FLOAT8(result);
}

/******************************************
 *
 * ST_MinimumClearanceLine
 *
 * Returns the minimum clearance line of a geometry.
 *
 ******************************************/
Datum ST_MinimumClearanceLine(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_MinimumClearanceLine);
Datum ST_MinimumClearanceLine(PG_FUNCTION_ARGS)
{
	GSERIALIZED* input;
	GSERIALIZED* result;
	GEOSGeometry* input_geos;
	GEOSGeometry* result_geos;
	int32_t srid;

	initGEOS(lwpgnotice, lwgeom_geos_error);

	input = PG_GETARG_GSERIALIZED_P(0);
	srid = gserialized_get_srid(input);
	input_geos = POSTGIS2GEOS(input);
	if (!input_geos)
		HANDLE_GEOS_ERROR("Geometry could not be converted to GEOS");

	result_geos = GEOSMinimumClearanceLine(input_geos);
	GEOSGeom_destroy(input_geos);
	if (!result_geos)
		HANDLE_GEOS_ERROR("Error computing minimum clearance");

	GEOSSetSRID(result_geos, srid);
	result = GEOS2POSTGIS(result_geos, LW_FALSE);
	GEOSGeom_destroy(result_geos);

	PG_FREE_IF_COPY(input, 0);
	PG_RETURN_POINTER(result);
}

/******************************************
 *
 * ST_OrientedEnvelope
 *
 ******************************************/
Datum ST_OrientedEnvelope(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ST_OrientedEnvelope);
Datum ST_OrientedEnvelope(PG_FUNCTION_ARGS)
{
	GSERIALIZED* input;
	GSERIALIZED* result;
	GEOSGeometry* input_geos;
	GEOSGeometry* result_geos;
	int32_t srid;

	initGEOS(lwpgnotice, lwgeom_geos_error);

	input = PG_GETARG_GSERIALIZED_P(0);
	srid = gserialized_get_srid(input);
	input_geos = POSTGIS2GEOS(input);
	if (!input_geos)
		HANDLE_GEOS_ERROR("Geometry could not be converted to GEOS");

	result_geos = GEOSMinimumRotatedRectangle(input_geos);
	GEOSGeom_destroy(input_geos);
	if (!result_geos)
		HANDLE_GEOS_ERROR("Error computing oriented envelope");

	GEOSSetSRID(result_geos, srid);
	result = GEOS2POSTGIS(result_geos, LW_FALSE);
	GEOSGeom_destroy(result_geos);

	PG_FREE_IF_COPY(input, 0);
	PG_RETURN_POINTER(result);
}
