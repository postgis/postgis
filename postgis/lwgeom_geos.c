/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 *
 * Copyright 2009-2011 Sandro Santilli <strk@keybit.net>
 * Copyright 2008 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright 2001-2003 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "lwgeom_geos.h"
#include "lwgeom_rtree.h"
#include "lwgeom_geos_prepared.h"
#include "funcapi.h"
#include "lwgeom_functions_analytic.h"

#include <string.h>
#include <assert.h>

/**
** NOTE: Buffer-based GeomUnion has been disabled due to
** limitations in the GEOS code (it would only work against polygons)
** DONE: Implement cascaded GeomUnion and remove old buffer-based code.
*/

/*
** Prototypes for SQL-bound functions
*/
Datum relate_full(PG_FUNCTION_ARGS);
Datum relate_pattern(PG_FUNCTION_ARGS);
Datum disjoint(PG_FUNCTION_ARGS);
Datum touches(PG_FUNCTION_ARGS);
Datum intersects(PG_FUNCTION_ARGS);
Datum crosses(PG_FUNCTION_ARGS);
Datum within(PG_FUNCTION_ARGS);
Datum contains(PG_FUNCTION_ARGS);
Datum containsproperly(PG_FUNCTION_ARGS);
Datum covers(PG_FUNCTION_ARGS);
Datum overlaps(PG_FUNCTION_ARGS);
Datum isvalid(PG_FUNCTION_ARGS);
Datum isvalidreason(PG_FUNCTION_ARGS);
Datum isvaliddetail(PG_FUNCTION_ARGS);
Datum buffer(PG_FUNCTION_ARGS);
Datum intersection(PG_FUNCTION_ARGS);
Datum convexhull(PG_FUNCTION_ARGS);
Datum topologypreservesimplify(PG_FUNCTION_ARGS);
Datum difference(PG_FUNCTION_ARGS);
Datum boundary(PG_FUNCTION_ARGS);
Datum symdifference(PG_FUNCTION_ARGS);
Datum geomunion(PG_FUNCTION_ARGS);
Datum ST_UnaryUnion(PG_FUNCTION_ARGS);
Datum issimple(PG_FUNCTION_ARGS);
Datum isring(PG_FUNCTION_ARGS);
Datum geomequals(PG_FUNCTION_ARGS);
Datum pointonsurface(PG_FUNCTION_ARGS);
Datum GEOSnoop(PG_FUNCTION_ARGS);
Datum postgis_geos_version(PG_FUNCTION_ARGS);
Datum centroid(PG_FUNCTION_ARGS);
Datum polygonize_garray(PG_FUNCTION_ARGS);
Datum LWGEOM_buildarea(PG_FUNCTION_ARGS); /* TODO: rename to match others
*/
Datum linemerge(PG_FUNCTION_ARGS);
Datum coveredby(PG_FUNCTION_ARGS);
Datum hausdorffdistance(PG_FUNCTION_ARGS);
Datum hausdorffdistancedensify(PG_FUNCTION_ARGS);

Datum pgis_union_geometry_array_old(PG_FUNCTION_ARGS);
Datum pgis_union_geometry_array(PG_FUNCTION_ARGS);


/*
** Prototypes end
*/

char lwgeom_geos_errmsg[LWGEOM_GEOS_ERRMSG_MAXSIZE];

extern void
lwgeom_geos_error(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	/* Call the supplied function */
	if ( LWGEOM_GEOS_ERRMSG_MAXSIZE-1 < vsnprintf(lwgeom_geos_errmsg, LWGEOM_GEOS_ERRMSG_MAXSIZE-1, fmt, ap) )
	{
		lwgeom_geos_errmsg[LWGEOM_GEOS_ERRMSG_MAXSIZE-1] = '\0';
	}

	va_end(ap);
}

GEOSGeometry*
LWGEOM_GEOS_buildArea(const GEOSGeometry* geom_in)
{
	GEOSGeometry *tmp;
	GEOSGeometry *geos_result, *shp;
	GEOSGeometry const *vgeoms[1];
	uint32 i, ngeoms;

	vgeoms[0] = geom_in;
	geos_result = GEOSPolygonize(vgeoms, 1);

	POSTGIS_DEBUGF(3, "GEOSpolygonize returned @ %p", geos_result);

	/* Null return from GEOSpolygonize (an exception) */
	if ( ! geos_result ) return 0;

	/*
	 * We should now have a collection
	 */
#if PARANOIA_LEVEL > 0
	if ( GEOSGeometryTypeId(geos_result) != COLLECTIONTYPE )
	{
		GEOSGeom_destroy(geos_result);
		lwerror("Unexpected return from GEOSpolygonize");
		return 0;
	}
#endif

	ngeoms = GEOSGetNumGeometries(geos_result);

	POSTGIS_DEBUGF(3, "GEOSpolygonize: ngeoms in polygonize output: %d", ngeoms);
	POSTGIS_DEBUGF(3, "GEOSpolygonize: polygonized:%s",
	               lwgeom_to_ewkt(GEOS2LWGEOM(geos_result, 0), PARSER_CHECK_NONE));

	/*
	 * No geometries in collection, early out
	 */
	if ( ngeoms == 0 )
	{
		return geos_result;
	}

	/*
	 * Return first geometry if we only have one in collection,
	 * to avoid the unnecessary Geometry clone below.
	 */
	if ( ngeoms == 1 )
	{
		tmp = (GEOSGeometry *)GEOSGetGeometryN(geos_result, 0);
		if ( ! tmp )
		{
			GEOSGeom_destroy(geos_result);
			return 0; /* exception */
		}
		shp = GEOSGeom_clone(tmp);
		GEOSGeom_destroy(geos_result); /* only safe after the clone above */
		return shp;
	}

	/*
	 * Iteratively invoke symdifference on outer rings
	 * as suggested by Carl Anderson:
	 * postgis-devel/2005-December/001805.html
	 */
	shp = NULL;
	for (i=0; i<ngeoms; ++i)
	{
		GEOSGeom extring;
		GEOSCoordSeq sq;

		/*
		 * Construct a Polygon from geometry i exterior ring
		 * We don't use GEOSGeom_clone on the ExteriorRing
		 * due to a bug in CAPI contained in GEOS 2.2 branch
		 * failing to properly return a LinearRing from
		 * a LinearRing clone.
		 */
		sq=GEOSCoordSeq_clone(GEOSGeom_getCoordSeq(
		                          GEOSGetExteriorRing(GEOSGetGeometryN( geos_result, i))
		                      ));
		extring = GEOSGeom_createPolygon(
		              GEOSGeom_createLinearRing(sq),
		              NULL, 0
		          );

		if ( extring == NULL ) /* exception */
		{
			lwerror("GEOSCreatePolygon threw an exception");
			return 0;
		}

		if ( shp == NULL )
		{
			shp = extring;
			POSTGIS_DEBUGF(3, "GEOSpolygonize: shp:%s",
			               lwgeom_to_ewkt(GEOS2LWGEOM(shp, 0), PARSER_CHECK_NONE));
		}
		else
		{
			tmp = GEOSSymDifference(shp, extring);
			POSTGIS_DEBUGF(3, "GEOSpolygonize: SymDifference(%s, %s):%s",
			               lwgeom_to_ewkt(GEOS2LWGEOM(shp, 0), PARSER_CHECK_NONE),
			               lwgeom_to_ewkt(GEOS2LWGEOM(extring, 0), PARSER_CHECK_NONE),
			               lwgeom_to_ewkt(GEOS2LWGEOM(tmp, 0), PARSER_CHECK_NONE)
			              );
			GEOSGeom_destroy(shp);
			GEOSGeom_destroy(extring);
			shp = tmp;
		}
	}

	GEOSGeom_destroy(geos_result);

	return shp;
}

PG_FUNCTION_INFO_V1(postgis_geos_version);
Datum postgis_geos_version(PG_FUNCTION_ARGS)
{
	const char *ver = GEOSversion();
	text *result = cstring2text(ver);
	PG_RETURN_POINTER(result);
}


#if POSTGIS_GEOS_VERSION >= 32

/**
 *  @brief Compute the Hausdorff distance thanks to the corresponding GEOS function
 *  @example hausdorffdistance {@link #hausdorffdistance} - SELECT st_hausdorffdistance(
 *      'POLYGON((0 0, 0 2, 1 2, 2 2, 2 0, 0 0))'::geometry,
 *      'POLYGON((0.5 0.5, 0.5 2.5, 1.5 2.5, 2.5 2.5, 2.5 0.5, 0.5 0.5))'::geometry);
 */

