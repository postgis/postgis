/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 *
 * Copyright 2011 Sandro Santilli <strk@keybit.net>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "lwgeom_geos.h"
#include "liblwgeom.h"
#include "liblwgeom_internal.h"
#include "profile.h"

#include <stdlib.h>


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

	LWDEBUG(2, "ptarray_fromGEOSCoordSeq called");

	if ( ! GEOSCoordSeq_getSize(cs, &size) )
		lwerror("Exception thrown");

	LWDEBUGF(4, " GEOSCoordSeq size: %d", size);

	if ( want3d )
	{
		if ( ! GEOSCoordSeq_getDimensions(cs, &dims) )
			lwerror("Exception thrown");

		LWDEBUGF(4, " GEOSCoordSeq dimensions: %d", dims);

		/* forget higher dimensions (if any) */
		if ( dims > 3 ) dims = 3;
	}

	LWDEBUGF(4, " output dimensions: %d", dims);

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
	int hasZ;
	int SRID = GEOSGetSRID(geom);

	/* GEOS's 0 is equivalent to our unknown as for SRID values */
	if ( SRID == 0 ) SRID = SRID_UNKNOWN;

	if ( want3d )
	{
		hasZ = GEOSHasZ(geom);
		if ( ! hasZ )
		{
			LWDEBUG(3, "Geometry has no Z, won't provide one");

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
		LWDEBUG(4, "lwgeom_from_geometry: it's a Point");
		cs = GEOSGeom_getCoordSeq(geom);
		if ( GEOSisEmpty(geom) )
		  return (LWGEOM*)lwpoint_construct_empty(SRID, want3d, 0);
		pa = ptarray_from_GEOSCoordSeq(cs, want3d);
		return (LWGEOM *)lwpoint_construct(SRID, NULL, pa);

	case GEOS_LINESTRING:
	case GEOS_LINEARRING:
		LWDEBUG(4, "lwgeom_from_geometry: it's a LineString or LinearRing");
		if ( GEOSisEmpty(geom) )
		  return (LWGEOM*)lwline_construct_empty(SRID, want3d, 0);

		cs = GEOSGeom_getCoordSeq(geom);
		pa = ptarray_from_GEOSCoordSeq(cs, want3d);
		return (LWGEOM *)lwline_construct(SRID, NULL, pa);

	case GEOS_POLYGON:
		LWDEBUG(4, "lwgeom_from_geometry: it's a Polygon");
		if ( GEOSisEmpty(geom) )
		  return (LWGEOM*)lwpoly_construct_empty(SRID, want3d, 0);
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
		LWDEBUG(4, "lwgeom_from_geometry: it's a Collection or Multi");

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



GEOSCoordSeq ptarray_to_GEOSCoordSeq(const POINTARRAY *);


GEOSCoordSeq
ptarray_to_GEOSCoordSeq(const POINTARRAY *pa)
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

		LWDEBUGF(4, "Point: %g,%g,%g", p.x, p.y, p.z);

    /* Make sure we don't pass any infinite values down into GEOS */
    /* GEOS 3.3+ is supposed to  handle this stuff OK */
#if POSTGIS_GEOS_VERSION < 33
    if ( isinf(p.x) || isinf(p.y) || (dims == 3 && isinf(p.z)) )
      lwerror("Infinite coordinate value found in geometry.");
#endif

		GEOSCoordSeq_setX(sq, i, p.x);
		GEOSCoordSeq_setY(sq, i, p.y);
		if ( dims == 3 ) GEOSCoordSeq_setZ(sq, i, p.z);
	}
	return sq;
}