PG_FUNCTION_INFO_V1(hausdorffdistance);
Datum hausdorffdistance(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeometry *g1;
	GEOSGeometry *g2;
	double result;
	int retcode;

	POSTGIS_DEBUG(2, "hausdorff_distance called");

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	if ( pglwgeom_is_empty(geom1) || pglwgeom_is_empty(geom2) )
		PG_RETURN_NULL();

	initGEOS(lwnotice, lwgeom_geos_error);

	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}

	g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
	if ( 0 == g2 )   /* exception thrown */
	{
		lwerror("Second argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		GEOSGeom_destroy(g1);
		PG_RETURN_NULL();
	}

	retcode = GEOSHausdorffDistance(g1, g2, &result);
	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (retcode == 0)
	{
		lwerror("GEOSHausdorffDistance: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL(); /*never get here */
	}

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_FLOAT8(result);
}

/**
 *  @brief Compute the Hausdorff distance with densification thanks to the corresponding GEOS function
 *  @example hausdorffdistancedensify {@link #hausdorffdistancedensify} - SELECT st_hausdorffdistancedensify(
 *      'POLYGON((0 0, 0 2, 1 2, 2 2, 2 0, 0 0))'::geometry,
 *      'POLYGON((0.5 0.5, 0.5 2.5, 1.5 2.5, 2.5 2.5, 2.5 0.5, 0.5 0.5))'::geometry, 0.5);
 */

PG_FUNCTION_INFO_V1(hausdorffdistancedensify);
Datum hausdorffdistancedensify(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeometry *g1;
	GEOSGeometry *g2;
	double densifyFrac;
	double result;
	int retcode;


	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	densifyFrac = PG_GETARG_FLOAT8(2);

	if ( pglwgeom_is_empty(geom1) || pglwgeom_is_empty(geom2) )
		PG_RETURN_NULL();

	initGEOS(lwnotice, lwgeom_geos_error);

	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}

	g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
	if ( 0 == g2 )   /* exception thrown at construction */
	{
		lwerror("Second argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		GEOSGeom_destroy(g1);
		PG_RETURN_NULL();
	}

	retcode = GEOSHausdorffDistanceDensify(g1, g2, densifyFrac, &result);
	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (retcode == 0)
	{
		lwerror("GEOSHausdorffDistanceDensify: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL(); /*never get here */
	}

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_FLOAT8(result);
}

#endif


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
	Datum datum;
	ArrayType *array;
	int is3d = 0;
	int nelems, i;
	PG_LWGEOM *result = NULL;
	PG_LWGEOM *pgis_geom = NULL;
	GEOSGeometry * g1 = NULL;
	GEOSGeometry * g2 = NULL;
	GEOSGeometry * geos_result=NULL;
	int srid=SRID_UNKNOWN;
	size_t offset = 0;
	bits8 *bitmap;
	int bitmask;
#if POSTGIS_DEBUG_LEVEL > 0
	static int call=1;
#endif
	int gotsrid = 0;
	int allpolys=1;

#if POSTGIS_DEBUG_LEVEL > 0
	call++;
	POSTGIS_DEBUGF(2, "GEOS incremental union (call %d)", call);
#endif

	datum = PG_GETARG_DATUM(0);

	/* TODO handle empties */

	/* Null array, null geometry (should be empty?) */
	if ( (Pointer *)datum == NULL ) PG_RETURN_NULL();

	array = DatumGetArrayTypeP(datum);

	nelems = ArrayGetNItems(ARR_NDIM(array), ARR_DIMS(array));

	bitmap = ARR_NULLBITMAP(array);

	POSTGIS_DEBUGF(3, "unite_garray: number of elements: %d", nelems);

	if ( nelems == 0 ) PG_RETURN_NULL();

	/* One-element union is the element itself */
	if ( nelems == 1 )
	{
		/* If the element is a NULL then we need to handle it separately */
		if (bitmap && (*bitmap & 1) == 0)
			PG_RETURN_NULL();
		else
			PG_RETURN_POINTER((PG_LWGEOM *)(ARR_DATA_PTR(array)));
	}

	/* Ok, we really need geos now ;) */
	initGEOS(lwnotice, lwgeom_geos_error);

	/*
	** First, see if all our elements are POLYGON/MULTIPOLYGON
	** If they are, we can use UnionCascaded for faster results.
	*/
	offset = 0;
	bitmask = 1;
	gotsrid = 0;
	for ( i = 0; i < nelems; i++ )
	{
		/* Don't do anything for NULL values */
		if ((bitmap && (*bitmap & bitmask) != 0) || !bitmap)
		{
			PG_LWGEOM *pggeom = (PG_LWGEOM *)(ARR_DATA_PTR(array)+offset);
			int pgtype = pglwgeom_get_type(pggeom);
			offset += INTALIGN(VARSIZE(pggeom));
			if ( ! gotsrid ) /* Initialize SRID */
			{
				srid = pglwgeom_get_srid(pggeom);
				gotsrid = 1;
				if ( pglwgeom_has_z(pggeom) ) is3d = 1;
			}
			else
			{
				error_if_srid_mismatch(srid, pglwgeom_get_srid(pggeom));
			}

			if ( pgtype != POLYGONTYPE && pgtype != MULTIPOLYGONTYPE )
			{
				allpolys = 0;
				break;
			}
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

	if ( allpolys )
	{
		/*
		** Everything is polygonal, so let's run the cascaded polygon union!
		*/
		int geoms_size = nelems;
		int curgeom = 0;
		GEOSGeometry **geoms = NULL;
		geoms = palloc( sizeof(GEOSGeometry *) * geoms_size );
		/*
		** We need to convert the array of PG_LWGEOM into a GEOS MultiPolygon.
		** First make an array of GEOS Polygons.
		*/
		offset = 0;
		bitmap = ARR_NULLBITMAP(array);
		bitmask = 1;
		for ( i = 0; i < nelems; i++ )
		{
			GEOSGeometry* g;

			/* Don't do anything for NULL values */
			if ((bitmap && (*bitmap & bitmask) != 0) || !bitmap)
			{
				PG_LWGEOM *pggeom = (PG_LWGEOM *)(ARR_DATA_PTR(array)+offset);
				int pgtype = pglwgeom_get_type(pggeom);
				offset += INTALIGN(VARSIZE(pggeom));
				if ( pgtype == POLYGONTYPE )
				{
					if ( curgeom == geoms_size )
					{
						geoms_size *= 2;
						geoms = repalloc( geoms, sizeof(GEOSGeom) * geoms_size );
					}
					g = (GEOSGeometry *)POSTGIS2GEOS(pggeom);
					if ( 0 == g )   /* exception thrown at construction */
					{
						/* TODO: release GEOS allocated memory ! */
						lwerror("One of the geometries in the set "
						        "could not be converted to GEOS: %s", lwgeom_geos_errmsg);
						PG_RETURN_NULL();
					}
					geoms[curgeom] = g;
					curgeom++;
				}
				if ( pgtype == MULTIPOLYGONTYPE )
				{
					int j = 0;
					LWMPOLY *lwmpoly = (LWMPOLY*)pglwgeom_deserialize(pggeom);;
					for ( j = 0; j < lwmpoly->ngeoms; j++ )
					{
						GEOSGeometry* g;
						if ( curgeom == geoms_size )
						{
							geoms_size *= 2;
							geoms = repalloc( geoms, sizeof(GEOSGeom) * geoms_size );
						}
						/* This builds a LWPOLY on top of the serialized form */
						g = LWGEOM2GEOS(lwpoly_as_lwgeom(lwmpoly->geoms[j]));
						if ( 0 == g )   /* exception thrown at construction */
						{
							/* TODO: cleanup all GEOS memory */
							lwerror("Geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
							PG_RETURN_NULL();
						}
						geoms[curgeom++] = g;
					}
					lwmpoly_free(lwmpoly);
				}
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
		/*
		** Take our GEOS Polygons and turn them into a GEOS MultiPolygon,
		** then pass that into cascaded union.
		*/
		if (curgeom > 0)
		{
			g1 = GEOSGeom_createCollection(GEOS_MULTIPOLYGON, geoms, curgeom);
			if ( ! g1 )
			{
				/* TODO: cleanup geoms memory */
				lwerror("Could not create MULTIPOLYGON from geometry array: %s", lwgeom_geos_errmsg);
				PG_RETURN_NULL();
			}
			g2 = GEOSUnionCascaded(g1);
			GEOSGeom_destroy(g1);
			if ( ! g2 )
			{
				lwerror("GEOSUnionCascaded: %s",
				        lwgeom_geos_errmsg);
				PG_RETURN_NULL();
			}

			GEOSSetSRID(g2, srid);
			result = GEOS2POSTGIS(g2, is3d);
			GEOSGeom_destroy(g2);
		}
		else
		{
			/* All we found were NULLs, so let's return NULL */
			PG_RETURN_NULL();
		}
	}
	else
	{
		/*
		** Heterogeneous result, let's slog through this one union at a time.
		*/
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

				pgis_geom = geom;

				POSTGIS_DEBUGF(3, "geom %d @ %p", i, geom);

				/* Check is3d flag */
				if ( pglwgeom_has_z(geom) ) is3d = 1;

				/* Check SRID homogeneity and initialize geos result */
				if ( ! geos_result )
				{
					geos_result = (GEOSGeometry *)POSTGIS2GEOS(geom);
					if ( 0 == geos_result )   /* exception thrown at construction */
					{
						lwerror("geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
						PG_RETURN_NULL();
					}
					srid = pglwgeom_get_srid(geom);
					POSTGIS_DEBUGF(3, "first geom is a %s", lwtype_name(pglwgeom_get_type(geom)));
				}
				else
				{
					error_if_srid_mismatch(srid, pglwgeom_get_srid(geom));

					g1 = POSTGIS2GEOS(pgis_geom);
					if ( 0 == g1 )   /* exception thrown at construction */
					{
						/* TODO: release GEOS allocated memory ! */
						lwerror("First argument geometry could not be converted to GEOS: %s",
						        lwgeom_geos_errmsg);
						PG_RETURN_NULL();
					}

					POSTGIS_DEBUGF(3, "unite_garray(%d): adding geom %d to union (%s)",
					               call, i, lwtype_name(pglwgeom_get_type(geom)));

					g2 = GEOSUnion(g1, geos_result);
					if ( g2 == NULL )
					{
						GEOSGeom_destroy((GEOSGeometry *)g1);
						GEOSGeom_destroy((GEOSGeometry *)geos_result);
						lwerror("GEOSUnion: %s", lwgeom_geos_errmsg);
					}
					GEOSGeom_destroy((GEOSGeometry *)g1);
					GEOSGeom_destroy((GEOSGeometry *)geos_result);
					geos_result = g2;
				}
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

		/* If geos_result is set then we found at least one non-NULL geometry */
		if (geos_result)
		{
			GEOSSetSRID(geos_result, srid);
			result = GEOS2POSTGIS(geos_result, is3d);
			GEOSGeom_destroy(geos_result);
		}
		else
		{
			/* All we found were NULLs, so let's return NULL */
			PG_RETURN_NULL();
		}

	}

	if ( result == NULL )
	{
		/* Union returned a NULL geometry */
		PG_RETURN_NULL();
	}

	PG_RETURN_POINTER(result);

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
#if POSTGIS_GEOS_VERSION < 33
	PG_RETURN_NULL();
	lwerror("The GEOS version this postgis binary "
	        "was compiled against (%d) doesn't support "
	        "'GEOSUnaryUnion' function (3.3.0+ required)",
	        POSTGIS_GEOS_VERSION);
	PG_RETURN_NULL();
#else /* POSTGIS_GEOS_VERSION >= 33 */
	PG_LWGEOM *geom1;
	int is3d;
	int srid;
	GEOSGeometry *g1, *g3;
	PG_LWGEOM *result;

	POSTGIS_DEBUG(2, "in ST_UnaryUnion");

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* UnaryUnion(empty) == (empty) */
	if ( pglwgeom_is_empty(geom1) )
		PG_RETURN_POINTER(geom1);

	is3d = ( pglwgeom_has_z(geom1) );

	srid = pglwgeom_get_srid(geom1);

	initGEOS(lwnotice, lwgeom_geos_error);

	PROFSTART(PROF_P2G1);
	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	PROFSTOP(PROF_P2G1);
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}

	POSTGIS_DEBUGF(3, "g1=%s", GEOSGeomToWKT(g1));

	PROFSTART(PROF_GRUN);
	g3 = GEOSUnaryUnion(g1);
	PROFSTOP(PROF_GRUN);

	POSTGIS_DEBUGF(3, "g3=%s", GEOSGeomToWKT(g3));

	GEOSGeom_destroy(g1);

	if (g3 == NULL)
	{
		lwerror("GEOSUnion: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL(); /* never get here */
	}


	GEOSSetSRID(g3, srid);

	PROFSTART(PROF_G2P);
	result = GEOS2POSTGIS(g3, is3d);
	PROFSTOP(PROF_G2P);

	GEOSGeom_destroy(g3);

	if (result == NULL)
	{
		elog(ERROR, "ST_UnaryUnion failed converting GEOS result Geometry to PostGIS format");
		PG_RETURN_NULL(); /*never get here */
	}

	/* compressType(result); */

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, NULL, result);

	PG_FREE_IF_COPY(geom1, 0);

	PG_RETURN_POINTER(result);
#endif /* POSTGIS_GEOS_VERSION >= 33 */
}


/**
 * @example geomunion {@link #geomunion} SELECT geomunion(
 *      'POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))',
 *      'POLYGON((5 5, 15 5, 15 7, 5 7, 5 5))'
 * );
 *
 */
PG_FUNCTION_INFO_V1(geomunion);
Datum geomunion(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	int is3d;
	int srid;
	GEOSGeometry *g1, *g2, *g3;
	PG_LWGEOM *result;

	POSTGIS_DEBUG(2, "in geomunion");

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	/* A.Union(empty) == A */
	if ( pglwgeom_is_empty(geom1) )
		PG_RETURN_POINTER(geom2);

	/* B.Union(empty) == B */
	if ( pglwgeom_is_empty(geom2) )
		PG_RETURN_POINTER(geom1);

	is3d = ( pglwgeom_has_z(geom1) || pglwgeom_has_z(geom2) );

	srid = pglwgeom_get_srid(geom1);
	error_if_srid_mismatch(srid, pglwgeom_get_srid(geom2));

	initGEOS(lwnotice, lwgeom_geos_error);

	PROFSTART(PROF_P2G1);
	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	PROFSTOP(PROF_P2G1);
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}

	PROFSTART(PROF_P2G2);
	g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
	PROFSTOP(PROF_P2G2);
	if ( 0 == g2 )   /* exception thrown at construction */
	{
		GEOSGeom_destroy(g1);
		lwerror("Second argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}

	POSTGIS_DEBUGF(3, "g1=%s", GEOSGeomToWKT(g1));
	POSTGIS_DEBUGF(3, "g2=%s", GEOSGeomToWKT(g2));

	PROFSTART(PROF_GRUN);
	g3 = GEOSUnion(g1,g2);
	PROFSTOP(PROF_GRUN);

	POSTGIS_DEBUGF(3, "g3=%s", GEOSGeomToWKT(g3));

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (g3 == NULL)
	{
		lwerror("GEOSUnion: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL(); /* never get here */
	}


	GEOSSetSRID(g3, srid);

	PROFSTART(PROF_G2P);
	result = GEOS2POSTGIS(g3, is3d);
	PROFSTOP(PROF_G2P);

	GEOSGeom_destroy(g3);

	if (result == NULL)
	{
		elog(ERROR,"GEOS union() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /*never get here */
	}

	/* compressType(result); */

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, geom2, result);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_POINTER(result);
}


/**
 *  @example symdifference {@link #symdifference} - SELECT symdifference(
 *      'POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))',
 *      'POLYGON((5 5, 15 5, 15 7, 5 7, 5 5))');
 */
PG_FUNCTION_INFO_V1(symdifference);
Datum symdifference(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeometry *g1, *g2, *g3;
	PG_LWGEOM *result;
	int is3d;
	int srid;

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	/* A.SymDifference(Empty) == A */
	if ( pglwgeom_is_empty(geom2) )
		PG_RETURN_POINTER(geom1);

	/* Empty.DymDifference(B) == Empty */
	if ( pglwgeom_is_empty(geom1) )
		PG_RETURN_POINTER(geom1);


	is3d = ( pglwgeom_has_z(geom1) ) ||
	       ( pglwgeom_has_z(geom2) );

	srid = pglwgeom_get_srid(geom1);
	error_if_srid_mismatch(srid, pglwgeom_get_srid(geom2));

	initGEOS(lwnotice, lwgeom_geos_error);

	PROFSTART(PROF_P2G1);
	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	PROFSTOP(PROF_P2G1);
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}

	PROFSTART(PROF_P2G2);
	g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
	PROFSTOP(PROF_P2G2);
	if ( 0 == g2 )   /* exception thrown at construction */
	{
		lwerror("Second argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		GEOSGeom_destroy(g1);
		PG_RETURN_NULL();
	}

	PROFSTART(PROF_GRUN);
	g3 = GEOSSymDifference(g1,g2);
	PROFSTOP(PROF_GRUN);

	if (g3 == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		lwerror("GEOSSymDifference: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL(); /*never get here */
	}

	POSTGIS_DEBUGF(3, "result: %s", GEOSGeomToWKT(g3));

	GEOSSetSRID(g3, srid);

	PROFSTART(PROF_G2P);
	result = GEOS2POSTGIS(g3, is3d);
	PROFSTOP(PROF_G2P);

	if (result == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		GEOSGeom_destroy(g3);
		elog(ERROR,"GEOS symdifference() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /*never get here */
	}

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	GEOSGeom_destroy(g3);

	/* compressType(result); */

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, geom2, result);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_POINTER(result);
}


PG_FUNCTION_INFO_V1(boundary);
Datum boundary(PG_FUNCTION_ARGS)
{
	PG_LWGEOM	*geom1;
	GEOSGeometry *g1, *g3;
	PG_LWGEOM *result;
	int srid;

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* Empty.Boundary() == Empty */
	if ( pglwgeom_is_empty(geom1) )
		PG_RETURN_POINTER(geom1);

	srid = pglwgeom_get_srid(geom1);

	initGEOS(lwnotice, lwgeom_geos_error);

	PROFSTART(PROF_P2G1);
	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1 );
	PROFSTOP(PROF_P2G1);
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}

	PROFSTART(PROF_GRUN);
	g3 = (GEOSGeometry *)GEOSBoundary(g1);
	PROFSTOP(PROF_GRUN);

	if (g3 == NULL)
	{
		GEOSGeom_destroy(g1);
		lwerror("GEOSBoundary: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL(); /* never get here */
	}

	POSTGIS_DEBUGF(3, "result: %s", GEOSGeomToWKT(g3));

	GEOSSetSRID(g3, srid);

	PROFSTART(PROF_G2P);
	result = GEOS2POSTGIS(g3, pglwgeom_has_z(geom1));

	PROFSTART(PROF_P2G1);

	if (result == NULL)
	{
		GEOSGeom_destroy(g1);

		GEOSGeom_destroy(g3);
		elog(NOTICE,"GEOS2POSTGIS threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /* never get here */
	}

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g3);

	/* compressType(result);   */

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, NULL, result);

	PG_FREE_IF_COPY(geom1, 0);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(convexhull);
Datum convexhull(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	GEOSGeometry *g1, *g3;
	PG_LWGEOM *result;
	LWGEOM *lwout;
	int srid;
	BOX2DFLOAT4 bbox;

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* Empty.ConvexHull() == Empty */
	if ( pglwgeom_is_empty(geom1) )
		PG_RETURN_POINTER(geom1);

	srid = pglwgeom_get_srid(geom1);

	initGEOS(lwnotice, lwgeom_geos_error);

	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);

	if ( 0 == g1 )   /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}

	g3 = (GEOSGeometry *)GEOSConvexHull(g1);

	if (g3 == NULL)
	{
		GEOSGeom_destroy(g1);
		lwerror("GEOSConvexHull: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL(); /* never get here */
	}


	POSTGIS_DEBUGF(3, "result: %s", GEOSGeomToWKT(g3));

	GEOSSetSRID(g3, srid);

	lwout = GEOS2LWGEOM(g3, pglwgeom_has_z(geom1));

	if (lwout == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g3);
		elog(ERROR,"convexhull() failed to convert GEOS geometry to LWGEOM");
		PG_RETURN_NULL(); /* never get here */
	}

	/* Copy input bbox if any */
	if ( pglwgeom_getbox2d_p(geom1, &bbox) )
	{
#ifdef GSERIALIZED_ON
		/* Force the box to have the same dimensionality as the lwgeom */
		bbox.flags = lwout->flags;
		lwout->bbox = gbox_copy(&bbox);
#else
		lwout->bbox = gbox_from_box2df(lwout->flags, &bbox);
#endif
	}

	result = pglwgeom_serialize(lwout);

	if (result == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g3);
		elog(ERROR,"GEOS convexhull() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /* never get here */
	}
	lwgeom_release(lwout);
	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g3);


	PG_FREE_IF_COPY(geom1, 0);
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(topologypreservesimplify);
Datum topologypreservesimplify(PG_FUNCTION_ARGS)
{
	PG_LWGEOM	*geom1;
	double	tolerance;
	GEOSGeometry *g1, *g3;
	PG_LWGEOM *result;

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	tolerance = PG_GETARG_FLOAT8(1);

	/* Empty.Simplify() == Empty */
	if ( pglwgeom_is_empty(geom1) )
		PG_RETURN_POINTER(geom1);

	initGEOS(lwnotice, lwgeom_geos_error);

	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}

	g3 = GEOSTopologyPreserveSimplify(g1,tolerance);

	if (g3 == NULL)
	{
		GEOSGeom_destroy(g1);
		lwerror("GEOSTopologyPreserveSimplify: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL(); /* never get here */
	}


	POSTGIS_DEBUGF(3, "result: %s", GEOSGeomToWKT(g3));

	GEOSSetSRID(g3, pglwgeom_get_srid(geom1));

	result = GEOS2POSTGIS(g3, pglwgeom_has_z(geom1));

	if (result == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g3);
		elog(ERROR,"GEOS topologypreservesimplify() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /* never get here */
	}
	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g3);

	PG_FREE_IF_COPY(geom1, 0);
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(buffer);
Datum buffer(PG_FUNCTION_ARGS)
{
	PG_LWGEOM	*geom1;
	double	size;
	GEOSGeometry *g1, *g3;
	PG_LWGEOM *result;
	int quadsegs = 8; /* the default */
	int nargs;
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
	static const double DEFAULT_MITRE_LIMIT = 5.0;
	static const int DEFAULT_ENDCAP_STYLE = ENDCAP_ROUND;
	static const int DEFAULT_JOIN_STYLE = JOIN_ROUND;

	double mitreLimit = DEFAULT_MITRE_LIMIT;
	int endCapStyle = DEFAULT_ENDCAP_STYLE;
	int joinStyle  = DEFAULT_JOIN_STYLE;
	char *param;
	char *params = NULL;

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	size = PG_GETARG_FLOAT8(1);
	
	/* Empty.Buffer() == Empty */
	if ( pglwgeom_is_empty(geom1) )
		PG_RETURN_POINTER(geom1);
	
	nargs = PG_NARGS();

	initGEOS(lwnotice, lwgeom_geos_error);

	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}

	if (nargs > 2)
	{
		/* We strdup `cause we're going to modify it */
		params = pstrdup(PG_GETARG_CSTRING(2));

		POSTGIS_DEBUGF(3, "Params: %s", params);

		for (param=params; ; param=NULL)
		{
			char *key, *val;
			param = strtok(param, " ");
			if ( param == NULL ) break;
			POSTGIS_DEBUGF(3, "Param: %s", param);

			key = param;
			val = strchr(key, '=');
			if ( val == NULL || *(val+1) == '\0' )
			{
				lwerror("Missing value for buffer "
				        "parameter %s", key);
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
					lwerror("Invalid buffer end cap "
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
					lwerror("Invalid buffer end cap "
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
			else
			{
				lwerror("Invalid buffer parameter: %s (accept: "
				        "'endcap', 'join', 'mitre_limit', "
				        "'miter_limit and "
				        "'quad_segs')", key);
				break;
			}
		}

		pfree(params); /* was pstrduped */

		POSTGIS_DEBUGF(3, "endCap:%d joinStyle:%d mitreLimit:%g",
		               endCapStyle, joinStyle, mitreLimit);

	}

#if POSTGIS_GEOS_VERSION >= 32

	g3 = GEOSBufferWithStyle(g1, size, quadsegs,
	                         endCapStyle, joinStyle, mitreLimit);

#else /* POSTGIS_GEOS_VERSION < 32 */

	if ( mitreLimit != DEFAULT_MITRE_LIMIT ||
	        endCapStyle != DEFAULT_ENDCAP_STYLE ||
	        joinStyle != DEFAULT_JOIN_STYLE )
	{
		lwerror("The GEOS version this postgis binary "
		        "was compiled against (%d) doesn't support "
		        "specifying a mitre limit != %d or styles different "
		        "from 'round' (needs 3.2 or higher)",
		        DEFAULT_MITRE_LIMIT, POSTGIS_GEOS_VERSION);
	}

	g3 = GEOSBuffer(g1,size,quadsegs);

#endif /* POSTGIS_GEOS_VERSION < 32 */

	if (g3 == NULL)
	{
		GEOSGeom_destroy(g1);
		lwerror("GEOSBuffer: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL(); /* never get here */
	}

	POSTGIS_DEBUGF(3, "result: %s", GEOSGeomToWKT(g3));

	GEOSSetSRID(g3, pglwgeom_get_srid(geom1));

	result = GEOS2POSTGIS(g3, pglwgeom_has_z(geom1));

	if (result == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g3);
		elog(ERROR,"GEOS buffer() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /* never get here */
	}
	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g3);

	PG_FREE_IF_COPY(geom1, 0);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(intersection);
Datum intersection(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeometry *g1, *g2, *g3;
	PG_LWGEOM *result;
	int is3d;
	int srid;

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	/* A.Intersection(Empty) == Empty */
	if ( pglwgeom_is_empty(geom2) )
		PG_RETURN_POINTER(geom2);

	/* Empty.Intersection(A) == Empty */
	if ( pglwgeom_is_empty(geom1) )
		PG_RETURN_POINTER(geom1);

	is3d = ( pglwgeom_has_z(geom1) ) ||
	       ( pglwgeom_has_z(geom2) );

	srid = pglwgeom_get_srid(geom1);
	error_if_srid_mismatch(srid, pglwgeom_get_srid(geom2));

	initGEOS(lwnotice, lwgeom_geos_error);

	POSTGIS_DEBUG(3, "intersection() START");

	PROFSTART(PROF_P2G1);
	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	PROFSTOP(PROF_P2G1);
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}

	PROFSTART(PROF_P2G2);
	g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
	PROFSTOP(PROF_P2G2);
	if ( 0 == g2 )   /* exception thrown at construction */
	{
		lwerror("Second argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		GEOSGeom_destroy(g1);
		PG_RETURN_NULL();
	}

	POSTGIS_DEBUG(3, " constructed geometrys - calling geos");
	POSTGIS_DEBUGF(3, " g1 = %s", GEOSGeomToWKT(g1));
	POSTGIS_DEBUGF(3, " g2 = %s", GEOSGeomToWKT(g2));
	/*POSTGIS_DEBUGF(3, "g2 is valid = %i",GEOSisvalid(g2)); */
	/*POSTGIS_DEBUGF(3, "g1 is valid = %i",GEOSisvalid(g1)); */

	PROFSTART(PROF_GRUN);
	g3 = GEOSIntersection(g1,g2);
	PROFSTOP(PROF_GRUN);

	POSTGIS_DEBUG(3, " intersection finished");

	if (g3 == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		lwerror("GEOSIntersection: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL(); /* never get here */
	}

	POSTGIS_DEBUGF(3, "result: %s", GEOSGeomToWKT(g3) ) ;

	GEOSSetSRID(g3, srid);

	PROFSTART(PROF_G2P);
	result = GEOS2POSTGIS(g3, is3d);
	PROFSTOP(PROF_G2P);

	if (result == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		GEOSGeom_destroy(g3);
		lwerror("GEOS Intersection() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /* never get here */
	}

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	GEOSGeom_destroy(g3);

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, geom2, result);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_POINTER(result);
}

/**
 * @example difference {@link #difference} - SELECT difference(
 *      'POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))',
 *	'POLYGON((5 5, 15 5, 15 7, 5 7, 5 5))');
 */
PG_FUNCTION_INFO_V1(difference);
Datum difference(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeometry *g1, *g2, *g3;
	PG_LWGEOM *result;
	int is3d;
	int srid;

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	/* A.Difference(Empty) == A */
	if ( pglwgeom_is_empty(geom2) )
		PG_RETURN_POINTER(geom1);

	/* Empty.Intersection(A) == Empty */
	if ( pglwgeom_is_empty(geom1) )
		PG_RETURN_POINTER(geom1);

	is3d = ( pglwgeom_has_z(geom1) ) ||
	       ( pglwgeom_has_z(geom2) );

	srid = pglwgeom_get_srid(geom1);
	error_if_srid_mismatch(srid, pglwgeom_get_srid(geom2));

	initGEOS(lwnotice, lwgeom_geos_error);

	PROFSTART(PROF_P2G1);
	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	PROFSTOP(PROF_P2G1);
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}

	PROFSTART(PROF_P2G2);
	g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
	PROFSTOP(PROF_P2G2);
	if ( 0 == g2 )   /* exception thrown at construction */
	{
		GEOSGeom_destroy(g1);
		lwerror("Second argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}

	PROFSTART(PROF_GRUN);
	g3 = GEOSDifference(g1,g2);
	PROFSTOP(PROF_GRUN);

	if (g3 == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		lwerror("GEOSDifference: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL(); /* never get here */
	}

	POSTGIS_DEBUGF(3, "result: %s", GEOSGeomToWKT(g3) ) ;

	GEOSSetSRID(g3, srid);

	PROFSTART(PROF_G2P);
	result = GEOS2POSTGIS(g3, is3d);
	PROFSTOP(PROF_G2P);

	if (result == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		GEOSGeom_destroy(g3);
		elog(ERROR,"GEOS difference() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /* never get here */
	}

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	GEOSGeom_destroy(g3);

	/* compressType(result); */

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, geom2, result);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_POINTER(result);
}


/**
	@example pointonsurface - {@link #pointonsurface} SELECT pointonsurface('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
*/
PG_FUNCTION_INFO_V1(pointonsurface);
Datum pointonsurface(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	GEOSGeometry *g1, *g3;
	PG_LWGEOM *result;

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* Empty.PointOnSurface == Empty */
	if ( pglwgeom_is_empty(geom1) )
		PG_RETURN_POINTER(geom1);

	initGEOS(lwnotice, lwgeom_geos_error);

	PROFSTART(PROF_P2G1);
	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	PROFSTOP(PROF_P2G1);
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		elog(WARNING, "GEOSPointOnSurface(): %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}

	PROFSTART(PROF_GRUN);
	g3 = GEOSPointOnSurface(g1);
	PROFSTOP(PROF_GRUN);

	if (g3 == NULL)
	{
		GEOSGeom_destroy(g1);
		lwerror("GEOSPointOnSurface: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL(); /* never get here */
	}

	POSTGIS_DEBUGF(3, "result: %s", GEOSGeomToWKT(g3) ) ;

	GEOSSetSRID(g3, pglwgeom_get_srid(geom1));

	PROFSTART(PROF_G2P);
	result = GEOS2POSTGIS(g3, pglwgeom_has_z(geom1));
	PROFSTOP(PROF_G2P);

	if (result == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g3);
		elog(ERROR,"GEOS pointonsurface() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /* never get here */
	}

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g3);

	/* compressType(result);  */

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, NULL, result);

	PG_FREE_IF_COPY(geom1, 0);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(centroid);
Datum centroid(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom, *result;
	GEOSGeometry *geosgeom, *geosresult;

	PROFSTART(PROF_QRUN);

	geom = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* Empty.Centroid() == Empty */
	if ( pglwgeom_is_empty(geom) )
		PG_RETURN_POINTER(geom);

	initGEOS(lwnotice, lwgeom_geos_error);

	PROFSTART(PROF_P2G1);
	geosgeom = (GEOSGeometry *)POSTGIS2GEOS(geom);
	PROFSTOP(PROF_P2G1);
	if ( 0 == geosgeom )   /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}

	PROFSTART(PROF_GRUN);
	geosresult = GEOSGetCentroid(geosgeom);
	PROFSTOP(PROF_GRUN);

	if ( geosresult == NULL )
	{
		GEOSGeom_destroy(geosgeom);
		lwerror("GEOSGetCentroid: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}

	GEOSSetSRID(geosresult, pglwgeom_get_srid(geom));

	PROFSTART(PROF_G2P);
	result = GEOS2POSTGIS(geosresult, pglwgeom_has_z(geom));
	PROFSTOP(PROF_G2P);

	if (result == NULL)
	{
		GEOSGeom_destroy(geosgeom);
		GEOSGeom_destroy(geosresult);
		elog(ERROR,"Error in GEOS-PGIS conversion");
		PG_RETURN_NULL();
	}
	GEOSGeom_destroy(geosgeom);
	GEOSGeom_destroy(geosresult);

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom, NULL, result);

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result);
}



/*---------------------------------------------*/


/**
 * @brief Throws an ereport ERROR if either geometry is a COLLECTIONTYPE.  Additionally
 * 		displays a HINT of the first 80 characters of the WKT representation of the
 * 		problematic geometry so a user knows which parameter and which geometry
 * 		is causing the problem.
 */
void errorIfGeometryCollection(PG_LWGEOM *g1, PG_LWGEOM *g2)
{
	int t1 = pglwgeom_get_type(g1);
	int t2 = pglwgeom_get_type(g2);

	char *hintmsg;
	char *hintwkt;
	size_t hintsz;
	LWGEOM *lwgeom;

	if ( t1 == COLLECTIONTYPE)
	{
		lwgeom = pglwgeom_deserialize(g1);
		hintwkt = lwgeom_to_wkt(lwgeom, WKT_SFSQL, DBL_DIG, &hintsz);
		hintmsg = lwmessage_truncate(hintwkt, 0, hintsz-1, 80, 1);
		ereport(ERROR,
		        (errmsg("Relate Operation called with a LWGEOMCOLLECTION type.  This is unsupported."),
		         errhint("Change argument 1: '%s'", hintmsg))
		       );
		pfree(hintwkt);
		pfree(hintmsg);
		lwgeom_free(lwgeom);
	}
	else if (t2 == COLLECTIONTYPE)
	{
		lwgeom = pglwgeom_deserialize(g2);
		hintwkt = lwgeom_to_wkt(lwgeom, WKT_SFSQL, DBL_DIG, &hintsz);
		hintmsg = lwmessage_truncate(hintwkt, 0, hintsz-1, 80, 1);
		ereport(ERROR,
		        (errmsg("Relate Operation called with a LWGEOMCOLLECTION type.  This is unsupported."),
		         errhint("Change argument 2: '%s'", hintmsg))
		       );
		pfree(hintwkt);
		pfree(hintmsg);
		lwgeom_free(lwgeom);
	}
}

PG_FUNCTION_INFO_V1(isvalid);
Datum isvalid(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	LWGEOM *lwgeom;
	bool result;
	GEOSGeom g1;

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* Empty.IsValid() == TRUE */
	if ( pglwgeom_is_empty(geom1) )
		PG_RETURN_BOOL(true);

	initGEOS(lwnotice, lwgeom_geos_error);

	PROFSTART(PROF_P2G1);

	lwgeom = pglwgeom_deserialize(geom1);
	if ( ! lwgeom )
	{
		lwerror("unable to deserialize input");
	}
	g1 = LWGEOM2GEOS(lwgeom);
	if ( ! g1 )
	{
		/* should we drop the following
		 * notice now that we have ST_isValidReason ?
		 */
		lwnotice("%s", lwgeom_geos_errmsg);
		lwgeom_release(lwgeom);
		PG_RETURN_BOOL(FALSE);
	}
	lwgeom_release(lwgeom);

	PROFSTOP(PROF_P2G1);

	PROFSTART(PROF_GRUN);
	result = GEOSisValid(g1);
	PROFSTOP(PROF_GRUN);

	GEOSGeom_destroy(g1);
	if (result == 2)
	{
		elog(ERROR,"GEOS isvalid() threw an error!");
		PG_RETURN_NULL(); /*never get here */
	}

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, NULL, NULL);

	PG_FREE_IF_COPY(geom1, 0);

	PG_RETURN_BOOL(result);
}

/*
** IsValidReason is only available in the GEOS
** C API > version 3.0
*/
PG_FUNCTION_INFO_V1(isvalidreason);
Datum isvalidreason(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = NULL;
	char *reason_str = NULL;
	text *result = NULL;
	const GEOSGeometry *g1 = NULL;

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	initGEOS(lwnotice, lwgeom_geos_error);

	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom);
	if ( g1 )
	{
		reason_str = GEOSisValidReason(g1);
		GEOSGeom_destroy((GEOSGeometry *)g1);
	}
	else
	{
		/* we don't use pstrdup here as we free later */
		reason_str = strdup(lwgeom_geos_errmsg);
	}


	if (reason_str == NULL)
	{
		elog(ERROR,"GEOS isvalidreason() threw an error!");
		PG_RETURN_NULL(); /* never get here */
	}
	
	result = cstring2text(reason_str);
	/* No pfree because GEOS did a standard malloc on the reason_str */
	free(reason_str);

	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_POINTER(result);

}

/*
** IsValidDetail is only available in the GEOS
** C API >= version 3.3
*/
PG_FUNCTION_INFO_V1(isvaliddetail);
Datum isvaliddetail(PG_FUNCTION_ARGS)
{
#if POSTGIS_GEOS_VERSION < 33
	lwerror("The GEOS version this postgis binary "
	        "was compiled against (%d) doesn't support "
	        "'isValidDetail' function (3.3.0+ required)",
	        POSTGIS_GEOS_VERSION);
	PG_RETURN_NULL();
#else /* POSTGIS_GEOS_VERSION >= 33 */

	PG_LWGEOM *geom = NULL;
	const GEOSGeometry *g1 = NULL;
	char *values[3]; /* valid bool, reason text, location geometry */
	char *geos_reason = NULL;
	char *reason = NULL;
	GEOSGeometry *geos_location = NULL;
	LWGEOM *location = NULL;
	char valid = 0;
	Datum result;
	TupleDesc tupdesc;
	HeapTuple tuple;
	AttInMetadata *attinmeta;
	int flags = 0;

	/*
	 * Build a tuple description for a
	 * valid_detail tuple
	 */
	tupdesc = RelationNameGetTupleDesc("valid_detail");
	if ( ! tupdesc )
	{
		lwerror("TYPE valid_detail not found");
		PG_RETURN_NULL();
	}

	/*
	 * generate attribute metadata needed later to produce
	 * tuples from raw C strings
	 */
	attinmeta = TupleDescGetAttInMetadata(tupdesc);

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	if ( PG_NARGS() > 1 && ! PG_ARGISNULL(1) ) {
		flags = PG_GETARG_INT32(1);
	}

	initGEOS(lwnotice, lwgeom_geos_error);

	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom);

	if ( g1 )
	{
		valid = GEOSisValidDetail(g1, flags,
			&geos_reason, &geos_location);
		GEOSGeom_destroy((GEOSGeometry *)g1);
		if ( geos_reason )
		{
			reason = pstrdup(geos_reason);
			GEOSFree(geos_reason);
		}
		if ( geos_location )
		{
			location = GEOS2LWGEOM(geos_location, GEOSHasZ(geos_location));
			GEOSGeom_destroy((GEOSGeometry *)geos_location);
		}

		if (valid == 2)
		{
			/* NOTE: should only happen on OOM or similar */
			lwerror("GEOS isvaliddetail() threw an exception!");
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
	values[2] =  location ? lwgeom_to_hexwkb(location, WKB_EXTENDED, 0) : 0;

	tuple = BuildTupleFromCStrings(attinmeta, values);
	result = HeapTupleGetDatum(tuple);

	PG_RETURN_HEAPTUPLEHEADER(result);

#endif /* POSTGIS_GEOS_VERSION >= 33 */
}

/**
 * overlaps(PG_LWGEOM g1,PG_LWGEOM g2)
 * @param g1
 * @param g2
 * @return  if GEOS::g1->overlaps(g2) returns true
 * @throw an error (elog(ERROR,...)) if GEOS throws an error
 */
PG_FUNCTION_INFO_V1(overlaps);
Datum overlaps(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeometry *g1, *g2;
	bool result;
	BOX2DFLOAT4 box1, box2;

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	error_if_srid_mismatch(pglwgeom_get_srid(geom1), pglwgeom_get_srid(geom2));

	/* A.Overlaps(Empty) == FALSE */
	if ( pglwgeom_is_empty(geom1) || pglwgeom_is_empty(geom2) )
		PG_RETURN_BOOL(false);

	/*
	 * short-circuit 1: if geom2 bounding box does not overlap
	 * geom1 bounding box we can prematurely return FALSE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( pglwgeom_getbox2d_p(geom1, &box1) &&
	        pglwgeom_getbox2d_p(geom2, &box2) )
	{
		if ( box2.xmax < box1.xmin ) PG_RETURN_BOOL(FALSE);
		if ( box2.xmin > box1.xmax ) PG_RETURN_BOOL(FALSE);
		if ( box2.ymax < box1.ymin ) PG_RETURN_BOOL(FALSE);
		if ( box2.ymin > box2.ymax ) PG_RETURN_BOOL(FALSE);
	}

	initGEOS(lwnotice, lwgeom_geos_error);

	PROFSTART(PROF_P2G1);
	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	PROFSTOP(PROF_P2G1);
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}

	PROFSTART(PROF_P2G2);
	g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
	PROFSTOP(PROF_P2G2);
	if ( 0 == g2 )   /* exception thrown at construction */
	{
		GEOSGeom_destroy(g1);
		lwerror("Second argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}

	PROFSTART(PROF_GRUN);
	result = GEOSOverlaps(g1,g2);
	PROFSTOP(PROF_GRUN);

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	if (result == 2)
	{
		lwerror("GEOSOverlaps: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL(); /* never get here */
	}

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, geom2, NULL);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(contains);
Datum contains(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeometry *g1, *g2;
	BOX2DFLOAT4 box1, box2;
	int type1, type2;
	LWGEOM *lwgeom;
	LWPOINT *point;
	RTREE_POLY_CACHE *poly_cache;
	MemoryContext old_context;
	bool result;
#ifdef PREPARED_GEOM
	PrepGeomCache *prep_cache;
#endif

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	error_if_srid_mismatch(pglwgeom_get_srid(geom1), pglwgeom_get_srid(geom2));

	/* A.Contains(Empty) == FALSE */
	if ( pglwgeom_is_empty(geom1) || pglwgeom_is_empty(geom2) )
		PG_RETURN_BOOL(false);

	POSTGIS_DEBUG(3, "contains called.");

	/*
	** short-circuit 1: if geom2 bounding box is not completely inside
	** geom1 bounding box we can prematurely return FALSE.
	** Do the test IFF BOUNDING BOX AVAILABLE.
	*/
	if ( pglwgeom_getbox2d_p(geom1, &box1) &&
	        pglwgeom_getbox2d_p(geom2, &box2) )
	{
		if ( ( box2.xmin < box1.xmin ) || ( box2.xmax > box1.xmax ) ||
		        ( box2.ymin < box1.ymin ) || ( box2.ymax > box1.ymax ) )
		{
			PG_RETURN_BOOL(FALSE);
		}
	}

	/*
	** short-circuit 2: if geom2 is a point and geom1 is a polygon
	** call the point-in-polygon function.
	*/
	type1 = pglwgeom_get_type(geom1);
	type2 = pglwgeom_get_type(geom2);
	if ((type1 == POLYGONTYPE || type1 == MULTIPOLYGONTYPE) && type2 == POINTTYPE)
	{
		POSTGIS_DEBUG(3, "Point in Polygon test requested...short-circuiting.");
		lwgeom = pglwgeom_deserialize(geom1);
		point = lwgeom_as_lwpoint(pglwgeom_deserialize(geom2));

		POSTGIS_DEBUGF(3, "Precall point_in_multipolygon_rtree %p, %p", lwgeom, point);

		/*
		 * Switch the context to the function-scope context,
		 * retrieve the appropriate cache object, cache it for
		 * future use, then switch back to the local context.
		 */
		old_context = MemoryContextSwitchTo(fcinfo->flinfo->fn_mcxt);
		poly_cache = retrieveCache(lwgeom, geom1, fcinfo->flinfo->fn_extra);
		fcinfo->flinfo->fn_extra = poly_cache;
		MemoryContextSwitchTo(old_context);

		if ( poly_cache->ringIndices )
		{
			result = point_in_multipolygon_rtree(poly_cache->ringIndices, poly_cache->polyCount, poly_cache->ringCounts, point);
		}
		else if ( type1 == POLYGONTYPE )
		{
			result = point_in_polygon((LWPOLY*)lwgeom, point);
		}
		else if ( type1 == MULTIPOLYGONTYPE )
		{
			result = point_in_multipolygon((LWMPOLY*)lwgeom, point);
		}
		else
		{
			/* Gulp! Should not be here... */
			elog(ERROR,"Type isn't poly or multipoly!");
			PG_RETURN_NULL();
		}
		PG_FREE_IF_COPY(geom1, 0);
		PG_FREE_IF_COPY(geom2, 1);
		lwgeom_release((LWGEOM *)lwgeom);
		lwgeom_release((LWGEOM *)point);
		if ( result == 1 ) /* completely inside */
		{
			PG_RETURN_BOOL(TRUE);
		}
		else
		{
			PG_RETURN_BOOL(FALSE);
		}
	}
	else
	{
		POSTGIS_DEBUGF(3, "Contains: type1: %d, type2: %d", type1, type2);
	}

	initGEOS(lwnotice, lwgeom_geos_error);

#ifdef PREPARED_GEOM
	prep_cache = GetPrepGeomCache( fcinfo, geom1, 0 );

	if ( prep_cache && prep_cache->prepared_geom && prep_cache->argnum == 1 )
	{
		g1 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
		if ( 0 == g1 )   /* exception thrown at construction */
		{
			lwerror("Geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
			PG_RETURN_NULL();
		}
		POSTGIS_DEBUG(4, "containsPrepared: cache is live, running preparedcontains");
		result = GEOSPreparedContains( prep_cache->prepared_geom, g1);
		GEOSGeom_destroy(g1);
	}
	else
#endif
	{
		g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
		if ( 0 == g1 )   /* exception thrown at construction */
		{
			lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
			PG_RETURN_NULL();
		}
		g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
		if ( 0 == g2 )   /* exception thrown at construction */
		{
			lwerror("Second argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
			GEOSGeom_destroy(g1);
			PG_RETURN_NULL();
		}
		POSTGIS_DEBUG(4, "containsPrepared: cache is not ready, running standard contains");
		result = GEOSContains( g1, g2);
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
	}

	if (result == 2)
	{
		lwerror("GEOSContains: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL(); /* never get here */
	}

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);

}

PG_FUNCTION_INFO_V1(containsproperly);
Datum containsproperly(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *				geom1;
	PG_LWGEOM *				geom2;
	bool 					result;
	BOX2DFLOAT4 			box1, box2;
#ifdef PREPARED_GEOM
	PrepGeomCache *	prep_cache;
#endif

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	error_if_srid_mismatch(pglwgeom_get_srid(geom1), pglwgeom_get_srid(geom2));

	/* A.ContainsProperly(Empty) == FALSE */
	if ( pglwgeom_is_empty(geom1) || pglwgeom_is_empty(geom2) )
		PG_RETURN_BOOL(false);

	/*
	* short-circuit: if geom2 bounding box is not completely inside
	* geom1 bounding box we can prematurely return FALSE.
	* Do the test IFF BOUNDING BOX AVAILABLE.
	*/
	if ( pglwgeom_getbox2d_p(geom1, &box1) &&
	        pglwgeom_getbox2d_p(geom2, &box2) )
	{
		if (( box2.xmin < box1.xmin ) || ( box2.xmax > box1.xmax ) ||
		        ( box2.ymin < box1.ymin ) || ( box2.ymax > box1.ymax ))
			PG_RETURN_BOOL(FALSE);
	}

	initGEOS(lwnotice, lwgeom_geos_error);

#ifdef PREPARED_GEOM
	prep_cache = GetPrepGeomCache( fcinfo, geom1, 0 );

	if ( prep_cache && prep_cache->prepared_geom && prep_cache->argnum == 1 )
	{
		GEOSGeometry *g = (GEOSGeometry *)POSTGIS2GEOS(geom2);
		if ( 0 == g )   /* exception thrown at construction */
		{
			lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
			PG_RETURN_NULL();
		}
		result = GEOSPreparedContainsProperly( prep_cache->prepared_geom, g);
		GEOSGeom_destroy(g);
	}
	else
#endif
	{
		GEOSGeometry *g2;
		GEOSGeometry *g1;

		g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
		if ( 0 == g1 )   /* exception thrown at construction */
		{
			lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
			PG_RETURN_NULL();
		}
		g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
		if ( 0 == g2 )   /* exception thrown at construction */
		{
			lwerror("Second argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
			GEOSGeom_destroy(g1);
			PG_RETURN_NULL();
		}
		result = GEOSRelatePattern( g1, g2, "T**FF*FF*" );
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
	}

	if (result == 2)
	{
		lwerror("GEOSContains: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL(); /* never get here */
	}

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}

/*
 * Described at
 * http://lin-ear-th-inking.blogspot.com/2007/06/subtleties-of-ogc-covers-spatial.html
 */
PG_FUNCTION_INFO_V1(covers);
Datum covers(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	bool result;
	BOX2DFLOAT4 box1, box2;
	int type1, type2;
	LWGEOM *lwgeom;
	LWPOINT *point;
	RTREE_POLY_CACHE *poly_cache;
	MemoryContext old_context;
#ifdef PREPARED_GEOM
	PrepGeomCache *prep_cache;
#endif

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	/* A.Covers(Empty) == FALSE */
	if ( pglwgeom_is_empty(geom1) || pglwgeom_is_empty(geom2) )
		PG_RETURN_BOOL(false);

	errorIfGeometryCollection(geom1,geom2);
	error_if_srid_mismatch(pglwgeom_get_srid(geom1), pglwgeom_get_srid(geom2));

	/*
	 * short-circuit 1: if geom2 bounding box is not completely inside
	 * geom1 bounding box we can prematurely return FALSE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( pglwgeom_getbox2d_p(geom1, &box1) &&
	        pglwgeom_getbox2d_p(geom2, &box2) )
	{
		if (( box2.xmin < box1.xmin ) || ( box2.xmax > box1.xmax ) ||
		        ( box2.ymin < box1.ymin ) || ( box2.ymax > box1.ymax ))
		{
			PG_RETURN_BOOL(FALSE);
		}
	}
	/*
	 * short-circuit 2: if geom2 is a point and geom1 is a polygon
	 * call the point-in-polygon function.
	 */
	type1 = pglwgeom_get_type(geom1);
	type2 = pglwgeom_get_type(geom2);
	if ((type1 == POLYGONTYPE || type1 == MULTIPOLYGONTYPE) && type2 == POINTTYPE)
	{
		POSTGIS_DEBUG(3, "Point in Polygon test requested...short-circuiting.");

		lwgeom = pglwgeom_deserialize(geom1);
		point = lwgeom_as_lwpoint(pglwgeom_deserialize(geom2));

		POSTGIS_DEBUGF(3, "Precall point_in_multipolygon_rtree %p, %p", lwgeom, point);

		/*
		 * Switch the context to the function-scope context,
		 * retrieve the appropriate cache object, cache it for
		 * future use, then switch back to the local context.
		 */
		old_context = MemoryContextSwitchTo(fcinfo->flinfo->fn_mcxt);
		poly_cache = retrieveCache(lwgeom, geom1, fcinfo->flinfo->fn_extra);
		fcinfo->flinfo->fn_extra = poly_cache;
		MemoryContextSwitchTo(old_context);

		if ( poly_cache->ringIndices )
		{
			result = point_in_multipolygon_rtree(poly_cache->ringIndices, poly_cache->polyCount, poly_cache->ringCounts, point);
		}
		else if ( type1 == POLYGONTYPE )
		{
			result = point_in_polygon((LWPOLY*)lwgeom, point);
		}
		else if ( type1 == MULTIPOLYGONTYPE )
		{
			result = point_in_multipolygon((LWMPOLY*)lwgeom, point);
		}
		else
		{
			/* Gulp! Should not be here... */
			elog(ERROR,"Type isn't poly or multipoly!");
			PG_RETURN_NULL();
		}

		PG_FREE_IF_COPY(geom1, 0);
		PG_FREE_IF_COPY(geom2, 1);
		lwgeom_release((LWGEOM *)lwgeom);
		lwgeom_release((LWGEOM *)point);
		if ( result != -1 ) /* not outside */
		{
			PG_RETURN_BOOL(TRUE);
		}
		else
		{
			PG_RETURN_BOOL(FALSE);
		}
	}
	else
	{
		POSTGIS_DEBUGF(3, "Covers: type1: %d, type2: %d", type1, type2);
	}

	initGEOS(lwnotice, lwgeom_geos_error);

#ifdef PREPARED_GEOM
	prep_cache = GetPrepGeomCache( fcinfo, geom1, 0 );

	if ( prep_cache && prep_cache->prepared_geom && prep_cache->argnum == 1 )
	{
		GEOSGeometry *g1 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
		if ( 0 == g1 )   /* exception thrown at construction */
		{
			lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
			PG_RETURN_NULL();
		}
		result = GEOSPreparedCovers( prep_cache->prepared_geom, g1);
		GEOSGeom_destroy(g1);
	}
	else
#endif
	{
		GEOSGeometry *g1;
		GEOSGeometry *g2;

		g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
		if ( 0 == g1 )   /* exception thrown at construction */
		{
			lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
			PG_RETURN_NULL();
		}
		g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
		if ( 0 == g2 )   /* exception thrown at construction */
		{
			lwerror("Second argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
			GEOSGeom_destroy(g1);
			PG_RETURN_NULL();
		}
		result = GEOSRelatePattern( g1, g2, "******FF*" );
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
	}

	if (result == 2)
	{
		lwerror("GEOSCovers: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL(); /* never get here */
	}

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);

}



PG_FUNCTION_INFO_V1(within);
Datum within(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeometry *g1, *g2;
	bool result;
	BOX2DFLOAT4 box1, box2;
	LWGEOM *lwgeom;
	LWPOINT *point;
	int type1, type2;
	MemoryContext old_context;
	RTREE_POLY_CACHE *poly_cache;

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	error_if_srid_mismatch(pglwgeom_get_srid(geom1), pglwgeom_get_srid(geom2));

	/* A.Within(Empty) == FALSE */
	if ( pglwgeom_is_empty(geom1) || pglwgeom_is_empty(geom2) )
		PG_RETURN_BOOL(false);

	/*
	 * short-circuit 1: if geom1 bounding box is not completely inside
	 * geom2 bounding box we can prematurely return FALSE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( pglwgeom_getbox2d_p(geom1, &box1) &&
	        pglwgeom_getbox2d_p(geom2, &box2) )
	{
		if ( ( box1.xmin < box2.xmin ) || ( box1.xmax > box2.xmax ) ||
		        ( box1.ymin < box2.ymin ) || ( box1.ymax > box2.ymax ) )
		{
			PG_RETURN_BOOL(FALSE);
		}
	}
	/*
	 * short-circuit 2: if geom1 is a point and geom2 is a polygon
	 * call the point-in-polygon function.
	 */
	type1 = pglwgeom_get_type(geom1);
	type2 = pglwgeom_get_type(geom2);
	if ((type2 == POLYGONTYPE || type2 == MULTIPOLYGONTYPE) && type1 == POINTTYPE)
	{
		POSTGIS_DEBUG(3, "Point in Polygon test requested...short-circuiting.");

		point = lwgeom_as_lwpoint(pglwgeom_deserialize(geom1));
		lwgeom = pglwgeom_deserialize(geom2);

		/*
		 * Switch the context to the function-scope context,
		 * retrieve the appropriate cache object, cache it for
		 * future use, then switch back to the local context.
		 */
		old_context = MemoryContextSwitchTo(fcinfo->flinfo->fn_mcxt);
		poly_cache = retrieveCache(lwgeom, geom2, fcinfo->flinfo->fn_extra);
		fcinfo->flinfo->fn_extra = poly_cache;
		MemoryContextSwitchTo(old_context);

		if ( poly_cache->ringIndices )
		{
			result = point_in_multipolygon_rtree(poly_cache->ringIndices, poly_cache->polyCount, poly_cache->ringCounts, point);
		}
		else if ( type2 == POLYGONTYPE )
		{
			result = point_in_polygon((LWPOLY*)lwgeom, point);
		}
		else if ( type2 == MULTIPOLYGONTYPE )
		{
			result = point_in_multipolygon((LWMPOLY*)lwgeom, point);
		}
		else
		{
			/* Gulp! Should not be here... */
			elog(ERROR,"Type isn't poly or multipoly!");
			PG_RETURN_NULL();
		}

		PG_FREE_IF_COPY(geom1, 0);
		PG_FREE_IF_COPY(geom2, 1);
		lwgeom_release((LWGEOM *)lwgeom);
		lwgeom_release((LWGEOM *)point);
		if ( result == 1 ) /* completely inside */
		{
			PG_RETURN_BOOL(TRUE);
		}
		else
		{
			PG_RETURN_BOOL(FALSE);
		}
	}

	initGEOS(lwnotice, lwgeom_geos_error);

	PROFSTART(PROF_P2G1);
	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	PROFSTOP(PROF_P2G1);
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}

	PROFSTART(PROF_P2G2);
	g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
	PROFSTOP(PROF_P2G2);
	if ( 0 == g2 )   /* exception thrown at construction */
	{
		lwerror("Second argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		GEOSGeom_destroy(g1);
		PG_RETURN_NULL();
	}

	PROFSTART(PROF_GRUN);
	result = GEOSWithin(g1,g2);
	PROFSTOP(PROF_GRUN);

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (result == 2)
	{
		lwerror("GEOSWithin: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL(); /* never get here */
	}

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, geom2, NULL);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}


/*
 * Described at:
 * http://lin-ear-th-inking.blogspot.com/2007/06/subtleties-of-ogc-covers-spatial.html
 */
PG_FUNCTION_INFO_V1(coveredby);
Datum coveredby(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeometry *g1, *g2;
	bool result;
	BOX2DFLOAT4 box1, box2;
	LWGEOM *lwgeom;
	LWPOINT *point;
	int type1, type2;
	MemoryContext old_context;
	RTREE_POLY_CACHE *poly_cache;
	char *patt = "**F**F***";

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	error_if_srid_mismatch(pglwgeom_get_srid(geom1), pglwgeom_get_srid(geom2));

	/* A.CoveredBy(Empty) == FALSE */
	if ( pglwgeom_is_empty(geom1) || pglwgeom_is_empty(geom2) )
		PG_RETURN_BOOL(false);

	/*
	 * short-circuit 1: if geom1 bounding box is not completely inside
	 * geom2 bounding box we can prematurely return FALSE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( pglwgeom_getbox2d_p(geom1, &box1) &&
	        pglwgeom_getbox2d_p(geom2, &box2) )
	{
		if ( ( box1.xmin < box2.xmin ) || ( box1.xmax > box2.xmax ) ||
		        ( box1.ymin < box2.ymin ) || ( box1.ymax > box2.ymax ) )
		{
			PG_RETURN_BOOL(FALSE);
		}

		POSTGIS_DEBUG(3, "bounding box short-circuit missed.");
	}
	/*
	 * short-circuit 2: if geom1 is a point and geom2 is a polygon
	 * call the point-in-polygon function.
	 */
	type1 = pglwgeom_get_type(geom1);
	type2 = pglwgeom_get_type(geom2);
	if ((type2 == POLYGONTYPE || type2 == MULTIPOLYGONTYPE) && type1 == POINTTYPE)
	{
		POSTGIS_DEBUG(3, "Point in Polygon test requested...short-circuiting.");

		point = lwgeom_as_lwpoint(pglwgeom_deserialize(geom1));
		lwgeom = pglwgeom_deserialize(geom2);

		/*
		 * Switch the context to the function-scope context,
		 * retrieve the appropriate cache object, cache it for
		 * future use, then switch back to the local context.
		 */
		old_context = MemoryContextSwitchTo(fcinfo->flinfo->fn_mcxt);
		poly_cache = retrieveCache(lwgeom, geom2, fcinfo->flinfo->fn_extra);
		fcinfo->flinfo->fn_extra = poly_cache;
		MemoryContextSwitchTo(old_context);

		if ( poly_cache->ringIndices )
		{
			result = point_in_multipolygon_rtree(poly_cache->ringIndices, poly_cache->polyCount, poly_cache->ringCounts, point);
		}
		else if ( type2 == POLYGONTYPE )
		{
			result = point_in_polygon((LWPOLY*)lwgeom, point);
		}
		else if ( type2 == MULTIPOLYGONTYPE )
		{
			result = point_in_multipolygon((LWMPOLY*)lwgeom, point);
		}
		else
		{
			/* Gulp! Should not be here... */
			elog(ERROR,"Type isn't poly or multipoly!");
			PG_RETURN_NULL();
		}

		PG_FREE_IF_COPY(geom1, 0);
		PG_FREE_IF_COPY(geom2, 1);
		lwgeom_release((LWGEOM *)lwgeom);
		lwgeom_release((LWGEOM *)point);
		if ( result != -1 ) /* not outside */
		{
			PG_RETURN_BOOL(TRUE);
		}
		else
		{
			PG_RETURN_BOOL(FALSE);
		}
	}

	initGEOS(lwnotice, lwgeom_geos_error);

	PROFSTART(PROF_P2G1);
	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	PROFSTOP(PROF_P2G1);
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}

	PROFSTART(PROF_P2G2);
	g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
	PROFSTOP(PROF_P2G2);
	if ( 0 == g2 )   /* exception thrown at construction */
	{
		lwerror("Second argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		GEOSGeom_destroy(g1);
		PG_RETURN_NULL();
	}

	PROFSTART(PROF_GRUN);
	result = GEOSRelatePattern(g1,g2,patt);
	PROFSTOP(PROF_GRUN);

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (result == 2)
	{
		lwerror("GEOSCoveredBy: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL(); /* never get here */
	}

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, geom2, NULL);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}



PG_FUNCTION_INFO_V1(crosses);
Datum crosses(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeometry *g1, *g2;
	bool result;
	BOX2DFLOAT4 box1, box2;

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	error_if_srid_mismatch(pglwgeom_get_srid(geom1), pglwgeom_get_srid(geom2));

	/* A.Crosses(Empty) == FALSE */
	if ( pglwgeom_is_empty(geom1) || pglwgeom_is_empty(geom2) )
		PG_RETURN_BOOL(false);

	/*
	 * short-circuit 1: if geom2 bounding box does not overlap
	 * geom1 bounding box we can prematurely return FALSE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( pglwgeom_getbox2d_p(geom1, &box1) &&
	        pglwgeom_getbox2d_p(geom2, &box2) )
	{
		if ( ( box2.xmax < box1.xmin ) || ( box2.xmin > box1.xmax ) ||
		        ( box2.ymax < box1.ymin ) || ( box2.ymin > box2.ymax ) )
		{
			PG_RETURN_BOOL(FALSE);
		}
	}

	initGEOS(lwnotice, lwgeom_geos_error);

	PROFSTART(PROF_P2G1);
	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	PROFSTOP(PROF_P2G1);
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}

	PROFSTART(PROF_P2G2);
	g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
	PROFSTOP(PROF_P2G2);
	if ( 0 == g2 )   /* exception thrown at construction */
	{
		lwerror("Second argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		GEOSGeom_destroy(g1);
		PG_RETURN_NULL();
	}

	PROFSTART(PROF_GRUN);
	result = GEOSCrosses(g1,g2);
	PROFSTOP(PROF_GRUN);

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (result == 2)
	{
		lwerror("GEOSCrosses: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL(); /* never get here */
	}

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, geom2, NULL);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(intersects);
Datum intersects(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	PG_LWGEOM *serialized_poly;
	bool result;
	BOX2DFLOAT4 box1, box2;
	int type1, type2, polytype;
	LWPOINT *point;
	LWGEOM *lwgeom;
	MemoryContext old_context;
	RTREE_POLY_CACHE *poly_cache;
#ifdef PREPARED_GEOM
	PrepGeomCache *prep_cache;
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	error_if_srid_mismatch(pglwgeom_get_srid(geom1), pglwgeom_get_srid(geom2));

	/* A.Intersects(Empty) == FALSE */
	if ( pglwgeom_is_empty(geom1) || pglwgeom_is_empty(geom2) )
		PG_RETURN_BOOL(false);

	/*
	 * short-circuit 1: if geom2 bounding box does not overlap
	 * geom1 bounding box we can prematurely return FALSE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( pglwgeom_getbox2d_p(geom1, &box1) &&
	        pglwgeom_getbox2d_p(geom2, &box2) )
	{
		if ( ( box2.xmax < box1.xmin ) || ( box2.xmin > box1.xmax ) ||
		        ( box2.ymax < box1.ymin ) || ( box2.ymin > box1.ymax ) )
		{
			PG_RETURN_BOOL(FALSE);
		}
	}

	/*
	 * short-circuit 2: if the geoms are a point and a polygon,
	 * call the point_outside_polygon function.
	 */
	type1 = pglwgeom_get_type(geom1);
	type2 = pglwgeom_get_type(geom2);
	if ( (type1 == POINTTYPE && (type2 == POLYGONTYPE || type2 == MULTIPOLYGONTYPE)) ||
	        (type2 == POINTTYPE && (type1 == POLYGONTYPE || type1 == MULTIPOLYGONTYPE)))
	{
		POSTGIS_DEBUG(3, "Point in Polygon test requested...short-circuiting.");

		if ( type1 == POINTTYPE )
		{
			point = lwgeom_as_lwpoint(pglwgeom_deserialize(geom1));
			lwgeom = pglwgeom_deserialize(geom2);
			serialized_poly = geom2;
			polytype = type2;
		}
		else
		{
			point = lwgeom_as_lwpoint(pglwgeom_deserialize(geom2));
			lwgeom = pglwgeom_deserialize(geom1);
			serialized_poly = geom1;
			polytype = type1;
		}
		/*
		 * Switch the context to the function-scope context,
		 * retrieve the appropriate cache object, cache it for
		 * future use, then switch back to the local context.
		 */
		old_context = MemoryContextSwitchTo(fcinfo->flinfo->fn_mcxt);
		poly_cache = retrieveCache(lwgeom, serialized_poly, fcinfo->flinfo->fn_extra);
		fcinfo->flinfo->fn_extra = poly_cache;
		MemoryContextSwitchTo(old_context);

		if ( poly_cache->ringIndices )
		{
			result = point_in_multipolygon_rtree(poly_cache->ringIndices, poly_cache->polyCount, poly_cache->ringCounts, point);
		}
		else if ( polytype == POLYGONTYPE )
		{
			result = point_in_polygon((LWPOLY*)lwgeom, point);
		}
		else if ( polytype == MULTIPOLYGONTYPE )
		{
			result = point_in_multipolygon((LWMPOLY*)lwgeom, point);
		}
		else
		{
			/* Gulp! Should not be here... */
			elog(ERROR,"Type isn't poly or multipoly!");
			PG_RETURN_NULL();
		}

		PG_FREE_IF_COPY(geom1, 0);
		PG_FREE_IF_COPY(geom2, 1);
		lwgeom_release((LWGEOM *)lwgeom);
		lwgeom_release((LWGEOM *)point);
		if ( result != -1 ) /* not outside */
		{
			PG_RETURN_BOOL(TRUE);
		}
		else
		{
			PG_RETURN_BOOL(FALSE);
		}
	}

	initGEOS(lwnotice, lwgeom_geos_error);
#ifdef PREPARED_GEOM
	prep_cache = GetPrepGeomCache( fcinfo, geom1, geom2 );

	if ( prep_cache && prep_cache->prepared_geom )
	{
		if ( prep_cache->argnum == 1 )
		{
			GEOSGeometry *g = (GEOSGeometry *)POSTGIS2GEOS(geom2);
			if ( 0 == g )   /* exception thrown at construction */
			{
				lwerror("Geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
				PG_RETURN_NULL();
			}
			result = GEOSPreparedIntersects( prep_cache->prepared_geom, g);
			GEOSGeom_destroy(g);
		}
		else
		{
			GEOSGeometry *g = (GEOSGeometry *)POSTGIS2GEOS(geom1);
			if ( 0 == g )   /* exception thrown at construction */
			{
				lwerror("Geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
				PG_RETURN_NULL();
			}
			result = GEOSPreparedIntersects( prep_cache->prepared_geom, g);
			GEOSGeom_destroy(g);
		}
	}
	else
#endif
	{
		GEOSGeometry *g1;
		GEOSGeometry *g2;
		g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
		if ( 0 == g1 )   /* exception thrown at construction */
		{
			lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
			PG_RETURN_NULL();
		}
		g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
		if ( 0 == g2 )   /* exception thrown at construction */
		{
			lwerror("Second argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
			GEOSGeom_destroy(g1);
			PG_RETURN_NULL();
		}
		result = GEOSIntersects( g1, g2);
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
	}

	if (result == 2)
	{
		lwerror("GEOSIntersects: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL(); /* never get here */
	}

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(touches);
Datum touches(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeometry *g1, *g2;
	bool result;
	BOX2DFLOAT4 box1, box2;

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	error_if_srid_mismatch(pglwgeom_get_srid(geom1), pglwgeom_get_srid(geom2));

	/* A.Touches(Empty) == FALSE */
	if ( pglwgeom_is_empty(geom1) || pglwgeom_is_empty(geom2) )
		PG_RETURN_BOOL(false);

	/*
	 * short-circuit 1: if geom2 bounding box does not overlap
	 * geom1 bounding box we can prematurely return FALSE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( pglwgeom_getbox2d_p(geom1, &box1) &&
	        pglwgeom_getbox2d_p(geom2, &box2) )
	{
		if ( ( box2.xmax < box1.xmin ) || ( box2.xmin > box1.xmax ) ||
		        ( box2.ymax < box1.ymin ) || ( box2.ymin > box1.ymax ) )
		{
			PG_RETURN_BOOL(FALSE);
		}
	}

	initGEOS(lwnotice, lwgeom_geos_error);

	PROFSTART(PROF_P2G1);
	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1 );
	PROFSTOP(PROF_P2G1);
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}

	PROFSTART(PROF_P2G2);
	g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2 );
	PROFSTOP(PROF_P2G2);
	if ( 0 == g2 )   /* exception thrown at construction */
	{
		lwerror("Second argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		GEOSGeom_destroy(g1);
		PG_RETURN_NULL();
	}

	PROFSTART(PROF_GRUN);
	result = GEOSTouches(g1,g2);
	PROFSTOP(PROF_GRUN);

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (result == 2)
	{
		lwerror("GEOSTouches: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL(); /* never get here */
	}

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, geom2, NULL);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(disjoint);
Datum disjoint(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeometry *g1, *g2;
	bool result;
	BOX2DFLOAT4 box1, box2;

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	error_if_srid_mismatch(pglwgeom_get_srid(geom1), pglwgeom_get_srid(geom2));

	/* A.Disjoint(Empty) == TRUE */
	if ( pglwgeom_is_empty(geom1) || pglwgeom_is_empty(geom2) )
		PG_RETURN_BOOL(true);

	/*
	 * short-circuit 1: if geom2 bounding box does not overlap
	 * geom1 bounding box we can prematurely return TRUE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( pglwgeom_getbox2d_p(geom1, &box1) &&
	        pglwgeom_getbox2d_p(geom2, &box2) )
	{
		if ( ( box2.xmax < box1.xmin ) || ( box2.xmin > box1.xmax ) ||
		        ( box2.ymax < box1.ymin ) || ( box2.ymin > box1.ymax ) )
		{
			PG_RETURN_BOOL(TRUE);
		}
	}

	initGEOS(lwnotice, lwgeom_geos_error);

	PROFSTART(PROF_P2G1);
	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	PROFSTOP(PROF_P2G1);
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}

	PROFSTART(PROF_P2G2);
	g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
	PROFSTOP(PROF_P2G2);
	if ( 0 == g2 )   /* exception thrown at construction */
	{
		lwerror("Second argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		GEOSGeom_destroy(g1);
		PG_RETURN_NULL();
	}

	PROFSTART(PROF_GRUN);
	result = GEOSDisjoint(g1,g2);
	PROFSTOP(PROF_GRUN);

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (result == 2)
	{
		lwerror("GEOSDisjoint: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL(); /* never get here */
	}

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, geom2, NULL);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(relate_pattern);
Datum relate_pattern(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	char *patt;
	bool result;
	GEOSGeometry *g1, *g2;
	int i;

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));


	/* TODO handle empty */

	errorIfGeometryCollection(geom1,geom2);
	error_if_srid_mismatch(pglwgeom_get_srid(geom1), pglwgeom_get_srid(geom2));

	initGEOS(lwnotice, lwgeom_geos_error);

	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}
	g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
	if ( 0 == g2 )   /* exception thrown at construction */
	{
		lwerror("Second argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		GEOSGeom_destroy(g1);
		PG_RETURN_NULL();
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

	if (result == 2)
	{
		lwerror("GEOSRelatePattern: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL(); /* never get here */
	}

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}



PG_FUNCTION_INFO_V1(relate_full);
Datum relate_full(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeometry *g1, *g2;
	char *relate_str;
	text *result;
#if POSTGIS_GEOS_VERSION >= 33
	int bnr = GEOSRELATE_BNR_OGC;
#endif

	POSTGIS_DEBUG(2, "in relate_full()");

	/* TODO handle empty */

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	if ( PG_NARGS() > 2 ) {
#if POSTGIS_GEOS_VERSION >= 33
		bnr = PG_GETARG_INT32(2);
#else
		lwerror("The GEOS version this postgis binary "
			"was compiled against (%d) doesn't support "
			"specifying a boundary node rule with ST_Relate"
			" (3.3.0+ required)",
			POSTGIS_GEOS_VERSION);
		PG_RETURN_NULL();
#endif
	}

	errorIfGeometryCollection(geom1,geom2);
	error_if_srid_mismatch(pglwgeom_get_srid(geom1), pglwgeom_get_srid(geom2));

	initGEOS(lwnotice, lwgeom_geos_error);

	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1 );
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}
	g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2 );
	if ( 0 == g2 )   /* exception thrown at construction */
	{
		lwerror("Second argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		GEOSGeom_destroy(g1);
		PG_RETURN_NULL();
	}

	POSTGIS_DEBUG(3, "constructed geometries ");

	if ((g1==NULL) || (g2 == NULL))
		elog(NOTICE,"g1 or g2 are null");

	POSTGIS_DEBUGF(3, "%s", GEOSGeomToWKT(g1));
	POSTGIS_DEBUGF(3, "%s", GEOSGeomToWKT(g2));

#if POSTGIS_GEOS_VERSION >= 33
	relate_str = GEOSRelateBoundaryNodeRule(g1, g2, bnr);
#else
	relate_str = GEOSRelate(g1, g2);
#endif

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (relate_str == NULL)
	{
		lwerror("GEOSRelate: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL(); /* never get here */
	}

	result = cstring2text(relate_str);
	free(relate_str);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_TEXT_P(result);
}


PG_FUNCTION_INFO_V1(geomequals);
Datum geomequals(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeometry *g1, *g2;
	bool result;
	BOX2DFLOAT4 box1, box2;

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	error_if_srid_mismatch(pglwgeom_get_srid(geom1), pglwgeom_get_srid(geom2));

	/* Different types can't be equal */
	if( pglwgeom_get_type(geom1) != pglwgeom_get_type(geom2) )
		PG_RETURN_BOOL(FALSE);
		
	/* Empty == Empty */
	if ( pglwgeom_is_empty(geom1) && pglwgeom_is_empty(geom2) )
		PG_RETURN_BOOL(TRUE);


	/*
	 * short-circuit: if geom2 bounding box does not equal
	 * geom1 bounding box we can prematurely return FALSE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( pglwgeom_getbox2d_p(geom1, &box1) &&
	        pglwgeom_getbox2d_p(geom2, &box2) )
	{
		if ( box2.xmax != box1.xmax ) PG_RETURN_BOOL(FALSE);
		if ( box2.xmin != box1.xmin ) PG_RETURN_BOOL(FALSE);
		if ( box2.ymax != box1.ymax ) PG_RETURN_BOOL(FALSE);
		if ( box2.ymin != box2.ymin ) PG_RETURN_BOOL(FALSE);
	}

	initGEOS(lwnotice, lwgeom_geos_error);

	PROFSTART(PROF_P2G1);
	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	PROFSTOP(PROF_P2G1);
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}

	PROFSTART(PROF_P2G2);
	g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
	PROFSTOP(PROF_P2G2);
	if ( 0 == g2 )   /* exception thrown at construction */
	{
		lwerror("Second argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		GEOSGeom_destroy(g1);
		PG_RETURN_NULL();
	}

	PROFSTART(PROF_GRUN);
	result = GEOSEquals(g1,g2);
	PROFSTOP(PROF_GRUN);

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (result == 2)
	{
		lwerror("GEOSEquals: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL(); /*never get here */
	}

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, geom2, NULL);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(issimple);
Datum issimple(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	GEOSGeometry *g1;
	int result;

	POSTGIS_DEBUG(2, "issimple called");

	geom = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	if ( pglwgeom_is_empty(geom) )
		PG_RETURN_BOOL(TRUE);

	initGEOS(lwnotice, lwgeom_geos_error);

	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom);
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}
	result = GEOSisSimple(g1);
	GEOSGeom_destroy(g1);

	if (result == 2)
	{
		lwerror("GEOSisSimple: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL(); /*never get here */
	}

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(isring);
Datum isring(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	GEOSGeometry *g1;
	int result;

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	if (pglwgeom_get_type(geom) != LINETYPE)
	{
		elog(ERROR,"isring() should only be called on a LINE");
	}

	/* Empty things can't close */
	if ( pglwgeom_is_empty(geom) )
		PG_RETURN_BOOL(FALSE);

	initGEOS(lwnotice, lwgeom_geos_error);

	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom );
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}
	result = GEOSisRing(g1);
	GEOSGeom_destroy(g1);

	if (result == 2)
	{
		lwerror("GEOSisRing: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}

	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_BOOL(result);
}



/*
**  GEOS <==> PostGIS conversion functions
**
** Default conversion creates a GEOS point array, then iterates through the
** PostGIS points, setting each value in the GEOS array one at a time.
**
*/

/* Return a POINTARRAY from a GEOSCoordSeq */
POINTARRAY *
ptarray_from_GEOSCoordSeq(const GEOSCoordSequence *cs, char want3d)
{
	uint32 dims=2;
	uint32 size, i, ptsize;
	POINTARRAY *pa;
	POINT4D point;

	POSTGIS_DEBUG(2, "ptarray_fromGEOSCoordSeq called");

	if ( ! GEOSCoordSeq_getSize(cs, &size) )
		lwerror("Exception thrown");

	POSTGIS_DEBUGF(4, " GEOSCoordSeq size: %d", size);

	if ( want3d )
	{
		if ( ! GEOSCoordSeq_getDimensions(cs, &dims) )
			lwerror("Exception thrown");

		POSTGIS_DEBUGF(4, " GEOSCoordSeq dimensions: %d", dims);

		/* forget higher dimensions (if any) */
		if ( dims > 3 ) dims = 3;
	}

	POSTGIS_DEBUGF(4, " output dimensions: %d", dims);

	ptsize = sizeof(double)*dims;

	pa = ptarray_construct((dims==3), 0, size);

	for (i=0; i<size; i++)
	{
		GEOSCoordSeq_getX(cs, i, &(point.x));
		GEOSCoordSeq_getY(cs, i, &(point.y));
		if ( dims >= 3 ) GEOSCoordSeq_getZ(cs, i, &(point.z));
		ptarray_set_point4d(pa,i,&point);
	}

	return pa;
}

/* Return an LWGEOM from a Geometry */
LWGEOM *
GEOS2LWGEOM(const GEOSGeometry *geom, char want3d)
{
	int type = GEOSGeomTypeId(geom) ;
	bool hasZ;
	int SRID = GEOSGetSRID(geom);

	/* GEOS's 0 is equivalent to our unknown as for SRID values */
	if ( SRID == 0 ) SRID = SRID_UNKNOWN;

	if ( want3d )
	{
		hasZ = GEOSHasZ(geom);
		if ( ! hasZ )
		{
			POSTGIS_DEBUG(3, "Geometry has no Z, won't provide one");

			want3d = 0;
		}
	}

/*
	if ( GEOSisEmpty(geom) )
	{
		return (LWGEOM*)lwcollection_construct_empty(COLLECTIONTYPE, SRID, want3d, 0);
	}
*/

	switch (type)
	{
		const GEOSCoordSequence *cs;
		POINTARRAY *pa, **ppaa;
		const GEOSGeometry *g;
		LWGEOM **geoms;
		uint32 i, ngeoms;

	case GEOS_POINT:
		POSTGIS_DEBUG(4, "lwgeom_from_geometry: it's a Point");
		cs = GEOSGeom_getCoordSeq(geom);
/* FIXME: dunno how to represent an empty point */
if ( GEOSisEmpty(geom) )
	return (LWGEOM*)lwcollection_construct_empty(COLLECTIONTYPE, SRID, want3d, 0);
		pa = ptarray_from_GEOSCoordSeq(cs, want3d);
		return (LWGEOM *)lwpoint_construct(SRID, NULL, pa);

	case GEOS_LINESTRING:
	case GEOS_LINEARRING:
		POSTGIS_DEBUG(4, "lwgeom_from_geometry: it's a LineString or LinearRing");
/* FIXME: dunno how to represent an empty linestring */
if ( GEOSisEmpty(geom) )
	return (LWGEOM*)lwcollection_construct_empty(COLLECTIONTYPE, SRID, want3d, 0);

		cs = GEOSGeom_getCoordSeq(geom);
		pa = ptarray_from_GEOSCoordSeq(cs, want3d);
		return (LWGEOM *)lwline_construct(SRID, NULL, pa);

	case GEOS_POLYGON:
		POSTGIS_DEBUG(4, "lwgeom_from_geometry: it's a Polygon");

/* FIXME: dunno how to represent an empty polygon */
if ( GEOSisEmpty(geom) )
	return (LWGEOM*)lwcollection_construct_empty(COLLECTIONTYPE, SRID, want3d, 0);
		ngeoms = GEOSGetNumInteriorRings(geom);
		ppaa = lwalloc(sizeof(POINTARRAY *)*(ngeoms+1));
		g = GEOSGetExteriorRing(geom);
		cs = GEOSGeom_getCoordSeq(g);
		ppaa[0] = ptarray_from_GEOSCoordSeq(cs, want3d);
		for (i=0; i<ngeoms; i++)
		{
			g = GEOSGetInteriorRingN(geom, i);
			cs = GEOSGeom_getCoordSeq(g);
			ppaa[i+1] = ptarray_from_GEOSCoordSeq(cs,
			                                      want3d);
		}
		return (LWGEOM *)lwpoly_construct(SRID, NULL,
		                                  ngeoms+1, ppaa);

	case GEOS_MULTIPOINT:
	case GEOS_MULTILINESTRING:
	case GEOS_MULTIPOLYGON:
	case GEOS_GEOMETRYCOLLECTION:
		POSTGIS_DEBUG(4, "lwgeom_from_geometry: it's a Collection or Multi");

		ngeoms = GEOSGetNumGeometries(geom);
		geoms = NULL;
		if ( ngeoms )
		{
			geoms = lwalloc(sizeof(LWGEOM *)*ngeoms);
			for (i=0; i<ngeoms; i++)
			{
				g = GEOSGetGeometryN(geom, i);
				geoms[i] = GEOS2LWGEOM(g, want3d);
			}
		}
		return (LWGEOM *)lwcollection_construct(type,
		                                        SRID, NULL, ngeoms, geoms);

	default:
		lwerror("GEOS2LWGEOM: unknown geometry type: %d", type);
		return NULL;

	}

}


PG_LWGEOM *
GEOS2POSTGIS(GEOSGeom geom, char want3d)
{
	LWGEOM *lwgeom;
	PG_LWGEOM *result;

	lwgeom = GEOS2LWGEOM(geom, want3d);
	if ( ! lwgeom )
	{
		lwerror("GEOS2POSTGIS: GEOS2LWGEOM returned NULL");
		return NULL;
	}

	POSTGIS_DEBUGF(4, "GEOS2POSTGIS: GEOS2LWGEOM returned a %s", lwgeom_summary(lwgeom, 0));

	if ( is_worth_caching_lwgeom_bbox(lwgeom) )
	{
		lwgeom_add_bbox(lwgeom);
	}

	result = pglwgeom_serialize(lwgeom);

	return result;
}

/*-----=POSTGIS2GEOS= */

GEOSCoordSeq ptarray_to_GEOSCoordSeq(POINTARRAY *);
GEOSGeom LWGEOM2GEOS(LWGEOM *lwgeom);

GEOSCoordSeq
ptarray_to_GEOSCoordSeq(POINTARRAY *pa)
{
	uint32 dims = 2;
	uint32 size, i;
	POINT3DZ p;
	GEOSCoordSeq sq;

	if ( FLAGS_GET_Z(pa->flags) ) dims = 3;
	size = pa->npoints;

	sq = GEOSCoordSeq_create(size, dims);
	if ( ! sq ) lwerror("Error creating GEOS Coordinate Sequence");

	for (i=0; i<size; i++)
	{
		getPoint3dz_p(pa, i, &p);

		POSTGIS_DEBUGF(4, "Point: %g,%g,%g", p.x, p.y, p.z);

		GEOSCoordSeq_setX(sq, i, p.x);
		GEOSCoordSeq_setY(sq, i, p.y);
		if ( dims == 3 ) GEOSCoordSeq_setZ(sq, i, p.z);
	}
	return sq;
}

GEOSGeometry *
LWGEOM2GEOS(LWGEOM *lwgeom)
{
	GEOSCoordSeq sq;
	GEOSGeom g, shell, *geoms;
	/*
	LWGEOM *tmp;
	*/
	uint32 ngeoms, i;
	int geostype;
#if POSTGIS_DEBUG_LEVEL >= 4
	char *wkt;
#endif

	POSTGIS_DEBUGF(4, "LWGEOM2GEOS got a %s", lwtype_name(lwgeom->type));

	if (lwgeom_has_arc(lwgeom))
	{
		POSTGIS_DEBUG(3, "LWGEOM2GEOS: arced geometry found.");

		lwerror("Exception in LWGEOM2GEOS: curved geometry not supported.");
		return NULL;
	}

	switch (lwgeom->type)
	{
		LWPOINT *lwp;
		LWPOLY *lwpoly;
		LWLINE *lwl;
		LWCOLLECTION *lwc;

	case POINTTYPE:
		lwp = (LWPOINT *)lwgeom;
		sq = ptarray_to_GEOSCoordSeq(lwp->point);
		g = GEOSGeom_createPoint(sq);
		if ( ! g )
		{
			/* lwnotice("Exception in LWGEOM2GEOS"); */
			return NULL;
		}
		break;
	case LINETYPE:
		lwl = (LWLINE *)lwgeom;
		sq = ptarray_to_GEOSCoordSeq(lwl->points);
		g = GEOSGeom_createLineString(sq);
		if ( ! g )
		{
			/* lwnotice("Exception in LWGEOM2GEOS"); */
			return NULL;
		}
		break;

	case POLYGONTYPE:
		lwpoly = (LWPOLY *)lwgeom;
		sq = ptarray_to_GEOSCoordSeq(lwpoly->rings[0]);
		shell = GEOSGeom_createLinearRing(sq);
		if ( ! shell ) return NULL;
		/*lwerror("LWGEOM2GEOS: exception during polygon shell conversion"); */
		ngeoms = lwpoly->nrings-1;
		geoms = malloc(sizeof(GEOSGeom)*ngeoms);
		for (i=1; i<lwpoly->nrings; ++i)
		{
			sq = ptarray_to_GEOSCoordSeq(lwpoly->rings[i]);
			geoms[i-1] = GEOSGeom_createLinearRing(sq);
			if ( ! geoms[i-1] )
			{
				--i;
				while (i) GEOSGeom_destroy(geoms[--i]);
				free(geoms);
				GEOSGeom_destroy(shell);
				return NULL;
			}
			/*lwerror("LWGEOM2GEOS: exception during polygon hole conversion"); */
		}
		g = GEOSGeom_createPolygon(shell, geoms, ngeoms);
		if ( ! g ) return NULL;
		free(geoms);
		break;
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COLLECTIONTYPE:
		if ( lwgeom->type == MULTIPOINTTYPE )
			geostype = GEOS_MULTIPOINT;
		else if ( lwgeom->type == MULTILINETYPE )
			geostype = GEOS_MULTILINESTRING;
		else if ( lwgeom->type == MULTIPOLYGONTYPE )
			geostype = GEOS_MULTIPOLYGON;
		else
			geostype = GEOS_GEOMETRYCOLLECTION;

		lwc = (LWCOLLECTION *)lwgeom;
		ngeoms = lwc->ngeoms;
		geoms = malloc(sizeof(GEOSGeom)*ngeoms);

		for (i=0; i<ngeoms; ++i)
		{
			GEOSGeometry* g = LWGEOM2GEOS(lwc->geoms[i]);
			if ( ! g )
			{
				while (i) GEOSGeom_destroy(geoms[--i]);
				free(geoms);
				return NULL;
			}
			geoms[i] = g;
		}
		g = GEOSGeom_createCollection(geostype, geoms, ngeoms);
		if ( ! g ) return NULL;
		free(geoms);
		break;

	default:
		lwerror("Unknown geometry type: %d - %s", lwgeom->type, lwtype_name(lwgeom->type));
		return NULL;
	}

	GEOSSetSRID(g, lwgeom->srid);

#if POSTGIS_DEBUG_LEVEL >= 4
	wkt = GEOSGeomToWKT(g);
	POSTGIS_DEBUGF(4, "LWGEOM2GEOS: GEOSGeom: %s", wkt);
	free(wkt);
#endif

	return g;
}

GEOSGeometry *
POSTGIS2GEOS(PG_LWGEOM *pglwgeom)
{
	GEOSGeometry *ret;
	LWGEOM *lwgeom = pglwgeom_deserialize(pglwgeom);
	if ( ! lwgeom )
	{
		lwerror("POSTGIS2GEOS: unable to deserialize input");
		return NULL;
	}
	ret = LWGEOM2GEOS(lwgeom);
	lwgeom_release(lwgeom);
	if ( ! ret )
	{
		/* lwerror("POSTGIS2GEOS conversion failed"); */
		return NULL;
	}
	return ret;
}


PG_FUNCTION_INFO_V1(GEOSnoop);
Datum GEOSnoop(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	GEOSGeometry *geosgeom;
	PG_LWGEOM *lwgeom_result;
#if POSTGIS_DEBUG_LEVEL > 0
	int result;
	LWGEOM_UNPARSER_RESULT lwg_unparser_result;
#endif

	initGEOS(lwnotice, lwgeom_geos_error);

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));


	geosgeom = (GEOSGeometry *)POSTGIS2GEOS(geom);
	if ( ! geosgeom ) PG_RETURN_NULL();

	PROFSTART(PROF_GRUN);
	PROFSTOP(PROF_GRUN);

	lwgeom_result = GEOS2POSTGIS(geosgeom, pglwgeom_has_z(geom));
	GEOSGeom_destroy(geosgeom);


	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(lwgeom_result);
}

PG_FUNCTION_INFO_V1(polygonize_garray);
Datum polygonize_garray(PG_FUNCTION_ARGS)
{
	Datum datum;
	ArrayType *array;
	int is3d = 0;
	uint32 nelems, i;
	PG_LWGEOM *result;
	GEOSGeometry *geos_result;
	const GEOSGeometry **vgeoms;
	int srid=SRID_UNKNOWN;
	size_t offset;
#if POSTGIS_DEBUG_LEVEL >= 3
	static int call=1;
#endif

#if POSTGIS_DEBUG_LEVEL >= 3
	call++;
#endif

	datum = PG_GETARG_DATUM(0);

	/* Null array, null geometry (should be empty?) */
	if ( (Pointer *)datum == NULL ) PG_RETURN_NULL();

	array = DatumGetArrayTypeP(datum);

	nelems = ArrayGetNItems(ARR_NDIM(array), ARR_DIMS(array));

	POSTGIS_DEBUGF(3, "polygonize_garray: number of elements: %d", nelems);

	if ( nelems == 0 ) PG_RETURN_NULL();

	/* Ok, we really need geos now ;) */
	initGEOS(lwnotice, lwgeom_geos_error);

	vgeoms = palloc(sizeof(GEOSGeometry *)*nelems);
	offset = 0;
	for (i=0; i<nelems; i++)
	{
		GEOSGeometry* g;
		PG_LWGEOM *geom = (PG_LWGEOM *)(ARR_DATA_PTR(array)+offset);
		offset += INTALIGN(VARSIZE(geom));

		g = (GEOSGeometry *)POSTGIS2GEOS(geom);
		if ( 0 == g )   /* exception thrown at construction */
		{
			lwerror("Geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
			PG_RETURN_NULL();
		}
		vgeoms[i] = g;
		if ( ! i )
		{
			srid = pglwgeom_get_srid(geom);
		}
		else
		{
			if ( srid != pglwgeom_get_srid(geom) )
			{
				elog(ERROR, "polygonize: operation on mixed SRID geometries");
				PG_RETURN_NULL();
			}
		}
	}

	POSTGIS_DEBUG(3, "polygonize_garray: invoking GEOSpolygonize");

	geos_result = GEOSPolygonize(vgeoms, nelems);

	POSTGIS_DEBUG(3, "polygonize_garray: GEOSpolygonize returned");

	for (i=0; i<nelems; ++i) GEOSGeom_destroy((GEOSGeometry *)vgeoms[i]);
	pfree(vgeoms);

	if ( ! geos_result ) PG_RETURN_NULL();

	GEOSSetSRID(geos_result, srid);
	result = GEOS2POSTGIS(geos_result, is3d);
	GEOSGeom_destroy(geos_result);
	if ( result == NULL )
	{
		elog(ERROR, "GEOS2POSTGIS returned an error");
		PG_RETURN_NULL(); /*never get here */
	}

	/*compressType(result); */

	PG_RETURN_POINTER(result);

}

PG_FUNCTION_INFO_V1(linemerge);
Datum linemerge(PG_FUNCTION_ARGS)
{
	PG_LWGEOM	*geom1;
	GEOSGeometry *g1, *g3;
	PG_LWGEOM *result;

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	initGEOS(lwnotice, lwgeom_geos_error);

	PROFSTART(PROF_P2G1);
	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	PROFSTOP(PROF_P2G1);
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}

	PROFSTART(PROF_GRUN);
	g3 = GEOSLineMerge(g1);
	PROFSTOP(PROF_GRUN);

	if (g3 == NULL)
	{
		elog(ERROR,"GEOS LineMerge() threw an error!");
		GEOSGeom_destroy(g1);
		PG_RETURN_NULL(); /*never get here */
	}


	POSTGIS_DEBUGF(3, "result: %s", GEOSGeomToWKT(g3) ) ;

	GEOSSetSRID(g3, pglwgeom_get_srid(geom1));

	PROFSTART(PROF_G2P);
	result = GEOS2POSTGIS(g3, pglwgeom_has_z(geom1));
	PROFSTOP(PROF_G2P);

	if (result == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g3);
		elog(ERROR,"GEOS LineMerge() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /*never get here */
	}
	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g3);


	/* compressType(result); */

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, NULL, result);

	PG_FREE_IF_COPY(geom1, 0);

	PG_RETURN_POINTER(result);
}

/*
 * Take a geometry and return an areal geometry
 * (Polygon or MultiPolygon).
 * Actually a wrapper around GEOSpolygonize,
 * transforming the resulting collection into
 * a valid polygonzl Geometry.
 */
PG_FUNCTION_INFO_V1(LWGEOM_buildarea);
Datum LWGEOM_buildarea(PG_FUNCTION_ARGS)
{
	int is3d = 0;
	PG_LWGEOM *result;
	PG_LWGEOM *geom;
	GEOSGeometry* geos_in;
	GEOSGeometry* geos_out;
	LWGEOM *lwgeom;
	int srid=SRID_UNKNOWN;
#if POSTGIS_DEBUG_LEVEL >= 3
	static int call=1;
#endif

	geom = (PG_LWGEOM*)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	lwgeom = pglwgeom_deserialize(geom);

	/* Can't build an area from an empty! */
	if ( lwgeom_is_empty(lwgeom) )
	{
		lwgeom_free(lwgeom);
		PG_RETURN_POINTER(geom);
	}

#if POSTGIS_DEBUG_LEVEL >= 3
	call++;
	lwnotice("buildarea called (call %d)", call);
#endif

	srid = pglwgeom_get_srid(geom);
	is3d = pglwgeom_has_z(geom);

	POSTGIS_DEBUGF(3, "LWGEOM_buildarea got geom @ %p", geom);

	initGEOS(lwnotice, lwgeom_geos_error);

	geos_in = LWGEOM2GEOS(lwgeom);
	lwgeom_free(lwgeom);
	
	if ( 0 == geos_in )   /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}
	geos_out = LWGEOM_GEOS_buildArea(geos_in);
	GEOSGeom_destroy(geos_in);

	if ( ! geos_out ) /* exception thrown.. */
	{
		lwerror("LWGEOM_GEOS_buildArea: %s", lwgeom_geos_errmsg);
		PG_RETURN_NULL();
	}

	/* If no geometries are in result collection, return NULL */
	if ( GEOSGetNumGeometries(geos_out) == 0 )
	{
		GEOSGeom_destroy(geos_out);
		PG_RETURN_NULL();
	}

	GEOSSetSRID(geos_out, srid);
	result = GEOS2POSTGIS(geos_out, is3d);
	GEOSGeom_destroy(geos_out);

#if PARANOIA_LEVEL > 0
	if ( result == NULL )
	{
		lwerror("serialization error");
		PG_RETURN_NULL(); /*never get here */
	}

#endif

	PG_RETURN_POINTER(result);

}