GEOSGeometry *
LWGEOM2GEOS(const LWGEOM *lwgeom)
{
	GEOSCoordSeq sq;
	GEOSGeom g, shell;
	GEOSGeom *geoms = NULL;
	/*
	LWGEOM *tmp;
	*/
	uint32 ngeoms, i;
	int geostype;
#if LWDEBUG_LEVEL >= 4
	char *wkt;
#endif

	LWDEBUGF(4, "LWGEOM2GEOS got a %s", lwtype_name(lwgeom->type));

	if (lwgeom_has_arc(lwgeom))
	{
		LWDEBUG(3, "LWGEOM2GEOS: arced geometry found.");

		lwerror("Exception in LWGEOM2GEOS: curved geometry not supported.");
		return NULL;
	}
	
	switch (lwgeom->type)
	{
		LWPOINT *lwp = NULL;
		LWPOLY *lwpoly = NULL;
		LWLINE *lwl = NULL;
		LWCOLLECTION *lwc = NULL;
#if POSTGIS_GEOS_VERSION < 33
		POINTARRAY *pa = NULL;
#endif
		
	case POINTTYPE:
		lwp = (LWPOINT *)lwgeom;
		
		if ( lwgeom_is_empty(lwgeom) )
		{
#if POSTGIS_GEOS_VERSION < 33
			pa = ptarray_construct_empty(lwgeom_has_z(lwgeom), lwgeom_has_m(lwgeom), 2);
			sq = ptarray_to_GEOSCoordSeq(pa);
			shell = GEOSGeom_createLinearRing(sq);
			g = GEOSGeom_createPolygon(shell, NULL, 0);
#else
			g = GEOSGeom_createEmptyPolygon();
#endif
		}
		else
		{
			sq = ptarray_to_GEOSCoordSeq(lwp->point);
			g = GEOSGeom_createPoint(sq);
		}
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
		if ( lwgeom_is_empty(lwgeom) )
		{
#if POSTGIS_GEOS_VERSION < 33
			POINTARRAY *pa = ptarray_construct_empty(lwgeom_has_z(lwgeom), lwgeom_has_m(lwgeom), 2);
			sq = ptarray_to_GEOSCoordSeq(pa);
			shell = GEOSGeom_createLinearRing(sq);
			g = GEOSGeom_createPolygon(shell, NULL, 0);
#else
			g = GEOSGeom_createEmptyPolygon();
#endif
		}
		else
		{
			sq = ptarray_to_GEOSCoordSeq(lwpoly->rings[0]);
			shell = GEOSGeom_createLinearRing(sq);
			if ( ! shell ) return NULL;
			/*lwerror("LWGEOM2GEOS: exception during polygon shell conversion"); */
			ngeoms = lwpoly->nrings-1;
			if ( ngeoms > 0 )
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
			if (geoms) free(geoms);
		}
		if ( ! g ) return NULL;
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
		if ( ngeoms > 0 )
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
		if ( geoms ) free(geoms);
		if ( ! g ) return NULL;
		break;

	default:
		lwerror("Unknown geometry type: %d - %s", lwgeom->type, lwtype_name(lwgeom->type));
		return NULL;
	}

	GEOSSetSRID(g, lwgeom->srid);

#if LWDEBUG_LEVEL >= 4
	wkt = GEOSGeomToWKT(g);
	LWDEBUGF(4, "LWGEOM2GEOS: GEOSGeom: %s", wkt);
	free(wkt);
#endif

	return g;
}




LWGEOM *
lwgeom_intersection(const LWGEOM *geom1, const LWGEOM *geom2)
{
	LWGEOM *result ;
	GEOSGeometry *g1, *g2, *g3 ;
	int is3d ;
	int srid ;

	PROFSTART(PROF_QRUN);

	/* A.Intersection(Empty) == Empty */
	if ( lwgeom_is_empty(geom2) )
		return lwgeom_clone(geom2);

	/* Empty.Intersection(A) == Empty */
	if ( lwgeom_is_empty(geom1) )
		return lwgeom_clone(geom1);

	/* ensure srids are identical */
	srid = (int)(geom1->srid);
	error_if_srid_mismatch(srid, (int)(geom2->srid));

	is3d = (FLAGS_GET_Z(geom1->flags) || FLAGS_GET_Z(geom2->flags)) ;

	initGEOS(lwnotice, lwgeom_geos_error);

	LWDEBUG(3, "intersection() START");

	PROFSTART(PROF_P2G1);
	g1 = LWGEOM2GEOS(geom1);
	PROFSTOP(PROF_P2G1);
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS.");
		return NULL ;
	}

	PROFSTART(PROF_P2G2);
	g2 = LWGEOM2GEOS(geom2);
	PROFSTOP(PROF_P2G2);
	if ( 0 == g2 )   /* exception thrown at construction */
	{
		lwerror("Second argument geometry could not be converted to GEOS.");
		GEOSGeom_destroy(g1);
		return NULL ;
	}

	LWDEBUG(3, " constructed geometrys - calling geos");
	LWDEBUGF(3, " g1 = %s", GEOSGeomToWKT(g1));
	LWDEBUGF(3, " g2 = %s", GEOSGeomToWKT(g2));
	/*LWDEBUGF(3, "g2 is valid = %i",GEOSisvalid(g2)); */
	/*LWDEBUGF(3, "g1 is valid = %i",GEOSisvalid(g1)); */

	PROFSTART(PROF_GRUN);
	g3 = GEOSIntersection(g1,g2);
	PROFSTOP(PROF_GRUN);

	LWDEBUG(3, " intersection finished");

	if (g3 == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		lwerror("Error performing intersection.");
		return NULL; /* never get here */
	}

	LWDEBUGF(3, "result: %s", GEOSGeomToWKT(g3) ) ;

	GEOSSetSRID(g3, srid);

	PROFSTART(PROF_G2P);
	result = GEOS2LWGEOM(g3, is3d);
	PROFSTOP(PROF_G2P);

	if (result == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		GEOSGeom_destroy(g3);
		lwerror("GEOS Intersection() threw an error (result postgis geometry formation)!");
		return NULL ; /* never get here */
	}

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	GEOSGeom_destroy(g3);

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, geom2, result);

	return result ;
}

LWGEOM *
lwgeom_difference(const LWGEOM *geom1, const LWGEOM *geom2)
{
	GEOSGeometry *g1, *g2, *g3;
	LWGEOM *result;
	int is3d;
	int srid;

	PROFSTART(PROF_QRUN);

	/* A.Difference(Empty) == A */
	if ( lwgeom_is_empty(geom2) )
		return lwgeom_clone(geom1);

	/* Empty.Intersection(A) == Empty */
	if ( lwgeom_is_empty(geom1) )
		return lwgeom_clone(geom1);

	/* ensure srids are identical */
	srid = (int)(geom1->srid);
	error_if_srid_mismatch(srid, (int)(geom2->srid));

	is3d = (FLAGS_GET_Z(geom1->flags) || FLAGS_GET_Z(geom2->flags)) ;

	initGEOS(lwnotice, lwgeom_geos_error);

	PROFSTART(PROF_P2G1);
	g1 = LWGEOM2GEOS(geom1);
	PROFSTOP(PROF_P2G1);
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	PROFSTART(PROF_P2G2);
	g2 = LWGEOM2GEOS(geom2);
	PROFSTOP(PROF_P2G2);
	if ( 0 == g2 )   /* exception thrown at construction */
	{
		GEOSGeom_destroy(g1);
		lwerror("Second argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	PROFSTART(PROF_GRUN);
	g3 = GEOSDifference(g1,g2);
	PROFSTOP(PROF_GRUN);

	if (g3 == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		lwerror("GEOSDifference: %s", lwgeom_geos_errmsg);
		return NULL ; /* never get here */
	}

	LWDEBUGF(3, "result: %s", GEOSGeomToWKT(g3) ) ;

	GEOSSetSRID(g3, srid);

	PROFSTART(PROF_G2P);
	result = GEOS2LWGEOM(g3, is3d);
	PROFSTOP(PROF_G2P);

	if (result == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		GEOSGeom_destroy(g3);
		lwerror("GEOS difference() threw an error (result postgis geometry formation)!");
		return NULL; /* never get here */
	}

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	GEOSGeom_destroy(g3);

	/* compressType(result); */

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, geom2, result);

	return result;
}

LWGEOM *
lwgeom_symdifference(const LWGEOM* geom1, const LWGEOM* geom2)
{
	GEOSGeometry *g1, *g2, *g3;
	LWGEOM *result;
	int is3d;
	int srid;

	PROFSTART(PROF_QRUN);

	/* A.SymDifference(Empty) == A */
	if ( lwgeom_is_empty(geom2) )
		return lwgeom_clone(geom1);

	/* Empty.DymDifference(B) == Empty */
	if ( lwgeom_is_empty(geom1) )
		return lwgeom_clone(geom1);

	/* ensure srids are identical */
	srid = (int)(geom1->srid);
	error_if_srid_mismatch(srid, (int)(geom2->srid));

	is3d = (FLAGS_GET_Z(geom1->flags) || FLAGS_GET_Z(geom2->flags)) ;

	initGEOS(lwnotice, lwgeom_geos_error);

	PROFSTART(PROF_P2G1);
	g1 = LWGEOM2GEOS(geom1);
	PROFSTOP(PROF_P2G1);
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	PROFSTART(PROF_P2G2);
	g2 = LWGEOM2GEOS(geom2);
	PROFSTOP(PROF_P2G2);
	if ( 0 == g2 )   /* exception thrown at construction */
	{
		lwerror("Second argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		GEOSGeom_destroy(g1);
		return NULL;
	}

	PROFSTART(PROF_GRUN);
	g3 = GEOSSymDifference(g1,g2);
	PROFSTOP(PROF_GRUN);

	if (g3 == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		lwerror("GEOSSymDifference: %s", lwgeom_geos_errmsg);
		return NULL; /*never get here */
	}

	LWDEBUGF(3, "result: %s", GEOSGeomToWKT(g3));

	GEOSSetSRID(g3, srid);

	PROFSTART(PROF_G2P);
	result = GEOS2LWGEOM(g3, is3d);
	PROFSTOP(PROF_G2P);

	if (result == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		GEOSGeom_destroy(g3);
		lwerror("GEOS symdifference() threw an error (result postgis geometry formation)!");
		return NULL ; /*never get here */
	}

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	GEOSGeom_destroy(g3);

	/* compressType(result); */

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, geom2, result);

	return result;
}

LWGEOM*
lwgeom_union(const LWGEOM *geom1, const LWGEOM *geom2)
{
	int is3d;
	int srid;
	GEOSGeometry *g1, *g2, *g3;
	LWGEOM *result;

	LWDEBUG(2, "in geomunion");

	PROFSTART(PROF_QRUN);

	/* A.Union(empty) == A */
	if ( lwgeom_is_empty(geom1) )
		return lwgeom_clone(geom2);

	/* B.Union(empty) == B */
	if ( lwgeom_is_empty(geom2) )
		return lwgeom_clone(geom1);


	/* ensure srids are identical */
	srid = (int)(geom1->srid);
	error_if_srid_mismatch(srid, (int)(geom2->srid));

	is3d = (FLAGS_GET_Z(geom1->flags) || FLAGS_GET_Z(geom2->flags)) ;

	initGEOS(lwnotice, lwgeom_geos_error);

	PROFSTART(PROF_P2G1);
	g1 = LWGEOM2GEOS(geom1);
	PROFSTOP(PROF_P2G1);
	if ( 0 == g1 )   /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	PROFSTART(PROF_P2G2);
	g2 = LWGEOM2GEOS(geom2);
	PROFSTOP(PROF_P2G2);
	if ( 0 == g2 )   /* exception thrown at construction */
	{
		GEOSGeom_destroy(g1);
		lwerror("Second argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	LWDEBUGF(3, "g1=%s", GEOSGeomToWKT(g1));
	LWDEBUGF(3, "g2=%s", GEOSGeomToWKT(g2));

	PROFSTART(PROF_GRUN);
	g3 = GEOSUnion(g1,g2);
	PROFSTOP(PROF_GRUN);

	LWDEBUGF(3, "g3=%s", GEOSGeomToWKT(g3));

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (g3 == NULL)
	{
		lwerror("GEOSUnion: %s", lwgeom_geos_errmsg);
		return NULL; /* never get here */
	}


	GEOSSetSRID(g3, srid);

	PROFSTART(PROF_G2P);
	result = GEOS2LWGEOM(g3, is3d);
	PROFSTOP(PROF_G2P);

	GEOSGeom_destroy(g3);

	if (result == NULL)
	{
		lwerror("GEOS union() threw an error (result postgis geometry formation)!");
		return NULL; /*never get here */
	}

	/* compressType(result); */

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, geom2, result);

	return result;
}


GEOSGeometry*
LWGEOM_GEOS_buildArea(const GEOSGeometry* geom_in)
{
	GEOSGeometry *tmp;
	GEOSGeometry *geos_result, *shp;
	GEOSGeometry const *vgeoms[1];
	uint32 i, ngeoms;
	int srid = GEOSGetSRID(geom_in);

	vgeoms[0] = geom_in;
	geos_result = GEOSPolygonize(vgeoms, 1);

	LWDEBUGF(3, "GEOSpolygonize returned @ %p", geos_result);

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

	LWDEBUGF(3, "GEOSpolygonize: ngeoms in polygonize output: %d", ngeoms);
	LWDEBUGF(3, "GEOSpolygonize: polygonized:%s",
	               lwgeom_to_ewkt(GEOS2LWGEOM(geos_result, 0), PARSER_CHECK_NONE));

	/*
	 * No geometries in collection, early out
	 */
	if ( ngeoms == 0 )
	{
		GEOSSetSRID(geos_result, srid);
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
		GEOSSetSRID(shp, srid);
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
			LWDEBUGF(3, "GEOSpolygonize: shp:%s",
			               lwgeom_to_ewkt(GEOS2LWGEOM(shp, 0), PARSER_CHECK_NONE));
		}
		else
		{
			tmp = GEOSSymDifference(shp, extring);
			LWDEBUGF(3, "GEOSpolygonize: SymDifference(%s, %s):%s",
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

	GEOSSetSRID(shp, srid);

	return shp;
}

LWGEOM*
lwgeom_buildarea(const LWGEOM *geom)
{
	GEOSGeometry* geos_in;
	GEOSGeometry* geos_out;
	LWGEOM* geom_out;
	int SRID = (int)(geom->srid);
	int is3d = FLAGS_GET_Z(geom->flags);

	/* Can't build an area from an empty! */
	if ( lwgeom_is_empty(geom) )
	{
		return (LWGEOM*)lwpoly_construct_empty(SRID, is3d, 0);
	}

	LWDEBUG(3, "buildarea called");

	LWDEBUGF(3, "LWGEOM_buildarea got geom @ %p", geom);

	initGEOS(lwnotice, lwgeom_geos_error);

	geos_in = LWGEOM2GEOS(geom);
	
	if ( 0 == geos_in )   /* exception thrown at construction */
	{
		lwerror("First argument geometry could not be converted to GEOS: %s", lwgeom_geos_errmsg);
		return NULL;
	}
	geos_out = LWGEOM_GEOS_buildArea(geos_in);
	GEOSGeom_destroy(geos_in);

	if ( ! geos_out ) /* exception thrown.. */
	{
		lwerror("LWGEOM_GEOS_buildArea: %s", lwgeom_geos_errmsg);
		return NULL;
	}

	/* If no geometries are in result collection, return NULL */
	if ( GEOSGetNumGeometries(geos_out) == 0 )
	{
		GEOSGeom_destroy(geos_out);
		return NULL;
	}

	geom_out = GEOS2LWGEOM(geos_out, is3d);
	GEOSGeom_destroy(geos_out);

#if PARANOIA_LEVEL > 0
	if ( geom_out == NULL )
	{
		lwerror("serialization error");
		return NULL;
	}

#endif

	return geom_out;
}

LWGEOM*
lwgeom_geos_noop(const LWGEOM* geom_in)
{
	GEOSGeometry *geosgeom;
	LWGEOM* geom_out;

	int is3d = FLAGS_GET_Z(geom_in->flags);

	initGEOS(lwnotice, lwgeom_geos_error);
	geosgeom = LWGEOM2GEOS(geom_in);
	if ( ! geosgeom ) {
		lwerror("Geometry could not be converted to GEOS: %s",
			lwgeom_geos_errmsg);
		return NULL;
	}
	geom_out = GEOS2LWGEOM(geosgeom, is3d);
	GEOSGeom_destroy(geosgeom);
	if ( ! geom_out ) {
		lwerror("GEOS Geometry could not be converted to LWGEOM: %s",
			lwgeom_geos_errmsg);
	}
	return geom_out;
	
}
